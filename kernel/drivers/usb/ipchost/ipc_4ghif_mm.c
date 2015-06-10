/*
 * Copyright (c) 2010 Motorola, Inc, All Rights Reserved.
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
 * 15-jan-2010       Motorola         initial version
 * 23-mar-2010       Motorola         correct the 512 bytes blocks freeing
 * 11-may-2010       Motorola         correct the 8KB bytes blocks freeing
 * 25-jun-2010       Motorola         4G HIF IPC low level driver on AP side - phase 2 (group frames) 
 */

 
/*!
 * @file drivers/usb/ipchost/ipc_4ghif_mm.c
 * @brief 4G HIF low level driver Descriptor Set
 *
 * Implementation of 4G HIF low level driver UL memory manager.
 *
 * @ingroup IPCFunction
 */


/*  This file contains 2 memory managers
    - the basic one with 47 8KB cells
    - the buddy memory manager with 512B /2KB /8KB cells


    
    --> Basic fixe size DATA_UL shared memory manager 
       
      This memory manager can allocate and free only 8KB blocks 
    
      It can allocate up to 47 cells 
      Note one cell is lost to detect memory full   

      This basic memory manager uses following constants:
      - DATA_UL_MM_BASE_ADR
      - DATA_UL_MM_NB      
      - DATA_UL_MM_SIZE    
    
    

    --> Buddy type memory manager for AP IPC 4GHIF
    
      This memory manager can allocate and free 8KB / 2KB / 512B blocks 

      It was coded for simplicity and speed.

      Bit field usage can seem slow compared to linked list.
      But with only 48 pages the speed is comparable and complexity is lower
      The design is optimized for the 48 pages case and speed. 
      (so it is not generic and some values are hardcoded). 

      Memory is divided in 48 pages of 8K.
      Each page contains 16 sectors of 512 bytes.
      Memory managers can allocate blocks of:
      - 512B (min size equal to the sector size) 
      - 2KB 
      - 8KB  (max size equal to the page size)
      One page stores only blocks of the same size
    
      So pages can be
      - empty (in the pool)
      - partially filled with blocks of the same size
      - totally filled with blocks of the same size

      6 bit fields indicates the pages type
      - pool        (free page not used)
      - full 8KB    (used by 1x 8KB block)
      - partial 2KB (used by 1x to 3x 2KB blocks)
      - full 2KB    (used by 4x 2KB block)
      - partial 512 (used by 1x to 15x 512B blocks)
      - full 512    (used by 16x 512B block)

      When a new block is added, it is added in the first partially used pages for the block type
      If no partially page for the block type is available then a new page from the pool is used
      Attributing first pages allows to recover fasters pages to the pool

      Memory manager provides some statistics allowing to debug, to get information to improve performances.
      (For instance if most of the frames are beetween 512B and 1KB, 1KB blocks should be added)
*/ 



/*==============================================================================
  INCLUDE FILES
==============================================================================*/

#include "ipc_4ghif_mm.h"
#include <linux/ipc_4ghif.h>


/*==============================================================================
  LOCAL CONSTANTS 
==============================================================================*/



/*==============================================================================
  LOCAL MACROS
==============================================================================*/

#define DEBUG(args...) //printk(args)
#define ENTER_FUNC() DEBUG("Enter %s\n", __FUNCTION__)



/*==============================================================================
  LOCAL FUNCTION PROTOTYPES
==============================================================================*/

#if (IPC_4GHIF_DATA_UL_MM_TYPE == IPC_4GHIF_DATA_UL_MM_BUDDY)
    void IPC_4GHIF_data_ul_mm_buddy_init(void);
    u32* IPC_4GHIF_data_ul_mm_buddy_alloc(u16 size);
    void IPC_4GHIF_data_ul_mm_buddy_free(u32 adr); 
    void IPC_4GHIF_data_ul_mm_buddy_get_stat(char* sz, u16 sz_size);
#else
    void IPC_4GHIF_data_ul_mm_basic_init(void);
    u32* IPC_4GHIF_data_ul_mm_basic_alloc(u16 size);
    void IPC_4GHIF_data_ul_mm_basic_free(u32 adr); 
    void IPC_4GHIF_data_ul_mm_basic_get_stat(char* sz, u16 sz_size);
