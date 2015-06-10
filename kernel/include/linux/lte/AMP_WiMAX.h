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


#ifndef _MOT_AMP_WIMAX_INC_
#define _MOT_AMP_WIMAX_INC_

#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_general.h>
#include <linux/lte/AMP_global.h>

#define SPFT2 TRUE
#define SPFT3 TRUE

#define IEEE80216e_cor2 TRUE
#define MULTI_CS TRUE
#define MAC_EHDU TRUE

#define AMP_WIMAX_MAX_BSID_TABLE (20)
#define AMP_WIMAX_MAX_NAP_PER_NSP (12)
#define AMP_WIMAX_MAX_NSP_PER_NAP (24)
#define AMP_WIMAX_MAX_NSP_REALM_LEN (80)

#include "AMP_WiMAX_DNM.h"          // include the do not modifify header

// Do we need to support more than one?
#define AMP_WIMAX_MAX_CLA_RULE_PER_SF (2)
#define AMP_WIMAX_MAX_PHS_RULE_PER_SF (2)

#define AMP_WIMAX_MAX_CLA_RULE_PER_CS ((AMP_WIMAX_MAX_CLA_RULE_PER_SF)*(AMP_WIMAX_MAX_SF))
#define AMP_WIMAX_REQUIRED_CLA_RULE_PER_CS (32)
#define AMP_WIMAX_CLA_RULE_PER_CS (MIN((AMP_WIMAX_REQUIRED_CLA_RULE_PER_CS), (AMP_WIMAX_MAX_CLA_RULE_PER_CS)))

#ifdef CONFIG_LTE

#define AMP_LTE_MAX_TFTS_PER_CS 8
#define AMP_LTE_MAX_RULE_PER_PFT 8
#define AMP_LTE_MAX_PFTS_PER_TFT 16
#define AMP_WIMAX_MAX_TFT 32
#define AMP_WIMAX_MAX_CS_CLA_RULE AMP_WIMAX_MAX_TFT

#else

#define AMP_WIMAX_MAX_CS_CLA_RULE AMP_WIMAX_MAX_CS

#endif


// Place holder for future PHS support
//#define AMP_WIMAX_MAX_IPV4_HEADER (256)
//#define AMP_WIMAX_MAX_IP_HEADER (2 + 14 + 4 + 20 + 12)            // IPv4 ???
//#define AMP_WIMAX_MAX_IPV6_HEADER (256)

// TLV for ND&S NSP List and verbose name
#define eAMP_WIMAX_MAC_NSP_LIST_TLV (133)
#define eAMP_WIMAX_MAC_VERBOSE_NAME_TLV (132)

// Retry initial scan after failure
#define AMP_WIMAX_MAX_INITIAL_SCAN_NUM_RETRY    (2)
// Wait before retry in secs
#define AMP_WIMAX_INITIAL_SCAN_FAIL_WAIT_TIME   (2)


// WiMAX RA/QA/SP definition

// place holder for error handling
typedef enum
{
    eAMP_WIMAX_REASON_UNKNOWN = -1
} AMP_WIMAX_REASON_CODE_E;

typedef enum
{
    eAMP_WIMAX_CONN_STATUS_UNKNOWN = -1,
    eAMP_WIMAX_MAX_LN_STATUS_START = 0,
    eAMP_WIMAX_MAX_LN_STATUS_END = eAMP_WIMAX_MAX_LN_STATUS_START + 20, // should be larger then the entrys in E_AMP_WIMAX_MAC_LN_STATUS
    eAMP_WIMAX_MAX_LN_STATUS_MAX_VAL
} AMP_WIMAX_CONN_STATUS_E;

typedef enum
{
    eAMP_WIMAX_QOS_STATUS_UNKNOWN = -1,
    eAMP_WIMAX_QOS_STATUS_SUCCESS = 0,
    eAMP_WIMAX_QOS_STATUS_IN_PROGRESS,
    eAMP_WIMAX_QOS_STATUS_FAIL,
    eAMP_WIMAX_QOS_STATUS_MAX_VAL
} AMP_WIMAX_QOS_STATUS_E;



// WiP CoCo definitions



// WiP service flow and bandwidth definitions

