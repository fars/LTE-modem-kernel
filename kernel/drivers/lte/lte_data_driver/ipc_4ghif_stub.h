/*
 * Copyright (C) 2009 Motorola, Inc
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
 *
 */
 

#ifndef __IPC_4GHIF_H__
#define __IPC_4GHIF_H__

#include <linux/interrupt.h>



/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\//\/\/\/\/\/\/\/\/\ structures / types / enums / constants /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/ 

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


/* packet structure. Preffered rather than using directly sk_buff */

typedef enum
{
    PKT_TYPE_NONE=0,
    PKT_TYPE_DL_DATA,
    PKT_TYPE_DL_MGMT,
    PKT_TYPE_DL_MON,
    PKT_TYPE_UL_DATA,
    PKT_TYPE_UL_MGMT
} IPC_4GHIF_PKT_TYPE;

typedef struct _sdma_packet
{
    u8    packet_type;    // packet type
    void* pbuf;           // virtual address of the buffer on AP memory                     
    size_t buf_size;      // size of the buffer in bytes
    unsigned long user;   // reserved for caller
    void* puser;          // reserved for caller
} sdma_packet;


/* callbacks */

typedef void (*fp_bp_ready)(void);
typedef void (*fp_receive_dl)(u8 packet_type, size_t buf_size);
typedef void (*fp_dl_complete)(sdma_packet* packet);
typedef void (*fp_ul_complete)(sdma_packet* packet);
typedef void (*fp_ul_resume)(void);

typedef struct _ipc_4ghif_callbacks
{
    fp_bp_ready    bp_ready;
    fp_receive_dl  receive_dl;
    fp_dl_complete dl_complete;
    fp_ul_complete ul_complete;
    fp_ul_resume   ul_resume;
} ipc_4ghif_callbacks;


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

typedef struct _fifo_msg
{
    u8 type;               // message type                     
    u8 tag;                // bearer ID
    u16 length;            // data length
    void* start_address;   // system physical data address
} fifo_msg;


/* Shared memory settings */

//#define SHARED_MEMORY_TEST_NEMO

#ifdef SHARED_MEMORY_TEST_NEMO

    #define IPC_SHARED_PHYS_ADDR 0x807FFA00
    #define IPC_SHARED_VIRT_ADDR 0xD83FFA00
  
    #define DATA_DL_SIZE 0x100
    #define DATA_UL_SIZE 0x100
    #define FIFO_UL_SIZE 0x100 
    #define FIFO_DL_SIZE 0x100

    #define FIFO_DL_MSG_NB   (FIFO_DL_SIZE/sizeof(fifo_msg))
    #define FIFO_UL_MSG_NB   (FIFO_DL_SIZE/sizeof(fifo_msg))

    #define DATA_DL_BASE      IPC_SHARED_VIRT_ADDR
    #define DATA_UL_BASE      (IPC_SHARED_VIRT_ADDR+0x100)
    #define FIFO_UL_READ_IND  (IPC_SHARED_VIRT_ADDR+0x200)
    #define FIFO_UL_WRITE_IND (IPC_SHARED_VIRT_ADDR+0x204)
    #define FIFO_DL_READ_IND  (IPC_SHARED_VIRT_ADDR+0x208)
    #define FIFO_DL_WRITE_IND (IPC_SHARED_VIRT_ADDR+0x20c)
    #define FIFO_UL_BASE      (IPC_SHARED_VIRT_ADDR+0x300)
    #define FIFO_DL_BASE      (IPC_SHARED_VIRT_ADDR+0x400)

    #define DATA_DL_PACKET_MAX_SIZE 64
    #define DATA_UL_PACKET_MAX_SIZE 64
    
    #define DATA_UL_MM_BASE_ADR (DATA_UL_BASE+IPC_SHARED_PHYS_ADDR-IPC_SHARED_VIRT_ADDR)   //0x807FFB00
    #define DATA_UL_MM_SIZE     64                                   //64
    #define DATA_UL_MM_NB       (DATA_UL_SIZE/DATA_UL_MM_SIZE)       //4 

#else

    #define IPC_SHARED_PHYS_ADDR  0x00080000
    #define IPC_SHARED_VIRT_ADDR  0xD8480000  //UU: need to add settings for the MMU segment virt:0xD8400000 phys:0

    #define DATA_DL_SIZE (122 * 1024)
    #define DATA_UL_SIZE (385 * 1024)
    #define FIFO_UL_SIZE (2 * 1024) 
    #define FIFO_DL_SIZE (2 * 1024)
    
    #define FIFO_DL_MSG_NB   (FIFO_DL_SIZE/sizeof(fifo_msg))
    #define FIFO_UL_MSG_NB   (FIFO_DL_SIZE/sizeof(fifo_msg))

    #define DATA_DL_BASE      IPC_SHARED_VIRT_ADDR
    #define DATA_UL_BASE      (IPC_SHARED_VIRT_ADDR+0x1e800)
    #define FIFO_UL_READ_IND  (IPC_SHARED_VIRT_ADDR+0x7ec00)
    #define FIFO_UL_WRITE_IND (IPC_SHARED_VIRT_ADDR+0x7ec04)
    #define FIFO_DL_READ_IND  (IPC_SHARED_VIRT_ADDR+0x7ec08)
    #define FIFO_DL_WRITE_IND (IPC_SHARED_VIRT_ADDR+0x7ec0c)
    #define FIFO_UL_BASE      (IPC_SHARED_VIRT_ADDR+0x7f000)
    #define FIFO_DL_BASE      (IPC_SHARED_VIRT_ADDR+0x7f800)

    #define DATA_DL_PACKET_MAX_SIZE 8188
    #define DATA_UL_PACKET_MAX_SIZE 8188 
    
    #define DATA_UL_MM_BASE_ADR (DATA_UL_BASE+IPC_SHARED_PHYS_ADDR-IPC_SHARED_VIRT_ADDR)  //0x0009e800
    #define DATA_UL_MM_SIZE     8192                                 //8192 (8K) esier to debug than 8188 for DATA_UL_PACKET_MAX_SIZE
    #define DATA_UL_MM_NB       (DATA_UL_SIZE/DATA_UL_MM_SIZE)       //48 (can be reduced to test ressources full)   

