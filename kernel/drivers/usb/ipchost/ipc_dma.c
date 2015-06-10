/*
 * Copyright (C) 2007-2010 Motorola, Inc
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
 * 12/07/2007      Motorola        USB-IPC initial
 * 03/22/2007      Motorola        USB-IPC header support
 * 05/09/2008      Motorola        Change Copyright and Changelog
 * 07/09/2008      Motorola        upmerge for 23.5
 * 11/03/2008      Motorola        Support sequence number
 * 03/06/2009      Motorola        Use mailboxes instead of USB
 * 05/07/2009      Motorola        Add IPC checksum 
 * 07/15/2009      Motorola        Use sDMA for endianess conversion (excepted for IPC header)      
 * 09/15/2009      Motorola        Workaround for Data stall issue.
 * 01/07/2010      Motorola        Remove workaround for Data stall issue
 * 12/18/2009      Motorola        Fix sDMA performance issue
 * 08/31/2010      Motorola        Fix Klockwork defects
 */

/*!
 * @file drivers/usb/ipchost/ipc_dma.c
 * @brief USB-IPC Descriptor Set
 *
 * This is the generic portion of the USB-IPC driver.
 *
 * @ingroup IPCFunction
 */


/*
 * Include Files
 */
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
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <mach/dma.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <asm/system.h>

/*#define IPC_DEBUG*/

#ifdef IPC_DEBUG
#define DEBUG(args...) printk(args)
#else
#define DEBUG(args...)
#endif

#if defined(USE_OMAP_SDMA)

#define IPC_DMA_NODE2BUF_ID      1
#define IPC_DMA_BUF2NODE_ID      2


typedef struct {
    int dma_ch;
    int node_index;
    int frame_index;
    int total_size;
    USB_IPC_API_PARAMS *ipc_ch;
    unsigned long      buf_phy;
    IPC_DATA_HEADER    *header;
    HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T  *node_ptr;
} IPC_DMA_MEMCPY;

IPC_DMA_MEMCPY ipc_memcpy_node2buf, ipc_memcpy_buf2node;

extern USB_IPC_API_PARAMS  usb_ipc_channels[MAX_USB_IPC_CHANNELS];

extern void usb_ipc_exchange_endian16(u16 *data);
extern void usb_ipc_exchange_endian32(unsigned long *data);
extern void usb_ipc_exchange_endian32_short(unsigned long *data);
extern void ipc_header_exchange_endian32(unsigned long * ptr_buf,unsigned short us_FrameNb);

#define NODE_BUF_PHYS_ADDR(x)   virt_to_phys(x)

/* Start DMA data transfer from memory to memory */
#define OMAP_DMA_MEM2MEM_SEND(ch, src, dest, len)                       \
    DEBUG("\n%s: ch=%d, dest = 0x%lx, src=0x%lx\n", __FUNCTION__, ch, dest, src);   \
    omap_set_dma_transfer_params(ch, OMAP_DMA_DATA_TYPE_S8, len, 1, OMAP_DMA_SYNC_FRAME, 0, 0); \
    omap_set_dma_dest_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_src_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_write_mode(ch,OMAP_DMA_WRITE_LAST_NON_POSTED); \
    omap_set_dma_prefetch_mode(ch, OMAP_DMA_PREFETCH_EN); \
    omap_set_dma_src_data_pack(ch, 1); \
    omap_set_dma_dest_data_pack(ch, 1); \
    omap_set_dma_dest_params(ch, 0, OMAP_DMA_AMODE_POST_INC, dest, 0, 0);     \
    omap_set_dma_src_params(ch, 0, OMAP_DMA_AMODE_POST_INC, src, 0, 0);       \
    omap_set_dma_src_endian_type(ch,OMAP_DMA_LITTLE_ENDIAN);\
    omap_set_dma_dst_endian_type(ch,OMAP_DMA_BIG_ENDIAN);\
    omap_start_dma(ch);
    
