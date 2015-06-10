/*
 * Copyright Motorola, Inc. 2005-2011
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
 * @brief general header file for generic structure or function
 *
 * @file AMP_general.h
 */

#ifndef _MOT_AMP_GENERAL_INC_
#define _MOT_AMP_GENERAL_INC_

#include <linux/lte/AMP_typedef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE 1
#endif

#ifndef ASSERT
# define ASSERT(x)           if(!x) {return -1;}        // JLI:temp
#endif

#ifndef NULL_ASSERT
#define NULL_ASSERT(x)           if(!x) {return NULL;}
#endif

#ifndef AMP_ASSERT
# define AMP_ASSERT(x,...)  if(!(x)) \
                                { \
                                    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_SP, AMP_LOG_LEVEL_DEBUG2, __VA_ARGS__); \
                                    return (eAMP_RET_FAIL); \
                                }
#endif

#ifndef MIN
#define MIN(x,y)        ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y)        ((x) > (y) ? (x) : (y))
#endif

#ifndef AMP_WIMAX_SF_TLV_MAX_LEN
#define AMP_WIMAX_SF_TLV_MAX_LEN(x)     ((x) < (0) ? ((x)*(-1)) : (x))
#endif

// String functions

#define TCHAR char
#define PATHDELIM "/"

typedef enum 
{
    eAMP_RET_FAIL = -1, 
    eAMP_RET_SUCCESS = 0,
    eAMP_RET_BUF_EXCEED_MAX_REF,
    eAMP_RET_BUF_MEM_LEAK,
    eAMP_RET_BUF_OUTOFBUF,
    eAMP_RET_IPC_FAIL,
    eAMP_RET_IPC_IS_USED,
    eAMP_RET_SM_FAIL,
    eAMP_RET_WIMAX_FAIL,
    eAMP_RET_WIMAX_RA_INIT_SCAN_FAIL,
    eAMP_RET_WIMAX_LINK_FAIL_START,
    eAMP_RET_WIMAX_LINK_FAIL_END = eAMP_RET_WIMAX_LINK_FAIL_START + 20, // more than entrys in E_AMP_WIMAX_MAC_LNDW_CODE
    eAMP_RET_WIMAX_QOS_FAIL_START,
    eAMP_RET_WIMAX_QOS_CHANGED_BY_BS,
    eAMP_RET_WIMAX_QOS_DELETED_BY_BS,
    eAMP_RET_WIMAX_QOS_FAIL,
    eAMP_RET_WIMAX_QOS_TRANSACTION_FAIL,    // a duplex transaction is in progress or reached the limit
    eAMP_RET_WIMAX_QOS_ID_FAIL,
    eAMP_RET_WIMAX_QOS_ID_NOT_FOUND,
    eAMP_RET_WIMAX_QOS_FAIL_END,
    eAMP_RET_WIMAX_QOS_TLV_IGNORE,
    eAMP_RET_WIMAX_QOS_PKT_DROP,
    eAMP_RET_WIMAX_QOS_AUTO_ARP,
    eAMP_RET_MAX_VAL
} AMP_RET_CODE_E;

typedef enum 
{
    eAMP_TIMER_UNKNOWN = -1, 
    eAMP_TIMER_SLOW = 0,
    eAMP_TIMER_MED,
    eAMP_TIMER_FAST,
    eAMP_TIMER_MAX_VAL
} AMP_TIMER_E;

// add IPv6 later
typedef struct 
{
    UINT32 u32IpVersion;    // 0 for IPv4, 1 for IPv6
    union
    {
        UINT32 u32Ipv4Addr;
        UINT8 u8Ipv6Addr[16];       // 128 bits
    } IP_ADDR_U;
    UINT16 u16IpPort;
    UINT16 u16IpProtocol;
} AMP_IP_ADDR_S;

// function prototype for timer callback function
typedef INT32 (*AMP_TimerFunctionCallback)(void* pvUserObject);

typedef struct 
{

    // these are public members that should be populated prior to create
    AMP_TimerFunctionCallback   pfTimerFunctionCallback;
    void*                       pvUserData;
    UINT32                      u32TimeOutMS;
    UINT32                      u32SingleShot;
    INT32                       i32RetCode;
    
    // these are private members and should not be modified by the caller
    UINT32                      u32Initialized;
    UINT32                      u32Expired;
    AMP_ThrdId			        sThrId;
	AMP_ThrdHandle		        sThrHandle;
    
} AMP_TimerFunctionObj;

