/*
 * Copyright Motorola, Inc. 2009
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at 
 * your option) any later version.  
 *
 * This library is distributed in the hope that it will by useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
 */
/*
 *
 * @brief Common module error codes
 *
 * @file AMP_common_error.h
 */

#ifndef _AMP_COMMON_ERROR_INC_
#define _AMP_COMMON_ERROR_INC_


// Error codes are 32-bit signed values and each module has its own
// error code offset.  The bits are as follows:
//
//  bit 31:         error bit
//  bit 30-28:      unused
//  bit 27-12:      module ID
//  bit 11-0:       error code
//
//  euuu mmmm mmmm mmmm mmmm eeee eeee eeee
//
// NOTICE: Three files exist which are:
//
//      AMP_common_error.h
//      AMP_lte_error.h
//      AMP_wimax_error.h
//

#define AMP_ERROR_NONE                              (0x00000000L)
#define _AMP_ERROR                                  (0x80000000L)


// COMMON modules start at: 0x00001000
// WiMAX modules start at:  0x00010000
// LTE modules start at:    0x00100000
#define _COMMON_ERROR_PLATFORM_OFF                  (0x00001000L)
#define _COMMON_ERROR_IPC_OFF                       (0x00002000L)
#define _COMMON_ERROR_XYZMOD_OFF                    (0x00003000L)

// Platform error codes
#define AMP_ERROR_PLATFORM_INVALID_PARAM            (_AMP_ERROR | _COMMON_ERROR_PLATFORM_OFF | 0x00000000L)
#define AMP_ERROR_PLATFORM_RESOURCE_ALLOC           (_AMP_ERROR | _COMMON_ERROR_PLATFORM_OFF | 0x00000001L)

// IPC error codes
#define AMP_ERROR_IPC_INVALID_PARAM                 (_AMP_ERROR | _COMMON_ERROR_IPC_OFF | 0x00000000L)
#define AMP_ERROR_IPC_RESOURCE_ALLOC                (_AMP_ERROR | _COMMON_ERROR_IPC_OFF | 0x00000001L)

// Some common XYZ module error codes
#define AMP_ERROR_COMXYZ_INVALID_PARAM              (_AMP_ERROR | _COMMON_ERROR_XYZMOD_OFF | 0x00000000L)
#define AMP_ERROR_COMXYZ_RESOURCE_ALLOC             (_AMP_ERROR | _COMMON_ERROR_XYZMOD_OFF | 0x00000001L)


#endif // _AMP_COMMON_ERROR_INC_
