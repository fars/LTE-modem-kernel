/*
 * Copyright (c) 2007-2011 Motorola, Inc, All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Changelog:
 * Date            Author          Comment
 * -----------------------------------------------------------------------------
 * 05/25/2011      Motorola        Need to add logging to track IPC ctrl msg round trip time
 * 04/21/2010	   Motorola        Add a synchronization between AP and BP to start IPC link
 * 09/25/2009      Motorola        Free mailbox 2 and 3 (they will be used in the future for 4G HIF)
 * 07/15/2009      Motorola        Use sDMA for endianess conversion (excepted for IPC header)
 * 07/16/2009      Motorola        Add support for short message handling
 * 06/04/2009      Motorola        Add bp panic through mailboxes
 * 06/02/2009      Motorola        Fix concurrent access to data receiver mailbox
 * 05/22/2009      Motorola        Add bp log through mailboxes / shared memory
 * 07/05/2009      Motorola        Add IPC checksum
 * 03/06/2009      Motorola        Use mailboxes instead of USB
 * 12/07/2007      Motorola        USB-IPC initial
 * 03/22/2007      Motorola        USB-IPC header support
 * 07/09/2007      Motorola        Change multi-node sending for Phase 1
 * 11/03/2007      Motorola        Support sequence number
 * 06/10/2010      Motorola        Remove netmux wait queue
 * 06/25/2010      Motorola        4G HIF IPC low level driver on AP side - phase 2 (group frames)- Update test application
 * 08/31/2010      Motorola        Fix Klockwork defects
 */

/*!
 * @file drivers/usb/ipchost/ipc_api.c
 * @brief USB-IPC Descriptor Set
 *
 * This is the generic portion of the USB-IPC driver.
 *
 * @ingroup IPCFunction
 */

/*=============================================================================
  INCLUDE FILES
==============================================================================*/
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/kdev_t.h>
#include <linux/ktime.h>
#include <linux/major.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/circ_buf.h>
#include <linux/uio.h>
#include <linux/poll.h>
#include <linux/usb_ipc.h>
#include <linux/ipc_api.h>
#include <linux/ipc_vr_shortmsg.h>
#include <linux/hw_mbox.h>
#include <linux/ipc_4ghif.h>
#include <mach/io.h>
#include <asm/mach/map.h>

/*=============================================================================
  LOCAL MACROS
==============================================================================*/
/*#define IPC_DEBUG*/

#ifdef IPC_DEBUG
#define DEBUG(args...) printk(args)
#define DEBUG_DLOG(args...) printk(args)
#define DEBUG_PANIC_CMD(args...) printk(args)
#define DEBUG_PANIC_RSP_LIGHT(args...) printk(args)
#define DEBUG_PANIC_RSP_HEAVY(args...) printk(args)
#else
#define DEBUG(args...)
#define DEBUG_DLOG(args...)
#define DEBUG_PANIC_CMD(args...)
#define DEBUG_PANIC_RSP_LIGHT(args...)
#define DEBUG_PANIC_RSP_HEAVY(args...)
#endif
#define ENTER_FUNC() DEBUG("Enter %s\n", __FUNCTION__)

#define HDR_FRAME_NB_ID 2
#define HDR_FIXED_WORD  8
#define HDR_CHECKSUM_ID 6
#define HDR_SHIFT       8

#if defined(USE_IPC_FRAME_HEADER) && defined(IPC_USE_TEMP_BUFFER)
Error:  Can not define both USE_IPC_FRAME_HEADER and IPC_USE_TEMP_BUFFER
#endif

#define IPC_USB_WRITE_ERROR(channel, status)            \
    DEBUG("%s: Submit URB error\n", __FUNCTION__);      \
    channel.write_flag = 0;                             \
    channel.write_ptr.end_flag = 0;                     \
    channel.cfg.notify_callback(&status);               \
    SEM_UNLOCK(&channel.write_ptr.write_mutex);

/*
 * The following macro is called if submit call USB read error
 */
#define IPC_USB_READ_ERROR(channel, status)             \
    DEBUG("%s: Submit URB error\n", __FUNCTION__);      \
    channel.read_flag = 0;                              \
    channel.cfg.notify_callback(&status);               \
    SEM_UNLOCK(&channel.read_ptr.read_mutex);           \
    return;
/*============================================================================
  LOCAL FUNCTION PROTOTYPES
==============================================================================*/
static void mailbox_shortmsg_callback(void);
static void (* mail_box6_irq_callback)(void) = mailbox_shortmsg_callback;
static void mailbox_short_msg_panic_proc(enum ipc_vr_short_msg msg_type, u16 data);
static void mailbox_panic_read_callback(void);


/*=============================================================================
  LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
===============================================================================*/
struct ipc_vr_short_msg_handler
{
    void (* handler_fcn_p)(enum ipc_vr_short_msg msg_type, u16 data);
};

/*==================================================================================================
  LOCAL CONSTANTS
==================================================================================================*/
const struct ipc_vr_short_msg_handler ipc_vr_short_msg_handler_tb[IPC_VR_SHORT_MSG_LAST] =
{
    { mailbox_short_msg_panic_proc },   /* HW_CTRL_SHORT_MSG_PANIC - handle BP panic. */
    { mailbox_short_msg_vr_write_proc }
};


/* This function is called when a new BP log chunk is available */
#ifdef CONFIG_MACH_W3G_LTE_DATACARD
#define IPC_LOG_SHARED_MEMORY_PHYS 0x8089C400
#define IPC_LOG_SHARED_MEMORY_VIRT 0xD839C400
#else
#define IPC_LOG_SHARED_MEMORY_PHYS 0x8079C400
#define IPC_LOG_SHARED_MEMORY_VIRT 0xD839C400
#endif

/*============================================================================
  LOCAL VARIABLES
==============================================================================*/

/* IPC API channels, define IPC APIs supported channels */
USB_IPC_API_PARAMS               usb_ipc_channels[MAX_USB_IPC_CHANNELS];
unsigned char   sent_sequence_number;
unsigned char   recv_sequence_number;

/* useful variables to debug IPC */
extern ipc_4ghif_stats the_ipc_4ghif_stats;

/* needed to mux DATA/ACK in mailbox 0 */
u8 en_read_msg=0;
u8 rx_read_msg=0;
u32 read_msg=0;
static spinlock_t ipc_mbox_rd_lock=SPIN_LOCK_UNLOCKED;

