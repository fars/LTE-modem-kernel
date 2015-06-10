/*
 * Copyright (C) 2007-2010 Motorola, Inc
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
 * 12-oct-2009       Motorola         file creation
 * 12-jan-2010       Motorola         added new memory manager
 * 15-jan-2010       Motorola         move mm prototypes and defines in a local header file
 * 25-jan-2010       Motorola         phase 2: group frames for sDMA transfers
 * 08/31/2010        Motorola         Fix Klockwork defects
 * 25-may-2011       Motorola         Need to add logging to track IPC ctrl msg round trip time
 */

#ifndef __IPC_4GHIF_H__
#define __IPC_4GHIF_H__

#include <linux/interrupt.h>



/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\//\/\/\/\/\/\/\/\/\ structures / types / enums / constants /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#define DATA_DL_PACKET_MAX_SIZE 8188
#define DATA_UL_PACKET_MAX_SIZE 8188

/* Warning: must not be more than 31*/
/* unless descriptor section in shared memory is increased*/
#define IPC_4GHIF_MAX_NB_UL_FRAMES 31
#define IPC_4GHIF_MAX_NB_DL_FRAMES 31

/* For 3GSM IPC timing stats */
#define DL_RCV2ACK_TIME_ARRAY_SIZE 30


/* errors returned by the IPC 4GHIF API functions */

typedef enum
{
    IPC_4GHIF_STATUS_OK = 0,           // No error
    IPC_4GHIF_STATUS_INVALID_PARAM,    // Invalid parameters (ie: NULL pointers)
    IPC_4GHIF_STATUS_BUSY,             // UL or DL transfer is already on going
    IPC_4GHIF_STATUS_CLOSED,           // IPC not open
    IPC_4GHIF_STATUS_ALREADY_OPEN,     // IPC already open
    IPC_4GHIF_STATUS_RESOURCES_FULL,   // No resource to send UL frame
    IPC_4GHIF_STATUS_NO_FRAME,         // No DL frame available
    IPC_4GHIF_STATUS_BP_NOT_READY,     // BP has not reported yet it is ready
    IPC_4GHIF_STATUS_AP_NOT_READY,     // Data driver did not responded to the BP, it is ready
    IPC_4GHIF_STATUS_NO_TRANS_ON_GOING // No UL or DL transfer on going
} IPC_4GHIF_STATUS;


typedef enum
{
    PKT_TYPE_NONE=0,
    PKT_TYPE_DL_DATA,
    PKT_TYPE_DL_MGMT,
    PKT_TYPE_DL_MON,
    PKT_TYPE_UL_DATA,
    PKT_TYPE_UL_MGMT
} IPC_4GHIF_PKT_TYPE;

/* FIFO constants */

typedef enum
{
    IPC_4GHIF_FIFO_STATUS_OK = 0,     // No error
    IPC_4GHIF_FIFO_STATUS_EMPTY,      // FIFO is empty (no new message)
    IPC_4GHIF_FIFO_STATUS_FULL        // FIFO FULL (cannot add new message)
} IPC_4GHIF_FIFO_STATUS;

typedef enum tIPC_MSG_TYPE_T
{
    eIPC_MSG_TYPE_INIT_REQ,
    eIPC_MSG_TYPE_INIT_CNF,
    eIPC_MSG_TYPE_DL_DATA,
    eIPC_MSG_TYPE_DL_MGMT,
    eIPC_MSG_TYPE_DL_MON,
    eIPC_MSG_TYPE_DL_ACK,
    eIPC_MSG_TYPE_UL_DATA,
    eIPC_MSG_TYPE_UL_MGMT,
    eIPC_MSG_TYPE_UL_ACK,

    eIPC_MSG_TYPE_LAST
} tIPC_MSG_TYPE;

/* packet structure. Prefered rather than using directly sk_buff */
typedef struct _sdma_packet
{
    u8    packet_type;    // packet type
    void* pbuf;           // virtual address of the buffer on AP memory
    size_t buf_size;      // size of the buffer in bytes
    bool transferred_status;  // frame transferred or not by the sDMA
    u8 bearerID;              // bearer of the frame
    unsigned long user;   // reserved for caller
    void* puser;          // reserved for caller
} sdma_packet;

