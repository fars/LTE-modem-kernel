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

#if defined(WIN32)
#if !defined(KMDF_VERSION) // Really a check to see if it's a driver

//----- windows platform -----
#include <windows.h>

// sync objects
#define AMP_SyncObj             HANDLE

// threads
#define AMP_ThrdHandle          HANDLE
#define AMP_ThrAttr             LPSECURITY_ATTRIBUTES 
#define AMP_ThrStartRoutine     LPTHREAD_START_ROUTINE
#define AMP_ThrdArg             void*
#define AMP_ThrCreateFlag       UINT32
#define AMP_ThrdId              DWORD

// creation flag
#define AMP_CtFlagCloneFS       0 
#define AMP_CtFlagCloneFiles    0

// service
#define AMP_SvcHandle           SC_HANDLE

// timer
#define AMP_TimerObj            time_t
#define AMP_TimeOutObj          UINT32

#define AMP_SetTimerObj         time_t

// socket
#define AMP_SOCKET              SOCKET
#define AMP_SOCKADDR            struct sockaddr
typedef struct sockaddr_in      AMP_SME_SOCKADDR;
#define AMP_SOCKLEN             INT32
#define AMP_INVALID_SOCKETFD    INVALID_SOCKET

#define AMP_SOCK_AF_UNSPEC      AF_UNSPEC
#define AMP_SOCK_AF_INET        AF_INET
#define AMP_SOCK_AF_INET6       AF_INET6
#define AMP_SOCK_AF_UNIX        -1

#define AMP_SOCK_STREAM         SOCK_STREAM
#define AMP_SOCK_DGRAM          SOCK_DGRAM
#define AMP_SOCK_RAW            SOCK_RAW

#define AMP_SOCK_IPPROTO_TCP    IPPROTO_TCP
#define AMP_SOCK_IPPROTO_UDP    IPPROTO_UDP

// error code define
#define AMP_CODE_WAIT_SUCCESS   WAIT_OBJECT_0
#define AMP_CODE_WAIT_TIMEOUT   WAIT_TIMEOUT
#define AMP_CODE_WAIT_ABANDONED WAIT_ABANDONED
#define AMP_CODE_WAIT_INVALID   -1

#define AMP_CODE_ENOTINITIALISED    WSANOTINITIALISED //The system is not initialized yet
#define AMP_CODE_EAGAIN         WSAEWOULDBLOCK //The socket is marked non-blocking and no connections are present to be accepted. 
#define AMP_CODE_EBADF          WSAEBADF  //The descriptor is invalid. 
#define AMP_CODE_ECONNABORTED   WSAECONNABORTED //A connection has been aborted. 
#define AMP_CODE_EINTR          WSAEINTR //The system call was interrupted
#define AMP_CODE_EINVAL         WSAEINVAL //Socket is not listening for connections, or addrlen is invalid
#define AMP_CODE_EMFILE         WSAEMFILE //The per-process limit of open file descriptors has been reached. 
#define AMP_CODE_ENOTSOCK       WSAENOTSOCK //The descriptor references a file, not a socket. 
#define AMP_CODE_EOPNOTSUPP     WSAEOPNOTSUPP //The referenced socket is not of type SOCK_STREAM.
#define AMP_CODE_EFAULT         WSAEFAULT //The addr argument is not in a writable part of the user address space. 
#define AMP_CODE_ENOMEM         WSAENOBUFS // Not enough free memory, the memory allocation is limited by the socket buffer limits 
#define AMP_CODE_EPROTO         WSAEPROTOTYPE // Protocol error.
#define AMP_CODE_EPERM          WSAEACCES // Firewall rules forbid connection.
#define AMP_CODE_EADDRINUSE     WSAEADDRINUSE // Address in use already

#else

// sync objects
#define AMP_SyncObj             HANDLE

// threads
#define AMP_ThrdHandle          HANDLE
#define AMP_ThrAttr             UINT32 
#define AMP_ThrStartRoutine     (void*())
#define AMP_ThrdArg             void*
#define AMP_ThrCreateFlag       UINT32
#define AMP_ThrdId              DWORD

// timer
#define AMP_TimerObj            time_t
#define AMP_TimeOutObj          UINT32

#define AMP_SetTimerObj         time_t

#endif // #if !defined(NDIS_MINIPORT_DRIVER)

#else

#if !defined(__KERNEL__) || defined(HOST_EMULATION)
//----- Linux user space ------
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <sys/time.h>

#ifdef EZXLOGLEVEL  // for sme integration
#include "EZX_TypeDefs.h"
#else
#ifndef QTOPIA_PHONE
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef signed char INT8;
typedef short INT16;
typedef int INT32;
#ifdef HOST_EMULATION
typedef long long INT64;
#endif
#endif // QTOPIA_PHONE

