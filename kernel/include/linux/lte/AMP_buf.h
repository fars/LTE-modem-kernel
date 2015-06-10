/*
 * Copyright Motorola, Inc. 2005-2008
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

#ifndef _MOT_AMP_BUF_INC_
#define _MOT_AMP_BUF_INC_

#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_general.h>
#include <linux/lte/AMP_global.h>

#define AMP_BUF_MAGIC (0x4D57)          // 'M''W'
#define AMP_BUF_MAX_REF (10)            // Need this value so PMDT functionality works properly.

//#define AMP_BUF_OWNER_IPC 0x01;


typedef enum
{
    eAMP_BUF_OWNER_UNKNOWN = 0,
    eAMP_BUF_OWNER_IPC = 1,
    eAMP_BUF_OWNER_RA = 2,
    eAMP_BUF_OWNER_ATOMIC = 0x10,  // specifies to use ATOMIC for kernel malloc
    eAMP_BUF_OWNER_MAX_VAL
} AMP_BUF_OWNER_E;

typedef struct
{
    UINT32 u16Magic:16;         // magic code, should be 'M''W'
    UINT32 u8Owner:8;           // For debug memory leak
    UINT32 n8UseCount:8;        // to avoid memory copy when forward
    UINT32 u32Size;             // only the length of data
} AMP_BUF_HDR_S;
#define AMP_BUF_HDR_LEN sizeof(AMP_BUF_HDR_S)

typedef struct
{
    AMP_BUF_HDR_S sHdr;
    UINT8 u8Data[1];
} AMP_BUF_S;

//static E_AMP_RET_CODE AMP_BUF_Init (void);
//static E_AMP_RET_CODE AMP_BUF_Uninit (void);

AMP_RET_CODE_E AMP_BUF_Get (UINT8 u8Owner, UINT32 u32Length, INT32 n32Priority, void ** ppBuf);
AMP_RET_CODE_E AMP_BUF_Free (void * pBuf);
AMP_RET_CODE_E AMP_BUF_AddRef (void * pBuf);

#endif  //_MOT_AMP_BUF_INC_