/* packet_info structure defines incoming packet characteristics */
typedef struct _packet_info
{
    u8    packet_type;    // packet type
    size_t buf_size;      // size of the buffer in bytes
} packet_info;


typedef struct _fifo_msg
{
    u8 type;               // message type
    u8 tag;                // bearer ID
    u16 length;            // data length
    void* start_address;   // system physical data address
} fifo_msg;


/* IPC stats */

typedef struct _ipc_4ghif_stats
{
    u32 ipc_4ghif_stat_ul_sdma_start;   // UL stats
    u32 ipc_4ghif_stat_ul_sdma_done;    //
    u32 ipc_4ghif_stat_ul_packets;      //
    u32 ipc_4ghif_stat_ul_bytes;        //
    u32 ipc_4ghif_stat_ul_gbytes;       //
    u32 ipc_4ghif_stat_ul_submit_full;  //
    u32 ipc_4ghif_stat_ul_resume_full;  //
    u32 ipc_4ghif_stat_ul_ack;          //

    u32 ipc_4ghif_stat_dl_notify;       // DL stats
    u32 ipc_4ghif_stat_dl_sdma_start;   //
    u32 ipc_4ghif_stat_dl_sdma_done;    //
    u32 ipc_4ghif_stat_dl_packets;      //
    u32 ipc_4ghif_stat_dl_bytes;        //
    u32 ipc_4ghif_stat_dl_gbytes;       //
    u32 ipc_4ghif_stat_dl_ack;          //

    u32 ipc_4ghif_stat_irq_nb;

    u32  ipc_3gsm_bp_data_dl;           // 3GSM IPC BP DL stats
    u32  ipc_3gsm_bp_ack_dl;
    u32  ipc_3gsm_bp_data_ul;           // 3GSM IPC BP UL stats
    u32  ipc_3gsm_bp_ack_ul;

    u32 ipc_3gsm_ap_data_dl;            // 3GSM IPC AP DL stats
    u32 ipc_3gsm_ap_ack_dl;
    u32 ipc_3gsm_ap_read_ex2;
    u32 ipc_3gsm_ap_data_ul;            // 3GSM IPC AP UL stats
    u32 ipc_3gsm_ap_ack_ul;
    u32 ipc_3gsm_ap_write_ex2;
    u32 dl_rcv2ack_time[DL_RCV2ACK_TIME_ARRAY_SIZE];  // 3GSM IPC BP DL timing stats
}
ipc_4ghif_stats;


/* callbacks */

typedef void (*fp_bp_ready)(void);
typedef void (*fp_receive_dl)(u8 packet_nb, packet_info frames_info[]);
typedef void (*fp_dl_complete)(sdma_packet* packet[]);
typedef void (*fp_ul_complete)(sdma_packet* packet[]);
typedef void (*fp_ul_resume)(void);


typedef struct _ipc_4ghif_callbacks
{
    fp_bp_ready    bp_ready;
    fp_receive_dl  receive_dl;
    fp_dl_complete dl_complete;
    fp_ul_complete ul_complete;
    fp_ul_resume   ul_resume;
} ipc_4ghif_callbacks;




/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

Function	                    Description

IPC_4GHIF_open	                    To initialize and open 4G HIF IPC HW resources. To register callbacks
IPC_4GHIF_close                     To close and free 4G HIF IPC HW resources. To unregister callbacks
IPC_4GHIF_ready                     To signal to BP, AP data driver is ready
IPC_4GHIF_submit_ul	            To send a frame to the BP
IPC_4GHIF_submit_dl	            To receive a frame from the BP


Callback	                    Description

IPC_4GHIF_bp_ready_callback	    Scheduled when BP 4GHIF IPC is ready
IPC_4GHIF_receive_dl_callback       Scheduled when a new DL frame from BP was received
IPC_4GHIF_dl_complete_callback      Scheduled when DL frame has been transferred from shared memory to user skbuff
IPC_4GHIF_ul_complete_callback      Scheduled when UL frame has been transferred from user skbuff to shared memory.
IPC_4GHIF_ul_resume_callback	    Scheduled when resources are available again after an unabaility to send an UL frame.

