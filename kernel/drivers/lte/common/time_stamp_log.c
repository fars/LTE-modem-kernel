/*
 * Copyright Motorola, Inc. 2009
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 *  Cambridge, MA 02139, USA
 */



#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/mm.h> 

#include <linux/ctype.h>
#include <linux/string.h>

#include "time_stamp_log.h"


//******************************************************************************
// defines
//******************************************************************************

#define TSL_SIGNATURE                        0xBEBEBAAF


//******************************************************************************
// typedefs
//******************************************************************************

/**
 * @struct tsl_log_
 * @brief time stamp log internal structure
 *
 * The TSL object
 */
typedef struct
{

    unsigned long signature;
    unsigned long size;
    unsigned long enable;
    spinlock_t sl_lock;
    int locked;
    unsigned long log_start;
    unsigned long log_end;
    unsigned long log_wrapped;
    unsigned long log_index;
    unsigned long tsl_entry_num_elements;
    unsigned long tsl_entry_index;
    tsl_entry_t* ptsl_entry;
    
} tsl_log_t;


//******************************************************************************
// static functions
//******************************************************************************

/**
 * Validates the object pointer as ours
 *
 * @param tsl_log_t* ptsl
 *
 * @return long, error code
 */ 
static long tsl_private_valid(tsl_log_t* ptsl)
{
    if (!ptsl || ptsl->signature != TSL_SIGNATURE || ptsl->size != sizeof(tsl_log_t))
        return TSL_ERROR_TSL_PRIVATE_INVALID;

    return TSL_ERROR_ERROR_NONE;
}


//******************************************************************************
// exported functions
//******************************************************************************

/**
 * Initializes the time stamp library.  Allocates resources necessary for
 * performing time stamps.
 *
 * @param - none
 *
 * @return long, error code
 */
long tsl_initialize(void)
{   
    return TSL_ERROR_ERROR_NONE;    
}


/**
 * Finialize the time stamp library.  Releases resources necessary for
 * performing time stamps.
 *
 * @param - none
 *
 * @return long, error code
 */
long tsl_finalize(void)
{    
    return TSL_ERROR_ERROR_NONE;    
}


/**
 * Allocate a time stamp log.
 *
 * @param - none
 *
 * @return void*, tsl structure
 */
void* tsl_alloc(unsigned long num_elements)
{
    tsl_log_t* ptsl_log = NULL;
    size_t tsl_entry_size = 0;
    unsigned long i = 0;
    tsl_entry_t* ptsl_entry = NULL;
    
    
    if (num_elements < TSL_MIN_ELEMENTS || num_elements > TSL_MAX_ELEMENTS)
        return NULL;
    
    ptsl_log = (tsl_log_t*) kmalloc(sizeof(tsl_log_t), GFP_KERNEL);
    if (ptsl_log)
    {
        memset(ptsl_log, 0x00, sizeof(tsl_log_t));
        ptsl_log->signature = TSL_SIGNATURE;
        ptsl_log->size = sizeof(tsl_log_t);
        ptsl_log->enable = 1;
        spin_lock_init(&ptsl_log->sl_lock);
        
        tsl_entry_size = sizeof(tsl_entry_t) * num_elements;
        ptsl_log->tsl_entry_num_elements = num_elements;
        ptsl_log->ptsl_entry = (tsl_entry_t*) kmalloc(tsl_entry_size, GFP_KERNEL);
        if (ptsl_log->ptsl_entry)
        {
            memset(ptsl_log->ptsl_entry, 0x00, tsl_entry_size);
            for (i = 0; i < num_elements; i++)
            {
                ptsl_entry = &(ptsl_log->ptsl_entry[i]);
                ptsl_entry->sequence = i;
            }
        }
        else
        {
            kfree(ptsl_log);
            ptsl_log = NULL;
        }
            
    }
    
    return (void*) ptsl_log;    
}


