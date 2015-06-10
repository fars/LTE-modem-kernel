/*
 * Copyright (C) 2009-2011 Motorola, Inc.
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
 *
 */


 /************************************************************
  * @file nas_host_intfc.h
  * 
  * @brief Header file for Host NAS messages
  *
  ************************************************************/


#ifndef NAS_HOST_INTFC_H
#define NAS_HOST_INTFC_H

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_APN_NAME_LENGTH 255
#ifdef LTE92

typedef enum 
{
	SMR_PDN_CONN_REQ_ID	      = 0x01,
	SMR_PDN_REL_REQ_ID	      = 0x02,
	SMR_BEARER_MOD_REQ_ID	  = 0x03,
	SMR_BEARER_MOD_REJ_ID	  = 0x04,

	SMR_BP_AP_BASE            = 0x40,
	SMR_PDN_CONN_CNF_ID	      = 0x00 + SMR_BP_AP_BASE,	    // Response to SMR_PDN_CONN_REQ_ID
	SMR_PDN_CONN_REJ_ID	      = 0x01 + SMR_BP_AP_BASE,      // Response to SMR_PDN_CONN_REQ_ID
	SMR_BEARER_MOD_REJ_IND	  = 0x02 + SMR_BP_AP_BASE,
	SMR_BEARER_REL_IND_ID	  = 0x03 + SMR_BP_AP_BASE,
	SMR_BEARER_MOD_IND_ID	  = 0x04 + SMR_BP_AP_BASE,
	SMR_PDN_REL_REJ	          = 0x05 + SMR_BP_AP_BASE
} tNAS_QA_MSG_TYPE_E;


/*****************************************************/
typedef struct
{
	UINT16 u16Length;
	union
	{
		UINT8 u8Ipv4Address[4];
		UINT8 u8Ipv6Address[8];
		UINT8 u8Ipv4v6Address[12];
	} ipAddress;
} tPDN_ADDRESS_S;

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
} tQOS_S;

/*****************************************************/

typedef struct
{
	UINT16	u16Length;
	UINT8  szApnName[MAX_APN_NAME_LENGTH];
} tAPN_S;
	

typedef struct
{
	UINT8 u8PdnType; //tPDN_TYPE_E
	tAPN_S tApn;
	UINT8 u8PcoTlv[1];
} tPDN_CONN_REQ_S;

typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8LinkedBearerId;
	UINT8 u8EsmCause;
	UINT8 u8PcoTlv[1];
} tPDN_REL_REQ_S;


typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8LinkedBearerId;
	tQOS_S tQos;
	UINT8 u8TftTlv[1];
} tBEARER_MOD_REQ_S;

typedef tPDN_REL_REQ_S tBEARER_MOD_REJ_S;

// NAS--> HIF
typedef struct
{
	UINT8 u8BearerId;
	tQOS_S tQos;
	tPDN_ADDRESS_S tPdnAddress;
	tAPN_S tApn;
	UINT8 u8PcoTlv[1];
} tPDN_CONN_CNF_S;

typedef struct
{
	UINT8 u8EsmCause;
	UINT8 u8PcoTlv[1];
} tPDN_CONN_REJ_S;

typedef tPDN_REL_REQ_S tBEARER_MOD_REJ_IND_S;

typedef tPDN_REL_REQ_S tBEARER_REL_IND_S;

typedef tBEARER_MOD_REQ_S tBEARER_MOD_IND_S;

typedef tPDN_REL_REQ_S tBEARER_REL_REJ_S;

#endif

#ifdef LTE93

typedef enum 
{
	ESM_PDN_CONN_REQ		= 0x01,
	ESM_PDN_CONN_CNF		= 0x02,
	ESM_PDN_CONN_REJ		= 0x03,
	ESM_PDN_CONN_REL_REQ	= 0x04, 
	ESM_PDN_CONN_REL_CNF	= 0x05,
	ESM_PDN_CONN_REL_REJ	= 0x06,
	ESM_BEARER_ALLOC_REQ	= 0x07,
	ESM_BEARER_ALLOC_CNF	= 0x08,
	ESM_BEARER_ALLOC_REJ	= 0x09,
	ESM_BEARER_REL_REQ		= 0x0a,
	ESM_BEARER_REL_CNF		= 0x0b,
	ESM_BEARER_REL_REJ		= 0x0c,
	ESM_BEARER_MOD_REQ		= 0x0d,
	ESM_BEARER_MOD_CNF		= 0x0e,
	ESM_BEARER_MOD_REJ		= 0x0f,
	ESM_BEARER_ALLOC_IND	= 0x10,
	ESM_BEARER_ALLOC_RSP	= 0x11,
	ESM_BEARER_REL_IND		= 0x12,
	ESM_BEARER_REL_RSP		= 0x13,
	ESM_BEARER_MOD_IND		= 0x14,
	ESM_BEARER_MOD_RSP		= 0x15,
/*
	SMR_PDN_REL_REQ_ID	      = 0x02,
	SMR_BEARER_MOD_REQ_ID	  = 0x03,
	SMR_BEARER_MOD_REJ_ID	  = 0x04,

	SMR_BP_AP_BASE            = 0x40,
	SMR_PDN_CONN_CNF_ID	      = 0x00 + SMR_BP_AP_BASE,	    // Response to SMR_PDN_CONN_REQ_ID
	SMR_PDN_CONN_REJ_ID	      = 0x01 + SMR_BP_AP_BASE,      // Response to SMR_PDN_CONN_REQ_ID
	SMR_BEARER_MOD_REJ_IND	  = 0x02 + SMR_BP_AP_BASE,
	SMR_BEARER_REL_IND_ID	  = 0x03 + SMR_BP_AP_BASE,
	SMR_BEARER_MOD_IND_ID	  = 0x04 + SMR_BP_AP_BASE,
	SMR_PDN_REL_REJ	          = 0x05 + SMR_BP_AP_BASE
*/
} tNAS_QA_MSG_TYPE_E;