#define OMAP_DMA_MEM2MEM_RCV(ch, src, dest, len)                        \
    DEBUG("\n%s: ch=%d, dest = 0x%lx, src=0x%lx\n", __FUNCTION__, ch, dest, src);   \
    omap_set_dma_transfer_params(ch, OMAP_DMA_DATA_TYPE_S8, len, 1, OMAP_DMA_SYNC_FRAME, 0, 0); \
    omap_set_dma_dest_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_src_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_write_mode(ch,OMAP_DMA_WRITE_LAST_NON_POSTED); \
    omap_set_dma_prefetch_mode(ch, OMAP_DMA_PREFETCH_EN); \
    omap_set_dma_src_data_pack(ch, 1); \
    omap_set_dma_dest_data_pack(ch, 1); \
    omap_set_dma_dest_params(ch, 0, OMAP_DMA_AMODE_POST_INC, dest, 0, 0);     \
    omap_set_dma_src_params(ch, 0, OMAP_DMA_AMODE_POST_INC, src, 0, 0);       \
    omap_set_dma_src_endian_type(ch,OMAP_DMA_BIG_ENDIAN);\
    omap_set_dma_dst_endian_type(ch,OMAP_DMA_LITTLE_ENDIAN);\
    omap_start_dma(ch);


/*
 *  Node 2 Buffer DMA callback
 */
void ipc_dma_node2buf_callback (int lch, u16 ch_status, void *data)
{
    int ret;
    USB_IPC_API_PARAMS *ipc_ch;     
    IPC_DATA_HEADER *header;
#ifdef CONFIG_MAILBOX_IPC_USE_CHECKSUM
    unsigned long * adr=0
#endif

    DEBUG("%s\n", __FUNCTION__);
    ipc_ch = ipc_memcpy_node2buf.ipc_ch;
    /* Set DONE bit. If all node buffer are copied to URB buffer, finished */
    ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].comand  |= NODE_DESCRIPTOR_DONE_BIT;

    ipc_memcpy_node2buf.buf_phy += ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].length;
    ipc_memcpy_node2buf.total_size +=  ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].length;
    ipc_ch->write_ptr.total_num += ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].length;

    if ( ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].comand & NODE_DESCRIPTOR_LAST_BIT)
        ipc_memcpy_node2buf.frame_index ++;

    header = (IPC_DATA_HEADER *)ipc_ch->write_ptr.temp_buff;

    if( (ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].comand & NODE_DESCRIPTOR_END_BIT) || (ipc_memcpy_node2buf.node_index >= (ipc_memcpy_node2buf.ipc_ch->max_node_num) ) || ipc_memcpy_node2buf.frame_index >= header->nb_frame ) {
        omap_stop_dma(ipc_memcpy_node2buf.dma_ch);
        omap_free_dma(ipc_memcpy_node2buf.dma_ch);
        dmac_inv_range(ipc_ch->write_ptr.temp_buff, ((void *)(ipc_ch->write_ptr.temp_buff)+ipc_ch->max_temp_buff_size));
        outer_inv_range((unsigned long)ipc_ch->write_ptr.temp_buff, ((unsigned long)(ipc_ch->write_ptr.temp_buff)+ipc_ch->max_temp_buff_size));

#ifdef CONFIG_MAILBOX_IPC_USE_CHECKSUM
        /*change for endian: swap data and length when odd number of frames and last data frame*/
        if(header->nb_frame%2)
        {
            /*swap first data frame*/
	    adr = (unsigned long *) __arch_ioremap(ipc_ch->write_ptr.temp_buff_phy_addr +(sizeof(IPC_DATA_HEADER_INDEX) + (sizeof(IPC_FRAME_DESCRIPTOR) * (header->nb_frame))-sizeof(short))
                                                                             ,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);
            if (adr) usb_ipc_exchange_endian32_short(adr);
            else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);
        }
                
        /*swap last data frame*/
        adr = (unsigned long *) __arch_ioremap(ipc_ch->write_ptr.temp_buff_phy_addr +       ipc_memcpy_node2buf.total_size  - (ipc_memcpy_node2buf.total_size%4)   
                                                                   ,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);
        if (adr) usb_ipc_exchange_endian32_short(adr);
        else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);

        ipc_buffer_add_checksum(ipc_ch->write_ptr.temp_buff);

        /*change for endian: swap data and length when odd number of frames and last data frame*/
        if(header->nb_frame%2)
        {
            /*swap first data frame*/
	    adr = (unsigned long *) __arch_ioremap(ipc_ch->write_ptr.temp_buff_phy_addr +(sizeof(IPC_DATA_HEADER_INDEX) + (sizeof(IPC_FRAME_DESCRIPTOR) * (header->nb_frame))-sizeof(short))
                                                                             ,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE) ;
            if (adr) usb_ipc_exchange_endian32_short(adr);
            else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);
        }
        /*swap last data frame*/                                                
        adr = (unsigned long *) __arch_ioremap(ipc_ch->write_ptr.temp_buff_phy_addr + ipc_memcpy_node2buf.total_size  - (ipc_memcpy_node2buf.total_size%4)   
                                                                   ,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);
        if (adr) usb_ipc_exchange_endian32_short(adr);
        else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);
