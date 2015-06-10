/*
 * Copyright (c) 2009-2011 Motorola Mobility, Inc, All Rights Reserved.
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
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12-oct-2009       Motorola         file creation
 * 23-dec-2009       Motorola         improve sDMA settings
 * 06-jan-2010       Motorola         adapt sDMA endianess to match the BP & LCM one
 * 08-jan-2010       Motorola         handle multiple UL-ACK
 * 12-jan-2010       Motorola         added new memory manager
 * 04-feb-2010       Motorola         Add separate FIFOs for ACKs (also known as phase3)
 * 29-mar-2010       Motorola         added mailbox users for W3G architecture in hw_mbox.h
 * 11-may-2010       Motorola         fill fifo_data_ul_msg before statrting the sDMA transfer
 * 12-may-2010       Motorola         4G HIF IPC low level driver on AP side - phase 2 (group frames)
 * 25-may-2011       Motorola         Need to add logging to track IPC ctrl msg round trip time
 * 31-may-2011       Motorola         Increase statistics buffer size (needed for previous change)
 */

/*!
 * @file drivers/usb/ipchost/ipc_4ghif.c
 * @brief 4G HIF low level driver Descriptor Set
 *
 * Implementation of 4G HIF low level driver.
 *
 * @ingroup IPCFunction
 */


/*==============================================================================
  INCLUDE FILES
==============================================================================*/

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/kdev_t.h>
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
#include <mach/io.h>
#include <asm/mach/map.h>
#include <mach/dma.h>
#include <asm/dma.h>
#include <linux/dma-mapping.h>
#include <linux/prcm-regs.h>
#include <asm/cacheflush.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/ipc_4ghif.h>
#include "ipc_4ghif_mm.h"

/*==============================================================================
  LOCAL CONSTANTS
==============================================================================*/

#define IPC_DMA_4GHIF_UL_ID 1
#define IPC_DMA_4GHIF_DL_ID 2
#define MAILBOX_READY_MSG  1
#define MAILBOX_DATA_MSG   2
#define MAILBOX_ACK_MSG    3

#define CDP_VALUE_FOR_TYPE3SD_LINKED_LIST (OMAP_DMA_CDP_LINKED_LIST_DESCR_LITTLE_ENDIAN | \
                       OMAP_DMA_CDP_LINKED_LIST_ENABLE_FAST_MODE    | \
                       OMAP_DMA_CDP_LINKED_LIST_TRANSFER_MODE       | \
                       OMAP_DMA_CDP_LINKED_LIST_DESCR_TYPE_3        | \
                       OMAP_DMA_CDP_LINKED_LIST_SRC_ADDRESS_LOAD    | \
                       OMAP_DMA_CDP_LINKED_LIST_DEST_ADDRESS_LOAD)

#define NEXT_DESCR_TYPE3SD            0x65000000
#define NEXT_DESCR_LAST               0x10000000
#define END_OF_DMA_TYPE3_LINKLIST     0xFFFFFFFF

/* For 3GSM IPC stats*/
#define IPC_DEBUG_STAT_ADDRESS    0x810FFE00
#define IPC_DEBUG_STAT_VIRT_ADDR  0xD84FFE00
/*==============================================================================
  LOCAL MACROS
==============================================================================*/
/*#define IPC_DEBUG*/

#ifdef IPC_DEBUG
#define DEBUG(args...) printk(args)
#else
#define DEBUG(args...)
#endif
#define ENTER_FUNC() DEBUG("Enter %s\n", __FUNCTION__)

#define TS_SUBMIT_UL       1
#define TS_SUBMIT_UL_CB    2
#define TS_SUBMIT_DL       3
#define TS_SUBMIT_DL_CB    4
#define TS_NEW_MSG         5
#define TS_RESOURCES_FULL  6
#define TS_RESUME_CB       7
#define TS_START_DMA_UL    8
#define TS_START_DMA_DL    9
#define TS_DMA_CB_UL       10
#define TS_DMA_CB_DL       11
#define TS_LAST            12

#define FIFO_FULL 0
#define MEM_FULL  1

/* 4GHIF timestamp is enabled when test driver is enabled. if needed to enable 4GHIF timestamp without the test driver then comment ifdef*/
//#ifdef CONFIG_IPC_4GHIF_TEST
    #define IPC_4GHIF_TIMESTAMP
//#endif

#ifdef IPC_4GHIF_TIMESTAMP

#define TX_KT_ARRAY_SIZE 2048
union ktime tx_kt_array[TX_KT_ARRAY_SIZE];
u32 tx_kt_array_arg[TX_KT_ARRAY_SIZE];
u32 tx_kt_array_arg2[TX_KT_ARRAY_SIZE];
u16 tx_kt_array_wr=0;
u16 tx_kt_array_used=0;
char * ts_function_array[TS_LAST]={"Empty","TS_SUBMIT_UL","TS_SUBMIT_UL_CB","TS_SUBMIT_DL","TS_SUBMIT_DL_CB","TS_NEW_MSG","TS_RESOURCES_FULL","TS_RESUME_CB","TS_START_DMA_UL","TS_START_DMA_DL","TS_DMA_CB_UL","TS_DMA_CB_DL"};

#define TIMESTAMP_4GHIF(step_arg,arg2) \
        tx_kt_array[tx_kt_array_wr]=ktime_get(); \
        tx_kt_array_arg[tx_kt_array_wr]=step_arg; \
	tx_kt_array_arg2[tx_kt_array_wr]=arg2;  \
        tx_kt_array_wr++; \
	if (tx_kt_array_used<TX_KT_ARRAY_SIZE) tx_kt_array_used++;\
        if(tx_kt_array_wr==TX_KT_ARRAY_SIZE) tx_kt_array_wr=0;

#define TIMESTAMP_4GHIF_UL  TIMESTAMP_4GHIF(TS_START_DMA_UL,sdma_ul_packet_nb);
#define TIMESTAMP_4GHIF_DL  TIMESTAMP_4GHIF(TS_START_DMA_DL,sdma_dl_packet_nb);

#else

#define TIMESTAMP_4GHIF(step_arg,arg2)
#define TIMESTAMP_4GHIF_UL
#define TIMESTAMP_4GHIF_DL

#endif //IPC_4GHIF_TIMESTAMP

#define ONE_GIGABYTE (1<<30)

#define IPC_4GHIF_BP_LOOPBACK_STUB_DISABLED            0
#define IPC_4GHIF_BP_LOOPBACK_STUB_ENABLED             1
#define IPC_4GHIF_BP_LOOPBACK_STUB_ENABLED_ONLY_FOR_UL 2
u8 ipc_4ghif_bp_loopback_stub_mode=IPC_4GHIF_BP_LOOPBACK_STUB_DISABLED;

/* Start DMA data transfer from memory to memory */
#define OMAP_DMA_4GHIF_UL(ch, descr)                                              \
    DEBUG("\n%s: ch=%d, descr = 0x%x\n", __FUNCTION__, ch, descr);   \
    omap_set_dma_transfer_params(ch, OMAP_DMA_DATA_TYPE_S8, 0, 1, OMAP_DMA_SYNC_FRAME, 0, 0); \
    omap_set_dma_dest_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_src_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_write_mode(ch,OMAP_DMA_WRITE_LAST_NON_POSTED); \
    omap_set_dma_prefetch_mode(ch, OMAP_DMA_PREFETCH_EN); \
    omap_set_dma_src_data_pack(ch, 1); \
    omap_set_dma_dest_data_pack(ch, 1); \
    omap_set_dma_dest_params(ch, 0, OMAP_DMA_AMODE_POST_INC, 0, 0, 0);     \
    omap_set_dma_src_params(ch, 0, OMAP_DMA_AMODE_POST_INC, 0, 0, 0);       \
    omap_set_dma_src_endian_type(ch,OMAP_DMA_LITTLE_ENDIAN);\
    omap_set_dma_dst_endian_type(ch,OMAP_DMA_LITTLE_ENDIAN);\
    omap_set_linked_list_dma_descr_type3sd(ch, CDP_VALUE_FOR_TYPE3SD_LINKED_LIST, descr ); \
    TIMESTAMP_4GHIF_UL; \
    omap_start_dma(ch);


#define OMAP_DMA_4GHIF_DL(ch, descr)                                              \
    DEBUG("\n%s: ch=%d, descr = 0x%x \n", __FUNCTION__, ch, descr);   \
    omap_set_dma_transfer_params(ch, OMAP_DMA_DATA_TYPE_S8, 0, 1, OMAP_DMA_SYNC_FRAME, 0, 0); \
    omap_set_dma_dest_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_src_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_write_mode(ch,OMAP_DMA_WRITE_LAST_NON_POSTED); \
    omap_set_dma_prefetch_mode(ch, OMAP_DMA_PREFETCH_EN); \
    omap_set_dma_src_data_pack(ch, 1); \
    omap_set_dma_dest_data_pack(ch, 1); \
    omap_set_dma_dest_params(ch, 0, OMAP_DMA_AMODE_POST_INC, 0, 0, 0);     \
    omap_set_dma_src_params(ch, 0, OMAP_DMA_AMODE_POST_INC, 0, 0, 0);       \
    omap_set_dma_src_endian_type(ch,OMAP_DMA_LITTLE_ENDIAN);\
    omap_set_dma_dst_endian_type(ch,OMAP_DMA_LITTLE_ENDIAN);\
    omap_set_linked_list_dma_descr_type3sd(ch, CDP_VALUE_FOR_TYPE3SD_LINKED_LIST, descr ); \
    TIMESTAMP_4GHIF_DL; \
    omap_start_dma(ch);



/*==============================================================================
  LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==============================================================================*/

/* Type 3 Descriptor with Source and Destination address update */
typedef struct {
    u32 next_descr_addr_ptr;
    u32 length_and_next_descr_type;
    u32 src_start_addr;
    u32 dst_start_addr;
} DMA_DESCR_STRUCT_TYPE_3_SD;

typedef struct
{
    int val;
    char* string_value;
} int_string_map;

typedef enum
{
    FIFO_4GHIF_STATUS_OK = 0,
    FIFO_4GHIF_STATUS_INVALID_MSG_TYPE,
    FIFO_4GHIF_STATUS_INVALID_MSG_LENGTH,
    FIFO_4GHIF_STATUS_ERROR
} FIFO_4GHIF_STATUS;

/*==============================================================================
  LOCAL FUNCTION PROTOTYPES
==============================================================================*/

/* ISR / Callbacks */
irqreturn_t mailbox_ISR_4GHIF(int irq, void *p);
void sdma_4ghif_ul_callback(int lch, u16 ch_status, void *data);
void sdma_4ghif_dl_callback(int lch, u16 ch_status, void *data);

/* Tasklets (to call data drivers callbacks outside interrupt time */
void dl_complete_tasklet_function(unsigned long param);
void ul_complete_tasklet_function(unsigned long param);
void bp_ready_tasklet_function(unsigned long param);
void receive_dl_tasklet_function(unsigned long param);
void ul_resume_tasklet_function(unsigned long param);

/* DL msg Processing */
static void fifo_4ghif_process_dl_msg(void);
static void fifo_ack_4ghif_process_dl_msg(void);
static FIFO_4GHIF_STATUS fifo_4ghif_get_frame_info(u32 * frame_nb);

/* FIFO management */
static inline void fifo_4ghif_init_ul_write_ul_read_ind_to_shared(void);
static inline void fifo_4ghif_update_ul_read_ind_from_shared(void);
static inline void fifo_4ghif_update_ul_write_ind_to_shared(void);
static inline void fifo_4ghif_update_dl_read_ind_to_shared(void);
static inline void fifo_4ghif_update_dl_write_ind_from_shared(void);
static inline void fifo_4ghif_increment_read_ind_after_get_msg(u8 offset);