/**
 * Free a time stamp log.
 *
 * @param - void* ptsl - pointer returned from tsl_alloc() 
 *
 * @return long, error code
 */
long tsl_free(void* ptsl)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE; 
    
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;
    
    // free the element array first
    if (ptsl_log->ptsl_entry)
        kfree(ptsl_log->ptsl_entry);
    
    // now free the object
    kfree(ptsl_log);
    

    return TSL_ERROR_ERROR_NONE;    
}


/**
 * Used to lock the TSL object
 * WARNING!!! Only one task should lock and unlock, specifically only the task
 * that calls the functions tsl_log_prime(), tsl_log_get_entry(), etc.
 *
 * @param - void* ptsl - pointer returned from tsl_alloc()
 * @param - long lock - 0 to unlock, otherwise lock
 *
 * @return long, error code
 */
long tsl_lock(void* ptsl, long lock)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE;
    unsigned long sl_flags = 0;
    
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;
    
    if (lock)
    {
        spin_lock_irqsave(&(ptsl_log->sl_lock), sl_flags);
        if (!ptsl_log->locked)
        {
            ptsl_log->locked = 1;
        }
        spin_unlock_irqrestore(&(ptsl_log->sl_lock), sl_flags);
    }
    else
    {
        spin_lock_irqsave(&(ptsl_log->sl_lock), sl_flags);
        if (ptsl_log->locked)
        {
            ptsl_log->locked = 0;
        }
        spin_unlock_irqrestore(&(ptsl_log->sl_lock), sl_flags);
    }
    

    return TSL_ERROR_ERROR_NONE;
}


/**
 * Enable or disable time stamp logging.
 *
 * @param - void* ptsl - pointer returned from tsl_alloc()
 * @param - unsigned long enable - enable or disable
 *
 * @return long, error code
 */
long tsl_enable(void* ptsl, unsigned long enable)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE; 
    
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;
    
    ptsl_log->enable = enable;


    return TSL_ERROR_ERROR_NONE;
}


/**
 * Reset time stamp logging.
 * WARNING!!! You need to make sure that no threads have outstanding indicies
 * they can use to post via tsl_add_time_stamp().  Also, you need to make sure
 * that you are not in the middle of a logging "fetch" sequence using the
 * functions tsl_log_prime(), tsl_log_get_entry(), etc.
 *
 * @param - void* ptsl - pointer returned from tsl_alloc()
 *
 * @return long, error code
 */
long tsl_reset(void* ptsl)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE;
    unsigned long i = 0;
    tsl_entry_t* ptsl_entry = NULL;
    unsigned long sl_flags = 0;
       
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;
    
    spin_lock_irqsave(&(ptsl_log->sl_lock), sl_flags);
    
    // if we are locked, we can't reset so return an error
    if (ptsl_log->locked)
    {
        spin_unlock_irqrestore(&(ptsl_log->sl_lock), sl_flags);
        return TSL_ERROR_TSL_LOCKED;
    }
    
    // reset the current index and other items
    ptsl_log->tsl_entry_index = 0;
    ptsl_log->log_start = 0;
    ptsl_log->log_end = 0;
    ptsl_log->log_wrapped = 0;
    ptsl_log->log_index = 0;
    
    // reset all of the entries and reinit the sequence number for each
    memset(ptsl_log->ptsl_entry, 0x00, sizeof(tsl_entry_t) * ptsl_log->tsl_entry_num_elements);
    for (i = 0; i < ptsl_log->tsl_entry_num_elements; i++)
    {
        ptsl_entry = &(ptsl_log->ptsl_entry[i]);
        ptsl_entry->sequence = i;
    }
            
    spin_unlock_irqrestore(&(ptsl_log->sl_lock), sl_flags);
    
    
    return TSL_ERROR_ERROR_NONE;
}


/**
 * Returns the current TSL index and auto increments
 *
 * @param - void* ptsl - pointer returned from tsl_alloc()
 * @param - unsigned long user - user storage
 * @param - void* puser - user storage
 * @param - unsigned long* pentry_index - index of reserved entry
 *
 * @return long, error code
 */