#endif


/*==============================================================================
  EXTERN FUNCTION PROTOTYPES
==============================================================================*/



/*==============================================================================
  LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==============================================================================*/



/*==============================================================================
  LOCAL VARIABLES
==============================================================================*/

#if (IPC_4GHIF_DATA_UL_MM_TYPE == IPC_4GHIF_DATA_UL_MM_BUDDY)

/* For buddy memory manager (512B / 2kB / 8kB cells)*/    

/* critical section */
static spinlock_t ipc_4ghif_data_buddy_mm_lock     = SPIN_LOCK_UNLOCKED;


/* pages / blocks / sectors managments */
u16 mm_bf_pool_pages[3];         //bit field of free pages in the pool
u16 mm_bf_2k_pages_partial[3];   //bit field of partially used pages for 2KB  blocks (free sectors remains)
u16 mm_bf_512_pages_partial[3];  //bit field of partially used pages for 512B blocks (free sectors remains)
u16 mm_bf_8k_pages_full[3];      //bit field of fully used pages for 8KB  blocks (all sectors are used)
u16 mm_bf_2k_pages_full[3];      //bit field of fully used pages for 2KB  blocks (all sectors are used)
u16 mm_bf_512_pages_full[3];     //bit field of fully used pages for 512B blocks (all sectors are used)

u16 mm_bf_sectors_used[MM_NB_PAGE]; //sector used in a page


/* stats */
#define MM_STAT_ON
#define MM_STAT_ALLOC_NB  1024
u16 mm_stat_alloc_size[MM_STAT_ALLOC_NB];
u16 mm_stat_alloc_ind=0;

u16 mm_stat_min_size_allocated;
u16 mm_stat_max_size_allocated;
u32 mm_stat_8k_blocks_allocated_from_start;
u32 mm_stat_2k_blocks_allocated_from_start;
u32 mm_stat_512_blocks_allocated_from_start;
u16 mm_stat_8k_alloc_failure;
u16 mm_stat_2k_alloc_failure;
u16 mm_stat_512_alloc_failure;
u32 mm_stat_8k_blocks_bytes_from_start;
u32 mm_stat_2k_blocks_bytes_from_start;
u32 mm_stat_512_blocks_bytes_from_start;
u32 mm_stat_8k_blocks_below_4k;
u32 mm_stat_2k_blocks_below_1k;
u32 mm_stat_512_blocks_below_256;

#else

/* For basic memory manager (47 * 8kB cells)*/    

/* critical section */
static spinlock_t ipc_4ghif_data_mm_lock     = SPIN_LOCK_UNLOCKED;

/* memory management */
u16 free;
u16 last;
u16 table_next[DATA_UL_MM_NB];

#endif // IPC_4GHIF_DATA_UL_MM_TYPE 


/*==============================================================================
  GLOBAL VARIABLES
==============================================================================*/



/*==============================================================================
  GLOBAL FUNCTIONS
==============================================================================*/

/*
API Function	                    Description

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
void IPC_4GHIF_data_ul_mm_init(void)
{
#if (IPC_4GHIF_DATA_UL_MM_TYPE == IPC_4GHIF_DATA_UL_MM_BUDDY)
    IPC_4GHIF_data_ul_mm_buddy_init();
#else
    IPC_4GHIF_data_ul_mm_basic_init();
#endif   
}


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
u32* IPC_4GHIF_data_ul_mm_alloc(u16 size)
{
#if (IPC_4GHIF_DATA_UL_MM_TYPE == IPC_4GHIF_DATA_UL_MM_BUDDY)
    return IPC_4GHIF_data_ul_mm_buddy_alloc(size);
#else
    return IPC_4GHIF_data_ul_mm_basicalloc(size);
#endif    
}


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
        - block address is not a used sector
=====================================================================================================================*/
void IPC_4GHIF_data_ul_mm_free(u32 adr)
{
#if (IPC_4GHIF_DATA_UL_MM_TYPE == IPC_4GHIF_DATA_UL_MM_BUDDY)
    IPC_4GHIF_data_ul_mm_buddy_free(adr);
#else
    IPC_4GHIF_data_ul_mm_basic_free(adr);
#endif    
}


