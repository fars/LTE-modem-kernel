/*
 * Copyright (C) 2007-2009, 2011 Motorola, Inc
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
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/07/2007      Motorola        USB-IPC initial
 * 03/22/2008      Motorola        USB-IPC header support
 * 05/09/2008      Motorola        Change Copyright and Changelog
 * 07/09/2008      Motorola        Change MAX_FRAME_SIZE for Phase 1
 * 11/03/2008      Motorola        Support sequence number
 * 03/06/2009      Motorola        Use mailboxes instead of USB
 * 05/06/2009      Motorola        Merge for IPC phase II: increase number of frames to 10
 * 05/07/2009      Motorola        Add IPC Checksum
 * 05/22/2009      Motorola        Modify AP buffers for LOG through IPC for datacard
 * 06/04/2009      Motorola        Add prototypes for BP panic through IPC for datacard
 * 07/16/2009      Motorola        Support short msg handling
 * 09/25/2009      Motorola        Free mailbox 2 and 3 (they will be used in the future for 4G HIF)
 * 07/27/2011      Motorola        Updated api names to make more sense.
 */

/*!
 * @file ipc_api.h
 * @brief Header file for USB-IPC
 *
 * This is the generic portion of the USB-IPC driver.
 *
 * @ingroup IPCFunction
 */


#ifndef __IPC_API_H__
#define __IPC_API_H__

#include <linux/interrupt.h>

/* the following macros shall be same with BP and NetMux */
#define MAX_FRAME_SIZE           1552
#define MAX_FRAME_NUM            10

/* supported max node number for write_ex2/read_ex2 */
#define MAX_IPC_EX2_NODE_NUM     50

/* bits for node comand */
#define NODE_DESCRIPTOR_END_BIT  0x4000
#define NODE_DESCRIPTOR_LAST_BIT 0x8000
#define NODE_DESCRIPTOR_DONE_BIT 0x2000

/* Log buffer in AP */
/* The 20 buffers of 3K used by USB were replaced by 2 buffers of 50KB
for datacard IPc using mailboxes and shared memory  */
#define MAX_LOG_BUF_NUM   2
#define MAX_LOG_BUF_SIZE  50*1024

/* the extra-header will be used for DATA channel */
#define IPC_FRAME_VERSION   0x0200

#define MAILBOX_CHANNEL_LOG  0
#define MAILBOX_CHANNEL_DATA 1<<29

#define MAILBOX_MSG_TYPE_TX   0
#define MAILBOX_MSG_TYPE_ACK  1<<30
#define MAILBOX_MSG_TYPE_NACK 2<<30

typedef struct {
    u16 length;
} IPC_FRAME_DESCRIPTOR;

typedef struct {
    u16 version;
    u16 nb_frame;
    u8  sequence_number;
    u8  options;
    u16 checksum;
    IPC_FRAME_DESCRIPTOR frames[MAX_FRAME_NUM];
} IPC_DATA_HEADER;

typedef struct {
    u16 version;
    u16 nb_frame;
    u8  sequence_number;
    u8  options;
    u16 checksum;
} IPC_DATA_HEADER_INDEX;


/*!
 * This enum defines the different status of an IPC link
 */
typedef enum {
    HW_CTRL_IPC_STATUS_OK = 0,
    HW_CTRL_IPC_STATUS_CHANNEL_UNAVAILABLE,
    HW_CTRL_IPC_STATUS_IPC_SUSPENDED,
    HW_CTRL_IPC_STATUS_READ_ON_GOING,
    HW_CTRL_IPC_STATUS_WRITE_ON_GOING,
    HW_CTRL_IPC_STATUS_INIT_ALREADY_CALLED,
    HW_CTRL_IPC_STATUS_ERROR
} HW_CTRL_IPC_STATUS_T;

/*!
 * This typedef defines the ioctls available
 * for kernel modules using IPC.
 */
typedef enum {
    HW_CTRL_IPC_SET_READ_CALLBACK = 0,
    HW_CTRL_IPC_SET_WRITE_CALLBACK,
    HW_CTRL_IPC_SET_NOTIFY_CALLBACK,
    HW_CTRL_IPC_SET_MAX_CTRL_STRUCT_NB
} HW_CTRL_IPC_IOCTL_ACTION_T;


