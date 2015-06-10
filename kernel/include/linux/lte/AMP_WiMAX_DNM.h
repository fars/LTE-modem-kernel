/*
 * Copyright Motorola, Inc. 2005-2006
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

#ifndef _MOT_AMP_WIMAX_DNM_INC_
#define _MOT_AMP_WIMAX_DNM_INC_

//#include <linux/lte/AMP_typedef.h>
//#include <linux/lte/AMP_general.h>
//#include <linux/lte/AMP_global.h>

// Layer 2 ICD R01.00.05
// The MS shall select transaction IDs from 0x0000 to 0x7FFF
// The BS shall select transaction IDs from 0x8000 to 0xFFFF
#define AMP_WIMAX_QA_CONTEXT_ID_MASK (0x7FFF)


// WiMAX ICD-009, 1/18/2006
#define AMP_SF_SF_CLASS_NAME_VoIP_1 "VoIP_1"
#define AMP_SF_SF_CLASS_NAME_DATA1M_UL "Data1M_UL"
#define AMP_SF_SF_CLASS_NAME_DATA1M_DL "Data1M_DL"
#define AMP_SF_SF_CLASS_NAME_BE_Data "BE_Data"
#define AMP_SF_SF_CLASS_NAME_BE_sig "BE_sig"

// WiMAX RA/QA/SP definition
#define AMP_WIMAX_QA_MAX_SFID_PER_PSC_REQ (8)

// AMP CoCo/SFM/BWM SPFT2, DNM
// common IPC messages for WiMAX modules, private
//*** when change this enum, For RA part, gsRaHandlerTable in WiMAX_roaming_agent.c should be updated
typedef enum
{
    eAMP_IPC_WIMAX_MSG_INVALID = -1,
    eAMP_IPC_WIMAX_MSG_MIN_VAL = 0x8000,

    // in this group, the ID is determined by WiP CoCo module
    eAMP_IPC_WIMAX_MSG_RA_BASE = 0x8000,
    eAMP_IPC_WIMAX_MSG_SCAN_REQ = 0x8001,
    eAMP_IPC_WIMAX_MSG_CONNECT_REQ = 0x8002,
    eAMP_IPC_WIMAX_MSG_HANDOVER_REQ = 0x8002,       // same as CONNECT
    eAMP_IPC_WIMAX_MSG_DISCONNECT_REQ = 0x8003,
    eAMP_IPC_WIMAX_MSG_SET_LINK_TRIGGERS_REQ = 0x8004,
    //eAMP_IPC_WIMAX_MSG_SET_LINK_QUALITY_WARNING_THRESHOLD = 0x8004,
    eAMP_IPC_WIMAX_MSG_GET_LINK_STATUS_REQ = 0x8005,   
    eAMP_IPC_WIMAX_MSG_SET_SCAN_SETTINGS_REQ = 0x8011,
    eAMP_IPC_WIMAX_MSG_GET_LINK_TRIGGERS_REQ = 0x8012,
    eAMP_IPC_WIMAX_MSG_IDLE_MODE_ENTER_REQ = 0x8015,
    eAMP_IPC_WIMAX_MSG_IDLE_MODE_EXIT_REQ = 0x8018,

    eAMP_IPC_WIMAX_MSG_LINK_UP_NTF = 0x8006,
    eAMP_IPC_WIMAX_MSG_LINK_DOWN_NTF = 0x8007,
    eAMP_IPC_WIMAX_MSG_HO_START_NTF = 0x8008,
    eAMP_IPC_WIMAX_MSG_HO_COMPLETE_NTF = 0x8009,   
    eAMP_IPC_WIMAX_MSG_LINK_TRIGGER_NTF = 0x800A,
    //eAMP_IPC_WIMAX_MSG_LINK_QUALITY_WARNING_NTF = 0x800A,
    eAMP_IPC_WIMAX_MSG_CONNECT_REPLY_NTF = 0x800F,
    eAMP_IPC_WIMAX_MSG_HO_REJ_NTF = 0x8010,
    eAMP_IPC_WIMAX_MSG_NEW_NSP_INFORMATION_NTF = 0x8014,  // only a place holder for CoCo message, application will never receive this message
    eAMP_IPC_WIMAX_MSG_IDLE_MODE_ENTERED_NTF = 0x8016,
    eAMP_IPC_WIMAX_MSG_IDLE_MODE_UPDATE_NTF = 0x8017,
    eAMP_IPC_WIMAX_MSG_IDLE_MODE_EXIT_NTF = 0x8019,
    eAMP_IPC_WIMAX_MSG_AIRPLANE_MODE_NTF = 0x801A,

    eAMP_IPC_WIMAX_MSG_LINK_STATUS_RSP = 0x800B,        // response to eAMP_IPC_WIMAX_MSG_GET_LINK_STATUS_REQ
    eAMP_IPC_WIMAX_MSG_SCAN_RESULT_RSP = 0x800C,        // 12, 13 and 14 are all for SCAN result in MAC
    eAMP_IPC_WIMAX_MSG_CURRENT_LINK_TRIGGER_RSP = 0x8013, // response to eAMP_IPC_WIMAX_MSG_GET_LINK_TRIGGERS_REQ
    eAMP_IPC_WIMAX_MSG_RA_MAX = 0x801A,

    // This group of message will not reach MAC
    eAMP_IPC_WIMAX_MSG_RA_REQ_LOCAL_BASE = 0x8060,
    eAMP_IPC_WIMAX_MSG_RA_NSP_REQ = 0x8060,     // for NAI, get info for one NSP
    eAMP_IPC_WIMAX_MSG_RA_NSP_LIST_REQ = 0x8061,    // get the dynamic NSP list per NAP
    eAMP_IPC_WIMAX_MSG_RA_DISCOVERED_NSP_REQ = 0x8062,      // for manual selection
    eAMP_IPC_WIMAX_MSG_RA_HOME_NSP_REQ = 0x8063,        // for NAI, get info for one NSP
    eAMP_IPC_WIMAX_MSG_RA_RESTART_NDS = 0x8064,			// Redo ND&S
    eAMP_IPC_WIMAX_MSG_RA_GET_IDLE_MODE_INFO_REQ = 0x8067,
    eAMP_IPC_WIMAX_MSG_RA_REQ_LOCAL_MAX = 0x8067,

    // This group of message is responded by RA
    eAMP_IPC_WIMAX_MSG_RA_RSP_LOCAL_BASE = 0x8080,
    eAMP_IPC_WIMAX_MSG_RA_NSP_RSP = 0x8080,
    eAMP_IPC_WIMAX_MSG_RA_NSP_LIST_RSP = 0x8081,
    eAMP_IPC_WIMAX_MSG_RA_DISCOVERED_NSP_RSP = 0x8082,
    eAMP_IPC_WIMAX_MSG_RA_HOME_NSP_RSP = 0x8083,
    eAMP_IPC_WIMAX_MSG_RA_GET_IDLE_MODE_INFO_RSP = 0x8087,
    eAMP_IPC_WIMAX_MSG_RA_RSP_LOCAL_MAX = 0x8087,

    eAMP_IPC_WIMAX_MSG_QOS_BASE = 0x80A0,
    eAMP_IPC_WIMAX_MSG_QOS_ALLOC_REQ = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0001,
    eAMP_IPC_WIMAX_MSG_QOS_UPDATE_REQ = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0002,
    eAMP_IPC_WIMAX_MSG_QOS_DELETE_REQ = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0003,
    eAMP_IPC_WIMAX_MSG_BWM_SF_INFO_REQ = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x000B,
    eAMP_IPC_WIMAX_MSG_BWM_SF_SILENCE_PERIOD_REQ = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x000D,
    eAMP_IPC_WIMAX_MSG_PSC_REQ = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x000F,

    eAMP_IPC_WIMAX_MSG_QOS_ALLOC_RSP = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0004,
    eAMP_IPC_WIMAX_MSG_QOS_UPDATE_RSP = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0005,
    eAMP_IPC_WIMAX_MSG_QOS_DELETE_RSP = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0006,
    eAMP_IPC_WIMAX_MSG_QOS_ALLOC_NTF = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0007,        // not used, only place holder
    eAMP_IPC_WIMAX_MSG_QOS_UPDATE_NTF = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0008,
    eAMP_IPC_WIMAX_MSG_QOS_DELETE_NTF = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0009,
    eAMP_IPC_WIMAX_MSG_CS_NTF = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x000A,
    eAMP_IPC_WIMAX_MSG_BWM_SF_INFO_RSP = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x000C,
    eAMP_IPC_WIMAX_MSG_BWM_SF_SILENCE_PERIOD_RSP = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x000E,
    eAMP_IPC_WIMAX_MSG_PSC_RSP = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0010,
    eAMP_IPC_WIMAX_MSG_PSC_NTF = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0011,

#ifdef CONFIG_LTE
    	eAMP_IPC_LTE_MSG_NAS_TFT = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0012,
    	eAMP_IPC_LTE_MSG_NAS_TFT_MODIFY = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0013,
	eAMP_IPC_WIMAX_MSG_QOS_MAX = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0013,
#else
    eAMP_IPC_WIMAX_MSG_QOS_MAX = eAMP_IPC_WIMAX_MSG_QOS_BASE + 0x0011,
#endif
    eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE = 0x80C0,
    eAMP_IPC_WIMAX_MSG_QOS_QUERY_REQ = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0001,
    eAMP_IPC_WIMAX_MSG_QOS_CURRENT_LIST_REQ = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0002,
    eAMP_IPC_WIMAX_MSG_QOS_ALLOC_ACK = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0003,
    eAMP_IPC_WIMAX_MSG_QOS_QUERY_RSP = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0004,
    eAMP_IPC_WIMAX_MSG_QOS_CURRENT_LIST_RSP = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0005,
    eAMP_IPC_WIMAX_MSG_QOS_NTF = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0006,
	eAMP_IPC_WIMAX_MSG_QOS_LOCAL_MAX = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0006,
    eAMP_IPC_WIMAX_MSG_MAX_VAL = 0xFFFF
} AMP_IPC_WIMAX_MSG_TYPE_E;


// WiP CoCo definitions

// AMP CoCo SPFT2
typedef enum
{
    eAMP_WIMAX_LINK_LNDW_CODE_UNKNOWN_REASON = 0,               //Other, unknown reason
    eAMP_WIMAX_LINK_LNDW_CODE_RA_DISCONNECT = 1,                    //Roaming Agent issued a Disconnect() command
    eAMP_WIMAX_LINK_LNDW_CODE_OUT_OF_RANGE_OF_NETWORK = 2,      //MS is out of range of the network and/or BS it is supposed to connect to 
    eAMP_WIMAX_LINK_LNDW_CODE_SYNCHRONIZATION_LOST = 3,     //MAC and/or PHY layer synchronization was lost
    eAMP_WIMAX_LINK_LNDW_CODE_RANGING_FAILURE = 4,              //There was an error during Initial/Handover or Periodic Ranging (retry limit exceeded)
    eAMP_WIMAX_LINK_LNDW_CODE_RANGING_REFUSED = 5,              //The MS received a RNG-RSP with status 'abort'
    eAMP_WIMAX_LINK_LNDW_CODE_BASIC_CAPABILITY_NEGOTIATION_FAILURE = 6,     //There was an error during Basic Capability Negotiation (retry limit exceeded)
    eAMP_WIMAX_LINK_LNDW_CODE_BASIC_CAPABILITY_NEGOTIATION_REFUSED = 7,     //The MS received a SBC-RSP message that prevents successful connection to the BS
    eAMP_WIMAX_LINK_LNDW_CODE_REAUTHORIZATION_FAILURE = 8,      //There was an error during (Re-) Authorization (retry limit exceeded)
    eAMP_WIMAX_LINK_LNDW_CODE_REAUTHORIZATION_REFUSED = 9,      //The MS's request for authorization was rejected by the network
    eAMP_WIMAX_LINK_LNDW_CODE_REGISTRATION_FAILURE = 10,        //There was an error during Registration (retry limit exceeded)
    eAMP_WIMAX_LINK_LNDW_CODE_REGISTRATION_REFUSED = 11,        //The MS received a REG-RSP message that prevents successful connection to the BS
    eAMP_WIMAX_LINK_LNDW_CODE_POWER_DOWN = 12,
    eAMP_WIMAX_LINK_LNDW_CODE_RESET_BY_NETWORK = 13             // received RES_CMD over the air
} AMP_WIMAX_LINK_LNDW_CODE_E;

// AMP CoCo SPFT2
typedef enum
{
    eAMP_WIMAX_LINK_STATUS_DISABLED = 0,    //Not connected to any network, no activities in the MS
    eAMP_WIMAX_LINK_STATUS_INITIAL_SCAN = 1,    //Not connected to any network, performing Initial Scan 
    eAMP_WIMAX_LINK_STATUS_CONNECTING = 2,      //Not connected, trying to connect to a specific BS or network
    eAMP_WIMAX_LINK_STATUS_OUT_OF_RANGE = 3,        //Not connected, unable to find network, periodically scanning
    eAMP_WIMAX_LINK_STATUS_CONNECTED = 4,       //In normal operation mode
    eAMP_WIMAX_LINK_STATUS_SLEEPING = 5,        //Connected, in Sleep Mode
    eAMP_WIMAX_LINK_STATUS_IDLE = 6,            //Connected, in Idle Mode
    eAMP_WIMAX_LINK_STATUS_IN_HANDOVER = 7      //Connected, in Handover mode
} AMP_WIMAX_LINK_STATUS_E;

// AMP CoCo SPFT2
typedef enum
{
    eAMP_WIMAX_LINK_HO_INITIATOR_UNKNOWN = -1,
    eAMP_WIMAX_LINK_HO_INITIATOR_MS = 0,        // MS MAC will initiate HO when HO is with in the same NAP
    eAMP_WIMAX_LINK_HO_INITIATOR_BS = 1,        // BS initiate HO
    eAMP_WIMAX_LINK_HO_INITIATOR_RA = 2,        // Roaming Agent initiate HO to different NAP or instructed by upper layer
    eAMP_WIMAX_LINK_HO_INITIATOR_MAX_VAL
} AMP_WIMAX_LINK_HO_INITIATOR_E;

// AMP CoCo SPFT2
typedef enum
{
   eAMP_WIMAX_LINK_HO_IP_REFRESH_NOT_REQUIRED = 0, // refresh not required, but still allowed
   eAMP_WIMAX_LINK_HO_IP_REFRESH_REQUIRED = 1 /*  MS shall trigger higher layer protocol to refresh 
                          its traffic IP address (e.g. DHCP Discover [IETF 
                          RFC 2131] or Mobile IPv4 re-registration [IETF RFC 
                          3344]). */
} AMP_WIMAX_LINK_HO_IP_REFRESH_CODE_E;