static int fifo_4ghif_ul_space_left(void);

static IPC_4GHIF_FIFO_STATUS fifo_4ghif_init(void);
static IPC_4GHIF_FIFO_STATUS fifo_4ghif_get_msg(fifo_msg* msg, u8 offset);
static IPC_4GHIF_FIFO_STATUS fifo_4ghif_add_msg(fifo_msg* msg);

/* FIFO ACK management */
static inline void fifo_ack_4ghif_init_ul_write_ul_read_ind_to_shared(void);
static inline void fifo_ack_4ghif_update_ul_read_ind_from_shared(void);
static inline void fifo_ack_4ghif_update_ul_write_ind_to_shared(void);
static inline void fifo_ack_4ghif_update_dl_read_ind_to_shared(void);
static inline void fifo_ack_4ghif_update_dl_write_ind_from_shared(void);
static inline void fifo_ack_4ghif_increment_read_ind_after_get_msg(void);

static int fifo_ack_4ghif_ul_space_left(void);

static IPC_4GHIF_FIFO_STATUS fifo_ack_4ghif_init(void);
static IPC_4GHIF_FIFO_STATUS fifo_ack_4ghif_get_msg(fifo_msg* msg);
static IPC_4GHIF_FIFO_STATUS fifo_ack_4ghif_add_msg(fifo_msg* msg);

/* Mailboxes functions */
static int mailbox_4ghif_send_msg(u32 msg);

/* Linked lists functions*/
static void fill_one_type3sd_descr(DMA_DESCR_STRUCT_TYPE_3_SD* t3_sd_descr, u32 src, u32 dst, u32 len, bool last);

/*==============================================================================
  EXTERN FUNCTION PROTOTYPES
==============================================================================*/
extern void simulate_ul_bp(void);
extern void simulate_loopback_bp_acks(void);
extern void simulate_loopback_bp_frames(void);
extern void simulate_bp_init(void);

/*==============================================================================
  LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==============================================================================*/


/*==============================================================================
  LOCAL VARIABLES
==============================================================================*/

bool b_ipc_4ghif_open = false;
bool b_bp_ready = false;
bool b_ap_ready = false;
bool b_sdma_ul_transfer_on_going = false;
bool b_sdma_dl_transfer_on_going = false;
bool b_ul_pause = false;
bool b_dl_pause = false;
bool b_process_dl_msg_on_going = false;

int dma_ch_ul=-1;
int dma_ch_dl=-1;

packet_info frames_info[IPC_4GHIF_MAX_NB_DL_FRAMES];
packet_info frames_info_for_receive_cb[IPC_4GHIF_MAX_NB_DL_FRAMES];
int sdma_ul_packet_nb = 0;
int sdma_dl_packet_nb = 0;
int incoming_dl_packet_nb = 0;
int incoming_dl_packet_nb_for_receive_cb = 0;

fp_bp_ready    p_bp_ready    = NULL;
fp_receive_dl  p_receive_dl  = NULL;
fp_dl_complete p_dl_complete = NULL;
fp_ul_complete p_ul_complete = NULL;
fp_ul_resume   p_ul_resume   = NULL;

void* p_sdma_packet_ul = NULL;
void* p_sdma_packet_dl = NULL;

union ktime tx_kt_fct_sdma_submit_ul;
union ktime tx_kt_before_consistent_sync;
union ktime tx_kt_start;
union ktime tx_kt_sdma_ul_cb;
union ktime tx_kt_delta;


/* FIFO variables */

u32 fifo_dl_read_ind  = 0; /* updated by AP - initialized by BP */
u32 fifo_ul_write_ind = 0; /* updated by AP - initialized by AP */
u32 fifo_dl_write_ind = 0; /* updated by BP - initialized by BP */
u32 fifo_ul_read_ind  = 0; /* updated by BP - initialized by AP */

fifo_msg fifo_data_dl_msg_table[IPC_4GHIF_MAX_NB_DL_FRAMES];
fifo_msg fifo_data_ul_msg_table[IPC_4GHIF_MAX_NB_UL_FRAMES];


/* FIFO ACK variables */

u32 fifo_ack_dl_read_ind  = 0; /* updated by AP - initialized by BP */
u32 fifo_ack_ul_write_ind = 0; /* updated by AP - initialized by AP */
u32 fifo_ack_dl_write_ind = 0; /* updated by BP - initialized by BP */
u32 fifo_ack_ul_read_ind  = 0; /* updated by BP - initialized by AP */


/* STAT variables */

ipc_4ghif_stats the_ipc_4ghif_stats;

/* critical sections to avoid simultaneous execution        */
/* allocate and free are also protected by critical section */

static spinlock_t ipc_4ghif_fifo_data_lock       = SPIN_LOCK_UNLOCKED;
static spinlock_t ipc_4ghif_fifo_ack_lock        = SPIN_LOCK_UNLOCKED;
static spinlock_t ipc_4ghif_process_msg_lock     = SPIN_LOCK_UNLOCKED;
static spinlock_t ipc_4ghif_mb_lock              = SPIN_LOCK_UNLOCKED;
static spinlock_t ipc_4ghif_process_ack_msg_lock = SPIN_LOCK_UNLOCKED;

/* dma descriptors array */
DMA_DESCR_STRUCT_TYPE_3_SD * dma_descr_dl_array = DMA_DESCR_DL_ARRAY_NON_CACHED;
DMA_DESCR_STRUCT_TYPE_3_SD * dma_descr_ul_array = DMA_DESCR_UL_ARRAY_NON_CACHED;

/*==============================================================================
  GLOBAL VARIABLES
==============================================================================*/

/*==============================================================================
  LOCAL FUNCTIONS
==============================================================================*/

/* 1. Tasklets used to call user callbacks outside interrupt time */

void dl_complete_tasklet_function(unsigned long param)
{

    TIMESTAMP_4GHIF(TS_SUBMIT_DL_CB,sdma_dl_packet_nb);
    ENTER_FUNC();

    /* indicate sDMA transfer is finished */
    b_sdma_dl_transfer_on_going=false;

    /* data driver callback*/
    if(p_dl_complete!=NULL) p_dl_complete(p_sdma_packet_dl);

    /* allow to process following FIFO_DL message */
    b_process_dl_msg_on_going = false;

    /* process the next FIFO_DL message (if there is one to process) */
    fifo_ack_4ghif_process_dl_msg();
    fifo_4ghif_process_dl_msg();
}


void ul_complete_tasklet_function(unsigned long param)
{
    TIMESTAMP_4GHIF(TS_SUBMIT_UL_CB,sdma_ul_packet_nb);
    ENTER_FUNC();

    /* indicate sDMA transfer is finished */
    b_sdma_ul_transfer_on_going=false;

    /* data driver callback*/
    if(p_ul_complete!=NULL) p_ul_complete(p_sdma_packet_ul);
}


void bp_ready_tasklet_function(unsigned long param)
{
    ENTER_FUNC();

    /* data driver callback*/
    if(p_bp_ready!=NULL) p_bp_ready();

    /* allow to process following FIFO_DL message */
    b_process_dl_msg_on_going = false;

    /* process the next FIFO_DL message (if there is one to process) */
    fifo_ack_4ghif_process_dl_msg();
    fifo_4ghif_process_dl_msg();
}


void receive_dl_tasklet_function(unsigned long param)
{
    TIMESTAMP_4GHIF(TS_NEW_MSG,incoming_dl_packet_nb_for_receive_cb);
    ENTER_FUNC();

    the_ipc_4ghif_stats.ipc_4ghif_stat_dl_notify++;

    /* data driver callback*/
    if(p_receive_dl!=NULL) p_receive_dl(incoming_dl_packet_nb_for_receive_cb ,frames_info_for_receive_cb);
}


void ul_resume_tasklet_function(unsigned long param)
{
    TIMESTAMP_4GHIF(TS_RESUME_CB,0);
    ENTER_FUNC();

    the_ipc_4ghif_stats.ipc_4ghif_stat_ul_resume_full++;

    /* data driver callback*/
    if(p_ul_resume!=NULL) p_ul_resume();
}


static DECLARE_TASKLET(dl_complete_tasklet, dl_complete_tasklet_function, (unsigned long) NULL);
static DECLARE_TASKLET(ul_complete_tasklet, ul_complete_tasklet_function, (unsigned long) NULL);
static DECLARE_TASKLET(bp_ready_tasklet   , bp_ready_tasklet_function   , (unsigned long) NULL);
static DECLARE_TASKLET(receive_dl_tasklet , receive_dl_tasklet_function , (unsigned long) NULL);
static DECLARE_TASKLET(ul_resume_tasklet  , ul_resume_tasklet_function  , (unsigned long) NULL);



/* 2. ISR / callback / processing functions */

/* ISR function called when an interruption occured on a mailbox (in our case, new message in mailbox 2) */
irqreturn_t mailbox_ISR_4GHIF(int irq, void *p)
{
    u32 i,NumMsg,mailbox_msg=0;
    HW_STATUS ret;

    ENTER_FUNC();

    ret = HW_MBOX_NumMsgGet((const u32)IO_ADDRESS(PRCM_MBOXES_BASE), HW_MBOX_ID_2, &NumMsg);

    /* new msg in mailbox 2*/
    if (ret != RET_OK) panic( "4G HIF ERROR in %s: impossible to get the number of messages in mailbox 2\n",__FUNCTION__);

    the_ipc_4ghif_stats.ipc_4ghif_stat_irq_nb++;

    for (i=0;i<NumMsg;i++)
    {
        /* empty mailbox 2 */
        HW_MBOX_MsgRead((const u32)IO_ADDRESS(PRCM_MBOXES_BASE), HW_MBOX_ID_2, &mailbox_msg);
        DEBUG("IPC 4GHIF: Receive_new_mailbox_msg = 0x%x\n",mailbox_msg);
    }

     /* clear events*/
     HW_MBOX_EventAck((const u32)IO_ADDRESS(PRCM_MBOXES_BASE), HW_MBOX_ID_2, HW_MBOX_U2_ARM, HW_MBOX_INT_NEW_MSG);

     /* Process the FIFO message */
     fifo_ack_4ghif_process_dl_msg();
     fifo_4ghif_process_dl_msg();

     return IRQ_HANDLED;
}