*/


/*=====================================================================================================================
Name: IPC_4GHIF_open
Description:
-	This function initializes the 4G HIF IPC HW resources (mailboxes, sDMA and shared memory).
-	And registers callbacks
Parameters:
-	Structure containing callback addresses.
Return value:
-	IPC_IPC_4GHIF_STATUS_OK
-	IPC_4GHIF_STATUS_ALREADY_OPEN: IPC already opened previously
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_open(ipc_4ghif_callbacks callbacks);


/*=====================================================================================================================
Name: IPC_4GHIF_close
Description:
-	This function frees the 4G HIF IPC HW resources (mailboxes, sDMA and shared memory).
-	And unregisters the callbacks
Parameters:
-	None
Return value:
-	IPC_4GHIF_STATUS_OK
-	IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes:
-	After closing, pending callbacks won't be scheduled.
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_close(void);


/*=====================================================================================================================
Name: IPC_4GHIF_ready
Description:
-	This function signals to the BP that AP data driver is ready by sending INIT_CNF message to the BP.
-       It should be called after receiving the bp_ready callback and when data driver is ready.
Parameters:
-	None
Return value:
-	IPC_4GHIF_STATUS_OK: function sends the message
-	IPC_4GHIF_STATUS_BP_NOT_READY: BP did not reported yet it is ready
-	IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes:
-	INIT_CNF message is only transmitted once to the BP. For instance
        if Data driver closes and opens again the IPC 4GHIF low level driver then INIT_CNF message won't be sent.
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_ready(void);


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
-        If the caller waits between IPC_4GHIF_receive_dl_callback and IPC_4GHIF_submit_dl function, it can get an updated state by calling this function   //UU ADD assumption caller not call submit dl and this in //
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_get_dl_frames_nb(u8* packet_nb, packet_info frames_info_array[IPC_4GHIF_MAX_NB_DL_FRAMES]);


/*=====================================================================================================================
Name: IPC_4GHIF_submit_dl
Description:
-	This function is used to start the transfer of a group of frames from shared memory to skbuff using sDMA
Parameters:
-	u8 packet_nb: number of frames to transfer.
-	sdma_packet* packet[]: array of pointers on  structure containing pointer to the data to transfer. On call entry, the pbuf and buf_size fields are initialized.
-	u8* processing_packet_nb: updated with the number of frames actually in transfer.
Return value:
-	IPC_4GHIF_STATUS_OK: transfer successfully started
-	IPC_4GHIF_STATUS_BUSY:  previous DL transferred not ended
-	IPC_4GHIF_STATUS_INVALID_PARAM: NULL pointer or size
-	IPC_4GHIF_STATUS_NO_FRAME: No DL frame available
-	IPC_4GHIF_STATUS_BP_NOT_READY: BP did not reported yet it is ready
-	IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes:
-	This function should be call after being informed by IPC_4GHIF_receive_dl_callback that a new DL frame is available
-	At the end of sDMA transfer, the callback registered by IPC_4GHIF_dl_complete_callback is scheduled
-	A new submit command must not be requested until the callback is called.
-	Callback function is called with the address of the packet structure array passed as parameter.
-	If caller wants to use the packet structure array in the callback, it should be still available on caller side.
-	Function panics if cannot obtain a DMA channel (should not occur)


=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_submit_dl(u8 packet_nb, sdma_packet* packet[], u8* processing_packet_nb);


/*=====================================================================================================================
Name: IPC_4GHIF_submit_ul
Description:
-	This function is used to start a frame transfer from skbuff to shared memory using sDMA and then to the BP.
Parameters:
-	u8 packet_nb: number of frames to transmit to the BP
-	sdma_packet* packet[]: array of pointer on structures containing addresses of  frames to transfer.
-	u8* processing_packet_nb: number of packet that will be send according available ressources
Return value:
-	IPC_4GHIF_STATUS_OK: transfer successfully started
-	IPC_4GHIF_STATUS_BUSY:  previous UL transferred not ended
-	IPC_4GHIF_STATUS_ERROR: NULL pointer or size
-	IPC_4GHIF_STATUS_RESOURCES_FULL: Cannot transmit the packet because shared memory (DATA_UL or FIFO_UL) is full
-	IPC_4GHIF_STATUS_BP_NOT_READY: BP did not reported yet it is ready
-	IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes:
-	At the end of sDMA transfer, the callback registered by IPC_4GHIF_ul_complete_callback is invoked.
-	A new submit must not be requested until the callback is called.
-	Callback function is called with the array of the packet parameters.
-	If caller wants to use the array of packet structures in the callback, it should be still available on caller side.
-	Function panics if cannot obtain a DMA channel (should not occur)
-	According to IPC resources availability, IPC will not always send all the submitted packets. The returned value in processing_packet_nb indicates the number of packets that are actually sent.
-	If it is not possible to send all the provided packets because shared memory (Data or FIFO) is full, then the function returns the IPC_4GHIF_STATUS_RESOURCES_FULL error. In that case the IPC_4GHIF_ul_resume_callback will be scheduled as soon as space will be available in shared memory. However, IPC will transfer the frames that can be transmitted (in the array order) according to the room available.
-	If less frames than expected are transferred, then the user should wait for the sdma_ul callback and then for the resume_ul callback.
-	A packet of the array cannot be sent if the previous packet in the array cannot be sent. The order in the queue is kept the same. For instance if the memory manager cannot allocate a 2KB block but can allocate a 512B block, no more packet will be transferred, even if a 512B packet is queued. The user will have to wait for more resources available to submit the remaining packets again.
-	In uplink direction, it is needed to indicate the bearer of the frame to the BP.
-	In any case, IPC will not send more packets than IPC_4GHIF_MAX_NB_UL/DL_FRAMES constants
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_submit_ul(u8 packet_nb, sdma_packet* packet[],u8* processing_packet_nb);


/*=====================================================================================================================
Name: IPC_4GHIF_cancel_ul
Description:
-	This function is used to cancel an UL frame transfer started by IPC_4GHIF_submit_ul
Parameters:
-	None
Return value:
-	IPC_4GHIF_STATUS_OK
Notes:
-	This function can be called if the caller does not receive uplink complete callback after IPC_4GHIF_submit_ul
-       Actual implementation is a panic to signal this issue.
-       This should not happend so if the caller detects this issue it is needed to investigate
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_cancel_ul(void);


/*=====================================================================================================================
Name: IPC_4GHIF_cancel_dl
Description:
-	This function is used to cancel a DL frame transfer started by IPC_4GHIF_submit_dl
Parameters:
-	None
Return value:
-	IPC_4GHIF_STATUS_OK
Notes:
-	This function can be calles if the caller does not receive downlink complete callback after IPC_4GHIF_submit_dl
-       Actual implementation is a panic to signal this issue.
-       This should not happend so if the caller detects this issue it is needed to investigate
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_cancel_dl(void);


/*=====================================================================================================================
Name: IPC_4GHIF_get_error_string
Description:
-	This function provides the error string associated to an error code
Parameters:
-       PC_4GHIF_STATUS error_code
-	char* error_string
-	int error_string_size
return value:
-	IPC_4GHIF_STATUS_OK
-	IPC_4GHIF_STATUS_INVALID_PARAM: invalid error code
Notes:
-	None
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_get_error_string(IPC_4GHIF_STATUS error_code, char* error_string, int error_string_size);


/*=====================================================================================================================
Name: IPC_4GHIF_get_statistics_string
Description:
-	This function is used to get 4GHIF IPC low level driver statistics in a string
Parameters:
-	char* sz   : stat string
-       u16 sz_size: stat string max size
Return value:
-	IPC_4GHIF_STATUS_OK
-	IPC_4GHIF_STATUS_INVALID_PARAM: NULL string pointer
Notes:
-       It is recommended to provide a 1024bytes string
-       This function fills a statistics stat string to be more generic than the IPC_4GHIF_get_statistics
-       It also includes UL memory manager statistics
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_get_statistics_string(char* sz, u16 sz_size);


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

IPC_4GHIF_STATUS IPC_4GHIF_debug_order(u32 order);

#endif    //__IPC_4GHIF_H__
