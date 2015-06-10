/*
 * Copyright Motorola, Inc. 2009
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

#ifndef LTE_HOST_QOS_H
#define LTE_HOST_QOS_H


#define MAX_FILTERS_PER_TFT		16
#define MAX_DEDICATED_BEARER_PER_PDN 16
#define MAX_APN_NAME_LENGTH 256
#define MAX_FILTERS_PER_PDN 256


// VzW requires 4
#define MAX_PDNS	5


#define IPV4_SOURCE_ADDRESS_TYPE          0x10
#define IPV6_SOURCE_ADDRESS_TYPE          0x20
#define PROTOCOL_IDENTIFIER_TYPE          0x30
#define NEXT_HEADER_TYPE                  0x31
#define SINGLE_SOURCE_PORT_TYPE      	  0x40
#define SOURCE_PORT_RANGE_TYPE       	  0x41
#define SINGLE_DESTINATION_PORT_TYPE      0x50
#define DESTINATION_PORT_RANGE_TYPE       0x51
#define SECURITY_PARAMETER_INDEX_TYPE     0x60
#define TYPE_OF_SERVICE_TYPE              0x70
#define TRAFFIC_CLASS_TYPE                0x71
#define FLOW_LABEL_TYPE                   0x80

// Masks
// Previously defined in AMP_WiMAX_DNM.h
#define LTE_CLA_RULE_IPV4_DESTINATION_ADDRESS_TYPE     (0x0000000000800000LL) // bit 20
#define LTE_CLA_RULE_IPV6_DESTINATION_ADDRESS_TYPE     (0x0000000001000000LL)
#define LTE_CLA_RULE_PROTOCOL_IDENTIFIER_TYPE          (0x0000000002000000LL)
#define LTE_CLA_RULE_NEXT_HEADER_TYPE                  (0x0000000004000000LL)
#define LTE_CLA_RULE_SINGLE_DESTINATION_PORT_TYPE      (0x0000000008000000LL)
#define LTE_CLA_RULE_DESTINATION_PORT_RANGE_TYPE       (0x0000000010000000LL)
#define LTE_CLA_RULE_SINGLE_SOURCE_PORT_TYPE           (0x0000000020000000LL)
#define LTE_CLA_RULE_SOURCE_PORT_RANGE_TYPE            (0x0000000040000000LL)
#define LTE_CLA_RULE_SECURITY_PARAMETER_INDEX_TYPE     (0x0000000080000000LL)
#define LTE_CLA_RULE_TYPE_OF_SERVICE_TYPE              (0x0000000100000000LL)
#define LTE_CLA_RULE_TRAFFIC_CLASS_TYPE                (0x0000000200000000LL)
#define LTE_CLA_RULE_FLOW_LABEL_TYPE                   (0x0000000400000000LL) // bit 31
#define LTE_CLA_RULE_MATCH_ALL				(0xffffffffffffffffLL)



//Messages from Service Provider to QA
typedef enum
{
	//eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE = 0x80c0,
        eIPC_LTE_MSG_QOS_PDN_REQ = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0007,
	eIPC_LTE_MSG_QOS_PDN_RSP = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0008,
	eIPC_LTE_MSG_QOS_CLASSIFIER_REQ = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x0009,
	eIPC_LTE_MSG_QOS_CLASSIFIER_RSP = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x000a,
   	eIPC_LTE_MSG_QOS_BEARERS_REQ = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x000b,
	eIPC_LTE_MSG_QOS_BEARERS_RSP = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x000c,
	//eAMP_IPC_WIMAX_MSG_QOS_LOCAL_MAX = eAMP_IPC_WIMAX_MSG_QOS_LOCAL_BASE + 0x000c,
} eIPC_LTE_QOS_LOCAL_MSG;
	/*
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
*/


typedef struct 
{
    UINT8      u8Len;           /* Length of SDF QoS */
    UINT8      uQciLabel;       /* QCI label, values 1 -7 supported  */
    UINT8      uMaxBitRateUplnk;     /* Maximum Bit Rate for uplink */
    UINT8      uMaxBitRateDownlnk;   /* Maximum Bit Rate for downlink */
    UINT8      uGaurenBitRateUplnk;  /* Guaranteed Bit Rate for Uplink   */
    UINT8      uGaurenBitRateDownlnk;  /* Guaranteed Bit Rate for downlink */
    UINT8      uMaxBitRateUplnkExt;     /* Maximum Bit Rate for uplink extended */
    UINT8      uMaxBitRateDownlnkExt;   /* Maximum Bit Rate for uplink extended */
    UINT8      uGaurenBitRateUplnkExt;  /* Guaranteed Bit Rate  extended     */
    UINT8      uGaurenBitRateDownlnkExt;  /* Guaranteed Bit Rate  extended     */
} tHIF_QOS;

