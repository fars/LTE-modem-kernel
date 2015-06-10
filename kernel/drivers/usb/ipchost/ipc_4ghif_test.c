/*
 * Copyright (C) 2010-2011 Motorola Mobility, Inc
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
 * 04/30/2010      Motorola        Initial creation
 * 05/11/2010      Motorola        Support 4G HIF phase 2  (group frames)
 * 05/31/2011      Motorola        Increase statistics buffer size
 */
 
/*!
 * @file drivers/usb/ipchost/ipc_4ghif_test.c
 * @brief 4G HIF IPC test Set 
 *
 *  This is the 4G HIF low level driver test set and performance/throughput measurement.
 *  
 *  It is necessary to stub/disable the LTE data driver (for example, modify the data driver so that module_init just returns 0).
 *  
 *  To use the test set:
 *  In the console (or adb shell):
 * 
 * >busybox mknod /dev/lld-hif-test c 100 0
 *     --> opens the char driver application test
 * 
 * >echo "print_off"  >  /dev/lld-hif-test
 *     --> disables debugging printk messages on the console
 * 
 * >echo "send 2  1500 "  >  /dev/lld-hif-test
 *	--> issues a UL transfer: 2 packets of 1500B
 *	
 * >echo "receive"  >  /dev/lld-hif-test
 *	--> issues a DL transfer: empties all the DL FIFO
 *	--> ATTENTION: "send" command must be called first to populate the DL FIFO 
 *	    (UL packets created by "send nb_packets length" are looped back by the BP)
 *	
 * >echo "send_receive 2  1500 "  >  /dev/lld-hif-test
 *	--> issues a UL/DL transfer: 2 packets of 1500B are sent to the BP and looped back to the AP
 *
 * >echo "stats "  >  /dev/lld-hif-test
 *    --> displays some statistics on the transfer
 *
 * > echo "print_on"  >  /dev/lld-hif-test
 *    --> enables debugging printk messages on the console
 *    --> ATTENTION: decreases the performances
 *
 * > echo "report" >  /dev/lld-hif-test
 *   --> displays timing statistics on the sDMA transfers on the console
 * @ingroup IPCFunction
 */



/*
 * Include Files
 */
#include <linux/cdev.h>
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
#include <linux/miscdevice.h>
#include <linux/kthread.h>
#include <linux/circ_buf.h>
#include <linux/uio.h>
#include <linux/poll.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/ipc_4ghif.h>
#include "ipc_4ghif_mm.h"

bool b_print_on=true;

#define IPC_DEBUG

#ifdef IPC_DEBUG
#define DEBUG(args...) if(b_print_on==true) printk(args)
#else
#define DEBUG(args...)
#endif

#define ENTER_FUNC()    DEBUG("Enter: %s\n", __FUNCTION__)

#define IPC_4G_HIF_TEST_MAJOR 100
#define IPC_4G_HIF_TEST_MINOR 0

union ktime tx_kt_start_transfer,tx_kt_end_transfer,tx_kt_delta_transfer,rx_kt_start_transfer,rx_kt_end_transfer,rx_kt_delta_transfer;
u64 dl_received_size, ul_sent_size=0;
u32 throughput;

DECLARE_WAIT_QUEUE_HEAD(ipc_4ghif_submit_ul_queue);

u32 lld_hif_buff_ul[IPC_4GHIF_MAX_NB_UL_FRAMES][2048];
u32 lld_hif_buff_dl[IPC_4GHIF_MAX_NB_DL_FRAMES][2048];

char sz_order[50];

sdma_packet lld_hif_ul_sdma_packet_array[IPC_4GHIF_MAX_NB_UL_FRAMES],lld_hif_dl_sdma_packet_array[IPC_4GHIF_MAX_NB_DL_FRAMES];
sdma_packet* lld_hif_ul_sdma_packet_array_ptr[IPC_4GHIF_MAX_NB_UL_FRAMES];
sdma_packet* lld_hif_dl_sdma_packet_array_ptr[IPC_4GHIF_MAX_NB_DL_FRAMES];
packet_info lld_hif_dl_packet_info_array[IPC_4GHIF_MAX_NB_DL_FRAMES];