// AMP CoCo SPFT2
typedef enum
{
    eAMP_WIMAX_LINK_HO_NOT_SUPPORTED_BY_MS = 0, //only for SPFT1, when HO is not supported yet
    eAMP_WIMAX_LINK_HO_NOT_ALLOWED_BY_RA = 1,   // This code will be returned if HO flag is not set in RA_CONNECT request
    eAMP_WIMAX_LINK_HO_NOT_SUPPORTED_BY_BS = 2  // BS does not support HO
} AMP_WIMAX_LINK_HO_REJECT_CODE_E;

// AMP CoCo SPFT2
typedef enum
{
    eAMP_WIMAX_LINK_HO_STATUS_SUCCESS = 0,  // successful HO to Target BS
    eAMP_WIMAX_LINK_HO_STATUS_FAILURE = 1   // failed HO, back at Serving BS
}AMP_WIMAX_LINK_HO_SUCCESS_STATUS_E;

// AMP CoCo SPFT2
typedef enum
{
    eAMP_WIMAX_LINK_DISCONNECT_GRACEFULLY_DISCONNECT_FROM_NETWORK = 0,
    eAMP_WIMAX_LINK_DISCONNECT_GRACELESS_EXIT_IMMEDIATELY = 1
} AMP_WIMAX_LINK_DISCONNECT_CODE_E;

// AMP CoCo SPFT3
//NSP information status of this NAP. Values:
typedef enum
{
    eAMP_WIMAX_LINK_NSP_STATE_UNKNOWN = 0,
    eAMP_WIMAX_LINK_NSP_NO_NAP_SHARING = 1,
    eAMP_WIMAX_LINK_NSP_NAP_SHARING_ENABLED_NSP_CHANGE_COUNT_UNKNOWN_NSP_LIST_UNKNOWN = 2,
    eAMP_WIMAX_LINK_NSP_NAP_SHARING_ENABLED_NSP_CHANGE_COUNT_VALID_NSP_LIST_UNKNOWN = 3,
    eAMP_WIMAX_LINK_NSP_NAP_SHARING_ENABLED_NSP_CHANGE_COUNT_VALID_NSP_LIST_KNOWN = 4
} AMP_WIMAX_LINK_NSP_INFO_STATUS_E;


// WiP service flow and bandwidth definitions

// IEEE 802.16 2004
// BS confirmation code defined in MS-BS Layer 2 Messages and Procedures ICD, R01.00.00, 1/31.06
typedef enum
{
    eAMP_WIMAX_SF_CC_CODE_UNKNOWN = -1,   
    eAMP_WIMAX_SF_CC_CODE_OK = 0,       // DSx-RSP, DSx-ACKOK/Success
    eAMP_WIMAX_SF_CC_CODE_REJ_OTHER = 1,        // DSx-RSP, DSx-ACK Rejection due to any other cause not listed in this table
    eAMP_WIMAX_SF_CC_CODE_REJ_UNRECOG_CFG = 2,      // reject-unrecognized-configuration-setting
    eAMP_WIMAX_SF_CC_CODE_REJ_RESOURCE = 3,     // DSA-RSP, DSC-RSP Rejection due to Resources not available, reject-temporary
    eAMP_WIMAX_SF_CC_CODE_REJ_ADMIN = 4,        // DSx-RSP, DSx-ACK Rejection due to administrative reasons, reject-permanent
    eAMP_WIMAX_SF_CC_CODE_REJ_NOT_OWNER = 5,    // reject-not-owner
    eAMP_WIMAX_SF_CC_CODE_REJ_SF_NOT_FOUND = 6,     // DSC-RSP, DSD-RSP, DSx-ACK Rejection due to Service flow with requested SFID being not found
    eAMP_WIMAX_SF_CC_CODE_REJ_SFID_USED = 7,        // DSA-RSP Rejection due to SFID being used by an Existing service Flow
    eAMP_WIMAX_SF_CC_CODE_REJ_MAND_PARAMS = 8,      // Rejection due to a Mandatory parameter being missing from the DSx-Req or DSx-RSP message
    eAMP_WIMAX_SF_CC_CODE_REJ_PHS = 9,      // DSA-RSP, DSC-RSP Rejection due to PHS being not supported/available but being requested in the CS parameters
    eAMP_WIMAX_SF_CC_CODE_REJ_UNKNOWN_TRANS_ID = 10,        // DSx-ACK Rejection due to Unknown Transaction ID in the DSx-RSP message
    eAMP_WIMAX_SF_CC_CODE_REJ_AUTH_FAIL = 11,       // reject-authentication-failure
    eAMP_WIMAX_SF_CC_CODE_REJ_ADD_ABORT = 12,       // DSA-ACK Rejection due to abort during the service flow addition procedure
    eAMP_WIMAX_SF_CC_CODE_REJ_NUM_LIMIT = 13,       // DSA-RSPRejection due to number of Dynamic Service Flows exceed the preconfigured limit.
    eAMP_WIMAX_SF_CC_CODE_REJ_NO_AUTH = 14,     // reject-not-authorized-for-the-requested-SAID
    eAMP_WIMAX_SF_CC_CODE_REJ_SAID = 15,        // DSA-RSP, DSC-RSP Rejection due unknown Target SAID (i.e. not an established SA), reject-fail-to-establish-the-requested-SA
    eAMP_WIMAX_SF_CC_CODE_REJ_PARAMS_NO_SUPPORT = 16,       // DSA-RSP, DSC-RSP Rejection due to requested parameter being not supported. The TLVs being rejected shall1 be sent in the DSx-RSP message
    eAMP_WIMAX_SF_CC_CODE_REJ_PARAMS_NO_SUPPORT_VALUE = 17,     // DSA-RSP, DSC-RSP, DSA-ACK, DSC-ACK Rejection due to requested parameter value being not supported.The TLVs being rejected shall1 be sent with the closest value that issupported in the DSx-RSP and DSx-ACK messages
    eAMP_WIMAX_SF_CC_CODE_MAX_VAL = 255
} AMP_WIMAX_SF_CC_CODE_E;

// AMP SFM SPFT2
typedef enum
{
    eAMP_WIMAX_SF_ADD_STATUS_UNKNOWN = -1,   
    eAMP_WIMAX_SF_ADD_STATUS_OK = eAMP_WIMAX_SF_CC_CODE_OK, //0, 
    eAMP_WIMAX_SF_ADD_STATUS_REJ_BY_BS = 1,     // Reject by BS; see Remote CC for the BS's Confirmation Code.
    eAMP_WIMAX_SF_ADD_STATUS_REJ_RESOURCE = eAMP_WIMAX_SF_CC_CODE_REJ_RESOURCE, //3,    // lack of resources.
    eAMP_WIMAX_SF_ADD_STATUS_REJ_MAND_PARAMS = eAMP_WIMAX_SF_CC_CODE_REJ_MAND_PARAMS, //8,  // required parameter not present.
    eAMP_WIMAX_SF_ADD_STATUS_REJ_PARAMS_NO_SUPPORT = eAMP_WIMAX_SF_CC_CODE_REJ_PARAMS_NO_SUPPORT, //16,     // not supported parameter.?
    eAMP_WIMAX_SF_ADD_STATUS_REJ_PARAMS_NO_SUPPORT_VALUE = eAMP_WIMAX_SF_CC_CODE_REJ_PARAMS_NO_SUPPORT_VALUE, //17,     // not supported parameter value.
    eAMP_WIMAX_SF_ADD_STATUS_FAIL = 81,     // retry limit exceeded.
    eAMP_WIMAX_SF_ADD_STATUS_MAX_VAL
} AMP_WIMAX_SF_ADD_STATUS_E;

// AMP SFM SPFT2
typedef enum
{
    eAMP_WIMAX_SF_CHANGE_STATUS_UNKNOWN = -1,   
    eAMP_WIMAX_SF_CHANGE_STATUS_OK = eAMP_WIMAX_SF_CC_CODE_OK, //0, 
    eAMP_WIMAX_SF_CHANGE_STATUS_REJ_BY_BS = 1,  // Reject by BS; see Remote CC for the BS's Confirmation Code.
    eAMP_WIMAX_SF_CHANGE_STATUS_REJ_RESOURCE = eAMP_WIMAX_SF_CC_CODE_REJ_RESOURCE, //3,     // lack of resources.
    eAMP_WIMAX_SF_CHANGE_STATUS_REJ_SFID = eAMP_WIMAX_SF_CC_CODE_REJ_SF_NOT_FOUND, //6,     // SFID unknown.
    eAMP_WIMAX_SF_CHANGE_STATUS_REJ_MAND_PARAMS = eAMP_WIMAX_SF_CC_CODE_REJ_MAND_PARAMS, // 8,  // required parameter not present.
    eAMP_WIMAX_SF_CHANGE_STATUS_REJ_PARAMS_NO_SUPPORT = eAMP_WIMAX_SF_CC_CODE_REJ_PARAMS_NO_SUPPORT, // 16,     // not supported parameter.?
    eAMP_WIMAX_SF_CHANGE_STATUS_REJ_PARAMS_NO_SUPPORT_VALUE = eAMP_WIMAX_SF_CC_CODE_REJ_PARAMS_NO_SUPPORT_VALUE, //17,  // not supported parameter value.
    eAMP_WIMAX_SF_CHANGE_STATUS_FAIL = 81,  // retry limit exceeded.
    eAMP_WIMAX_SF_CHANGE_STATUS_ABORT_HOST_DELETE = 82,     // by host-initiated delete.
    eAMP_WIMAX_SF_CHANGE_STATUS_ABORT_BS_CHANGE = 83,   // by BS-initiated change.
    eAMP_WIMAX_SF_CHANGE_STATUS_ABORT_BS_DELETE = 84,   // by BS-initiated delete.
    eAMP_WIMAX_SF_CHANGE_STATUS_ABORT_HOST_CHANGE = 85,     // by host-initiated change.
    eAMP_WIMAX_SF_CHANGE_STATUS_MAX_VAL
} AMP_WIMAX_SF_CHANGE_STATUS_E;

// AMP SFM SPFT2
typedef enum
{
    eAMP_WIMAX_SF_DELETE_STATUS_UNKNOWN = -1,   
    eAMP_WIMAX_SF_DELETE_STATUS_OK = eAMP_WIMAX_SF_CC_CODE_OK, //0, 
    eAMP_WIMAX_SF_DELETE_STATUS_REJ_BY_BS = 1,  // Reject by BS; see Remote CC for the BS's Confirmation Code.
    eAMP_WIMAX_SF_DELETE_STATUS_REJ_RESOURCE = eAMP_WIMAX_SF_CC_CODE_REJ_RESOURCE, //3,     // lack of resources.
    eAMP_WIMAX_SF_DELETE_STATUS_REJ_SFID = eAMP_WIMAX_SF_CC_CODE_REJ_SF_NOT_FOUND, //6,     // SFID unknown.
    eAMP_WIMAX_SF_DELETE_STATUS_REJ_MAND_PARAMS = eAMP_WIMAX_SF_CC_CODE_REJ_MAND_PARAMS, //8,   // required parameter not present.
    eAMP_WIMAX_SF_DELETE_STATUS_FAIL = 81,  // retry limit exceeded. (Locally the SF has been deleted, but at the BS it may still exist.)
    eAMP_WIMAX_SF_DELETE_STATUS_MAX_VAL
} AMP_WIMAX_SF_DELETE_STATUS_E;

// WiMAX ICD-009, 1/18/2006
typedef enum
{
    eAMP_WIMAX_SF_STATE_UNKNOWN = -1,
    eAMP_WIMAX_SF_STATE_PROVISIONED = 0,
    eAMP_WIMAX_SF_STATE_ACTIVATE = 1,
    eAMP_WIMAX_SF_STATE_DEFAULT = 2,
    eAMP_WIMAX_SF_STATE_RESERVED_MIN = 3,
    eAMP_WIMAX_SF_STATE_RESERVED_MAX = 7
} AMP_WIMAX_SF_STATE_E;


// AMP SFM SPFT2
typedef enum
{
    eAMP_WIMAX_SF_DIRECTION_UNKNOWN = -1,   
    eAMP_WIMAX_SF_DIRECTION_UPLINK = 0, 
    eAMP_WIMAX_SF_DIRECTION_DNLINK = 1, 
    eAMP_WIMAX_SF_DIRECTION_MAX_VAL
} AMP_WIMAX_SF_DIRECTION_E;

