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
 * @file include/asm-arm/arch-appsliver/AMP_ipc.h
 *
 * This file contains the user space interface for the AMP IPC implementation.
 *
 * Last Changed Revision: $Rev: 3204 $
 * Last Changed Date: $LastChangedDate: 2009-07-07 17:53:05 -0500 (Tue, 07 Jul 2009) $
 */
#ifndef __AMP_IPC_H__
#define __AMP_IPC_H__

#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_global.h>
#include <linux/lte/AMP_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMP_IPC_MAX_NODE    (40)        // for S_AMP_IPC_MSG address id
#define AMP_IPC_ADDR_USER_START         ((AMP_MAX_IPC_NODE) - 10)       // We need to agree on a number, 10 user node for now

/**
 * For user mode
 */
#define AMP_IPC_ADDR_USER       (0x8000)

/**
 * For kernel mode
 */
#define AMP_IPC_ADDR_KERNEL     (0x4000)

/**
 * For cross processor
 */
#define AMP_IPC_ADDR_MP         (0x2000)

/**
 * The right 12 bits should have a value less than AMP_MAX_IPC_NODE and be unique.
 */

/**
 * Message will be delivered to every node, not available yet.
 */
#define AMP_IPC_ADDR_BROADCAST  (0xFFFF)

/**
 * Message will be delivered to multiple nodes which registeres on a particular
 * message, not available yet.
 */
#define AMP_IPC_ADDR_REGISTED   (0x0000)
//#define AMP_IPC_ADDR_WIMAX_RA (0x8001)            // if moved to user space
//#define AMP_IPC_ADDR_WIMAX_QA (0x8002)            // if moved to user space
/**
 * For Kernel space node, 0x8001 if moved to user space.
 */
#define AMP_IPC_ADDR_WIMAX_RA   (0x4001)

/**
 * 0x8002 if moved to user space.
 */
#define AMP_IPC_ADDR_WIMAX_QA   (0x4002)
/**
 * For user space WiMAX SP
 */
#define AMP_IPC_ADDR_WIMAX_SP   (0x8003)

/**
 * For security supplicant
 */
#define AMP_IPC_ADDR_SUPPLICANT (0x8004)

/**
 * For temporary test.
 */
#define AMP_IPC_ADDR_WIMAX_SP_TEST  (0x8005)

#define AMP_IPC_ADDR_WIMAX_DRIVER   (0x4006)

/**
 * MAC simulator as a kernel model.
 */
#define AMP_IPC_ADDR_WIMAX_MAC_SIMULATOR    (0x4007)

/**
 * For cross processor node
 */
#define AMP_IPC_ADDR_CDMA       (0x2008)

/**
 * From the logging point of view, ipc module itself is a component.
 */
#define AMP_IPC_ADDR_IPC    (0x4009)

/**
 * HiHal
 */
#define AMP_IPC_ADDR_HIHAL  (0x400A)

/**
 * Host interface driver.
 */
#define AMP_IPC_ADDR_HID    (0x400B)

#define AMP_IPC_ADDR_WIMAX_MAC  (0x400C)

/**
 * Send Monitor Mode through IPC.
 */
#define AMP_IPC_ADDR_WIMAX_MONITOR_MODE (0x800D)

/**
 * Capture PMDT log through IPC.
 */
#define AMP_IPC_ADDR_WIMAX_PMDT (0x800E)

#ifdef CONFIG_LTE
/**
 * Sending Triggers to the driver to send ABS TIME to the LTE CORE MODEM
 */
#define AMP_IPC_ADDR_LTE_SYNC_APP (0x800F)

/**
 * Interface management for the LTE interface
 */
#define AMP_IPC_ADDR_LTE_IFC_MGMT_APP (0x8010)
/**
 * VPHY Proxy Application for LTE 
 */
#define AMP_IPC_ADDR_LTE_VPHY_PROXY_APP (0x8011)

/**
 * Increase this number after you add in new component name/addr.
 */
#define AMP_IPC_MAX_COMPONENT   17
#else
#define AMP_IPC_MAX_COMPONENT   14
#endif

// When the mask high bit is set, the message subtype is private and does not need to be unique acrossing nodes
// Otherwise, the message subtypes are global which can be registed by multiple nodes
#define AMP_IPC_MSG_PRIVATE_MASK        (0x8000)        // for future support