bool b_ul_done,b_resource_not_full=0;
u32 nb_tx_transferred,nb_rx_transferred=0;

static dev_t lld_hif_test_maj_min=MKDEV(IPC_4G_HIF_TEST_MAJOR,IPC_4G_HIF_TEST_MINOR);
static struct cdev lld_hif_test_cdev;

bool b_test_integrity=false;
typedef struct _ipc_4ghif_buf_compare_results
{
   u32 NbDiffs;
   int PacketIndex;
} ipc_4ghif_buf_compare_results;

int ipc_4ghif_test_integrity(u8 * adr1,u8 * adr2,u16 length,ipc_4ghif_buf_compare_results * buf_compare_results )
{
    int i;
    
    ENTER_FUNC();
     
    if ((adr1==NULL)||(adr2==NULL)||(length<=0)||(buf_compare_results==NULL)) 
    {
       printk(KERN_ALERT "4GHIF ERROR in %s: invalid pointer or buffer size.\n",__FUNCTION__);   
       return -1;
    }

    /*init*/
    buf_compare_results->NbDiffs = 0;
    buf_compare_results->PacketIndex = -1;
        
    for (i=0;i<length;i++)
    {
       if ((u8)(*adr1) != (u8)(*adr2))
       {
	    buf_compare_results->NbDiffs++;
	    if (buf_compare_results->PacketIndex==-1)  buf_compare_results->PacketIndex=i;
       }
       
      (adr1)++;
      (adr2)++;
    }   
    return 0;
}


void ipc_4ghif_test_usage(void)
{
    printk("ipc_4ghif_test usage:\n");
    printk("echo \"cmd\" > /dev/lld-hif-test\n");
    printk("cmd:      - print_off: disables debugging printk messages on the \n"); 
    printk("console.\n");
    printk("          - print_on: enables debugging printk messages on the \n");
    printk("console. \n");
    printk("Be careful, this command decreases the performances.\n");
    printk("          - test_integrity_on: compare packet payloads.\n");
    printk("          - test_integrity_off: disable integrity test.\n");
    printk("          - stats: displays some statistics on the transfer.\n");
    printk("          - report: displays timing statistics on the sDMA \n");
    printk("transfers on the console.\n");
    printk("          - bp_hif_stub_on: stubs the BP HIF: UL frames are ACKed.\n");
    printk("          - bp_hif_loopback: stubs the BP HIF: UL frames are ACKed\n");
    printk("and looped back to the AP HIF.\n");
    printk("          - bp_hif_stub_off: removes the BP HIF stub.\n");
    printk("and looped back to the AP HIF.\n");
    printk("          - send  nb_packets nb_packets_by_block size: issues UL \n");
    printk("transfers of nb_packets packets of size bytes grouped by \n");
    printk("nb_packets_by_block.\n");
    printk("          - receive: issues a DL transfer: empties all DL FIFO.\n");
    printk("          - send_receive nb_packets nb_packets_by_block size: issues n");
    printk("UL/DL transfers: nb_packets packets of size bytes are sent to \n");
    printk("the BP by groups of nb_packets_by_block and DL FIFO is emptied.\n");
};

	   
/* file operation for 4G HIF test */
static int ipc_4ghif_test_open(struct inode * inode, struct file * file)
{
	ENTER_FUNC();
	return 0;
}

static int ipc_4ghif_test_close(struct inode * inode, struct file * file)
{
	ENTER_FUNC();
	return 0;
}

static ssize_t ipc_4ghif_test_read(struct file * filp, char __user * buf, size_t count, loff_t * l)
{
       ENTER_FUNC();
       return 0;
}

