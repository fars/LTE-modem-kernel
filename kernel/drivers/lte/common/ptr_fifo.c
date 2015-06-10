/*
 * Copyright Motorola, Inc. 2009
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */
 
 /**
 * @file ptr_fifo.c
 * @brief ptr_fifo implementation
 *
 * Last Changed Revision: $Rev$
 * Last Changed Date: $LastChangedDate$
 */


#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#endif


#include "ptr_fifo.h"


//******************************************************************************
// defines
//******************************************************************************

//#define _ENABLE_DEBUG_PRINT

#if defined(__KERNEL__)

// lock marcos
#define ACQUIRE_LOCK(__ptrfifo,__sl_flags)\
{\
    if (!(__ptrfifo->flags & PTR_FIFO_FLAG_NO_LOCK))\
        spin_lock_irqsave(&(__ptrfifo->sl_lock), __sl_flags);\
}
#define RELEASE_LOCK(__ptrfifo,__sl_flags)\
{\
    if (!(__ptrfifo->flags & PTR_FIFO_FLAG_NO_LOCK))\
        spin_unlock_irqrestore(&(__ptrfifo->sl_lock), __sl_flags);\
}
#define ALLOC_LOCK(__ptrfifo)\
{\
    if (!(__ptrfifo->flags & PTR_FIFO_FLAG_NO_LOCK))\
        spin_lock_init(&(__ptrfifo->sl_lock));\
}
#define FREE_LOCK(__ptrfifo)\
{\
\
}

// memory alloc and free macros
#define MEM_ALLOC_FUNC(memsize) kmalloc(memsize, GFP_KERNEL)
#define MEM_FREE_FUNC(memptr) kfree(memptr)

#if defined (_ENABLE_DEBUG_PRINT)
#define DEBUG_PRINT(a) printk(a)
#else
#define DEBUG_PRINT(a)
#endif

#else

// user space and windows macros needs to be implemented

#endif


// the FIFO objects signature
#define PTR_FIFO_SIGNATURE  0xBEEDDEEB


//******************************************************************************
// typedefs
//******************************************************************************

typedef struct
{
    unsigned long signature;        // PTR_FIFO signature
    unsigned long size;             // this structure size
    unsigned long fifo_size;        // size of the fifo data
    unsigned long flags;            // fifo creation flags
    void** pfifo_data;              // fifo data itself
    int tail;                       // tail index
    int head;                       // head index
    int num_elements;               // current number of elements
#if defined(__KERNEL__)
    spinlock_t sl_lock;             // linux kernel lock
#else
    // user space and windows locks needs to be implemented
#endif
    
} PTR_FIFO;


//******************************************************************************
// static functions
//******************************************************************************

/**
 * Validates the PTR_FIFO as ours
 *
 * @param PTR_FIFO* pptr_fifo
 *
 * @return long, error code
 */ 
static long ptr_fifo_object_valid(PTR_FIFO* pptr_fifo)
{
    
    if (!pptr_fifo || pptr_fifo->signature != PTR_FIFO_SIGNATURE || pptr_fifo->size != sizeof(PTR_FIFO))
        return PTR_FIFO_ERROR_INVALID_HANDLE;

    return PTR_FIFO_ERROR_NONE;
}


//******************************************************************************
// exported functions
//******************************************************************************

/**
 * Allocate PTR_FIFO object
 *
 * @param unsigned long fifo_size - fifo element size
 * @param PTR_FIFO_HANDLE* phptr_fifo - ptr to a PTR_FIFO object
 *
 * @return long, error code
 */ 