static void fifo_4ghif_process_dl_msg(void)
{
    FIFO_4GHIF_STATUS fifo_msg_status;
    unsigned long flags;
    fifo_msg fifo_dl_msg;

    ENTER_FUNC();

    spin_lock_irqsave(&ipc_4ghif_process_msg_lock,flags);

    /* exit if already frame in processing */
    if(b_process_dl_msg_on_going) { spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags); return; }

    if(b_bp_ready==false)
    {
        if(fifo_4ghif_get_msg(&fifo_dl_msg, 0)!=IPC_4GHIF_FIFO_STATUS_EMPTY)
        {
            if(fifo_dl_msg.type!=eIPC_MSG_TYPE_INIT_REQ)
            {
                spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags);
                panic("4GHIF ERROR in %s: Expected INIT_REQ but received %d message",__FUNCTION__,fifo_dl_msg.type);
                return;
            }

            DEBUG("4GHIF: receiving INIT_REQ\n");
            b_bp_ready=true;
            fifo_4ghif_increment_read_ind_after_get_msg(1);
            b_process_dl_msg_on_going=true;
            tasklet_schedule(&bp_ready_tasklet);
        }
    }
    else
    {
        /* Fill in Data DL msg table using FIFO msg information and get the number of frames available*/
        fifo_msg_status = fifo_4ghif_get_frame_info(&incoming_dl_packet_nb);

        /* Check errors*/
        if (fifo_msg_status != FIFO_4GHIF_STATUS_OK)
        {
            if (fifo_msg_status == FIFO_4GHIF_STATUS_INVALID_MSG_TYPE)
            {
                spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags);
                panic("4GHIF ERROR in %s: Expected IDL_DATA/MGMT/MON but received other message type",__FUNCTION__);
                return;
            }
            else if (fifo_msg_status == FIFO_4GHIF_STATUS_INVALID_MSG_LENGTH)
            {
                spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags);
                panic("4GHIF ERROR in %s: Incorrect data length",__FUNCTION__);
                return;
            }
            else
            {
                spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags);
                panic("4GHIF ERROR in %s: fifo_4ghif_get_frame_info returned an error\n",__FUNCTION__);
                return;
            }
        }

        /* Data DL msg table is filled in, now notify the data driver that some frames are available*/
        if(incoming_dl_packet_nb > 0) /*dl frames available*/
        {
            b_process_dl_msg_on_going=true;

            /* update the current number of packets to be transferred*/
	    incoming_dl_packet_nb_for_receive_cb = incoming_dl_packet_nb;

	    /*fill in the frame info to inform the data driver by the receive callback */
	    memcpy(frames_info_for_receive_cb,frames_info,incoming_dl_packet_nb_for_receive_cb*sizeof(packet_info));

            tasklet_schedule(&receive_dl_tasklet);
            /* FIFO DL read pointer will be incremented when we know the actual number of frames processed (in IPC_4GHIF_submit_dl)*/
        }
    }

    spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags);

    return;
}


static void fifo_ack_4ghif_process_dl_msg(void)
{
    unsigned long flags;
    fifo_msg fifo_ack_dl_msg;

    ENTER_FUNC();

    spin_lock_irqsave(&ipc_4ghif_process_ack_msg_lock,flags);

    /* read FIFO DL message
       It is possible there is no message in ACK FIFO:
       - if function is called from dl_complete_tasklet_function
       - if only message in DATA FIFO
       Note, there is no message type field in ACK FIFO (All messages are UL-ACK) */
    while(fifo_ack_4ghif_get_msg(&fifo_ack_dl_msg)!=IPC_4GHIF_FIFO_STATUS_EMPTY)
    {
        DEBUG("4GHIF: receiving ACK-UL of frame 0x%x\n",(u32) fifo_ack_dl_msg.start_address);
        the_ipc_4ghif_stats.ipc_4ghif_stat_ul_ack++;

        /* free  UL data shared memory*/
        IPC_4GHIF_data_ul_mm_free((u32) fifo_ack_dl_msg.start_address);

        /* If UL pause, see if can resume it: some room available in DATA UL FIFO */
        if(b_ul_pause && (fifo_4ghif_ul_space_left()>0))
        {
            b_ul_pause=false;
            tasklet_schedule(&ul_resume_tasklet);
        }

        fifo_ack_4ghif_increment_read_ind_after_get_msg();
    }

    spin_unlock_irqrestore(&ipc_4ghif_process_ack_msg_lock,flags);
}

static FIFO_4GHIF_STATUS fifo_4ghif_get_frame_info(u32 * frame_nb)
{
    FIFO_4GHIF_STATUS status = FIFO_4GHIF_STATUS_ERROR;
    u32 index=0;
    int ack_ul_space_left=0;
    fifo_msg fifo_dl_msg;

    /* check params*/
    if (frame_nb != NULL)
    {
        *frame_nb=0;
        ack_ul_space_left = fifo_ack_4ghif_ul_space_left();

        while( fifo_4ghif_get_msg(&fifo_dl_msg,(u8)index)!=IPC_4GHIF_FIFO_STATUS_EMPTY && index<IPC_4GHIF_MAX_NB_DL_FRAMES )
        {
            /* check message type*/
            if(fifo_dl_msg.type!=eIPC_MSG_TYPE_DL_DATA && fifo_dl_msg.type!=eIPC_MSG_TYPE_DL_MGMT && fifo_dl_msg.type!=eIPC_MSG_TYPE_DL_MON) return FIFO_4GHIF_STATUS_INVALID_MSG_TYPE;

            DEBUG("4GHIF: receiving DATA/MGMT/MON DL\n");

            /* check message length*/
            if ((fifo_dl_msg.length == 0)||(fifo_dl_msg.length > DATA_DL_PACKET_MAX_SIZE)) return FIFO_4GHIF_STATUS_INVALID_MSG_LENGTH;

            /* check there is some room to ack the current frame */
           if (ack_ul_space_left <= index) break;

            /*fill in the data DL msg table*/
            fifo_data_dl_msg_table[index].type          = fifo_dl_msg.type;
            fifo_data_dl_msg_table[index].tag           = fifo_dl_msg.tag;
            fifo_data_dl_msg_table[index].length        = fifo_dl_msg.length;
            fifo_data_dl_msg_table[index].start_address = fifo_dl_msg.start_address;

            /*fill in the frame info table to be provided to the data driver*/
            frames_info[index].packet_type = PKT_TYPE_DL_DATA;
            if(fifo_dl_msg.type==eIPC_MSG_TYPE_DL_MGMT) frames_info[index].packet_type = PKT_TYPE_DL_MGMT;
            if(fifo_dl_msg.type==eIPC_MSG_TYPE_DL_MON ) frames_info[index].packet_type = PKT_TYPE_DL_MON ;
            frames_info[index].buf_size    = fifo_dl_msg.length;

            index++;
        }
        *frame_nb = index;
        status = FIFO_4GHIF_STATUS_OK;
    }
    return status;
}

void sdma_4ghif_ul_callback(int lch, u16 ch_status, void *data)
{
    u32 ret, index;

    TIMESTAMP_4GHIF(TS_DMA_CB_UL,sdma_ul_packet_nb);
    ENTER_FUNC();

    the_ipc_4ghif_stats.ipc_4ghif_stat_ul_sdma_done++;

    /* free the UL used dma ch */
    omap_stop_dma(dma_ch_ul);
    omap_free_dma(dma_ch_ul);

    /* Add messages UL_DATA message in FIFO_UL */
    /* Space issue should not occur because room availablity in FIFO UL has been checked before sDMA transfer */
    for(index=0; index < sdma_ul_packet_nb ;index++)
    {
        if(fifo_4ghif_add_msg(&fifo_data_ul_msg_table[index])==IPC_4GHIF_FIFO_STATUS_FULL) panic("4GHIF ERROR in %s: no space in FIFO for DATA UL msg\n",__FUNCTION__);
    }

    /* Signal BP, new message is available in FIFO_UL via mailbox 3 */
    if(ipc_4ghif_bp_loopback_stub_mode==IPC_4GHIF_BP_LOOPBACK_STUB_DISABLED)
    {
        ret = mailbox_4ghif_send_msg(MAILBOX_DATA_MSG);

        DEBUG("4GHIF: Send_msg = 0x%x\n",MAILBOX_DATA_MSG);

        if(ret!=RET_OK) panic("4GHIF ERROR in %s: CANNOT_SEND_MSG BY mailbox 3\n",__FUNCTION__);
    }
    /* or stub the BP */
    else if(ipc_4ghif_bp_loopback_stub_mode==IPC_4GHIF_BP_LOOPBACK_STUB_ENABLED) simulate_loopback_bp_frames();
    else if(ipc_4ghif_bp_loopback_stub_mode==IPC_4GHIF_BP_LOOPBACK_STUB_ENABLED_ONLY_FOR_UL) simulate_ul_bp();

    /* Inform Data driver, transfer is done */
    tasklet_schedule(&ul_complete_tasklet);
}


void sdma_4ghif_dl_callback(int lch, u16 ch_status, void *data)
{
    fifo_msg fifo_ack_dl_msg;
    u32 ret,index;

    TIMESTAMP_4GHIF(TS_DMA_CB_DL,sdma_dl_packet_nb);
    ENTER_FUNC();
    the_ipc_4ghif_stats.ipc_4ghif_stat_dl_sdma_done++;

    /* free the DL used dma ch */
    omap_stop_dma(dma_ch_dl);
    omap_free_dma(dma_ch_dl);

    /* Ack the DL-DATA (and also DL_MGMT / DL-MON) to allow BP to free the DATA_DL memory */

    /* Add DL_ACK messages in FIFO_UL */
    for (index=0; index < sdma_dl_packet_nb; index++)
    {
        /* type, tag and length are not used in ACK FIFO*/
        fifo_ack_dl_msg.type = eIPC_MSG_TYPE_DL_ACK;
        fifo_ack_dl_msg.tag = fifo_data_dl_msg_table[index].tag;
        fifo_ack_dl_msg.length = fifo_data_dl_msg_table[index].length;
        fifo_ack_dl_msg.start_address = fifo_data_dl_msg_table[index].start_address;

        /*should not occur cause room availability for ACK msg has been checked*/
        if(fifo_ack_4ghif_add_msg(&fifo_ack_dl_msg)==IPC_4GHIF_FIFO_STATUS_FULL) panic("4GHIF ERROR in %s: no space in FIFO for ACK DL msg\n",__FUNCTION__);
    }

    /* Signal BP, new message is available in FIFO_UL via mailbox 3 */
    if(ipc_4ghif_bp_loopback_stub_mode==IPC_4GHIF_BP_LOOPBACK_STUB_DISABLED || ipc_4ghif_bp_loopback_stub_mode==IPC_4GHIF_BP_LOOPBACK_STUB_ENABLED_ONLY_FOR_UL)
    {
        ret = mailbox_4ghif_send_msg(MAILBOX_ACK_MSG);
        DEBUG("4GHIF: Send_msg = 0x%x\n",MAILBOX_DATA_MSG);
        if(ret!=RET_OK) panic("4GHIF ERROR in %s: CANNOT_SEND_MSG BY mailbox 3\n",__FUNCTION__);
    }
    /* or stub the BP */
    else if(ipc_4ghif_bp_loopback_stub_mode==IPC_4GHIF_BP_LOOPBACK_STUB_ENABLED) simulate_loopback_bp_acks();

    the_ipc_4ghif_stats.ipc_4ghif_stat_dl_ack += sdma_dl_packet_nb;

    /* Inform Data driver, data are available in provided buffer */
    tasklet_schedule(&dl_complete_tasklet);
}



/* 3. FIFO functions */


/* 3.1. FIFO index updates functions (shared <--> ind) */

/* initialize ul write index and dl read index in shared memory                       */
/* Do that in 32 bits to clear the 32 bits because next access will be done in 8 bits */
static inline void fifo_4ghif_init_ul_write_ul_read_ind_to_shared(void)
{
    *(volatile u32*)(FIFO_UL_WRITE_IND)=0;
    *(volatile u32*)(FIFO_UL_READ_IND)=0;
    /*dl read indexes are initialized by BP*/
}

/* update UL read index (shared->ind) */
static inline void fifo_4ghif_update_ul_read_ind_from_shared(void)
{
    fifo_ul_read_ind=*(volatile u8*)(FIFO_UL_READ_IND);
}


/* update UL write index (ind->shared) */
static inline void fifo_4ghif_update_ul_write_ind_to_shared(void)
{
    *(volatile u8*)(FIFO_UL_WRITE_IND)=fifo_ul_write_ind;
}


/* update DL read index (ind->shared) */
static inline void fifo_4ghif_update_dl_read_ind_to_shared(void)
{
    *(volatile u8*)(FIFO_DL_READ_IND)=fifo_dl_read_ind;
}


/* update DL write index (shared->ind) */
static inline void fifo_4ghif_update_dl_write_ind_from_shared(void)
{
    fifo_dl_write_ind=*(volatile u8*)(FIFO_DL_WRITE_IND);
}