/*=====================================================================================================================
Name: IPC_4GHIF_data_ul_mm_get_stat
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
void IPC_4GHIF_data_ul_mm_get_stat_string(char* sz, u16 sz_size)
{
#if (IPC_4GHIF_DATA_UL_MM_TYPE == IPC_4GHIF_DATA_UL_MM_BUDDY)
    IPC_4GHIF_data_ul_mm_buddy_get_stat(sz, sz_size);
#else
    IPC_4GHIF_data_ul_mm_basic_get_stat(sz, sz_size);
#endif    
}



/*==============================================================================
  LOCAL FUNCTIONS
==============================================================================*/

#if (IPC_4GHIF_DATA_UL_MM_TYPE == IPC_4GHIF_DATA_UL_MM_BUDDY)

/* For buddy memory manager (512B / 2kB / 8kB cells)*/    

/*------------------------------ bit field functions -------------------------------------*/

/*
    first_one_bit_index_in_u16

    This function return the index of the first bit at one of the passed u16
    if bit_field = 0x1254 = 0b0001 0010 0101 0100 (bit15..0) fct returns 2 

    research is quick because it needs only 4 tests

    if it is needed to be quicker, it is possible to replace it
    by a return constant array (but more space is needed
*/
int first_one_bit_index_in_u16(u16 bit_field)
{
    if(bit_field==0) return -1; //no bit at 1

    if(bit_field&0xff)
    {
        if(bit_field&0xf) //search one bit at 0 in xxxxxxxxxxxxTTTT
        {
            if(bit_field&0x3) { if(bit_field&0x1) return 0; else return 1; }
            else              { if(bit_field&0x4) return 2; else return 3; } 
        }
        else               //search one bit at 0 in xxxxxxxxTTTTxxxx
        {
            if(bit_field&0x30) { if(bit_field&0x10) return 4; else return 5; }
            else               { if(bit_field&0x40) return 6; else return 7; } 
        }
    }
    else
    {
        if(bit_field&0xf00) //search one bit at 0 in xxxxTTTTxxxxxxxx
        {
            if(bit_field&0x300) { if(bit_field&0x100) return 8;  else return 9;  }
            else                { if(bit_field&0x400) return 10; else return 11; } 
        }
        else               //search one bit at 0 in TTTTxxxxxxxxxxxx
        {
            if(bit_field&0x3000) { if(bit_field&0x1000) return 12; else return 13; }
            else                 { if(bit_field&0x4000) return 14; else return 15; } 
        }
    }
}


/*
    first_zero_bit_index_in_u16

    This function returns  the index of the first bit at zero of the passed u16
    if bit_field = 0x1253 = 0b0001 0010 0101 0011 (bit15..0) fct returns 2 

    research is quick because it needs only 4 tests

    if it is needed to be quicker, it is possible to save the inv and call by
    implementing the same algo as first_one_bit_index_in_u16
*/
int first_zero_bit_index_in_u16(u16 bit_field)
{
    return first_one_bit_index_in_u16(bit_field^0xffff);
}


/*
    find_page_in_mm_bf_pages

    This fct finds the first page in the provided bit field

*/
int find_page_in_mm_bf_pages(u16 tab[])
{
    if(tab[0]!=0) return    first_one_bit_index_in_u16(tab[0]);
    if(tab[1]!=0) return 16+first_one_bit_index_in_u16(tab[1]);
    if(tab[2]!=0) return 32+first_one_bit_index_in_u16(tab[2]);
    return -1;
}


/*
    get_1_nb_bit_in_u16
    
    This function returns the number of bits at 1 in a u16
*/
int get_1_nb_bit_in_u16(u16 word)
{
    int i,nb;
       
    nb=0;
    for(i=0;i<16;i++) if(word&(1<<i)) nb++;
    
    return nb;
}



/*------------------------------ allocate / free code -------------------------------------*/