typedef struct tClassifier_table_entry
{
	tPACKET_FILTER tPacketFilter;
	struct tClassifier_table_entry *tNextPF;
	struct tClassifier_table_entry *tPrevPF;
} tCLASSIFIER_TABLE_ENTRY;

typedef struct
{
	UINT8 u8EpsBearerId;
	tHIF_QOS tHifQos;
	UINT8 u8NumFilters;
	tCLASSIFIER_TABLE_ENTRY *tPacketFilters[MAX_FILTERS_PER_TFT]; 
}tTFT;


#define TD_TFT_OP_CODE_CREATE_NEW_TFT                       0x01
#define TD_TFT_OP_CODE_DELETE_EXISTING_TFT                  0x02
#define TD_TFT_OP_CODE_ADD_PKT_FLTR_TO_EXISTING_TFT         0x03
#define TD_TFT_OP_CODE_REPLACE_PKT_FLTR_TO_EXISTING_TFT     0x04
#define TD_TFT_OP_CODE_DELETE_PKT_FLTR_FROM_EXISTING_TFT    0x05
#define TD_TFT_OP_CODE_NO_TFT_OPERATION                     0x06

typedef struct
{
	UINT8 u8EpsBearerId;
	UINT8 u8TFTOpCode:3;
	UINT8 u8EBit:1;
	UINT8 u8NumFilters:4;
	union
	{
		UINT8 u8PfidList[MAX_FILTERS_PER_TFT];
		tPACKET_FILTER tPacketFilters[MAX_FILTERS_PER_TFT];
	}packet_filters;
} tTFT_RECVD;


typedef struct 
{
	tTFT tDefaultBearer;
        UINT8 u8MatchAll;               /* Set to 1 if wild card filter is present for the default bearer */
        UINT8 u8SrcMacAddr[6];		/* Source MAC Address of USB interface on host for the interface */
        UINT8 u8PdnType;
        UINT8 u8PdnAddress[12];
	UINT8 u8AccessPointName[MAX_APN_NAME_LENGTH];
	UINT8 u8NumDedicatedBearers;
	tTFT tDedicatedBearer[MAX_DEDICATED_BEARER_PER_PDN]; //tTFT need not have filter tables as the filters are anyways independent of bearers; We can match the filter to bearer using bearerID   
	tCLASSIFIER_TABLE_ENTRY *tClassifierTable;
} tPDN_CLASSIFIER;

typedef struct 
{
    //INT32 n32QosId;
    //UINT32 u32Sfid;                 // only used in SF delete req and BWM message
    //AMP_TimerObj u32TimeStamp;
    UINT32 u32SfContextHigh;
    UINT16 u16SfContext;
    UINT16 u16TransactionId;        // equal to transaction table index
    UINT16 u16SenderAddr;
    UINT16 u16SfmMsgType;
} LTE_WAQA_SF_TRANSACTION_S;

typedef struct 
{
	LTE_WAQA_SF_TRANSACTION_S sSfTransactionTable[AMP_WIMAX_MAX_SF_TRANSACTION];	
} LTE_QATRAN_S; 

typedef struct
{
	UINT8 u8EpsBearerId;
	UINT8 u8AccessPointName[MAX_APN_NAME_LENGTH];
} tPDN_ID;


typedef struct 
{
	tPDN_ID pdn_id;
	UINT8 u8NumFilters;
	tPACKET_FILTER tPacketFilters[MAX_FILTERS_PER_PDN];
} tGet_Pkt_Filter_Response;

typedef struct
{
	UINT8 u8EpsBearerId;
	tHIF_QOS tHifQos;
	UINT8 u8NumFilters;
	//tPACKET_FILTER tPacketFilters[MAX_FILTERS_PER_TFT]; 
} tBEARER;

typedef struct
{
	//tPDN_ID pdn_id; 
	tBEARER tDefaultBearer;
	UINT8 u8NumDedicatedBearers;	
	tBEARER tDedicatedBearer[MAX_DEDICATED_BEARER_PER_PDN]; 
} tBearer_Info;

typedef struct 
{

    // WARNING, see notes above regarding the AMP_WAQA_KERNEL_ALL_DATA_S
    AMP_WAQA_CS_CLA_RULE_S sCsClaRule[AMP_WIMAX_MAX_CS_CLA_RULE];   // for QoS Agent access
    AMP_WAQA_CS_CLA_RULE_S sCsClaRuleCache[AMP_WIMAX_MAX_CS_CLA_RULE]; // for call back access
    
} AMP_LTEQA_S;
#endif