long ptr_fifo_alloc(unsigned long fifo_size, unsigned long flags, PTR_FIFO_HANDLE* phptr_fifo)
{
    PTR_FIFO* pptr_fifo = NULL;
    

    if (!phptr_fifo)
        return PTR_FIFO_ERROR_INVALID_PARAMETER;
    
    if (fifo_size < 2)
        return PTR_FIFO_ERROR_INVALID_FIFO_SIZE;
    
    // we need one extra element due to FIFO full check method
    fifo_size += 1;
    
    *phptr_fifo = NULL;
    
    pptr_fifo = (PTR_FIFO*) MEM_ALLOC_FUNC(sizeof(PTR_FIFO));
    if (!pptr_fifo)
        return PTR_FIFO_ERROR_RESOURCE_ALLOC;
    
    memset(pptr_fifo, 0x00, sizeof(PTR_FIFO));
    
    pptr_fifo->signature = PTR_FIFO_SIGNATURE;
    pptr_fifo->size = sizeof(PTR_FIFO);
    pptr_fifo->fifo_size = fifo_size;
    pptr_fifo->flags = flags;
    
    // now allocate the fifo data itself, void* x fifo_size
    pptr_fifo->pfifo_data = MEM_ALLOC_FUNC(sizeof(void*) * fifo_size);
    if (!pptr_fifo->pfifo_data)
    {
        MEM_FREE_FUNC(pptr_fifo);
        return PTR_FIFO_ERROR_RESOURCE_ALLOC;
    }
    
    // allocate the locking mechanism
    ALLOC_LOCK(pptr_fifo);
    
    // cast to handle
    *phptr_fifo = (PTR_FIFO_HANDLE*) pptr_fifo;
    
    return PTR_FIFO_ERROR_NONE;
}


/**
 * Free PTR_FIFO object
 *
 * @param PTR_FIFO_HANDLE* phptr_fifo - ptr to a PTR_FIFO object
 *
 * @return long, error code
 */ 
long ptr_fifo_free(PTR_FIFO_HANDLE* phptr_fifo)
{
    PTR_FIFO* pptr_fifo = NULL;
    long retval = PTR_FIFO_ERROR_NONE;


    if (!phptr_fifo)
        return PTR_FIFO_ERROR_INVALID_PARAMETER;
    
    pptr_fifo = (PTR_FIFO*) *((PTR_FIFO_HANDLE*)phptr_fifo);
    
    // validate the handle
    if ((retval = ptr_fifo_object_valid(pptr_fifo)) != PTR_FIFO_ERROR_NONE)
        return retval;
    
    // free the locking mechanism
    FREE_LOCK(pptr_fifo);
    
    if (pptr_fifo->pfifo_data)
    {
        MEM_FREE_FUNC(pptr_fifo->pfifo_data);
        pptr_fifo->pfifo_data = NULL;
    }
    
    MEM_FREE_FUNC(pptr_fifo);
    
    // reset back to NULL
    *phptr_fifo = (PTR_FIFO_HANDLE*) NULL;
    
    
    return retval;
}


/**
 * Returns the empty status of the PTR_FIFO object
 *
 * @param PTR_FIFO_HANDLE hptr_fifo - handle to a PTR_FIFO object
 * @param int* pempty - 1 if empty, otherwise 0
 *
 * @return long, error code
 */
long ptr_fifo_is_empty(PTR_FIFO_HANDLE hptr_fifo, int* pempty)
{
    PTR_FIFO* pptr_fifo = (PTR_FIFO*) hptr_fifo;
    unsigned long sl_flags = 0;
    long retval = PTR_FIFO_ERROR_NONE; 
    
    
    // validate the handle
    if ((retval = ptr_fifo_object_valid(pptr_fifo)) != PTR_FIFO_ERROR_NONE)
        return retval;
    
    if (!pempty)
        return PTR_FIFO_ERROR_INVALID_PARAMETER;
    
    ACQUIRE_LOCK(pptr_fifo, sl_flags);
    
    *pempty = (pptr_fifo->head == pptr_fifo->tail) ? 1 : 0;
    
    RELEASE_LOCK(pptr_fifo, sl_flags);
    
    return retval;
}