static ssize_t ipc_4ghif_test_write(struct file * filp, const char * buf, size_t count, loff_t * l)
{
        IPC_4GHIF_STATUS status;
	int i,j,nb_packets,packet_size,nb_packets_by_block,nb_blocks,nb_remaining_packets,nb_submitted_packets=0;
	u8 nb_tx_ok,nb_rx_ok=0;
	u8 cmd_buf[100];
	char * sz_stats[1536];
	u8 shift=0,max_shift=3;

	ENTER_FUNC();
	
	copy_from_user(cmd_buf,buf,count);
	cmd_buf[count-1]=0;
	
	sscanf((char*)cmd_buf,"%s",sz_order);
	DEBUG("order: %s\n",sz_order);
	
	if (strcmp(sz_order,"print_on")==0)
	{
	   b_print_on=true;
	}
	else if (strcmp(sz_order,"print_off")==0)
	{
	   b_print_on=false;
	}
	
	else if (strcmp(sz_order,"test_integrity_on")==0)
	{
	     b_test_integrity=true;
	     DEBUG("4G HIF IPC: Integrity test enabled.\n");
	}
	else if (strcmp(sz_order,"test_integrity_off")==0)
	{
	     b_test_integrity=false;
	     DEBUG("4G HIF IPC: Integrity test disabled.\n");
	}

	else if (strcmp(sz_order,"report")==0)
	{
	   IPC_4GHIF_debug_order(1);
	}
	
	else if (strcmp(sz_order,"stats")==0)
	{	    
	   IPC_4GHIF_get_statistics_string(sz_stats, 1536);
	   printk("\n%s\n",sz_stats);
        }

        else if (strcmp(sz_order,"bp_hif_stub_on")==0)
        {
            IPC_4GHIF_debug_order(5);   
        }
        else if (strcmp(sz_order,"bp_hif_stub_off")==0)
        {
            IPC_4GHIF_debug_order(3);
        }
        else if (strcmp(sz_order,"bp_hif_loopback")==0)
        {
            IPC_4GHIF_debug_order(4);
        }

	else if ((strcmp(sz_order,"send")==0)||(strcmp(sz_order,"send_receive")==0))
	{
            /* parse command line*/
	    sscanf((char*)cmd_buf,"%s %d %d %d",sz_order,&nb_packets,&nb_packets_by_block,&packet_size );

	    /* check args*/
	    if ((nb_packets==0)||(nb_packets_by_block==0)||(packet_size==0))
            {
                ipc_4ghif_test_usage();
		return count;
            }

	    DEBUG("Nb packets: %d, Nb packets by block: %d, packet size: %d\n",nb_packets,nb_packets_by_block,packet_size);	    

	    /* check error cases*/
	    if (nb_packets < nb_packets_by_block)
	    {
	    	printk("Invalid param NB_PACKETS=%d is bigger than NB_PACKET_BY_BLOCK=%d\n",nb_packets,nb_packets_by_block);
	    } 
	    if (nb_packets_by_block > IPC_4GHIF_MAX_NB_UL_FRAMES) 
	    {
	        printk("Invalid param NB_PACKET_BY_BLOCK is bigger than IPC_4GHIF_MAX_NB_UL_FRAME=%d\n",IPC_4GHIF_MAX_NB_UL_FRAMES);
		return count;
	    }
	    if (packet_size > DATA_UL_PACKET_MAX_SIZE)
	    {
	    	printk("Invalid param PACKET_SIZE=%d is bigger than DATA_UL_PACKET_MAX_SIZE=%d\n",packet_size,DATA_UL_PACKET_MAX_SIZE);
	    }
	   	    
    	    /*fill the UL buffers*/
	    for(i=0;i<IPC_4GHIF_MAX_NB_UL_FRAMES;i++)
	    {
                for (j=0;j<2048;j++)
		{
		   lld_hif_buff_ul[i][j]=i+1+j;
		}		  	
	    }
	    
	    nb_blocks = nb_packets/nb_packets_by_block;
	    nb_remaining_packets = nb_packets;
	    nb_submitted_packets=nb_packets_by_block;
	    i=1;
	    ul_sent_size=0;
	    shift=0;
	    
	    tx_kt_start_transfer=ktime_get();
	     
	    while (nb_remaining_packets > 0)
	    {
	    	    
	      if (nb_remaining_packets < nb_packets_by_block)  nb_submitted_packets=nb_remaining_packets;
	    
	        //give some packets
	        for(j=0;j<nb_submitted_packets;j++)
	        {
	            lld_hif_buff_ul[j][shift] = lld_hif_buff_ul[j][shift]+1;
		    
		    lld_hif_ul_sdma_packet_array[j].packet_type=PKT_TYPE_UL_DATA;
                    //lld_hif_ul_sdma_packet_array[j].pbuf=(void*)&lld_hif_buff_ul[(j+shift)%IPC_4GHIF_MAX_NB_UL_FRAMES];  //virtual
		    lld_hif_ul_sdma_packet_array[j].pbuf=(void*)&lld_hif_buff_ul[j];  //virtual
                    lld_hif_ul_sdma_packet_array[j].buf_size=packet_size;
                    lld_hif_ul_sdma_packet_array[j].bearerID=1; 
	            lld_hif_ul_sdma_packet_array[j].transferred_status=false;   

		    //fill the pointer array with all packets
		    lld_hif_ul_sdma_packet_array_ptr[j]=&lld_hif_ul_sdma_packet_array[j];
		    
		    ul_sent_size += (u64)packet_size;             
                }
	
	        shift++;
		shift = shift%max_shift; 
		
		b_ul_done=0;
		b_resource_not_full=0;
	    
	        DEBUG("Submit UL nb %d\n",i);
		i++;

		status=IPC_4GHIF_submit_ul(nb_submitted_packets,lld_hif_ul_sdma_packet_array_ptr,&nb_tx_ok);
				
		nb_tx_transferred=nb_tx_ok;

		if (status==IPC_4GHIF_STATUS_RESOURCES_FULL)
		{
		        DEBUG("Resource full\n");
			
			/*if no packet has been transferred: don't wait for UL CB*/
		        if (nb_tx_ok==0) b_ul_done=1;
		 }
		 else if (status!=IPC_4GHIF_STATUS_OK)
		 {
		    printk("IPC_4GHIF_submit_ul returned %d\n",status);
		    tx_kt_end_transfer=ktime_get();
		    return count;		    
		 }
		 else /* IPC_4GHIF_STATUS_OK*/
		 {
		     b_resource_not_full=1;
                 }
			        
		nb_remaining_packets -= nb_tx_ok;			
		wait_event_interruptible(ipc_4ghif_submit_ul_queue,b_ul_done&&b_resource_not_full);
	  }
	  tx_kt_end_transfer=ktime_get();
	  tx_kt_delta_transfer=ktime_sub(tx_kt_end_transfer,tx_kt_start_transfer);
	  
	  /* throughput in Mbps*/
	  if ((u32)tx_kt_delta_transfer.tv.sec>0)
	  {
	      if (strcmp(sz_order,"send_receive")==0)
              {
                  throughput = ((u32)(ul_sent_size>>16))/((u32)tx_kt_delta_transfer.tv.sec);
	      }
	      else
	      {
	           throughput = (u32) ((ul_sent_size>>17))/((u32)tx_kt_delta_transfer.tv.sec);
	      }
	    
	      printk("Sent %d MB in %4d.%09d s at %d Mbps \n",(u32) (ul_sent_size>>20),tx_kt_delta_transfer.tv.sec,tx_kt_delta_transfer.tv.nsec,throughput);
	   }  
	   else  printk("Sent %d MB in %4d.%09d s \n",(u32) (ul_sent_size>>20),tx_kt_delta_transfer.tv.sec,tx_kt_delta_transfer.tv.nsec);
	  
	}
        else if (strcmp(sz_order,"receive")==0)
	{
	    dl_received_size=0;
	    
	    rx_kt_start_transfer=ktime_get();
	    
	    for (i=0;i<IPC_4GHIF_MAX_NB_DL_FRAMES;i++)
            {
  	        lld_hif_dl_sdma_packet_array[i].packet_type=PKT_TYPE_NONE; //will be filled automatically
                lld_hif_dl_sdma_packet_array[i].pbuf=(void*)&(lld_hif_buff_dl[i]);  //virtual
                lld_hif_dl_sdma_packet_array[i].buf_size=8192;                //will be filled automatically   UU before multi was 0 (Need to tell that to Todd)

	        lld_hif_dl_sdma_packet_array_ptr[i]=&lld_hif_dl_sdma_packet_array[i];
	    }

            status = IPC_4GHIF_submit_dl(IPC_4GHIF_MAX_NB_DL_FRAMES,lld_hif_dl_sdma_packet_array_ptr,&nb_rx_ok); //IPC_4GHIF_STATUS 
	    nb_rx_transferred=nb_rx_ok;
           
	    rx_kt_end_transfer=ktime_get();
	    rx_kt_delta_transfer=ktime_sub(rx_kt_end_transfer,rx_kt_start_transfer);

	    DEBUG("I4HT submit_dl returns %d (%d packet are in processing)\n",status,nb_rx_ok);

	    for (i=0;i<nb_rx_transferred;i++)
	    {
	        dl_received_size += (u64) lld_hif_dl_sdma_packet_array_ptr[i]->buf_size;
	    }
	   
	   
	   if (rx_kt_delta_transfer.tv.sec>0)
	   {	    
	       /* throughput in Mbps*/
               throughput = (u32) ((dl_received_size>>17))/((u32)rx_kt_delta_transfer.tv.sec);
	    
	       printk("Received %d MB in %4d.%09d s at %d Mbps \n",(u32)(dl_received_size>>20),rx_kt_delta_transfer.tv.sec,rx_kt_delta_transfer.tv.nsec,throughput); 
	   }
	   else printk("Received %d MB in %4d.%09d s \n",(u32)(dl_received_size>>20),rx_kt_delta_transfer.tv.sec,rx_kt_delta_transfer.tv.nsec); 
	   
        }
	
	else
	{
	   /* Usage*/
	   ipc_4ghif_test_usage();           
	}
	
		
	return count;
}