// service flow TLV, 802.16e 2005, cor-1
// 0: No available MBS
// 1: Single-BS-MBS
// 2: Multi-BS-MBS
typedef enum
{
    eAMP_WIMAX_SF_MBS_SERVICE_UNKNOWN = -1,
    eAMP_WIMAX_SF_MBS_SERVICE_NONE = 0,
    eAMP_WIMAX_SF_MBS_SERVICE_SINGLE = 1,
    eAMP_WIMAX_SF_MBS_SERVICE_MULTI = 2
} AMP_WIMAX_SF_MBS_SERVICE_E;

// service flow TLV, 802.16e D12
//Bit 0: Provisioned Set
//Bit 1: Admitted Set
//Bit 2: Active Set
//Bits 3–7: Reserved
typedef enum
{
    eAMP_WIMAX_SF_QOS_PARAMS_SET_UNKNOWN = -1,
    eAMP_WIMAX_SF_QOS_PARAMS_SET_Provisioned = 0x1,
    eAMP_WIMAX_SF_QOS_PARAMS_SET_Admitted = 0x2,
    eAMP_WIMAX_SF_QOS_PARAMS_SET_Active = 0x4
} AMP_WIMAX_SF_QOS_PARAMS_SET_E;

// service flow TLV, 802.16e 2005, cor-1
//0: Continuing Grant Service
//1: Real Time–Variable Rate Service
//2: Non-Real Time–Variable Rate Service
//3: Best Efforts Service
//4: Extended Real-Time Variable Rate Service
typedef enum
{
    eAMP_WIMAX_SF_TYPE_OF_DATA_DELIVERY_SERVICES_UNKNOWN = -1,
    eAMP_WIMAX_SF_TYPE_OF_DATA_DELIVERY_SERVICES_CGS = 0,
    eAMP_WIMAX_SF_TYPE_OF_DATA_DELIVERY_SERVICES_RTVRS = 1,
    eAMP_WIMAX_SF_TYPE_OF_DATA_DELIVERY_SERVICES_NRTVRS = 2,
    eAMP_WIMAX_SF_TYPE_OF_DATA_DELIVERY_SERVICES_BES = 3,
    eAMP_WIMAX_SF_TYPE_OF_DATA_DELIVERY_SERVICES_ERTVRS = 4
} AMP_WIMAX_SF_TYPE_OF_DATA_DELIVERY_SERVICES_E;

// service flow TLV, 802.16e 2005, cor-1
typedef enum
{
    eAMP_WIMAX_SF_SCHEDULER_UNKNOWN = -1,
    eAMP_WIMAX_SF_SCHEDULER_RESERVED = 0,
    eAMP_WIMAX_SF_SCHEDULER_UNDEFINED = 1,
    eAMP_WIMAX_SF_SCHEDULER_BE = 2,         // Best Effort
    eAMP_WIMAX_SF_SCHEDULER_NRTPS = 3,          // Non-real-time Polling Service
    eAMP_WIMAX_SF_SCHEDULER_RTPS = 4,           // Real-time Polling Service
    eAMP_WIMAX_SF_SCHEDULER_ERTPS = 5,          // Extended Real-Time Polling Service   
    eAMP_WIMAX_SF_SCHEDULER_UGS = 6,            // Unsolicited Grant Service
    eAMP_WIMAX_SF_SCHEDULER_MAX_VAL
} AMP_WIMAX_SF_SCHEDULER_TYPE_E;

// Convergence Sublayer type, 802.16e 2005 Cor1
typedef enum
{  
    eAMP_WIMAX_SF_CLA_RULE_TYPE_RULE_PRIORITY = 1,      
    eAMP_WIMAX_SF_CLA_RULE_TYPE_IP_DSCP = 2,        
    eAMP_WIMAX_SF_CLA_RULE_TYPE_IP_PROT = 3, 
    eAMP_WIMAX_SF_CLA_RULE_TYPE_IP_SRC_ADDR_MASK = 4,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_IP_DEST_ADDR_MASK = 5,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_IP_SRC_PORT_RANGE = 6,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_IP_DEST_PORT_RANGE = 7,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_IP_DEST_MAC_ADDR = 8,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_IP_SRC_MAC_ADDR = 9,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_SAP = 10,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_8021D_USER_PRIORITY = 11,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_8021Q_VLAN_ID = 12,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_PHSI = 13,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_CLASSIFIER_RULE_INDEX = 14,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_IPV6_FLOW_LABEL = 15,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_ROHC_LARGE_CONTEXT_ID = 16,     // 0–65535: Context ID
    eAMP_WIMAX_SF_CLA_RULE_TYPE_17 = 17,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_ROHC_SHORT_CONTEXT_ID = 18,     // 0-15: Context ID
    eAMP_WIMAX_SF_CLA_RULE_TYPE_CLA_ACTION_RULE = 19,
    eAMP_WIMAX_SF_CLA_RULE_TYPE_MAX_VAL
    //eAMP_WIMAX_SF_CLA_RULE_TYPE_VENDOR_SPECIFIC_PARAM = 143       // not supported
} AMP_WIMAX_SF_CLA_RULE_TYPE_E;

// 802.16e 2005 cor-1
//0 — DSC Add Classifier
//1 — DSC Replace Classifier
//2 — DSC Delete Classifier
typedef enum
{
    eAMP_WIMAX_SF_CLA_DSC_ACTION_UNKNOWN = -1,   
    eAMP_WIMAX_SF_CLA_DSC_ACTION_ADD = 0,       
    eAMP_WIMAX_SF_CLA_DSC_ACTION_REPLACE = 1,       
    eAMP_WIMAX_SF_CLA_DSC_ACTION_DELETE = 2,       
} AMP_WIMAX_SF_CLA_DSC_ACTION_E;

// 802.16e 2005 cor-1
//0 — Add PHS Rule
//1 — Set PHS Rule
//2 — Delete PHS Rule
//3 — Delete all PHS Rules
typedef enum
{
    eAMP_WIMAX_SF_PHS_DSC_ACTION_UNKNOWN = -1,   
    eAMP_WIMAX_SF_PHS_DSC_ACTION_ADD = 0,       
    eAMP_WIMAX_SF_PHS_DSC_ACTION_SET = 1,       
    eAMP_WIMAX_SF_PHS_DSC_ACTION_DELETE = 2,
    eAMP_WIMAX_SF_PHS_DSC_ACTION_DELETE_ALL = 3
} AMP_WIMAX_SF_PHS_DSC_ACTION_E;

// Convergence Sublayer type, 802.16e 2005 Cor1
typedef enum
{
    eAMP_WIMAX_SF_CS_RESERVED = 0,
    eAMP_WIMAX_SF_CS_PACKET_IPV4 = 1,
    eAMP_WIMAX_SF_CS_PACKET_IPV6 = 2,
    eAMP_WIMAX_SF_CS_PACKET_802_3_ETHERNET = 3,
    eAMP_WIMAX_SF_CS_PACKET_802_1Q_VLAN = 4,
    eAMP_WIMAX_SF_CS_PACKET_IPV4_802_3_ETHERNET = 5,
    eAMP_WIMAX_SF_CS_PACKET_IPV6_802_3_ETHERNET = 6,
    eAMP_WIMAX_SF_CS_PACKET_IPV4_802_1Q_VLAN = 7,
    eAMP_WIMAX_SF_CS_PACKET_IPV6_802_1Q_VLAN = 8,
    eAMP_WIMAX_SF_CS_PACKET_ATM = 9,
    eAMP_WIMAX_SF_CS_PACKET_ETH_ROHC = 10,              // Packet, 802.3/etherneta with ROHC header compression
    eAMP_WIMAX_SF_CS_PACKET_ETH_ECRTP = 11,             // Packet, 802.3/ethernetb with ECRTP header compression
    eAMP_WIMAX_SF_CS_PACKET_IP2_ROHC = 12,              // Packet, IP2 with ROHC header compression
    eAMP_WIMAX_SF_CS_PACKET_IP2_ECRTP = 13,             // Packet, IP2 with ECRTP header compression
    eAMP_WIMAX_SF_CS_MAX_VAL
} AMP_WIMAX_SF_CS_TYPE_E;

typedef enum
{
    eAMP_WIMAX_SF_CS_GPCS_PROTOCOL_TYPE_UNKNOWN = -1,
    eAMP_WIMAX_SF_CS_GPCS_PROTOCOL_TYPE_ETHERNET = 0x0000,
    eAMP_WIMAX_SF_CS_GPCS_PROTOCOL_TYPE_MPLS = 0x0001,
    eAMP_WIMAX_SF_CS_GPCS_PROTOCOL_TYPE_PPP = 0x0002,
    eAMP_WIMAX_SF_CS_GPCS_PROTOCOL_TYPE_RAW_IP = 0x0003,
    eAMP_WIMAX_SF_CS_GPCS_PROTOCOL_TYPE_ECRTP = 0x0004
} AMP_WIMAX_SF_CS_GPCS_PROTOCOL_TYPE_E;

// service flow TLV, 802.16e 2005 Cor-1
//Bit #0 – Service flow shall not use broadcast bandwidth request opportunities. (Uplink only)
//Bit #1 – Service flow shall not use multicast bandwidth request opportunities. (Uplink only)
//Bit #2 – The service flow shall not piggyback requests with data. (Uplink only)
//Bit #3 – The service flow shall not fragment data.
//Bit #4 – The service flow shall not suppress payload headers (CS parameter), If bit #4 is set to’0’ and both the SS and the BS support
        //PHS (according to section 11.7.7.3), each SDU for this service flow shall be prefixed by a PHSI
        //field, which may be set to 0 (see section 5.2). If bit #4 is set to ‘1’, none of the SDUs for this service
        //flow will have a PHSI field.
//Bit #5 – The service flow shall not pack multiple SDUs (or fragments) into single MAC PDUs.
//Bit #6 – The service flow shall not include CRC in the MAC PDU.
//Bit #7 – Reserved; shall be set to zero
typedef enum
{
    eAMP_WIMAX_SF_REQ_TX_POLICY_NO_BROADCAST_BW_REQ = 0x1,      // uplink only
    eAMP_WIMAX_SF_REQ_TX_POLICY_NO_MULTICAST_BW_REQ = 0x2,      // uplink only
    eAMP_WIMAX_SF_REQ_TX_POLICY_NO_PIGGYBACK = 0x4,
    eAMP_WIMAX_SF_REQ_TX_POLICY_NO_FRAGMENT = 0x8,
    eAMP_WIMAX_SF_REQ_TX_POLICY_NO_PHS = 0x10,
    eAMP_WIMAX_SF_REQ_TX_POLICY_NO_PACK_SDU = 0x20,
    eAMP_WIMAX_SF_REQ_TX_POLICY_NO_CRC = 0x40,
    eAMP_WIMAX_SF_REQ_TX_POLICY_NO_RESERVED7 = 0x80     // should be zero
} AMP_WIMAX_SF_REQ_TRANSMISSION_POLICY_E;

// service flow TLV, 802.16e 2005, cor-1
// 0-15: Reserved
// 1–204016, 32, 64, 128, 256, 512, or 1024 or 2048: Desired/Agreed size in bytes
// 2041-65535: Reserved
typedef enum
{
    eAMP_WIMAX_SF_ARQ_BLOCK_SIZE_16 = 16,
    eAMP_WIMAX_SF_ARQ_BLOCK_SIZE_32 = 32,
    eAMP_WIMAX_SF_ARQ_BLOCK_SIZE_64 = 64,
    eAMP_WIMAX_SF_ARQ_BLOCK_SIZE_128 = 128,
    eAMP_WIMAX_SF_ARQ_BLOCK_SIZE_256 = 256,
    eAMP_WIMAX_SF_ARQ_BLOCK_SIZE_512 = 512,
    eAMP_WIMAX_SF_ARQ_BLOCK_SIZE_1024 = 1024
} AMP_WIMAX_SF_ARQ_BLOCK_SIZE_E;

// service flow TLV, 802.16e D12
//0 = variable-length SDUs
//1 = fixed-length SDUs
//default = 0
typedef enum
{
    eAMP_WIMAX_SF_SDU_LEN_IND_VARIABLE = 0,
    eAMP_WIMAX_SF_SDU_LEN_IND_FIXED = 1
} AMP_WIMAX_SF_SDU_LEN_IND_E;

// service flow TLV, 802.16e 2005, cor-1
// 0: No traffic indication
// 1: Traffic indication
typedef enum
{
    eAMP_WIMAX_SF_TRAFFIC_IND_PREF_NONE = 0,
    eAMP_WIMAX_SF_TRAFFIC_IND_PREF_YES = 1
} AMP_WIMAX_SF_TRAFFIC_IND_PREF_E;

// service flow TLV, 802.16e 2005, cor-1
// 0: No paging generation
// 1: Paging generation
typedef enum
{
    eAMP_WIMAX_SF_PAGING_PREF_NONE = 0,
    eAMP_WIMAX_SF_PAGING_PREF_YES = 1
} AMP_WIMAX_SF_PAGING_PREF_E;

// service flow TLV, 802.16e D12
//0 = ARQ Not Requested/Accepted
//1 = ARQ Requested/Accepted
typedef enum
{
    eAMP_WIMAX_SF_ARQ_UNKNOWN = -1,
    eAMP_WIMAX_SF_ARQ_NOT = 0,
    eAMP_WIMAX_SF_ARQ = 1
} AMP_WIMAX_SF_ARQ_E;

