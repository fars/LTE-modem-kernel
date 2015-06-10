/*
 * Copyright (C) 2010 Motorola, Inc
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
 * 15-jan-2010       Motorola         file creation
 * 04-feb-2010       Motorola         Add separate FIFOs for ACKs (also known as phase3) 
 * 25-jun-2010       Motorola         4G HIF IPC low level driver on AP side - phase 2 (group frames) 
 */
 
#ifndef __IPC_4GHIF_MM_H__
#define __IPC_4GHIF_MM_H__


#include <linux/interrupt.h>


/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\//\/\/\/\/\/\/\/\/\ structures / types / enums / constants /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/ 

/* memory manager selection */
#define IPC_4GHIF_DATA_UL_MM_BASIC 0
#define IPC_4GHIF_DATA_UL_MM_BUDDY 1
#define IPC_4GHIF_DATA_UL_MM_TYPE IPC_4GHIF_DATA_UL_MM_BUDDY


/* Shared memory settings */
#define IPC_SHARED_PHYS_ADDR  0x81080000
#define IPC_SHARED_VIRT_ADDR  0xD8480000

/* FIFO constants*/
#define FIFO_DL_MSG_NB        256     
#define FIFO_UL_MSG_NB        256
#define FIFO_UL_SIZE          (8 * FIFO_UL_MSG_NB)   //8=sizeof(fifo_msg)
#define FIFO_DL_SIZE          (8 * FIFO_DL_MSG_NB)    
#define FIFO_UL_READ_IND      (IPC_SHARED_VIRT_ADDR+0x7f800)
#define FIFO_UL_WRITE_IND     (IPC_SHARED_VIRT_ADDR+0x7f804)
#define FIFO_DL_READ_IND      (IPC_SHARED_VIRT_ADDR+0x7f808)
#define FIFO_DL_WRITE_IND     (IPC_SHARED_VIRT_ADDR+0x7f80c)
#define FIFO_UL_BASE          (IPC_SHARED_VIRT_ADDR+0x7e000)
#define FIFO_DL_BASE          (IPC_SHARED_VIRT_ADDR+0x7e800)

/* FIFO ACK constants*/
#define FIFO_ACK_DL_MSG_NB    256                        
#define FIFO_ACK_UL_MSG_NB    256
#define FIFO_ACK_UL_SIZE      (4 * FIFO_ACK_UL_MSG_NB) 
#define FIFO_ACK_DL_SIZE      (4 * FIFO_ACK_DL_MSG_NB)    
#define FIFO_ACK_UL_READ_IND  (IPC_SHARED_VIRT_ADDR+0x7f810)
#define FIFO_ACK_UL_WRITE_IND (IPC_SHARED_VIRT_ADDR+0x7f814)
#define FIFO_ACK_DL_READ_IND  (IPC_SHARED_VIRT_ADDR+0x7f818)
#define FIFO_ACK_DL_WRITE_IND (IPC_SHARED_VIRT_ADDR+0x7f81c)
#define FIFO_ACK_UL_BASE      (IPC_SHARED_VIRT_ADDR+0x7f000)
#define FIFO_ACK_DL_BASE      (IPC_SHARED_VIRT_ADDR+0x7f400)

/* sDMA descriptor address in shared memory*/
#define DMA_DESCR_DL_ARRAY_NON_CACHED  (IPC_SHARED_VIRT_ADDR+0x7fa00)
#define DMA_DESCR_UL_ARRAY_NON_CACHED  (IPC_SHARED_VIRT_ADDR+0x7fc00)

/* Data constants*/
#define DATA_DL_SIZE (120 * 1024)
#define DATA_UL_SIZE (384 * 1024)
#define DATA_DL_BASE (IPC_SHARED_VIRT_ADDR+DATA_UL_SIZE)
#define DATA_UL_BASE IPC_SHARED_VIRT_ADDR