#endif          
                
        //change for endian
        usb_ipc_exchange_endian16((u16 *)&(header->sequence_number));
        ipc_header_exchange_endian32((unsigned long *)ipc_ch->write_ptr.temp_buff,header->nb_frame);    
        //change for endian end

#ifndef IPC_USE_ZERO_LEN_FRAME
        ipc_ch->write_ptr.end_flag = 1;
#endif /* !IPC_USE_ZERO_LEN_FRAME */

        dmac_clean_range(ipc_ch->write_ptr.temp_buff,
                         ((void *)((ipc_ch->write_ptr.temp_buff)+sizeof(IPC_DATA_HEADER_INDEX)+
                          (sizeof(IPC_FRAME_DESCRIPTOR) * (header->nb_frame)))));
	outer_clean_range((unsigned long)ipc_ch->write_ptr.temp_buff,
                         ((unsigned long)((ipc_ch->write_ptr.temp_buff)+sizeof(IPC_DATA_HEADER_INDEX)+
                          (sizeof(IPC_FRAME_DESCRIPTOR) * (header->nb_frame)))));	

        ret = mailbox_write(ipc_memcpy_node2buf.total_size);
        if(ret != 0) {
            ipc_ch->write_flag = 0;
        }

        return;
    }

    ipc_memcpy_node2buf.node_index ++;

    dmac_clean_range(ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr,
                     ((void *)((ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr)+
                      ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].length)));
    outer_clean_range((unsigned long)ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr,
                     ((unsigned long)((ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr)+
                      ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].length)));

    DEBUG("Continue DMA:buf_phy=%lx total_size=%d node_index=%d\n", ipc_memcpy_node2buf.buf_phy, ipc_memcpy_node2buf.total_size, ipc_memcpy_node2buf.node_index);
    OMAP_DMA_MEM2MEM_SEND(ipc_memcpy_node2buf.dma_ch, NODE_BUF_PHYS_ADDR(ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr), ipc_memcpy_node2buf.buf_phy, ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].length);
}

/*
 *  Node 2 Buffer DMA transfer
 */