//0 for MS created, 1 for BS created
typedef enum
{
    eAMP_WIMAX_SF_CREATER_MSS = 0,
    eAMP_WIMAX_SF_CREATER_BS = 1,
} AMP_WIMAX_SF_CREATER_E;


// QoS parameter type
typedef enum
{
    eAMP_WIMAX_QOS_PARAM_SELECT_UNKNOWN = -1,
    eAMP_WIMAX_QOS_PARAM_SELECT_COPS = 0,           // will be in the future
    eAMP_WIMAX_QOS_PARAM_SELECT_SDP,            // will be in the future
    eAMP_WIMAX_QOS_PARAM_SELECT_SME,
    eAMP_WIMAX_QOS_PARAM_SELECT_FULL,
    eAMP_WIMAX_QOS_PARAM_SELECT_MAX_VAL
} AMP_WIMAX_QOS_PARAM_SELECT_E;

typedef struct
{
    UINT8 u8Data[1];
} AMP_WIMAX_COPS_TOKEN_S;

typedef struct
{
    UINT8 u8Data[1];
} AMP_WIMAX_SDP_SESSION_S;


typedef struct
{
    UINT32 u32QosId;
    UINT32 u8UseCount:8;
    UINT32 u8Status:8;
    UINT32 u8Creator:8;     // 0 for MS created, 1 for BS created
    UINT32 u8Pad:8;
} AMP_WIMAX_QOS_DESC_S;

// Message payload

// The message for both RA and QA from WiMAX Network driver


// The message for RA

typedef struct
{
    AMP_WIMAX_MAC_ID_S sNspId;      // get current NSP if sNspId is invalid
} AMP_WIMAX_MSG_NSP_REQ_S;

// when both NAPID and NSPID are invalid, the whole table of NSP infomation will be sent out
typedef struct
{
    AMP_WIMAX_MAC_ID_S sNapId;      // If valid, total NSP list for the NAP
    AMP_WIMAX_MAC_ID_S sNspId;      // Ignored if NAPID is valid, If valid, all info about this NSP
    UINT8 ubNewOnly:1;              // If set to 1, only the new NSP since last report, valid when both NAPID and NSPID are invalid
    UINT8 ubNoVerboseName:1;        // If set to 1, only NSPID is reported, not apply to NSPID is valid
} AMP_WIMAX_MSG_NSP_LIST_REQ_S;

typedef struct
{
    AMP_WIMAX_MAC_ID_S sNspId;      // get one NSP if sNspId is valid, or all NSP discovered
} AMP_WIMAX_MSG_DISCOVERED_NSP_REQ_S;

typedef struct
{
    AMP_WIMAX_MAC_ID_S sNspId;
    AMP_WIMAX_MAC_ID_S sNapId[AMP_WIMAX_MAX_NAP_PER_NSP];
    UINT8 szNspName[AMP_WIMAX_MAX_NSP_REALM_LEN];
    UINT8 szNspRealm[AMP_WIMAX_MAX_NSP_REALM_LEN];
    UINT8 u8NumNap;
    UINT8 u8NapIndex;
    UINT8 u8RoamingAndNapSharingFlag;
    UINT8 u8Pad;
} AMP_WIMAX_NSP_ENTRY_S;

typedef struct
{
    AMP_WIMAX_MAC_ID_S sNapId;
    UINT8 u8NspChangeCount;
    UINT8 u8Pad[3];
    AMP_WIMAX_NSP_ENTRY_S sNspEntry[AMP_WIMAX_MAX_NSP_PER_NAP];
} AMP_WIMAX_NAP_ENTRY_S;

typedef struct
{
    AMP_WIMAX_NSP_ENTRY_S sNspEntry;
} AMP_WIMAX_MSG_NSP_RSP_S;

// One or more entries. The end is marked by a zero entry
typedef struct
{
    AMP_WIMAX_NAP_ENTRY_S sNapEntry[1];
} AMP_WIMAX_MSG_NSP_LIST_RSP_S;

// One or more entries. The end is marked by a zero entry
typedef struct
{
    AMP_WIMAX_NSP_ENTRY_S sNspEntry[1];
} AMP_WIMAX_MSG_DISCOVERED_NSP_RSP_S;