static int ipc_4ghif_test_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	ENTER_FUNC();
	return 0;
}

static struct file_operations ipc_4ghif_test_fops= {
        owner:          THIS_MODULE,
        open:         ipc_4ghif_test_open,
        release:      ipc_4ghif_test_close,
        read:         ipc_4ghif_test_read,
        ioctl:        ipc_4ghif_test_ioctl,
        write:        ipc_4ghif_test_write,
};

/* callbacks*/
static void ipc_4ghif_test_bp_ready_cb(void)
{           
     ENTER_FUNC();
     IPC_4GHIF_ready();    
}

 static void ipc_4ghif_test_receive_dl_cb(u8 packet_nb, packet_info frames_info[])
{
     int i,nb_ok=0; 
     IPC_4GHIF_STATUS status;
     
     ENTER_FUNC();
     
     DEBUG("I4HT receive_dl_callback: received %d packets (first packet type=%d size=%d)\n",packet_nb,frames_info[0].packet_type,frames_info[0].buf_size);
    
     for (i=0;i<packet_nb;i++) /*packet_nb < IPC_4GHIF_MAX_NB_DL_FRAMES*/
     {

	lld_hif_dl_sdma_packet_array[i].packet_type=frames_info[i].packet_type;      //will be filled automatically
        lld_hif_dl_sdma_packet_array[i].pbuf=(void*)&(lld_hif_buff_dl[i]);  //virtual
        lld_hif_dl_sdma_packet_array[i].buf_size=8192;                //will be filled automatically   UU before multi was 0 (Need to tell that to Todd)

	lld_hif_dl_sdma_packet_array_ptr[i]=&lld_hif_dl_sdma_packet_array[i];
     }	    
	 
      
     if ((strcmp(sz_order,"send_receive")==0)||(strcmp(sz_order,"receive")==0))
     { 
        status = IPC_4GHIF_submit_dl(packet_nb,lld_hif_dl_sdma_packet_array_ptr,&nb_ok); //IPC_4GHIF_STATUS 
	DEBUG("I4HT submit_dl returns %d (%d packet are in processing)\n",status,nb_ok);
	
        if (status!=IPC_4GHIF_STATUS_OK)
	{ 
	     printk("IPC_4GHIF_submit_dl returned %d\n",status);
	     return;
	}
	    
	nb_rx_transferred=nb_ok;

	for (i=0;i<nb_rx_transferred;i++)
	{
	     dl_received_size += (u64) lld_hif_dl_sdma_packet_array_ptr[i]->buf_size;
	}

     }
     
}

