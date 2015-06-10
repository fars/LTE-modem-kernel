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


/**
 * @File: time_stamp_log.h
 * @brief Time stamp log library
 *
 * Last Changed Revision: $Rev$
 * Last Changed Date: $LastChangedDate$
 */
 
#ifndef _TIME_STAMP_LOG_H_
#define _TIME_STAMP_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/ktime.h>
#include <linux/hrtimer.h>

//******************************************************************************
// defines
//******************************************************************************

#define TSL_MAX_STAMPS                      5
#define TSL_MIN_ELEMENTS                    1
#define TSL_MAX_ELEMENTS                    (1024 * 16)
#define TSL_NOP_ENTRY_INDEX                 0x7FFFFFFF

#define TSL_ERROR_ERROR_NONE                 0
#define TSL_ERROR_TSL_PRIVATE_INVALID       -1
#define TSL_ERROR_TSL_INVALID_PARAM         -2
#define TSL_ERROR_TSL_LOCKED                -3

#define TSL_WARNING_TSL_LAST_LOG_ENTRY       1
    

//******************************************************************************
// typedefs
//******************************************************************************

/**
 * @struct tsl_entry_t
 * @brief time stamp entry
 *
 * Used in the creation of a network object
 */
typedef struct
{

    unsigned long sequence;
    unsigned long user;
    void* puser;
    union ktime stamps[TSL_MAX_STAMPS];
    
} tsl_entry_t;


//******************************************************************************
// exported functions
//******************************************************************************

long tsl_initialize(void);
long tsl_finalize(void);
void* tsl_alloc(unsigned long num_elements);
long tsl_free(void* ptsl);
long tsl_lock(void* ptsl, long lock);
long tsl_enable(void* ptsl, unsigned long enable);
long tsl_reset(void* ptsl);
long tsl_get_entry_index_autoinc(void* ptsl, unsigned long user, void* puser, unsigned long* pentry_index);
long tsl_add_time_stamp(void* ptsl, unsigned long entry_index, unsigned long ts_index);
long tsl_log_prime(void* ptsl);
long tsl_log_get_entry(void* ptsl, tsl_entry_t** pptsl_entry);
long tsl_log_get_num_entries(void* ptsl, unsigned long* pnum_entries);
long tsl_log_get_entry_by_index(void* ptsl, unsigned long index, tsl_entry_t** pptsl_entry);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif  // _TIME_STAMP_LOG_H_