static u32 allocate_8k(void)
{
    u32 adr;
    int page;
    unsigned long flags;    

    spin_lock_irqsave(&ipc_4ghif_data_buddy_mm_lock,flags);

    /* alloc a new page */
    page=find_page_in_mm_bf_pages(mm_bf_pool_pages);
    if(page==-1) 
    { 
        spin_unlock_irqrestore(&ipc_4ghif_data_buddy_mm_lock,flags);
        return (u32)NULL; //no space;
    }

    /* remove page from pool */
    mm_bf_pool_pages[page/16]&=(0xffff^(1<<(page&0x0f)));

    /* mark page as used (full in the 8K case) */
    mm_bf_8k_pages_full[page/16]|=(1<<(page&0x0f));

    /* mark sectors as used */
    mm_bf_sectors_used[page]=0xffff;

    spin_unlock_irqrestore(&ipc_4ghif_data_buddy_mm_lock,flags);

    /* compute address */
    adr=MM_MEM_BASE_ADR+page*MM_PAGE_SIZE;
    DEBUG("adr=0x%0x (page=%d)",adr,page);

    return adr;
}


static u32 allocate_2k(void)
{
    u32 adr;
    int page;
    int sector;
    unsigned long flags;    
    
    spin_lock_irqsave(&ipc_4ghif_data_buddy_mm_lock,flags);   

    page=find_page_in_mm_bf_pages(mm_bf_2k_pages_partial);

    /* alloc new page (if needed) */
    if(page==-1)
    {
        /* alloc a new page */
        page=find_page_in_mm_bf_pages(mm_bf_pool_pages);
        if(page==-1) 
        { 
            spin_unlock_irqrestore(&ipc_4ghif_data_buddy_mm_lock,flags);
            return (u32)NULL; //no space;
        }

        /* remove page from pool */
        mm_bf_pool_pages[page/16]&=(0xffff^(1<<(page&0x0f)));

        /* mark page as partially used (free in the 2K case) */
        mm_bf_2k_pages_partial[page/16]|=(1<<(page&0x0f));

        /* mark sectors as unused except the firsts */
        mm_bf_sectors_used[page]=0x000f;
        sector=0;
    }
    else
    {
        sector=first_zero_bit_index_in_u16(mm_bf_sectors_used[page]);

        if(sector==-1) panic("4GHIF ERROR: MM: No sector available in allocate 2K"); //never

        /* mark sectors are used */
        mm_bf_sectors_used[page]|=0xf<<sector;

        /* put page in full if all sectors are used */
        if(mm_bf_sectors_used[page]==0xffff)
        {
             /* remove page from partial */
             mm_bf_2k_pages_partial[page/16]&=(0xffff^(1<<(page&0x0f)));

             /* put page as full */
             mm_bf_2k_pages_full[page/16]|=(1<<(page&0x0f));
        }
    }
    spin_unlock_irqrestore(&ipc_4ghif_data_buddy_mm_lock,flags);

    adr=MM_MEM_BASE_ADR+page*MM_PAGE_SIZE+sector*MM_SECTOR_SIZE; 
    DEBUG("ar=0x%0x (page=%d, sector=%d)",adr,page,sector);

    return adr;
}


static u32 allocate_512(void)
{
    u32 adr;
    int page;
    int sector;
    unsigned long flags;    

    spin_lock_irqsave(&ipc_4ghif_data_buddy_mm_lock,flags);

    page=find_page_in_mm_bf_pages(mm_bf_512_pages_partial);

    /* alloc new page (if needed) */
    if(page==-1)
    {
        /* alloc a new page */
        page=find_page_in_mm_bf_pages(mm_bf_pool_pages);
        if(page==-1) 
        { 
            spin_unlock_irqrestore(&ipc_4ghif_data_buddy_mm_lock,flags);
            return (u32)NULL; //no space;
        }

        /* remove page from pool */
        mm_bf_pool_pages[page/16]&=(0xffff^(1<<(page&0x0f)));

        /* mark page as partially used */
        mm_bf_512_pages_partial[page/16]|=(1<<(page&0x0f));

        /* mark sectors as unused except the first */
        mm_bf_sectors_used[page]=1;
        sector=0;
    }
    else
    {
        sector=first_zero_bit_index_in_u16(mm_bf_sectors_used[page]);

        if(sector==-1) panic("4GHIF ERROR: MM: No sector available in allocate 512"); //never

        /* mark sectors are used */
        mm_bf_sectors_used[page]|=1<<sector;

        /* put page in full if all sectors are used */
        if(mm_bf_sectors_used[page]==0xffff)
        {
            /* remove page from partial */
            mm_bf_512_pages_partial[page/16]&=(0xffff^(1<<(page&0x0f)));

            /* put page as full */
            mm_bf_512_pages_full[page/16]|=(1<<(page&0x0f));
        }
    }
    spin_unlock_irqrestore(&ipc_4ghif_data_buddy_mm_lock,flags);

    adr=MM_MEM_BASE_ADR+page*MM_PAGE_SIZE+sector*MM_SECTOR_SIZE;
    DEBUG("adr=0x%0x (page=%d, sector=%d)",adr,page,sector);

    return adr;
}