/* FIFO ACK functions */
/* initialize ul write index and dl read index in shared memory*/
static inline void fifo_ack_4ghif_init_ul_write_ul_read_ind_to_shared(void)
{
    *(volatile u32*)(FIFO_ACK_UL_WRITE_IND)=0;
    *(volatile u32*)(FIFO_ACK_UL_READ_IND)=0;
    /*dl read indexes are initialized by BP*/
}


/* update UL read index (shared->ind) */
static inline void fifo_ack_4ghif_update_ul_read_ind_from_shared(void)
{
    fifo_ack_ul_read_ind=*(volatile u8*)(FIFO_ACK_UL_READ_IND);
}


/* update UL write index (ind->shared) */
static inline void fifo_ack_4ghif_update_ul_write_ind_to_shared(void)
{
    *(volatile u8*)(FIFO_ACK_UL_WRITE_IND)=fifo_ack_ul_write_ind;
}


/* update DL read index (ind->shared) */
static inline void fifo_ack_4ghif_update_dl_read_ind_to_shared(void)
{
    *(volatile u8*)(FIFO_ACK_DL_READ_IND)=fifo_ack_dl_read_ind;
}


/* update DL write index (shared->ind) */
static inline void fifo_ack_4ghif_update_dl_write_ind_from_shared(void)
{
    fifo_ack_dl_write_ind=*(volatile u8*)(FIFO_ACK_DL_WRITE_IND);
}



/* 3.2. FIFO space management functions */

static int fifo_4ghif_ul_space_left(void)
{
    int space, used;

    /* update UL read index (shared->ind) */
    fifo_4ghif_update_ul_read_ind_from_shared();

    /* compute used space */
    used=(fifo_ul_write_ind-fifo_ul_read_ind) % FIFO_UL_MSG_NB;

    /* deduce remaining space ( -1 cell needed to detect full status) */
    space=FIFO_UL_MSG_NB-1-used;
    if(space<0) space=0;

    DEBUG("4GHIF: FCT UL SPACE used=%d real_space=%d\n",used,space);

    return space;
}


static int fifo_ack_4ghif_ul_space_left(void)
{
    int space, used;

    /* update UL read index (shared->ind) */
    fifo_ack_4ghif_update_ul_read_ind_from_shared();

    /* compute used space */
    used=(fifo_ack_ul_write_ind-fifo_ack_ul_read_ind) % FIFO_ACK_UL_MSG_NB;

    /* deduce remaining space ( -1 cell needed to detect full status) */
    space=FIFO_ACK_UL_MSG_NB-1-used;
    if(space<0) space=0;

    DEBUG("4GHIF: FCT UL ACK SPACE used=%d real_space=%d\n",used,space);

    return space;
}

/* 3.3. FIFO general functions */

/* about FIFO functions protection by critiacal sections:
   - init: not protected (not needed)
   - get & inc: not protected (because protected by the process & process ack fcts)
   - add: protected
*/

static IPC_4GHIF_FIFO_STATUS fifo_4ghif_init(void)
{
    /* init the 2 FIFO indexes managed by the AP (the 2 others are managed by BP and are loaded before their use) */
    fifo_dl_read_ind  = 0;
    fifo_ul_write_ind = 0;

    /* clear UL write index and DL read index in shared memory (ind->shared) */
    fifo_4ghif_init_ul_write_ul_read_ind_to_shared();

    return IPC_4GHIF_FIFO_STATUS_OK;
}

static IPC_4GHIF_FIFO_STATUS fifo_ack_4ghif_init(void)
{
    /* init the 2 FIFO indexes managed by the AP (the 2 others are managed by BP and are loaded before their use) */
    fifo_ack_dl_read_ind  = 0;
    fifo_ack_ul_write_ind = 0;

    /* clear UL write index and DL read index in shared memory (ind->shared) */
    fifo_ack_4ghif_init_ul_write_ul_read_ind_to_shared();

    return IPC_4GHIF_FIFO_STATUS_OK;
}

/*
   Get the a message from the FIFO-DL

   Warning:
   read index is not incremented in this function
   (because it should not be the case in one particular case),
   so need to call fifo_4ghif_increment_read_ind_after_get_msg
*/
static IPC_4GHIF_FIFO_STATUS fifo_4ghif_get_msg(fifo_msg* msg, u8 offset)
{
    u32 fifo_dl_read_ind_w_offset;
    volatile u8* fifo_dl_read_ptr;

    ENTER_FUNC();

    /* check params */
    if(msg==NULL) panic("4GHIF ERROR in %s: NULL FIFO msg \n", __FUNCTION__);

    /* update DL write index (shared->ind) */
    fifo_4ghif_update_dl_write_ind_from_shared();

    /* point on the new message */
    fifo_dl_read_ind_w_offset = fifo_dl_read_ind + offset;
    if(fifo_dl_read_ind_w_offset>=FIFO_DL_MSG_NB) fifo_dl_read_ind_w_offset-=FIFO_DL_MSG_NB;
    fifo_dl_read_ptr = (u8*) (FIFO_DL_BASE + fifo_dl_read_ind_w_offset * sizeof(fifo_msg));

    /* check there is a new message */
    if(fifo_dl_write_ind==fifo_dl_read_ind_w_offset) return IPC_4GHIF_FIFO_STATUS_EMPTY;

    /* read the message
      TYPE_8.TAG_8.LENGHT_16   BIG 0123   LITTLE 3210
      START_ADDRESS_32             4567          7654
    */
    msg->type=fifo_dl_read_ptr[0];
    msg->tag=fifo_dl_read_ptr[1];
    msg->length=(fifo_dl_read_ptr[3]<<8)+(fifo_dl_read_ptr[2]);
    msg->start_address=(void*)(fifo_dl_read_ptr[7]<<24)+(fifo_dl_read_ptr[6]<<16)+(fifo_dl_read_ptr[5]<<8)+(fifo_dl_read_ptr[4]);

    return IPC_4GHIF_FIFO_STATUS_OK;
}


static IPC_4GHIF_FIFO_STATUS fifo_ack_4ghif_get_msg(fifo_msg* msg)
{
    volatile u8* fifo_ack_dl_read_ptr;

    ENTER_FUNC();

    /* check params */
    if(msg==NULL) panic("4GHIF ERROR in %s: NULL ACK FIFO msg\n",__FUNCTION__);

    /* update DL write index (shared->ind) */
    fifo_ack_4ghif_update_dl_write_ind_from_shared();

    /* check there is a new message */
    if(fifo_ack_dl_write_ind==fifo_ack_dl_read_ind) return IPC_4GHIF_FIFO_STATUS_EMPTY;

    /* read the message
      not used for FIFO ACK: TYPE_8.TAG_8.LENGHT_16   BIG 0123   LITTLE 3210
      START_ADDRESS_32                                    4567          7654
    */

    fifo_ack_dl_read_ptr = (u8*) (FIFO_ACK_DL_BASE + 4 * fifo_ack_dl_read_ind);

    msg->type=eIPC_MSG_TYPE_UL_ACK;
    msg->tag=0;
    msg->length=0;
    msg->start_address=(void*)(fifo_ack_dl_read_ptr[3]<<24)+(fifo_ack_dl_read_ptr[2]<<16)+(fifo_ack_dl_read_ptr[1]<<8)+(fifo_ack_dl_read_ptr[0]);

    return IPC_4GHIF_FIFO_STATUS_OK;
}

static void fifo_4ghif_increment_read_ind_after_get_msg(u8 offset)
{
    ENTER_FUNC();

    /* increment read index */
    fifo_dl_read_ind += offset;
    if(fifo_dl_read_ind>=FIFO_DL_MSG_NB) fifo_dl_read_ind-=FIFO_DL_MSG_NB;

    /* update DL read index (ind->shared) */
    fifo_4ghif_update_dl_read_ind_to_shared();
}


static void fifo_ack_4ghif_increment_read_ind_after_get_msg(void)
{
    ENTER_FUNC();

    /* increment read index */
    fifo_ack_dl_read_ind++;
    if(fifo_ack_dl_read_ind>=FIFO_ACK_DL_MSG_NB) fifo_ack_dl_read_ind=0;

    /* update DL read index (ind->shared) */
    fifo_ack_4ghif_update_dl_read_ind_to_shared();
}


static IPC_4GHIF_FIFO_STATUS fifo_4ghif_add_msg(fifo_msg* msg)
{
    unsigned long flags;
    volatile u8* fifo_ul_write_ptr;

    ENTER_FUNC();

    spin_lock_irqsave(&ipc_4ghif_fifo_data_lock,flags);

    /* check params */
    if(msg==NULL)
    {
        spin_unlock_irqrestore(&ipc_4ghif_fifo_data_lock,flags);
        panic("4GHIF ERROR in %s: NULL FIFO msg\n",__FUNCTION__);
    }

    /* check there is a space in FIFO */
    if(fifo_4ghif_ul_space_left()==0)
    {
        spin_unlock_irqrestore(&ipc_4ghif_fifo_data_lock,flags);
        return IPC_4GHIF_FIFO_STATUS_FULL;
    }

    /* write the message
      TYPE_8.TAG_8.LENGHT_16   BIG 0123   LITTLE 3210
      START_ADDRESS_32             4567          7654
    */

    fifo_ul_write_ptr= (u8*) (FIFO_UL_BASE + fifo_ul_write_ind * sizeof(fifo_msg));

    fifo_ul_write_ptr[0]=msg->type;
    fifo_ul_write_ptr[1]=msg->tag;
    fifo_ul_write_ptr[2]=msg->length;
    fifo_ul_write_ptr[3]=(msg->length)>>8;

    fifo_ul_write_ptr[7]=(u8)( ((u32)(msg->start_address))>>24);
    fifo_ul_write_ptr[6]=(u8)( ((u32)(msg->start_address))>>16);
    fifo_ul_write_ptr[5]=(u8)( ((u32)(msg->start_address))>>8);
    fifo_ul_write_ptr[4]=(u8)( (u32) msg->start_address);

    /* increment read index and update it in shared memory */
    fifo_ul_write_ind++;
    if(fifo_ul_write_ind>=FIFO_UL_MSG_NB) fifo_ul_write_ind=0;

    /* update UL write index (ind->shared) */
    fifo_4ghif_update_ul_write_ind_to_shared();

    spin_unlock_irqrestore(&ipc_4ghif_fifo_data_lock,flags);
    return IPC_4GHIF_FIFO_STATUS_OK;
}


static IPC_4GHIF_FIFO_STATUS fifo_ack_4ghif_add_msg(fifo_msg* msg)
{
    unsigned long flags;
    volatile u8* fifo_ack_ul_write_ptr;

    ENTER_FUNC();

    spin_lock_irqsave(&ipc_4ghif_fifo_ack_lock,flags);

    /* check params */
    if(msg==NULL)
    {
        spin_unlock_irqrestore(&ipc_4ghif_fifo_ack_lock,flags);
        panic("4GHIF ERROR in %s: NULL FIFO ACK msg\n",__FUNCTION__);
    }

    /* check there is a space in FIFO */
    if(fifo_ack_4ghif_ul_space_left()==0)
    {
        spin_unlock_irqrestore(&ipc_4ghif_fifo_ack_lock,flags);
        return IPC_4GHIF_FIFO_STATUS_FULL;
    }

    /* write the message
      not used for FIFO ACK: TYPE_8.TAG_8.LENGHT_16   BIG 0123   LITTLE 3210
      START_ADDRESS_32                                    4567          7654
    */

    fifo_ack_ul_write_ptr= (u8*) (FIFO_ACK_UL_BASE + 4 * fifo_ack_ul_write_ind);

    fifo_ack_ul_write_ptr[3]=(u8)( ((u32)(msg->start_address))>>24);
    fifo_ack_ul_write_ptr[2]=(u8)( ((u32)(msg->start_address))>>16);
    fifo_ack_ul_write_ptr[1]=(u8)( ((u32)(msg->start_address))>>8);
    fifo_ack_ul_write_ptr[0]=(u8)( (u32) msg->start_address);

    /* increment read index and update it in shared memory */
    fifo_ack_ul_write_ind++;
    if(fifo_ack_ul_write_ind>=FIFO_ACK_UL_MSG_NB) fifo_ack_ul_write_ind=0;

    /* update UL write index (ind->shared) */
    fifo_ack_4ghif_update_ul_write_ind_to_shared();

    spin_unlock_irqrestore(&ipc_4ghif_fifo_ack_lock,flags);
    return IPC_4GHIF_FIFO_STATUS_OK;
}