// place holder for future QoS profile mapping
// DSCP   0-7 = CoS 0
// DSCP  8-15 = CoS 1
// DSCP 16-23 = CoS 2
// DSCP 24-31 = CoS 3
// DSCP 32-39 = CoS 4
// DSCP 40-47 = CoS 5
// DSCP 48-55 = CoS 6
// DSCP 56-63 = CoS 7
// 
// DSCP    Precedence        Service
//0             0                         Best effort 
//8             1                         Class 1 
//16             2                         Class 2 
//24             3                         Class 3 
//32             4                         Class 4 
//40                 5                             Express forwarding 
//48             6                         Control 
//56             7                         Control 

 
// 802 CoS    DSCP     Use  
//0 0                  Best effort 
//3 26                 Voice control (SIP, H.323) 
//5 46                 Voice data (RTP, RTSP) 
//2 18                 Better effort data 
//2 18                 Better effort 
//1 10                 Streaming video 
//6 48                 Network-layer control (OSPF, RIP, EIGRP) 
//7 -                 Link-layer control (STP, CDP, UDLD) 

//Precedence Purpose 
//0 Routine 
//1 Priority 
//2 Immediate 
//3 Flash 
//4 Flash Override 
//5 CRITIC/ECP 
//6 Internetwork Control 
//7 Network Control 


typedef struct 
{ 
    UINT8 u8DSCP;
    UINT8 u8Pad[3];
} AMP_DSCP_S;

// Global defines
#define AMP_MAX_NAME_PATH   256

// Linux platform, sme interface use a local unix domain socket, which map to a file trace in the 
    // nothing need to be done now
#define INLINE inline


#if !defined(NDIS_MINIPORT_DRIVER)

// Platform API
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_InitIpcObject(UINT16 u16MyAddrId, void* vInitQObject, void* vInitListObject);
extern EXPORTDLL void* EXPORTCDECL AMP_Malloc(UINT32 u32Size, INT32 n32Priority);
extern EXPORTDLL void EXPORTCDECL AMP_Free(void *vMemPtr);
extern EXPORTDLL AMP_SyncObj EXPORTCDECL AMP_CreateMutex(void);
extern EXPORTDLL AMP_SyncObj EXPORTCDECL AMP_CreateCntSemaphore(void);
extern EXPORTDLL AMP_SyncObj EXPORTCDECL AMP_CreateSpinLock(void);
extern EXPORTDLL AMP_SyncObj EXPORTCDECL AMP_CreateEvent(void);
extern EXPORTDLL void EXPORTCDECL AMP_DestroyMutex(AMP_SyncObj hSyncObj);
extern EXPORTDLL void EXPORTCDECL AMP_DestroyCntSemaphore(AMP_SyncObj hSyncObj);
extern EXPORTDLL void EXPORTCDECL AMP_DestroySpinLock(AMP_SyncObj hSyncObj);
extern EXPORTDLL void EXPORTCDECL AMP_DestroyEvent(AMP_SyncObj hSyncObj);
extern EXPORTDLL INT32 EXPORTCDECL AMP_MutexLock(AMP_SyncObj hSyncObj);
extern EXPORTDLL INT32 EXPORTCDECL AMP_MutexUnLock(AMP_SyncObj hSyncObj);
extern EXPORTDLL INT32 EXPORTCDECL AMP_CntSemTake(AMP_SyncObj hSyncObj);
extern EXPORTDLL INT32 EXPORTCDECL AMP_CntSemTimedTake(AMP_SyncObj hSyncObj, void * timeout);
extern EXPORTDLL INT32 EXPORTCDECL AMP_CntSemGive(AMP_SyncObj hSyncObj);
extern EXPORTDLL INT32 EXPORTCDECL AMP_CrtSecEnter(AMP_SyncObj hSyncObj);
extern EXPORTDLL INT32 EXPORTCDECL AMP_CrtSecLeave(AMP_SyncObj hSyncObj);
extern EXPORTDLL INT32 EXPORTCDECL AMP_WaitEvent(AMP_SyncObj hSyncObj, AMP_SyncObj hMutex, void* timeOut);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SignalEvent(AMP_SyncObj hSyncObj);
extern EXPORTDLL INT32 EXPORTCDECL AMP_CreateThread(AMP_ThrdHandle *handle, AMP_ThrAttr attr, AMP_ThrStartRoutine func, 
    AMP_ThrdArg arg, AMP_ThrCreateFlag flag, AMP_ThrdId *thrId);