static void free_8k(u8 page)  //sector information not used
{
    unsigned long flags;

    spin_lock_irqsave(&ipc_4ghif_data_buddy_mm_lock,flags);

    /* mark sectors as unused */
    mm_bf_sectors_used[page]=0;

    /* remove 8K page full */
    mm_bf_8k_pages_full[page/16]&=(0xffff^(1<<(page&0x0f)));

    /* add free page */
    mm_bf_pool_pages[page/16]|=(1<<(page&0x0f));

    spin_unlock_irqrestore(&ipc_4ghif_data_buddy_mm_lock,flags);
}


static void free_2k(u8 page, u8 sector)
{
    unsigned long flags;

    if(sector%4) panic("Bad sector alignment");

    spin_lock_irqsave(&ipc_4ghif_data_buddy_mm_lock,flags);

    /* if page was full, put it in partial */
    if(mm_bf_sectors_used[page]==0xffff)
    {
        /* remove 2K page full */
        mm_bf_2k_pages_full[page/16]&=(0xffff^(1<<(page&0x0f)));

        /* set 2K page partial */
        mm_bf_2k_pages_partial[page/16]|=(1<<(page&0x0f));
    }

    /* mark sectors as unused */
    mm_bf_sectors_used[page]&=(0xffff^(0xf<<sector));

    /* if page empty return it to the pool */
    if(mm_bf_sectors_used[page]==0)
    {
        /* remove 2K page free */
        mm_bf_2k_pages_partial[page/16]&=(0xffff^(1<<(page&0x0f)));

        /* add free page */
        mm_bf_pool_pages[page/16]|=(1<<(page&0x0f));
    }

    spin_unlock_irqrestore(&ipc_4ghif_data_buddy_mm_lock,flags);
}


static void free_512(u8 page, u8 sector)
{
    unsigned long flags;

    spin_lock_irqsave(&ipc_4ghif_data_buddy_mm_lock,flags);

    /* if page was full, put it in partial */
    if(mm_bf_sectors_used[page]==0xffff)
    {
        /* remove 512 page full */
        mm_bf_512_pages_full[page/16]&=(0xffff^(1<<(page&0x0f)));

        /* set 512 page partial */
        mm_bf_512_pages_partial[page/16]|=(1<<(page&0x0f));
    }

    /* mark sectors as unused */
    mm_bf_sectors_used[page]&=(0xffff^(1<<sector));

    /* if page empty return it to the pool */
    if(mm_bf_sectors_used[page]==0)
    {
        /* remove 2K page free */
        mm_bf_512_pages_partial[page/16]&=(0xffff^(1<<(page&0x0f)));

        /* add free page */
        mm_bf_pool_pages[page/16]|=(1<<(page&0x0f));
    }

    spin_unlock_irqrestore(&ipc_4ghif_data_buddy_mm_lock,flags);
}


/*------------------------------ buddy manager functions -------------------------------------*/