static void ipc_4ghif_test_dl_complete_cb(sdma_packet* packet[])
{  
    /*for integrity tests*/
/*    ipc_4ghif_buf_compare_results integrity_dl_res;
    int index,delta_index_dl,ipc_4ghif_test_fifo_dl_read_ind=0;
    volatile u8 * dl_fifo_msg=0;
    u16 dl_packet_length=0;
    void * dl_shared_mem_start_address=0;
    u8 * dl_shared_mem_start_address_virt=0;*/

    ENTER_FUNC();

/*    if (b_test_integrity==true) 
    {
        memset(&integrity_dl_res,0,sizeof(ipc_4ghif_buf_compare_results));
	ipc_4ghif_test_fifo_dl_read_ind = *(volatile u8*)(FIFO_DL_READ_IND);

	for (index=0;index< nb_rx_transferred;index++)
	{
            delta_index_dl = ipc_4ghif_test_fifo_dl_read_ind-1-index;

	    if (delta_index_dl<0) dl_fifo_msg = (volatile u8*)(FIFO_DL_BASE+(FIFO_DL_MSG_NB+delta_index_dl)*sizeof(fifo_msg));
	    else dl_fifo_msg = (volatile u8*)(FIFO_DL_BASE+(delta_index_dl*sizeof(fifo_msg)));

            dl_packet_length=(u16)((dl_fifo_msg[3]<<8)+(dl_fifo_msg[2]));
            dl_shared_mem_start_address =(void *)(dl_fifo_msg[7]<<24)+(dl_fifo_msg[6]<<16)+(dl_fifo_msg[5]<<8)+(dl_fifo_msg[4]);
            dl_shared_mem_start_address_virt = (u8 *)(((u32) (dl_shared_mem_start_address)&(0x000FFFFF))|(IPC_SHARED_VIRT_ADDR&(0xFFF00000)));
*/
	    /* check size*/
/*            if (dl_packet_length != packet[nb_rx_transferred-index-1]->buf_size) printk(KERN_ALERT "4GHIF IPC ERROR in %s: expected size %d but current packet size is %d.\n",__FUNCTION__,dl_packet_length,packet[nb_rx_transferred-index-1]->buf_size);  
*/
	    /* check integrity*/
/*           if (!ipc_4ghif_test_integrity((u8*)packet[nb_rx_transferred-index-1]->pbuf,dl_shared_mem_start_address_virt,dl_packet_length,&integrity_dl_res))
	    {
	        if (integrity_dl_res.NbDiffs != 0)
	        {
	            printk(KERN_INFO "4G HIF IPC test: compared 0x%x and 0x%x of size %d\n",packet[nb_rx_transferred-index-1]->pbuf,dl_shared_mem_start_address_virt,dl_packet_length); 
	            printk(KERN_INFO "Diff number= %d , first diff index=%d \n",integrity_dl_res.NbDiffs,integrity_dl_res.PacketIndex);
	        }   
	    }
        }
    }
 */   
}

