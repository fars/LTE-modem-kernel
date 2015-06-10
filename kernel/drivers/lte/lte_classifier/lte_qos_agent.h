/*
 * Copyright Motorola, Inc. 2005-2011,
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


#ifndef _MOT_AMP_WIMAX_QOS_AGENT_INC_
#define _MOT_AMP_WIMAX_QOS_AGENT_INC_

#include <linux/timer.h>
#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_global.h>
#include <linux/lte/AMP_general.h>
#include <linux/lte/mac_msg.h>
#include <linux/lte/AMP_ipc.h>
#include <linux/lte/AMP_WiMAX.h>
#include "lte_qos_agent_DNM.h"        // include the DNM part




#define AMP_WIMAX_MAX_QOS_ID (20)
#define AMP_WIMAX_MAX_SF (AMP_WIMAX_MAX_QOS_ID)
#define AMP_WIMAX_MAX_SF_TRANSACTION (20)
#define AMP_WIMAX_MAX_CS ((((eAMP_WIMAX_SF_CS_MAX_VAL)/4)*4) + 4)

#define AMP_WIMAX_MAX_SFM_MAC_MSG_SIZE (MAX_WIMAX_CONTROL_PACKET_SIZE - sizeof(AMP_HOST_MAC_MSG_HDR_S))



// QoS Agent private data structures

typedef enum 
{
    eAMP_SM_QA_STATE_UNKNOWN = -1,
    eAMP_SM_QA_STATE_STARTED = 0,   // must be the smallest known state
    //eAMP_SM_QA_STATE_CS_LIST,
    eAMP_SM_QA_STATE_CS_SET,    //We no longer need CS_LIST state since only one CS notif will be sent by MAC. CS_SET will reflect that CS list is provided
    eAMP_SM_QA_STATE_READY, // QA READY can be reached by CS notif followed by SF added; or just SF added, the CS information will be derived from SF
    eAMP_SM_QA_STATE_MAX_VAL
} AMP_SM_QA_STATE_E;

typedef enum 
{
    eAMP_SM_QA_EVENT_UNKNOWN = -1,
    eAMP_SM_QA_EVENT_CS_NOTIF_LIST = 0,
    eAMP_SM_QA_EVENT_CS_NOTIF,
    eAMP_SM_QA_EVENT_DEFAULT_SF,
    eAMP_SM_QA_EVENT_MAX_VAL
} AMP_SM_QA_EVENT_E;

typedef struct 
{
    INT32 n32QosId;     // equal to QoS table index
    INT8 n8SfIndex;     // index to the SF table
    INT8 n8UseCount;
    INT8 n8QosStatus;   // AMP_WIMAX_QOS_STATUS_E
    UINT8 u8Creator;
} AMP_WAQA_QOS_ID_S;

// Layer 2 ICD R01.00.05, transaction should expire after Timer T10
typedef struct 
{
    INT32 n32QosId;
    UINT32 u32Sfid;                 // only used in SF delete req and BWM message
    AMP_TimerObj u32TimeStamp;
    UINT32 u32SfContextHigh;
    UINT16 u16SfContext;
    UINT16 u16TransactionId;        // equal to transaction table index
    UINT16 u16SenderAddr;
    UINT16 u16SfmMsgType;
} AMP_WAQA_SF_TRANSACTION_S;

typedef struct 
{
    UINT32 u32Sfid;
    AMP_WIMAX_SF_CLA_DSC_ACTION_E eCurrentActionCla;
    AMP_WIMAX_SF_PHS_DSC_ACTION_E eCurrentActionPhs;
    AMP_WIMAX_MAC_SFM_MSG_E eCurrentActionSf;
    UINT64 u64SfMask;
    UINT64 u64SfMaskChanged;
    UINT32 u32ClaMaskChanged;
    UINT32 u32PhsFlag;
    AMP_WIMAX_QOS_FULL_PARAMS_S sQosParamsSet;
    UINT32 u32NumOfClaRules;
    AMP_WIMAX_MSG_QOS_CLA_DESC_S sClaRules[AMP_WIMAX_MAX_CLA_RULE_PER_SF];
    UINT32 u32PartialPhsRuleMask; // this will tell which phs rule is partial
    AMP_IPC_MSG_S * psSavedIpcMsgReq; // saved ipc request with partial phs rule
    UINT32 u32NumOfPhsRules;
    AMP_WIMAX_MSG_QOS_PHS_DESC_S sPhsRules[AMP_WIMAX_MAX_PHS_RULE_PER_SF];
} AMP_WAQA_SF_DESC_S;

typedef struct 
{
    UINT32 u32TimeoutLow;
    UINT32 u32SfTimeout;
    UINT8 u8CsProvisioned[AMP_WIMAX_MAX_CS];
    UINT8 u8pad[2];
} AMP_WAQA_CONFIG_S;

typedef struct
{
    UINT16 u16Type;
    INT16 n16Length;
    UINT8 * pu8Value;
    UINT64 u64BitSelect;
} AMP_WAQA_TLV_MAP_S;

typedef struct
{
    AMP_WIMAX_SF_CS_TYPE_E eCsType;
    UINT8 bIsShared:1;
    UINT8 bIsActive:1;
    UINT8 u8Pad:6;
    UINT8 u8Category;       //AMP_WIMAX_CS_ENCAPSULATION_E
    UINT8 u8Pad1[2];
} AMP_WAQA_CS_S;

typedef enum
{
    CS_ENCAPSULATION_GPCS       = 0x0,
    CS_ENCAPSULATION_IP         = 0x1,
    CS_ENCAPSULATION_IPV6       = 0x2,
    CS_ENCAPSULATION_802DOT3    = 0x4,
    CS_ENCAPSULATION_VLAN       = 0x8,
    CS_ENCAPSULATION_ROHC       = 0x10,
    CS_ENCAPSULATION_MAX_VAL
} AMP_WIMAX_CS_ENCAPSULATION_E;

typedef enum 
{
#ifdef CONFIG_LTE
    eAMP_WIMAX_CS_CLA_FLAG_IPV4_HEADER      = 0x40,
#else
    eAMP_WIMAX_CS_CLA_FLAG_IPV4_HEADER      = 0x0,
#endif
    eAMP_WIMAX_CS_CLA_FLAG_IPV6_HEADER      = 0x1,
    eAMP_WIMAX_CS_CLA_FLAG_PORTS            = 0x2,
    eAMP_WIMAX_CS_CLA_FLAG_MAC_ADDRESSES    = 0x4,
    eAMP_WIMAX_CS_CLA_FLAG_ETHERTYPE        = 0x8,
    eAMP_WIMAX_CS_CLA_FLAG_DSAP             = 0x10,
    eAMP_WIMAX_CS_CLA_FLAG_VLAN_TAG         = 0x20,
    eAMP_WIMAX_CS_CLA_FLAG_MAX_VAL
} AMP_WIMAX_CS_CLA_FLAG_E;

// these structures are now passed to Windows kernel driver via
// an ioctl, guarantee alignment
#pragma pack(push,4)
typedef struct
{
    AMP_WIMAX_SF_CS_TYPE_E eDefaultCsType;      // still need it?
    UINT8 u8NumSharedCs;
    UINT8 u8NumActiveCs;
    UINT8 u8Category;   
    UINT8 u8Pad;
    UINT8 u8CsShared[AMP_WIMAX_MAX_CS]; // Nogotiated with the BS
    UINT8 u8CsActive[AMP_WIMAX_MAX_CS]; // Has active service flow with the BS
    AMP_WAQA_CS_S* psCs;
} AMP_WAQA_CS_INFO_S;

typedef struct
{
    AMP_WAQA_QOS_ID_S sQosIdTable[AMP_WIMAX_MAX_QOS_ID];
    AMP_WAQA_SF_TRANSACTION_S sSfTransactionTable[AMP_WIMAX_MAX_SF_TRANSACTION];
    UINT64 u64PermittedParamsMask;
    UINT64 u64MandatorySfForSchedular[eAMP_WIMAX_SF_SCHEDULER_MAX_VAL];
    UINT32 u32PermitedClaRule[eAMP_WIMAX_SF_CS_MAX_VAL];
    UINT32 u32DefaultDlSfid;
    UINT32 u32DefaultUlSfid;
    AMP_WAQA_SF_DESC_S sSfTable[AMP_WIMAX_MAX_SF];      // must be the last item
} AMP_WAQA_SF_INFO_S;

typedef struct
{
    AMP_WIMAX_SF_CS_TYPE_E eCsType;
    UINT32 u32MagicCount;
    UINT32 u32NumOfClaRules;
#ifdef CONFIG_LTE
    UINT32  u32NumOfPfts;
    AMP_LTE_PFT_QOS_S sPfts[AMP_LTE_MAX_PFTS_PER_TFT];
#endif
    UINT32 u32ActiveRuleMask;   // each bit mark one active uplink classification rule. 32 maximum for now
    AMP_WIMAX_MSG_QOS_CLA_DESC_S sClaRules[AMP_WIMAX_CLA_RULE_PER_CS];
} AMP_WAQA_CS_CLA_RULE_S;

#define DIRECTION_TFT_UL 0x2
#define DIRECTION_TFT_DL 0x1
#define DIRECTION_TFT_BI (DIRECTION_TFT_UL | DIRECTION_TFT_DL)

typedef struct
{
	UINT8 u8EpsId;
	UINT8 u8SpareBits:2;
	UINT8 u8Direction:2;
	UINT8 u8PacketFilterIdentifier:4;
   	UINT8 u8PacketFilterPrecedence;       // 0-255
	UINT64 u32ClaMask;
		
	AMP_WIMAX_ONE_BYTE_ARRAY_S sIpProt;
	AMP_WIMAX_DOUBLE_FOUR_ONE_BYTE_ARRAY_S sIpv4DestAddrMask;
	AMP_WIMAX_DOUBLE_SIXTEEN_BYTE_ARRAY_S sIpv6DestAddrMask;
	AMP_WIMAX_DOUBLE_TWO_BYTE_ARRAY_S sIpSrcPort;
	AMP_WIMAX_DOUBLE_TWO_BYTE_ARRAY_S sIpDestPort;
	AMP_WIMAX_FOUR_BYTE_ARRAY_S sSpi;
	AMP_WIMAX_TWO_ONE_BYTE_ARRAY_S sDscp;
	UINT8 u8Ipv6FlowLabel[3];  

}tPACKET_FILTER;

#pragma pack(pop)

typedef struct 
{
    UINT8       u8IpTos;            // IP ToS/DSCP
    UINT8       u8IpProtocol;       // IPv4 Protocol or IPv6 Next Header field
    UINT8       u8IpVersion;
    UINT8       u8pad;
    UINT32      u32Ipv6FlowLabel;   // IPv6 Flow Label
    UINT8*      pu8IpSourceAddr;    // IP source address
    UINT8*      pu8IpDestAddr;      // IP destination address
    UINT16      u16SourcePort;      // source port
    UINT16      u16DestPort;        // destination port
    UINT8*      pu8DestMacAddr;     // MAC destination address
    UINT8*      pu8SourceMacAddr;   // MAC source address
    UINT16      u16Ethertype;       // Ethertype
    UINT8       u8Dsap;             // DSAP
    UINT8       u8UserPriority;     // 802.1D/1Q User Priority
    UINT16      u16VlanId;          // 802.1Q VLAN_ID
    UINT16      u16Present;         // flags to indicate presence of parameters in a packet
    int         n32IpAddressLength; // only set when IPv4 or IPv6 header found
} AMP_WAQA_CLA_PACKET_ANALYSIS_RESULTS_S;


typedef struct 
{
    UINT16 u16VersionMajor;
    UINT16 u16VersionMinar;
    AMP_SM_QA_STATE_E eCurrentState;
    AMP_WIMAX_QA_STATUS_S sQaStatus;        // bits for the status of QA, accessed through IOCTL or WiMAX SP
    UINT32 u16SeqNum:16;
    ///UINT32 u8CsSet:8;
    ///UINT32 u8DefaultSf:8;
    UINT32 bSrcIpAddrExist:8;
    UINT32 u8Pad1:8;
    AMP_ThrdId sThrPid;
    AMP_ThrdHandle sThrHandle; // thread handle is needed by windows place
    //AMP_IPC_QUEUE_HEAD_S sQueue;
#if defined(__KERNEL__) && !defined(HOST_EMULATION)
    // FIXME, currently we don't use this timer, need to check on this to see if we really need.
    struct timer_list sTimerSlow;
#endif
    AMP_WIMAX_MAC_EHDU_S sMacEhdu;
    //struct net_device * pDev;
    AMP_IPC_MSG_S * psIpcMsgCurrent;
    ///AMP_WIMAX_MSG_BS_ENTRY_S sCurrentBs;
    UINT8 u8EtherHeader[16];
    UINT8 u8SrcIpAddress[4];    
    AMP_WAQA_CONFIG_S sQaConfig;
    AMP_WAQA_CS_INFO_S sCsInfo;
    AMP_WAQA_SF_INFO_S sSfInfo;
    AMP_WAQA_CLA_PACKET_ANALYSIS_RESULTS_S sPacketResult;

#ifdef CONFIG_LTE
    UINT32  u32NumTFTs;
#endif

    // WARNING, see notes above regarding the AMP_WAQA_KERNEL_ALL_DATA_S
    AMP_WAQA_CS_CLA_RULE_S sCsClaRule[AMP_WIMAX_MAX_CS_CLA_RULE];   // for QoS Agent access
    AMP_WAQA_CS_CLA_RULE_S sCsClaRuleCache[AMP_WIMAX_MAX_CS_CLA_RULE]; // for call back access
    
} AMP_WAQA_S;



#endif //_MOT_AMP_WIMAX_QOS_AGENT_INC_