void IPC_4GHIF_data_ul_mm_buddy_init(void)
{
    ENTER_FUNC()
    
    int i;

    /* all pages are put in the pool */

    for(i=0;i<3;i++)
    {
        mm_bf_pool_pages[i]=0xffff;
        mm_bf_8k_pages_full[i]=0;
        mm_bf_2k_pages_partial[i]=0;  mm_bf_2k_pages_full[i]=0;
        mm_bf_512_pages_partial[i]=0; mm_bf_512_pages_full[i]=0;    
    }

    for(i=0;i<MM_NB_PAGE;i++) mm_bf_sectors_used[i]=0;

    /* init stats */

    mm_stat_alloc_ind=0;
    for(i=0;i<MM_STAT_ALLOC_NB;i++) mm_stat_alloc_size[i]=0xffff;
    mm_stat_min_size_allocated=0xffff;
    mm_stat_max_size_allocated=0;
    mm_stat_8k_blocks_allocated_from_start=0;
    mm_stat_2k_blocks_allocated_from_start=0;
    mm_stat_512_blocks_allocated_from_start=0;
    mm_stat_8k_alloc_failure=0;
    mm_stat_2k_alloc_failure=0;
    mm_stat_512_alloc_failure=0;
    mm_stat_8k_blocks_bytes_from_start=0;
    mm_stat_2k_blocks_bytes_from_start=0;
    mm_stat_512_blocks_bytes_from_start=0;
    mm_stat_8k_blocks_below_4k=0;
    mm_stat_2k_blocks_below_1k=0;
    mm_stat_512_blocks_below_256=0;
}


u32* IPC_4GHIF_data_ul_mm_buddy_alloc(u16 size)
{
    u32 adr;
   
    ENTER_FUNC()

    /* some checks */
    if(size>MM_PAGE_SIZE) panic("4GHIF ERROR: MM: Cannot allocate more than 8KB");

#ifndef MM_STAT_ON
    /* allocation in correct blocks */
    if(size>2048)     return allocate_8k(); 
    else if(size>512) return allocate_2k();
    else              return allocate_512();
#else
    /* some stats */
    if(size<mm_stat_min_size_allocated) mm_stat_min_size_allocated=size;
    if(size>mm_stat_max_size_allocated) mm_stat_max_size_allocated=size;

    mm_stat_alloc_size[mm_stat_alloc_ind++]=size;
    if(mm_stat_alloc_ind==MM_STAT_ALLOC_NB) mm_stat_alloc_ind=0;

    /* allocation in correct blocks */
    if(size>2048) 
    {
        adr=allocate_8k();

        if(adr!=(u32)NULL) 
        { 
            mm_stat_8k_blocks_allocated_from_start++; 
            mm_stat_8k_blocks_bytes_from_start+=size; 
            if(size<=4096) mm_stat_8k_blocks_below_4k++;
        } 
        else mm_stat_8k_alloc_failure++;
    }
    else if(size>512)
    {
        adr=allocate_2k();

        if(adr!=(u32)NULL) 
        { 
            mm_stat_2k_blocks_allocated_from_start++; 
            mm_stat_2k_blocks_bytes_from_start+=size; 
            if(size<=1024) mm_stat_2k_blocks_below_1k++;
        } 
        else mm_stat_2k_alloc_failure++;
    }
    else
    {
        adr=allocate_512();
        if(adr!=(u32)NULL) 
        { 
            mm_stat_512_blocks_allocated_from_start++; 
            mm_stat_512_blocks_bytes_from_start+=size; 
            if(size<=256) mm_stat_512_blocks_below_256++;
        }
        else mm_stat_512_alloc_failure++;
    }

    return (u32*)adr;
#endif //MM_STAT_ON
}