extern EXPORTDLL INT32 EXPORTCDECL AMP_TerminateThread(AMP_ThrdHandle handle, AMP_ThrdId thrId, INT32 signal, INT32 priority);
extern EXPORTDLL INT32 EXPORTCDECL AMP_DaemonizeThread(UINT8 name[], INT32 noChDir, INT32 noClose);
extern EXPORTDLL INT32 EXPORTCDECL AMP_WaitThreadFinish(AMP_ThrdHandle handle, AMP_ThrdId thrId);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SignalPending(void * thread);
extern EXPORTDLL void EXPORTCDECL AMP_ReSchedule(UINT32 time);
extern EXPORTDLL AMP_TimerObj EXPORTCDECL AMP_GetCurTime(void);

extern EXPORTDLL UINT32 EXPORTCDECL AMP_GetTimeValue(AMP_TimerObj timeObj);
extern EXPORTDLL AMP_TimeOutObj EXPORTCDECL AMP_CreateTimeOutObj(AMP_TimerObj timeObj, UINT32 timeOutVal);
extern EXPORTDLL void EXPORTCDECL AMP_TimerSub(AMP_TimerObj * tvp, AMP_TimerObj * uvp, AMP_TimerObj * vvp ); 
extern EXPORTDLL void EXPORTCDECL AMP_TimerAdd(AMP_TimerObj * tvp, AMP_TimerObj * uvp, AMP_TimerObj * vvp );
extern EXPORTDLL INT32 EXPORTCDECL AMP_TimerIsSet(AMP_SetTimerObj *tvp);
extern EXPORTDLL INT32 EXPORTCDECL AMP_ControlService(UINT8* cmd, AMP_SvcHandle handle, UINT32 code, void* status);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockStartup(void);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockCleanup(void);
extern AMP_SOCKET AMP_SockCreate(INT32 sockFamily, INT32 sockType, INT32 protocol);
extern AMP_SOCKET AMP_SockAccept(AMP_SOCKET sockHdl, AMP_SOCKADDR *addr, AMP_SOCKLEN *addrLen);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockBind(AMP_SOCKET sockHdl, AMP_SOCKADDR *addr, AMP_SOCKLEN addrLen);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockListen(AMP_SOCKET sockHdl, INT32 backlog);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockSend(AMP_SOCKET sockHdl, char* buf, INT32 len, INT32 flags);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockRecv(AMP_SOCKET sockHdl, char* buf, INT32 len, INT32 flags);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockSetRecvTimeout(AMP_SOCKET sockHdl, UINT32 ui32Timeout);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockGetRecvTimeout(AMP_SOCKET sockHdl, UINT32* pui32Timeout);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockConnect(AMP_SOCKET sockHdl, AMP_SOCKADDR *servAddr, AMP_SOCKLEN addrLen);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockShutdown(AMP_SOCKET sockHdl, INT32 how);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockClose(AMP_SOCKET sockHdl);
extern EXPORTDLL AMP_SOCKET EXPORTCDECL AMP_SockSMECreate(AMP_SME_SOCKADDR  *addr);
extern EXPORTDLL AMP_SOCKET EXPORTCDECL AMP_SockIoctlCreate(AMP_SME_SOCKADDR  *addr);
extern EXPORTDLL INT32 EXPORTCDECL AMP_SockSetBlock(AMP_SOCKET n32Fd, INT32 b32Block);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_GetRootPath(TCHAR *pathBuff,
                                              INT32 buffLen, INT32 *pathLen);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_CreateTimerFunction(
    AMP_TimerFunctionObj* pTimerFunctionObj);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_KillTimerFunction(
    AMP_TimerFunctionObj* pTimerFunctionObj);

#endif // #if !defined(NDIS_MINIPORT_DRIVER)

#ifdef __cplusplus
}
#endif

#endif // #define _MOT_AMP_GENERAL_INC_
