/*
 * Copyright (c) 2009 Motorola, Inc, All Rights Reserved.
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
 * 
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


//#include <linux/ipc_4ghif.h>
#include "ipc_4ghif_stub.h"

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
//#include <linux/usb_ipc.h>
//#include <linux/ipc_api.h>
//#include <linux/ipc_vr_shortmsg.h>
//#include <linux/hw_mbox.h>
//#include <mach/io.h>
//#include <asm/mach/map.h>
//#include <asm/dma.h>
//#include <linux/prcm-regs.h>

/*==============================================================================
  LOCAL MACROS
==============================================================================*/
#if 0
#define DEBUG(args...) printk(args)
#define ENTER_FUNC() DEBUG("4GHIF: Enter %s\n", __FUNCTION__)
#else
#define DEBUG(args...)
#define ENTER_FUNC()
#endif


/*==============================================================================
  LOCAL FUNCTION PROTOTYPES
==============================================================================*/

/*==============================================================================
  LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==============================================================================*/

/*==============================================================================
  LOCAL CONSTANTS
==============================================================================*/

#define IPC_DMA_4GHIF_UL_ID 1
#define IPC_DMA_4GHIF_DL_ID 2

#define HW_MBOX_U2_ARM 2         //  Should be in /vobs/linuxjava/ti_kernel/ti_kernel/linux-2.6.x/arch/arm/plat-omap/include/mach/irqs.h                               
#define INT_24XX_MAIL_U2_MPU 27  //  but this file does not define U2 for Wrigley (CR IKCHUM75 was raised to track that)

#define MAILBOX_READY_MSG  1
#define MAILBOX_DATA_MSG   2 
#define MAILBOX_ACK_MSG    3


/*==============================================================================
  LOCAL VARIABLES
==============================================================================*/

bool b_ipc_4ghif_open = false;
bool b_sdma_ul_transfer_on_going = false;
bool b_sdma_dl_transfer_on_going = false;
bool b_bp_ready = true;
bool b_ap_ready = false;
bool b_ul_pause = false;
bool b_process_dl_msg_on_going = false;

fp_bp_ready    p_bp_ready=NULL;    //UU: see if better to replace by the structure
fp_receive_dl  p_receive_dl=NULL;
fp_dl_complete p_dl_complete=NULL;
fp_ul_complete p_ul_complete=NULL;
fp_ul_resume   p_ul_resume=NULL;

sdma_packet* p_sdma_packet_ul = NULL;
sdma_packet* p_sdma_packet_dl = NULL;

size_t last_dl_packet_size = 0;
u8     last_dl_packet_type = 0;


// FIFO

u8* fifo_dl_read_ptr=NULL;
u8* fifo_ul_write_ptr=NULL;
u8* fifo_dl_write_ptr=NULL;
u8* fifo_ul_read_ptr=NULL;

fifo_msg fifo_data_dl_msg;  
fifo_msg fifo_data_ul_msg; 


#define DL_SIM_TIMER_INTERVAL       (HZ / 4)
#define RESUME_TIMER_INTERVAL       (HZ / 2)
#define SIM_UL_BUSY_COUNT           20
#define SIM_UL_LOST_INT_COUNT       10
#define SIM_DL_LOST_INT_COUNT       15
#define SIM_DL_MGMT_PACKET_COUNT    50


// timer used for dl simulation
struct timer_list   dl_timer;
int                 dl_timer_enabled = 1;

// timer used for dl simulation, enable this to simulate UL busy condition
struct timer_list   resume_timer;
int                 resume_timer_enabled = 1;

// enable for lost interrupt simulations
static int enable_ul_lost_interrupt = 1;
static int enable_dl_lost_interrupt = 1;

// enable for dl mgmt simulation
static int enable_dl_mgmt_packet = 1;

// index for DL simulation table below
static int curr_dl_sim_packet = 0;


typedef struct
{
    char*   data;
    size_t  size;
    
} t_sim_packets;