int ipc_dma_memcpy_node2buf(USB_IPC_API_PARAMS *ipc_ch, HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *ctrl_ptr)
{
    int ret, i;
    int frame_size, frame_index, len;
    unsigned long * adr=0;
    IPC_DATA_HEADER *header;

    DEBUG("%s\n", __FUNCTION__);

    /*alloc DMA channel*/
    ret = omap_request_dma(IPC_DMA_NODE2BUF_ID, NULL, ipc_dma_node2buf_callback, NULL, &ipc_memcpy_node2buf.dma_ch);
	
    if(ret != 0)   { /* error: using memcpy */
        /* copy data into write URB buffer */
        frame_size = 0;
        frame_index = 0;
        //len = sizeof(IPC_DATA_HEADER);
        header = (IPC_DATA_HEADER *)ipc_ch->write_ptr.temp_buff;
        len = sizeof(IPC_DATA_HEADER_INDEX) + ((header->nb_frame) * sizeof(IPC_FRAME_DESCRIPTOR));
        for(i = 0; i < ipc_ch->max_node_num; i++) {
            if( (len + ctrl_ptr[i].length) > ipc_ch->max_temp_buff_size ) {
                break;
            }
            memcpy((void *)&ipc_ch->write_ptr.temp_buff[len], ctrl_ptr[i].data_ptr, ctrl_ptr[i].length);
            frame_size += ctrl_ptr[i].length;
            len        += ctrl_ptr[i].length;
            ctrl_ptr[i].comand |= NODE_DESCRIPTOR_DONE_BIT;
            ipc_ch->write_ptr.total_num +=ctrl_ptr[i].length ;
            if( ctrl_ptr[i].comand & NODE_DESCRIPTOR_LAST_BIT )  {
                header->frames[frame_index].length = frame_size;
                frame_index ++;
                frame_size = 0;
            }
            if(ctrl_ptr[i].comand & NODE_DESCRIPTOR_END_BIT) {
                break;
            }
        }
        header->nb_frame = frame_index;
        //change for endian
        len = sizeof(IPC_DATA_HEADER_INDEX) + ((header->nb_frame) * sizeof(IPC_FRAME_DESCRIPTOR));
        if (header->nb_frame > 0) {
            for (i = 0; i < header->nb_frame; i++)
                len += header->frames[i].length;
        }
        ipc_header_exchange_endian32((unsigned long *)ipc_ch->write_ptr.temp_buff,header->nb_frame);
        //change for endian end
#ifndef IPC_USE_ZERO_LEN_FRAME
        ipc_ch->write_ptr.end_flag = 1;
#endif /* !IPC_USE_ZERO_LEN_FRAME */
        ret = mailbox_write(len);
        if(ret != 0) {
            ipc_ch->write_flag = 0;
        }

        return 0;
    }

    /* generate header */
    header = (IPC_DATA_HEADER *)ipc_ch->write_ptr.temp_buff;
    frame_size = 0;
    frame_index = 0;
    len = sizeof(IPC_DATA_HEADER_INDEX) + ((header->nb_frame) * sizeof(IPC_FRAME_DESCRIPTOR));
    for(i = 0; i < ipc_ch->max_node_num; i++) {
        if( ((len + ctrl_ptr[i].length) > ipc_ch->max_temp_buff_size) || (ctrl_ptr[i].length <= 0) || (ctrl_ptr[i].length > MAX_FRAME_SIZE)) {
            break;
        }
        frame_size += ctrl_ptr[i].length;
        len        += ctrl_ptr[i].length;
        if( ctrl_ptr[i].comand & NODE_DESCRIPTOR_LAST_BIT )  {
            header->frames[frame_index].length = frame_size;
            DEBUG("header->frames[%d]=%d\n", frame_index, frame_size);
            frame_index ++;
            frame_size = 0;
        }
        if(ctrl_ptr[i].comand & NODE_DESCRIPTOR_END_BIT) {
            break;
        }
    }
    header->nb_frame = frame_index;
    if (frame_index == 0) {
        omap_free_dma(ipc_memcpy_node2buf.dma_ch);
        ipc_header_exchange_endian32((unsigned long *)ipc_ch->write_ptr.temp_buff,header->nb_frame);
                
#ifndef IPC_USE_ZERO_LEN_FRAME
        ipc_ch->write_ptr.end_flag = 1;
#endif /* !IPC_USE_ZERO_LEN_FRAME */
        return -1;
    }
    DEBUG("header->version=%x header->nb_frame=%d write_ptr.temp_buff_phy_addr=%x\n", header->version, header->nb_frame, ipc_ch->write_ptr.temp_buff_phy_addr);

    ipc_memcpy_node2buf.ipc_ch        = ipc_ch;
    ipc_memcpy_node2buf.node_index    = 0;
    ipc_memcpy_node2buf.frame_index    = 0;
    ipc_memcpy_node2buf.node_ptr      = ctrl_ptr;
    ipc_memcpy_node2buf.total_size    = sizeof(IPC_DATA_HEADER_INDEX) + (sizeof(IPC_FRAME_DESCRIPTOR) * (header->nb_frame));
    //ipc_memcpy_node2buf.total_size    = 0;
    ipc_memcpy_node2buf.buf_phy       = ipc_ch->write_ptr.temp_buff_phy_addr + 
        (sizeof(IPC_DATA_HEADER_INDEX) + (sizeof(IPC_FRAME_DESCRIPTOR) * (header->nb_frame)));
    /*change for endian*/
    if(header->nb_frame%2)
    {
        adr = (unsigned long *) __arch_ioremap(ipc_ch->write_ptr.temp_buff_phy_addr +(sizeof(IPC_DATA_HEADER_INDEX) + (sizeof(IPC_FRAME_DESCRIPTOR) * (header->nb_frame))-sizeof(short))
                                                                        ,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);
        if (adr) usb_ipc_exchange_endian32_short(adr);        
        else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);
    }
 
    DEBUG("Start DMA:buf_phy=%lx total_size=%d node_index=%d\n", ipc_memcpy_node2buf.buf_phy, ipc_memcpy_node2buf.total_size, ipc_memcpy_node2buf.node_index);

    dmac_clean_range(ipc_ch->write_ptr.temp_buff,
                     (void *)((ipc_ch->write_ptr.temp_buff)+
                      sizeof(IPC_DATA_HEADER_INDEX)+
                      (sizeof(IPC_FRAME_DESCRIPTOR)*(header->nb_frame))));
    outer_clean_range((unsigned long)ipc_ch->write_ptr.temp_buff,
                     (unsigned long)((ipc_ch->write_ptr.temp_buff)+
                      sizeof(IPC_DATA_HEADER_INDEX)+
                      (sizeof(IPC_FRAME_DESCRIPTOR)*(header->nb_frame))));

    dmac_clean_range(ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr,
                     (void *)((ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr)+
                      ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].length));
    outer_clean_range((unsigned long)ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr,
                     (unsigned long)((ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr)+
                      ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].length));

    /* set DMA parameters, then start DMA transfer */
    OMAP_DMA_MEM2MEM_SEND(ipc_memcpy_node2buf.dma_ch, NODE_BUF_PHYS_ADDR(ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].data_ptr), ipc_memcpy_node2buf.buf_phy, ipc_memcpy_node2buf.node_ptr[ipc_memcpy_node2buf.node_index].length);
    return 0;
}