static void ipc_4ghif_test_ul_complete_cb(sdma_packet* packet[])
{
    /*for integrity tests*/
    ipc_4ghif_buf_compare_results integrity_ul_res;
    int index,delta_index_ul,ipc_4ghif_test_fifo_ul_write_ind=0;
    volatile u8 * ul_fifo_msg=0;
    u16 ul_packet_length=0;
    void * ul_shared_mem_start_address=0;
    u8 * ul_shared_mem_start_address_virt=0;

    ENTER_FUNC();

    if (b_test_integrity==true) 
    {
          memset(&integrity_ul_res,0,sizeof(ipc_4ghif_buf_compare_results));
	  ipc_4ghif_test_fifo_ul_write_ind = *(volatile u8*)(FIFO_UL_WRITE_IND);
	
	  for (index=0;index< nb_tx_transferred;index++)
	  {
               delta_index_ul = ipc_4ghif_test_fifo_ul_write_ind-1-index;
	   
	       if (delta_index_ul<0) ul_fifo_msg = (volatile u8*)(FIFO_UL_BASE+(FIFO_UL_MSG_NB+delta_index_ul)*sizeof(fifo_msg));
	       else ul_fifo_msg = (volatile u8*)(FIFO_UL_BASE+(delta_index_ul*sizeof(fifo_msg)));
	   
               ul_packet_length=(u16)((ul_fifo_msg[3]<<8)+(ul_fifo_msg[2]));
               ul_shared_mem_start_address =(void *)(ul_fifo_msg[7]<<24)+(ul_fifo_msg[6]<<16)+(ul_fifo_msg[5]<<8)+(ul_fifo_msg[4]);
               ul_shared_mem_start_address_virt = (u8 *)(((u32) (ul_shared_mem_start_address)&(0x000FFFFF))|(IPC_SHARED_VIRT_ADDR&(0xFFF00000)));

	       /* check size*/
               if (ul_packet_length != packet[nb_tx_transferred-index-1]->buf_size) printk(KERN_ALERT "4GHIF IPC ERROR in %s: expected size %d but current packet size is %d.\n",__FUNCTION__,ul_packet_length,packet[nb_tx_transferred-index-1]->buf_size);  

	       /* check integrity*/
	       if (!ipc_4ghif_test_integrity((u8*)packet[nb_tx_transferred-index-1]->pbuf,ul_shared_mem_start_address_virt,ul_packet_length,&integrity_ul_res))
	       {
	            if (integrity_ul_res.NbDiffs != 0)
	            {
	                  printk(KERN_INFO "4G HIF IPC test: compared 0x%x and 0x%x of size %d\n",packet[nb_tx_transferred-index-1]->pbuf,ul_shared_mem_start_address_virt,ul_packet_length); 
	                  printk(KERN_INFO "Diff number= %d , first diff index=%d \n",integrity_ul_res.NbDiffs,integrity_ul_res.PacketIndex);
	            }   
	      }
	  }
    }

     b_ul_done=1;
    wake_up(&ipc_4ghif_submit_ul_queue);
}