/* critical section to avoid writing at the same moment on mailbox 1 */
static spinlock_t ipc_mbox_wr_lock=SPIN_LOCK_UNLOCKED;

/* for debug */
union ktime kt_dl_rcv;
/*===========================================================================
  GLOBAL VARIABLES
=============================================================================*/
#ifdef USE_OMAP_SDMA
//#define USB_DMA_COHERENT_ALLOC_FREE
extern void ipc_dma_memcpy_buf2node(USB_IPC_API_PARAMS *ipc_ch);
extern int ipc_dma_memcpy_node2buf(USB_IPC_API_PARAMS *ipc_ch,  HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *ctrl_ptr);
#endif

/* For IPC sync*/
extern void LDInit(void);
u32 bp_ipc_ready = 0;

#define IPC_USB_WRITE_ERROR(channel, status)                   \
       DEBUG("%s: Submit URB error\n", __FUNCTION__);          \
       channel.write_flag = 0;                                 \
       channel.write_ptr.end_flag = 0;                         \
       channel.cfg.notify_callback(&status);                   \
       SEM_UNLOCK(&channel.write_ptr.write_mutex);

/*===========================================================================
  LOCAL FUNCTIONS
=============================================================================*/
static void mailbox_short_msg_panic_proc(enum ipc_vr_short_msg msg_type, u16 data)
{
    /* Let the virtual register code know that the BP has panicked. */
    mailbox_vr_bp_panicked();
    /* Replace the mailbox 6 handler with panic callback. */
    mail_box6_irq_callback = mailbox_panic_read_callback;
}


