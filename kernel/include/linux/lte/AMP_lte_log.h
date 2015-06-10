/*
 * Copyright Motorola, Inc. 2005 - 2008
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


#ifndef _AMP_LTE_LOG_INC_
#define _AMP_LTE_LOG_INC_

#ifndef __KERNEL__
#include <syslog.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


// NOTICE:  This code needs to be modified to be more like the original AMP_LOG code
// where it uses a table and the table lives in one module. For now, we will include
// a static table in this header file


// mapping on log level
#define AMP_LOG_LEVEL_NONE     -1
#define AMP_LOG_LEVEL_ERROR     2
#define AMP_LOG_LEVEL_WARNING   3
#define AMP_LOG_LEVEL_DEBUG1    4
#define AMP_LOG_LEVEL_DEBUG2    5
#define AMP_LOG_LEVEL_DEBUG3    6


// log addr id
#define AMP_LOG_ADDR_LTE_DATA_DRIVER        0
#define AMP_LOG_ADDR_LTE_NET_DRIVER         1
#define AMP_LOG_ADDR_LTE_SYNC_APP           2
#define AMP_LOG_ADDR_LTE_IFC_MGMT           3
#define AMP_LOG_ADDR_LTE_VPHY_APP           4
#define AMP_LOG_ADDR_LTE_HIFD_APP           5
#define AMP_LOG_ADDR_WIMAX_QA               6
#define AMP_LOG_MAX_COMPONENT               7

typedef struct
{
    char                u8AddrName[128];
    unsigned short      u16AddrId;
    short               n16LogLevel;
    
} AMP_LOG_S;


// this is very bad, don't include data in headers. this logging will eventually be
// replaced so we will live with it for now.  there is no way to dynamically change
// these settings, a re-compile is necessary
static AMP_LOG_S gsLogTable[AMP_LOG_MAX_COMPONENT] =
{
    {"LTE_DATA_DRIVER",     AMP_LOG_ADDR_LTE_DATA_DRIVER,       AMP_LOG_LEVEL_WARNING},
    {"LTE_NET_DRIVER",      AMP_LOG_ADDR_LTE_NET_DRIVER,        AMP_LOG_LEVEL_WARNING},
    {"LTE_SYNC_APP",        AMP_LOG_ADDR_LTE_SYNC_APP,          AMP_LOG_LEVEL_DEBUG3},
    {"LTE_IFC_MGMT",        AMP_LOG_ADDR_LTE_IFC_MGMT,          AMP_LOG_LEVEL_WARNING},
    {"LTE_VPHY_APP",        AMP_LOG_ADDR_LTE_VPHY_APP,          AMP_LOG_LEVEL_DEBUG3},
    {"LTE_HIFD_APP",        AMP_LOG_ADDR_LTE_HIFD_APP,          AMP_LOG_LEVEL_DEBUG3}, 
    {"LTE_QA_DRIVER", 	    AMP_LOG_ADDR_WIMAX_QA,              AMP_LOG_LEVEL_WARNING} 		    
};


#ifndef __KERNEL__
// user land logging
#define AMP_LOG_Error(fmt, args...) syslog(LOG_ERR, "ERROR: " fmt, ## args)
#define AMP_LOG_Info(fmt, args...)  syslog(LOG_INFO,"INFO: "  fmt, ## args)
// currently, there is no syslog in android on cedar point hardware 
#if defined (TARGET_OS) && (#TARGET_OS==android)
#define AMP_LOG_Print(addrId, logLevel, format, args...)\
do{\
    char* name;\
    name = (addrId < 0 || addrId >= AMP_LOG_MAX_COMPONENT) ? "INVALID LOG ADDRESS" : gsLogTable[addrId].u8AddrName;\
    if (gsLogTable[addrId].n16LogLevel >= logLevel) {\
        printf("%d:%s:" format, logLevel, name, ## args);\
    }\
} while (0)
#else
#define AMP_LOG_Print(addrId, logLevel, format, args...)\
do{\
    char* name;\
    name = (addrId < 0 || addrId >= AMP_LOG_MAX_COMPONENT) ? "INVALID LOG ADDRESS" : gsLogTable[addrId].u8AddrName;\
    if (gsLogTable[addrId].n16LogLevel >= logLevel) {\
        syslog(LOG_INFO, "%d:%s:" format, logLevel, name, ## args);\
    }\
} while (0)
#endif
#else
// kernel space logging
#define AMP_LOG_Error(fmt, args...) printk("LTE ERROR: " fmt, ## args)
#define AMP_LOG_Info(fmt, args...) printk("LTE INFO: " fmt, ## args)
#if defined (AMP_LOG_ENABLE_LOG_PRINT)
#define AMP_LOG_Print(addrId, logLevel, format, args...)\
do{\
    char* name;\
    name = (addrId < 0 || addrId >= AMP_LOG_MAX_COMPONENT) ? "INVALID LOG ADDRESS" : gsLogTable[addrId].u8AddrName;\
    if (gsLogTable[addrId].n16LogLevel >= logLevel) {\
        printk("%d:%s:" format, logLevel, name, ## args);\
    }\
} while (0)
#else
#define AMP_LOG_Print(addrId, logLevel, format, args...)
#endif // INCLUDE_AP_LTE_LOGGING
#endif

// set the log level on the fly
#define AMP_LOG_SetLogLevel(addrId, logLevel)\
do{\
    if (addrId >= 0 && addrId < AMP_LOG_MAX_COMPONENT){\
        gsLogTable[addrId].n16LogLevel = (short) logLevel;\
    }\
} while (0)

// get the log level on the fly
#define AMP_LOG_GetLogLevel(addrId, logLevel)\
do{\
    if (addrId >= 0 && addrId < AMP_LOG_MAX_COMPONENT){\
        logLevel = gsLogTable[addrId].n16LogLevel;\
    }\
} while (0)

#ifdef __cplusplus
}
#endif

#endif  //_AMP_LTE_LOG_INC_