static void ipc_4ghif_test_ul_resume_cb(void)
{
     ENTER_FUNC();
     b_resource_not_full=1;
     wake_up(&ipc_4ghif_submit_ul_queue);   
}
    

ipc_4ghif_callbacks ipc_4ghif_test_callbacks =  
{
            ipc_4ghif_test_bp_ready_cb,
            ipc_4ghif_test_receive_dl_cb,
            ipc_4ghif_test_dl_complete_cb,
            ipc_4ghif_test_ul_complete_cb,
            ipc_4ghif_test_ul_resume_cb
};

/*
 * driver module init/exit functions
 */
static int __init ipc_4ghif_test_init(void)
{
        int err=ENODEV;
	IPC_4GHIF_STATUS ipc_status;

	ENTER_FUNC();
	
	if (register_chrdev_region(lld_hif_test_maj_min,1,"lld-hif-test"))
	{
	  err= -ENODEV;
	  goto err_majmin;
	}
	
	cdev_init(&lld_hif_test_cdev,&ipc_4ghif_test_fops);
	
	if (cdev_add(&lld_hif_test_cdev,lld_hif_test_maj_min,1))
	{
	  err= -ENODEV;
	  goto err_devadd;
	}
	
	
    
        ipc_status = IPC_4GHIF_open(ipc_4ghif_test_callbacks);
        if (ipc_status == IPC_4GHIF_STATUS_OK) return 0;
	else  
	{
	    printk(KERN_ALERT "Error: IPC_4GHIF_open returned %d.\n",ipc_status);
	    err = (int) ipc_status;
	}

       err_devadd:
            printk(KERN_ALERT "Error: delete lld_hif_test_cdev.\n");
            cdev_del(&lld_hif_test_cdev);
       
       err_majmin:
             printk(KERN_ALERT "Error: unregister lld_hif_test char dev region.\n");
             unregister_chrdev_region(lld_hif_test_maj_min,1);
             return err; 
}

static void __exit ipc_4ghif_test_exit(void)
{
	ENTER_FUNC();
	IPC_4GHIF_close();
	cdev_del(&lld_hif_test_cdev);
	unregister_chrdev_region(lld_hif_test_maj_min,1);
}

/* the module entry declaration of this driver */

late_initcall(ipc_4ghif_test_init);
module_exit(ipc_4ghif_test_exit);

/* Module */
MODULE_DESCRIPTION("4G HIF IPC Test Module");
MODULE_AUTHOR("Motorola");
MODULE_LICENSE("GPL");