/* This function is called each time BP panic engine sends a msg (4 bytes) of a panic packet */
/* For each panic packet:                                                                    */
/* - first msg is the packet header 0xABCDLLLL (with L=LEN in bytes)                         */
/* - following msg are the panic data (4bytes by 4 bytes).                                   */
/* (Padding is handled by driver thanks LEN field of header)                                 */
static void mailbox_panic_read_callback(void)
{
    u32 msg = 0;

    /* read the message, for flow control, when mailbox FIFO is full bp wait before writing a new msg */
    HW_MBOX_MsgRead((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_6, &msg);

    /* clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_6,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);

    /* display receive message according debug level needed */
    DEBUG_PANIC_RSP_HEAVY("DPANIC - Rcv 0x%08X\n",msg); // heavy read debug (beware genrate watchdog timeout bp side for big packets)
    DEBUG_PANIC_RSP_LIGHT("+");                         // light read debug

    /* send the message to the driver */
    mb_ipc_panic_copy_to_drv_buf(msg);
}

static void trace_dl_rcv(void)
{
    kt_dl_rcv = ktime_get();
}

static void trace_dl_ack(void)
{
    union ktime kt_dl_ack,kt_dl_delta;
    u32 rcv2ack_sec;

    kt_dl_ack = ktime_get();
    kt_dl_delta = ktime_sub(kt_dl_ack,kt_dl_rcv);

    rcv2ack_sec = (u32)kt_dl_delta.tv.sec;
    if (rcv2ack_sec >= DL_RCV2ACK_TIME_ARRAY_SIZE) rcv2ack_sec = DL_RCV2ACK_TIME_ARRAY_SIZE-1;

    (the_ipc_4ghif_stats.dl_rcv2ack_time[rcv2ack_sec])++;

}

/*==================================================================================================
                                       GLOBAL FUNCTIONS
==================================================================================================*/
/*
 * this function will be called by USB URB OUT callback ...
 */
void usb_ipc_api_write_callback(USB_IPC_CHANNEL_INDEX ch_index, int trans_flag, int trans_size)
{
    HW_CTRL_IPC_WRITE_STATUS_T  ipc_status;
    HW_CTRL_IPC_NOTIFY_STATUS_T ipc_notify_status;

    ENTER_FUNC();
    DEBUG("\n%s: trans_size = %d\n", __FUNCTION__, trans_size);

    ipc_notify_status.channel = &usb_ipc_channels[ch_index].ch;
    ipc_notify_status.status  = HW_CTRL_IPC_STATUS_ERROR;

    /* sent_sequence_number will increase 1 on every read operation, and modulo 256 by using wrap around on a unsigned char.*/
    sent_sequence_number++;

    if (usb_ipc_channels[ch_index].write_ptr.total_num > trans_size)
        usb_ipc_channels[ch_index].write_ptr.total_num = trans_size;

    /* init write callback argument */
    ipc_status.channel  = &usb_ipc_channels[ch_index].ch;

    /* all transmitted is done */
    if(usb_ipc_channels[ch_index].write_ptr.end_flag) {
        usb_ipc_channels[ch_index].write_flag = 0;
        usb_ipc_channels[ch_index].write_ptr.end_flag = 0;

        ipc_status.nb_bytes =  usb_ipc_channels[ch_index].write_ptr.total_num;
        if(usb_ipc_channels[ch_index].cfg.write_callback != NULL)  {
            usb_ipc_channels[ch_index].cfg.write_callback(&ipc_status);
        } else {
            SEM_UNLOCK(&usb_ipc_channels[ch_index].write_ptr.write_mutex);
        }
        return;

#ifdef USE_IPC_FRAME_HEADER
#ifdef IPC_USE_ZERO_LEN_FRAME
        /* send extra zero packet */
        usb_ipc_channels[ch_index].write_ptr.end_flag = 1;

#endif  /* IPC_USE_ZERO_LEN_FRAME */
#endif //
    }
}

/*
 * The following macro is called if submit call USB read error
 */
#define IPC_USB_READ_ERROR(channel, status)                    \
       DEBUG("%s: Submit URB error\n", __FUNCTION__);          \
       channel.read_flag = 0;                                  \
       channel.cfg.notify_callback(&status);                   \
       SEM_UNLOCK(&channel.read_ptr.read_mutex);               \
       return;


/* these endianness conversion functions are used for IPC headers (not transferred using the DMA)*/
/*endianess conversion in a 16 bits word*/
void usb_ipc_exchange_endian16(u16 *data)
{
    unsigned char ch;
    ch = ((unsigned char *)data)[0];
    ((unsigned char *)data)[0] = ((unsigned char *)data)[1];
    ((unsigned char *)data)[1] = ch;
}
/*endianess conversion in a 32 bits word*/
void usb_ipc_exchange_endian32(unsigned long *data)
{
        unsigned long ul_temp32_r[1];
        unsigned long ul_temp32_w[1];
        /*use cached temp word to decrease write access time*/
        ul_temp32_r[0] = data[0];

        ((unsigned char *) ul_temp32_w)[0] = ((unsigned char *) ul_temp32_r)[3];
        ((unsigned char *) ul_temp32_w)[3] = ((unsigned char *) ul_temp32_r)[0];
        ((unsigned char *) ul_temp32_w)[1] = ((unsigned char *) ul_temp32_r)[2];
        ((unsigned char *) ul_temp32_w)[2] = ((unsigned char *) ul_temp32_r)[1];

        /* copy the temp word into shared mem (non cached)*/
        data[0] = ul_temp32_w[0];
}

/*swap the 2 shorts in a 32 bits word*/
void usb_ipc_exchange_endian32_short(unsigned long *data)
{
        unsigned short sh;
        sh = ((unsigned short *)data)[0];
        ((unsigned short *)data)[0] = ((unsigned short *)data)[1];
        ((unsigned short *)data)[1] = sh;
}


/* change header endianess*/
void ipc_header_exchange_endian32(unsigned long * ptr_buf,unsigned short us_FrameNb)
{
 /*
   | Version_H |Version_L |N_Frames_H | N_Frames_L| -->  |N_Frames_H | N_Frames_L| Version_H |Version_L|
        8b          8b          8b         8b                   8b          8b          8b         8b

  | SeqNumber| Options |CheckSum_H |CheckSum_L |   -->   |CheckSum_H |CheckSum_L | SeqNumber(*)|  Options(*) |
      8b          8b          8b         8b                   8b          8b          8b         8b

      For N_Frames:
  |Frame1Len_H|Frame1Len_L |Frame2Len_H|Frame2Len_L| -->|Frame2Len_H|Frame2Len_L|Frame1Len_H|Frame1Len_L |
      8b          8b          8b         8b                   8b          8b          8b         8b

  |FrameNLen_H|FrameNLen_L |   DATA_1  |   DATA_2 | -->  | FrameNLen_H|FrameNLen_L |  DATA_1  |   DATA_2 | (no change for odd number of frames)
      8b          8b          8b         8b                   8b          8b          8b         8b

(*) SeqNumber and options are swapped separately

*/
    int i;

    /*process version and frame nb*/
	usb_ipc_exchange_endian32_short(&ptr_buf[0]);

    /*process checksum, options and sequence nb*/
	usb_ipc_exchange_endian32_short(&ptr_buf[1]);

	/*process frame lengths excepted the last one when there is an odd frame number*/
	for (i=0;i<(((us_FrameNb+1)/2)-us_FrameNb%2);i++)
	{
	   usb_ipc_exchange_endian32_short(&ptr_buf[2+i]);
	}

}

#ifdef CONFIG_MAILBOX_IPC_USE_CHECKSUM
/*
  This function adds a checksum to protect an IPC message (BP-> AP)
  Call this function before inverssing the buffer endianess !
*/
void ipc_buffer_add_checksum(unsigned long * ptr_buf)
{
    /*
      Buf structure:                               bytes # in big endian

      IPC_VERSION (16 bits)  FRAME NB (16 bits)    0.1.2.3
      FLAGS (16 bits)        CHECKSUM (16 bits)    4.5.6.7

      LEN  FRAME 1  (16 bits)  if frame 1 exists   8...
      LEN  FRAME 2  (16 bits)  if frame 2 exists
      ...
      LEN  FRAME 10 (16 bits)  if frame 10 exists

      DATA FRAME 1 (LEN1 bytes) if frame 1 exists
      DATA FRAME 2 (LEN2 bytes) if frame 2 exists
      ...
      DATA FRAME 10(LEN10 bytes) if frame 10 exists
    */

    unsigned char  *tx_buf;
    unsigned char  *tx_buf_base;
    unsigned short check_sum;
    unsigned short nb_frame;
    unsigned short len;
    unsigned short i,j;

    tx_buf_base=(unsigned char *)ptr_buf;
    tx_buf=tx_buf_base;
    check_sum=0;

    nb_frame=tx_buf_base[HDR_FRAME_NB_ID]+(tx_buf_base[HDR_FRAME_NB_ID+1]<<HDR_SHIFT);

    DEBUG("CHECKSUM -> (ADD) / tx_buf[i]=0x"); for(i=0;i<10;i++) DEBUG("%02X.",tx_buf_base[i]);  DEBUG(" nb_frame=%d\n",nb_frame);


    /* protect header */
    for(i=0;i<HDR_CHECKSUM_ID;i++) check_sum+=(*tx_buf++);
    tx_buf+=2;
    for(i=0;i<(2*nb_frame);i++)
    {
        check_sum+=(*tx_buf++);
    }

    /* protect frames */
    for(i=0;i<nb_frame;i++)
    {
        len=tx_buf_base[HDR_FIXED_WORD+2*i]+(tx_buf_base[HDR_FIXED_WORD+1+2*i]<<HDR_SHIFT);
        DEBUG("CHECKSUM -> frame %d/%d len=%d\n",i,nb_frame,len);
        for(j=0;j<len;j++) check_sum+=(*tx_buf++);
    }

    tx_buf_base[HDR_CHECKSUM_ID]=(unsigned char) check_sum;
    tx_buf_base[HDR_CHECKSUM_ID+1]=(unsigned char)(check_sum>>HDR_SHIFT);

    DEBUG("CHECKSUM -> computed = %d = 0x%04X / tx_buf[i]=0x",check_sum,check_sum);
    for(i=0;i<10;i++) DEBUG("%02X.",tx_buf_base[i]);
    DEBUG("\n");
}


/*
  This function adds a checksum to protect an IPC message (BP-> AP)
  Call this function after inverssing the buffer endianess !
  return 1 if ok, return 0 if checksum not correct
*/
unsigned char ipc_buffer_check_checksum(unsigned long * ptr_buf)
{
    /*
      Buf structure:                               bytes # in big endian

      IPC_VERSION (16 bits)  FRAME NB (16 bits)    0.1.2.3
      FLAGS (16 bits)        CHECKSUM (16 bits)    4.5.6.7

      LEN  FRAME 1  (16 bits)  if frame 1 exists   8...
      LEN  FRAME 2  (16 bits)  if frame 2 exists
      ...
      LEN  FRAME 10 (16 bits)  if frame 10 exists

      DATA FRAME 1 (LEN1 bytes) if frame 1 exists
      DATA FRAME 2 (LEN2 bytes) if frame 2 exists
      ...
      DATA FRAME 10(LEN10 bytes) if frame 10 exists
    */

    unsigned char  *tx_buf;
    unsigned char  *tx_buf_base;
    unsigned short check_sum;
    unsigned short msg_check_sum;
    unsigned short nb_frame;
    unsigned short len;
    unsigned short i,j;

    tx_buf_base=(unsigned char *)ptr_buf;
    tx_buf=tx_buf_base;
    check_sum=0;

    nb_frame=tx_buf_base[HDR_FRAME_NB_ID]+(tx_buf_base[HDR_FRAME_NB_ID+1]<<HDR_SHIFT);

    DEBUG("CHECKSUM -> (CHECK) / tx_buf[i]=0x"); for(i=0;i<10;i++) DEBUG("%02X.",tx_buf_base[i]);  DEBUG(" nb_frame=%d\n",nb_frame);

    /* protect header */
    for(i=0;i<HDR_CHECKSUM_ID;i++) check_sum+=(*tx_buf++);
    tx_buf+=2;
    for(i=0;i<(2*nb_frame);i++)
    {
        check_sum+=(*tx_buf++);
    }

    /* protect frames */
    for(i=0;i<nb_frame;i++)
    {
        len=tx_buf_base[HDR_FIXED_WORD+2*i]+(tx_buf_base[HDR_FIXED_WORD+1+2*i]<<HDR_SHIFT);
        DEBUG("CHECKSUM -> frame %d/%d len=%d\n",i,nb_frame,len);
        for(j=0;j<len;j++) check_sum+=(*tx_buf++);
    }

    msg_check_sum=tx_buf_base[HDR_CHECKSUM_ID]+(tx_buf_base[HDR_CHECKSUM_ID+1]<<HDR_SHIFT);

    DEBUG("CHECKSUM -> computed = %d = 0x%04X / in msg = %d = 0x%04X / tx_buf[i]=0x",check_sum,check_sum,msg_check_sum,msg_check_sum);
    for(i=0;i<10;i++) DEBUG("%02X.",tx_buf_base[i]);
    DEBUG("\n");

    if(check_sum!=msg_check_sum) { DEBUG("CHECKSUM -> CHECKSUM ERROR\n"); return 0; }

    DEBUG("CHECKSUM -> CHECKSUM OK\n"); return 1;
}

#endif //#ifdef CONFIG_MAILBOX_IPC_USE_CHECKSUM

/*
 * called by mailbox read callback ...
 */
void ipc_api_read_callback(USB_IPC_CHANNEL_INDEX ch_index, int real_size)
{
    HW_CTRL_IPC_READ_STATUS_T           ipc_status;
    HW_CTRL_IPC_NOTIFY_STATUS_T         ipc_notify_status;

    IPC_DATA_HEADER * header;
    unsigned short NbFrames;
    unsigned long * adr=0;

    ENTER_FUNC();

    /* initialize notify callback argument */
    ipc_notify_status.channel = &usb_ipc_channels[ch_index].ch;
    ipc_notify_status.status = HW_CTRL_IPC_STATUS_ERROR;

    /* initialize read callback argument */
    ipc_status.channel  = &usb_ipc_channels[ch_index].ch;

    header = (IPC_DATA_HEADER *)usb_ipc_channels[ch_index].read_ptr.temp_buff;
    //change for endian
    NbFrames = ((unsigned short *) usb_ipc_channels[ch_index].read_ptr.temp_buff)[0];
    ipc_header_exchange_endian32((unsigned long *)usb_ipc_channels[ch_index].read_ptr.temp_buff,NbFrames);
    usb_ipc_exchange_endian16((u16 *)&(header->sequence_number));
    if(header->nb_frame%2)
    {
        adr = (unsigned long *) __arch_ioremap(usb_ipc_channels[ch_index].read_ptr.temp_buff_phy_addr +(sizeof(IPC_DATA_HEADER_INDEX) + (sizeof(IPC_FRAME_DESCRIPTOR) * (header->nb_frame))-sizeof(short)),
                                               W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);

        if (adr) usb_ipc_exchange_endian32_short(adr);
        else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);
    }
    //change for endian end

#ifdef CONFIG_MAILBOX_IPC_USE_CHECKSUM
    /*change for endian: swap last data frame*/
    adr = (unsigned long *) __arch_ioremap(usb_ipc_channels[ch_index].read_ptr.temp_buff_phy_addr + real_size  - (real_size%4)
	     					,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);
    if (adr) usb_ipc_exchange_endian32_short(adr);
    else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);

    if(ipc_buffer_check_checksum(usb_ipc_channels[ch_index].read_ptr.temp_buff)==0)
    {
        printk("The checksum is not correct\n");
        panic("panic for wrong checksum");
    }
    /*change for endian: swap last data frame*/
    adr =(unsigned long *) __arch_ioremap(usb_ipc_channels[ch_index].read_ptr.temp_buff_phy_addr + real_size  - (real_size%4)
	     					,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);
    if (adr) usb_ipc_exchange_endian32_short(adr);
    else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);