// must stay as 32 bits
typedef struct
{
    UINT8 bAutoStart:1; // When the bit is set, RA thread will drive the state machine, otherwise, application has to drive it
    UINT8 bEnabled:1;       // RA thread is up and running
    UINT8 bConnected:1;     // The REG successfully
    UINT8 bQaReady:1;       // default service flow is setup
    UINT8 bSpRunning:1;     // WiMAX SP is running
    UINT8 bCurrentBsInfoReady:1;
    UINT8 bMacEnabled:1;
    UINT8 uPad:2;
    UINT8 u8Pad[3];
} AMP_WIMAX_RA_STATUS_S;

typedef struct
{
    UINT8 nEnabled:1;       // RA thread is up and running
    UINT8 nCs:1;        // CS negotiated
    UINT8 uPad:6;
    UINT8 u8Pad[3];
} AMP_WIMAX_QA_STATUS_S;

typedef struct
{
    AMP_WIMAX_CONN_STATUS_E eConnStatus;
    AMP_WIMAX_RA_STATUS_S sRAStatus;
} AMP_WIMAX_MSG_LINK_STATUS_IND_S;


// QoS agent command message

typedef struct
{
    AMP_WIMAX_SF_CLA_DSC_ACTION_E eCurrentAction;
    UINT32 u32Sfid; 
    UINT32 u32ClaMask;
    AMP_WIMAX_SF_CLA_RULE_S sClaRule;
} AMP_WIMAX_MSG_QOS_CLA_DESC_S;

#ifdef CONFIG_LTE
typedef struct
{
    AMP_WIMAX_MSG_QOS_CLA_DESC_S sClaRules;
} AMP_LTE_PFT_QOS_S;
#endif

typedef struct
{
    AMP_WIMAX_SF_PHS_DSC_ACTION_E eCurrentAction;
    UINT32 u32PhsMask;
    AMP_WIMAX_SF_PHS_RULE_S sPhsRule;
} AMP_WIMAX_MSG_QOS_PHS_DESC_S;

typedef struct
{
    UINT32 u32QosId;
    UINT64 u64SfRequiredParamsMask;     // one bit for each SF TLV
    AMP_WIMAX_QOS_PARAM_SELECT_E u32QosParamsSelect;
    union
    {
        AMP_WIMAX_QOS_SME_PARAMS_S sQosSmeParams;       // will be out later since it is a subset of the full params
        AMP_WIMAX_QOS_FULL_PARAMS_S sQosFullParams;
        AMP_WIMAX_COPS_TOKEN_S sSdp;                    // SDP
        AMP_WIMAX_COPS_TOKEN_S sCopsToken;          // COPS token
    } QOS_PARAMS_U;
    UINT32 u32NumOfClaRules;
    AMP_WIMAX_MSG_QOS_CLA_DESC_S sClaRules[AMP_WIMAX_MAX_CLA_RULE_PER_SF];
    UINT32 u32NumOfPhsRules;
    AMP_WIMAX_MSG_QOS_PHS_DESC_S sPhsRules[AMP_WIMAX_MAX_PHS_RULE_PER_SF];
} AMP_WIMAX_MSG_QOS_ALLOC_REQ_S;

typedef struct
{
    UINT32 u32QosId;
} AMP_WIMAX_MSG_QOS_DELETE_REQ_S;

typedef struct
{
    UINT32 u32QosId;
    UINT64 u64SfRequiredParamsMask;     // one bit for each SF TLV
    AMP_WIMAX_QOS_PARAM_SELECT_E u32QosParamsSelect;
    union
    {
        AMP_WIMAX_QOS_SME_PARAMS_S sQosSmeParams;
        AMP_WIMAX_QOS_FULL_PARAMS_S sQosFullParams;
        AMP_WIMAX_COPS_TOKEN_S sSdp;                    // SDP
        AMP_WIMAX_COPS_TOKEN_S sCopsToken;          // COPS token
    } QOS_PARAMS_U;
    UINT32 u32NumOfClaRules;
    AMP_WIMAX_MSG_QOS_CLA_DESC_S sClaRules[AMP_WIMAX_MAX_CLA_RULE_PER_SF];
    UINT32 u32NumOfPhsRules;
    AMP_WIMAX_MSG_QOS_PHS_DESC_S sPhsRules[AMP_WIMAX_MAX_PHS_RULE_PER_SF];
} AMP_WIMAX_MSG_QOS_UPDATE_REQ_S;