// service flow TLV, 802.16e 2005, cor-1
// 0 = 3-bit FSN
// 1 = 11-bit FSN
// Default = 1
typedef enum
{
    eAMP_WIMAX_SF_FSN_SIZE_UNKNOWN = -1,
    eAMP_WIMAX_SF_FSN_SIZE_3_BIT = 0,
    eAMP_WIMAX_SF_FSN_SIZE_11_BIT = 1
} AMP_WIMAX_SF_FSN_SIZE_E;

// service flow TLV, 802.16e 2005, cor-1
// 0–No support for PDU SN in this connection (default)
// 1–PDU SN (short) extended SH
// 2–PDU SN (long) extended SH
// 3-256–Reserved.
typedef enum
{
    eAMP_WIMAX_SF_PDU_SN_EXT_HARQ_UNKNOWN = -1,
    eAMP_WIMAX_SF_PDU_SN_EXT_HARQ_NONE = 0,
    eAMP_WIMAX_SF_PDU_SN_EXT_HARQ_SHORT = 1,
    eAMP_WIMAX_SF_PDU_SN_EXT_HARQ_LONG = 2
} AMP_WIMAX_SF_PDU_SN_EXT_HARQ_E;

// service flow TLV, 802.16e D12
//0 – Order of delivery is not preserved
//1 – Order of delivery is preserved
typedef enum
{
    eAMP_WIMAX_SF_ARQ_ORDER_UNKNOWN = -1,
    eAMP_WIMAX_SF_ARQ_ORDER_NOT_PRESERVED = 0,
    eAMP_WIMAX_SF_ARQ_ORDER_PRESERVED = 1
} AMP_WIMAX_SF_ARQ_ORDER_E;

// AMP PSC SPFT3
typedef enum
{
    eAMP_WIMAX_PSC_STATUS_UNKNOWN = -1,
    eAMP_WIMAX_PSC_STATUS_SUCCESS = 0,
    eAMP_WIMAX_PSC_STATUS_REJ_FORMAT_ERROR = 1,
    eAMP_WIMAX_PSC_STATUS_REJ_NO_LINK = 2,
    eAMP_WIMAX_PSC_STATUS_REJ_NO_BS_SUPPORT = 3,
    eAMP_WIMAX_PSC_STATUS_REJ_SFID_INCORRECT = 4,
    eAMP_WIMAX_PSC_STATUS_REJ_UNKNOWN_PSC_ID = 5,
    eAMP_WIMAX_PSC_STATUS_REJ_NO_RSP = 6,
    eAMP_WIMAX_PSC_STATUS_FAIL_BS_DISAPPREVES = 7,
    eAMP_WIMAX_PSC_STATUS_FAIL_NO_RESPONSE = 8,
    eAMP_WIMAX_PSC_STATUS_MAX_VAL
} AMP_WIMAX_PSC_STATUS_E;


// Prepare for classification rules, 802.16e 2005 Cor1
typedef struct
{
    UINT32 u32NumOfEntry;
    UINT8 u8Entry[1];
} AMP_WIMAX_ONE_BYTE_ARRAY_S;

typedef struct
{
    UINT32 u32NumOfEntry;
    UINT16 u16Entry[1];
} AMP_WIMAX_TWO_BYTE_ARRAY_S;

typedef struct
{
    UINT32 u32NumOfEntry;
    UINT32 u32Entry[1];
} AMP_WIMAX_FOUR_BYTE_ARRAY_S;

typedef struct
{
    UINT8 u8Entry1;
    UINT8 u8Entry2;
}AMP_WIMAX_TWO_ONE_BYTE_ARRAY_S;

typedef struct
{
    UINT16 u16Entry1;
    UINT16 u16Entry2;
} AMP_WIMAX_DOUBLE_TWO_BYTE_S;

typedef struct
{
    UINT32 u32NumOfEntry;
    AMP_WIMAX_DOUBLE_TWO_BYTE_S sEntry[1];
} AMP_WIMAX_DOUBLE_TWO_BYTE_ARRAY_S;


typedef struct
{
    UINT32 u32Entry1;
    UINT32 u32Entry2;
} AMP_WIMAX_DOUBLE_FOUR_BYTE_S;

typedef struct
{
    UINT32 u32NumOfEntry;
    AMP_WIMAX_DOUBLE_FOUR_BYTE_S sEntry[1];
} AMP_WIMAX_DOUBLE_FOUR_BYTE_ARRAY_S;


typedef struct
{
    UINT8 u8Entry1[4];
    UINT8 u8Entry2[4];
} AMP_WIMAX_DOUBLE_FOUR_ONE_BYTE_S;

typedef struct
{
    UINT32 u32NumOfEntry;
    AMP_WIMAX_DOUBLE_FOUR_ONE_BYTE_S sEntry[1];
} AMP_WIMAX_DOUBLE_FOUR_ONE_BYTE_ARRAY_S;

typedef struct
{
    UINT8 u8Entry1[6];
    UINT8 u8Entry2[6];
} AMP_WIMAX_DOUBLE_SIX_BYTE_S;

typedef struct
{
    UINT32 u32NumOfEntry;
    AMP_WIMAX_DOUBLE_SIX_BYTE_S sEntry[1];
} AMP_WIMAX_DOUBLE_SIX_BYTE_ARRAY_S;

typedef struct
{
    UINT8 u8Entry1[16];
    UINT8 u8Entry2[16];
} AMP_WIMAX_DOUBLE_SIXTEEN_BYTE_S;

typedef struct
{
    UINT32 u32NumOfEntry;
    AMP_WIMAX_DOUBLE_SIXTEEN_BYTE_S sEntry[1];
} AMP_WIMAX_DOUBLE_SIXTEEN_BYTE_ARRAY_S;

// 802.16e 2005 Cor1
#define AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY (0x0000000000000001LL)     /// 1
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_DSCP (0x0000000000000002LL)   /// 2
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_PROT (0x0000000000000004LL)   // 3
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_ADDR_MASK (0x0000000000000008LL)  // 4
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_ADDR_MASK (0x0000000000000010LL) // 5
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_PORT_RANGE (0x0000000000000020LL) // 6
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_PORT_RANGE (0x0000000000000040LL)    // 7
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_MAC_ADDR (0x0000000000000080LL)  // 8
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_MAC_ADDR (0x0000000000000100LL)   // 9
#define AMP_WIMAX_SF_CLA_RULE_MASK_SAP (0x0000000000000200LL)   /// 10
#define AMP_WIMAX_SF_CLA_RULE_MASK_8021D_USER_PRIORITY (0x0000000000000400LL)   /// 11
#define AMP_WIMAX_SF_CLA_RULE_MASK_8021Q_VLAN_ID (0x0000000000000800LL) /// 12
#define AMP_WIMAX_SF_CLA_RULE_MASK_PHSI (0x0000000000001000LL)  /// 13
#define AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX (0x0000000000002000LL) /// 14
#define AMP_WIMAX_SF_CLA_RULE_MASK_IPV6_FLOW_LABEL (0x0000000000004000LL)   // 15
#define AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_LARGE_CONTEXT_ID (0x0000000000008000LL) // 16
#define AMP_WIMAX_SF_CLA_RULE_MASK_17 (0x0000000000010000LL)
#define AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_SHORT_CONTEXT_ID (0x0000000000020000LL) // 18
#define AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE (0x0000000000040000LL)   // 19
#define AMP_WIMAX_SF_CLA_RULE_MASK_VENDOR (0x8000000000000000LL)    /// 143

#ifdef CONFIG_LTE
//#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_DSCP_MASK_TYPE (0x0000000000800000LL)   // 20
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_PORT_TYPE (0x0000000001000000LL) // 21
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_PORT_TYPE (0x0000000002000000LL)  // 22
#define AMP_WIMAX_SF_CLA_RULE_MASK_IP_NEXT_HEADER_TYPE (0x0000000004000000LL)   // 23
#endif

// 802.16e 2005 Cor1
// The negative length means variable length up to the number as maximum
#define AMP_WIMAX_SF_CLA_RULE_LEN_RULE_PRIORITY (1)     /// 1, 0-255
#define AMP_WIMAX_SF_CLA_RULE_LEN_IP_DSCP (3)   /// 2
#define AMP_WIMAX_SF_CLA_RULE_LEN_IP_PROT (1)   // 3
#define AMP_WIMAX_SF_CLA_RULE_LEN_IP_SRC_ADDR_MASK (-32)    // 4, 8 for IPv4, 32 for IPv6
#define AMP_WIMAX_SF_CLA_RULE_LEN_IP_DEST_ADDR_MASK (-32)   // 5, 8 for IPv4, 32 for IPv6
#define AMP_WIMAX_SF_CLA_RULE_LEN_IP_SRC_PORT_RANGE (4) // 6
#define AMP_WIMAX_SF_CLA_RULE_LEN_IP_DEST_PORT_RANGE (4)    // 7
#define AMP_WIMAX_SF_CLA_RULE_LEN_IP_DEST_MAC_ADDR (12) // 8
#define AMP_WIMAX_SF_CLA_RULE_LEN_IP_SRC_MAC_ADDR (12)  // 9
#define AMP_WIMAX_SF_CLA_RULE_LEN_SAP (3)   /// 10
#define AMP_WIMAX_SF_CLA_RULE_LEN_8021D_USER_PRIORITY (2)   /// 11
#define AMP_WIMAX_SF_CLA_RULE_LEN_8021Q_VLAN_ID (2) /// 12
#define AMP_WIMAX_SF_CLA_RULE_LEN_PHSI (1)  /// 13
#define AMP_WIMAX_SF_CLA_RULE_LEN_CLASSIFIER_RULE_INDEX (2) /// 14
#define AMP_WIMAX_SF_CLA_RULE_LEN_IPV6_FLOW_LABEL (3)   // 15
#define AMP_WIMAX_SF_CLA_RULE_LEN_ROHC_LARGE_CONTEXT_ID (2) // 16, 0-65535
#define AMP_WIMAX_SF_CLA_RULE_LEN_17 (0)    // 17
#define AMP_WIMAX_SF_CLA_RULE_LEN_ROHC_SHORT_CONTEXT_ID (1) // 18, 0-15
//Bit #0:
//0 = none.
//1 = Discard packet
//Bit #1-7: Reserved.
#define AMP_WIMAX_SF_CLA_RULE_LEN_CLA_ACTION_RULE (1)   // 19
#define AMP_WIMAX_SF_CLA_RULE_LEN_VENDOR (-4)   /// 143, variable
#define AMP_WIMAX_SF_CLA_RULE_MAX_LEN_VENDOR (4)

// 802.16e 2005 Cor1
#define AMP_WIMAX_SF_CLA_TLV_MASK_REQUIRED (0xFFFFFFFF)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_ALL (0xFFFFFFFF)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE (0)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_EMPTY (AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_IPV4 ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DSCP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_PROT \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_IPV6  ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DSCP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_PROT \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IPV6_FLOW_LABEL \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_802_3_ETHERNET ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_SAP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021D_USER_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021Q_VLAN_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_802_1Q_VLAN ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_SAP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021D_USER_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021Q_VLAN_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_IPV4_802_3_ETHERNET ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DSCP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_PROT \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_SAP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021D_USER_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021Q_VLAN_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_IPV6_802_3_ETHERNET ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DSCP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_PROT \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_SAP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021D_USER_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021Q_VLAN_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IPV6_FLOW_LABEL \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_IPV4_802_1Q_VLAN ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DSCP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_PROT \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_SAP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021D_USER_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021Q_VLAN_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IPV6_FLOW_LABEL \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_LARGE_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_SHORT_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_IPV6_802_1Q_VLAN ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DSCP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_PROT \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_ADDR_MASK \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_PORT_RANGE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_DEST_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IP_SRC_MAC_ADDR \
            | AMP_WIMAX_SF_CLA_RULE_MASK_SAP \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021D_USER_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_8021Q_VLAN_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_IPV6_FLOW_LABEL \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_LARGE_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_SHORT_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_ATM ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_ETH_ROHC ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_LARGE_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_SHORT_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_ETH_ECRTP ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_LARGE_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_SHORT_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_IP2_ROHC ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_LARGE_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_SHORT_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)
#define AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_PACKET_IP2_ECRTP ( \
            AMP_WIMAX_SF_CLA_TLV_MASK_PERMITTED_BY_SS_MAC_NONE \
            | AMP_WIMAX_SF_CLA_RULE_MASK_RULE_PRIORITY \
            | AMP_WIMAX_SF_CLA_RULE_MASK_PHSI \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLASSIFIER_RULE_INDEX \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_LARGE_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_ROHC_SHORT_CONTEXT_ID \
            | AMP_WIMAX_SF_CLA_RULE_MASK_CLA_ACTION_RULE)


