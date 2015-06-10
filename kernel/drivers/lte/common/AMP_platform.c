/*
 * Copyright Motorola, Inc. 2005-2008
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
 */

/*
 * @file platform.c
 * @brief Common code space for platform related API (thread, mutex, semaphore ...) 
 *
 * @note to save some kernel name space, if an API doesn't have a functional body, I don't
 *       export the symbol. If later on you have change the API and add in the functional
 *       body, you need to also export the symbol in the kernel space.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_general.h>
#include <linux/lte/AMP_ipc.h>

/*******************************************************************************
* Constants
********************************************************************************/

/*******************************************************************************
* Typedefs
********************************************************************************/

/*******************************************************************************
* Global Variables
********************************************************************************/

/*******************************************************************************
* Local Variables
********************************************************************************/

/*******************************************************************************
* Local Prototypes
********************************************************************************/
AMP_SyncObj Si_CreateSyncObject(INT32 n32Type, INT32 u32InitValue);
void Si_DestorySyncObject(AMP_SyncObj hSyncObj, INT32 n32Type);

/*******************************************************************************
* Local Functions
********************************************************************************/
/**
 * @private
 * Create an AMP Synchronization object with indicated type and indicated initial value.
 *
 * @param n32Type - [in] Type of Synchronization object.
 * @param n32InitValue - [in] Initial value of Synchronization object.
 * @return Pointer to the newly created Synchronization object.
 */
AMP_SyncObj Si_CreateSyncObject(INT32 n32Type, INT32 n32InitValue)
{
    AMP_SyncObj hSyncObj = NULL; 

    switch(n32Type)
    {
        case AMP_SYNC_MUTEX:
            break;
            
        case AMP_SYNC_CNTSEM:

            break;
            
        case AMP_SYNC_SPINLOCK:

            break;

        case AMP_SYNC_EVENT:

            break;
            
        default:
            //AMP_ASSERT(0, "AMP_CreateSyncObject: invalid sync object type\n");
            break;
    }
    
    return hSyncObj;
}

/**
 * @private
 * Destroy an AMP Synchronization object
 *
 * @param hSyncObj - [in] Synchronization object handle.
 * @param n32Type - [in] Synchronization object type.
 */
void Si_DestorySyncObject(AMP_SyncObj hSyncObj, INT32 n32Type)
{
    if (!hSyncObj)
        return;

    switch(n32Type)
    {
        case AMP_SYNC_MUTEX:
   
            break;
            
        case AMP_SYNC_CNTSEM:
   
            break;
            
        case AMP_SYNC_SPINLOCK:
 
            break;

        case AMP_SYNC_EVENT:
  
            break;

        default:
            break;
    }
}

/*******************************************************************************
* Global Functions
********************************************************************************/

// ---- IPC entry ----
/**
 * Initialize necessary object pieces for an given IPC node.
 *
 * @param u16MyAddrId - [in] IPC node.
 * @param *vInitQObject - [in] IPC init object. Linux only.
 * @param *vInitListObject - [in] IPC init object list header. Linux only.
 */
AMP_RET_CODE_E AMP_InitIpcObject(UINT16 u16MyAddrId, void* vInitQObject, void* vInitListObject)
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;

    eRet = AMP_IPC_Init(u16MyAddrId, NULL);

    return eRet;
}
EXPORT_SYMBOL(AMP_InitIpcObject);


// ---- Memory ----
/**
 * Memory allocation. 
 *
 * @param u32Size - [in] Memory size.
 * @param n32Priority - [in] Memory allocation priority flag.
 */
void* AMP_Malloc(UINT32 u32Size, INT32 n32Priority)
{
    void* vPtr = NULL;
    vPtr = (void*)malloc(u32Size);
    return vPtr;
}
EXPORT_SYMBOL(AMP_Malloc);

/**
 * Free the given memory buffer.
 *
 * @param *vMemPtr - [in] Memory buffer pointer.
 */
void AMP_Free(void *vMemPtr)
{
    if (!vMemPtr)
        return;

    free(vMemPtr);
}
EXPORT_SYMBOL(AMP_Free);

// ---- Sync objects (semaphore, mutext, spinlock, ..) ----
/**
 * Create a mutex object.
 *
 * @return Pointer to the newly created mutex object.
 */
AMP_SyncObj AMP_CreateMutex(void)
{
    return Si_CreateSyncObject(AMP_SYNC_MUTEX, FALSE); // initially not owned
}

