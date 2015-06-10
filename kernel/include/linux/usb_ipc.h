/*
 * Copyright (c) 2007 - 2009 Motorola, Inc, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Changelog:
 * Date            Author          Comment
 * -----------------------------------------------------------------------------
 * 04/15/2009	   Motorola	       IPC using mailboxes
 * 12/07/2007      Motorola        USB-IPC initial
 * 03/22/2007      Motorola        USB-IPC header support
 * 
 */
 
/*!
 * @file usb_ipc.h
 * @brief Header file for USB-IPC
 *
 * This is the generic portion of the USB-IPC driver.
 *
 * @ingroup IPCFunction
 */


#ifndef _USB_IPC_H_
#define _USB_IPC_H_

#include <linux/ipc_api.h>

#ifndef __KERNEL__

/* device node for BP log:  major=180, minor=250 */
#define USB_IPC_LOG_DRV_NAME        "/dev/logger"

#else  /*  */

/* device name in kernel */
#define USB_IPC_LOG_DRV_NAME       "logger"
#define USB_IPC_LOG_MINOR_NUM      250

/*buffers in shared memory*/
#ifdef CONFIG_MACH_W3G_LTE_DATACARD
#define IPC_BUFF_RX_AP_BASE 0x808F8000
#define IPC_BUFF_TX_AP_BASE 0x808FBCBC
#else
#define IPC_BUFF_RX_AP_BASE 0x807F8000
#define IPC_BUFF_TX_AP_BASE 0x807FBCBC
#endif

/* define IPC channels which will map with each USB class driver */
typedef enum {
	IPC_DATA_CH_NUM = 0,
	IPC_LOG_CH_NUM,
	MAX_USB_IPC_CHANNELS
} USB_IPC_CHANNEL_INDEX;

/* define the read/write type */
typedef enum {
        /* read type */
        HW_CTRL_IPC_READ_TYPE,
        HW_CTRL_IPC_READ_EX2_TYPE,

        /* write type */
        HW_CTRL_IPC_WRITE_TYPE,
        HW_CTRL_IPC_WRITE_EX_CONT_TYPE,
        HW_CTRL_IPC_WRITE_EX_LIST_TYPE,
        HW_CTRL_IPC_WRITE_EX2_TYPE
} IPC_CHANNEL_ACCESS_TYPE;

/* macros for LinkDriver read/write semphore */
#define SEM_LOCK_INIT(x)     init_MUTEX_LOCKED(x)
#define SEM_LOCK(x)          down(x)
#define SEM_UNLOCK(x)        up(x)

/* if defined, the temporary buffer is used to merge scatter buffers into one frame buffer */
/* only useful, if one frame is saved into more than one scatter buffers */
//#define IPC_USE_TEMP_BUFFER
#define USE_IPC_FRAME_HEADER
/* use DMA to merge scatter buffer into one frame buffer. ONLY valid while USE_IPC_FRAME_HEADER is defined  */
#if defined(USE_IPC_FRAME_HEADER)
#define USE_OMAP_SDMA
#endif

/* Structure used to manage data read/write of IPC APIs */
typedef struct  {

    /* parameters for IPC channels */
    HW_CTRL_IPC_CHANNEL_T ch;
    HW_CTRL_IPC_OPEN_T    cfg;
    int                   open_flag;
    int                   read_flag;
    int                   write_flag;
    int                   max_node_num;  /* max node number for xxx_write/read_ex2 */

    /* read APIs parameters */
    IPC_CHANNEL_ACCESS_TYPE  read_type;  /* indicate read function types: xxx_read or xxx_read_ex2 */
    struct   {                           /* all parameters used for xxx_read/read_ex2() function call */
        struct {
                unsigned char *buf;
                unsigned short nb_bytes;
        } gen_ptr;

        /* the following parameters are used as the node descriptor read operation */
        HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *node_ptr;
        int  node_index;
        int  node_num;
#if defined(IPC_USE_TEMP_BUFFER) || defined(USE_IPC_FRAME_HEADER)
        unsigned char *temp_buff;   // temporary buffer if the node buffer is smaller than EP maxpacksize or frame size
#if defined(USE_OMAP_SDMA)
        dma_addr_t         temp_buff_phy_addr;
#endif
        int temp_buff_flag;         // flag to indicat whether temporary buffer is used
#endif
        int        total_num;               // totally read byte number for callback arguments
        struct semaphore read_mutex;   /* only useful, if LinkDriver read callback is NULL */
    } read_ptr;

    /* write API parameters */
    IPC_CHANNEL_ACCESS_TYPE  write_type; /* indicate write function types: xxx_write, xxx_write_ex, or xxx_write_ex2 */
    struct   {
        struct {
                unsigned char *buf;
                unsigned short nb_bytes;
        } gen_ptr;

        HW_CTRL_IPC_CONTIGUOUS_T           *cont_ptr;
        HW_CTRL_IPC_LINKED_LIST_T          *list_ptr;

        /* the following parameters are used as the node descriptor read operation */
        HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *node_ptr;
        int  node_index;
        int  node_num;
        int  end_flag;              // indicate whether the last transmit is a zero package of all frames
        int  total_num;             // totally write byte number for callback arguments
        struct semaphore  write_mutex;  /* in case, LinkDriver write callback is NULL */
#if defined(IPC_USE_TEMP_BUFFER) || defined(USE_IPC_FRAME_HEADER)
        unsigned char *temp_buff;   // temporary buffer if the node buffer is smaller than EP maxpacksize or frame size
#if defined(USE_OMAP_SDMA)
        dma_addr_t         temp_buff_phy_addr;
#endif
        int temp_buff_flag;         // flag to indicat whether temporary buffer is used
#endif
    } write_ptr;

#ifdef USE_IPC_FRAME_HEADER
    unsigned long max_temp_buff_size;
#endif
   
} USB_IPC_API_PARAMS;


#endif  //__KERNEL__

#endif // _USB_IPC_H_