/*
 *  Buffer 2 Node DMA callback
 */
void ipc_dma_buf2node_callback (int lch, u16 ch_status, void *data)
{
    int size;
    unsigned long * adr=0;
    HW_CTRL_IPC_WRITE_STATUS_T  ipc_status;
    USB_IPC_API_PARAMS *ipc_ch;

    DEBUG("%s\n", __FUNCTION__);

    ipc_ch = ipc_memcpy_buf2node.ipc_ch;
        
    /*change for endian*/                                                                                                                                                  
    if (ipc_memcpy_buf2node.header->nb_frame%2)
    {
        adr = (unsigned long *) __arch_ioremap(ipc_ch->read_ptr.temp_buff_phy_addr  +(sizeof(IPC_DATA_HEADER_INDEX) + (sizeof(IPC_FRAME_DESCRIPTOR) * (ipc_memcpy_buf2node.header->nb_frame))-sizeof(short))
                                                                         ,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);
        if (adr) usb_ipc_exchange_endian32_short(adr);
        else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);
    }

    /* Set DONE bit. If all node buffer are copied to URB buffer, finished */
    ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].comand  |= NODE_DESCRIPTOR_DONE_BIT;
    ipc_memcpy_buf2node.buf_phy += ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].length;
    ipc_memcpy_buf2node.header->frames[ipc_memcpy_buf2node.frame_index].length -= ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].length;
    if(ipc_memcpy_buf2node.header->frames[ipc_memcpy_buf2node.frame_index].length == 0) {
        ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].comand  |= NODE_DESCRIPTOR_LAST_BIT;
        ipc_memcpy_buf2node.frame_index ++;
    }

    ipc_memcpy_buf2node.total_size +=  ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].length;

    ipc_memcpy_buf2node.node_index ++;
    if( (ipc_memcpy_buf2node.node_index >= ipc_memcpy_buf2node.ipc_ch->read_ptr.node_num) || 
        (ipc_memcpy_buf2node.frame_index >= ipc_memcpy_buf2node.header->nb_frame) ) {
        omap_stop_dma(ipc_memcpy_buf2node.dma_ch);
        omap_free_dma(ipc_memcpy_buf2node.dma_ch);

        ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index - 1].comand  |= NODE_DESCRIPTOR_END_BIT;
		
        DEBUG("DMA copy ok:header->version=%x header->nb_frame=%d\n", ipc_memcpy_buf2node.header->version, ipc_memcpy_buf2node.header->nb_frame);
        DEBUG("buf_phy=%lx total_size=%d node_index=%d frame_index=%d\n", ipc_memcpy_buf2node.buf_phy, ipc_memcpy_buf2node.total_size, ipc_memcpy_buf2node.node_index, ipc_memcpy_buf2node.frame_index);

        /*
          for (i=0;i<ipc_memcpy_buf2node.node_index;i++)
          DEBUG("node_ptr[%d].comand=%x length=%d\n", i, ipc_ch->read_ptr.node_ptr[i].comand, ipc_ch->read_ptr.node_ptr[i].length);
          for (i=0;i<ipc_memcpy_buf2node.total_size;i++)
          DEBUG("temp_buf[%d]=%x, temp_buf_phy_addr[%d]=%x\n", i, ipc_ch->read_ptr.temp_buff[i], i, ((unsigned char *)(phys_to_virt(ipc_ch->read_ptr.temp_buff_phy_addr)))[i]);
        */
        /* clear flag to indicate API read function call is done */
        ipc_memcpy_buf2node.ipc_ch->read_flag = 0;

        /* read callback, ... */
        if(ipc_memcpy_buf2node.ipc_ch->cfg.read_callback != NULL)  {
            ipc_status.nb_bytes = ipc_memcpy_buf2node.total_size;
            ipc_status.channel  = &ipc_memcpy_buf2node.ipc_ch->ch;
            ipc_memcpy_buf2node.ipc_ch->cfg.read_callback(&ipc_status);
        } else {
            //up(&ipc_memcpy_buf2node.ipc_ch->read_ptr.read_mutex);
            SEM_UNLOCK(&ipc_memcpy_buf2node.ipc_ch->read_ptr.read_mutex);
        }
	
        return;
    }

    /* set DMA parameters, then start DMA transfer */
    size = ipc_memcpy_buf2node.header->frames[ipc_memcpy_buf2node.frame_index].length;
        
    /*change for endian*/                                                                                                                                                  
    if (ipc_memcpy_buf2node.header->nb_frame%2)
    {        
        adr = (unsigned long *) __arch_ioremap(ipc_ch->read_ptr.temp_buff_phy_addr +(sizeof(IPC_DATA_HEADER_INDEX) +  (sizeof(IPC_FRAME_DESCRIPTOR) * (ipc_memcpy_buf2node.header->nb_frame))-sizeof(short))
                                                                         ,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);
        if (adr) usb_ipc_exchange_endian32_short(adr);
        else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);
    }           

    if( size > ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].length) {
        size = ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].length;
    }
    ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].length = size;

    DEBUG("Continue DMA:buf_phy=%lx total_size=%d node_index=%d frame_index=%d\n", ipc_memcpy_buf2node.buf_phy, ipc_memcpy_buf2node.total_size, ipc_memcpy_buf2node.node_index, ipc_memcpy_buf2node.frame_index);
    /* set DMA parameters, then start DMA transfer */
    OMAP_DMA_MEM2MEM_RCV(ipc_memcpy_buf2node.dma_ch, ipc_memcpy_buf2node.buf_phy, NODE_BUF_PHYS_ADDR(ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].data_ptr), size);
}