/*!
 * Definition of IPC channel types
 *
 * There are currently three types of channels:
 *
 * - Short Message channels. Used to transfer 32-bits
 *   messages from MCU to DSP and vice versa
 *
 * - Packet Data channels. Useful to transfer data between
 *   the two cores.
 *
 * - Logging channel. This type of channel is read-only from
 *   the MCU. It is used to report log events to the MCU.
 *
 */
typedef enum {
    HW_CTRL_IPC_PACKET_DATA = 0,
    HW_CTRL_IPC_CHANNEL_LOG,
    HW_CTRL_IPC_SHORT_MSG
} HW_CTRL_IPC_CHANNEL_TYPE_T;

/*!
 * This enum defines the write modes IPC can support.
 *
 * - Contigous mode: All data is stored in a contigous
 *   memory zone.
 *
 * - LinkedList mode: Data can be presented in chunks of
 *   non-contiguous memory. Up to 11 chunks can be processed
 *   by this mode.
 *
 *  Note that only Packet Data channels can support LinkedList mode.
 */
typedef enum {
    HW_CTRL_IPC_MODE_CONTIGUOUS = 0,
    HW_CTRL_IPC_MODE_LINKED_LIST
} HW_CTRL_IPC_MODE_T;

typedef struct {
} HW_CTRL_IPC_INIT_T;

typedef struct {
    /*!
     * the channel from which data was read
     */
    int channel_nb;
} HW_CTRL_IPC_CHANNEL_T;

/*!
 * A structure of this type is passed as parameter when a read
 * callback is invoked. This normally happens when a read transfer has been
 * completed.
 */
typedef struct {
    /*!
     * the channel handler from which data was read
     */
    HW_CTRL_IPC_CHANNEL_T *channel;

    /*!
     * number of bytes read.
     */
    int nb_bytes;
} HW_CTRL_IPC_READ_STATUS_T;

/*!
 * A structure of this type is passed as parameter when a write
 * callback is invoked. This normally happens when a write transfer has been
 * completed.
 */
#define HW_CTRL_IPC_WRITE_STATUS_T     HW_CTRL_IPC_READ_STATUS_T

/*!
 * A structure of this type is passed as parameter when a notify
 * callback is invoked. This happens only when a transfer has been
 * abnormally terminated.
 */
typedef struct {
    /*!
     * the channel handler from which data was read
     */
    HW_CTRL_IPC_CHANNEL_T *channel;

    /*!
     * status code
     */
    HW_CTRL_IPC_STATUS_T status;
} HW_CTRL_IPC_NOTIFY_STATUS_T;

/*
 * This struct defines parameters needed to
 * execute a write using the linked list mode
 */
typedef struct HW_CTRL_IPC_LINKED_LIST_T {
    /*!
     * pointer to data to be transfered
     */
    unsigned char *data_ptr;

    /*!
     * Lenght of data
     */
    unsigned int length;

    /*!
     * Pointer to the next chunk of memory containing data
     * to be transferred
     */
    struct HW_CTRL_IPC_LINKED_LIST_T *next;
} HW_CTRL_IPC_LINKED_LIST_T;

/*
 * This struct defines parameters needed to
 * execute a write using the normal mode
 */
typedef struct {
    /*!
     * pointer to data to be transfered
     */
    unsigned char *data_ptr;

    /*!
     * Lenght of data
     */
    unsigned int length;
} HW_CTRL_IPC_CONTIGUOUS_T;

/*
 * This structure is used by the write_ex function
 * which is in charge of execute the LinkedList mode
 * transfer.
 */
typedef struct {
    /*!
     * Type of transfer to execute
     */
    HW_CTRL_IPC_MODE_T ipc_memory_read_mode;

    /*!
     * Pointer to a buffer holding all data to
     * transfer orpointer to a buffer holding
     * first chunk of data to transfer
     */
    union {
        HW_CTRL_IPC_CONTIGUOUS_T *cont_ptr;
        HW_CTRL_IPC_LINKED_LIST_T *list_ptr;
    } read;
} HW_CTRL_IPC_WRITE_PARAMS_T;

/*
 * Structure used to pass configuration parameters needed
 * to open an IPC channel.
 */