/* 4. Mailboxes functions */

static int mailbox_4ghif_send_msg(u32 msg)
{
    unsigned long flags;
    int ret;

    ENTER_FUNC();

    spin_lock_irqsave(&ipc_4ghif_mb_lock,flags);
    ret = HW_MBOX_MsgWrite((const u32)IO_ADDRESS(PRCM_MBOXES_BASE), HW_MBOX_ID_3, msg);
    spin_unlock_irqrestore(&ipc_4ghif_mb_lock,flags);

    return ret;
}

/* 5. Linked lists functions */
static void fill_one_type3sd_descr(DMA_DESCR_STRUCT_TYPE_3_SD* t3_sd_descr, u32 src, u32 dst, u32 len, bool last)
{
    u32 descr_phys_adr;
    descr_phys_adr=(((u32)t3_sd_descr&(0x000FFFFF))|(IPC_SHARED_PHYS_ADDR&(0xFFF00000)));

    t3_sd_descr->src_start_addr=src;
    t3_sd_descr->dst_start_addr=dst;
    t3_sd_descr->length_and_next_descr_type=NEXT_DESCR_TYPE3SD|(len&0x00FFFFFF); /*length is limited to 24bits*/
    if(last) t3_sd_descr->length_and_next_descr_type|=NEXT_DESCR_LAST;
    t3_sd_descr->next_descr_addr_ptr=(u32)(descr_phys_adr+sizeof(DMA_DESCR_STRUCT_TYPE_3_SD));
}


/*==============================================================================
  GLOBAL FUNCTIONS
==============================================================================*/

/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\//\/\/\/\/\/\/\/\/\ API functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

Function                        Description

IPC_4GHIF_open                     To initialize and open 4G HIF IPC HW resources. To register callbacks
IPC_4GHIF_close                    To close and free 4G HIF IPC HW resources. To unregister callbacks
IPC_4GHIF_ready                    To signal to BP, AP data driver is ready
IPC_4GHIF_submit_ul                To send a frame to the BP
IPC_4GHIF_submit_dl                To receive a frame from the BP


Callback                        Description

IPC_4GHIF_bp_ready_callback        Scheduled when BP 4GHIF IPC is ready
IPC_4GHIF_receive_dl_callback      Scheduled when a new DL frame from BP was received
IPC_4GHIF_dl_complete_callback     Scheduled when DL frame has been transferred from shared memory to user skbuff
IPC_4GHIF_ul_complete_callback     Scheduled when UL frame has been transferred from user skbuff to shared memory.
IPC_4GHIF_ul_resume_callback       Scheduled when resources are available again after an unabaility to send an UL frame.

*/


/*=====================================================================================================================
Name: IPC_4GHIF_open
Description:
-    This function initializes the 4G HIF IPC HW resources (mailboxes, sDMA and shared memory).
-    And registers callbacks
Parameters:
-    Structure containing callback addresses.
Return value:
-    IPC_IPC_4GHIF_STATUS_OK
-    IPC_4GHIF_STATUS_ALREADY_OPEN: IPC already opened previously
=====================================================================================================================*/
IPC_4GHIF_STATUS IPC_4GHIF_open(ipc_4ghif_callbacks callbacks)
{
    ENTER_FUNC();

    if(b_ipc_4ghif_open!=false) return IPC_4GHIF_STATUS_ALREADY_OPEN;

    /* register callbacks (before IRQ enable in case a msg has been already received on MB2) */

    p_bp_ready=callbacks.bp_ready;
    p_receive_dl=callbacks.receive_dl;
    p_dl_complete=callbacks.dl_complete;
    p_ul_complete=callbacks.ul_complete;
    p_ul_resume=callbacks.ul_resume;

    if(b_bp_ready==false) { fifo_4ghif_init(); fifo_ack_4ghif_init(); IPC_4GHIF_data_ul_mm_init(); }

    /* 4ghif ipc is opened */

    b_ipc_4ghif_open = true;

    /*register mailbox ISR*/

    if (request_irq(INT_24XX_MAIL_U2_MPU, mailbox_ISR_4GHIF, IRQF_DISABLED,"mailbox_4ghif", NULL))
    {
        panic("4GHIF ERROR in %s: unable to register MAILBOX IPC 4GHIF interrupt!\n",__FUNCTION__);
    }

    /*open MB 2,3*/

    /*clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(PRCM_MBOXES_BASE),HW_MBOX_ID_2,HW_MBOX_U2_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventAck((const u32)IO_ADDRESS(PRCM_MBOXES_BASE),HW_MBOX_ID_3,HW_MBOX_U2_ARM,HW_MBOX_INT_ALL);

    /*disable mailboxes*/
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(PRCM_MBOXES_BASE),HW_MBOX_ID_2,HW_MBOX_U2_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(PRCM_MBOXES_BASE),HW_MBOX_ID_3,HW_MBOX_U2_ARM,HW_MBOX_INT_ALL);
    /*enable mailbox 2*/
    HW_MBOX_EventEnable((const u32)IO_ADDRESS(PRCM_MBOXES_BASE),HW_MBOX_ID_2,HW_MBOX_U2_ARM,HW_MBOX_INT_NEW_MSG);

    /* clear stats */
    memset(&the_ipc_4ghif_stats,0,sizeof(ipc_4ghif_stats));

    /* schedule bp ready callback if bp already signaled it is ready
       (occured when data driver close and re-open the ipc 4ghif
       else bp ready callback will be scheduled as soon as bp signal it is ready */
    if(b_bp_ready) tasklet_schedule(&bp_ready_tasklet);

    return IPC_4GHIF_STATUS_OK;
}


/*=====================================================================================================================
Name: IPC_4GHIF_close
Description:
-    This function frees the 4G HIF IPC HW resources (mailboxes, sDMA and shared memory).
-    And unregisters the callbacks
Parameters:
-    None
Return value:
-    IPC_4GHIF_STATUS_OK
-    IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes:
-    After closing, pending callbacks won't be scheduled.
=====================================================================================================================*/
IPC_4GHIF_STATUS IPC_4GHIF_close(void)
{
    ENTER_FUNC();

    if(b_ipc_4ghif_open==false) return IPC_4GHIF_STATUS_CLOSED;

    /* 4ghif ipc is closed */

    b_ipc_4ghif_open = false;

    /*close MB 2,3*/

    /*disable mailboxes*/
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(PRCM_MBOXES_BASE),HW_MBOX_ID_2,HW_MBOX_U2_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventDisable((const u32)IO_ADDRESS(PRCM_MBOXES_BASE),HW_MBOX_ID_3,HW_MBOX_U2_ARM,HW_MBOX_INT_ALL);

    /*clear events*/
    HW_MBOX_EventAck((const u32)IO_ADDRESS(PRCM_MBOXES_BASE),HW_MBOX_ID_2,HW_MBOX_U2_ARM,HW_MBOX_INT_ALL);
    HW_MBOX_EventAck((const u32)IO_ADDRESS(PRCM_MBOXES_BASE),HW_MBOX_ID_3,HW_MBOX_U2_ARM,HW_MBOX_INT_ALL);

    /*unregister IT*/

    free_irq(INT_24XX_MAIL_U2_MPU, NULL);

    /* unregister callbacks */

    p_bp_ready=NULL;
    p_receive_dl=NULL;
    p_dl_complete=NULL;
    p_ul_complete=NULL;
    p_ul_resume=NULL;

    return IPC_4GHIF_STATUS_OK;
}


/*=====================================================================================================================
Name: IPC_4GHIF_ready
Description:
-    This function signals to the BP that AP data driver is ready by sending INIT_CNF message to the BP.
-       It should be called after receiving the bp_ready callback and when data driver is ready.
Parameters:
-    None
Return value:
-    IPC_4GHIF_STATUS_OK: function sends the message
-    IPC_4GHIF_STATUS_BP_NOT_READY: BP did not reported yet it is ready
-    IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes:
-    INIT_CNF message is only transmitted once to the BP. For instance
        if Data driver closes and opens again the IPC 4GHIF low level driver then INIT_CNF message won't be sent.
=====================================================================================================================*/
IPC_4GHIF_STATUS IPC_4GHIF_ready(void)
{
    fifo_msg init_cnf_msg;

    ENTER_FUNC();

    if(b_ipc_4ghif_open==false) return IPC_4GHIF_STATUS_CLOSED;

    if(b_bp_ready==false) return IPC_4GHIF_STATUS_BP_NOT_READY;

    if(b_ap_ready==false)
    {
        init_cnf_msg.type=eIPC_MSG_TYPE_INIT_CNF;
        init_cnf_msg.tag=0;
        init_cnf_msg.length=0;
        init_cnf_msg.start_address=NULL;

        if(fifo_4ghif_add_msg(&init_cnf_msg)==IPC_4GHIF_FIFO_STATUS_FULL) panic("4GHIF ERROR in %s: no space in FIFO for INIT CNF MSG\n",__FUNCTION__);

        /* Signal BP that the AP is ready via mailbox 3 */
        mailbox_4ghif_send_msg(MAILBOX_READY_MSG);
    }

    b_ap_ready=true;

    return IPC_4GHIF_STATUS_OK;
}

/*=====================================================================================================================
Name: IPC_4GHIF_get_dl_frames_nb
Description:
-        This function can be used to get the current number and characteristics of available DL packets in the shared memory.
Parameters:
-        u8* packet_nb: number of available packet.
-        packet_info  frames_info[IPC_4GHIF_MAX_NB_DL_FRAMES]: characteristics of each available packet
Return value:
-        None
Notes:
-        This function can be called to poll the availability of new packets whereas polling method is not the preferred solution.
-        If the caller waits between IPC_4GHIF_receive_dl_callback and IPC_4GHIF_submit_dl function, it can get an updated state by calling this function
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_get_dl_frames_nb(u8* packet_nb, packet_info frames_info_array[IPC_4GHIF_MAX_NB_DL_FRAMES])
{
    FIFO_4GHIF_STATUS fifo_msg_status;
    unsigned long flags;

    ENTER_FUNC();

    spin_lock_irqsave(&ipc_4ghif_process_msg_lock,flags);

    /* exit if already frame in processing */
    if(b_sdma_dl_transfer_on_going) { spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags); return IPC_4GHIF_STATUS_BUSY; }

    /* Fill in Data DL msg table using FIFO msg information and get the number of frames available*/
    fifo_msg_status = fifo_4ghif_get_frame_info(&incoming_dl_packet_nb);

     /* Check errors*/
     if (fifo_msg_status != FIFO_4GHIF_STATUS_OK)
     {
         if (fifo_msg_status == FIFO_4GHIF_STATUS_INVALID_MSG_TYPE)
         {
             spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags);
             panic("4GHIF ERROR in %s: Expected IDL_DATA/MGMT/MON but received other message type",__FUNCTION__);
             return IPC_4GHIF_STATUS_INVALID_PARAM;
         }
         else if (fifo_msg_status == FIFO_4GHIF_STATUS_INVALID_MSG_LENGTH)
         {
             spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags);
             panic("4GHIF ERROR in %s: Incorrect data length",__FUNCTION__);
             return IPC_4GHIF_STATUS_INVALID_PARAM;
         }
         else
         {
             spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags);
             panic("4GHIF ERROR in %s: fifo_4ghif_get_frame_info returned an error\n",__FUNCTION__);
             return IPC_4GHIF_STATUS_INVALID_PARAM;
         }
     }

     /* Data DL msg table is filled in, now notify the data driver that some frames are available*/
     if(incoming_dl_packet_nb > 0) /*dl frames available*/
     {
        b_process_dl_msg_on_going=true;

        /* update the current number of packets to be transferred*/
        *packet_nb=incoming_dl_packet_nb;

	/*fill in the frame info for the data driver */
	memcpy(frames_info_array,frames_info,incoming_dl_packet_nb*sizeof(packet_info));

        /* FIFO DL read pointer will be incremented when we know the actual number of frames processed (in IPC_4GHIF_submit_dl)*/
    }

    spin_unlock_irqrestore(&ipc_4ghif_process_msg_lock,flags);

    return IPC_4GHIF_STATUS_OK;
}