static char dl_sim_packet_A[22] =
    {0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    
static char dl_sim_packet_B[28] =
    {0xB, 0xB, 0xB, 0xB, 0xB, 0xB, 0xB, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    
static char dl_sim_packet_C[256] =
    {0};

static char dl_sim_packet_D[512] =
    {0};
    
    
#define MAX_SIM_PACKETS 4
static t_sim_packets dl_sim_packets[MAX_SIM_PACKETS] = 
{
    {dl_sim_packet_A, sizeof(dl_sim_packet_A)},
    {dl_sim_packet_B, sizeof(dl_sim_packet_B)},
    {dl_sim_packet_C, sizeof(dl_sim_packet_C)},
    {dl_sim_packet_D, sizeof(dl_sim_packet_D)}
};

/*==============================================================================
  GLOBAL VARIABLES
==============================================================================*/

/*==============================================================================
  LOCAL FUNCTIONS
==============================================================================*/


/* tasklet used to call user callbacks outside interrupt time */

void dl_complete_tasklet_function(unsigned long param)
{
    int bytes_to_copy = 0;
    static int invoke_count = 0;
    int force_lost_interrupt = 0;
    
    
    ENTER_FUNC();
 
 
    invoke_count++;

    // force a lost DL interrupt if enabled
    force_lost_interrupt = (invoke_count % SIM_DL_LOST_INT_COUNT) == 0 ? 1 : 0;
        
    if (force_lost_interrupt && enable_dl_lost_interrupt)
    {
        printk("4GHIF: DL lost interrupt simulation, will not invoke callback\n");
        return;
    }
     
    /* indicate sDMA transfer is finished */ 
    b_sdma_dl_transfer_on_going=false;   //UU: check location for this flag

    // simulate the SDMA xfer
    bytes_to_copy = min(p_sdma_packet_dl->buf_size, dl_sim_packets[curr_dl_sim_packet].size);
    memcpy(p_sdma_packet_dl->pbuf, dl_sim_packets[curr_dl_sim_packet].data, bytes_to_copy);
    p_sdma_packet_dl->buf_size = bytes_to_copy;
    
    /* data driver callback*/
    if(p_dl_complete!=NULL) p_dl_complete(p_sdma_packet_dl);
    
    // used as a flag to denote DL payload is available
    b_process_dl_msg_on_going = false;
    
}


void ul_complete_tasklet_function(unsigned long param)
{
    static int invoke_count = 0;
    int force_lost_interrupt = 0;
    
    ENTER_FUNC();


    invoke_count++;

    // force a lost UL interrupt if enabled
    force_lost_interrupt = (invoke_count % SIM_UL_LOST_INT_COUNT) == 0 ? 1 : 0;
        
    if (force_lost_interrupt && enable_ul_lost_interrupt)
    {
        printk("4GHIF: UL lost interrupt simulation, will not invoke callback\n");
        return;
    }

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
}

void receive_dl_tasklet_function(unsigned long param)
{
    static int invoke_count = 0;
    
    
    ENTER_FUNC();
     
    // calc index into the simulation data table
    curr_dl_sim_packet = invoke_count % MAX_SIM_PACKETS;
    invoke_count++;
    
    // return the size of the active data packet
    last_dl_packet_size = dl_sim_packets[curr_dl_sim_packet].size;
    
    // every so often, tag as a management packet, the data driver will drop it
    if (enable_dl_mgmt_packet)
        last_dl_packet_type = ((invoke_count % SIM_DL_MGMT_PACKET_COUNT) == 0) ? PKT_TYPE_DL_MGMT : PKT_TYPE_DL_DATA;
    else
        last_dl_packet_type = PKT_TYPE_DL_DATA;
    
    /* data driver callback*/
    if(p_receive_dl!=NULL) p_receive_dl(last_dl_packet_type, last_dl_packet_size);
}

void ul_resume_tasklet_function(unsigned long param)
{
    ENTER_FUNC();

    /* data driver callback*/
    if(p_ul_resume!=NULL) p_ul_resume();
}


static DECLARE_TASKLET(dl_complete_tasklet, dl_complete_tasklet_function, (unsigned long) NULL);
static DECLARE_TASKLET(ul_complete_tasklet, ul_complete_tasklet_function, (unsigned long) NULL);
static DECLARE_TASKLET(bp_ready_tasklet   , bp_ready_tasklet_function   , (unsigned long) NULL);
static DECLARE_TASKLET(receive_dl_tasklet , receive_dl_tasklet_function , (unsigned long) NULL);
static DECLARE_TASKLET(ul_resume_tasklet  , ul_resume_tasklet_function  , (unsigned long) NULL);


/****/

void dl_simulation_func(unsigned long ptr)
{

    DEBUG("4GHIF: dl_simulation_func watchdog fired;\n");
    
    
    // simulate a DL payload available if there isn't already one
    if (b_process_dl_msg_on_going == false)
    {
        b_process_dl_msg_on_going = true;
        tasklet_schedule(&receive_dl_tasklet);
    }
    
    // reschedule our timer
    dl_timer.expires = jiffies + DL_SIM_TIMER_INTERVAL;
    add_timer(&dl_timer);
    
    
    return;
}


void resume_simulation_func(unsigned long ptr)
{

    DEBUG("4GHIF: resume_simulation_func watchdog fired;\n");
    
    // simply schedule the resume tasklet to schedule   
    tasklet_schedule(&ul_resume_tasklet);
    
    return;
}

/***/

/* ISR function called when an interruption occured on a mailbox*/
irqreturn_t mailbox_ISR_4GHIF(int irq, void *p)
{
        
    ENTER_FUNC();


    return IRQ_HANDLED;
}


void fifo_4ghif_process_dl_msg(void)
{      
    ENTER_FUNC();

    
}


void sdma_4ghif_ul_callback(int lch, u16 ch_status, void *data)
{
    
    ENTER_FUNC();

         
    tasklet_schedule(&ul_complete_tasklet);
}


void sdma_4ghif_dl_callback(int lch, u16 ch_status, void *data)
{
    
    ENTER_FUNC();
    
    
    tasklet_schedule(&dl_complete_tasklet);
}



/* FIFO functions */

IPC_4GHIF_FIFO_STATUS fifo_4ghif_init(void)
{
   
    return IPC_4GHIF_FIFO_STATUS_OK;
}

IPC_4GHIF_FIFO_STATUS fifo_4ghif_get_msg(fifo_msg* msg)
{
   

    return IPC_4GHIF_FIFO_STATUS_OK;
}

IPC_4GHIF_FIFO_STATUS fifo_4ghif_add_msg(fifo_msg* msg)
{
 
    return IPC_4GHIF_FIFO_STATUS_OK;
}


IPC_4GHIF_FIFO_STATUS fifo_4ghif_ul_is_not_full(void)
{
 
    
    return IPC_4GHIF_FIFO_STATUS_OK;
}



//Basic fix size DATA_UL shared memory manager


//256 entry max ! (else change all u8 by u16)
u8 free;
u8 last;
u8 table_next[DATA_UL_MM_NB];

void ipc_4ghif_data_ul_init(void)
{
    int i;
    
    free=0;
    last=DATA_UL_MM_NB-1;
    for(i=0;i<(DATA_UL_MM_NB-1);i++) table_next[i]=i+1;
    table_next[DATA_UL_MM_NB-1]=0; 
}

u32* ipc_4ghif_data_ul_alloc(u16 size)
{
    u32 adr;
    u8 use;
    
    /* some checks */
    if(size>DATA_UL_MM_SIZE) return NULL;  //packet too big (should panic)
    
    if(free==last) return NULL; //DATA_UL full
    
    /* compute the physical address */ 
    adr=(u32)(DATA_UL_MM_BASE_ADR+free*DATA_UL_MM_SIZE);
    
    /* return the adress of the first cell in the linked list */
    use=free;
    free=table_next[free];
    table_next[use]=use;   //point on own cell to mark cell is used
    
     DEBUG("4G HIF: Address 0x%x allocated\n",adr);
    
    return ((u32*)adr);
}

void ipc_4ghif_data_ul_free(u32 adr)
{
    u8 ind;
    
    DEBUG("4G HIF: Free address 0x%x\n",adr);
    
    /* some checks */
    if((adr-DATA_UL_MM_BASE_ADR)%DATA_UL_MM_SIZE!=0) panic("4G HIF ERROR: FREE UL DATA shared memory: address not aligned\n"); //not aligned
    
    ind=(adr-DATA_UL_MM_BASE_ADR)/DATA_UL_MM_SIZE;
    
    if(ind>DATA_UL_MM_NB) panic("4G HIF ERROR: FREE UL DATA shared memory: out of range address\n"); //outside range
    
    if(table_next[ind]!=ind) panic("4G HIF ERROR: FREE UL DATA shared memory: cell already free\n"); //cell not used so nothing to free
    
    /* point next of this cell somwhere else (not important, just to mark cell not used) */
    table_next[ind]=ind^1;

    /* add the freed cell at the end of the linked list */
    table_next[last]=ind; last=ind;
}





/*==============================================================================
  GLOBAL FUNCTIONS
==============================================================================*/

/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\//\/\/\/\/\/\/\/\/\ API functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ 

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
IPC_4GHIF_STATUS IPC_4GHIF_open(ipc_4ghif_callbacks callbacks)
{
    if(b_ipc_4ghif_open!=false) return IPC_4GHIF_STATUS_ALREADY_OPEN;

    /* register callbacks (before IRQ enable in case a msg has been already received on MB2) */
    
    p_bp_ready=callbacks.bp_ready;
    p_receive_dl=callbacks.receive_dl;
    p_dl_complete=callbacks.dl_complete;
    p_ul_complete=callbacks.ul_complete;
    p_ul_resume=callbacks.ul_resume;
    
    if(b_bp_ready==false) { fifo_4ghif_init(); ipc_4ghif_data_ul_init(); }

    /* 4ghif ipc is opened */
    
    b_ipc_4ghif_open = true;

    // init our C and D packets
    memset(dl_sim_packet_C, 0xC, sizeof(dl_sim_packet_C));
    memset(dl_sim_packet_D, 0xD, sizeof(dl_sim_packet_D));
    
    /* set up our DL simulation timer */
    init_timer(&dl_timer);
    dl_timer.function = dl_simulation_func;
    dl_timer.data = (unsigned long) 0;
    
    /* set up our resume simulation timer */
    init_timer(&resume_timer);
    resume_timer.function = resume_simulation_func;
    resume_timer.data = (unsigned long) 0;
    
    /* schedule bp ready callback if bp already signaled it is ready 
       (occured when data driver close and re-open the ipc 4ghif
       else bp ready callback will be scheduled as soon as bp signal it is ready */
    if(b_bp_ready) tasklet_schedule(&bp_ready_tasklet);

    return IPC_4GHIF_STATUS_OK;    
}


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
IPC_4GHIF_STATUS IPC_4GHIF_close(void)
{
    if(b_ipc_4ghif_open==false) return IPC_4GHIF_STATUS_CLOSED;


    // kill our timer if running
    if (dl_timer_enabled)
        del_timer(&dl_timer);
        
    // kill our resume timer if running
    if (resume_timer_enabled)
        del_timer(&resume_timer);
        
    /* 4ghif ipc is closed */

    b_ipc_4ghif_open = false;
    
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
IPC_4GHIF_STATUS IPC_4GHIF_ready(void)
{
      
    if(b_ipc_4ghif_open==false) return IPC_4GHIF_STATUS_CLOSED;

    if(b_bp_ready==false) return IPC_4GHIF_STATUS_BP_NOT_READY;
    
    if(b_ap_ready==false)
    {
   
    }
    
    b_ap_ready=true;
    
    // now start DL traffic if enabled
    if (dl_timer.function && dl_timer_enabled)
    { 
        DEBUG("IPC_4GHIF_ready adding dl_timer\n");
        dl_timer.expires = jiffies + DL_SIM_TIMER_INTERVAL;
        add_timer(&dl_timer);
    }
    
    return IPC_4GHIF_STATUS_OK;
}



/*=====================================================================================================================
Name: IPC_4GHIF_submit_dl
Description:  
-	This function is used to start a frame transfer from shared memory to skbuff using sDMA 
Parameters:
-	sdma_packet* packet: structure containing pointer to the data to transfer. On call entry, buf_size field is initialized.  
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
-       UU: Need to pass physical address in packet->pbuf (Check with Todd if needed to be changed)
-       UU: Caller alloc buffer (Check with Todd if needed to be changed)
=====================================================================================================================*/
IPC_4GHIF_STATUS IPC_4GHIF_submit_dl(sdma_packet* packet)
{
    
    /* check error cases */   
    if(b_ipc_4ghif_open==false) return IPC_4GHIF_STATUS_CLOSED;

    if(b_bp_ready==false) return IPC_4GHIF_STATUS_BP_NOT_READY;
    if(b_ap_ready==false) return IPC_4GHIF_STATUS_AP_NOT_READY;
    
    if(b_sdma_dl_transfer_on_going!=false) return IPC_4GHIF_STATUS_BUSY;

    if(b_process_dl_msg_on_going==false) return IPC_4GHIF_STATUS_NO_FRAME;
    
    if(packet==NULL) return IPC_4GHIF_STATUS_INVALID_PARAM;
    
    
    /* update packet type indication */
    packet->packet_type = last_dl_packet_type;
    
    b_sdma_dl_transfer_on_going = true;
    
    // assign and schedule the tasklet at this time
    p_sdma_packet_dl = packet;
    tasklet_schedule(&dl_complete_tasklet);

    return IPC_4GHIF_STATUS_OK;
}


/*=====================================================================================================================
Name: IPC_4GHIF_submit_ul
Description:  
-	This function is used to start a frame transfer from skbuff to shared memory using sDMA and then to the BP. 
Parameters:
-	sdma_packet* packet: structure containing pointer to the data to transfer.
-	u8 bearerID: bearer for this frame  
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
-	If it is not possible to send the packet because shared memory is full, then function returns the IPC_4GHIF_STATUS_RESOURCES_FULL error. In this case the IPC_4GHIF_ul_resume_callback will be scheduled as soon as space will be available in shared memory.
-	In uplink direction, it is needed to indicate the bearer of the frame to the BP.  
-	NOTE: For current USB LTE, the bearer ID is pre-pended to the payload.  We need to talk about this
=====================================================================================================================*/
IPC_4GHIF_STATUS IPC_4GHIF_submit_ul(sdma_packet* packet, u8 bearerID)
{
    static int invoke_count = 0;
    int force_busy_error = 0;
    
        
    if(b_ipc_4ghif_open==false) return IPC_4GHIF_STATUS_CLOSED;

    if(b_bp_ready==false) return IPC_4GHIF_STATUS_BP_NOT_READY;
    if(b_ap_ready==false) return IPC_4GHIF_STATUS_AP_NOT_READY;
    
    if(b_sdma_ul_transfer_on_going!=false) return IPC_4GHIF_STATUS_BUSY;
    
    if(packet==NULL) return IPC_4GHIF_STATUS_INVALID_PARAM;
    if(packet->pbuf==NULL || packet->buf_size==0 || packet->buf_size>DATA_UL_PACKET_MAX_SIZE) return IPC_4GHIF_STATUS_INVALID_PARAM;
    
    if( (packet->packet_type!=PKT_TYPE_UL_DATA) && (packet->packet_type!=PKT_TYPE_UL_MGMT) ) return IPC_4GHIF_STATUS_INVALID_PARAM;
    
    
    invoke_count++;
    
    // force a busy error every so often if the resume timer is enabled
    force_busy_error = ((invoke_count % SIM_UL_BUSY_COUNT) == 0) ? 1 : 0;
    if (force_busy_error && resume_timer.function && resume_timer_enabled)
    { 
        printk("IPC_4GHIF_submit_ul - forcing ERROR, adding resume_timer - 2 sec tmo\n");
        resume_timer.expires = jiffies + RESUME_TIMER_INTERVAL;
        add_timer(&resume_timer);
        return IPC_4GHIF_STATUS_RESOURCES_FULL;
    }
    else
    {
        // assign and schedule the tasklet at this time
        b_sdma_ul_transfer_on_going = true;
        p_sdma_packet_ul = packet;
        tasklet_schedule(&ul_complete_tasklet);
    }
    
    return IPC_4GHIF_STATUS_OK;
}

IPC_4GHIF_STATUS IPC_4GHIF_cancel_ul(void)
{

    b_sdma_ul_transfer_on_going = false;
    
    return IPC_4GHIF_STATUS_OK;
}

IPC_4GHIF_STATUS IPC_4GHIF_cancel_dl(void)
{

    b_sdma_dl_transfer_on_going = false;
    
    return IPC_4GHIF_STATUS_OK;
}


EXPORT_SYMBOL(IPC_4GHIF_open);
EXPORT_SYMBOL(IPC_4GHIF_close);
EXPORT_SYMBOL(IPC_4GHIF_ready);
EXPORT_SYMBOL(IPC_4GHIF_submit_ul);
EXPORT_SYMBOL(IPC_4GHIF_submit_dl);
EXPORT_SYMBOL(IPC_4GHIF_cancel_ul);
EXPORT_SYMBOL(IPC_4GHIF_cancel_dl);