typedef struct {
    /*!
     * type of IPC channel to open
     */
    HW_CTRL_IPC_CHANNEL_TYPE_T type;

    /*!
     * index defining the physical channel
     * that will be used for this IPC channel
     */
    int index;

    /*!
     * read callback provided by the user, called when a read
     * transfer has been finished
     */
    void (*read_callback) (HW_CTRL_IPC_READ_STATUS_T * status);

    /*!
     * write callback provided by the user, called when a write
     * transfer has been finished
     */
    void (*write_callback) (HW_CTRL_IPC_WRITE_STATUS_T * status);

    /*!
     * notify callback provided by the user, called when an error
     * occurs during a transfer.
     */
    void (*notify_callback) (HW_CTRL_IPC_NOTIFY_STATUS_T * status);
} HW_CTRL_IPC_OPEN_T;

/*!@param *data_control_struct_ipcv2
 *   Data Node Descriptor (Buffer Descriptor):
 *------------------------------------------------------------------------------
 *| 31 30  29  28  27  26  25  24  23  22  21  20  19  18  17  16  15  0|
 *------------------------------------------------------------------------------
 *| L  E   D   R   R   R   R   R   |<--Reserved-->|<- Length-> |
 *------------------------------------------------------------------------------
 *| <---------------------------- Data Ptr ----------------------------------->|
 *------------------------------------------------------------------------------
 *
 * L bit (LAST): If set, means that this buffer of data is the last buffer of the frame
 * E bit (END): If set, we reached the end of the buffers passed to the function
 * D bit (DONE): Only valid on the read callback. When set, means that the buffer has been
 * filled by the SDMA.
 * Length: Length of data pointed by this node in bytes
 * Data Ptr: Pointer to the data pointed to by this node.
 */
typedef struct ipc_dataNodeDescriptor {
    unsigned short length;
    unsigned short comand;
    void *data_ptr;
} HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T;

/*!
 * This function is called when kernel is loading the module.
 * It initializes all low-level resources needed by the POSIX
 * layer in order to make SDMA/MU transfers.
 *
 * @param init_params   Pointer to a struct containing global
 *                      initialization parameters.
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_init(const HW_CTRL_IPC_INIT_T * init_params);

/*!
 * Opens an IPC link. This functions can be called directly by kernel
 * modules. POSIX implementation of the IPC Driver also calls it.
 *
 * @param config        Pointer to a struct containing configuration para
 *                      meters for the channel to open (type of channel,
 *                      callbacks, etc)
 *
 * @return              returns a virtual channel handler on success, a NULL pointer otherwise.
 */
HW_CTRL_IPC_CHANNEL_T *hw_ctrl_ipc_open(const HW_CTRL_IPC_OPEN_T * config);

/*!
 * Close an IPC link. This functions can be called directly by kernel
 * modules. POSIX implementation of the IPC Driver also calls it.
 *
 * @param channel       handler to the virtual channel to close.
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_close(HW_CTRL_IPC_CHANNEL_T * channel);

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
                                       void *param);
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
                                           ctrl_ptr);

/*!
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
                                          ctrl_ptr);

#ifdef CONFIG_MAILBOX_IPC_ARCH_V2
/*!
 * This function is used to send acknowledge to a read message
 * transfers.
 *
 * u32 ui_msg: acknowledgement message
 *
 * @return              returns HW_CTRL_IPC_STATUS_OK on success, an error code
 *                      otherwise.
 */
irqreturn_t mailbox_ISR(int irq, void *p);
void mailbox_ipc_read_callback(u32 msg);
void mailbox_ipc_write_callback(u32 msg);
int mailbox_write(u16 length);
HW_CTRL_IPC_STATUS_T hw_ctrl_ipc_send_read_ack(u32 ui_msg);
#else
irqreturn_t mailbox_ISR(int irq, void *p);
void mailbox_ipc_read_callback(void);
void mailbox_ipc_write_callback(void);
int mailbox_write(u16 length);
#endif

void mailbox_dlog_ack(void);
void mb_ipc_log_readscheduler(unsigned long mb_buff_virt_addr, unsigned int mb_buff_length);

int mailbox_vr_available(void);
void mailbox_panic_send_cmd(u32 panic_cmd);
void mb_ipc_panic_copy_to_drv_buf(u32 msg);
#ifdef CONFIG_MAILBOX_IPC_USE_CHECKSUM
void ipc_buffer_add_checksum(unsigned long * ptr_buf);
unsigned char ipc_buffer_check_checksum(unsigned long * ptr_buf);
#endif //#ifdef CONFIG_MAILBOX_IPC_USE_CHECKSUM

#endif                          //__IPC_API_H__