void IPC_4GHIF_data_ul_mm_buddy_free(u32 adr)
{
    u8 page;
    u8 sector;
    
    ENTER_FUNC()
    DEBUG("Request to free: 0x%x",adr);

    /* some checks */
    if((adr<MM_MEM_BASE_ADR)||(adr>=(MM_MEM_BASE_ADR+MM_MEM_SIZE))) panic("4GHIF ERROR: MM: FREE ADR OUTSIDE MEMORY RANGE");  
    
    if(((adr-MM_MEM_BASE_ADR)%MM_SECTOR_SIZE)!=0) panic("\n4GHIF ERROR: MM: FREE ADR NOT ALIGNED ON A SECTOR");     

    page=(u8) ((adr-MM_MEM_BASE_ADR)/MM_PAGE_SIZE);
    sector=(u8) ((adr-MM_MEM_BASE_ADR-page*MM_PAGE_SIZE)/MM_SECTOR_SIZE);

    if((mm_bf_sectors_used[page]&(1<<sector))==0) panic("4GHIF ERROR: MM: FREE SECTOR NOT USED"); 

    /* free according type */
    if(mm_bf_8k_pages_full[page/16]&(1<<(page&15))) free_8k(page);

    else if((mm_bf_2k_pages_partial[page/16]|mm_bf_2k_pages_full[page/16])&(1<<(page&15))) free_2k(page,sector);

    else free_512(page,sector);
}


void IPC_4GHIF_data_ul_mm_buddy_get_stat(char* sz, u16 sz_size)  
{
    char szTemp[512];
    int nb_pool_page,nb_8k_page,nb_2k_page_full,nb_2k_page_partial,nb_512_page_full,nb_512_page_partial;
    int nb_2k_block_used,nb_512_block_used;
    int i;
    
    /* 1. extract statistics from bit fields */
    
    nb_pool_page=nb_8k_page=nb_2k_page_full=nb_2k_page_partial=nb_512_page_full=nb_512_page_partial=0;
    for(i=0;i<(MM_NB_PAGE/16);i++)
    {
        nb_pool_page       +=get_1_nb_bit_in_u16(mm_bf_pool_pages[i]);
        nb_8k_page         +=get_1_nb_bit_in_u16(mm_bf_8k_pages_full[i]);
	nb_2k_page_full    +=get_1_nb_bit_in_u16(mm_bf_2k_pages_full[i]);
	nb_2k_page_partial +=get_1_nb_bit_in_u16(mm_bf_2k_pages_partial[i]);
	nb_512_page_full   +=get_1_nb_bit_in_u16(mm_bf_512_pages_full[i]);
	nb_512_page_partial+=get_1_nb_bit_in_u16(mm_bf_512_pages_partial[i]);
    }
    nb_2k_block_used=4*nb_2k_page_full;
    for(i=0;i<MM_NB_PAGE;i++) if(mm_bf_2k_pages_partial[i/16]&(1<<(i&0x1f)))
    {
        nb_2k_block_used+=(get_1_nb_bit_in_u16(mm_bf_sectors_used[i])/4);
    }
    nb_512_block_used=16*nb_512_page_full;
    for(i=0;i<MM_NB_PAGE;i++) if(mm_bf_512_pages_partial[i/16]&(1<<(i&0x1f)))
    {
        nb_512_block_used+=get_1_nb_bit_in_u16(mm_bf_sectors_used[i]);
    }
    
    /* 2. generate statistic string
    
       buddy ul memory manager:
       8KB pages:
         Free 10/48                                                 
         8KB  10/48                       
         2KB  10/48 (3  full) 11blocks/40 
         512B 18/48 (10 full) 11blocks/160 */

    sprintf(szTemp,"Buddy ul memory manager:\n8KB pages\n Free %2d/48\n 8KB  %2d/48\n 2KB  %2d/48 (%2d  full) %dblocks/%d\n 512B %2d/48 (%2d  full) %dblocks/%d",
                nb_pool_page,
                nb_8k_page,
		nb_2k_page_full+nb_2k_page_partial,nb_2k_page_full,nb_2k_block_used,4*(nb_2k_page_full+nb_2k_page_partial),
		nb_512_page_full+nb_512_page_partial,nb_512_page_full,nb_512_block_used,16*(nb_512_page_full+nb_512_page_partial) );
		
    strncpy(sz,szTemp,(int)sz_size);
}




#else // IPC_4GHIF_DATA_UL_MM_TYPE

/* For basic memory manager (47 * 8kB cells)*/    


void IPC_4GHIF_data_ul_mm_basic_init(void)
{
    int i;
    
    free=0;
    last=DATA_UL_MM_NB-1;
    for(i=0;i<(DATA_UL_MM_NB-1);i++) table_next[i]=i+1;
    table_next[DATA_UL_MM_NB-1]=0; 
}