/**
 * Returns the number of elements in the PTR_FIFO object
 *
 * @param PTR_FIFO_HANDLE hptr_fifo - handle to a PTR_FIFO object
 * @param int* pnum_elements - number of elements
 *
 * @return long, error code
 */
long ptr_fifo_num_elements(PTR_FIFO_HANDLE hptr_fifo, int* pnum_elements)
{
    PTR_FIFO* pptr_fifo = (PTR_FIFO*) hptr_fifo;
    unsigned long sl_flags = 0;
    long retval = PTR_FIFO_ERROR_NONE;
    
    
    // validate the handle
    if ((retval = ptr_fifo_object_valid(pptr_fifo)) != PTR_FIFO_ERROR_NONE)
        return retval;
    
    if (!pnum_elements)
        return PTR_FIFO_ERROR_INVALID_PARAMETER;
    
    ACQUIRE_LOCK(pptr_fifo, sl_flags);
    
    *pnum_elements = pptr_fifo->num_elements;
    
    RELEASE_LOCK(pptr_fifo, sl_flags);
    
    return retval;
}


/**
 * Returns the number of free elements available in the PTR_FIFO object
 *
 * @param PTR_FIFO_HANDLE hptr_fifo - handle to a PTR_FIFO object
 * @param int* pfree_avail - number of free available
 *
 * @return long, error code
 */
long ptr_fifo_free_avail(PTR_FIFO_HANDLE hptr_fifo, int* pfree_avail)
{
    PTR_FIFO* pptr_fifo = (PTR_FIFO*) hptr_fifo;
    unsigned long sl_flags = 0;
    long retval = PTR_FIFO_ERROR_NONE;
    
    
    // validate the handle
    if ((retval = ptr_fifo_object_valid(pptr_fifo)) != PTR_FIFO_ERROR_NONE)
        return retval;
    
    if (!pfree_avail)
        return PTR_FIFO_ERROR_INVALID_PARAMETER;

    ACQUIRE_LOCK(pptr_fifo, sl_flags);
    
    // remember, we increased this size by 1 in the alloc function
    *pfree_avail = (pptr_fifo->fifo_size - 1) - pptr_fifo->num_elements;
    
    RELEASE_LOCK(pptr_fifo, sl_flags);
    
    return retval;   
}


/**
 * Inserts a pointer into in the PTR_FIFO object
 *
 * @param PTR_FIFO_HANDLE hptr_fifo - handle to a PTR_FIFO object
 * @param void* pv - pointer to insert
 *
 * @return long, error code
 */
long ptr_fifo_insert(PTR_FIFO_HANDLE hptr_fifo, void* pv)
{
    PTR_FIFO* pptr_fifo = (PTR_FIFO*) hptr_fifo;
    unsigned long sl_flags = 0;
    long retval = PTR_FIFO_ERROR_NONE;
    int t = 0;
#if defined (_ENABLE_DEBUG_PRINT)
    int free_avail = 0;
    char dbgmsg[512];
#endif    
 
 
    // validate the handle
    if ((retval = ptr_fifo_object_valid(pptr_fifo)) != PTR_FIFO_ERROR_NONE)
        return retval;
        
    ACQUIRE_LOCK(pptr_fifo, sl_flags);
    
    // check to see if tail + 1 equals head
    t = (pptr_fifo->tail + 1) % pptr_fifo->fifo_size;
    if(t == pptr_fifo->head)
    {
#if defined (_ENABLE_DEBUG_PRINT)
        sprintf(dbgmsg, "FIFO overflow\n");
#endif
        RELEASE_LOCK(pptr_fifo, sl_flags);
        DEBUG_PRINT(dbgmsg);
        
        return PTR_FIFO_ERROR_FIFO_OVERFLOW;
    }
    else
    {
        pptr_fifo->tail = t;
        pptr_fifo->pfifo_data[pptr_fifo->tail] = pv;
        pptr_fifo->num_elements++;
        
 #if defined (_ENABLE_DEBUG_PRINT)
        free_avail = (pptr_fifo->fifo_size - 1) - pptr_fifo->num_elements;
        sprintf(dbgmsg, "Insert ptr:%p at index:%d: num_elements:%d free avail:%d\n",
            pv, pptr_fifo->tail, pptr_fifo->num_elements, free_avail);
 #endif
 
    }
    
    RELEASE_LOCK(pptr_fifo, sl_flags);
    DEBUG_PRINT(dbgmsg);
    
    return retval;
}