#endif

    if(header->version != IPC_FRAME_VERSION) {
        printk("The version(0x%x) is not equal to expected version(0x%x)\n", header->version, IPC_FRAME_VERSION);
        panic("panic for wrong verion");
    }
    if(header->sequence_number != recv_sequence_number) {
        printk("The sequence number(%d) is not equal to expected sequence number(%d)\n", header->sequence_number, recv_sequence_number);
        panic("panic for wrong sequence number");
    }
    DEBUG("%s:version(0x%x), sequence number(%d)\n", __FUNCTION__, header->version, header->sequence_number);
    /* recv_sequence_number will increase 1 on every read operation, and modulo 256 by using wrap around on a unsigned char.*/
    recv_sequence_number++;

    if( (real_size <= sizeof(IPC_DATA_HEADER_INDEX) ) && (header->nb_frame <= 0)) {
        IPC_USB_READ_ERROR(usb_ipc_channels[ch_index], ipc_notify_status);
    }

    ipc_dma_memcpy_buf2node(&usb_ipc_channels[ch_index]);

}


/*!
 * Opens an IPC link. This functions can be called directly by kernel
 * modules. POSIX implementation of the IPC Driver also calls it.
 *
 * @param config        Pointer to a struct containing configuration para
 *                      meters for the channel to open (type of channel,
 *                      callbacks, etc)
 *
 * @return              returns a virtual channel handler on success, a NULL
 *                      pointer otherwise.
 */
