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
/**
 * @file AMP_typedef.h
 * 
 * This file contains the typedefs for the Roaming agent and Quality of Service
 * Agent.
 * 
 * Last Changed Revision: $Rev: 3240 $
 * Last Changed Date: $LastChangedDate: 2009-07-20 22:44:04 -0500 (Mon, 20 Jul 2009) $
 */

#ifndef _MOT_AMP_TYPEDEF_INC_
#define _MOT_AMP_TYPEDEF_INC_

#ifdef __cplusplus
extern "C" {
#endif

//----- Linux kernel space -----
#include <linux/types.h>

#ifndef CONFIG_UBUNTU
typedef int gfp_t;
#endif

typedef u8 UINT8;
typedef u16 UINT16;
typedef u32 UINT32;
typedef u64 UINT64;

typedef s8 INT8;
typedef s16 INT16;
typedef s32 INT32;
typedef s64 INT64;

// sync objects
#define AMP_SyncObj             void*

// threads
#define AMP_ThrdHandle          pid_t
#define AMP_ThrAttr             void*
typedef int (*AMP_ThrStartRoutine)(void *);
#define AMP_ThrdArg             void*
#define AMP_ThrCreateFlag       UINT32
#define AMP_ThrdId              pid_t

// creation flag
#define AMP_CtFlagCloneFS       CLONE_FS 
#define AMP_CtFlagCloneFiles    CLONE_FILES

// service
#define AMP_SvcHandle           void*

// timer
#define AMP_TimerObj            UINT32
#define AMP_TimeOutObj          UINT32

#define AMP_SetTimerObj         UINT32

// socket
#define AMP_SOCKET              INT32
#define AMP_SOCKADDR            INT32
#define AMP_SME_SOCKADDR        INT32
#define AMP_SOCKLEN             INT32

// error code define

// sync object defines
#define AMP_SYNC_MUTEX          (1)
#define AMP_SYNC_BINSEM         AMP_SYNC_MUTEX
#define AMP_SYNC_CNTSEM         (2)
#define AMP_SYNC_SPINLOCK       (3) // windows: critical section, linux kernel: spinlock, user space: same as mutex
#define AMP_SYNC_EVENT          (4) 

#define AMP_SYNC_MAX_SEM_RES    (10) // maximum resource that a counting semaphore guard

#define EXPORTDLL


#define EXPORTCDECL 
#ifdef __cplusplus
}
#endif

#endif // #define _MOT_AMP_TYPEDEF_INC_