/*****************************************************/
typedef struct
{
	UINT16 u16Length;
	union
	{
		UINT8 u8Ipv4Address[4];
		UINT8 u8Ipv6Address[8];
		UINT8 u8Ipv4v6Address[12];
	} ipAddress;
} tPDN_ADDRESS_S;

typedef struct
{
	UINT8      u8Len;           /* Length of SDF QoS */
	UINT8	   u8QoS[1]; 	
} tQOS_S;


typedef struct
{
	UINT16	u16Length;
	UINT8  szApnName[MAX_APN_NAME_LENGTH];
} tAPN_S;
	

typedef enum 
{
TAG_BOOL_FLAG		=	0x01,
TAG_UINT8			=	0x02,
TAG_UINT16			=	0x03,
TAG_UINT32			=	0x04,
TAG_QOS				=	0x05,
TAG_TFA				=	0x06,
TAG_BEARER_ID		=	0x07,
TAG_ESM_CAUSE		=	0x08,
TAG_PDN_TYPE		=	0x0D,
TAG_APN_TYPE		=	0x0E,
TAG_PDN_ADDRESS_TYPE=	0x0F,
TAG_PCO_TYPE		=	0x10,
TAG_APN_AMBR_TYPE	=	0x11
}tTLV_TYPE_E; 

typedef struct
{
	UINT8 u8PdnType; //tPDN_TYPE_E
	tAPN_S tApn;
	UINT8 u8PcoTlv[1];
} tPDN_CONN_REQ_S;

typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8EsmCause;
	UINT8 u8PcoTlv[1];
} tPDN_REL_REQ_S;

typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8EsmCause;//shd be 36 always 
	//tBEARER_REL_TFT tBearer_Rel; 
	UINT8 u8PcoTlv[1];
} tBEARER_REL_REQ_S;

typedef struct
{
	UINT8 tft_temp; 
	UINT8 tft_Len; 
	UINT8 no_pkt_filters:4; 
	UINT8 ebit:1; 
	UINT8 tft_OpCode:3; 
	UINT8 pfid; 
}tBEARER_REL_TFT; 

typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8EsmCause;
	UINT8 u8PcoTlv[1];
}tPDN_REL_REJ_S;

typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8LinkedBearerId;
	UINT8 u8EsmCause;
	UINT8 u8PcoTlv[1];
}tBEARER_REL_IND_S; 

typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8PcoTlv[1];
} tPDN_BEARER_REL_CNF_S;


typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8LinkedBearerId;
	UINT8 u8QosTftTlv[1];
} tBEARER_MOD_REQ_S;

typedef struct
{
	UINT8 u8LinkedBearerId;
	UINT8 u8QoSTftTlv[1];
} tBEARER_ALLOC_REQ_S;

typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8LinkedBearerId;
	UINT8 u8QoSTftTlv[1];	
} tBEARER_ALLOC_IND_S;

typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8EsmCause;
} tHOST_RSP_MSG_S;

//typedef tPDN_REL_REQ_S tBEARER_MOD_REJ_S;

typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8LinkedBearerId;
	UINT8 u8EsmCause;
	UINT8 u8PcoTlv[1];
} tBEARER_MOD_REJ_IND_S;

// NAS--> HIF
typedef struct
{
	UINT8 u8BearerId;
	UINT8 u8QosPdnLv[1];	
} tPDN_CONN_CNF_S;

typedef struct
{
	UINT8 u8EsmCause;
	UINT8 u8PcoTlv[1];
} tPDN_CONN_REJ_S;



//typedef tPDN_REL_REQ_S tBEARER_MOD_REJ_IND_S;

//typedef tPDN_REL_REQ_S tBEARER_REL_IND_S;

typedef tBEARER_ALLOC_IND_S tBEARER_MOD_IND_S;

//typedef tBEARER_MOD_IND_S tBEARER_MOD_ALLOC_S;

typedef tPDN_REL_REQ_S tBEARER_REL_REJ_S;

#endif

#ifdef __cplusplus
}
#endif

#endif