/**
 * Create a counting semaphore.
 *
 * @return Pointer to the newly created counting semaphore object.
 */
AMP_SyncObj AMP_CreateCntSemaphore(void)
{
    return Si_CreateSyncObject(AMP_SYNC_CNTSEM, 0); // initial count to 0
}

/**
 * Create a spin lock.
 *
 * @return Pointer to the newly created spin lock object.
 */
AMP_SyncObj AMP_CreateSpinLock(void)
{
    return Si_CreateSyncObject(AMP_SYNC_SPINLOCK, 0); 
}

/**
 * Create a waitable event.
 *
 * @return Pointer to the newly created waitable event object.
 */
AMP_SyncObj AMP_CreateEvent(void)
{
    return Si_CreateSyncObject(AMP_SYNC_EVENT, 0); 
}

/**
 * Destroy the mutex object.
 *
 * @param hSyncObj - [in] Mutex object handler.
 */
void AMP_DestroyMutex(AMP_SyncObj hSyncObj)
{
    return Si_DestorySyncObject(hSyncObj, AMP_SYNC_MUTEX);
}

/**
 * Destroy the counting semaphore object.
 *
 * @param hSyncObj - [in] Counting semaphore handler.
 */
void AMP_DestroyCntSemaphore(AMP_SyncObj hSyncObj)
{
    return Si_DestorySyncObject(hSyncObj, AMP_SYNC_CNTSEM);
}

/**
 * Destroy the spin lock object.
 *
 * @param hSyncObj - [in] Spin lock handler.
 */
void AMP_DestroySpinLock(AMP_SyncObj hSyncObj)
{
    return Si_DestorySyncObject(hSyncObj, AMP_SYNC_SPINLOCK);
}

/**
 * Destroy the waitable event object.
 *
 * @param hSyncObj - [in] Waitable event handler.
 */
void AMP_DestroyEvent(AMP_SyncObj hSyncObj)
{
    return Si_DestorySyncObject(hSyncObj, AMP_SYNC_EVENT);
}

/**
 * Enter the mutex lock.
 *
 * @param hSyncObj - [in] Mutex object handler.
 */
INT32 AMP_MutexLock(AMP_SyncObj hSyncObj)
{
    INT32 status = 0;

    return status;
}

/**
 * Leave the mutex lock.
 *
 * @param hSyncObj - [in] Mutex object handler.
 */
INT32 AMP_MutexUnLock(AMP_SyncObj hSyncObj)
{
    INT32 status = 0;

    return status;
}

/**
 * Take the counting semaphore.
 *
 * @param hSyncObj - [in] Counting semaphore handler.
 */
INT32 AMP_CntSemTake(AMP_SyncObj hSyncObj)
{
    INT32 status = 0;

    return status;
}

/**
 * Take the counting semaphore.
 *
 * @param hSyncObj - [in] Counting semaphore handler.
 */
INT32 AMP_CntSemTimedTake(AMP_SyncObj hSyncObj, void *timeOut)
{
    INT32 status = 0;
    return status;
}

/**
 * Give the counting semaphore.
 *
 * @param hSyncObj - [in] Counting semaphore handler.
 */
INT32 AMP_CntSemGive(AMP_SyncObj hSyncObj)
{
    INT32 status = 0;
    return status;
}

/**
 * Enter the critical section.
 *
 * @param hSyncObj - [in] Critical section handler.
 */
INT32 AMP_CrtSecEnter(AMP_SyncObj hSyncObj)
{
    INT32 status = 0;

    return status;
}

/**
 * Leave the critical section.
 *
 * @param hSyncObj - [in] Critical section handler.
 */
INT32 AMP_CrtSecLeave(AMP_SyncObj hSyncObj)
{
    INT32 status = 0;

    return status;
}

/**
 * Wait on an event object.
 *
 * @param hSyncObj - [in] Event object that we are waiting on.
 * @param hMutex - [in] Mutex handler we are holding.
 * @param *timeOut - [in] Time out.
 */
INT32 AMP_WaitEvent(AMP_SyncObj hSyncObj, AMP_SyncObj hMutex, void* timeOut)
{
    INT32 status = 0;

    return status;
}

/**
 * Signal an event object.
 *
 * @param hSyncObj - [in] Event object that we will signal on.
 */