long tsl_get_entry_index_autoinc(void* ptsl, unsigned long user, void* puser, unsigned long* pentry_index)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE;
    unsigned long entry_index = 0;
    tsl_entry_t* ptsl_entry = NULL;
    unsigned long sl_flags = 0;
    
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;
    
    if (!pentry_index)
        return TSL_ERROR_TSL_INVALID_PARAM;
        
    *pentry_index = TSL_NOP_ENTRY_INDEX;
    
    // if not enabled or we are locked, then return
    if (!ptsl_log->enable || ptsl_log->locked)
        return TSL_ERROR_ERROR_NONE;

    // sync via spinlock
    spin_lock_irqsave(&(ptsl_log->sl_lock), sl_flags);
    
    // current index is what is available
    entry_index = ptsl_log->tsl_entry_index % ptsl_log->tsl_entry_num_elements;

    // get pointer to entry and reset time stamps to zero and set the user fields
    ptsl_entry = &(ptsl_log->ptsl_entry[entry_index]);
    memset(ptsl_entry->stamps, 0x00, sizeof(ptsl_entry->stamps));
    ptsl_entry->user = user;
    ptsl_entry->puser = puser;    

    // return this index to caller for reference
    *pentry_index = entry_index;
    
    // increment for next call
    ptsl_log->tsl_entry_index++;

    // set the wrapped flag
    if (ptsl_log->tsl_entry_index > ptsl_log->tsl_entry_num_elements)
        ptsl_log->log_wrapped = 1;

    spin_unlock_irqrestore(&(ptsl_log->sl_lock), sl_flags);
    

    return TSL_ERROR_ERROR_NONE;
}


/**
 * Returns the current TSL index and auto increments
 *
 * @param - void* ptsl - pointer returned from tsl_alloc()
 * @param - unsigned long* pindex - index of reserved entry
 *
 * @return long, error code
 */
long tsl_add_time_stamp(void* ptsl, unsigned long entry_index, unsigned long ts_index)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE;
    tsl_entry_t* ptsl_entry = NULL; 
    
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;
    
    // if not enabled or passed a NOP index, then return
    if (!ptsl_log->enable || entry_index == TSL_NOP_ENTRY_INDEX)
        return TSL_ERROR_ERROR_NONE;
    
    // validate indicies
    if (entry_index >= ptsl_log->tsl_entry_num_elements || ts_index >= TSL_MAX_STAMPS)
        return TSL_ERROR_TSL_INVALID_PARAM;

    // save the current time into the specified entry and stamp index
    ptsl_entry = &(ptsl_log->ptsl_entry[entry_index]);
    ptsl_entry->stamps[ts_index] = ktime_get();


    return TSL_ERROR_ERROR_NONE;
}


/**
 * Used to prime the start and stop indicies for obtaining the time stamp entries
 *  WARNING!!!  The TSL object should be locked prior to calling this function
 *
 * @param - void* ptsl - pointer returned from tsl_alloc()
 *
 * @return long, error code
 */
long tsl_log_prime(void* ptsl)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE;
    
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;

    // see if we have wrapped
    if (ptsl_log->log_wrapped)
    {
        // wrapped, so start at the oldest entry which would be the next available
        ptsl_log->log_start = (ptsl_log->tsl_entry_index % ptsl_log->tsl_entry_num_elements);
        
        // if start is index zero, then make end the last element in the array,
        // otherwise, the end is the previous element
        if (ptsl_log->log_start == 0)
            ptsl_log->log_end = ptsl_log->tsl_entry_num_elements - 1;
        else
            ptsl_log->log_end = ptsl_log->log_start - 1;
    }
    else
    {
        // no wrap, so simply the first element and then the next available minus one
        ptsl_log->log_start = 0;
        ptsl_log->log_end = ptsl_log->tsl_entry_index - 1;
    }    

    // this log_index is used in the tsl_log_get_entry() function to sequence
    // the log fetches in order
    ptsl_log->log_index = ptsl_log->log_start;
    

    return TSL_ERROR_ERROR_NONE;
}


