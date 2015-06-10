#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/ipc_api.h>
#include <linux/hw_mbox.h>
#include <mach/io.h>
#include <asm/mach/map.h>
#include <mach/dma.h>
#include <asm/dma.h>
#include <linux/dma-mapping.h>   
#include <linux/prcm-regs.h>
#include <asm/cacheflush.h>
#include <linux/ipc_4ghif.h>
#include "ipc_4ghif_mm.h"




u32 ack_array[16];
int ack_nb=0;
static spinlock_t ipc_4ghif_mb_lock      = SPIN_LOCK_UNLOCKED;


/*
    simulate_loopback_bp: stub the bp in loopback momde
    - When receiving a UL-FRAME
        - remove DATA-UL frame from FIFO
	- add it in DATA-DL FIFO (for loopback)
	- DO NOT ACK the UL-Frame, because it is needed to keep it for the DL loopback until receiving ACK-DL
    - When receiving ACK-DL
        - remove ACK-DL from FIFO
        - add ACK-UL in FIFO
	- When 16 ACK-UL ready, send IT to AP

    To use this stub in 4GHIF code, replace the 2 mailboxes msg to the BP by this stub
        sdma_4ghif_ul_callback --> simulate_loopback_bp or better simulate_loopback_bp_frames
	sdma_4ghif_dl_callback --> simulate_loopback_bp or beeter simulate_loopback_bp_acks
*/
// DATA-UL -> DATA-DL
// ACK-DL -> ACK-UL (IT MB2 when 16 Acks)
void simulate_loopback_bp_frames(void)
{
    u32 ind_ul_bp_read;
    u32 ind_ul_ap_write;   
    u32 ind_dl_bp_write;
    u32 ind_dl_ap_read;
    volatile u8* ptr_ul_bp_read;
    volatile u8* ptr_dl_bp_write;
    int i;
    unsigned long flags;

    //1. Loopback DATA frame DATA-UL -> DATA-DL
    while(1)
    {
        ind_ul_bp_read =*(volatile u8*)(FIFO_UL_READ_IND );
        ind_ul_ap_write=*(volatile u8*)(FIFO_UL_WRITE_IND);

        ind_dl_bp_write=*(volatile u8*)(FIFO_DL_WRITE_IND);
        ind_dl_ap_read =*(volatile u8*)(FIFO_DL_READ_IND );
 
        if(ind_ul_bp_read==ind_ul_ap_write) break; //FIFO UL DATA empty
	
	if( ((ind_dl_bp_write+1)&255) == ind_dl_ap_read) panic("simulate_loopback_bp: DATA-DL full"); //can also be changed to flush the frame (in this case, it is needed to ACK-UL it)

        ptr_ul_bp_read =(u8*) (FIFO_UL_BASE + 8*ind_ul_bp_read);
	ptr_dl_bp_write=(u8*) (FIFO_DL_BASE + 8*ind_dl_bp_write);

        if(ptr_ul_bp_read[0]==eIPC_MSG_TYPE_UL_DATA || ptr_ul_bp_read[0]==eIPC_MSG_TYPE_UL_MGMT) //only loopback DAT frames
        {
	    if(ptr_ul_bp_read[0]==eIPC_MSG_TYPE_UL_DATA) ptr_dl_bp_write[0]=eIPC_MSG_TYPE_DL_DATA;
	    if(ptr_ul_bp_read[0]==eIPC_MSG_TYPE_UL_MGMT) ptr_dl_bp_write[0]=eIPC_MSG_TYPE_DL_MGMT;
	    for(i=1;i<8;i++) ptr_dl_bp_write[i]=ptr_ul_bp_read[i];
	
	    // ADD DATA-DL
            ind_dl_bp_write=(ind_dl_bp_write+1)&255;
            *(volatile u8*)(FIFO_DL_WRITE_IND)=ind_dl_bp_write;
	}
	
	// REMOVE DATA-UL
        ind_ul_bp_read=(ind_ul_bp_read+1)&255;
        *(volatile u8*)(FIFO_UL_READ_IND)=ind_ul_bp_read;
    }
    
    /* notify the AP that DL frames are available*/
    spin_lock_irqsave(&ipc_4ghif_mb_lock,flags);
    HW_MBOX_MsgWrite((const u32)IO_ADDRESS(PRCM_MBOXES_BASE), HW_MBOX_ID_2, 0);
    spin_unlock_irqrestore(&ipc_4ghif_mb_lock,flags);
	
}
        