u32* IPC_4GHIF_data_ul_mm_basic_alloc(u16 size)
{

    u32 adr;
    u8 use;
    unsigned long flags;
    
    /* some checks */
    if(size>DATA_UL_MM_SIZE) panic("4GHIF ERROR: UL PACKET TOO BIG");
    
    spin_lock_irqsave(&ipc_4ghif_data_mm_lock,flags);

    if(free==last) //DATA_UL full
    {
        spin_unlock_irqrestore(&ipc_4ghif_data_mm_lock,flags);
        return NULL; 
    }
    
    /* compute the physical address */ 
    adr=(u32)(DATA_UL_MM_BASE_ADR+free*DATA_UL_MM_SIZE);
    
    /* return the adress of the first cell in the linked list */
    use=free;
    free=table_next[free];
    table_next[use]=use;   //point on own cell to mark cell is used
    
    spin_unlock_irqrestore(&ipc_4ghif_data_mm_lock,flags);

    DEBUG("4GHIF: Address 0x%x allocated\n",adr);
  
    return ((u32*)adr);
}


void IPC_4GHIF_data_ul_mm_basic_free(u32 adr)
{
    u8 ind;
    unsigned long flags;
    
    DEBUG("4GHIF: Free address 0x%x\n",adr);
    
    /* some checks */
    if(((adr-DATA_UL_MM_BASE_ADR)%DATA_UL_MM_SIZE)!=0) panic("4GHIF ERROR: FREE UL DATA shared memory: address not aligned\n"); //not aligned
    
    ind=(adr-DATA_UL_MM_BASE_ADR)/DATA_UL_MM_SIZE;
    
    if(ind>DATA_UL_MM_NB) panic("4GHIF ERROR: FREE UL DATA shared memory: out of range address\n"); //outside range
    
    spin_lock_irqsave(&ipc_4ghif_data_mm_lock,flags);
   
    if(table_next[ind]!=ind) 
    {
        spin_unlock_irqrestore(&ipc_4ghif_data_mm_lock,flags);
        panic("4GHIF ERROR: FREE UL DATA shared memory: cell already free\n"); //cell not used so nothing to free
    }
    
    /* point next of this cell somwhere else (not important, just to mark cell not used) */
    table_next[ind]=ind+1; if(table_next[ind]==DATA_UL_MM_NB) table_next[ind]=0;

    /* add the freed cell at the end of the linked list */
    table_next[last]=ind; last=ind;

    spin_unlock_irqrestore(&ipc_4ghif_data_mm_lock,flags);
}


void IPC_4GHIF_data_ul_mm_basic_get_stat(char* sz, u16 sz_size)
{
    int i;
    int used_blocks;
    char szTemp[512];
    
    /* 1. extract statistics */
     
    used_blocks=0;
    for(i=0;i<DATA_UL_MM_NB;i++) if(table_next[i]==i) used_blocks++; 

    /* 2. generate statistic string 
    
        basic ul memory manager:
	8KB used blocks: 12/47 */
    
    sprintf(szTemp,"Basic ul memory manager:\n  8KB used blocks: %d/%d",used_blocks,DATA_UL_MM_NB-1);

    strncpy(sz,szTemp,(int)sz_size);
}


#endif //IPC_4GHIF_DATA_UL_MM_TYPE



void IPC_4GHIF_data_ul_display_last_allocated_size(void)
{
    int i,ind;
    
#if (IPC_4GHIF_DATA_UL_MM_TYPE == IPC_4GHIF_DATA_UL_MM_BUDDY)
    printk("\nMM last allocated size");
    for(i=0;i<MM_STAT_ALLOC_NB;i++)
    {
        ind=mm_stat_alloc_ind+i;
        if(ind>=MM_STAT_ALLOC_NB) ind-=MM_STAT_ALLOC_NB;
	if(mm_stat_alloc_size[ind]!=0xffff) printk("\nSIZE= %d",mm_stat_alloc_size[ind]);
    }
    printk("\n");
#else
    printk("\nNo data for basic MM\n");
#endif    
}


