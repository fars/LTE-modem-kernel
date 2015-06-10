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

#ifndef _AMP_LTE_ERROR_INC_
#define _AMP_LTE_ERROR_INC_

#include <AMP_common_error.h>

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


// COMMON modules start at: 0x00001000
// WiMAX modules start at:  0x00010000
// LTE modules start at:    0x00100000
#define _LTE_ERROR_NET_DRV_OFF                      (0x00100000L)
#define _LTE_ERROR_LTEDD_OFF                        (0x00200000L)
#define _LTE_ERROR_HIFD_OFF                         (0x00300000L)

// Net Driver error codes
#define AMP_ERROR_NET_DRV_INVALID_PARAM             (_AMP_ERROR | _LTE_ERROR_NET_DRV_OFF | 0x00000000L)
#define AMP_ERROR_NET_DRV_RESOURCE_ALLOC            (_AMP_ERROR | _LTE_ERROR_NET_DRV_OFF | 0x00000001L)
#define AMP_ERROR_NET_DRV_REGISTRATION              (_AMP_ERROR | _LTE_ERROR_NET_DRV_OFF | 0x00000002L)
#define AMP_ERROR_NET_DRV_MAX_DEVICES               (_AMP_ERROR | _LTE_ERROR_NET_DRV_OFF | 0x00000003L)
#define AMP_ERROR_NET_DRV_DEVICE_NOT_FOUND          (_AMP_ERROR | _LTE_ERROR_NET_DRV_OFF | 0x00000004L)
#define AMP_ERROR_NET_DRV_INTERNAL                  (_AMP_ERROR | _LTE_ERROR_NET_DRV_OFF | 0x00000005L)
#define AMP_ERROR_NET_DRV_PRIVATE_INVALID           (_AMP_ERROR | _LTE_ERROR_NET_DRV_OFF | 0x00000006L)
#define AMP_ERROR_NET_DRV_BUF_TO_SMALL              (_AMP_ERROR | _LTE_ERROR_NET_DRV_OFF | 0x00000007L)
#define AMP_ERROR_NET_DRV_PACKET_DROPPED            (_AMP_ERROR | _LTE_ERROR_NET_DRV_OFF | 0x00000008L)

// LTE Data Driver error codes
#define AMP_ERROR_LTEDD_INVALID_PARAM               (_AMP_ERROR | _LTE_ERROR_LTEDD_OFF | 0x00000000L)
#define AMP_ERROR_LTEDD_RESOURCE_ALLOC              (_AMP_ERROR | _LTE_ERROR_LTEDD_OFF | 0x00000001L)
#define AMP_ERROR_LTEDD_PRIVATE_INVALID             (_AMP_ERROR | _LTE_ERROR_LTEDD_OFF | 0x00000002L)
#define AMP_ERROR_LTEDD_FIFO_INSERT                 (_AMP_ERROR | _LTE_ERROR_LTEDD_OFF | 0x00000003L)
#define AMP_ERROR_LTEDD_FIFO_REMOVE                 (_AMP_ERROR | _LTE_ERROR_LTEDD_OFF | 0x00000004L)
#define AMP_ERROR_LTEDD_INTERNAL_SYNC               (_AMP_ERROR | _LTE_ERROR_LTEDD_OFF | 0x00000005L)
#define AMP_ERROR_LTEDD_NET_INFO                    (_AMP_ERROR | _LTE_ERROR_LTEDD_OFF | 0x00000006L)

// HIFd error codes
#define AMP_ERROR_HIFD_INVALID_PARAM                (_AMP_ERROR | _LTE_ERROR_HIFD_OFF | 0x00000000L)
#define AMP_ERROR_HIFD_RESOURCE_ALLOC               (_AMP_ERROR | _LTE_ERROR_HIFD_OFF | 0x00000001L)


#endif // _AMP_LTE_ERROR_INC_