/*typedef struct
{
    UINT32 u32BitsToStartSilence;       // 8 frames?
    UINT32 u32BitsToStopSilence;        // one frame?
} AMP_WIMAX_MSG_QOS_SILENCE_INFO_REQ_S;

typedef struct
{
    UINT8 u8SilenceStartFlag;       // TRUE for start and flase for stop
    UINT8 u8SidFrameSize;
    UINT8 u8SidInterval;
    UINT8 u8Pad;
} AMP_WIMAX_MSG_QOS_SILENCE_REQ_S;*/

typedef struct
{
    UINT32 u32QosId;
} AMP_WIMAX_MSG_QOS_QUERY_REQ_S;

//QoS Agent reply and notif message

// immediatelly ACK for QoS Alloc
typedef struct
{
    UINT32 u32QosId;        //= WiMAX QA sign a number
    UINT32 u32Status;        //= we will define status code for it
} AMP_WIMAX_MSG_QOS_ALLOC_ACK_S;

typedef struct
{
    UINT32 u32QosId;
    UINT32 u32Status;
    UINT64 u64SfParamsMask;     // one bit for each SF TLV
    AMP_WIMAX_QOS_FULL_PARAMS_S sQosFullParams;
    UINT32 u32NumOfClaRules;
    AMP_WIMAX_MSG_QOS_CLA_DESC_S sClaRules[AMP_WIMAX_MAX_CLA_RULE_PER_SF];
    UINT32 u32NumOfPhsRules;
    AMP_WIMAX_MSG_QOS_PHS_DESC_S sPhsRules[AMP_WIMAX_MAX_PHS_RULE_PER_SF];
} AMP_WIMAX_MSG_QOS_ALLOC_REPLY_S;

typedef struct
{
    UINT32 u32QosId;
    UINT32 u32Status;
} AMP_WIMAX_MSG_QOS_DELETE_REPLY_S;

typedef AMP_WIMAX_MSG_QOS_ALLOC_REPLY_S AMP_WIMAX_MSG_QOS_UPDATE_REPLY_S;

typedef AMP_WIMAX_MSG_QOS_ALLOC_REPLY_S AMP_WIMAX_MSG_QOS_QUERY_REPLY_S;

typedef struct
{
    UINT32 u8MarkOfEnd:8;
    UINT32 u8Pad:8;
    UINT32 u16NumOfSf:16;
    AMP_WIMAX_QOS_DESC_S sQosDesc[1];
} AMP_WIMAX_MSG_QOS_CURRENT_LIST_REPLY_S;

typedef AMP_WIMAX_MSG_QOS_ALLOC_REPLY_S AMP_WIMAX_MSG_QOS_NOTIF_S;

#define AMP_WIMAX_CS_NOTIF_SIZE ((AMP_WIMAX_MAX_CS)+4)
typedef struct
{
    UINT32 u16NumOfCs:16;
    UINT32 u8IsActive:8;
    UINT32 u8Pad:8;
    UINT8 u8Cs[1];
} AMP_WIMAX_MSG_CS_NOTIF_S;

/**
 * @struct AMP_IPC_PMDT_HDR_S
 * The PMDT command header structure.
 */
typedef struct
{
    UINT16 u16Type;    //< Host message type mapping to module ID
    UINT16 u16Length;  //< payload length in bytes
} AMP_IPC_PMDT_HDR_S;

/**
 * @enum AMP_IPC_PMDT_E
 * This is the list of available types for the AMP_IPC_PMDT_S messages.
 */
typedef enum
{
  eAMP_IPC_PMDT_LOG_START = 0,      //< Open log files to uploading data.
  eAMP_IPC_PMDT_LOG_STOP = 1,       //< Close log files when complete.
  eAMP_IPC_PMDT_LOG_ITCM_DATA = 2,  //< Log ITCM data.
  eAMP_IPC_PMDT_LOG_DTCM_DATA = 3,  //< Log DTCM data.
  eAMP_IPC_PMDT_LOG_RTCM_DATA = 4,  //< Log RTCM data.
  eAMP_IPC_PMDT_LOG_STCM_DATA = 5,  //< Log STCM data.
  eAMP_IPC_PMDT_DOWNLOAD_WIP = 6,   //< Download new FW image
} AMP_IPC_PMDT_E;

#endif  //_MOT_AMP_WIMAX_INC_