// IEEE 802.16e 2005 Cor-1
// Layer 2 ICD R01.00.05
// CS parameter encoding rules: For default flows an empty Packet Classification
//Rule is sent to the MS. For all other non-default pre-provisioned flows a
//valid Packet Classification Rule (as configured for the service class name in
//the NMS) must be sent to the MS. BS initiated service flows will define one
//and only one Classifier Rule
// 
// If PHS is enabled according to Request/transmission policy then all SDUs on this flow
//will be prepended with a PHSI field. The PHSI field value of the SDU shall be set to 0 until
//the PHS rule is complete. A complete PHS rule requires that PHSI, PHSM, PHSS and
//PHSF be defined. Once the rule is complete, the sender may use the PHSI value assigned
//during PHS rule creation.
//
// structures for classification rules defined in 802.16e 2005 Cor1
typedef struct
{
    UINT8 u8RulePriority;       // 0-255
    UINT8 u8Pad1[3];
    UINT8 u8DscpLow;
    UINT8 u8DscpHigh;
    UINT8 u8DscpMask;
    UINT8 u8Pad2;
    AMP_WIMAX_ONE_BYTE_ARRAY_S sIpProt;
    //AMP_WIMAX_DOUBLE_FOUR_BYTE_ARRAY_S sIpSrcAddrMask;
    //AMP_WIMAX_DOUBLE_FOUR_BYTE_ARRAY_S sIpDestAddrMask;
    AMP_WIMAX_DOUBLE_SIXTEEN_BYTE_ARRAY_S sIpSrcAddrMask;
    AMP_WIMAX_DOUBLE_SIXTEEN_BYTE_ARRAY_S sIpDestAddrMask;
    AMP_WIMAX_DOUBLE_TWO_BYTE_ARRAY_S sIpSrcPort;
    AMP_WIMAX_DOUBLE_TWO_BYTE_ARRAY_S sIpDestPort;
    AMP_WIMAX_DOUBLE_SIX_BYTE_ARRAY_S sDestMacAddr;
    AMP_WIMAX_DOUBLE_SIX_BYTE_ARRAY_S sSrcMacAddr;
    UINT8 u8SapType;
    UINT8 u8SapEprot1;
    UINT8 u8SapEprot2;
    UINT8 u8Pad3;
    UINT8 u8Ieee8021DUserPriorityLow;
    UINT8 u8Ieee8021DUserPriorityHigh;
    UINT8 u8Pad4[2];
    UINT8 u8VlanId1;
    UINT8 u8VlanId2;
    UINT8 u8Pad5[2];
    UINT8 u8Phsi;
    UINT8 u8Pad6[3];
    UINT16 u16ClassifierRuleIndex;
    UINT8 u8Pad7[2];
    UINT8 u8Ipv6FlowLabel[3];       // n*3
    UINT8 u8Pad8;
    UINT16 u16RohcLargeContextId;
    UINT8 u8Pad9[2];
    UINT8 u8RohcShortContextId;
    UINT8 u8Pad10[3];
    UINT8 u8ClaActionRule;
    UINT8 u8Pad11[3];
    UINT8 u8VendorSpecificClassifierParameters[AMP_WIMAX_SF_CLA_RULE_MAX_LEN_VENDOR];
} AMP_WIMAX_SF_CLA_RULE_S;

// 802.16e 2005 Cor2
#define AMP_WIMAX_SF_PHS_RULE_MASK_PHSI (0x0000000000000001LL)      /// 1
#define AMP_WIMAX_SF_PHS_RULE_MASK_PHSF (0x0000000000000002LL)  /// 2
#define AMP_WIMAX_SF_PHS_RULE_MASK_PHSM (0x0000000000000004LL)  // 3
#define AMP_WIMAX_SF_PHS_RULE_MASK_PHSS (0x0000000000000008LL)  // 4
#define AMP_WIMAX_SF_PHS_RULE_MASK_PHSV (0x0000000000000010LL)  // 5
#define AMP_WIMAX_SF_PHS_RULE_MASK_VENDOR (0x8000000000000000LL)    /// 143
                                                                    // 
// Layer 2 ICD R01.00.05
// If PHS is to be used during the lifetime of this flow according to bit 4 of
//wmanIfBsQoSReqTxPolicy then bit 4 of Request/transmission policy shall
//be set to 0 in the DSA-REQ otherwise a value of 1 shall be used. PHS rule
//TLVs will be set in a later DSC-REQ
// structures for PHS defined in 802.16e 2005
#define AMP_WIMAX_MAX_PHSF_LENGTH (256)
#define AMP_WIMAX_MAX_PHSM_LENGTH ((AMP_WIMAX_MAX_PHSF_LENGTH)/8)
typedef struct
{
    UINT8 u8Phsi;   // between 1 and 255, unique per service flow.
    UINT8 u8Phss;   // the total number of bytes in the header to be suppressed
    UINT8 u8Phsv;   // 0 = verify, 1 = don’t verify
    UINT8 u8Pad;
    UINT8 u8Phsf[AMP_WIMAX_MAX_PHSF_LENGTH];        // a string of bytes containing the header information to be 
                            // suppressed by the sending CS and reconstructed by the receiving 
                            // CS. The most significant byte of the string corresponds to the 
                            // first byte of the CS-SDU. The length n shall always be the same as the value for PHSS.
    UINT8 u8Phsm[AMP_WIMAX_MAX_PHSM_LENGTH];        // PHSS/8
} AMP_WIMAX_SF_PHS_RULE_S;

// IEEE 802.16e 2005 Cor-1
#define AMP_WIMAX_SF_TLV_MASK_SFID (0x0000000000000001LL)   /// 1
#define AMP_WIMAX_SF_TLV_MASK_CID (0x0000000000000002LL)    /// 2
#define AMP_WIMAX_SF_TLV_MASK_SERVICE_CLASS_NAME (0x0000000000000004LL) /// 3
#define AMP_WIMAX_SF_TLV_MASK_MBS_SERVICE (0x0000000000000008LL)    // 4
#define AMP_WIMAX_SF_TLV_MASK_QOS_PARAMS_SET_TYPE (0x0000000000000010LL)    /// 5
#define AMP_WIMAX_SF_TLV_MASK_TRAFFIC_PRIORITY (0x0000000000000020LL)   // 6
#define AMP_WIMAX_SF_TLV_MASK_MAX_SUSTAINED_TRAFFIC_RATE (0x0000000000000040LL) // 7
#define AMP_WIMAX_SF_TLV_MASK_MAX_TRAFFIC_BURST (0x0000000000000080LL)  /// 8
#define AMP_WIMAX_SF_TLV_MASK_MIN_RESERVED_TRAFFIC_RATE (0x0000000000000100LL)  // 9
#define AMP_WIMAX_SF_TLV_MASK_RESERVED10 (0x0000000000000200LL)     // 10
#define AMP_WIMAX_SF_TLV_MASK_SF_SCHEDULING_TYPE (0x0000000000000400LL)     // 11  cor2D3 renamed to Uplink Grant Scheduling Type
#define AMP_WIMAX_SF_TLV_MASK_REQ_TRANSMISSION_POLICY (0x0000000000000800LL)    // 12
#define AMP_WIMAX_SF_TLV_MASK_TOLERATED_JITTER (0x0000000000001000LL)   /// 13
#define AMP_WIMAX_SF_TLV_MASK_MAX_LATENCY (0x0000000000002000LL)    // 14
#define AMP_WIMAX_SF_TLV_MASK_SDU_IND (0x0000000000004000LL)    /// 15
#define AMP_WIMAX_SF_TLV_MASK_SDU_SIZE (0x0000000000008000LL)   /// 16
#define AMP_WIMAX_SF_TLV_MASK_TARGET_SAID (0x0000000000010000LL)    // 17
#define AMP_WIMAX_SF_TLV_MASK_ARQ_ENABLE (0x0000000000020000LL)     /// 18
#define AMP_WIMAX_SF_TLV_MASK_ARQ_WINDOW_SIZE (0x0000000000040000LL)    /// 19
#define AMP_WIMAX_SF_TLV_MASK_ARQ_RETRY_TIMEOUT_TX (0x0000000000080000LL)   /// 20
#define AMP_WIMAX_SF_TLV_MASK_ARQ_RETRY_TIMEOUT_RX (0x0000000000100000LL)   /// 21
#define AMP_WIMAX_SF_TLV_MASK_ARQ_BLOCK_LIFETIME (0x0000000000200000LL)     /// 22
#define AMP_WIMAX_SF_TLV_MASK_ARQ_SYNC_LOSS (0x0000000000400000LL)  /// 23
#define AMP_WIMAX_SF_TLV_MASK_ARQ_DELIVER_IN_ORDER (0x0000000000800000LL)   /// 24
#define AMP_WIMAX_SF_TLV_MASK_ARQ_PURGE_TIMEOUT (0x0000000001000000LL)  /// 25
#define AMP_WIMAX_SF_TLV_MASK_ARQ_BLOCK_SIZE (0x0000000002000000LL)     // 26
#define AMP_WIMAX_SF_TLV_MASK_RECEIVER_ARQ_ACK_PROCESSING_TIME (0x0000000004000000LL)   // 27
#define AMP_WIMAX_SF_TLV_MASK_CS_SPECIFICATION (0x0000000008000000LL)   // 28
#define AMP_WIMAX_SF_TLV_MASK_TYPE_OF_DATA_DELIVERY_SERVICES (0x0000000010000000LL) // 29
#define AMP_WIMAX_SF_TLV_MASK_SDU_INTER_ARRIVAL_INTERVAL (0x0000000020000000LL)     // 30
#define AMP_WIMAX_SF_TLV_MASK_TIME_BASE (0x0000000040000000LL)      // 31
#define AMP_WIMAX_SF_TLV_MASK_PAGING_PREFERENCE (0x0000000080000000LL)  // 32
#define AMP_WIMAX_SF_TLV_MASK_MBS_ZONE_IDENTIFIER_ASSIGNMENT (0x0000000100000000LL) // 33
#ifdef IEEE80216e_cor2
#define AMP_WIMAX_SF_TLV_MASK_RESERVED34 (0x0000000200000000LL) //// 34
#else
#define AMP_WIMAX_SF_TLV_MASK_TRAFFIC_INDICATION_PREFERENCE (0x0000000200000000LL)  // 34
#endif
#define AMP_WIMAX_SF_TLV_MASK_GLOBAL_SERVICE_CLASS_NAME (0x0000000400000000LL)  // 35
#define AMP_WIMAX_SF_TLV_MASK_RESERVED36 (0x0000000800000000LL)     // 36
#define AMP_WIMAX_SF_TLV_MASK_SN_FEEDBACK_ENABLED (0x0000001000000000LL)    // 37
#define AMP_WIMAX_SF_TLV_MASK_FSN_SIZE (0x00000002000000000LL)      // 38
#define AMP_WIMAX_SF_TLV_MASK_CID_ALLOCATION_FOR_ACTIVE_BSS (0x0000004000000000LL)  // 39
#define AMP_WIMAX_SF_TLV_MASK_UNSOLICITED_GRANT_INTERVAL (0x0000008000000000LL) // 40
#define AMP_WIMAX_SF_TLV_MASK_UNSOLICITED_POLLING_INTERVAL (0x0000010000000000LL)   // 41
#define AMP_WIMAX_SF_TLV_MASK_PDU_SN_EXT_HARQ (0x0000020000000000LL)    // 42
#define AMP_WIMAX_SF_TLV_MASK_MBS_CONTENTS_ID (0x0000040000000000LL)    // 43
#define AMP_WIMAX_SF_TLV_MASK_HARQ_SERVICE_FLOWS (0x0000080000000000LL) // 44
#define AMP_WIMAX_SF_TLV_MASK_AUTHORIZATION_TOKEN (0x0000100000000000LL)    // 45
#define AMP_WIMAX_SF_TLV_MASK_HARQ_CHANNEL_MAPPING (0x0000200000000000LL)   // 46
#define AMP_WIMAX_SF_TLV_MASK_ROHC_PARAM_PAYLOAD (0x0000400000000000LL) // 47

// IEEE 802.16e 2005 Cor2/D2
// The negative length means variable length up to the number as maximum
// When the length is defined as zero, the TLV will be ignored
#define AMP_WIMAX_SF_TLV_LEN_SFID (4)   /// 1, 1–4 294 967 295
#define AMP_WIMAX_SF_TLV_LEN_CID (2)    /// 2
#define AMP_WIMAX_SF_MAX_LEN_SERVICE_CLASS_NAME (32)    /// 3, 2 to 128 ?????
#define AMP_WIMAX_SF_TLV_LEN_SERVICE_CLASS_NAME (-32)   /// 3, 2 to 128 ?????
#define AMP_WIMAX_SF_TLV_LEN_MBS_SERVICE (1)    // 4, AMP_WIMAX_SF_MBS_SERVICE_E, ARQ shall not be enabled for this connection.
                                                // The MS shall not include MBS zone identifier and MBS content ID in DSA-REQ