/*=====================================================================================================================
Name: IPC_4GHIF_submit_dl
Description:
-        This function is used to start the transfer of a group of frames from shared memory to skbuff using sDMA
Parameters:
-        u8 packet_nb: number of frames to transfer.
-        sdma_packet* packet[]: array of pointers on  structure containing pointer to the data to transfer. On call entry, the pbuf and buf_size fields are initialized.
-        u8* processing_packet_nb: updated with the number of frames actually in transfer.
Return value:
-        IPC_4GHIF_STATUS_OK: transfer successfully started
-        IPC_4GHIF_STATUS_BUSY:  previous DL transferred not ended
-        IPC_4GHIF_STATUS_INVALID_PARAM: NULL pointer or size
-        IPC_4GHIF_STATUS_NO_FRAME: No DL frame available
-        IPC_4GHIF_STATUS_BP_NOT_READY: BP did not reported yet it is ready
-        IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes:
-        This function should be called after being informed by IPC_4GHIF_receive_dl_callback that a new DL frame is available
-        At the end of sDMA transfer, the callback registered by IPC_4GHIF_dl_complete_callback is scheduled
-        A new submit command must not be requested until the callback is called.
-        Callback function is called with the address of the packet structure array passed as parameter.
-        If caller wants to use the packet structure array in the callback, it should be still available on caller side.
-        Function panics if cannot obtain a DMA channel (should not occur)


=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_submit_dl(u8 packet_nb, sdma_packet* packet[], u8* processing_packet_nb)
{
    IPC_4GHIF_STATUS ret = IPC_4GHIF_STATUS_OK;
    u32 index;
    bool last;
    u32 descr_phys_adr;

    TIMESTAMP_4GHIF(TS_SUBMIT_DL,0);
    ENTER_FUNC();

    *processing_packet_nb = 0;

    /* 1. check error cases */

    if(b_ipc_4ghif_open==false) return IPC_4GHIF_STATUS_CLOSED;

    if(b_bp_ready==false) return IPC_4GHIF_STATUS_BP_NOT_READY;
    if(b_ap_ready==false) return IPC_4GHIF_STATUS_AP_NOT_READY;

    if(b_sdma_dl_transfer_on_going!=false) return IPC_4GHIF_STATUS_BUSY;

    if(b_process_dl_msg_on_going==false) return IPC_4GHIF_STATUS_NO_FRAME;

    if((packet==NULL) || (packet_nb ==0) || (packet_nb > IPC_4GHIF_MAX_NB_DL_FRAMES) || (processing_packet_nb==NULL ))return IPC_4GHIF_STATUS_INVALID_PARAM;

    sdma_dl_packet_nb = incoming_dl_packet_nb;
    if (packet_nb < incoming_dl_packet_nb) sdma_dl_packet_nb = packet_nb;

    for (index=0;index < sdma_dl_packet_nb ; index ++)
    {
        /* check the skbuf is big enough to store the current frame*/
        if (packet[index]->buf_size==0 || packet[index]->buf_size < (u16)fifo_data_dl_msg_table[index].length) return IPC_4GHIF_STATUS_INVALID_PARAM;

        /* check provided buffer exists*/
        if(packet[index]->pbuf==NULL) return IPC_4GHIF_STATUS_INVALID_PARAM;
    }


    /* 2. init & start transfer */
    b_sdma_dl_transfer_on_going=true;

    /* update return parameters and keep a pointer on the packets to return it in the callback */
    *processing_packet_nb = sdma_dl_packet_nb;
    for (index=0;index < sdma_dl_packet_nb ; index ++)
    {
        packet[index]->buf_size = (u16)fifo_data_dl_msg_table[index].length;
        packet[index]->packet_type = frames_info[index].packet_type;
        packet[index]->transferred_status = true;
        packet[index]->bearerID = fifo_data_dl_msg_table[index].tag;
    }
    p_sdma_packet_dl=packet;

    /* Since we know how many frames are in processing, we can update the FIFO DL read pointer*/
    fifo_4ghif_increment_read_ind_after_get_msg((u8)sdma_dl_packet_nb);

    /* alloc DMA channel */
    if (omap_request_dma(IPC_DMA_4GHIF_DL_ID, NULL, sdma_4ghif_dl_callback, NULL, &dma_ch_dl) != 0) panic("4GHIF ERROR in %s: Cannot obtain sDMA CH for DL\n",__FUNCTION__);

    /* compute parameters (create SDMA descriptors) and start sDMA transfer */
    last=false;
    for (index=0;index < sdma_dl_packet_nb ; index ++)
    {
        /* synchronize the cache*/
        dmac_inv_range( (void*)(packet[index]->pbuf), (void*)((u32)(packet[index]->pbuf)+(u32)(packet[index]->buf_size)) );
        outer_inv_range( (u32)(packet[index]->pbuf),   (u32)(packet[index]->pbuf)+(u32)(packet[index]->buf_size) );

        /* fill in the descriptors for the DMA linked list trabsfer*/
        if(index==((sdma_dl_packet_nb)-1)) last=true;
        fill_one_type3sd_descr(&dma_descr_dl_array[index],(u32)fifo_data_dl_msg_table[index].start_address, (u32)((void*)virt_to_phys(packet[index]->pbuf)),packet[index]->buf_size,last);

        /* some stats */
        the_ipc_4ghif_stats.ipc_4ghif_stat_dl_packets++;
        the_ipc_4ghif_stats.ipc_4ghif_stat_dl_bytes += (u32)packet[index]->buf_size;
	if(the_ipc_4ghif_stats.ipc_4ghif_stat_dl_bytes>=ONE_GIGABYTE){the_ipc_4ghif_stats.ipc_4ghif_stat_dl_bytes-=ONE_GIGABYTE; the_ipc_4ghif_stats.ipc_4ghif_stat_dl_gbytes++; }
    }
    /* fill in the last descriptor*/
    (dma_descr_dl_array[sdma_dl_packet_nb]).next_descr_addr_ptr=END_OF_DMA_TYPE3_LINKLIST;


    /* configure DMA channel and start DMA transfer */
    descr_phys_adr=SHARED_MEM_VIRT_TO_PHYS(dma_descr_dl_array);
    OMAP_DMA_4GHIF_DL(dma_ch_dl,descr_phys_adr);


    /* some stats */
    the_ipc_4ghif_stats.ipc_4ghif_stat_dl_sdma_start++;

    return ret;
}