HW_CTRL_IPC_CHANNEL_T * hw_ctrl_ipc_open(const HW_CTRL_IPC_OPEN_T * config)
{
    USB_IPC_CHANNEL_INDEX      channel;
    IPC_DATA_HEADER *header = NULL;


    ENTER_FUNC();
    switch (config->type) {
        case HW_CTRL_IPC_PACKET_DATA:
            channel = IPC_DATA_CH_NUM;
            break;
        case HW_CTRL_IPC_CHANNEL_LOG:
            channel = IPC_LOG_CH_NUM;
            break;
        default:
            return NULL;
    }

    if (usb_ipc_channels[channel].open_flag != 0)  {
        return NULL;
    }


#ifdef CONFIG_MAILBOX_IPC_ARCH_V2
    /*open MB 0,1*/
    /*clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_1,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);

    /*disable mailboxes*/
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_1,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    /*enable mailbox 0*/
    HW_MBOX_EventEnable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);
#else
    /*register mailbox IT*/
    hw_ctrl_ipc_ap_init();
    /*open MB 0,1,2,3*/
    /*clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_1,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_2,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_3,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);

    /*disable mailboxes*/
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_1,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_2,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_3,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    /*enable mailbox 3*/
    HW_MBOX_EventEnable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_3,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);
#endif

    /*open MB 4 and 5 for DLOG */
    /*clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_4,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_5,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    /*disable mailboxes*/
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_4,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_5,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    /*enable mailbox 4*/
    HW_MBOX_EventEnable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_4,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);

    /*open MB 6 and 7 for PANIC   MB6 BP->AP   MB7 AP->BP */
    /*clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_6,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_7,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    /*disable mailboxes*/
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_6,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_7,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    /*enable mailbox 6*/
    HW_MBOX_EventEnable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_6,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);


    /* init IPC API channel parameters */
    usb_ipc_channels[channel].ch.channel_nb = (int)channel;
    memcpy((void *)&(usb_ipc_channels[channel].cfg), (void *)config, sizeof(HW_CTRL_IPC_OPEN_T));
    usb_ipc_channels[channel].max_node_num = MAX_IPC_EX2_NODE_NUM;

    usb_ipc_channels[channel].max_temp_buff_size = (MAX_FRAME_SIZE * MAX_FRAME_NUM) + sizeof(IPC_DATA_HEADER);

    usb_ipc_channels[channel].read_ptr.temp_buff_phy_addr = (dma_addr_t)IPC_BUFF_RX_AP_BASE;
    usb_ipc_channels[channel].read_ptr.temp_buff = (unsigned char *) __arch_ioremap(IPC_BUFF_RX_AP_BASE,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);

    usb_ipc_channels[channel].write_ptr.temp_buff_phy_addr = (dma_addr_t)IPC_BUFF_TX_AP_BASE;
    usb_ipc_channels[channel].write_ptr.temp_buff = (unsigned char *) __arch_ioremap(IPC_BUFF_TX_AP_BASE,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);

    /* read header init */
    header = (IPC_DATA_HEADER *) usb_ipc_channels[channel].read_ptr.temp_buff;
    header->version  = IPC_FRAME_VERSION;
    header->nb_frame = 0;
    header->sequence_number = sent_sequence_number = 0;

    /* write header init */
    header = (IPC_DATA_HEADER *) usb_ipc_channels[channel].write_ptr.temp_buff;
    header->version = IPC_FRAME_VERSION;
    header->nb_frame = 0;
    header->sequence_number = recv_sequence_number = 0;

    /* set flag to indicate this channel is opened */
    usb_ipc_channels[channel].open_flag  = 1;

    mailbox_vr_init();

    return &usb_ipc_channels[channel].ch;
}

/*!
 * Close an IPC link. This functions can be called directly by kernel
 * modules.
 *
 * @param channel       handler to the virtual channel to close.
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_close(HW_CTRL_IPC_CHANNEL_T * channel)
{
    DEBUG("%s: channel_nb = %d\n", __FUNCTION__, channel->channel_nb);
    if( (usb_ipc_channels[channel->channel_nb].open_flag == 0) ||
        (usb_ipc_channels[channel->channel_nb].write_flag) ||
        (usb_ipc_channels[channel->channel_nb].read_flag) ){
        return HW_CTRL_IPC_STATUS_ERROR;
    }

    usb_ipc_channels[channel->channel_nb].open_flag = 0;
    return HW_CTRL_IPC_STATUS_OK;
}


/*!
 * Used to set various channel parameters
 *
 * @param channel handler to the virtual channel where read has
 *                been requested.
 * @param action  IPC driver control action to perform.
 * @param param   parameters required to complete the requested action
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_ioctl(HW_CTRL_IPC_CHANNEL_T * channel,
                                       HW_CTRL_IPC_IOCTL_ACTION_T action,
                                       void *param)
{
    /* detect channel */
    if (usb_ipc_channels[channel->channel_nb].open_flag == 0) {
        return HW_CTRL_IPC_STATUS_ERROR;
    }

    switch (action) {
        case HW_CTRL_IPC_SET_READ_CALLBACK:
            usb_ipc_channels[channel->channel_nb].cfg.read_callback = param;
            break;
        case HW_CTRL_IPC_SET_WRITE_CALLBACK:
            usb_ipc_channels[channel->channel_nb].cfg.write_callback = param;
            break;
        case HW_CTRL_IPC_SET_NOTIFY_CALLBACK:
            usb_ipc_channels[channel->channel_nb].cfg.notify_callback = param;
            break;
        case HW_CTRL_IPC_SET_MAX_CTRL_STRUCT_NB:
            usb_ipc_channels[channel->channel_nb].max_node_num = *(int *)param;
            break;
        default:
            return HW_CTRL_IPC_STATUS_ERROR;
    }

    return HW_CTRL_IPC_STATUS_OK;
}