void simulate_loopback_bp_acks(void)
{
    u32 ind_ul_ack_bp_write;
    u32 ind_ul_ack_ap_read;   
    u32 ind_dl_ack_ap_write;
    u32 ind_dl_ack_bp_read;   
    volatile u8* ptr_ul_ack_write;
    volatile u8* ptr_dl_ack_read;   
    int i;
    unsigned long flags;

    //2. Loopback ACK ACK-DL -> ACK-UL and send IT to AP when having 16 ACK-UL
    while(1)
    {
        ind_dl_ack_ap_write=*(volatile u8*)(FIFO_ACK_UL_WRITE_IND);
        ind_dl_ack_bp_read =*(volatile u8*)(FIFO_ACK_UL_READ_IND );

        ind_ul_ack_ap_read =*(volatile u8*)(FIFO_ACK_DL_READ_IND );
        ind_ul_ack_bp_write=*(volatile u8*)(FIFO_ACK_DL_WRITE_IND);

        if(ind_dl_ack_ap_write==ind_dl_ack_bp_read) break; //FIFO DL ACK empty
	
        ptr_dl_ack_read  = (u8*) (FIFO_ACK_UL_BASE + 4 * ind_dl_ack_bp_read );
        ptr_ul_ack_write = (u8*) (FIFO_ACK_DL_BASE + 4 * ind_ul_ack_bp_write);
	
	for(i=0;i<4;i++) ptr_ul_ack_write[i]=ptr_dl_ack_read[i];
	ack_nb++;
	
	// REMOVE ACK-DL
	ind_dl_ack_bp_read=(ind_dl_ack_bp_read+1)&255;
        *(volatile u8*)(FIFO_ACK_UL_READ_IND)=ind_dl_ack_bp_read;	
		    
	// ADD ACK-UL
        ind_ul_ack_bp_write=(ind_ul_ack_bp_write+1)&255;
        if(ind_ul_ack_ap_read==ind_ul_ack_bp_write) panic("simulate_ul_bp: ACK-UL full");
        *(volatile u8*)(FIFO_ACK_DL_WRITE_IND)=ind_ul_ack_bp_write;	
	
	if(ack_nb==16)
	{
	    ack_nb=0;
	    
           // Notify AP new ACK-UL 

           spin_lock_irqsave(&ipc_4ghif_mb_lock,flags);
           HW_MBOX_MsgWrite((const u32)IO_ADDRESS(PRCM_MBOXES_BASE), HW_MBOX_ID_2, 0);
           spin_unlock_irqrestore(&ipc_4ghif_mb_lock,flags);
	}
    }
    
}

void simulate_loopback_bp(void)
{
    simulate_loopback_bp_frames();
    simulate_loopback_bp_acks();
}