#define AMP_WIMAX_SF_TLV_LEN_QOS_PARAMS_SET_TYPE (1)    /// 5, AMP_WIMAX_SF_QOS_PARAMS_SET_E
#define AMP_WIMAX_SF_TLV_LEN_TRAFFIC_PRIORITY (1)   // 6, 0-7
#define AMP_WIMAX_SF_TLV_LEN_MAX_SUSTAINED_TRAFFIC_RATE (4) // 7, bits per second
#define AMP_WIMAX_SF_TLV_LEN_MAX_TRAFFIC_BURST (4)  /// 8, bytes
#define AMP_WIMAX_SF_TLV_LEN_MIN_RESERVED_TRAFFIC_RATE (4)  // 9, bits per second
#define AMP_WIMAX_SF_TLV_LEN_RESERVED10 (0)     // 10
#define AMP_WIMAX_SF_TLV_LEN_SF_SCHEDULING_TYPE (1)     // 11, AMP_WIMAX_SF_SCHEDULER_TYPE_E
#define AMP_WIMAX_SF_TLV_LEN_REQ_TRANSMISSION_POLICY (1)    // 12, AMP_WIMAX_SF_REQ_TRANSMISSION_POLICY_E
#define AMP_WIMAX_SF_TLV_LEN_TOLERATED_JITTER (4)   /// 13, ms
#define AMP_WIMAX_SF_TLV_LEN_MAX_LATENCY (4)    // 14, ms
#define AMP_WIMAX_SF_TLV_LEN_SDU_IND (1)    /// 15, AMP_WIMAX_SF_SDU_LEN_IND_E
#define AMP_WIMAX_SF_TLV_LEN_SDU_SIZE (1)   /// 16, bytes, default 49
#define AMP_WIMAX_SF_TLV_LEN_TARGET_SAID (2)    // 17
#define AMP_WIMAX_SF_TLV_LEN_ARQ_ENABLE (1)     /// 18, AMP_WIMAX_SF_ARQ_E
#define AMP_WIMAX_SF_TLV_LEN_ARQ_WINDOW_SIZE (2)    /// 19, > 0 and <= (ARQ_BSN_MODULUS/2)
#define AMP_WIMAX_SF_TLV_LEN_ARQ_RETRY_TIMEOUT_TX (2)   /// 20, 0–655350 (10 ìs granularity), (Cor2 0–6553500, 100 us granularity)
#define AMP_WIMAX_SF_TLV_LEN_ARQ_RETRY_TIMEOUT_RX (2)   /// 21, 0–655350 (10 ìs granularity), (Cor2 0–6553500, 100 us granularity)
#define AMP_WIMAX_SF_TLV_LEN_ARQ_BLOCK_LIFETIME (2)     /// 22, 0 = Infinite, 1–655350 (10 ìs granularity), (Cor2 0–6553500, 100 us granularity)
#define AMP_WIMAX_SF_TLV_LEN_ARQ_SYNC_LOSS (2)  /// 23, 0 = Infinite, 1–655350 (10 ìs granularity), (Cor2 0–6553500, 100 us granularity)
#define AMP_WIMAX_SF_TLV_LEN_ARQ_DELIVER_IN_ORDER (1)   /// 24, 0 – Order of delivery is not preserved, 1 – Order of delivery is preserved
#define AMP_WIMAX_SF_TLV_LEN_ARQ_PURGE_TIMEOUT (2)  /// 25, 0 = Infinite, 1–65535 (10 ìs granularity), (Cor2 0–6553500, 100 us granularity)
#ifdef IEEE80216e_cor2
// 0 = Reserved
//1-2040 = Desired/Agreed size in bytes
//2041-65535 = Reserved
//For DSA-REQ and REG-REQ:
//Bit 0-3: encoding for proposed minimum block size (M)
//Bit 4-7: encoding for proposed maximum block size (N)
//where:
//The minimum block size is equal to 2^(M+4) and the maximum block size is equal to 2^(N+4), M<=6, N<=6, and M<=N
//For DSA-RSP and REG-RSP:
//Bit 0-3: encoding for selected block size (P)
//Bit 4-7: set to 0
//where:
//The selected block size is equal to 2^(P+4),P<=6 and M<=P<=N
#define AMP_WIMAX_SF_TLV_LEN_ARQ_BLOCK_SIZE (1)     // 26, AMP_WIMAX_SF_ARQ_BLOCK_SIZE_E
#else
#define AMP_WIMAX_SF_TLV_LEN_ARQ_BLOCK_SIZE (2)     // 26, AMP_WIMAX_SF_ARQ_BLOCK_SIZE_E
#endif
#ifdef IEEE80216e_cor2
#define AMP_WIMAX_SF_TLV_LEN_RECEIVER_ARQ_ACK_PROCESSING_TIME (1)   // 27, ???? standard not finished
#else
#define AMP_WIMAX_SF_TLV_LEN_RECEIVER_ARQ_ACK_PROCESSING_TIME (0)   // 27, 0-255
#endif
#define AMP_WIMAX_SF_TLV_LEN_CS_SPECIFICATION (1)   // 28, AMP_WIMAX_SF_CS_TYPE_E
#define AMP_WIMAX_SF_TLV_LEN_TYPE_OF_DATA_DELIVERY_SERVICES (1) // 29, AMP_WIMAX_SF_TYPE_OF_DATA_DELIVERY_SERVICES_E
#define AMP_WIMAX_SF_TLV_LEN_SDU_INTER_ARRIVAL_INTERVAL (2)     // 30, in the resolution milliseconds
#define AMP_WIMAX_SF_TLV_LEN_TIME_BASE (2)      // 31, milliseconds
#define AMP_WIMAX_SF_TLV_LEN_PAGING_PREFERENCE (1)  // 32, AMP_WIMAX_SF_PAGING_PREF_E
#ifdef IEEE80216e_cor2
#define AMP_WIMAX_SF_TLV_LEN_MBS_ZONE_IDENTIFIER_ASSIGNMENT (1) // 33
#else
#define AMP_WIMAX_SF_TLV_LEN_MBS_ZONE_IDENTIFIER_ASSIGNMENT (8) // 33
#endif
#ifdef IEEE80216e_cor2
#define AMP_WIMAX_SF_TLV_LEN_RESERVED34 (0) // 34, obsolite
#else
#define AMP_WIMAX_SF_TLV_LEN_TRAFFIC_INDICATION_PREFERENCE (1)  // 34, AMP_WIMAX_SF_TRAFFIC_IND_PREF_E
#endif
#ifdef IEEE80216e_cor2
#define AMP_WIMAX_SF_MAX_LEN_GLOBAL_SERVICE_CLASS_NAME (6)  // 35
#define AMP_WIMAX_SF_TLV_LEN_GLOBAL_SERVICE_CLASS_NAME (-6) // 35
#else
#define AMP_WIMAX_SF_TLV_LEN_GLOBAL_SERVICE_CLASS_NAME (6)  // 35
#endif
#define AMP_WIMAX_SF_TLV_LEN_RESERVED36 (0)     // 36
#define AMP_WIMAX_SF_TLV_LEN_SN_FEEDBACK_ENABLED (1)    // 37, 0 - SN feedback is disabled (default), 1 - SN feedback is enabled
#define AMP_WIMAX_SF_TLV_LEN_FSN_SIZE (1)       // 38, AMP_WIMAX_SF_FSN_SIZE_E
#define AMP_WIMAX_SF_MAX_LEN_CID_ALLOCATION_FOR_ACTIVE_BSS (4)  // 39, variable, (Num of active BS -1) * 2
#define AMP_WIMAX_SF_TLV_LEN_CID_ALLOCATION_FOR_ACTIVE_BSS (-4) // 39, variable, (Num of active BS -1) * 2
#define AMP_WIMAX_SF_TLV_LEN_UNSOLICITED_GRANT_INTERVAL (2) // 40, ms
#define AMP_WIMAX_SF_TLV_LEN_UNSOLICITED_POLLING_INTERVAL (2)   // 41, ms
#define AMP_WIMAX_SF_TLV_LEN_PDU_SN_EXT_HARQ (1)    // 42, AMP_WIMAX_SF_PDU_SN_EXT_HARQ_E
#define AMP_WIMAX_SF_MAX_LEN_MBS_CONTENTS_ID (6)    // 43, variable
#define AMP_WIMAX_SF_TLV_LEN_MBS_CONTENTS_ID (-6)   // 43, variable
#define AMP_WIMAX_SF_TLV_LEN_HARQ_SERVICE_FLOWS (1) // 44, 0 = Non HARQ (default), 1 = HARQ Connection
#define AMP_WIMAX_SF_MAX_LEN_AUTHORIZATION_TOKEN (4)    // 45, variable
#define AMP_WIMAX_SF_TLV_LEN_AUTHORIZATION_TOKEN (-4)   // 45, variable
#define AMP_WIMAX_SF_MAX_LEN_HARQ_CHANNEL_MAPPING (6)   // 46, variable
#define AMP_WIMAX_SF_TLV_LEN_HARQ_CHANNEL_MAPPING (-6)  // 46, variable
#define AMP_WIMAX_SF_MAX_LEN_ROHC_PARAM_PAYLOAD (8) // 47, variable
#define AMP_WIMAX_SF_TLV_LEN_ROHC_PARAM_PAYLOAD (-8)    // 47, variable

#define AMP_WIMAX_SF_TLV_MASK_NONE (0x0LL)
#define AMP_WIMAX_SF_TLV_MASK_ALL (0xFFFFFFFFFFFFFFFFLL)

#define AMP_WIMAX_SF_TLV_MASK_PERMITTED_BY_SS_MAC ( \
            AMP_WIMAX_SF_TLV_MASK_NONE \
            | AMP_WIMAX_SF_TLV_MASK_SERVICE_CLASS_NAME \
            | AMP_WIMAX_SF_TLV_MASK_MBS_SERVICE \
            | AMP_WIMAX_SF_TLV_MASK_QOS_PARAMS_SET_TYPE \
            | AMP_WIMAX_SF_TLV_MASK_TRAFFIC_PRIORITY \
            | AMP_WIMAX_SF_TLV_MASK_MAX_SUSTAINED_TRAFFIC_RATE \
            | AMP_WIMAX_SF_TLV_MASK_MAX_TRAFFIC_BURST \
            | AMP_WIMAX_SF_TLV_MASK_MIN_RESERVED_TRAFFIC_RATE \
            | AMP_WIMAX_SF_TLV_MASK_SF_SCHEDULING_TYPE \
            | AMP_WIMAX_SF_TLV_MASK_REQ_TRANSMISSION_POLICY \
            | AMP_WIMAX_SF_TLV_MASK_TOLERATED_JITTER \
            | AMP_WIMAX_SF_TLV_MASK_MAX_LATENCY \
            | AMP_WIMAX_SF_TLV_MASK_SDU_IND \
            | AMP_WIMAX_SF_TLV_MASK_ARQ_ENABLE \
            | AMP_WIMAX_SF_TLV_MASK_ARQ_WINDOW_SIZE \
            | AMP_WIMAX_SF_TLV_MASK_ARQ_BLOCK_LIFETIME \
            | AMP_WIMAX_SF_TLV_MASK_ARQ_DELIVER_IN_ORDER \
            | AMP_WIMAX_SF_TLV_MASK_ARQ_PURGE_TIMEOUT \
            | AMP_WIMAX_SF_TLV_MASK_ARQ_BLOCK_SIZE \
            | AMP_WIMAX_SF_TLV_MASK_RECEIVER_ARQ_ACK_PROCESSING_TIME \
            | AMP_WIMAX_SF_TLV_MASK_CS_SPECIFICATION \
            | AMP_WIMAX_SF_TLV_MASK_TYPE_OF_DATA_DELIVERY_SERVICES \
            | AMP_WIMAX_SF_TLV_MASK_SDU_INTER_ARRIVAL_INTERVAL \
            | AMP_WIMAX_SF_TLV_MASK_TIME_BASE \
            | AMP_WIMAX_SF_TLV_MASK_PAGING_PREFERENCE \
            | AMP_WIMAX_SF_TLV_MASK_GLOBAL_SERVICE_CLASS_NAME \
            | AMP_WIMAX_SF_TLV_MASK_SN_FEEDBACK_ENABLED \
            | AMP_WIMAX_SF_TLV_MASK_FSN_SIZE \
            | AMP_WIMAX_SF_TLV_MASK_UNSOLICITED_GRANT_INTERVAL \
            | AMP_WIMAX_SF_TLV_MASK_UNSOLICITED_POLLING_INTERVAL \
            | AMP_WIMAX_SF_TLV_MASK_PDU_SN_EXT_HARQ \
            | AMP_WIMAX_SF_TLV_MASK_MBS_CONTENTS_ID \
            | AMP_WIMAX_SF_TLV_MASK_HARQ_SERVICE_FLOWS \
            | AMP_WIMAX_SF_TLV_MASK_AUTHORIZATION_TOKEN \
            | AMP_WIMAX_SF_TLV_MASK_HARQ_CHANNEL_MAPPING)

/*#ifndef IEEE80216e_cor2
#define AMP_WIMAX_SF_TLV_MASK_PERMITTED_BY_SS_MAC ( \
            AMP_WIMAX_SF_TLV_MASK_PERMITTED_BY_SS_MAC \
            | AMP_WIMAX_SF_TLV_MASK_TRAFFIC_INDICATION_PREFERENCE)
#endif*/

#define AMP_WIMAX_SF_TLV_MASK_REQUIRED_QA ( \
            AMP_WIMAX_SF_TLV_MASK_NONE \
            | AMP_WIMAX_SF_TLV_MASK_QOS_PARAMS_SET_TYPE \
            | AMP_WIMAX_SF_TLV_MASK_SF_SCHEDULING_TYPE \
            | AMP_WIMAX_SF_TLV_MASK_REQ_TRANSMISSION_POLICY \
            | AMP_WIMAX_SF_TLV_MASK_ARQ_ENABLE \
            | AMP_WIMAX_SF_TLV_MASK_CS_SPECIFICATION \
            | AMP_WIMAX_SF_TLV_MASK_TYPE_OF_DATA_DELIVERY_SERVICES \
            | AMP_WIMAX_SF_TLV_MASK_HARQ_SERVICE_FLOWS)

// keep the un-permitted type here as reference
/*          | AMP_WIMAX_SF_TLV_MASK_SFID \*/
/*          | AMP_WIMAX_SF_TLV_MASK_CID \*/
/*          | AMP_WIMAX_SF_TLV_MASK_SDU_SIZE \*/
/*          | AMP_WIMAX_SF_TLV_MASK_TARGET_SAID \*/
/*          | AMP_WIMAX_SF_TLV_MASK_ARQ_RETRY_TIMEOUT_TX \*/
/*          | AMP_WIMAX_SF_TLV_MASK_ARQ_RETRY_TIMEOUT_RX \*/
/*          | AMP_WIMAX_SF_TLV_MASK_ARQ_SYNC_LOSS \*/
/*          | AMP_WIMAX_SF_TLV_MASK_MBS_ZONE_IDENTIFIER_ASSIGNMENT \*/
/*          | AMP_WIMAX_SF_TLV_MASK_CID_ALLOCATION_FOR_ACTIVE_BSS \*/