typedef enum
{
    eAMP_IPC_MSG_INVALID = -1,
    eAMP_IPC_MSG_FAIL_REPORT = 0,
    eAMP_IPC_MSG_REG = 1,
    eAMP_IPC_MSG_RESET = 2,
    eAMP_IPC_MSG_SHUTDOWN = 3,
    eAMP_IPC_MSG_START = 4,
    eAMP_IPC_MSG_UNREG = 5,
    eAMP_IPC_MSG_FROM_MAC = 6,
    eAMP_IPC_MSG_TO_MAC = 7,
    eAMP_IPC_MSG_TIMEOUT = 8,
    eAMP_IPC_MSG_SET_LOGLEVEL = 9,
    eAMP_IPC_MSG_QA_READY = 10,             // QA to RA
    eAMP_IPC_MSG_QA_NO_DEFAULT_SF = 11,             // service flow was all deleted
    eAMP_IPC_MSG_QA_DEFAULT_SF_SET = 12,            // service flow was resumed
    eAMP_IPC_MSG_CONFIG_BASE = 0x0040,
    eAMP_IPC_MSG_GET = 0x0041,           // Regular Get, start from 0x0040, it is hard coded in wimax_dev.h as #define AMP_IPC_MSG_CONFIG_OFFSET   0x40
    eAMP_IPC_MSG_GET1ST = 0x0042,       // Get_first
    eAMP_IPC_MSG_GETNEXT = 0x0043,      // Get_next
    eAMP_IPC_MSG_SET = 0x0044,          // SET
    eAMP_IPC_MSG_RESPONSE = 0x0045,     //Response back
    eAMP_IPC_MSG_IFC_LINKDN = 0x0046,   // RA to driver
    eAMP_IPC_MSG_IFC_LINKUP = 0x0047,   // RA to driver
    eAMP_IPC_MSG_SET_IP_ADDR = 0x0048,  // driver to QA
#if defined(WIN32) || defined(HOST_EMULATION)
    eAMP_IPC_MSG_DRIVERCTL_REQ  = 0x0049,       // ioctl replacement
    eAMP_IPC_MSG_DRIVERCTL_RSP  = 0x004A,       // ioctl replacement
#endif
    eAMP_IPC_MSG_PMDT = 0x0100,         // Network driver to PMDT application
#ifdef CONFIG_LTE
    eAMP_IPC_MSG_LTE_SYNC = 0x0200,                   // LTE SYNC APP to Network driver application
    eAMP_IPC_MSG_LTE_INTERFACE_MGMT = 0x0300,         // Network driver to LTE Interface Mgmt application
	eAMP_IPC_MSG_LTE_VPHY_PROXY = 0x0400,			  // Network Driver to VPHY Proxy APP
#endif
    eAMP_IPC_MSG_MAX_GLOBAL = 0x7FFF,
    eAMP_IPC_MSG_MIN_PRIVATE = 0x8000,
    eAMP_IPC_MSG_MAX_VAL = 0xFFFF
} AMP_IPC_MSG_TYPE_E;

typedef struct
{
    UINT32 u16AddrIdSrc:16;     // This servers as queue ID
    UINT32 u16AddrIdDest:16;    // This servers as queue ID, used to be u16AddrId
    UINT32 u8Priority:8;        // for future support
    UINT32 u8Reserved:8;        // not used
    UINT32 u16SeqNum:16;        // seq num
                                //
    UINT32 u32Context;          // associate replay to request, only the lower 16 bits in local endian is passed to MAC
                                // QoS Agent will enforce the lower 16 bits in range defined by Layer 2 ICD R01.00.05
                                // The MS shall select transaction IDs from 0x0000 to 0x7FFF
                                // The BS shall select transaction IDs from 0x8000 to 0xFFFF
    UINT32 u16MsgId:16;         // This is actually the message type
    UINT32 u16Length:16;        // The length for the payload only
} AMP_IPC_MSG_HDR_S;

typedef struct
{
    AMP_IPC_MSG_HDR_S sHdr;
    UINT8 u8Data[1];
} AMP_IPC_MSG_S;

/**
 * Used for netlink address (16 bytes) or MAC-host interface header, only one
 * layer of preheader for now
 */
#define AMP_IPC_PRE_HEADER_SIZE (16)            // used for netlink address (16 bytes) or MAC-host interface header, only one layer of preheader for now
#define AMP_IPC_HEADER_SIZE (sizeof(AMP_HOST_MAC_MSG_HDR_S))
#define AMP_IPC_TOTAL_HEADER_SIZE   ((AMP_IPC_PRE_HEADER_SIZE) + (AMP_IPC_HEADER_SIZE))
//#define AMP_IPC_MAX_PAYLOAD_SIZE  (MAX((MAX_WIMAX_CONTROL_PACKET_SIZE), 1504))        // if VLAN tag is included
#define AMP_IPC_MAX_PAYLOAD_SIZE    (AMP_WIMAX_PACKET_SIZE)
#define AMP_IPC_MAX_MSG_SIZE    ((AMP_IPC_TOTAL_HEADER_SIZE) + (AMP_IPC_MAX_PAYLOAD_SIZE))