#endif










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
Name: IPC_4GHIF_submit_dl
Description:  
-	This function is used to start a frame transfer from shared memory to skbuff using sDMA 
Parameters:
-	sdma_packet* packet: structure containing pointer to the data to transfer. 
            On call entry, packet_type and buf_size field are initialized.  
	    Buffer address field filled by caller is virtual
Return value: 
-	IPC_4GHIF_STATUS_OK: transfer successfully started
-	IPC_4GHIF_STATUS_BUSY:  previous DL transferred not ended
-	IPC_4GHIF_STATUS_INVALID_PARAM: NULL pointer      
-	IPC_4GHIF_STATUS_NO_FRAME: No DL frame available
-	IPC_4GHIF_STATUS_BP_NOT_READY: BP did not reported yet it is ready
-       IPC_4GHIF_STATUS_AP_NOT_READY: Data driver did not responded to the BP, it is ready
-	IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes:
-	This function should be call after being informed by IPC_4GHIF_receive_dl_callback that a new DL frame is available  
-	At the end of sDMA transfer, the callback registered by IPC_4GHIF_dl_complete_callback is scheduled
-	A new submit command must not be requested until the callback is called.
-	Callback function is called with the address of the packet structure passed as parameter. 
-	If caller wants to use the packet structure in the callback, it should be still available on caller side.
-	Function panics if cannot obtain a DMA channel (should not occur)
-       Caller needs to allocate the buffer
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_submit_dl(sdma_packet* packet);


/*=====================================================================================================================
Name: IPC_4GHIF_submit_ul
Description:  
-	This function is used to start a frame transfer from skbuff to shared memory using sDMA and then to the BP. 
Parameters:
-	sdma_packet* packet: structure containing pointer to the data to transfer
                             Buffer address field filled by caller is virtual
			     packet_type field can be only PKT_TYPE_UL_DATA or PKT_TYPE_UL_MGMT
-	u8 bearerID: for UL-DATA: bearer for this frame 
                     for UL_MGMT: not used 
Return value: 
-	IPC_4GHIF_STATUS_OK: transfer successfully started
-	IPC_4GHIF_STATUS_BUSY:  previous UL transferred not ended
-	IPC_4GHIF_STATUS_INVALID_PARAM: NULL pointer or size    
-	IPC_4GHIF_STATUS_RESOURCES_FULL: Cannot transmit the packet because shared memory (DATA_UL or FIFO_UL) is full
-	IPC_4GHIF_STATUS_BP_NOT_READY: BP did not reported yet it is ready
-       IPC_4GHIF_STATUS_AP_NOT_READY: Data driver did not responded to the BP, it is ready
-	IPC_4GHIF_STATUS_CLOSED: 4G HIF IPC already closed
Notes: 
-	At the end of sDMA transfer, the callback registered by IPC_4GHIF_ul_complete_callback is invoked.
-	A new submit must not be requested until the callback is called.
-	Callback function is called with the address of the packet parameter. 
-	If caller wants to use the packet structure in the callback, it should be still available on caller side.
-	Function panics if cannot obtain a DMA channel (should not occure)
-	If it is not possible to send the packet because shared memory is full, then function returns the IPC_4GHIF_STATUS_RESOURCES_FULL error. 
            In this case the IPC_4GHIF_ul_resume_callback will be scheduled as soon as space will be available in shared memory.
-	In uplink direction, it is needed to indicate the bearer of the frame to the BP.  
-	NOTE: For current USB LTE, the bearer ID is pre-pended to the payload.  We need to talk about this
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_submit_ul(sdma_packet* packet, u8 bearerID);


/*=====================================================================================================================
Name: IPC_4GHIF_cancel_ul
Description:  
-	This function is used to cancel an UL frame transfer started by IPC_4GHIF_submit_ul 
Parameters:
-	None
Return value: 
-	IPC_4GHIF_STATUS_OK
Notes: 
-	This function can be call if caller do not receive uplink complete callback after IPC_4GHIF_submit_ul
-       Actual implementation is to panic to signal this issue. 
-       This should not happend so if caller detects this issue it is needed to investiguate 
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
-	This function can be call if caller do not receive downlink complete callback after IPC_4GHIF_submit_dl
-       Actual implementation is to panic to signal this issue. 
-       This should not happend so if caller detects this issue it is needed to investiguate 
=====================================================================================================================*/

IPC_4GHIF_STATUS IPC_4GHIF_cancel_dl(void);


 

#endif    //__IPC_4GHIF_H__