INT32 AMP_SignalEvent(AMP_SyncObj hSyncObj)
{
    INT32 status = 0;

    return status;
}

// ---- Thread ----
/**
 * Create a thread.
 *
 * @param attr - [in] Target thread attribute.
 * @param func - [in] Target thread main entrance.
 * @param arg - [in] Arg passed in to the target thread.
 * @param flag - [in] Target thread creation flag.
 * @param *handle - [out] Return handle for the newly created thread.
 * @param *thrId - [out] Return thread ID for the newly created thread.
 */
INT32 AMP_CreateThread(AMP_ThrdHandle *handle, AMP_ThrAttr attr, AMP_ThrStartRoutine func, 
    AMP_ThrdArg arg, AMP_ThrCreateFlag flag, AMP_ThrdId *thrId)
{
    INT32 status = 0;
    return status;
}
EXPORT_SYMBOL(AMP_CreateThread);


/**
 * Terminate a thread.
 *
 * @param handle - [in] Thread handler.
 * @param thrId - [in] Thread id.
 * @param signal - [in] Signal passed in to the target thread.
 * @param priority - [in] Target thread priority.
 */
INT32 AMP_TerminateThread(AMP_ThrdHandle handle, AMP_ThrdId thrId, INT32 signal, INT32 priority)
{
    INT32 status = 0;
    
    return status;
}
EXPORT_SYMBOL(AMP_TerminateThread);

/**
 * Join in a terminated thread.
 *
 * @param handle - [in] Thread handler.
 * @param thrId - [in] Thread id.
 */
INT32 AMP_WaitThreadFinish(AMP_ThrdHandle handle, AMP_ThrdId thrId)
{
    
}

/**
 * Daemonize a thread.
 *
 * @param name - [in] Thread name.
 * @param noChDir - [in] Flag.
 * @param noClose - [in] Flag.
 */
INT32 AMP_DaemonizeThread(UINT8 name[AMP_MAX_NAME_PATH], INT32 noChDir, INT32 noClose)
{
    INT32 status = 0;
    return status;
}
EXPORT_SYMBOL(AMP_DaemonizeThread);

/**
 * Check if a signal is pending on the thread.
 *
 * @param *thread - [in] Not used.
 */
INT32 AMP_SignalPending(void* thread)
{
    INT32 status = 0;
    return status;
}

EXPORT_SYMBOL(AMP_SignalPending);

/**
 * Reschedule (sleep) on a given time period (second).
 *
 * @param time - [in] Sleep time second.
 */
void AMP_ReSchedule(UINT32 time)
{
    return;
}
EXPORT_SYMBOL(AMP_ReSchedule);


// ---- Timer related ----
/**
 * Get current time stamp.
 *
 * @note The linux kernel use these global variable like jiffies, current to denote a global
 *  object, which to the other platforms, is awkward.
 *  Currently I just put a simple conversion into a windows timer call, need more time to look over it.
 *  Well, I look it over, the catawba component currently use struct timeval and struct timespec, to work
 *  on some timeout function and denote current time, translate to windows world, I will use the CRT time
 *  function directly for a unix sytle time (the value since UTC), looks like it's sufficient now.
 *
 */
AMP_TimerObj AMP_GetCurTime(void)
{
    AMP_TimerObj curTime; 
    return curTime;
}
EXPORT_SYMBOL(AMP_GetCurTime);


/**
 * Convert the input AMP timer object to a time value.
 *
 * @param timeObj - [in] Timer object.
 * @return Time value.
 */
UINT32 AMP_GetTimeValue(AMP_TimerObj timeObj)
{
    UINT32 timeVal; 
    
    return timeVal;
}
EXPORT_SYMBOL(AMP_GetTimeValue);

/**
 * Create a time out object. 
 * How's the time out object formed? It's based on the input timer object + timeOutVal in seconds
 *
 * @param timeObj - [in] Timer object.
 * @param timeOutVal - [in] Time out value.
 * @return Timer object.
 */
AMP_TimeOutObj AMP_CreateTimeOutObj(AMP_TimerObj timeObj, UINT32 timeOutVal)
{
	AMP_TimeOutObj timeOutObj; 

	return timeOutObj;
}

/**
 * Subtract time value. 
 *
 * @param *tvp - [in] Timer object.
 * @param *uvp - [in] Timer object.
 * @param *vvp - [out] Timer object.
 */