/**
 * Removes a pointer from the PTR_FIFO object
 *
 * @param PTR_FIFO_HANDLE hptr_fifo - handle to a PTR_FIFO object
 * @param void** ppv - pointer to receive removed pointer
 *
 * @return long, error code
 */
long ptr_fifo_remove(PTR_FIFO_HANDLE hptr_fifo, void** ppv)
{
    PTR_FIFO* pptr_fifo = (PTR_FIFO*) hptr_fifo;
    unsigned long sl_flags = 0;
    long retval = PTR_FIFO_ERROR_NONE;
    int empty;
#if defined (_ENABLE_DEBUG_PRINT)
    int free_avail = 0;
    char dbgmsg[512];
#endif    
    
    
    // validate the handle
    if ((retval = ptr_fifo_object_valid(pptr_fifo)) != PTR_FIFO_ERROR_NONE)
        return retval;
    
    if (!ppv)
        return PTR_FIFO_ERROR_INVALID_PARAMETER;
        
        
    ACQUIRE_LOCK(pptr_fifo, sl_flags);
    
    empty = (pptr_fifo->head == pptr_fifo->tail) ? 1 : 0;
    if (empty)
    {
#if defined (_ENABLE_DEBUG_PRINT)
        strcpy(dbgmsg, "FIFO underflow\n");
#endif
        RELEASE_LOCK(pptr_fifo, sl_flags);
        DEBUG_PRINT(dbgmsg);
        
        return PTR_FIFO_ERROR_FIFO_UNDERFLOW;
    }
    else
    {
        pptr_fifo->head = (pptr_fifo->head + 1) % pptr_fifo->fifo_size;
        *ppv = pptr_fifo->pfifo_data[pptr_fifo->head];
        pptr_fifo->num_elements--;
 
 #if defined (_ENABLE_DEBUG_PRINT)
        free_avail = (pptr_fifo->fifo_size - 1) - pptr_fifo->num_elements;
        sprintf(dbgmsg, "Removed ptr:%p from index:%d: num_elements:%d free avail:%d\n",
            *ppv, pptr_fifo->head, pptr_fifo->num_elements, free_avail);
 #endif
 
    }
    
    RELEASE_LOCK(pptr_fifo, sl_flags);
    DEBUG_PRINT(dbgmsg);
    
    return retval;
}

/**
 * Returns a pointer from the PTR_FIFO object head, but doesn't remove
 *
 * @param PTR_FIFO_HANDLE hptr_fifo - handle to a PTR_FIFO object
 * @param void** ppv - pointer to receive "peeked" pointer
 *
 * @return long, error code
 */
