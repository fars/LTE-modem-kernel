/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#ifndef __PVR_DEBUG_H__
#define __PVR_DEBUG_H__


#include "img_types.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define PVR_MAX_DEBUG_MESSAGE_LEN	(512)

#define DBGPRIV_FATAL		0x01UL
#define DBGPRIV_ERROR		0x02UL
#define DBGPRIV_WARNING		0x04UL
#define DBGPRIV_MESSAGE		0x08UL
#define DBGPRIV_VERBOSE		0x10UL
#define DBGPRIV_CALLTRACE	0x20UL
#define DBGPRIV_ALLOC		0x40UL
#define DBGPRIV_ALLLEVELS	(DBGPRIV_FATAL | DBGPRIV_ERROR | DBGPRIV_WARNING | DBGPRIV_MESSAGE | DBGPRIV_VERBOSE)



#define PVR_DBG_FATAL		DBGPRIV_FATAL,__FILE__, __LINE__
#define PVR_DBG_ERROR		DBGPRIV_ERROR,__FILE__, __LINE__
#define PVR_DBG_WARNING		DBGPRIV_WARNING,__FILE__, __LINE__
#define PVR_DBG_MESSAGE		DBGPRIV_MESSAGE,__FILE__, __LINE__
#define PVR_DBG_VERBOSE		DBGPRIV_VERBOSE,__FILE__, __LINE__
#define PVR_DBG_CALLTRACE	DBGPRIV_CALLTRACE,__FILE__, __LINE__
#define PVR_DBG_ALLOC		DBGPRIV_ALLOC,__FILE__, __LINE__

#if defined(DEBUG)
	#define PVR_ASSERT(EXPR) if (!(EXPR)) PVRSRVDebugAssertFail(__FILE__, __LINE__);	
	#define PVR_DPF(X)		PVRSRVDebugPrintf X
	#define PVR_TRACE(X)	PVRSRVTrace X

IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVDebugAssertFail(const IMG_CHAR *pszFile,
									IMG_UINT32 ui32Line);

IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVDebugPrintf(IMG_UINT32 ui32DebugLevel,
									const IMG_CHAR *pszFileName,
									IMG_UINT32 ui32Line,
									const IMG_CHAR *pszFormat,
									...);

IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVTrace(const IMG_CHAR* pszFormat, ... );

IMG_VOID PVRDebugSetLevel (IMG_UINT32 uDebugLevel);

		#if defined(PVR_DBG_BREAK_ASSERT_FAIL)
			#define PVR_DBG_BREAK	PVRSRVDebugAssertFail("PVR_DBG_BREAK", 0)
		#else
			#define PVR_DBG_BREAK
		#endif

#else

#if defined(TIMING)

	#define PVR_ASSERT(EXPR)
	#define PVR_DPF(X)
	#define PVR_TRACE(X)	PVRSRVTrace X
	#define PVR_DBG_BREAK

IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVTrace(const IMG_CHAR* pszFormat, ... );

#else
IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVDebugAssertFail(const IMG_CHAR *pszFile,
									IMG_UINT32 ui32Line);
IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVDebugPrintf(IMG_UINT32 ui32DebugLevel,
						   const IMG_CHAR *pszFileName,
						   IMG_UINT32 ui32Line,
						   const IMG_CHAR *pszFormat,
						   ...);
#ifdef CONFIG_SGX_RELEASE_LOGGING
	#define PVR_ASSERT(EXPR) if (!(EXPR)) PVRSRVDebugAssertFail(__FILE__, __LINE__);
	#define PVR_DPF(X) PVRSRVDebugPrintf X
#else
	#define PVR_ASSERT(EXPR)
	#define PVR_DPF(X)
#endif
	#define PVR_TRACE(X)
	#define PVR_DBG_BREAK

#endif 
#endif 


#if defined (__cplusplus)
}
#endif

#endif	