/*!
 * This function is a variant on the write() function, and is used to send a
 * group of frames made of various pieces each to the IPC driver.
 * It is mandatory to allow high throughput on IPC while minimizing the time
 * spent in the drivers / interrupts.
 *
 * @param channel       handler to the virtual channel where read has
 *                      been requested.
 * @param ctrl_ptr      Pointer on the control structure.
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_write_ex2(HW_CTRL_IPC_CHANNEL_T * channel,
                                           HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *
                                           ctrl_ptr)
{
    int ret = -1;
    int i;

    IPC_DATA_HEADER *header = NULL;


    ENTER_FUNC(); the_ipc_4ghif_stats.ipc_3gsm_ap_write_ex2++;

    if(usb_ipc_channels[channel->channel_nb].open_flag == 0) {
        printk("%s: Error: channel unavailable\n", __FUNCTION__);
        return HW_CTRL_IPC_STATUS_CHANNEL_UNAVAILABLE;
    }
    if(usb_ipc_channels[channel->channel_nb].write_flag == 1) {
        printk("%s: Error: channel write ongoing\n", __FUNCTION__);
        return HW_CTRL_IPC_STATUS_WRITE_ON_GOING;
    }

    if((ctrl_ptr == NULL) || (channel == NULL)) {
        printk("%s: Error: Input arguments\n", __FUNCTION__);
        return HW_CTRL_IPC_STATUS_ERROR;
    }

    usb_ipc_channels[channel->channel_nb].write_flag = 1;

    /* save the parameters and start write operation */
    usb_ipc_channels[channel->channel_nb].write_type = HW_CTRL_IPC_WRITE_EX2_TYPE;
    usb_ipc_channels[channel->channel_nb].write_ptr.node_ptr   = ctrl_ptr;
    usb_ipc_channels[channel->channel_nb].write_ptr.node_index = 0;

    for(i = 0; i < usb_ipc_channels[channel->channel_nb].max_node_num; i++) {
        if(ctrl_ptr[i].comand & NODE_DESCRIPTOR_END_BIT) {
            break;
        }
    }
    usb_ipc_channels[channel->channel_nb].write_ptr.node_num   = i + 1;
    usb_ipc_channels[channel->channel_nb].write_ptr.total_num  = 0;
    usb_ipc_channels[channel->channel_nb].write_ptr.end_flag   = 0;


    header = (IPC_DATA_HEADER *)usb_ipc_channels[channel->channel_nb].write_ptr.temp_buff;
    /* This re-assign is for avoiding wrong version in next sending */
    header->version  = IPC_FRAME_VERSION;
    /* get frame num */
    header->nb_frame = 0;
    header->sequence_number = sent_sequence_number;
    DEBUG("%s:version(0x%x), sequence number(%d)\n", __FUNCTION__,  header->version, header->sequence_number);
    for(i = 0; i < usb_ipc_channels[channel->channel_nb].write_ptr.node_num; i++) {
        if( ctrl_ptr[i].comand & NODE_DESCRIPTOR_LAST_BIT )  {
           header->nb_frame ++;
            if( header->nb_frame >= MAX_FRAME_NUM)  {
                break;
            }
        }
    }

    ret = ipc_dma_memcpy_node2buf(&usb_ipc_channels[channel->channel_nb], ctrl_ptr);
    if(ret != 0) {
        usb_ipc_channels[channel->channel_nb].write_flag = 0;
        return HW_CTRL_IPC_STATUS_ERROR;
    }

    if(usb_ipc_channels[channel->channel_nb].cfg.write_callback == NULL)  {
        SEM_LOCK(&usb_ipc_channels[channel->channel_nb].write_ptr.write_mutex);
        usb_ipc_channels[channel->channel_nb].write_flag = 0;
    }

    return HW_CTRL_IPC_STATUS_OK;
}

/*
 * This function is used to give a set of buffers to the IPC and enable data
 * transfers.
 *
 * @param channel       handler to the virtual channel where read has
 *                      been requested.
 * @param ctrl_ptr      Pointer on the control structure.
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_read_ex2(HW_CTRL_IPC_CHANNEL_T * channel,
                                          HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *
                                          ctrl_ptr)
{
    int  i,exe;
    unsigned long flags;

    ENTER_FUNC(); the_ipc_4ghif_stats.ipc_3gsm_ap_read_ex2++;

    if((channel == NULL) || (ctrl_ptr == NULL)) {
        printk("%s: Error: Input arguments\n", __FUNCTION__);
        return HW_CTRL_IPC_STATUS_ERROR;
    }

    if(usb_ipc_channels[channel->channel_nb].open_flag == 0) {
        printk("%s: Error: channel unavailable\n", __FUNCTION__);
        printk("Error: open flag = %d\n", usb_ipc_channels[channel->channel_nb].open_flag);
        return HW_CTRL_IPC_STATUS_CHANNEL_UNAVAILABLE;
    }
    if(usb_ipc_channels[channel->channel_nb].read_flag == 1) {
        printk("%s: Error: channel read onging\n", __FUNCTION__);
        return HW_CTRL_IPC_STATUS_READ_ON_GOING;
    }

    usb_ipc_channels[channel->channel_nb].read_flag = 1;

    /* save the parameters and start read operation */
    usb_ipc_channels[channel->channel_nb].read_type = HW_CTRL_IPC_READ_EX2_TYPE;
    usb_ipc_channels[channel->channel_nb].read_ptr.node_ptr   = ctrl_ptr;
    usb_ipc_channels[channel->channel_nb].read_ptr.node_index = 0;

    for(i = 0; i < usb_ipc_channels[channel->channel_nb].max_node_num; i++) {
        if(ctrl_ptr[i].comand & NODE_DESCRIPTOR_END_BIT) {
            break;
        }
    }
    usb_ipc_channels[channel->channel_nb].read_ptr.node_num = i + 1;
    usb_ipc_channels[channel->channel_nb].read_ptr.total_num  = 0;


    /*enable mailbox 0*/
#ifdef CONFIG_MAILBOX_IPC_ARCH_V2
    spin_lock_irqsave(&ipc_mbox_rd_lock,flags);
      exe=0;
      en_read_msg=1;
      if(en_read_msg && rx_read_msg) { en_read_msg=rx_read_msg=0; exe=1; /*mailbox_ipc_read_callback(read_msg);*/ }
    spin_unlock_irqrestore(&ipc_mbox_rd_lock,flags);
    if(exe) mailbox_ipc_read_callback(read_msg);
#else
    HW_MBOX_EventEnable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),
                        HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);
#endif

    /* No callback, waiting receiving is done */
    if(usb_ipc_channels[channel->channel_nb].cfg.read_callback == NULL)  {
        SEM_LOCK(&usb_ipc_channels[channel->channel_nb].read_ptr.read_mutex);
        usb_ipc_channels[channel->channel_nb].read_flag = 0;
    }
    return HW_CTRL_IPC_STATUS_OK;
}


