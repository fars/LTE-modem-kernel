/*
 * Copyright Motorola, Inc. 2005-2006 
 * 
 * This Program is distributed in the hope that it will 
 * be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of 
 * MERCHANTIBILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * This program is free software; you can redistribute it 
 * and/or modify it under the terms of the GNU General 
 * Public License as published by the Free Software 
 * Foundation; either version 2 of the License, or (at 
 * your option) any later version.  You should have 
 * received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free 
 * Software Foundation, Inc., 675 Mass Ave, 
 *  Cambridge, MA 02139, USA 
 */
/**
 *  @File:  mac_msg.h
 *  
 *  This file contains some const. & data structures used for both
 *  AP and Wimax processor. It is key to keep the msgTyeps the same 
 *  for Ap & MAC.
 * 
 * Last Changed Revision: $Rev: 3240 $ 
 * Last Changed Date: $LastChangedDate: 2009-07-20 22:44:04 -0500 (Mon, 20 Jul 2009) $  
 *
 ***********************************************************************
 */

#ifndef _AMP_MACMSG_H_
#define _AMP_MACMSG_H_

#include <linux/lte/AMP_typedef.h>

/**
 * the Max. payload size a management packet
 */
#define MAX_WIMAX_CONTROL_PACKET_SIZE (1920)  //The MAC can handle 2048 bytes

/* Managment Data structure defined for the tx/rx packet transmission */

/**
 * @struct AMP_HOST_MAC_MSG_HDR_S
 * The HMI (Host Mac interface) header structure. Do not change this!
 */
typedef struct _AMP_HOST_MAC_MSG_HDR_S
{
    UINT16 u16HostMsgType;    //< Host message type mapping to module ID 
    UINT16 u16Context;        //< conext for the response
    UINT8 u8MacMsgType;       //< WiP message type   
    UINT8 u8MsgSubtype;       //< message sub-type used in each module
    UINT16 u16Length;         //< payload length in bytes
} AMP_HOST_MAC_MSG_HDR_S;

/**
 * @struct AMP_HOST_MAC_MSG_HDR_S
 * The management message packet data structure. Do not change this!
 */
typedef struct
{
    AMP_HOST_MAC_MSG_HDR_S sHdr;
    UINT8 u8Data[1];
} AMP_HOST_MAC_MSG_S;


/**
 * @struct AMP_HOST_MAC_MSG_HDR_S
 * The HMI (Host Mac interface) header structure. Do not change this!
 */
typedef struct //_AMP_HOST_MAC_MSG_HDR_S
{
    UINT16 u16HostMsgType;    //< Host message type mapping to module ID 
    UINT16 u16Context;        //< conext for the response
    UINT8 u8MacMsgType;       //< WiP message type   
    UINT8 u8MsgSubtype;       //< message sub-type used in each module
    UINT16 u16Length;         //< payload length in bytes
} AMP_HOST_NAS_MSG_HDR_S;

/**
 * @struct AMP_HOST_MAC_MSG_HDR_S
 * The management message packet data structure. Do not change this!
 */
typedef struct //_AMP_HOST_MAC_MSG_S
{
    AMP_HOST_NAS_MSG_HDR_S sHdr;
    UINT8 u8Data[1];
} AMP_HOST_NAS_MSG_S;

/**
 * @enum AMP_MAC_HDR_HOST_MSG_E
 * The host message type.
 */
typedef enum
{
    eMAC_HDR_HOST_MSG_AUTHORIZATION = 1,     //< Security
    eMAC_HDR_HOST_MSG_QOSA = 2,              //< QoS Agent
    eMAC_HDR_HOST_MSG_ROAMINGA = 3,          //< roaming agent
    eMAC_HDR_HOST_MSG_PWRM = 4,              //< power management
    eMAC_HDR_HOST_MSG_DIAGNOSTICS = 7,       //< debug & diagnostics
    eMAC_HDR_HOST_MSG_CONFIG = 8,            //< Configuration
    eMAC_HDR_HOST_MSG_NETWORK_DRIVER = 5,  //< Wimax network driver.
    eMAC_HDR_HOST_MSG_LTE_LOG_MESSAGE = 25,  //< LTE log message from the BB processor
    eMAC_HDR_HOST_MSG_LTE_NAS_TFT_MSG = 26,  //< LTE NAS TFT Message 
    eMAC_HDR_HOST_MSG_LTE_NAS_IP_MSG  = 27,  //< LTE NAS IP Message
	eMAC_HDR_HOST_MSG_LTE_VPHY_PROXY  = 28,   // < LTE VPHY Message Proxied as a Mgmt Message
  
    eMAC_HDR_HOST_SERVICE_PROVIDER = 81,

    eMAC_HDR_HOST_MSG_ENG_TEST_DRIVER = 255,  //< Engineering testing driver.
    eMAC_HDR_HOST_MSG_SERICE_PROVIDER = 258, //< Wimax service provider.
} AMP_MAC_HDR_HOST_MSG_E;