/**
 * Used to get the next log entry since the last call to this function
 *  WARNING!!!  You must call tsl_log_prime() prior to calling this function.
 *  WARNING!!!  The TSL object should be locked prior to calling this function.
 *  The caller should repeatedly call this function until an error or
 *  TSL_WARNING_TSL_LAST_LOG_ENTRY is returned.
 *
 * @param - void* ptsl - pointer returned from tsl_alloc()
 * @param - tsl_entry_t** pptsl_entry - ptr-ptr to TSL entry
 *
 * @return long, error code
 */
long tsl_log_get_entry(void* ptsl, tsl_entry_t** pptsl_entry)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE;
    unsigned long entry_index = 0;
    tsl_entry_t* ptsl_entry = NULL; 
    
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;
    
    if (!pptsl_entry)
        return TSL_ERROR_TSL_INVALID_PARAM; 
    
    // force into array bounds and then get pointer to structure
    entry_index = (ptsl_log->log_index % ptsl_log->tsl_entry_num_elements);
    ptsl_entry = &(ptsl_log->ptsl_entry[entry_index]);
    
    ptsl_log->log_index++;
    
    // is this the last entry?
    if (entry_index == ptsl_log->log_end)
        retval = TSL_WARNING_TSL_LAST_LOG_ENTRY;
    
    *pptsl_entry = ptsl_entry;
    

    return retval;
        
}


/**
 * Used to get the number of entries
 *  WARNING!!!  The TSL object should be locked prior to calling this function.
 *
 * @param - void* ptsl - pointer returned from tsl_alloc()
 * @param - unsigned long* pnum_entries - number of entries
 *
 * @return long, error code
 */
long tsl_log_get_num_entries(void* ptsl, unsigned long* pnum_entries)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE;
    
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;
    
    if (!pnum_entries)
        return TSL_ERROR_TSL_INVALID_PARAM; 
    
    if (ptsl_log->log_wrapped)    
        *pnum_entries = ptsl_log->tsl_entry_num_elements;
    else
        *pnum_entries = ptsl_log->tsl_entry_index;
    

    return retval;
        
}


/**
 * Used to get the log entry using specified index
 *  WARNING!!!  You must call tsl_log_prime() prior to calling this function.
 *  WARNING!!!  The TSL object should be locked prior to calling this function.
 *
 * @param - void* ptsl - pointer returned from tsl_alloc()
 * @param - unsigned long index - index to fetch
 * @param - tsl_entry_t** pptsl_entry - ptr-ptr to TSL entry
 *
 * @return long, error code
 */
long tsl_log_get_entry_by_index(void* ptsl, unsigned long index, tsl_entry_t** pptsl_entry)
{
    tsl_log_t* ptsl_log = (tsl_log_t*) ptsl;
    long retval = TSL_ERROR_ERROR_NONE;
    unsigned long entry_index = 0;
    tsl_entry_t* ptsl_entry = NULL; 
    
    
    // validate this object
    if ((retval = tsl_private_valid(ptsl_log)) != TSL_ERROR_ERROR_NONE)
        return retval;
    
    if (!pptsl_entry)
        return TSL_ERROR_TSL_INVALID_PARAM; 
    
    // force into array bounds and then get pointer to structure
    entry_index = ((ptsl_log->log_start + index) % ptsl_log->tsl_entry_num_elements);
    ptsl_entry = &(ptsl_log->ptsl_entry[entry_index]);
    
    // is this the last entry?
    if (entry_index == ptsl_log->log_end)
        retval = TSL_WARNING_TSL_LAST_LOG_ENTRY;
    
    *pptsl_entry = ptsl_entry;
    

    return retval;
        
}