/*
 * IPC API module init function
 */
int __init ipc_api_init(void)
{
    int i;
    for(i = 0; i < MAX_USB_IPC_CHANNELS; i++) {
        memset((void *)&usb_ipc_channels[i], 0, sizeof(USB_IPC_API_PARAMS));
        /* initialize the semphores */
        SEM_LOCK_INIT(&usb_ipc_channels[i].write_ptr.write_mutex);
        SEM_LOCK_INIT(&usb_ipc_channels[i].read_ptr.read_mutex);
    }

    the_ipc_4ghif_stats.ipc_3gsm_ap_data_dl=0;
    the_ipc_4ghif_stats.ipc_3gsm_ap_ack_dl=0;
    the_ipc_4ghif_stats.ipc_3gsm_ap_read_ex2=0;
    the_ipc_4ghif_stats.ipc_3gsm_ap_data_ul=0;
    the_ipc_4ghif_stats.ipc_3gsm_ap_ack_ul=0;
    the_ipc_4ghif_stats.ipc_3gsm_ap_write_ex2=0;

    printk("IPC API initialized.\n");
    return 0;
}

static void mailbox_shortmsg_callback(void)
{
    u32 msg;
    enum ipc_vr_short_msg msg_type;

    /* Read the message from the mailbox. */
    HW_MBOX_MsgRead((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_6,&msg);/* clear events*/

    msg_type = (enum ipc_vr_short_msg)(msg>>16);
    if ((msg_type < sizeof(ipc_vr_short_msg_handler_tb)/sizeof(ipc_vr_short_msg_handler_tb[0])) &&
        (ipc_vr_short_msg_handler_tb[msg_type].handler_fcn_p != NULL))
        {
            /* Call the handler for the data. */
        ipc_vr_short_msg_handler_tb[msg_type].handler_fcn_p(msg_type, (msg&0xffff));
        }
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),
                     HW_MBOX_ID_6,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);
}