// for data path
// multiple CS SPFT3, the data is in MAC endian - currently little endian only
typedef struct 
{
    UINT32 u32Sfid;     // For GPCS or Header Compression CS types (using ROHC or ECRTP), this field indicates the SFID of the service flow the CSSDU is transported on. For other CS types, this field is 0.
    UINT16 u16Reserved;     // 0
    UINT8 u8CsSpec;     // Identifies the CS type associated with the CSSDU. 
    UINT8 u8Phsi;       // For GPCS, this field indicates the PHSI that is to be used for PHS suppression in the (uplink) transmit direction. For other CS types, this field is 0. For the (downlink) receive direction, this field is 0.
} AMP_WIMAX_MAC_EHDU_S; // Extended Host Data Unit for multiple CS support

#define MULTI_CS_EHDU_LEN   sizeof(AMP_WIMAX_MAC_EHDU_S)

/* The last msg type value shared between MAC and Host */
#define MAX_MAC_MSG_TYPE (255)  

// The MSB defines the user defined type if allowed later
#define MAC_MSG_HDR_HOST_MSG_PRIVATE (0x8000) 

/**
 * @enum AMP_MAC_HDR_MAC_MSG_E
 * The WiP message type. Do not change it. They come from WiP.
 */
typedef enum
{
//#ifndef CONFIG_LTE
    eMAC_SECURITY_MESSAGE = 1,      //< Key management
    eMAC_SFM_MESSAGE = 2,           //< Service Flow Management
    eMAC_COCO_MESSAGE = 3,          //< Connection Control
    eMAC_PWRM_MESSAGE = 4,          //< Power management
    eMAC_BWM_MESSAGE = 5,           //< Band width management
    eMAC_TPC_MESSAGE = 6,           //< Transmit Power Control
 //   eMAC_DIAGNOSTIC_MESSAGE = 7,    // real time diagnostics
    eMAC_LSM_MESSAGE = 8,           //< Local Station Management
    eMAC_PMDT_MESSAGE = 9,          //< PMDT (Post Mortum Debug Tool)
    eMAC_ENG_TST_MESSAGE = 255,     //< MAC Engineering test type
//#endif
// eMAC_LSM_MESSAGE = 8,
#ifdef CONFIG_LTE
	ePROV_ID_EMM = 0x80,
	ePROV_ID_ESM = 0x81,
	ePROV_ID_RATM = 0x82,
#endif 	
} AMP_MAC_HDR_MAC_MSG_E;

#ifdef CONFIG_LTE
/*  
 * @enum tLTE_BP_MODULE_E
 * BP Module types.
 * //// TBD: Check values on BP side
 */
typedef enum
{
	LTE_HMM_RATM = 11,
	LTE_HMM_NAS = 0x80,
	LTE_HMM_VPHY = 13
} tLTE_BP_MODULE_E;
#endif

#define OPERATION_MODE_POINT_MULTIPOINT 1           // NORMAL mode bit 
#define OPERATION_MODE_MESH             2           // Mesh mode (not support yet) 
//#define OPERATION_MODE_DIAGNOSTICS   0X80000000     // Diagnostics mode 

/**
 * @enum eAMP_PMDT_SUBTYPE_E
 * The available MAC subtypes for the MAC PMDT message
 */
typedef enum
{
    eAMP_PMDT_SUBTYPE_START = 0,       //< PMDT Start, ITCM data follows
    eAMP_PMDT_SUBTYPE_DTCM = 1,        //< DTCM data follows
    eAMP_PMDT_SUBTYPE_RTCM = 2,        //< RTCM data follows
    eAMP_PMDT_SUBTYPE_STCM = 3,        //< STCM data follows
    eAMP_PMDT_SUBTYPE_STOP = 4,        //< PMDT completed
} AMP_PMDT_SUBTYPE_E;
#define PMDT_USB_UPLOAD_BUFFER_SIZE 1500
#define PMDT_NUM_PMDT_AREAS         eAMP_PMDT_SUBTYPE_STOP