/*=====================================================================================================================
Name: IPC_4GHIF_submit_ul
Description:
-        This function is used to start a frame transfer from skbuff to shared memory using sDMA and then to the BP.
Parameters:
-        u8 packet_nb: number of frames to transmit to the BP
-        sdma_packet* packet[]: array of pointer on structures containing addresses of  frames to transfer.
-        u8* processing_packet_nb: number of packet that will be send according available ressources
Return value:
-        IPC_4GHIF_STATUS_OK: transfer successfully started
-        IPC_4GHIF_STATUS_BUSY:  previous UL transferred not ended
-        IPC_4GHIF_STATUS_ERROR: NULL pointer or size
-        IPC_4GHIF_STATUS_RESOURCES_FULL: Cannot transmit the packet because shared memory (DATA_UL or FIFO_UL) is full
-        IPC_4GHIF_STATUS_BP_NOT_READY: BP did not reported yet it is ready
-        IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes:
-        At the end of sDMA transfer, the callback registered by IPC_4GHIF_ul_complete_callback is invoked.
-        A new submit must not be requested until the callback is called.
-        Callback function is called with the array of the packet parameters.
-        If caller wants to use the array of packet structures in the callback, it should be still available on caller side.
-        Function panics if cannot obtain a DMA channel (should not occur)
-        According to IPC resources availability, IPC will not always send all the submitted packets. The returned value in processing_packet_nb indicates the number of packets that are actually sent.
-        If it is not possible to send all the provided packets because shared memory (Data or FIFO) is full, then the function returns the IPC_4GHIF_STATUS_RESOURCES_FULL error. In that case the IPC_4GHIF_ul_resume_callback will be scheduled as soon as space will be available in shared memory. However, IPC will transfer the frames that can be transmitted (in the array order) according to the room available.
-        If less frames than expected are transferred, then the user should wait for the sdma_ul callback and then for the resume_ul callback.
-        A packet of the array cannot be sent if the previous packet in the array cannot be sent. The order in the queue is kept the same. For instance if the memory manager cannot allocate a 2KB block but can allocate a 512B block, no more packet will be transferred, even if a 512B packet is queued. The user will have to wait for more resources available to submit the remaining packets again.
-        In uplink direction, it is needed to indicate the bearer of the frame to the BP.
-        In any case, IPC will not send more packets than IPC_4GHIF_MAX_NB_UL/DL_FRAMES constants
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_submit_ul(u8 packet_nb, sdma_packet* packet[],u8* processing_packet_nb)
{
    IPC_4GHIF_STATUS ret=IPC_4GHIF_STATUS_OK;
    u32 index;
    u32 adr[IPC_4GHIF_MAX_NB_UL_FRAMES];
    bool last;
    unsigned long flags;
    u32 descr_phys_adr;


    TIMESTAMP_4GHIF(TS_SUBMIT_UL,0);
    ENTER_FUNC();

    *processing_packet_nb = 0;


    /* 1. check error cases */

    if(b_ipc_4ghif_open==false) return IPC_4GHIF_STATUS_CLOSED;

    if(b_bp_ready==false) return IPC_4GHIF_STATUS_BP_NOT_READY;
    if(b_ap_ready==false) return IPC_4GHIF_STATUS_AP_NOT_READY;

    if(b_sdma_ul_transfer_on_going!=false) return IPC_4GHIF_STATUS_BUSY;

    if(packet==NULL) return IPC_4GHIF_STATUS_INVALID_PARAM;

    if((packet==NULL) || (packet_nb == 0) || (packet_nb > IPC_4GHIF_MAX_NB_UL_FRAMES) || (processing_packet_nb==NULL ))return IPC_4GHIF_STATUS_INVALID_PARAM;

    for (index=0; index < packet_nb; index++)
    {
        if(packet[index]->pbuf==NULL || packet[index]->buf_size==0 || packet[index]->buf_size>DATA_UL_PACKET_MAX_SIZE) return IPC_4GHIF_STATUS_INVALID_PARAM;

        if( (packet[index]->packet_type!=PKT_TYPE_UL_DATA) && (packet[index]->packet_type!=PKT_TYPE_UL_MGMT) ) return IPC_4GHIF_STATUS_INVALID_PARAM;
    }


    /* 2. reserves FIFO-UL and DATA-UL ressources */

    for (index=0; index < packet_nb; index++)
    {
        packet[index]->transferred_status=false;

        /* pause if no room in FIFO-UL */
        spin_lock_irqsave(&ipc_4ghif_fifo_data_lock,flags);

        if(fifo_4ghif_ul_space_left() <= index)
        {
            b_ul_pause = true;
            ret = IPC_4GHIF_STATUS_RESOURCES_FULL;
            the_ipc_4ghif_stats.ipc_4ghif_stat_ul_submit_full++;

            spin_unlock_irqrestore(&ipc_4ghif_fifo_data_lock,flags);
	    TIMESTAMP_4GHIF(TS_RESOURCES_FULL,FIFO_FULL);
            break;
        }

        spin_unlock_irqrestore(&ipc_4ghif_fifo_data_lock,flags);

        /*allocate shared buffers in UL-DATA memory*/
        if((adr[index]=(u32)(IPC_4GHIF_data_ul_mm_alloc(packet[index]->buf_size)))==0)
        {
            b_ul_pause = true;
            ret = IPC_4GHIF_STATUS_RESOURCES_FULL;
            the_ipc_4ghif_stats.ipc_4ghif_stat_ul_submit_full++;
	    TIMESTAMP_4GHIF(TS_RESOURCES_FULL,MEM_FULL);
            break;
        }

        packet[index]->transferred_status=true;
        (*processing_packet_nb)++;
    }

    if( (*processing_packet_nb)==0 ) return  IPC_4GHIF_STATUS_RESOURCES_FULL;

    /* keep the packet pointer to return it during the callback */
    p_sdma_packet_ul=packet;
    sdma_ul_packet_nb=*processing_packet_nb;


    /* 3. prepare the FIFO message for sDMA UL callback */
    for (index=0;index < sdma_ul_packet_nb ; index++)
    {
        fifo_data_ul_msg_table[index].length=packet[index]->buf_size;
        fifo_data_ul_msg_table[index].start_address=(void*)adr[index];
        if(packet[index]->packet_type==PKT_TYPE_UL_DATA)
        {
            fifo_data_ul_msg_table[index].type=eIPC_MSG_TYPE_UL_DATA;
            fifo_data_ul_msg_table[index].tag=packet[index]->bearerID;
        }
        if(packet[index]->packet_type==PKT_TYPE_UL_MGMT)
        {
            fifo_data_ul_msg_table[index].type=eIPC_MSG_TYPE_UL_MGMT;
            fifo_data_ul_msg_table[index].tag=0;
        }
    }


    /* 4. init & start transfer */

    /* compute parameters and start sDMA transfer */

    /* alloc DMA channel */
    if (omap_request_dma(IPC_DMA_4GHIF_UL_ID, NULL, sdma_4ghif_ul_callback, NULL, &dma_ch_ul) != 0) panic("4GHIF ERROR in %s: Cannot obtain sDMA CH for UL\n",__FUNCTION__);

    b_sdma_ul_transfer_on_going=true;

    /* create SDMA descriptors */
    last=false;
    for (index=0;index < sdma_ul_packet_nb ; index++)
    {
        /* synchronize the cache*/
        dmac_clean_range( (void*)(packet[index]->pbuf), (void*)((u32)(packet[index]->pbuf)+(u32)(packet[index]->buf_size)) );
        outer_clean_range( (u32)(packet[index]->pbuf),   (u32)(packet[index]->pbuf)+(u32)(packet[index]->buf_size) );

        /* fill in the descriptors for the DMA linked list trabsfer*/
        if(index==((sdma_ul_packet_nb)-1)) last=true;
        fill_one_type3sd_descr(&dma_descr_ul_array[index], (u32)((void*)virt_to_phys(packet[index]->pbuf)), adr[index], packet[index]->buf_size, last);

        /* some stats */
        the_ipc_4ghif_stats.ipc_4ghif_stat_ul_packets++;
        the_ipc_4ghif_stats.ipc_4ghif_stat_ul_bytes += (u32)packet[index]->buf_size;
	if(the_ipc_4ghif_stats.ipc_4ghif_stat_ul_bytes>=ONE_GIGABYTE) { the_ipc_4ghif_stats.ipc_4ghif_stat_ul_bytes-=ONE_GIGABYTE; the_ipc_4ghif_stats.ipc_4ghif_stat_ul_gbytes++; }
    }
    /* fill in the last descriptor*/
    (dma_descr_ul_array[sdma_ul_packet_nb]).next_descr_addr_ptr=END_OF_DMA_TYPE3_LINKLIST;

    /* configure DMA channel and start DMA transfer */
    descr_phys_adr=SHARED_MEM_VIRT_TO_PHYS(dma_descr_ul_array);
    OMAP_DMA_4GHIF_UL(dma_ch_ul,descr_phys_adr);

    the_ipc_4ghif_stats.ipc_4ghif_stat_ul_sdma_start++;

    return ret;
}


/*=====================================================================================================================
Name: IPC_4GHIF_cancel_ul
Description:
-    This function is used to cancel an UL frame transfer started by IPC_4GHIF_submit_ul
Parameters:
-    None
Return value:
-    IPC_4GHIF_STATUS_OK
Notes:
-    This function can be call if caller do not receive uplink complete callback after IPC_4GHIF_submit_ul
-       Actual implementation is to panic to signal this issue.
-       This should not happend so if caller detects this issue it is needed to investiguate
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_cancel_ul(void)
{
    ENTER_FUNC();

    /*some checks*/
    if(b_sdma_ul_transfer_on_going==false) return IPC_4GHIF_STATUS_NO_TRANS_ON_GOING;

    /* A. Normal behavior*/
    panic("4GHIF ERROR: Caller reports it never receive the UL complete callback");

    /* B. To test data driver
      if this solution becomes standard, then
      - check b_sdma_ul_transfer_on_going
      - unreserve FIFO,
      - deallocate DATA,
      - reject missing SDMA callback if it finally occurs   */
    /* b_sdma_ul_transfer_on_going = false; */

    return IPC_4GHIF_STATUS_OK;
}


/*=====================================================================================================================
Name: IPC_4GHIF_cancel_dl
Description:
-    This function is used to cancel a DL frame transfer started by IPC_4GHIF_submit_dl
Parameters:
-    None
Return value:
-    IPC_4GHIF_STATUS_OK
Notes:
-    This function can be call if caller do not receive downlink complete callback after IPC_4GHIF_submit_dl
-       Actual implementation is to panic to signal this issue.
-       This should not happend so if caller detects this issue it is needed to investiguate
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_cancel_dl(void)
{
    ENTER_FUNC();

    /*some checks*/
    if(b_sdma_dl_transfer_on_going==false) return IPC_4GHIF_STATUS_NO_TRANS_ON_GOING;

    /* A. Normal behavior */
    panic("4GHIF ERROR: Caller reports it never receive the DL complete callback");

    /* B. To test data driver
       if this solution becomes standard, then
       - check b_sdma_ul_transfer_on_going
       - reject missing SDMA callback if it finally occurs */
    /* b_sdma_dl_transfer_on_going = false; */

    return IPC_4GHIF_STATUS_OK;
}