long ptr_fifo_peek(PTR_FIFO_HANDLE hptr_fifo, void** ppv)
{
    PTR_FIFO* pptr_fifo = (PTR_FIFO*) hptr_fifo;
    unsigned long sl_flags = 0;
    long retval = PTR_FIFO_ERROR_NONE;
    int empty;
    int head;
#if defined (_ENABLE_DEBUG_PRINT)
    int free_avail = 0;
    char dbgmsg[512];
#endif    
    
    
    // validate the handle
    if ((retval = ptr_fifo_object_valid(pptr_fifo)) != PTR_FIFO_ERROR_NONE)
        return retval;
    
    if (!ppv)
        return PTR_FIFO_ERROR_INVALID_PARAMETER;
        
        
    ACQUIRE_LOCK(pptr_fifo, sl_flags);
    
    empty = (pptr_fifo->head == pptr_fifo->tail) ? 1 : 0;
    if (empty)
    {
#if defined (_ENABLE_DEBUG_PRINT)
        strcpy(dbgmsg, "FIFO underflow\n");
#endif
        RELEASE_LOCK(pptr_fifo, sl_flags);
        DEBUG_PRINT(dbgmsg);
        
        return PTR_FIFO_ERROR_FIFO_UNDERFLOW;
    }
    else
    {
        head = (pptr_fifo->head + 1) % pptr_fifo->fifo_size;
        *ppv = pptr_fifo->pfifo_data[head];
 
 #if defined (_ENABLE_DEBUG_PRINT)
        free_avail = (pptr_fifo->fifo_size - 1) - pptr_fifo->num_elements;
        sprintf(dbgmsg, "Peek ptr:%p from index:%d: num_elements:%d free avail:%d\n",
            *ppv, head, pptr_fifo->num_elements, free_avail);
 #endif
 
    }
    
    RELEASE_LOCK(pptr_fifo, sl_flags);
    DEBUG_PRINT(dbgmsg);
    
    return retval;
}


/**
 * Returns a pointer from the PTR_FIFO object head + offset, but doesn't remove
 *
 * @param PTR_FIFO_HANDLE hptr_fifo - handle to a PTR_FIFO object
 * @param unsigned long offset - offset from head (offset = 0 is head)
 * @param void** ppv - pointer to receive "peeked" pointer
 *
 * @return long, error code
 */
long ptr_fifo_peek_offset(PTR_FIFO_HANDLE hptr_fifo, unsigned long offset, void** ppv)
{
    PTR_FIFO* pptr_fifo = (PTR_FIFO*) hptr_fifo;
    unsigned long sl_flags = 0;
    long retval = PTR_FIFO_ERROR_NONE;
    int empty;
    int head;
#if defined (_ENABLE_DEBUG_PRINT)
    int free_avail = 0;
    char dbgmsg[512];
#endif    
    
    
    // validate the handle
    if ((retval = ptr_fifo_object_valid(pptr_fifo)) != PTR_FIFO_ERROR_NONE)
        return retval;
    
    if (!ppv)
        return PTR_FIFO_ERROR_INVALID_PARAMETER;
    
    
    ACQUIRE_LOCK(pptr_fifo, sl_flags);
    
    empty = (pptr_fifo->head == pptr_fifo->tail) ? 1 : 0;
    if (empty)
    {
#if defined (_ENABLE_DEBUG_PRINT)
        strcpy(dbgmsg, "FIFO underflow\n");
#endif
        RELEASE_LOCK(pptr_fifo, sl_flags);
        DEBUG_PRINT(dbgmsg);
        
        return PTR_FIFO_ERROR_FIFO_UNDERFLOW;
    }
    else
    {
        // make sure that we don't exceed the number of elements
        if (offset + 1 > pptr_fifo->num_elements)
        {
#if defined (_ENABLE_DEBUG_PRINT)
            strcpy(dbgmsg, "FIFO underflow\n");
#endif
            RELEASE_LOCK(pptr_fifo, sl_flags);
            DEBUG_PRINT(dbgmsg);
            
            return PTR_FIFO_ERROR_FIFO_UNDERFLOW;
        }
    
        head = (pptr_fifo->head + 1 + offset) % pptr_fifo->fifo_size;
        *ppv = pptr_fifo->pfifo_data[head];
 
 #if defined (_ENABLE_DEBUG_PRINT)
        free_avail = (pptr_fifo->fifo_size - 1) - pptr_fifo->num_elements;
        sprintf(dbgmsg, "Peek Offset (%d) ptr:%p from index:%d: num_elements:%d free avail:%d\n",
            offset, *ppv, head, pptr_fifo->num_elements, free_avail);
 #endif
 
    }
    
    RELEASE_LOCK(pptr_fifo, sl_flags);
    DEBUG_PRINT(dbgmsg);
    
    return retval;
}