/**
 * @enum AMP_MAC_POWER_MODE_MSG_E
 * The power management module.
 */
typedef enum
{
    eMAC_NORMAL_MODE = 1,        //< NORMAL MODE
    eMAC_IDLE_MODE = 2,          //< Idle mode
    eMAC_SLEEP_MODE = 3,         //< sleep Mode
} AMP_MAC_POWER_MODE_MSG_E;

/**
 * @enum MAC_MSG_NETWORK_DRIVER_E
 * The Network message ID or subtype.
 */
typedef enum
{
    eMAC_MSG_NETWORK_DRIVER_COMMIT = 1,
    eMAC_MSG_NETWORK_DRIVER_GET_NAME = 2,
    eMAC_MSG_NETWORK_DRIVER_GET_BSID = 3,
    eMAC_MSG_NETWORK_DRIVER_GET_FREQ = 4,
    eMAC_MSG_NETWORK_DRIVER_SET_OPMODE = 5,
    eMAC_MSG_NETWORK_DRIVER_GET_OPMODE = 6,
    eMAC_MSG_NETWORK_DRIVER_SET_WARNING_THREADSHOLD = 7,
    eMAC_MSG_NETWORK_DRIVER_GET_WARNING_THREADSHOLD = 8,
    eMAC_MSG_NETWORK_DRIVER_GET_RANGE_PARAM = 9,
    eMAC_MSG_NETWORK_DRIVER_GET_MAC_ADDR = 10,
    eMAC_MSG_NETWORK_DRIVER_SET_CONFIG = 11,
    eMAC_MSG_NETWORK_DRIVER_GET_CONFIG = 12,
    eMAC_MSG_NETWORK_DRIVER_GET_SCAN_RESULT = 13,
    eMAC_MSG_NETWORK_DRIVER_GET_TX_POWER = 14,
    eMAC_MSG_NETWORK_DRIVER_SET_POWER_MODE = 15,
    eMAC_MSG_NETWORK_DRIVER_GET_POWER_MODE = 16,
    eMAC_MSG_NETWORK_DRIVER_GET_VERSION_NUM = 17,
    eMAC_MSG_NETWORK_DRIVER_RESET_MAC = 18,
    eMAC_MSG_NETWORK_DRIVER_GET_RESET_RESULT = 19,
    eMAC_MSG_NETWORK_DRIVER_GET_COMM_QUALITY = 20,
    eMAC_MSG_NETWORK_DRIVER_MAC_ENG_TEST = 255,
} MAC_MSG_NETWORK_DRIVER_E;

/**
 * @enum AMP_GETOBJECT_SUBTYPE_E
 * The SetObject/GetObject command type.
 */
typedef enum
{
    eAMP_GETOBJECT_SUBTYPE_GET = 1,                     //< Regular Get
    eAMP_GETOBJECT_SUBTYPE_GET1ST = 2,                  //< Get_first
    eAMP_GETOBJECT_SUBTYPE_GETNEXT = 3,                 //< Get_next
    eAMP_SETOBJECT_SUBTYPE_SET = 4,                     //< SET
    eAMP_GETOBJECT_SUBTYPE_RESPONSE = 5,                //< Response back
    eMAC_MSG_NETWORK_DRIVER_MAC_READY = 6,              //< MAC ready message
#ifdef CONFIG_LTE
    eMAC_MSG_NETWORK_DRIVER_MAC_THROTTLE_CLOSE = 7,     //< MAC throttle close (close valve)
    eMAC_MSG_NETWORK_DRIVER_MAC_THROTTLE_OPEN = 8,      //< MAC throttle open (open valve)
#else
    eMAC_MSG_MAC_RESET = 7               //< Reset MAC message
#endif
} AMP_GETOBJECT_SUBTYPE_E;

/**
 * @struct SETOBJECT_COMMAND_S
 * The SetObject command header structure.
 */
typedef struct _SETOBJECT_COMMAND
{
    UINT32 u32Index1_1;       //< MSBs of the Index 1
    UINT32 u32Index1_2;       //< LSBs of the index 1
    UINT32 u32Index2_1;       //< MSBs of the Index 2
    UINT32 u32Index2_2;       //< LSBs of the Index 2
    UINT16 u16Oid;            //< Object Identification
    UINT16 u16Length;         //< number of bytes for the SET command
} SETOBJECT_COMMAND_S;