/*=====================================================================================================================
Name: IPC_4GHIF_get_error_string
Description:
-    This function provides the error string assocaited to an error code
Parameters:
-       PC_4GHIF_STATUS error_code
-    char* error_string
-    int error_string_size
return value:
-    IPC_4GHIF_STATUS_OK
-    IPC_4GHIF_STATUS_INVALID_PARAM: invalid error code
Notes:
-    None
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_get_error_string(IPC_4GHIF_STATUS error_code, char* error_string, int error_string_size)
{
    static int_string_map error_map[] =
    {
        {IPC_4GHIF_STATUS_OK, "Ok"},
        {IPC_4GHIF_STATUS_INVALID_PARAM, "Invalid parameter"},
        {IPC_4GHIF_STATUS_BUSY, "Busy"},
        {IPC_4GHIF_STATUS_CLOSED, "Closed"},
        {IPC_4GHIF_STATUS_ALREADY_OPEN, "Already open"},
        {IPC_4GHIF_STATUS_RESOURCES_FULL, "Resuorces full"},
        {IPC_4GHIF_STATUS_NO_FRAME, "No frame"},
        {IPC_4GHIF_STATUS_BP_NOT_READY, "BP not ready"},
        {IPC_4GHIF_STATUS_AP_NOT_READY, "AP not ready"},
        {IPC_4GHIF_STATUS_NO_TRANS_ON_GOING, "No trans on-going"},
        {-1, NULL},
    };

    int i = 0;
    int_string_map*  map = &(error_map[i]);

    if (!error_string) return IPC_4GHIF_STATUS_INVALID_PARAM;

    while(map->string_value != NULL)
    {
        if ((int)error_code == map->val)
        {
            strncpy(error_string, map->string_value, (size_t) error_string_size);
            return IPC_4GHIF_STATUS_OK;
        }
        i++;
        map = &(error_map[i]);
    }

    return IPC_4GHIF_STATUS_INVALID_PARAM;

}


/*=====================================================================================================================
Name: IPC_4GHIF_get_statistics_string
Description:
-    This function is used to get 4GHIF IPC low level driver statistics in a string
Parameters:
-    char* sz   : stat string
-       u16 sz_size: stat string max size
Return value:
-    IPC_4GHIF_STATUS_OK
-    IPC_4GHIF_STATUS_INVALID_PARAM: NULL string pointer
Notes:
-       It is recommended to provide a 1024bytes string
-       This function fills a statistics stat string to be more generic than the IPC_4GHIF_get_statistics
-       It also includes UL memory manager statistics
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_get_statistics_string(char* sz, u16 sz_size)
{
    char szTemp[1536];
    int i;


    if(sz==NULL) return IPC_4GHIF_STATUS_INVALID_PARAM;

    /* 1. Add standard statistics */
    strcpy(szTemp,"SDMA:");
    sprintf(&szTemp[strlen(szTemp)],"\n  DL");
    sprintf(&szTemp[strlen(szTemp)],"\n    rcv_cb:%d"                  ,the_ipc_4ghif_stats.ipc_4ghif_stat_dl_notify);
    sprintf(&szTemp[strlen(szTemp)],"\n    sdma_start:%9d done: %d"    ,the_ipc_4ghif_stats.ipc_4ghif_stat_dl_sdma_start, the_ipc_4ghif_stats.ipc_4ghif_stat_dl_sdma_done  );
    sprintf(&szTemp[strlen(szTemp)],"\n    packets:   %9d ack:  %d"    ,the_ipc_4ghif_stats.ipc_4ghif_stat_dl_packets,    the_ipc_4ghif_stats.ipc_4ghif_stat_dl_ack        );
    sprintf(&szTemp[strlen(szTemp)],"\n    Gbytes:    %9d bytes:%d"    ,the_ipc_4ghif_stats.ipc_4ghif_stat_dl_gbytes,     the_ipc_4ghif_stats.ipc_4ghif_stat_dl_bytes      );
    sprintf(&szTemp[strlen(szTemp)],"\n    DATA BPwr=%-3d APrd=%-3d"   ,*(volatile u8*)(FIFO_DL_WRITE_IND)    ,*(volatile u8*)(FIFO_DL_READ_IND));
    sprintf(&szTemp[strlen(szTemp)],"\n    ACK  APwr=%-3d BPrd=%-3d"   ,*(volatile u8*)(FIFO_ACK_UL_WRITE_IND),*(volatile u8*)(FIFO_ACK_UL_READ_IND));
    sprintf(&szTemp[strlen(szTemp)],"\n    ISR nb=%d"                  ,the_ipc_4ghif_stats.ipc_4ghif_stat_irq_nb);
    sprintf(&szTemp[strlen(szTemp)],"\n  UL");
    sprintf(&szTemp[strlen(szTemp)],"\n    sdma_start:%9d done: %d"    ,the_ipc_4ghif_stats.ipc_4ghif_stat_ul_sdma_start, the_ipc_4ghif_stats.ipc_4ghif_stat_ul_sdma_done  );
    sprintf(&szTemp[strlen(szTemp)],"\n    packets:   %9d ack:  %d"    ,the_ipc_4ghif_stats.ipc_4ghif_stat_ul_packets,    the_ipc_4ghif_stats.ipc_4ghif_stat_ul_ack        );
    sprintf(&szTemp[strlen(szTemp)],"\n    Gbytes:    %9d bytes:%d"    ,the_ipc_4ghif_stats.ipc_4ghif_stat_ul_gbytes,     the_ipc_4ghif_stats.ipc_4ghif_stat_ul_bytes      );
    sprintf(&szTemp[strlen(szTemp)],"\n    full:      %9d resume:%d"   ,the_ipc_4ghif_stats.ipc_4ghif_stat_ul_submit_full,the_ipc_4ghif_stats.ipc_4ghif_stat_ul_resume_full);
    sprintf(&szTemp[strlen(szTemp)],"\n    DATA APwr=%-3d BPrd=%-3d"   ,*(volatile u8*)(FIFO_UL_WRITE_IND)    ,*(volatile u8*)(FIFO_UL_READ_IND));
    sprintf(&szTemp[strlen(szTemp)],"\n    ACK  BPwr=%-3d APrd=%-3d"   ,*(volatile u8*)(FIFO_ACK_DL_WRITE_IND),*(volatile u8*)(FIFO_ACK_DL_READ_IND));

    /* 2. Add UL memory manager statistics */
    strcat(&szTemp[strlen(szTemp)],"\n");
    IPC_4GHIF_data_ul_mm_get_stat_string(&szTemp[strlen(szTemp)], 1024-strlen(szTemp));

    /* 3. Add 3GSM IPC statistics */
    the_ipc_4ghif_stats.ipc_3gsm_bp_data_dl = *((volatile u32 *) (IPC_DEBUG_STAT_VIRT_ADDR));
    the_ipc_4ghif_stats.ipc_3gsm_bp_ack_dl = *((volatile u32 *) (IPC_DEBUG_STAT_VIRT_ADDR+sizeof(u32)));
    the_ipc_4ghif_stats.ipc_3gsm_bp_data_ul = *((volatile u32 *) (IPC_DEBUG_STAT_VIRT_ADDR+2*sizeof(u32)));
    the_ipc_4ghif_stats.ipc_3gsm_bp_ack_ul = *((volatile u32 *) (IPC_DEBUG_STAT_VIRT_ADDR+3*sizeof(u32)));
    sprintf(&szTemp[strlen(szTemp)],"\nIPC 3GSM stats");
    sprintf(&szTemp[strlen(szTemp)],"\n  DL");
    sprintf(&szTemp[strlen(szTemp)],"\n  BP: DATA sent:%9d  DATA ACK:%9d",the_ipc_4ghif_stats.ipc_3gsm_bp_data_dl,the_ipc_4ghif_stats.ipc_3gsm_bp_ack_dl);
    sprintf(&szTemp[strlen(szTemp)],"\n  AP: DATA sent:%9d  DATA ACK:%9d read nb:%9d",the_ipc_4ghif_stats.ipc_3gsm_ap_data_dl,the_ipc_4ghif_stats.ipc_3gsm_ap_ack_dl,the_ipc_4ghif_stats.ipc_3gsm_ap_read_ex2);
    sprintf(&szTemp[strlen(szTemp)],"\n  UL");
    sprintf(&szTemp[strlen(szTemp)],"\n  BP: DATA sent:%9d  DATA ACK:%9d",the_ipc_4ghif_stats.ipc_3gsm_bp_data_ul,the_ipc_4ghif_stats.ipc_3gsm_bp_ack_ul);
    sprintf(&szTemp[strlen(szTemp)],"\n  AP: DATA sent:%9d  DATA ACK:%9d  write nb:%9d",the_ipc_4ghif_stats.ipc_3gsm_ap_data_ul,the_ipc_4ghif_stats.ipc_3gsm_ap_ack_ul,the_ipc_4ghif_stats.ipc_3gsm_ap_write_ex2);
    sprintf(&szTemp[strlen(szTemp)],"\n  DL Timing stats (rcv to ack (s)):");
    for (i=0; i< DL_RCV2ACK_TIME_ARRAY_SIZE;i++)
    {
        sprintf(&szTemp[strlen(szTemp)],"\n%02ds %dx",i,the_ipc_4ghif_stats.dl_rcv2ack_time[i]);
    }
    /* 4. return the string */
    strncpy(sz,szTemp,(int)sz_size);

    return IPC_4GHIF_STATUS_OK;
}

/*=====================================================================================================================
Name: ipc_4ghif_test_stats
Description:
-	This function is used by the test order mechanisum to display 4GHIF IPC stats
Parameters:
-	None
Return value:
-	None
Notes:
-       None
=====================================================================================================================*/

void ipc_4ghif_test_stats(void)
{
      char szStat[1024]="\0";
      int ret;

      ret=(int)IPC_4GHIF_get_statistics_string(szStat, 1024);
      if(ret==IPC_4GHIF_STATUS_OK) printk(szStat);
}


#ifdef IPC_4GHIF_TIMESTAMP
/*=====================================================================================================================
Name: ipc_4ghif_test_report
Description:
-	This function is used by the test order mechanisum  to display 4GHIF IPC debug info, timestamps
Parameters:
-	None
Return value:
-	None
Notes:
-       None
=====================================================================================================================*/

void ipc_4ghif_test_report(void)
{
    int i,ind,arg,submit_ul_ind,submit_dl_ind,start_dma_ul_ind,start_dma_dl_ind;
    u32 nb_tx_packets,nb_rx_packets;
    union ktime kt_delta;

    ind=tx_kt_array_wr;
    if (tx_kt_array_used < TX_KT_ARRAY_SIZE) ind=0;

    printk ("\n  FIFO index  Function  Time(s)     [Delta(s)]     [Nb packets]\n");

    submit_ul_ind=submit_dl_ind=start_dma_ul_ind=start_dma_dl_ind=-1;

    for(i=0;i<tx_kt_array_used;i++)
    {
        arg=tx_kt_array_arg[ind];
	if ((arg >0)&&(arg<TS_LAST))
	{
	     printk("\n%4d: %4d.%09d %20s",ind,tx_kt_array[ind].tv.sec,tx_kt_array[ind].tv.nsec,ts_function_array[arg]);
	}
	else  printk("\n Error in %s: Invalid arg!\n",__FUNCTION__);

	if (arg==TS_SUBMIT_UL)
	{
	    submit_ul_ind=ind;
	}
	if (arg==TS_SUBMIT_DL)
	{
	    submit_dl_ind=ind;
	}
	if ((arg==TS_SUBMIT_UL_CB)&&(submit_ul_ind>=0))
	{
	      kt_delta=ktime_sub(tx_kt_array[ind],tx_kt_array[submit_ul_ind]);
	      nb_tx_packets=tx_kt_array_arg2[ind];
	      submit_ul_ind=-1;
	      printk("%4d.%09d %4d",kt_delta.tv.sec,kt_delta.tv.nsec,nb_tx_packets);
	}
	if ((arg==TS_SUBMIT_DL_CB)&&(submit_dl_ind>=0))
	{
	      kt_delta=ktime_sub(tx_kt_array[ind],tx_kt_array[submit_dl_ind]);
	      nb_rx_packets=tx_kt_array_arg2[ind];
	      submit_dl_ind=-1;
	      printk("%4d.%09d %d",kt_delta.tv.sec,kt_delta.tv.nsec,nb_rx_packets);
	}
	if (arg==TS_START_DMA_UL)
	{
	    start_dma_ul_ind=ind;
	}
	if (arg==TS_START_DMA_DL)
	{
	    start_dma_dl_ind=ind;
	}
	if ((arg==TS_DMA_CB_UL)&&(start_dma_ul_ind>=0))
	{
	      kt_delta=ktime_sub(tx_kt_array[ind],tx_kt_array[start_dma_ul_ind]);
	      nb_tx_packets=tx_kt_array_arg2[ind];
	      start_dma_ul_ind=-1;
	      printk("%4d.%09d %4d",kt_delta.tv.sec,kt_delta.tv.nsec,nb_tx_packets);
	}
	if ((arg==TS_DMA_CB_DL)&&(start_dma_dl_ind>=0))
	{
	      kt_delta=ktime_sub(tx_kt_array[ind],tx_kt_array[start_dma_dl_ind]);
	      nb_tx_packets=tx_kt_array_arg2[ind];
	      start_dma_dl_ind=-1;
	      printk("%4d.%09d %4d",kt_delta.tv.sec,kt_delta.tv.nsec,nb_tx_packets);
	}

	ind=ind+1; if(ind==TX_KT_ARRAY_SIZE) ind=0;
    }

}
#endif // 4GHIF_TIMESTAMP

/*=====================================================================================================================
Name: IPC_4GHIF_debug_order
Description:
-	This function is used to set 4GHIF IPC debug level or start test sets
Parameters:
-	u32 order: debug command
Return value:
-	IPC_4GHIF_STATUS_OK
Notes:
-       None
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_debug_order(u32 order)
{
    switch (order)
    {
#ifdef IPC_4GHIF_TIMESTAMP
        case 1: ipc_4ghif_test_report(); break;
#endif // IPC_4GHIF_TIMESTAMP
	case 2: ipc_4ghif_test_stats();  break;
	case 3: ipc_4ghif_bp_loopback_stub_mode=IPC_4GHIF_BP_LOOPBACK_STUB_DISABLED;             printk("\n4GHIF BP loopback stub DISABLED\n"); break;
	case 4: simulate_bp_init();
	        ipc_4ghif_bp_loopback_stub_mode=IPC_4GHIF_BP_LOOPBACK_STUB_ENABLED;              printk("\n4GHIF BP loopback stub ENABLED\n");  break;
	case 5: simulate_bp_init();
	        ipc_4ghif_bp_loopback_stub_mode=IPC_4GHIF_BP_LOOPBACK_STUB_ENABLED_ONLY_FOR_UL;  printk("\n4GHIF BP loopback stub ENABLED ONLY IN UL\n");  break;
	case 6: IPC_4GHIF_data_ul_display_last_allocated_size(); break;
	case 7: memset(&the_ipc_4ghif_stats,0,sizeof(ipc_4ghif_stats)); break;
	default: printk("\nhif order usage\n1- 4GHIF timestamp report (need test build)\n2- 4GHIF stats\n3- Disable BP loopback stub\n4- Enable	BP\n5- Enable BP loopback stub only in UL\n6- Last UL MM allocated size\n7- Reset statistics\n"); break;
    }

    return IPC_4GHIF_STATUS_OK;
}


EXPORT_SYMBOL(IPC_4GHIF_open);
EXPORT_SYMBOL(IPC_4GHIF_close);
EXPORT_SYMBOL(IPC_4GHIF_ready);
EXPORT_SYMBOL(IPC_4GHIF_submit_ul);
EXPORT_SYMBOL(IPC_4GHIF_submit_dl);
EXPORT_SYMBOL(IPC_4GHIF_cancel_ul);
EXPORT_SYMBOL(IPC_4GHIF_cancel_dl);
EXPORT_SYMBOL(IPC_4GHIF_get_dl_frames_nb);
EXPORT_SYMBOL(IPC_4GHIF_get_error_string);
EXPORT_SYMBOL(IPC_4GHIF_get_statistics_string);
EXPORT_SYMBOL(IPC_4GHIF_debug_order);