/*=============================================================================
                                       GLOBAL FUNCTIONS
===============================================================================*/
/* This function is to call when driver needs to send a CMD to the BP panic engine */
void mailbox_panic_send_cmd(u32 panic_cmd)
{
    HW_MBOX_MsgWrite((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_7, panic_cmd);

    DEBUG_PANIC_CMD("DPANIC - Send CMD 0x%08X\n",panic_cmd);
}

/* Call this function when BP log chunk has be read to acknowledge the BP */
void mailbox_dlog_ack(void)
{
    u32 ui_msg=(u32)0x40000000; // O1=ACK 0=LOG 0000000000000 LEN=0000000000000000
    HW_MBOX_MsgWrite((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_5,ui_msg);
    DEBUG_DLOG("DLOG Ack\n");
}


/* This function is called when a new BP log chunk is available */
#ifdef CONFIG_MACH_W3G_LTE_DATACARD
#define IPC_LOG_SHARED_MEMORY_PHYS 0x8089C400
#define IPC_LOG_SHARED_MEMORY_VIRT 0xD839C400
#else
#define IPC_LOG_SHARED_MEMORY_PHYS 0x8079C400
#define IPC_LOG_SHARED_MEMORY_VIRT 0xD839C400
#endif
void mailbox_dlog_read_callback(void)
{
    u32 msg = 0;
    u16 data_size = 0;
    u32 chunk_phys_adr=0;
    u32 chunk_virt_adr=0;

    HW_MBOX_MsgRead((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_4,    &msg);
    data_size = (u16)msg;
    chunk_phys_adr=IPC_LOG_SHARED_MEMORY_PHYS+((msg&0x01FF0000)>>6); //in fact: 0x8079C400+(((msg&0x01FF0000)>>16)<<10);
    chunk_virt_adr=IPC_LOG_SHARED_MEMORY_VIRT+((msg&0x01FF0000)>>6); //in fact: 0xD839C400+(((msg&0x01FF0000)>>16)<<10);

    /* clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_4,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);

    // Display only Chunk characteristics
    DEBUG_DLOG("DLOG Msg=0x%08X phys=0x%08X virt=0x%04X len=0x%04X\n",msg,chunk_phys_adr,chunk_virt_adr,data_size);

    mb_ipc_log_readscheduler((unsigned long) chunk_virt_adr, (unsigned int) data_size);
}

/* ISR function called when an interruption occured on a mailbox*/
#ifdef CONFIG_MAILBOX_IPC_ARCH_V2
irqreturn_t mailbox_ISR(int irq, void *p)
{
    u32 NumMsg=0;
    HW_STATUS ret;
    u32 msg = 0;

    ret = HW_MBOX_NumMsgGet((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_0,&NumMsg);
    /* new msg in mailbox 0*/
    if ((ret == RET_OK)&&(NumMsg >0))
    {
        HW_MBOX_MsgRead((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_0,    &msg);
	HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);

	/* msg type init */
    	if(bp_ipc_ready == 0)
    	{
           if (msg==0xE000000)
	   {
	       /* Wake up the netmux_wait wait queue*/
    	       DEBUG(KERN_INFO "IPC sync message received\n");
               LDInit();
	       bp_ipc_ready = 1;
               return IRQ_HANDLED;
           }
    	}

	/* msg type (2 MSB): O1=ACK 00=DATA 10=SPARE 11=SPARE */
	if((msg&0xC0000000)==0x40000000)
	{
	    /* callback for ACK UL */
            mailbox_ipc_write_callback(msg);
	    the_ipc_4ghif_stats.ipc_3gsm_ap_ack_ul++;
	}
	else if((msg&0xC0000000)==0)
	{
	    /* callback for new DL data from BP (only if read_ex2 was done, else store the message and send it for next
	    read_ex2)*/
            read_msg=msg;
	    rx_read_msg=1;
            trace_dl_rcv();
            if(en_read_msg && rx_read_msg) { en_read_msg=rx_read_msg=0;mailbox_ipc_read_callback(read_msg); }
	    the_ipc_4ghif_stats.ipc_3gsm_ap_data_dl++;
	}
	else
	{
	    panic("Unknown type for IPC BP to AP mailbox 0 message");
	}
    }

    ret = HW_MBOX_NumMsgGet((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_4,&NumMsg);
    /* new msg in mailbox 4*/
    if ((ret == RET_OK)&&(NumMsg >0))
    {
        /* callback*/
        mailbox_dlog_read_callback();
    }

    ret = HW_MBOX_NumMsgGet((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_6,&NumMsg);
    /* new msg in mailbox 6*/
    if ((ret == RET_OK)&&(NumMsg >0))
    {
        mail_box6_irq_callback();
    }

    return IRQ_HANDLED;
}
#else
irqreturn_t mailbox_ISR(int irq, void *p)
{
    u32 NumMsg=0;
    HW_STATUS ret;

    ret = HW_MBOX_NumMsgGet((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_0,&NumMsg);

    /* new msg in mailbox 0*/
    if ((ret == RET_OK)&&(NumMsg >0)&&(usb_ipc_channels[IPC_DATA_CH_NUM].read_flag == 1))
    {
        /* callback*/
        mailbox_ipc_read_callback();
    }

    ret = HW_MBOX_NumMsgGet((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_3,&NumMsg);
    /* new msg in mailbox 3*/
    if ((ret == RET_OK)&&(NumMsg >0))
    {
        /* callback*/
        mailbox_ipc_write_callback();
    }


    ret = HW_MBOX_NumMsgGet((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_4,&NumMsg);
    /* new msg in mailbox 4*/
    if ((ret == RET_OK)&&(NumMsg >0))
    {
        /* callback*/
        mailbox_dlog_read_callback();
    }

    ret = HW_MBOX_NumMsgGet((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_6,&NumMsg);
    /* new msg in mailbox 6*/
    if ((ret == RET_OK)&&(NumMsg >0))
    {
        mail_box6_irq_callback();
    }

    return IRQ_HANDLED;
}
#endif

/* BP signaled AP there is some data available for read in the shared memory*/
#ifdef CONFIG_MAILBOX_IPC_ARCH_V2
void mailbox_ipc_read_callback(u32 msg)
{
    u16 data_size = 0;

    data_size = (u16)msg;

    ipc_api_read_callback(IPC_DATA_CH_NUM, (int) data_size);
}
#else
void mailbox_ipc_read_callback(void)
{
    u16 data_size = 0;
    u32 msg = 0;

    /*disable the receiver*/
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);

    HW_MBOX_MsgRead((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_0,   &msg);
    data_size = (u16)msg;

    /* clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);

    ipc_api_read_callback(IPC_DATA_CH_NUM, (int) data_size);
}
#endif

/* Signal upper layer that data has been read by the BP*/
#ifdef CONFIG_MAILBOX_IPC_ARCH_V2
void mailbox_ipc_write_callback(u32 msg)
{
    u16 data_size = 0;

    data_size = (u16)msg;

    usb_ipc_api_write_callback(IPC_DATA_CH_NUM, 0, (int) data_size);
}
#else
void mailbox_ipc_write_callback(void)
{
    u16 data_size = 0;
    u32 msg = 0;

    HW_MBOX_MsgRead((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE), HW_MBOX_ID_3,    &msg);
    data_size = (u16)msg;

    /* clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_3,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);

    usb_ipc_api_write_callback(IPC_DATA_CH_NUM, 0, (int) data_size);
}
#endif

/* Signal BP that some data is available for read in the shared memory*/
#ifdef CONFIG_MAILBOX_IPC_ARCH_V2
int mailbox_write(u16 length)
{
    unsigned long flags;
    int ret;

    u32 ui_msg=(u32)(length | MAILBOX_CHANNEL_DATA | MAILBOX_MSG_TYPE_TX);

    spin_lock_irqsave(&ipc_mbox_wr_lock,flags);
    ret = ((int)HW_MBOX_MsgWrite((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_1,ui_msg));
    spin_unlock_irqrestore(&ipc_mbox_wr_lock,flags);

    the_ipc_4ghif_stats.ipc_3gsm_ap_data_ul++;

    return ret;
}
#else
int mailbox_write(u16 length)
{
    u32 ui_msg=(u32)(length | MAILBOX_CHANNEL_DATA | MAILBOX_MSG_TYPE_TX);
    return ((int)HW_MBOX_MsgWrite((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_2,ui_msg));
}
#endif

#ifdef CONFIG_MAILBOX_IPC_ARCH_V2
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_send_read_ack(u32 ui_msg)
{
    unsigned long flags;

    trace_dl_ack();

    spin_lock_irqsave(&ipc_mbox_wr_lock,flags);
    HW_MBOX_MsgWrite((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_1,ui_msg);
    spin_unlock_irqrestore(&ipc_mbox_wr_lock,flags);

    the_ipc_4ghif_stats.ipc_3gsm_ap_ack_dl++;

    return HW_CTRL_IPC_STATUS_OK;
}
#endif

int mailbox_vr_available(void)
{
    /* The emulated registers can be used if the BP is running and the vr's have been initialized. */
    return ((mail_box6_irq_callback == mailbox_shortmsg_callback) && (mailbox_vr_initialized()));
}

void hw_ctrl_ipc_ap_init(void)
{
    printk(KERN_INFO "Register mailbox IT\n");

    /*register IT*/
    if (request_irq(INT_24XX_MAIL_U0_MPU, mailbox_ISR, IRQF_DISABLED,"mailbox", NULL))
    {
        printk(KERN_INFO "Error: unable to register MAILBOX IPC interrupt!\n");
    }

    /*open mailbox 0 for BP IPC init notification*/
    /*clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    /*disable mailbox*/
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_ALL);
    /*enable mailbox 0*/
    HW_MBOX_EventEnable((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),HW_MBOX_ID_0,HW_MBOX_U0_ARM,HW_MBOX_INT_NEW_MSG);

}

core_initcall(ipc_api_init);
EXPORT_SYMBOL(hw_ctrl_ipc_open);
EXPORT_SYMBOL(hw_ctrl_ipc_close);
EXPORT_SYMBOL(hw_ctrl_ipc_write_ex2);
EXPORT_SYMBOL(hw_ctrl_ipc_read_ex2);
EXPORT_SYMBOL(hw_ctrl_ipc_ioctl);
EXPORT_SYMBOL(hw_ctrl_ipc_ap_init);
#ifdef CONFIG_MAILBOX_IPC_ARCH_V2
EXPORT_SYMBOL(hw_ctrl_ipc_send_read_ack);
#else
EXPORT_SYMBOL(HW_MBOX_MsgWrite);
#endif
EXPORT_SYMBOL(mailbox_vr_available);
