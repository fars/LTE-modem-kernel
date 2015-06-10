
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
 * Cambridge, MA 02139, USA
 */
 
 /**
 * @file ptr_fifo.h
 * @brief ptr_fifo exported functions
 *
 * Last Changed Revision: $Rev$
 * Last Changed Date: $LastChangedDate$
 */
 
#ifndef __PTR_FIFO_H__
#define __PTR_FIFO_H__

#ifdef __cplusplus
extern "C" {
#endif

//******************************************************************************
// defines
//******************************************************************************

#define PTR_FIFO_ERROR_NONE                 0
#define PTR_FIFO_ERROR_INVALID_PARAMETER   -1
#define PTR_FIFO_ERROR_INVALID_FIFO_SIZE   -2
#define PTR_FIFO_ERROR_RESOURCE_ALLOC      -3
#define PTR_FIFO_ERROR_INVALID_HANDLE      -4
#define PTR_FIFO_ERROR_FIFO_OVERFLOW       -5
#define PTR_FIFO_ERROR_FIFO_UNDERFLOW      -6


#define PTR_FIFO_FLAG_NONE                  0x00000000
#define PTR_FIFO_FLAG_NO_LOCK               0x00000001

//******************************************************************************
// tyepdefs
//******************************************************************************

typedef void* PTR_FIFO_HANDLE;


//******************************************************************************
// exported functions
//******************************************************************************

long ptr_fifo_alloc(unsigned long fifo_size, unsigned long flags, PTR_FIFO_HANDLE* phptr_fifo);
long ptr_fifo_free(PTR_FIFO_HANDLE* phptr_fifo);
long ptr_fifo_is_empty(PTR_FIFO_HANDLE hptr_fifo, int* pempty);
long ptr_fifo_num_elements(PTR_FIFO_HANDLE hptr_fifo, int* pnum_elements);
long ptr_fifo_free_avail(PTR_FIFO_HANDLE hptr_fifo, int* pfree_avail);
long ptr_fifo_insert(PTR_FIFO_HANDLE hptr_fifo, void* pv);
long ptr_fifo_remove(PTR_FIFO_HANDLE hptr_fifo, void** ppv);
long ptr_fifo_peek(PTR_FIFO_HANDLE hptr_fifo, void** ppv);
long ptr_fifo_peek_offset(PTR_FIFO_HANDLE hptr_fifo, unsigned long offset, void** ppv);

#ifdef __cplusplus
}
#endif

#endif // __PTR_FIFO_H__