void AMP_TimerSub( AMP_TimerObj * tvp, AMP_TimerObj * uvp, AMP_TimerObj * vvp )
{

} 

/**
 * Add on a time value. 
 *
 * @param *tvp - [in] Timer object.
 * @param *uvp - [in] Timer object.
 * @param *vvp - [out] Timer object.
 */
void AMP_TimerAdd( AMP_TimerObj * tvp, AMP_TimerObj * uvp, AMP_TimerObj * vvp )
{

} 

/**
 * Check if the timer is set (timer value > 0).
 *
 * @param *tvp - [in] Timer object.
 */
int AMP_TimerIsSet(AMP_SetTimerObj *tvp)
{

}



// ---- File operation ----

// ---- Environment variable ----

// ---- System Service ----

/**
 * Execute a system service.
 * 
 * @note The unix style system call is a very strange api, personally, I would rather prefer not to have this api,
 *  in stead, go to the more linear way as fork/exec, set up enviroment variable, then to execute. The equvalent
 *  api in Windows is CreateProcess, however, in real world, it's not used like that. Most of the windows cmd
 *  execute in the system is thru direct api call, like service control, object control.
 *  In our catawba environment, for instance, a call to ifup -> system("ifup ..") will be a service control call
 *  in the windows, i.e., ControlService. 
 *  It's still unclear how's the catawba NDIS driver operate, so I leave the windows portion blank. 
 *
 * @param *cmd - [in] System command.
 * @param handle - [in] Service handler.
 * @param code - [in] Code.
 * @param code - [in] Status code.
 */
INT32 AMP_ControlService(UINT8* cmd, AMP_SvcHandle handle, UINT32 code, void* status)
{
    INT32 ret = 0;

    return ret;
}

// ---- Socket Interface ----

/**
 * Start up socket system. Windows ONLY.
 * 
 */
INT32 AMP_SockStartup(void)
{
    INT32 ret = 0;

    // nothing need to be done now
    return ret;
}

/**
 * Shutdown socket system. Windows ONLY.
 * 
 */
INT32 AMP_SockCleanup(void)
{
    INT32 ret = 0;
    // nothing need to be done now

    return ret;
}

/**
 * Create a socket object.
 *
 * @param sockFamily - [in] Socket family.
 * @param sockType - [in] Socket type.
 * @param protocol - [in] Protocol.
 * @return Socket object.
 */
AMP_SOCKET AMP_SockCreate(INT32 sockFamily, INT32 sockType, INT32 protocol)
{
    AMP_SOCKET sockHdl; 

    return sockHdl;
}

/**
 * Accept an incoming socket call.
 *
 * @param sockHdl - [in] Socket handler.
 * @param *addr - [in] Socket address.
 * @param *addrLen - [in] Socket address length.
 * @return Incoming socket object.
 */
AMP_SOCKET AMP_SockAccept(AMP_SOCKET sockHdl, AMP_SOCKADDR *addr, AMP_SOCKLEN *addrLen)
{
    AMP_SOCKET sockFd; 

    return sockFd;
}

/**
 * Bind to an socket address.
 *
 * @param sockHdl - [in] Socket handler.
 * @param *addr - [in] Socket address.
 * @param addrLen - [in] Socket address length.
 */
INT32 AMP_SockBind(AMP_SOCKET sockHdl, AMP_SOCKADDR *addr, AMP_SOCKLEN addrLen)
{
    INT32 status = 0;
    // nothing need to be done now
    return status;
}

/**
 * Listen on an socket.
 *
 * @param sockHdl - [in] Socket handler.
 * @param backlog - [in] Backlog length.
 */
INT32 AMP_SockListen(AMP_SOCKET sockHdl, INT32 backlog)
{
    INT32 status = 0; 

    return status;
}

/**
 * Send a msg buffer to a socket.
 *
 * @param sockHdl - [in] Socket handler.
 * @param *buf - [in] Buffer pointer.
 * @param len - [in] Buffer length.
 * @param flags - [in] Option flags.
 */
INT32 AMP_SockSend(AMP_SOCKET sockHdl, char* buf, INT32 len, INT32 flags)
{
    INT32 byteSent = 0; 

    return byteSent;
}

/**
 * Recv a msg buffer from a socket.
 *
 * @param sockHdl - [in] Socket handler.
 * @param *buf - [in] Input buffer pointer.
 * @param len - [in] Buffer length.
 * @param flags - [in] Option flags.
 */