// Based WiMAX 16e D12
// 
// BE mandatory QoS service flow parameters
#define AMP_WIMAX_SF_TLV_MASK_MANDATORY_BE ( \
    AMP_WIMAX_SF_TLV_MASK_NONE \
    | AMP_WIMAX_SF_TLV_MASK_MAX_SUSTAINED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_TRAFFIC_PRIORITY \
    | AMP_WIMAX_SF_TLV_MASK_REQ_TRANSMISSION_POLICY)

// ertPS mandatory QoS service flow parameters
#define AMP_WIMAX_SF_TLV_MASK_MANDATORY_ERTPS ( \
    AMP_WIMAX_SF_TLV_MASK_NONE \
    | AMP_WIMAX_SF_TLV_MASK_MAX_LATENCY \
    | AMP_WIMAX_SF_TLV_MASK_TOLERATED_JITTER \
    | AMP_WIMAX_SF_TLV_MASK_MIN_RESERVED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_MAX_SUSTAINED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_TRAFFIC_PRIORITY \
    | AMP_WIMAX_SF_TLV_MASK_REQ_TRANSMISSION_POLICY \
    | AMP_WIMAX_SF_TLV_MASK_UNSOLICITED_GRANT_INTERVAL)

// nrtPS mandatory QoS service flow parameters
#define AMP_WIMAX_SF_TLV_MASK_MANDATORY_NRTPS ( \
    AMP_WIMAX_SF_TLV_MASK_NONE \
    | AMP_WIMAX_SF_TLV_MASK_MIN_RESERVED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_MAX_SUSTAINED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_TRAFFIC_PRIORITY \
    | AMP_WIMAX_SF_TLV_MASK_REQ_TRANSMISSION_POLICY)

// rtPS mandatory QoS service flow parameters
#define AMP_WIMAX_SF_TLV_MASK_MANDATORY_RTPS ( \
    AMP_WIMAX_SF_TLV_MASK_NONE \
    | AMP_WIMAX_SF_TLV_MASK_MAX_LATENCY \
    | AMP_WIMAX_SF_TLV_MASK_MIN_RESERVED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_MAX_SUSTAINED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_TRAFFIC_PRIORITY \
    | AMP_WIMAX_SF_TLV_MASK_REQ_TRANSMISSION_POLICY \
    | AMP_WIMAX_SF_TLV_MASK_UNSOLICITED_POLLING_INTERVAL)

// UGS mandatory QoS service flow parameters
#define AMP_WIMAX_SF_TLV_MASK_MANDATORY_UGS ( \
    AMP_WIMAX_SF_TLV_MASK_NONE \
    | AMP_WIMAX_SF_TLV_MASK_TOLERATED_JITTER \
    | AMP_WIMAX_SF_TLV_MASK_MIN_RESERVED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_MAX_LATENCY \
    | AMP_WIMAX_SF_TLV_MASK_REQ_TRANSMISSION_POLICY \
    | AMP_WIMAX_SF_TLV_MASK_UNSOLICITED_GRANT_INTERVAL) 
//  | AMP_WIMAX_SF_TLV_MASK_SDU_SIZE 


// default SME QoS service flow parameters
#define AMP_WIMAX_SF_TLV_MASK_SME_BE ( \
    AMP_WIMAX_SF_TLV_MASK_NONE \
    | AMP_WIMAX_SF_TLV_MASK_MAX_SUSTAINED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_TRAFFIC_PRIORITY \
    | AMP_WIMAX_SF_TLV_MASK_REQ_TRANSMISSION_POLICY \
    | AMP_WIMAX_SF_TLV_MASK_MAX_TRAFFIC_BURST \
    | AMP_WIMAX_SF_TLV_LEN_MIN_RESERVED_TRAFFIC_RATE \
    | AMP_WIMAX_SF_TLV_MASK_TOLERATED_JITTER \
    | AMP_WIMAX_SF_TLV_MASK_MAX_LATENCY \
    | AMP_WIMAX_SF_TLV_MASK_SF_SCHEDULING_TYPE \
    | AMP_WIMAX_SF_TLV_MASK_TYPE_OF_DATA_DELIVERY_SERVICES)

// structures for service flow parameters defined in 802.16e 2005
//#define AMP_MAX_QOS_SERVICE_CLASS_NAME_LEN (32)

#define AMP_MAX_PROFILE_NAME_LEN (32)
#define AMP_QOS_FULL_PARAMS_NON_TLV_LEN (8 + (AMP_MAX_PROFILE_NAME_LEN))
typedef struct
{
    // not TLV
    UINT32 u32Codec;        // use RTP payload type
    UINT32 u8SfInitState:8; // 8 bits, AMP_WIMAX_SF_STATE_E
    UINT32 u8Dscp:8;        // DSCP code
    UINT32 u16Pad:16;
    UINT8 szSfProfileName [AMP_MAX_PROFILE_NAME_LEN];       // optional

    // 802.16e 2005 TLV
    UINT32 u32MaxSustainedTrafficRate;      // bits per second
    UINT32 u32MaxTrafficBurst;          // bytes of burst size
    UINT32 u32MinReservedTrafficRate;       // bits per second
    UINT32 u32ToleratedJitter;          // ms
    UINT32 u32MaxLatency;               // ms
    UINT8 szServiceClassName [AMP_WIMAX_SF_MAX_LEN_SERVICE_CLASS_NAME];     // optional, // 2-128, Null-terminated string of ASCII characters. The length of the string, including null-terminator may not exceed 128 bytes
#ifdef IEEE80216e_cor2
    UINT8 u8GlobalServiceClassName[AMP_WIMAX_SF_MAX_LEN_GLOBAL_SERVICE_CLASS_NAME];         //Variable: combination of ASCII characters and hex values
#else
    UINT8 u8GlobalServiceClassName[AMP_WIMAX_SF_TLV_LEN_GLOBAL_SERVICE_CLASS_NAME];
#endif
    UINT8 u8Direction;      // 0: up link, 1: downlink  
    UINT8 u8TrafficPriority;    // 0 to 7—Higher numbers indicate higher priority, Default 0  
    UINT8 u8SfUplinkGrantSchedulingType;            // AMP_WIMAX_SF_SCHEDULER_TYPE_E
    UINT8 u8CsSpecification;            // AMP_WIMAX_SF_CS_TYPE_E
    UINT8 u8TypeOfDataDeliveryServices;     // AMP_WIMAX_SF_TYPE_OF_DATA_DELIVERY_SERVICES_E
    UINT8 u8Pad;
} AMP_WIMAX_QOS_SME_PARAMS_S;

// IEEE 802.16e 2005 Cor2/D2
typedef struct
{
    AMP_WIMAX_QOS_SME_PARAMS_S sSmeParams;
    UINT32 u32Sfid;     //1–4 294 967 295
    UINT16 u16Cid;
    UINT16 u16TargetSaid;
    UINT16 u16ArqWindowSize;        // > 0 and ¡Ü (ARQ_BSN_MODULUS/2)
    UINT16 u16ArqRetryTimeoutTx;        // TRANSMITTER_DELAY 0–655350 (100 µs granularity)
    UINT16 u16ArqRetryTimeoutRx;        // RECEIVER_DELAY 0–655350 (100 µs granularity)
    UINT16 u16ArqBlockLifetime;         // 0 = Infinite 1–655350 (100 µs granularity)
    UINT16 u16ArqSyncLoss;              // 0 = Infinite 1–655350 (100 µs granularity)
    UINT16 u16ArqPurgeTimeout;          // 0 = Infinite 1–65535 (100 µs granularity)
#ifndef IEEE80216e_cor2 
    UINT16 u16ArqBlockSize;             // 0-15: Reserved 1–204016, 32, 64, 128, 256, 512, or 1024 or 2048: Desired/Agreed size in bytes 2041-65535: Reserved
#endif
    UINT16 u16SduInterArrivalInterval;          // SDU Inter-arrival Interval in the resolution of 0.5 milliseconds
    UINT16 u16TimeBase;         // Time base in milliseconds
    UINT16 u16UnsolicitedGrantInterval;     //ms
    UINT16 u16UnsolicitedPollingInterval;   //ms
#ifndef IEEE80216e_cor2
    UINT16 u16Pad;
#endif
    UINT8 u8MbsService;         // 0: No available MBS,1: Single-BS-MBS, 2: Multi-BS-MBS
    UINT8 u8QosParamsSet;       // AMP_WIMAX_SF_TLV_VALUE_QOS_PARAMS_SET_E
    UINT8 u8Reserved10;     // was Minimum tolerable traffic rate
    UINT8 u8ReqTransmissionPolicy;  // AMP_WIMAX_SF_REQ_TRANSMISSION_POLICY_E
    UINT8 u8SduLenInd;                  // AMP_WIMAX_SF_TLV_VALUE_SDU_LEN_IND_E
    UINT8 u8SduSize;                    // number of bytes, default 49  
    UINT8 u8ArqEnable;      // AMP_WIMAX_SF_TLV_VALUE_ARQ_E 
    UINT8 u8ArqDeliverInOrder;          // AMP_WIMAX_SF_TLV_VALUE_ARQ_ORDER_E
#ifdef IEEE80216e_cor2
    UINT8 u8ArqBlockSize;               //For DSA-REQ and REG-REQ:
                                        //Bit 0-3: encoding for proposed minimum block size (M)
                                        //Bit 4-7: encoding for proposed maximum block size (N)
                                        //where:
                                        //The minimum block size is equal to
                                        //2^(M+4) and the maximum block size is
                                        //equal to 2^(N+4), M<=6, N<=6, and M<=ƒ«
                                        //For DSA-RSP and REG-RSP:
                                        //Bit 0-3: encoding for selected block size (P)
                                        //Bit 4-7: set to 0
                                        //where:
                                        //The selected block size is equal to 2^(P+4),
                                        //P<=6 and M<=P<=N
    UINT8 u8ReceiverArqAckProcessingTime;  // cor2
#else
    UINT8 u8ReceiverArqAckProcessingTime;               // 0–255
#endif
    UINT8 u8PagingPreference;       // 0: No paging generation, 1: Paging generation
#ifdef IEEE80216e_cor2
    UINT8 u8MbsZoneIdentifierAssignment;        // MBS zone identifier
    UINT8 u8Reserved34;     // cor2 obsolit
#else
    UINT8 u8TrafficIndicationPreference;    // 0: No traffic indication, 1: Traffic indication
#endif
    UINT8 u8Reserved36;
    UINT8 u8SnFeedbackEnabled;  // 0 - SN feedback is disabled (default), 1 - SN feedback is enabled    
    UINT8 u8FsnSize;            //0 = 3-bit FSN, 1 = 11-bit FSN, Default = 1
    UINT8 u8HarqServiceFlows;       // 0 = Non HARQ (default), 1 = HARQ Connection
    UINT8 u8PduSnExtHarq;           //0–No support for PDU SN in this connection (default)
                                    //1–PDU SN (short) extended SH
                                    //2–PDU SN (long) extended SH
                                    //3-256–Reserved.
#ifndef IEEE80216e_cor2
    UINT8 u8MbsZoneIdentifierAssignment[AMP_WIMAX_SF_TLV_LEN_MBS_ZONE_IDENTIFIER_ASSIGNMENT];       // MBS zone identifier
#else
    UINT8 u8Pad[2];
#endif
    UINT8 u8AuthorizationToken[AMP_WIMAX_SF_MAX_LEN_AUTHORIZATION_TOKEN];       // variable
    UINT8 u8MbsContentsId[AMP_WIMAX_SF_MAX_LEN_MBS_CONTENTS_ID];            // variable (2*n) MBS Contents ID(0), MBS Contents ID(1), ..., MBS Contents ID(n-1)   
    UINT8 u8Harq_Channel_Mapping[AMP_WIMAX_SF_MAX_LEN_HARQ_CHANNEL_MAPPING];  // variable, HARQ channel Index (1 byte each)
    UINT8 u8CidAllocationForActiveBss[AMP_WIMAX_SF_MAX_LEN_CID_ALLOCATION_FOR_ACTIVE_BSS];      // variable, (Num of active BS -1) * 2
    UINT8 u8RohcParamPayload[AMP_WIMAX_SF_MAX_LEN_ROHC_PARAM_PAYLOAD];          //Variable: 
} AMP_WIMAX_QOS_FULL_PARAMS_S;

// Message payload

// The message for both RA and QA from WiMAX Network driver



// The message for RA from WiMAX Network driver

// AMP CoCo SPFT2
// Just byte array, one info byte followed by three bytes NAPID
typedef struct
{
    UINT8 ubValid:1;    // b0 for the first byte
    UINT8 uRsrvd:7; // =0 for now
    UINT8 u8Id[3];
} AMP_WIMAX_MAC_ID_S;

// AMP CoCo SPFT2
typedef struct
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
} AMP_WIMAX_MAC_BSID_S;

// AMP CoCo SPFT2
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
    UINT8 ubHandoverAllowed:1;
    UINT8 ubVisitedNspIdIncluded:1;     // 1= visited_nsp_id included
                                        // 0= visited_nsp_id is not included
    UINT8 u8Reserved:6;
    UINT8 u8NspId[3];
} AMP_WIMAX_MSG_CONNECT_S;

// AMP CoCo SPFT2
typedef struct 
{
    UINT8 u8DistMode;
    UINT8 u8Pad[3];
} AMP_WIMAX_MSG_DISCONNECT_S;