typedef struct
{
    UINT32 u16AddrId:16;    // The reporting node
    UINT32 u16Pad:16;
    UINT32 u32Code;
} AMP_IPC_MSG_FAIL_REPORT_S;

typedef struct
{
    UINT8 u8RaMode;
    UINT8 u8Pad[3];
} AMP_IPC_MSG_RESET_S;

typedef struct
{
    UINT8 u8RaMode;
    UINT8 u8Pad[3];
} AMP_IPC_MSG_START_S;

typedef struct
{
    UINT8 u8RaMode;
    UINT8 u8Now;
    UINT8 u8Pad[2];
} AMP_IPC_MSG_SHUTDOWN_S;

typedef struct
{
    UINT32 u16AddrId:16;
    UINT32 u16MsgId:16;             // to register on a message
    UINT32 u32MailBox;              // for initialize mail box in user space AMP_IPC_Init
} AMP_IPC_MSG_REGISTER_S;

typedef struct
{
    UINT32 u16AddrId:16;
    UINT32 u16MsgId:16;
} AMP_IPC_MSG_UNREGISTER_S;

typedef struct
{
    UINT32 u32TimeStamp;        // in jiffies
    UINT32 u16TimerType:8;
    UINT32 u24Pad:24;
} AMP_IPC_MSG_TIMEOUT_S;

typedef struct
{
    UINT32 u8Owner:8;    // owner and memory alloc flags, refer to AMP_BUF_OWNER_E
    UINT32 reserved:24;
} AMP_IPC_EXT_ATTR_S;

#ifdef HOST_EMULATION
typedef struct {
    UINT32 flags:16;
    UINT32 size:16;
    UINT32 total;
    UINT8  dlData[1];
} AMP_IPC_MSG_DWNLD_S;

// some bits for the download flags:
#define DWNLD_START_MASK 0x0001
#define DWNLD_END_MASK   0x0002

#endif // HOST_EMULATION

typedef AMP_RET_CODE_E (*pf_IPC_CALLBACK)(AMP_IPC_MSG_S * psMsg);

extern EXPORTDLL void EXPORTCDECL AMP_Library_Init(void);
extern EXPORTDLL void EXPORTCDECL AMP_Library_Uninit(void);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_Init (UINT16 u16MyAddrId, void * pMailBox);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_Uninit (UINT16 u16MyAddrId);
//AMP_RET_CODE_E AMP_IPC_Start (UINT16 u16MyAddrId);        // not sure if we want IPC has its own task, or we can put this optional
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_Register (UINT16 u16PeerAddrId, UINT16 u16MsgId, pf_IPC_CALLBACK pfCallback);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_Unregister (UINT16 u16PeerAddrId, UINT16 u16MsgId);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_GetMsg (UINT16 u16MyAddrId, AMP_IPC_MSG_S ** ppMsg, INT32 n32Timeout);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_PutMsg (UINT16 u16MyAddrId, AMP_IPC_MSG_S * pMsg);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_Select (UINT16 u16MyAddrId, UINT32 u32Timeout);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_GetBuf (UINT32 u32Length, void ** ppBuf);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_FreeBuf (void * pBuf);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_AddRef (void * pBuf);
extern EXPORTDLL AMP_RET_CODE_E EXPORTCDECL AMP_IPC_Dump (AMP_IPC_MSG_S * psIpcMsg);

extern AMP_RET_CODE_E AMP_IPC_PutMsgExt (AMP_IPC_EXT_ATTR_S * pExtAttr, AMP_IPC_MSG_S * pMsg);
extern AMP_RET_CODE_E AMP_IPC_GetBufExt (AMP_IPC_EXT_ATTR_S * pExtAttr, UINT32 u32Length, void ** ppBuf);

#ifdef __KERNEL__
// implementation for kernel only
typedef enum
{
    eAMP_IPC_WAKEUP_INVALID = -1,
    eAMP_IPC_WAKEUP_TASK = 0,
    eAMP_IPC_WAKEUP_QUEUE,
    eAMP_IPC_WAKEUP_MAX_VAL
} AMP_IPC_WAKEUP_TYPE_E;
extern AMP_RET_CODE_E AMP_IPC_RegWakeup (UINT16 u16MyAddrId, AMP_IPC_WAKEUP_TYPE_E eWakeupType, void * pBuf);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __AMP_IPC_H__ */