INT32 AMP_SockRecv(AMP_SOCKET sockHdl, char* buf, INT32 len, INT32 flags)
{
    INT32 byteRecv = 0; 

    return byteRecv;
}


/**
 * Set the timeout on an socket.
 *
 * @param sockHdl - [in] Socket handler.
 * @param ui32Timeout - [in] timeout in milliseconds.
 */
INT32 AMP_SockSetRecvTimeout(AMP_SOCKET sockHdl, UINT32 ui32Timeout)
{
    INT32 status = 0; 

    return status;
}


/**
 * Get the timeout on an socket.
 *
 * @param sockHdl - [in] Socket handler.
 * @param pui32Timeout - [out] timeout in milliseconds.
 */
INT32 AMP_SockGetRecvTimeout(AMP_SOCKET sockHdl, UINT32* pui32Timeout)
{
    INT32 status = 0;
    
    if (!pui32Timeout)
        return -1;
         
    return status;
}


/**
 * Connect to a socket.
 *
 * @param sockHdl - [in] Socket handler.
 * @param *servAddr - [in] Socket address.
 * @param addrLen - [in] Socket address length.
 */
INT32 AMP_SockConnect(AMP_SOCKET sockHdl, AMP_SOCKADDR *servAddr, AMP_SOCKLEN addrLen)
{
    INT32 status = 0; 

    return status;
}

/**
 * Shutdown a socket connection.
 *
 * @param sockHdl - [in] Socket handler.
 * @param how - [in] Optional method.
 */
INT32 AMP_SockShutdown(AMP_SOCKET sockHdl, INT32 how)
{
    INT32 status = 0; 

    return status;
}

/**
 * Close a socket connection.
 *
 * @param sockHdl - [in] Socket handler.
 */
INT32 AMP_SockClose(AMP_SOCKET sockHdl)
{
    INT32 status = 0; 

    return status;
}

/**
 * Create SME socket interface.
 *
 * @param *addr - [in] Socket address.
 * @return SME socket handler.
 */
AMP_SOCKET AMP_SockSMECreate(AMP_SME_SOCKADDR  *addr)
{
    AMP_SOCKET sockHdl;
    sockHdl = socket(PF_LOCAL, SOCK_STREAM, 0);

    memset( addr, 0, sizeof(struct sockaddr_un) );
    addr->sun_family = PF_LOCAL;
    strncpy( addr->sun_path, AMP_SME_LOCAL_WIMAX_SOCKET_PATH, strlen(AMP_SME_LOCAL_WIMAX_SOCKET_PATH));
    
    return sockHdl;
}


/**
 * Set socket blocking option.
 *
 * @param n32Fd - [in] Socket file handler.
 * @param b32Block - [in] Blocking mode.
 */
INT32 AMP_SockSetBlock(AMP_SOCKET n32Fd, INT32 b32Block)
{
    INT32 status = 0;

    return status;
}


/////////////////////////////////////////////////////////////////////////////
// @brief Returns the base path (i.e. full executable path less executable)
//
// @param[in]  buffLen  Length of pathBuff
// @param[out] pathBuff Buffer filled with the path name, including trailing
//                      delimiter
// @param[out] pathLen  Length of parsed path (not including NULL)
//
// @return Result of path retrieval.
/////////////////////////////////////////////////////////////////////////////
AMP_RET_CODE_E AMP_GetRootPath(TCHAR *pathBuff, INT32 buffLen, INT32 *pathLen)
{
    TCHAR *curToken, *prevToken, *delim=PATHDELIM;
        char tmpPath[20];

    if (snprintf(tmpPath, sizeof(tmpPath), "/proc/%d/exe", getpid()) >= 
                    sizeof(tmpPath))
    {
        return eAMP_RET_BUF_OUTOFBUF;
    }

    if (readlink(tmpPath, pathBuff, buffLen) <= 0)
    {
        return eAMP_RET_BUF_OUTOFBUF;
    }

    // Search for the last "\".  strtok is too destructive.
    curToken = pathBuff;
    prevToken = NULL;
    while (*curToken != '\0')
    {
        if (*delim == *curToken)
        {
            prevToken = curToken;
        }
        curToken++;
    }

    // See if we found a proper path
    if (NULL != prevToken)
    {
        // Get rid of everything past the last delimiter
        *prevToken = '\0';
        *pathLen = (INT32)(prevToken - pathBuff);
        return eAMP_RET_SUCCESS;
    }
    else
    {
        return eAMP_RET_FAIL;
    }
    // Find our location
    return eAMP_RET_FAIL;
}