// AMP CoCo SPFT3
#ifndef SPFT3
typedef AMP_WIMAX_MAC_BSID_S AMP_WIMAX_MSG_SCAN_S;
#else
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
    UINT8 ubNspSearchEnabled:1;     // 1=active nsp search is enabled (i.e. MS uses SBC-REQ to request SII messages)
                                    // 0= active nsp search is not allowed (but MS may still receive unsolicited SII-ADV messages)
    UINT8 ubVerboseNspRequest:1;    // 1= MS requests verbose nsp names using SBC-REQ - only valid if active_nsp_search is enabled
                                    // 0= MS does not request verbose names
    UINT8 u8Reserved:6;
    UINT8 u8Pad[3];
} AMP_WIMAX_MSG_SCAN_S;
#endif



// AMP CoCo SPFT2
// Roaming Agent reply and notif messages
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
    UINT8 u8StatusCode;
    UINT8 u8IpRefresh; //enum of type IP_REFRESH, see below
    UINT8 u8Pad[2];
} AMP_WIMAX_MSG_HANDOVER_COMPLETE_IND_S;

// AMP CoCo SPFT2
typedef struct
{
    UINT8 u8RejectCode; //using HO_REJECT_CODE enum
    UINT8 reserved[3];
} AMP_WIMAX_MSG_HANDOVER_REJECTED_S;

// AMP CoCo SPFT2
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
    UINT8 u8HoIndicator;        //  0 - MS initiated, 1 - BS, 2 - AP
    UINT8 u8Pad[3];
} AMP_WIMAX_MSG_HANDOVER_START_IND_S;

// AMP CoCo SPFT2
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
    UINT8 u8ReasonCode;  //using AMP_WIMAX_MAC_LNDW_CODE_E
    UINT8 u8MacDisabled:1; //b0
    UINT8 u8Reserved:7;
    UINT8 u8Pad[2];
} AMP_WIMAX_MSG_LINK_DOWN_S;

// AMP CoCo SPFT2
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
    UINT8 u8ReasonCode;  //using AMP_WIMAX_MAC_LNDW_CODE_E
    UINT8 u8MacDisabled:1; //b0
    UINT8 u8Reserved:7;
    UINT8 u8Pad[2];
} AMP_WIMAX_MSG_CONNECT_REPLY_S;


// AMP CoCo SPFT2
typedef AMP_WIMAX_MAC_BSID_S AMP_WIMAX_MSG_LINK_UP_S;

#ifndef SPFT3
// AMP CoCo SPFT2
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
    UINT32 u32DlFrequency; // BS DL center frequency in Hz 
    UINT32 u32UlFrequency; // BS UL frequency in Hz 
    UINT8 u8IdCell; // b0-b7
    UINT8 u8SegmentId;  // b8-b15
    UINT8 u8Rssi;       // b16-b23, Unsigned integer, in 0.25dB;0x00=-103.75, report values in the range –103.75 dBm to –40 dBm.
    INT8 n8Cinr;        // b24-b31, Signed integer, in 0.5dB
    UINT16 u16BandWidth;                        //new, using BANDWIDTH enum
    UINT16 u16FftSize;                      //new, using FFT_SIZE enum
    UINT8 u8CyclicPrefix;                   //new, using CYCLIC_PREFIX enum
    UINT8 u8FrameDurationCode;      //new, using FRAME_DURATION enum
    UINT8 u8PreambleIndex;              //new, valid range 0..113
    UINT8 u8Pad;                                //new, set to 0
} AMP_WIMAX_MSG_BS_ENTRY_S;
#else
// AMP CoCo SPFT3
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
    UINT32 u32DlFrequency; // BS DL center frequency in Hz 
    UINT32 u32UlFrequency; // BS UL frequency in Hz 
    UINT8 u8IdCell; // b0-b7
    UINT8 u8SegmentId;  // b8-b15
    UINT8 u8Rssi;       // b16-b23, Unsigned integer, in 0.25dB;0x00=-103.75, report values in the range –103.75 dBm to –40 dBm.
    INT8 n8Cinr;        // b24-b31, Signed integer, in 0.5dB
    UINT16 u16BandWidth;                        //new, using BANDWIDTH enum
    UINT16 u16FftSize;                      //new, using FFT_SIZE enum
    UINT8 u8CyclicPrefix;                   //new, using CYCLIC_PREFIX enum
    UINT8 u8FrameDurationCode;      //new, using FRAME_DURATION enum
    UINT8 u8PreambleIndex;              //new, valid range 0..113
    UINT8 u8Pad;                                //new, set to 0
    UINT8 u8NspInfoStatus;  //using AMP_WIMAX_LINK_NSP_INFO_STATUS_E enum
    UINT8 u8NspChangeCount; //NSP change count from DCD
    UINT16 u16Reserved;
} AMP_WIMAX_MSG_BS_ENTRY_S;
#endif

// AMP CoCo SPFT2
typedef struct 
{
    AMP_WIMAX_MSG_BS_ENTRY_S sBsEntry[AMP_WIMAX_MAX_BSID_TABLE];
} AMP_WIMAX_MSG_SCAN_RESULT_S; 

// AMP CoCo SPFT3
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;
    AMP_WIMAX_MAC_ID_S sBaseStationId;
    UINT8 u8NspChangeCount;     // Contents of NSP Change Count TLV as received in SBC-RSP or SII-ADV message
    UINT8 u8Reserved[3];
    UINT8 u8HifTlv[1];          // NSP List TLV, T=140
                                // List of 3-byte NSPIDs. Length is multiple of 3.
                                // Verbose NSP Name List TLV, T=139
                                // List of verbose names of the Network Service Provider(s). 
                                // The value of Verbose NSP Name List is a compound list of verbose NSP name 
                                // lengths and verbose NSP names. The order of the Verbose NSP Name Lengths 
                                // and Verbose NSP Names presented in the Verbose NSP Name List TLV shall be 
                                // in the same order as the NSP IDs presented in the NSP List TLV.

} AMP_WIMAX_MSG_NEW_NSP_INFO_S;

// AMP CoCo SPFT3
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;      // NAPID of Preferred BS
    AMP_WIMAX_MAC_ID_S sBaseStationId;  // BSID of Preferred BS
    UINT16 u16PagingGroupId;
    UINT16 u16PagingOffset;
    UINT16 u16PagingCycle;
    UINT16 u16Reserved;                 // set to zero
    UINT8 u8PagingController[6];        // MAC address of Paging Controller
    UINT16 u16Pad;                      // set to zero
} AMP_WIMAX_MSG_PAGING_GROUP_INFO_S;

// AMP CoCo SPFT3
typedef AMP_WIMAX_MSG_PAGING_GROUP_INFO_S AMP_WIMAX_MSG_IDLE_MODE_ENTERED_NTF_S;

// AMP CoCo SPFT3
typedef AMP_WIMAX_MSG_PAGING_GROUP_INFO_S AMP_WIMAX_MSG_IDLE_MODE_UPDATE_NTF_S;

// AMP CoCo SPFT3
typedef struct 
{
    AMP_WIMAX_MAC_ID_S sNetworkId;      // NAPID of Preferred BS
    AMP_WIMAX_MAC_ID_S sBaseStationId;  // BSID of Preferred BS
    UINT8 u8IpRefresh;                  // AMP_WIMAX_LINK_HO_IP_REFRESH_CODE_E
                                        // 0 = REFRESH_NOT_REQUIRED
                                        // 1 = REFRESH_REQUIRED
    UINT8 u8Pad[3];
} AMP_WIMAX_MSG_IDLE_MODE_EXIT_NTF_S;

typedef AMP_WIMAX_MSG_PAGING_GROUP_INFO_S AMP_WIMAX_MSG_IDLE_MODE_INFO_RSP_S;

// QoS agent command message

// AMP BWM messages 
//

// BWM SPFT2
typedef enum
{
    eAMP_IPC_WIMAX_MAC_MSG_BWM_STATUS_CODE_INVALID = -1,
    eAMP_IPC_WIMAX_MAC_MSG_BWM_STATUS_CODE_OK = 0,
    eAMP_IPC_WIMAX_MAC_MSG_BWM_STATUS_CODE_REJ_SFID = 6,
    eAMP_IPC_WIMAX_MAC_MSG_BWM_STATUS_CODE_REJ_PARAMS_MISSING = 8,
    eAMP_IPC_WIMAX_MAC_MSG_BWM_STATUS_CODE_REJ_PARAMS_NO_SUPPORT = 17,
    eAMP_IPC_WIMAX_MAC_MSG_BWM_STATUS_CODE_MAX_VAL
} AMP_WIMAX_BWM_STATUS_CODE_E;

// BWM SPFT2
typedef enum
{
    eAMP_IPC_WIMAX_MAC_MSG_BWM_SILENCE_PERIOD_IND_INVALID = -1,
    eAMP_IPC_WIMAX_MAC_MSG_BWM_SILENCE_PERIOD_IND_STOP = 0,
    eAMP_IPC_WIMAX_MAC_MSG_BWM_SILENCE_PERIOD_IND_START = 1,
    eAMP_IPC_WIMAX_MAC_MSG_BWM_SILENCE_PERIOD_IND_MAX_VAL
} AMP_WIMAX_BWM_SILENCE_PERIOD_IND_E;

// BWM SPFT2
typedef struct
{
    UINT32 u32QosId;
    UINT32 u32SilenceThreshold;         // in number of milliseconds. 0-9999
    UINT32 u32AggrInterval;         // milliseconds that an incremental bandwidth request is advertised before an 
                                        // aggregate request must be sent. 0 - 9999
    UINT32 u32CdmaTimeout;              // milliseconds that a (BE) connection didnot get a grant (while the data is 
                                        // in the queue) or could not transmit a bandwidth request before a CDMA code
                                        // transmitted. 50 - 60000, default is 150ms
} AMP_WIMAX_MSG_BWM_SF_INFO_REQ_S;

// BWM SPFT2
typedef struct
{
    UINT32 u32QosId;
    UINT8 u8Pad[3];
    UINT8 u8SilencePeriodInd;
    UINT32 u32ComfortNoiseFrameSize;    // Ignored by MAC now
    UINT32 u32ComfortNoiseFrameInteval; // Ignored by MAC now
} AMP_WIMAX_MSG_BWM_SF_SILENCE_PERIOD_REQ_S;

// BWM SPFT2
typedef struct
{
    UINT32 u32QosId;
    UINT8 u8Status;
    UINT8 u8Pad[3];
} AMP_WIMAX_MSG_BWM_SF_INFO_RSP_S;

// BWM SPFT2
typedef struct
{
    UINT32 u32QosId;
    UINT8 u8Status;
    UINT8 u8Pad[2];
    UINT8 u8SilencePeriodInd;
} AMP_WIMAX_MSG_BWM_SF_SILENCE_PERIOD_RSP_S;

// PSC SPFT3
typedef struct 
{
    UINT8   u8PscId;    /* Valid range 0..63, rest reserved */
    UINT8   u8Definition:1;
    UINT8   u8Enable:1;
    UINT8   u8TrafficIndRequired:1;
    UINT8   u8TrafficTriggeredWakening:1;
    UINT8   u8Reserved:4;
    UINT8   u8ListeningWindow;
    UINT8   u8InitialSleepWindow;
    UINT32  u32FinalSleepWindow;    /* Valid range is dependent on frame */
    UINT16  u16ReactTime;
    UINT16  u16AllTransportConnections:1;
    UINT16  u16ManagementConnections:1;
    UINT16  u16UlOnly:1;
    UINT16  u16DlOnly:1;
    UINT16  u16CqiFeedbackAligned:1;
    UINT16  u16Reserved:8;
    UINT16  u16NumberOfSfids:3;     /* n */
} AMP_WIMAX_MSG_PSC_SPEC_S;

/*typedef struct 
{
    UINT32 u32AllTxConnections:1;
    UINT32 u32ManagementConnections:1;
    UINT32 u32UlOnly:1;
    UINT32 u32DlOnly:1;
    UINT32 u32NumberOfSfids:28;  
} AMP_WIMAX_MSG_PSC_CONN_INFO_S;*/

typedef struct
{
    AMP_WIMAX_MSG_PSC_SPEC_S sPscSpec;
//  AMP_WIMAX_MSG_PSC_CONN_INFO_S sPscConnInfo;
    UINT32 u32Sfid[AMP_WIMAX_QA_MAX_SFID_PER_PSC_REQ];
    UINT8 u8HifTlv[1];
} AMP_WIMAX_MSG_PSC_REQ_S;

typedef struct
{
    AMP_WIMAX_PSC_STATUS_E ePscStatus;
    AMP_WIMAX_MSG_PSC_SPEC_S sPscSpec;
//  AMP_WIMAX_MSG_PSC_CONN_INFO_S sPscConnInfo;
    UINT32 u32Sfid[AMP_WIMAX_QA_MAX_SFID_PER_PSC_REQ];
    UINT8 u8HifTlv[1];
} AMP_WIMAX_MSG_PSC_RSP_S;

typedef struct
{
    AMP_WIMAX_MSG_PSC_SPEC_S sPscSpec;
//  AMP_WIMAX_MSG_PSC_CONN_INFO_S sPscConnInfo;
    UINT32 u32Sfid[AMP_WIMAX_QA_MAX_SFID_PER_PSC_REQ];
    UINT8 u8HifTlv[1];
} AMP_WIMAX_MSG_PSC_NTF_S;

typedef struct 
{
    UINT8 u8AirplaneMode;               // Airplane Mode Values
                                        // 0 = NORMAL MODE
                                        // 1 = AIRPLANE MODE
    UINT8 u8Pad[3];
} AMP_WIMAX_MSG_AIRPLANE_MODE_NTF_S;

#endif  //_MOT_AMP_WIMAX_DNM_INC_