/*
 *  Buffer 2 Node DMA transfer
 */
void ipc_dma_memcpy_buf2node(USB_IPC_API_PARAMS *ipc_ch)
{
    int ret;
    int size, len, index, frame_index, num;
    unsigned long * adr=0;
    IPC_DATA_HEADER *header;
    HW_CTRL_IPC_WRITE_STATUS_T  ipc_status;
        
 

    DEBUG("%s\n", __FUNCTION__);
  
        
    /* alloc DMA channel */
    ret = omap_request_dma(IPC_DMA_BUF2NODE_ID, NULL, ipc_dma_buf2node_callback, NULL, &ipc_memcpy_buf2node.dma_ch);
    if(ret != 0)   { /* error: using memcpy */
        header = (IPC_DATA_HEADER *)(ipc_ch->read_ptr.temp_buff);
	
		
		
        size = header->frames[0].length;
        //len  = sizeof(IPC_DATA_HEADER);
        len  = sizeof(IPC_DATA_HEADER_INDEX) + (sizeof(IPC_FRAME_DESCRIPTOR) * (header->nb_frame));
        /* copy data from temporary buffer to scatter buffers */
        for(index = 0, frame_index = 0; index < ipc_ch->read_ptr.node_num; index ++) {
            num = (size > (ipc_ch->read_ptr.node_ptr)[index].length) ? (ipc_ch->read_ptr.node_ptr)[index].length : size;
            memcpy((ipc_ch->read_ptr.node_ptr)[index].data_ptr, (void *)&((ipc_ch->read_ptr.temp_buff)[len]), num);
            ipc_ch->read_ptr.total_num += num;
            /* set flag to indicate received data is filled ... */
            (ipc_ch->read_ptr.node_ptr)[index].comand |= NODE_DESCRIPTOR_DONE_BIT;
            size -= num;
            len  += num;
            if( size == 0 ) {
                (ipc_ch->read_ptr.node_ptr)[index].length = num;
                (ipc_ch->read_ptr.node_ptr)[index].comand |= NODE_DESCRIPTOR_LAST_BIT;
                frame_index ++;
                if((frame_index < header->nb_frame)&&( header->nb_frame <= MAX_FRAME_NUM))
                {
                    size = header->frames[frame_index].length;
                }
                else break;
            }
        }
        if( index < ipc_ch->read_ptr.node_num ) {
            ipc_ch->read_ptr.node_ptr[index].comand |= NODE_DESCRIPTOR_END_BIT;
        }
        else {
            ipc_ch->read_ptr.node_ptr[index - 1].comand |= NODE_DESCRIPTOR_END_BIT;
        }

        /* clear flag to indicate API read function call is done */
        ipc_ch->read_flag = 0;
        if(ipc_ch->cfg.read_callback != NULL)  {
            ipc_status.nb_bytes = ipc_ch->read_ptr.total_num;
            ipc_status.channel  = &ipc_ch->ch;
            ipc_ch->cfg.read_callback(&ipc_status);
        } else {
            SEM_UNLOCK(&ipc_ch->read_ptr.read_mutex);
        }
        return;
    }

    header = (IPC_DATA_HEADER *)(ipc_ch->read_ptr.temp_buff);
    DEBUG("header->version=%x header->nb_frame=%d read_ptr.temp_buff_phy_addr=%x\n", header->version, header->nb_frame, ipc_ch->read_ptr.temp_buff_phy_addr);

    ipc_memcpy_buf2node.node_index    = 0;
    ipc_memcpy_buf2node.frame_index   = 0;
    //ipc_memcpy_buf2node.total_size    = sizeof(IPC_DATA_HEADER_INDEX) + sizeof(IPC_FRAME_DESCRIPTOR) * (ipc_memcpy_buf2node.header->nb_frame);
    ipc_memcpy_buf2node.total_size    = 0;
    ipc_memcpy_buf2node.ipc_ch        = ipc_ch;
    ipc_memcpy_buf2node.header        = (IPC_DATA_HEADER *)(ipc_ch->read_ptr.temp_buff);
    ipc_memcpy_buf2node.buf_phy       = ipc_ch->read_ptr.temp_buff_phy_addr + 
        (sizeof(IPC_DATA_HEADER_INDEX) + (sizeof(IPC_FRAME_DESCRIPTOR) * (ipc_memcpy_buf2node.header->nb_frame)));

    /* set DMA parameters, then start DMA transfer */
    size = ipc_memcpy_buf2node.header->frames[ipc_memcpy_buf2node.frame_index].length;
    if( size > ipc_ch->read_ptr.node_ptr[0].length) {
        size = ipc_ch->read_ptr.node_ptr[0].length;
    }

    ipc_ch->read_ptr.node_ptr[0].length = size;

    /*change for endian*/                                                                                                                                                      
    if (ipc_memcpy_buf2node.header->nb_frame%2)
    { 
        adr = (unsigned long *) __arch_ioremap(ipc_ch->read_ptr.temp_buff_phy_addr +(sizeof(IPC_DATA_HEADER_INDEX) +  (sizeof(IPC_FRAME_DESCRIPTOR) * (ipc_memcpy_buf2node.header->nb_frame))-sizeof(short))
                                                                         ,W3G_IPC_SHARED_MEMORY_SIZE,MT_DEVICE);
        if (adr) usb_ipc_exchange_endian32_short(adr);
        else  panic("Error in %s: ioremap returned NULL adr\n",__FUNCTION__);
    }									     

    dmac_clean_range(ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].data_ptr,
                     (void *) ((ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].data_ptr)+size));
    outer_clean_range((unsigned long)ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].data_ptr,
                     (unsigned long) ((ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].data_ptr)+size));
  
    /* set DMA parameters, then start DMA transfer */
    OMAP_DMA_MEM2MEM_RCV(ipc_memcpy_buf2node.dma_ch, ipc_memcpy_buf2node.buf_phy, NODE_BUF_PHYS_ADDR(ipc_ch->read_ptr.node_ptr[ipc_memcpy_buf2node.node_index].data_ptr), size);
      
}
#endif