typedef unsigned long long UINT64;

#endif // EZXLOGLEVEL

// sync objects
#define AMP_SyncObj             void*

// threads
#define AMP_ThrdHandle          pthread_t
#define AMP_ThrAttr             pthread_attr_t* 
typedef void *(*AMP_ThrStartRoutine)(void *); 
#define AMP_ThrdArg             void*
#define AMP_ThrCreateFlag       UINT32
#define AMP_ThrdId              pthread_t

// creation flag
#ifdef HOST_EMULATION
#define AMP_CtFlagCloneFS               0
#define AMP_CtFlagCloneFiles    0
#endif

// service
#define AMP_SvcHandle           void*

// timer
#define AMP_TimerObj            struct timeval
#define AMP_TimeOutObj          struct timespec

#define AMP_SetTimerObj         struct itimerval

// socket
#define AMP_SOCKET              INT32
#define AMP_SOCKADDR            struct sockaddr
typedef struct sockaddr_un      AMP_SME_SOCKADDR;
#define AMP_SOCKLEN             socklen_t
#define AMP_INVALID_SOCKETFD    -1

#define AMP_SOCK_AF_UNSPEC      -1
#define AMP_SOCK_AF_INET        AF_INET
#define AMP_SOCK_AF_INET6       AF_INET6
#define AMP_SOCK_AF_UNIX        AF_UNIX

#define AMP_SOCK_STREAM         SOCK_STREAM
#define AMP_SOCK_DGRAM          SOCK_DGRAM
#define AMP_SOCK_RAW            SOCK_RAW

#define AMP_SOCK_PF_INET        PF_INET
#define AMP_SOCK_PF_INET6       PF_INET6
#define AMP_SOCK_PF_UNIX        PF_UNIX

#define AMP_SOCK_IPPROTO_TCP    IPPROTO_TCP
#define AMP_SOCK_IPPROTO_UDP    IPPROTO_UDP

// error code define
#define AMP_CODE_WAIT_SUCCESS   0
#define AMP_CODE_WAIT_TIMEOUT   ETIMEDOUT
#define AMP_CODE_WAIT_ABANDONED ENOTLOCKED
#define AMP_CODE_WAIT_INVALID   EINVAL

#define AMP_CODE_ENOTINITIALISED    0 //The system is not initialized yet
#define AMP_CODE_EAGAIN         EAGAIN  //The socket is marked non-blocking and no connections are present to be accepted. 
#define AMP_CODE_EBADF          EBADF   //The descriptor is invalid. 
#define AMP_CODE_ECONNABORTED   ECONNABORTED //A connection has been aborted. 
#define AMP_CODE_EINTR          EINTR //The system call was interrupted
#define AMP_CODE_EINVAL         EINVAL //Socket is not listening for connections, or addrlen is invalid
#define AMP_CODE_EMFILE         EMFILE //The per-process limit of open file descriptors has been reached. 
#define AMP_CODE_ENOTSOCK       ENOTSOCK //The descriptor references a file, not a socket. 
#define AMP_CODE_EOPNOTSUPP     EOPNOTSUPP //The referenced socket is not of type SOCK_STREAM.
#define AMP_CODE_EFAULT         EFAULT //The addr argument is not in a writable part of the user address space. 
#define AMP_CODE_ENOMEM         ENOMEM // Not enough free memory, the memory allocation is limited by the socket buffer limits 
#define AMP_CODE_EPROTO         EPROTO // Protocol error.
#define AMP_CODE_EPERM          EPERM // Firewall rules forbid connection.
#define AMP_CODE_EADDRINUSE     EADDRINUSE // Address in use already

#else
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

#endif // #ifndef __KERNEL__

#endif // #ifdef WIN32

// sync object defines
#define AMP_SYNC_MUTEX          (1)
#define AMP_SYNC_BINSEM         AMP_SYNC_MUTEX
#define AMP_SYNC_CNTSEM         (2)
#define AMP_SYNC_SPINLOCK       (3) // windows: critical section, linux kernel: spinlock, user space: same as mutex
#define AMP_SYNC_EVENT          (4) 

#define AMP_SYNC_MAX_SEM_RES    (10) // maximum resource that a counting semaphore guard

#if defined(WIN32) && defined(__cplusplus) && !defined(NDIS_MINIPORT_DRIVER)
#define EXPORTDLL __declspec(dllexport)
#else
#define EXPORTDLL
#endif

#if defined(WIN32) && !defined(NDIS_MINIPORT_DRIVER)
#define EXPORTCDECL __cdecl
#else
#define EXPORTCDECL 
#endif
#ifdef __cplusplus
}
#endif

#endif // #define _MOT_AMP_TYPEDEF_INC_