/*
    simulate_ul_bp: stub the bp in UL direction
    - remove DATA-UL frames from FIFO
    - add acks in ACK-UL
    - send IT to AP by MB 2 each 16 acks

    To use this stub in 4GHIF code, replace the mailboxes msg to the BP by this stub
        sdma_4ghif_ul_callback --> simulate_loopback_bp or better simulate_loopback_bp_frames
*/
// DATA-UL -> ACK-UL (IT MB2 when 16 Acks) 
void simulate_ul_bp(void)
{
    u32 ind_ul_bp_read;
    u32 ind_ul_ap_write;
    u32 ind_ul_ack_bp_write;
    u32 ind_ul_ack_ap_read;
    volatile u8* ptr_ul_bp_read;
    volatile u8* ptr_ul_ack_write;
    u32 ack_adr;
    int i;
    unsigned long flags;

    while(1)
    {
        ind_ul_bp_read=*(volatile u8*)(FIFO_UL_READ_IND);
        ind_ul_ap_write=*(volatile u8*)(FIFO_UL_WRITE_IND);
 
        if(ind_ul_bp_read==ind_ul_ap_write) break; //FIFO UL DATA empty

        //extract ack 
        ptr_ul_bp_read= (u8*) (FIFO_UL_BASE + 8*ind_ul_bp_read);

        if(ptr_ul_bp_read[0]==eIPC_MSG_TYPE_UL_DATA || ptr_ul_bp_read[0]==eIPC_MSG_TYPE_UL_MGMT) //send only ACKs for DTAT messages
        {
            //ack_array[ack_nb++]=(void*)(ptr_ul_bp_read[7]<<24)+(ptr_ul_bp_read[6]<<16)+(ptr_ul_bp_read[5]<<8)+(ptr_ul_bp_read[4]);
            ack_array[ack_nb++]=(u32) (ptr_ul_bp_read[7]<<24)+(ptr_ul_bp_read[6]<<16)+(ptr_ul_bp_read[5]<<8)+(ptr_ul_bp_read[4]);
	    if(ack_nb==16) //send acks by 16
            {
                ack_nb=0;

                // Fill ACK-UL FIFO
 
                for(i=0;i<16;i++)
                {
                    ind_ul_ack_ap_read=*(volatile u8*)(FIFO_ACK_DL_READ_IND); //to check later if ack-ul full
                    ind_ul_ack_bp_write=*(volatile u8*)(FIFO_ACK_DL_WRITE_IND);

                    ptr_ul_ack_write = (u8*) (FIFO_ACK_DL_BASE + 4 * ind_ul_ack_bp_write);

                    ack_adr=ack_array[i];

                    ptr_ul_ack_write[3]=(u8)( ((u32)(ack_adr))>>24);
                    ptr_ul_ack_write[2]=(u8)( ((u32)(ack_adr))>>16);
                    ptr_ul_ack_write[1]=(u8)( ((u32)(ack_adr))>>8);
                    ptr_ul_ack_write[0]=(u8)(  (u32) ack_adr);

                    ind_ul_ack_bp_write=(ind_ul_ack_bp_write+1)&255;
                    if(ind_ul_ack_ap_read==ind_ul_ack_bp_write) panic("simulate_ul_bp: ACK-UL full");
                    *(volatile u8*)(FIFO_ACK_DL_WRITE_IND)=ind_ul_ack_bp_write;
                }

                // Notify AP new ACK-UL 

                spin_lock_irqsave(&ipc_4ghif_mb_lock,flags);
                HW_MBOX_MsgWrite((const u32)IO_ADDRESS(PRCM_MBOXES_BASE), HW_MBOX_ID_2, 0);
                spin_unlock_irqrestore(&ipc_4ghif_mb_lock,flags);
            }
        }

        // update DATA-UL fifo (Extract frames from DATA-UL)

        ind_ul_bp_read=(ind_ul_bp_read+1)&255;
        *(volatile u8*)(FIFO_UL_READ_IND)=ind_ul_bp_read;
    }
}


/*
    Set BP RD ptr equal to AP WR ptr before starting the loopback stub
*/
void simulate_bp_init(void)
{
    u32 data;
    
    printk("\nSet 4GHIF BP read pointers to AP write pointers\n");
    //*(volatile u8*)(FIFO_ACK_UL_READ_IND)=*(volatile u8*)(FIFO_ACK_UL_WRITE_IND);    // ind_dl_ack_bp_read=ind_dl_ack_ap_write
    //*(volatile u8*)(FIFO_UL_READ_IND)=*(volatile u8*)(FIFO_UL_WRITE_IND);            // ind_ul_bp_read=ind_ul_ap_write    

    data=*(volatile u8*)(FIFO_ACK_UL_WRITE_IND);    // ind_dl_ack_bp_read=ind_dl_ack_ap_write
    *(volatile u8*)(FIFO_ACK_UL_READ_IND)=data,
    
    data=*(volatile u8*)(FIFO_UL_WRITE_IND);        // ind_ul_bp_read=ind_ul_ap_write
    *(volatile u8*)(FIFO_UL_READ_IND)=data;                
}