/////////////////////////////////////////////////////////////////////////////
// @brief The timer function used for the AMP_CreateTimerFunction and 
// AMP_KillTimerFunction functions.  Will sleep and then invoke the
// specified callback.  If the callback returns non-zero, the thread
// function will terminate regardless of u32SingleShot
//
// @param[in]  pThreadData  Pointer to AMP_TimerFunctionObj struct
//
// @return void*
/////////////////////////////////////////////////////////////////////////////
static void* stWiMaxTimerFunctionThread(void* pThreadData)
{
	AMP_TimerFunctionObj* pTimerFunctionObj = (AMP_TimerFunctionObj*) pThreadData;
	INT32 i32RetCode = 0;


	while (1)
	{
	    if (!pTimerFunctionObj)
	        break;
	        
        msleep(pTimerFunctionObj->u32TimeOutMS);
	
	    // invoke the callback, if return is non-zero then terminate
	    if (pTimerFunctionObj->pfTimerFunctionCallback)
	        i32RetCode = pTimerFunctionObj->pfTimerFunctionCallback(pTimerFunctionObj->pvUserData);
	    
	    // if single shot or callback returned non-zero, then terminate
	    if (pTimerFunctionObj->u32SingleShot || i32RetCode)
	        break;
	
	}
	
	if (pTimerFunctionObj)
	{
	    pTimerFunctionObj->i32RetCode = i32RetCode;
        pTimerFunctionObj->sThrHandle = 0;
        pTimerFunctionObj->sThrId = 0;
        pTimerFunctionObj->u32Initialized = 0;
        pTimerFunctionObj->u32Expired = 1;
	}
	
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// @brief Used to create a single shot or repeating timer in milliseonds 
//
// @param[in]  pTimerFunctionObj  Pointer to function control structure
//
// @return AMP_RET_CODE_E - amp return code
/////////////////////////////////////////////////////////////////////////////
EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_CreateTimerFunction(
    AMP_TimerFunctionObj* pTimerFunctionObj)
{
    AMP_RET_CODE_E eStatus = eAMP_RET_SUCCESS;


    // pointer check
    if (!(pTimerFunctionObj))
        return eAMP_RET_FAIL;

    // must have a callback funtion        
    if (!(pTimerFunctionObj->pfTimerFunctionCallback))
        return eAMP_RET_FAIL; 

    // initalize our private members
    pTimerFunctionObj->sThrHandle = 0;
    pTimerFunctionObj->sThrId = 0;
    pTimerFunctionObj->u32Initialized = 1;
    pTimerFunctionObj->u32Expired = 0;
    
	// create a timer thread
    AMP_CreateThread(&(pTimerFunctionObj->sThrHandle), NULL,
        (AMP_ThrStartRoutine) stWiMaxTimerFunctionThread, pTimerFunctionObj, 
		NULL, &(pTimerFunctionObj->sThrId));

    return eStatus;
}

/////////////////////////////////////////////////////////////////////////////
// @brief Used to cancel the timer created by AMP_CreateTimerFunction
//
// @param[in]  pTimerFunctionObj  Pointer to function control structure
//
// @return AMP_RET_CODE_E - amp return code
/////////////////////////////////////////////////////////////////////////////
EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_KillTimerFunction(
    AMP_TimerFunctionObj* pTimerFunctionObj)
{
    AMP_RET_CODE_E eStatus = eAMP_RET_SUCCESS;


    // pointer check
    if (!(pTimerFunctionObj))
        return eAMP_RET_FAIL;
        
    if (pTimerFunctionObj->sThrHandle || pTimerFunctionObj->sThrId)
    {
        AMP_TerminateThread(pTimerFunctionObj->sThrHandle, pTimerFunctionObj->sThrId, 0, 0); 
        AMP_WaitThreadFinish(pTimerFunctionObj->sThrHandle, pTimerFunctionObj->sThrId);
    }

    pTimerFunctionObj->sThrHandle = 0;
    pTimerFunctionObj->sThrId = 0;
    pTimerFunctionObj->u32Initialized = 0;
    pTimerFunctionObj->u32Expired = 1;

    return eStatus;
}


// ---- Trace, Assert ----

// ---- Misc ----

