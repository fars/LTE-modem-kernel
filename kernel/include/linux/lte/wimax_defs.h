/*
 * Copyright Motorola, Inc. 2007
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
 *  @File:  wimax_defs.h
 *
 *  This file contains some globle const. definitions for MOT 802.16.
 *  It will be used both in applications of kernel space and user space.
 *
 * Last Changed Revision: $Rev: 3170 $
 * Last Changed Date: $LastChangedDate: 2009-06-25 16:47:30 -0500 (Thu, 25 Jun 2009) $
 *
 ***********************************************************************
 */

#ifndef _MOT_WiMAX_DEFS_H_
#define _MOT_WiMAX_DEFS_H_

#if !defined(WIN32) && !defined(HOST_EMULATION)
#include <linux/ioctl.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#include <linux/if.h>
#endif
#include <linux/types.h>
#include <linux/wireless.h>
#endif

#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_global.h>
#include <linux/lte/AMP_general.h>

/* This will defined whether the wimax wireless extension merges with WIFI */
#define WIMAX_WIFI_WIRELESS_EXT

/* Constants needed for the Driver. */
#ifndef ETH_ALEN
    #define ETH_ALEN    6
#endif

/* The Main thread only process 5 IPC messages at a time */
#define NUMBER_CMD_PER_PROCESS  5

/* This definition is used to define the network interface name used for WiMAX. */
#ifdef CONFIG_LTE
#define AMP_WIMAX_IFC_NAME      "lte0"
#else
#define AMP_WIMAX_IFC_NAME      "wip0"
#endif
/**
 * Following constants are used for configurations.
 */
#ifdef CONFIG_LTE
// No host MOIDs currently defined for LTE
// LTE uses first 5 bits for module ID followed by gap of 1 bits
// Use first 5 bits for module ID .. if bit 10 is set its a host moid
#define AP_OBJECT_MASK     0x0400
// Bit 15 to 12 for module ID in AP
#define AP_OBJECT_ID_MASK  0xf000 
#else
/** used for determining whether is a WiP object or AP object.*/
#define AP_OBJECT_MASK     0x8000
/** bit 14 to 10 is the Module ID in AP */
#define AP_OBJECT_ID_MASK  0x7C00
#endif
/** The lower byte is for the Message subtype for the GetObject/SetObject. */
#define MSG_ID_MASK   0x00ff
/** The higher byte is for the number of OIDs in this command. */
#define NUMBER_OF_OIDS 0xff00
/** The offset of configuration message IDs for IPC */
#define AMP_IPC_MSG_CONFIG_OFFSET   (eAMP_IPC_MSG_CONFIG_BASE)

/**
 * @struct wimax_test_param_t
 * @brief  wimax_test parameter struct.
 *
 * This structure is used to pass in parameters from the test application
 * to the Wimax Network Driver
 */
typedef struct
{
    char *cmd_str;  /**< Contains the command / IOCTL to execute */
    long level;
} wimax_test_param_t;

/* Use this define to enable MAC2MAC testing code. */
//#define MAC2MAC_TESTING     1

#ifdef MAC2MAC_TESTING
    #define CONVERGENCE_SUB_TYPE_IPV4_ONLY  (0x01)

typedef struct _MAC2MAC_CFG_SETTING_S
{
    INT8 u8PcMacArray[6];
    INT8 u8MsMacArray[6];
    INT8 u8BsMacArray[6];
    INT8 u8ConvergSubType;
} MAC2MAC_CFG_SETTING_S;
#endif /* MAC2MAC_TESTING */

/**
 * @enum AMP_NETWORK_DRIVER_CALLBACK_E
 * The clients supported in Network driver.
 */
typedef enum
{
    eSTART_NET_DR_CLIENT_AUTHORIZATION = 0, ///< Security module
    eSTART_NET_DR_CLIENT_QOSA = 1,          ///< QoS Agent module
    eSTART_NET_DR_CLIENT_ROAMINGA = 2,      ///< Roaming Agent module
    eSTART_NET_DR_CLIENT_PWRM = 3,          ///< Power amangement module
    eSTART_NET_DR_CLIENT_CONFIG = 4,        ///< configuration module
    eSTART_NET_DR_CLIENT_DIAGNOSTICS = 5,   ///< debug & diagnostics module
    eNET_DR_GET_OFFSET_FROM_QOS = 128,      ///< get offset from QOS agent for TX
    eNET_DR_GET_HEADERINFO_FROM_QOSA = 129, ///< message betwen QOS & Network Dr. RX
} AMP_NETWORK_DRIVER_CALLBACK_E;

/**
 * @struct IWA_BSID_S
 * @brief Network Id defined from COCO.
 */
typedef struct
{
    UINT32 u32NetworkId;
    UINT32 u32BaseStationId;
} IWA_BSID_S;

/**
 * The bit map indication for each module that Network driver can start
 */
#define START_NET_DR_CLIENT_AUTHORIZATION  1 ///< start Security module
#define START_NET_DR_CLIENT_QOSA       2     ///< start QoS Agent module
#define START_NET_DR_CLIENT_ROAMINGA   4     ///< start Roaming Agent module
#define START_NET_DR_CLIENT_PWRM       8     ///< start Power amangement module
#define START_NET_DR_CLIENT_CONFIG    16     ///< start configuration module
#define START_NET_DR_CLIENT_DIAGNOSTICS 32   ///< start debug & diagnostics module

/** @enum AMP_NETWORK_DRIVER_CONTROL_E
 * Information passed through SIOCSIWACLIENTSTART to tell Network driver to
 * start, reset or shutdown
 */
typedef enum
{
    eAMP_NET_DR_START = 0, ///< start network driver submodule
    eAMP_NET_DR_SHUTDOWN = 1,          ///< shutdown network driver submodule
    eAMP_NET_DR_RESET = 2,      ///< reset network driver submodule
} AMP_NETWORK_DRIVER_CONTROL_E;

/** @enum AMP_NETWORK_DRIVER_SHUTDOWN_E
 * Information passed through disconnect callback to tell Network driver to
 * start, reset or shutdown
 */
typedef enum
{
    eAMP_NET_DR_GRACEFUL = 0,
    eAMP_NET_DR_NOW = 1,
    eAMP_NET_DR_HARD = 2,
} AMP_NETWORK_DRIVER_SHUTDOWN_E;

/** @AMP_NETWORK_DRIVER_STARTMODE_AUTO
 * when this bit is set, the submodule will be started in auto mode. Otherwise
 * the submodule will be started in manual mode. Currently it is only used by
 * Roaming Agent. In auto mode, Roaming Agent will start network scan and entry
 * automatically based on configuration, it will also handle HANDOVER
 * automatically when configurated and apply to the situation.
 */
#define AMP_NETWORK_DRIVER_STARTMODE_AUTO (1)
//#define START_NET_DR_CLIENT_RESETMODE_SOFT (2)        // JLI: do we need this?
//#define START_NET_DR_CLIENT_RESETMODE_HARD (4)

/**
 * @struct AMP_WIMAX_DRIVER_START_FLAG_S
 * @brief The paramter used to start, stop, or reset the Network clients.
 */
typedef struct _START_FLAG
{
    UINT8 u8Control;        ///< AMP_NETWORK_DRIVER_CONTROL_E
    UINT8 u8Mode;          ///< AMP_NETWORK_DRIVER_STARTMODE_AUTO for now
    UINT8 u8Reserved[2];
} AMP_WIMAX_DRIVER_START_FLAG_S;

#endif /* _MOT_WiMAX_DEFS_H_ */