/* For buddy memory manager (512B / 2kB / 8kB cells)*/    
#define MM_MEM_BASE_ADR       (DATA_UL_BASE+IPC_SHARED_PHYS_ADDR-IPC_SHARED_VIRT_ADDR)  // 0x810800000
#define MM_MEM_SIZE           (384*1024)                         // 384KB
#define MM_PAGE_SIZE          8192                               // 8KB
#define MM_SECTOR_SIZE        512                                // 0.5KB
#define MM_NB_SECTOR_BY_PAGE  (MM_PAGE_SIZE/MM_SECTOR_SIZE)      // 16
#define MM_NB_PAGE            (MM_MEM_SIZE/MM_PAGE_SIZE)         // 48
#define MM_NB_SECTOR          (MM_NB_PAGE*MM_NB_SECTOR_BY_PAGE)  // 768

/* For basic memory manager (47 * 8kB cells)*/    
#define DATA_UL_MM_BASE_ADR   (DATA_UL_BASE+IPC_SHARED_PHYS_ADDR-IPC_SHARED_VIRT_ADDR)  // 0x810800000
#define DATA_UL_MM_SIZE       8192                               //8192 (8K) esier to debug than 8188 for DATA_UL_PACKET_MAX_SIZE
#define DATA_UL_MM_NB         (DATA_UL_SIZE/DATA_UL_MM_SIZE)     //48 (can be reduced to test ressources full)   

/* Macro to convert shared memory virtual address into physical address*/
#define SHARED_MEM_VIRT_TO_PHYS(adr)  (u32)((((u32) adr)&(0x000FFFFF))|(IPC_SHARED_PHYS_ADDR&(0xFFF00000)))
/* Macro to convert shared memory physical address into virtual address*/
#define SHARED_MEM_PHYS_TO_VIRT(adr)  (u32)((((u32) adr)&(0x000FFFFF))|(IPC_SHARED_VIRT_ADDR&(0xFFF00000)))

/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
Function	                    Description

IPC_4GHIF_ul_mm_init                To initialize the memory manager
IPC_4GHIF_ul_mm_alloc               To alloc a memory block
IPC_4GHIF_ul_mm_free                To free a memory block
*/

/*=====================================================================================================================
Name: IPC_4GHIF_ul_mm_init
Description:  
-	This function initializes the 4G HIF IPC ul memory manager
Parameters:
-	None
Return value: 
-	None
=====================================================================================================================*/
void IPC_4GHIF_data_ul_mm_init(void);


/*=====================================================================================================================
Name: IPC_4GHIF_ul_mm_alloc
Description:  
-	This function allocates a memory block
Parameters:
-	u16 size: requested size (8KB max)
Return value: 
-       NULL: cannot allocate (no space for the requested block, ressources full)
-	u32: physicall address of the allocated block
Notes:
-       This function panics if requested size is bigger than the max supported size (8KB)
=====================================================================================================================*/
u32* IPC_4GHIF_data_ul_mm_alloc(u16 size);


/*=====================================================================================================================
Name: IPC_4GHIF_ul_mm_free
Description:  
-	This function frees a memory block
Parameters:
-	u32 adr: block address
Return value: 
-       None
Notes:
-       This function panics if:
        - block address is outside memory range
        - block address is not aligned on a sector
        - block address is a not used sector
=====================================================================================================================*/
void IPC_4GHIF_data_ul_mm_free(u32 adr); 


/*=====================================================================================================================
Name: IPC_4GHIF_data_ul_mm_get_stat_string
Description:  
-	This function fills a string with memory manager statistics
Parameters:
-	char* sz   : stat string
-       u16 sz_size: stat string max size
Return value: 
-       None
Notes:
-       It is recommended to provide a 512bytes string
=====================================================================================================================*/
void IPC_4GHIF_data_ul_mm_get_stat_string(char* sz, u16 sz_size);

/*=====================================================================================================================
Name: IPC_4GHIF_data_ul_display_last_allocated_size
Description:  
-	This function displays some stats about the memory manager (last allocated sizes).
Parameters:
-	None
Return value: 
-       None
=====================================================================================================================*/
void IPC_4GHIF_data_ul_display_last_allocated_size(void);


#endif // __IPC_4GHIF_MM_H__