/**
 * @struct OBJECT_RESPONSE_PAYLOAD
 * The SetObject/GetObject response payload.
 */

typedef struct _OBJECT_RESPONSE
{
    UINT32 u32Status;
    SETOBJECT_COMMAND_S sCmd;
    UINT8 data[];
} OBJECT_RESPONSE_PAYLOAD_S;

/**
 * @struct GETOBJECT_COMMAND_S
 * The GetObject command header structure.
 */
typedef struct _GETOBJECT_COMMAND
{
    UINT32 u32Index1_1;       //< MSBs of the Index 1
    UINT32 u32Index1_2;       //< LSBs of the index 1
    UINT32 u32Index2_1;       //< MSBs of the Index 2
    UINT32 u32Index2_2;       //< LSBs of the Index 2
    UINT16 u16Oid;            //< Object Identification
    UINT16 reserved;         
} GETOBJECT_COMMAND_S;
/**
 * @enum CONFIG_STATUS_CODE_E
 * The Configuration return code.
 * MUST MATCH motorola/modem/umts/src/stack_lte/bp_sw/common/include/lte_mgmt_apif.h
 */
typedef enum
{
  eCONFIG_STATUS_CODE_OK = 0,              //< Success
  eCONFIG_STATUS_CODE_PARTIAL = 1,         //< SuccessPartial
  eCONFIG_STATUS_CODE_ENDOF_INST = 2,      //< End Of Instance
  eCONFIG_STATUS_CODE_ERROR = 3,           //< Error
  eCONFIG_STATUS_CODE_MALFORMED_REQ = 4,   //< Malformed Request
  eCONFIG_STATUS_CODE_ILLEGAL_REQ = 5,     //< Illegal Request
  eCONFIG_STATUS_CODE_INVALID_MOID = 6,    //< Invalid OID
  eCONFIG_STATUS_CODE_INVALID_VALUE = 7,   //< Invalid Value
  eCONFIG_STATUS_CODE_INVALID_INDEX = 8,   //< Invalid Index
  eCONFIG_STATUS_CODE_INVALID_ACCESS = 9,  //< Invalid Access 
  eCONFIG_STATUS_CODE_NO_RESOURCE = 10,    //< NoResources
}CONFIG_STATUS_CODE_E;

#if defined(WIN32) || defined(HOST_EMULATION)

// Windows replacement for ioctl functionality

/**
 ** @struct DRIVERCTL_REQ_S
 ** The Driver Control command header structure.
 **/
typedef struct _DRIVERCTL_REQ
{
    UINT32 u32subtype;  //< Message subtype
    UINT32 u32value;    //< Value of the parameter
    UINT8  u8fixed;     //< Do not use auto select
    UINT8  u8disabled;  //< Disable the feature
    UINT16 u16flags;    //< Feature flags
    UINT16 reserved;
} DRIVERCTL_REQ_S;

/**
 ** @struct DRIVERCTL_RSP_S
 ** The Driver Control command header structure.
 **/
typedef struct _DRIVERCTL_RSP
{
    UINT32 u32subtype;  //< Message subtype
    UINT32 u32status;   //< Value of the parameter
    UINT32 u32value;    //< Value of the parameter
    UINT16 u16flags;    //< Feature flags
    UINT16 reserved;
} DRIVERCTL_RSP_S;


#endif /* WIN32 or HOST_EMULATION */

#ifdef HOST_EMULATION

/**
 ** @enum AMP_DRIVERCTL_SUBTYPE_E
 ** The SetObject/GetObject command type.
 **/
typedef enum
{
        eAMP_DRIVERCTL_SUBTYPE_CLIENTSTART = 1,      //< Client Start
        eAMP_DRIVERCTL_SUBTYPE_SETOPMODE = 2,   //< Set OP mode
        eAMP_DRIVERCTL_SUBTYPE_GETOPMODE = 3,  //< Get OP mode
        eAMP_DRIVERCTL_SUBTYPE_RESET = 4,  //< Reset
        eAMP_DRIVERCTL_SUBTYPE_SHUTDOWN = 5   //< Shutdown
} AMP_DRIVERCTL_SUBTYPE_E;


#endif /* HOST_EMULATION */

#endif  /* _AMP_MACMSG_H_ */

