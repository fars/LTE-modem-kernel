/*
 * Copyright Motorola, Inc. 2005-2011
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

#define MODVERSIONS
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <asm/uaccess.h>

#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/if_vlan.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <linux/lte/AMP_WiMAX.h>
#include <linux/lte/AMP_ipc.h>
#include <linux/lte/mac_msg.h>
#include <linux/lte/wimax_defs.h>
#include <linux/lte/lte_sp_msgs.h>
#include <linux/lte/host_nas_intfc.h>
#include "lte_qos_agent.h"
#include "lte_host_qos.h"

#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_lte_log.h>
#include <linux/lte/AMP_lte_error.h>
#include "../common/ptr_fifo.h"
#include "../common/time_calc.h"

#include "lte_classifier.h"
#include "net_driver.h"
#include "lte_data_driver.h"


// IKCHUM-2783: ignore unused variables caused by disabling AMP_LOG_Print
#pragma GCC diagnostic ignored "-Wunused-variable"


//static UINT32 u32QaInitialized = 1;

#define QOS_WORKAROUND_ENABLED 1


static AMP_RET_CODE_E QA_HandleUserMsg(AMP_HOST_MAC_MSG_S * psIpcMsg);
static AMP_RET_CODE_E QA_ProcessMacMsg(AMP_HOST_MAC_MSG_S * psIpcMsg);

static long WIMAX_QA_DataPlanTx(struct sk_buff * psSkb, UINT16 * pu16Offset, UINT8 * pEps_id);
AMP_RET_CODE_E QA_CsExam(struct sk_buff * psSkb, AMP_WIMAX_SF_CS_TYPE_E* peCsType);
AMP_RET_CODE_E QA_Cssdu(AMP_WIMAX_SF_CS_TYPE_E eCsType, UINT16 * pu16Offset);
AMP_RET_CODE_E QA_Cla(struct sk_buff * psSkb, UINT8 * pEps_id);

#ifdef LTE92
static AMP_RET_CODE_E QA_Process_Bearer_Mod(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E QA_Process_Bearer_Rel(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_Process_PDNRelRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_Process_PDNConnRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E QA_ProcessRegConf_Lte(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_Process_BearerModRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_BearerMod_Process_QOS(tTFT * pTFT1, tBEARER_MOD_IND_S *bearer_mod1);

#elif LTE93
static AMP_RET_CODE_E QA_Process_Bearer_Mod(AMP_HOST_MAC_MSG_S * psIpcMsgRsp, tNAS_QA_MSG_TYPE_E eMsgId);
static AMP_RET_CODE_E QA_Process_Bearer_Rel(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_Process_PDNRelRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_Process_PDNConnRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp, UINT16 msgid);
static AMP_RET_CODE_E QA_ProcessRegConf_Lte(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_Process_BearerModRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E QA_Process_Bearer_Rel_Cnf(AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_BearerMod_Process_QOS(tTFT * pTFT1, UINT8 *bearer_mod1);
#endif


static AMP_RET_CODE_E QA_CmpRuleResults(tPACKET_FILTER * cla_rule, AMP_WAQA_CLA_PACKET_ANALYSIS_RESULTS_S * psResults);
static AMP_RET_CODE_E QA_ProcessDetachConf_Lte (AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_Add_SortTFT(tTFT_RECVD * ptft_recvd, UINT8 bearer_index, INT8 ct_index, UINT8 u8DefaultBearerFlag);
static AMP_RET_CODE_E Qa_Replace_SortTFT(tTFT_RECVD * ptft_recvd, UINT8 bearer_index, INT8 ct_index, UINT8 u8DefaultBearerFlag);
static AMP_RET_CODE_E QA_Process_PdnConn_Cnf (AMP_HOST_MAC_MSG_S * psIpcMsgRsp);
static AMP_RET_CODE_E Qa_BearerMod_Process_Filter (UINT16 offsetlen, UINT16 u16ReadIndex, UINT8 *pu8Data, UINT8 * pnum_pkt_filter, tBEARER_MOD_IND_S * bearer_mod);
static AMP_RET_CODE_E Qa_Replace_Filters (tTFT_RECVD *ptft_recvd, UINT8 bearer_index, UINT8 num_filters, INT8 ct_index, UINT8 u8DefaultBearerFlag);
static AMP_RET_CODE_E Qa_Process_Bearer_Delete(tTFT_RECVD *ptft_recvd, UINT8 LinkedBearerId, INT8 ct_index);
static AMP_RET_CODE_E Qa_BearerMod_Process_Filter_Del ( UINT16 *poffsetlen, UINT16 *pu16ReadIndex, UINT8 **ppu8Data, UINT8 num_filters);
static AMP_RET_CODE_E Qa_Delete_Filter(tTFT_RECVD * ptft_recvd, UINT8 bearer_index, UINT8 num_filters, INT8 ct_index, UINT8 u8DefaultBearerFlag);
static AMP_RET_CODE_E Qa_Bearer_Delete ( UINT8 bearer_id, UINT8 linked_bearer_id, INT8 ct_index);
static AMP_RET_CODE_E Qa_Process_Pdn_ConnReq (AMP_HOST_MAC_MSG_S * psIpcMsgReq);
static AMP_RET_CODE_E Qa_Process_Pdn_RelReq (AMP_HOST_MAC_MSG_S * psIpcMsgReq);
static AMP_RET_CODE_E Qa_Process_Bearer_ModReq (AMP_HOST_MAC_MSG_S * psIpcMsgReq);
static void QA_PrintFilterList(INT8 ct_index);
static long free_mgmt_payload(mgmt_packet_t* pmgmt_packet);
static mgmt_packet_t* alloc_mgmt_payload(size_t payload_size);
#define printX(x) #x
// Fix for Drop8
UINT8 gEpsId=-1;

static AMP_WAQA_S gsQa = {0};
static tPDN_CLASSIFIER gsQa_lte[MAX_PDNS] =  {0};
static tTFT_RECVD tft_recvd = {0};
static LTE_QATRAN_S gsQa_Tran = {0};
static spinlock_t mr_lock = SPIN_LOCK_UNLOCKED;
static unsigned long flags;

//cedar point
#define DO_DYNAMIC_DEV
#if defined (DO_DYNAMIC_DEV)
struct class* lte_qa_class = NULL;
#endif

typedef struct
{
    unsigned long signature;                        // our modules signature
    unsigned long size;                             // this structure's size
    void* pnet;                                     // qa handle returned from the qa probe call
    PTR_FIFO_HANDLE h_tx_mgmt_fifo;                 // handle to a ptr fifo object for TX mgmt packets
    PTR_FIFO_HANDLE h_rx_mgmt_fifo;                 // handle to a ptr fifo object for RX mgmt packets
    unsigned long tx_mgmt_packet_write_cnt;         // total tx mgmt packet write count
    unsigned long tx_mgmt_packet_read_cnt;          // total tx mgmt packet read count
    unsigned long rx_mgmt_packet_write_cnt;         // total rx mgmt packet write count
    unsigned long rx_mgmt_packet_read_cnt;          // total rx mgmt packet read count
    wait_queue_head_t txq;                          // tx wait queue
    wait_queue_head_t rxq;			    // rx wait queue
    struct semaphore sem;                           // mutual exclusion semaphore
    struct cdev cdev;                               // char device structure
} lte_qa_dev_t;

static int g_dev_major = 0;
static int g_dev_minor = 0;
static lte_qa_dev_t s_g_pdev = {0};
static lte_qa_dev_t* g_pdev = &s_g_pdev;
//static struct proc_dir_entry* g_proc_entry = NULL;
#define DEVICE_NAME "lte_classifier"
#define DEVICE_NAME_DEV1 "lte_classifier"
#define LTE_QA_SIGNATURE                        0xBAADBEEF
#define LTE_QA_TX_MGMT_PACKET_FIFO_SIZE         8
#define LTE_QA_RX_MGMT_PACKET_FIFO_SIZE         8

#define LTEQA_GLOBAL_FLAG_NONE         0x00000000
#define LTEQA_GLOBAL_FLAG_BRIDGE_MODE  0x00000001

static unsigned long g_lte_qa_global_flags = LTEQA_GLOBAL_FLAG_NONE;

// data plane.
/*********************************************************************
 *
 * Description: Status WIMAX_QA_DataPlanTx(struct sk_buff * psSkb, UINT16 * pu16Offset, UINT16 * pu16Qid, UINT8 ** ppu8Ehdu, UINT16 * pu16EhduLen)
 * @brief
 *          This function is WiMAX network driver registered callback function
 *
 * Arguments (IN/OUT):
 *                          @param[in] struct sk_buff * psSkb - Socket buffer
 *                          @param[out] UINT16 * pu16Offset - the offset to shift for sk_buff, depend on CS type
 *                          @param[out] UINT16 * pu16Qid, the queue ID assigned to transmit the packet to MAC
 *                          @param[out] UINT8 ** ppu8Ehdu, the EHDU transmit together with data packet
 *                          @param[out] UINT16 * pu16EhduLen, the length of EHDU transmit together with data packet
 *
 * Revision:
 *
 ********************************************************************/

static long WIMAX_QA_DataPlanTx(struct sk_buff * psSkb, UINT16 * pu16Offset, UINT8 * pEps_id)
{
    AMP_WIMAX_SF_CS_TYPE_E eCsType = eAMP_WIMAX_SF_CS_RESERVED;

    int eRet =0;
    AMP_ASSERT(psSkb && psSkb->dev && psSkb->dev->dev_addr && pu16Offset, "WIMAX_QA_DataPlanTx: null pointer\n");
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                  "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : Entered the wimax_qa_dataplantx call back fn \n");
     //Set bSupportEhdu if network driver allows EHDU
    eRet = (int) QA_CsExam(psSkb, &eCsType);
    eCsType = eAMP_WIMAX_SF_CS_PACKET_IPV4;
    if (eRet == eAMP_RET_SUCCESS)
    {
	eRet = (int) QA_Cssdu(eCsType, pu16Offset);
	eRet = (int) QA_Cla(psSkb, pEps_id);
    }
    else
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                      "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the Qa_cssexam failed ...indicate to driver to drop the packet\n");
    }

    return eRet;
}


/*********************************************************************
 *
 * Description: Status QA_CsExam(struct sk_buff * psSkb, AMP_WIMAX_SF_CS_TYPE_E* peCsType, UINT8 * pbUseEhdu)
 * @brief
 * This function is used for Data packet examination
 *
 * Arguments (IN/OUT):
 *                              @param[in] struct sk_buff * psSkb - socket buffer
 *                              @param[out] AMP_WIMAX_SF_CS_TYPE_E* peCsType - cs type of the data packet
 *                              @param[out] INT8 * pbUseEhdu - If more than one Cs is used buseEHDU is 1 else 0
 *                              @param[out] AMP_WAQA_CLA_PACKET_ANALYSIS_RESULTS_S sPacketresult - Structure which has Packet inspection results for a data packet
 *
 *********************************************************************/

AMP_RET_CODE_E QA_CsExam(struct sk_buff * psSkb, AMP_WIMAX_SF_CS_TYPE_E* peCsType)
{

    UINT32 PacketDropFlag          = 0,j;
    AMP_WIMAX_SF_CS_TYPE_E eCsType = eAMP_WIMAX_SF_CS_RESERVED;
    struct iphdr *iph   = NULL;
    struct ipv6hdr *ipv6h   = NULL;
    unsigned char *ip_raw   = NULL;
    unsigned char *mac_raw  = NULL;
    struct udphdr *uh = NULL;

    gsQa.sPacketResult.u16Present  = 0;

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                  "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : Entered the cs_exam fn\n");

    AMP_ASSERT(psSkb && peCsType, "WIMAX_QA_DataPlanTx: QA_CsExam: null pointer\n");

    if (!pskb_may_pull(psSkb, sizeof(struct iphdr)))
    {
        return eAMP_RET_FAIL;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
    iph = (struct iphdr *)skb_network_header(psSkb);
    ip_raw = (unsigned char *)skb_network_header(psSkb);
    ipv6h = (struct ipv6hdr *)skb_network_header(psSkb);
    mac_raw = (unsigned char *)skb_mac_header(psSkb);

    /* drop the packet if the mac header isn't set.
       a socket write to lte0 will not have a eth mac
       header set in the SKB. If we don't the, it will
       crash later on in the classifier. Raw sockets using
       this net device do not have the mac header set by
       the net driver as expected by the kernel.
    */
    if (!skb_mac_header_was_set(psSkb))
    {
        return eAMP_RET_FAIL;
    }
#else
    iph = psSkb->nh.iph;
    ip_raw = psSkb->nh.raw;
    ipv6h = psSkb->nh.ipv6h;
    mac_raw = (unsigned char *)psSkb->mac.raw;
#endif

    gsQa.sPacketResult.pu8SourceMacAddr = mac_raw + ETH_ALEN;
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
                  "QA_CsExam : Source MAC address of the  packet is  mac_addr:%02x:%02x:%02x:%02x:%02x:%02x \n",
                   gsQa.sPacketResult.pu8SourceMacAddr[0],
                   gsQa.sPacketResult.pu8SourceMacAddr[1],
                   gsQa.sPacketResult.pu8SourceMacAddr[2],
                   gsQa.sPacketResult.pu8SourceMacAddr[3],
                   gsQa.sPacketResult.pu8SourceMacAddr[4],
                   gsQa.sPacketResult.pu8SourceMacAddr[5]
    );

    gsQa.sPacketResult.u8IpVersion = iph->version;   // IPv4 Protocol or IPv6 Next header field

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                  "QA_CsExam : the ipversion of the packet is %d.\n",gsQa.sPacketResult.u8IpVersion);

    if (gsQa.sPacketResult.u8IpVersion == 4)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                      "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the entered th v4 section\n");
	gsQa.sPacketResult.u8IpTos = iph->tos;
    	gsQa.sPacketResult.u8IpProtocol = iph->protocol; //TCP or UDP

	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the ipprotocol of the packet is %d.\n",gsQa.sPacketResult.u8IpProtocol);

        gsQa.sPacketResult.pu8IpSourceAddr =  ip_raw + 12;   // IP source address
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                      "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the source ip address is %d.%d.%d.%d\n",
                gsQa.sPacketResult.pu8IpSourceAddr[0], gsQa.sPacketResult.pu8IpSourceAddr[1],
                gsQa.sPacketResult.pu8IpSourceAddr[2], gsQa.sPacketResult.pu8IpSourceAddr[3]);

        //for future reference - if the src/dest IP addr is 192.168.0.1; '192' is stored in pu8IpDestAddr[0], '168' in pu8IpDestAddr[1]...so on

        gsQa.sPacketResult.pu8IpDestAddr = ip_raw + 16;

        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                      "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the dest ip address is %d.%d.%d.%d\n",
              gsQa.sPacketResult.pu8IpDestAddr[0], gsQa.sPacketResult.pu8IpDestAddr[1],
                gsQa.sPacketResult.pu8IpDestAddr[2], gsQa.sPacketResult.pu8IpDestAddr[3]);

        gsQa.sPacketResult.u16Present |=eAMP_WIMAX_CS_CLA_FLAG_IPV4_HEADER;
        gsQa.sPacketResult.n32IpAddressLength = 4;
    }
    if (gsQa.sPacketResult.u8IpVersion == 6)
    {
        //gsQa.sPacketResult.pu8IpSourceAddr = ipv6h->saddr.in6_u.u6_addr8; // IP source address
        gsQa.sPacketResult.pu8IpSourceAddr = ip_raw  + 8;
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_CsExam :  source ipv6 address is ");
        for (j=0; j<16; j++)
        {
           AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
                      " %x \n", gsQa.sPacketResult.pu8IpSourceAddr[j]);
        }
        //gsQa.sPacketResult.pu8IpDestAddr = ipv6h->daddr.in6_u.u6_addr8;   // IP destination address
        gsQa.sPacketResult.pu8IpDestAddr = ip_raw + 24;   // IP destination address
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                      "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the dest ipv6 address of packet is %d.\n", gsQa.sPacketResult.pu8IpDestAddr[0]);
        if (gsQa.sPacketResult.pu8IpSourceAddr || gsQa.sPacketResult.pu8IpDestAddr)
        {
            gsQa.sPacketResult.u16Present |= eAMP_WIMAX_CS_CLA_FLAG_IPV6_HEADER;
        }

	gsQa.sPacketResult.u8IpTos = ipv6h->priority;
    	gsQa.sPacketResult.u8IpProtocol = ipv6h->nexthdr; //TCP or UDP

	 AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                  "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the ipprotocol of the packet is %d.\n",gsQa.sPacketResult.u8IpProtocol);
        gsQa.sPacketResult.n32IpAddressLength = 16;

        gsQa.sPacketResult.u32Ipv6FlowLabel = (UINT32)simple_strtol(ipv6h->flow_lbl, NULL, 10); // ??IPv6 Flow Label
        gsQa.sPacketResult.u32Ipv6FlowLabel = ipv6h->flow_lbl[0] | ipv6h->flow_lbl[1]<<8 | ipv6h->flow_lbl[2]<<16;

        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                      "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the flow label of the packet is %d.\n", gsQa.sPacketResult.u32Ipv6FlowLabel);
    }

    if (iph->protocol == 6)
    {

        struct tcphdr *th = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
        /* th = (struct tcphdr *)skb_transport_header(psSkb); */
	if (gsQa.sPacketResult.u8IpVersion == 4)
	{
		th = (struct tcphdr *) (ip_raw + iph->ihl*4);
	}

	else /* (gsQa.sPacketResult.u8IpVersion == 6) */
	{
		th = (struct tcphdr *)(ip_raw + 40);
	}

#else
        th = psSkb->h.th;
#endif
        gsQa.sPacketResult.u16SourcePort = th->source;     // source port
        gsQa.sPacketResult.u16SourcePort = be16_to_cpu(gsQa.sPacketResult.u16SourcePort);

        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                      "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the tcp source port is %d.\n",gsQa.sPacketResult.u16SourcePort);
        //if (gsQa.sPacketResult.u16SourcePort)
        gsQa.sPacketResult.u16DestPort = th->dest; // destination port
        gsQa.sPacketResult.u16DestPort = be16_to_cpu(gsQa.sPacketResult.u16DestPort);

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                      "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the tcp source port is %d.\n",gsQa.sPacketResult.u16DestPort);
        if (gsQa.sPacketResult.u16DestPort || gsQa.sPacketResult.u16SourcePort)
        {
            gsQa.sPacketResult.u16Present |=eAMP_WIMAX_CS_CLA_FLAG_PORTS;
        }
    }
    if (gsQa.sPacketResult.u8IpProtocol == 17)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
        /* uh = (struct udphdr *)skb_transport_header(psSkb);  */

	if (gsQa.sPacketResult.u8IpVersion == 4)
	{
		uh = (struct udphdr *) (ip_raw + iph->ihl*4);
	}

	if (gsQa.sPacketResult.u8IpVersion == 6)
	{
		uh = (struct udphdr *)(ip_raw + 40);
	}
#else
        uh = psSkb->h.uh;
#endif
        if (uh)
        {
            gsQa.sPacketResult.u16SourcePort = uh->source;     // source port
            gsQa.sPacketResult.u16SourcePort = be16_to_cpu(gsQa.sPacketResult.u16SourcePort);

            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                          "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the udp source port is %d.\n",gsQa.sPacketResult.u16SourcePort);

            //if (gsQa.sPacketResult.u16SourcePort == 67)
            //{
              //  return eAMP_RET_SUCCESS;
            //}

            gsQa.sPacketResult.u16DestPort = uh->dest; // destination port
            gsQa.sPacketResult.u16DestPort = be16_to_cpu(gsQa.sPacketResult.u16DestPort);

            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the udp dest port is %d.\n",gsQa.sPacketResult.u16DestPort);

            if (gsQa.sPacketResult.u16DestPort || gsQa.sPacketResult.u16SourcePort)
            {
                gsQa.sPacketResult.u16Present |=eAMP_WIMAX_CS_CLA_FLAG_PORTS;
            }

         }


    }

    if (gsQa.sCsInfo.u8NumActiveCs == 1)
    {
        // we do not need semaphore here since the default service flow will always be pushed down from BS at the beginning
        // and will never be deleted till link down. It is safe to just get the first CS here.
        eCsType = gsQa.sCsInfo.u8CsActive[0];
        //*pbUseEhdu = FALSE;

        // with this, we do not need the IPv4 and IPv6 filter script called by WIMAX_SP anymore
        if (eCsType == eAMP_WIMAX_SF_CS_PACKET_IPV4)
        {
#ifdef QOS_WORKAROUND_ENABLED
            if (gsQa.sPacketResult.u8IpVersion == 6)
                PacketDropFlag = 1;
#endif
    }
        else
        {
            if (eCsType == eAMP_WIMAX_SF_CS_PACKET_IPV6)
    {
                if (gsQa.sPacketResult.u8IpVersion == 4)
                    PacketDropFlag = 1;
            }
        }
    }
    else
    {
        // we have multiple CS active
        //*pbUseEhdu = TRUE;
        eCsType=eAMP_WIMAX_SF_CS_MAX_VAL;

        if (gsQa.sCsInfo.u8Category & CS_ENCAPSULATION_VLAN)
        {
            // will assume VLAN CS take precedence over plan Ethernet
            if (gsQa.sPacketResult.u8IpVersion == 4)
                eCsType= eAMP_WIMAX_SF_CS_PACKET_IPV4_802_1Q_VLAN;
            else if (gsQa.sPacketResult.u8IpVersion == 6)
                eCsType= eAMP_WIMAX_SF_CS_PACKET_IPV6_802_1Q_VLAN;
            else
                eCsType= eAMP_WIMAX_SF_CS_PACKET_802_1Q_VLAN;
        }
        else if (gsQa.sCsInfo.u8Category & CS_ENCAPSULATION_802DOT3)
        {
            //Ethernet CS
            if (gsQa.sPacketResult.u8IpVersion == 4)
                eCsType= eAMP_WIMAX_SF_CS_PACKET_IPV4_802_3_ETHERNET;
            else if (gsQa.sPacketResult.u8IpVersion == 6)
                eCsType=eAMP_WIMAX_SF_CS_PACKET_IPV6_802_3_ETHERNET;
            else
                eCsType= eAMP_WIMAX_SF_CS_PACKET_802_3_ETHERNET;
        }
        else if ((gsQa.sCsInfo.u8Category & CS_ENCAPSULATION_IP) || (gsQa.sCsInfo.u8Category & CS_ENCAPSULATION_IPV6))
        {
            // The choice will be IPv4 CS or IPv6 CS
            if (gsQa.sPacketResult.u8IpVersion == 4)
                eCsType= eAMP_WIMAX_SF_CS_PACKET_IPV4;
            else if (gsQa.sPacketResult.u8IpVersion == 6)
                eCsType= eAMP_WIMAX_SF_CS_PACKET_IPV6;
        }
        else
            eCsType=eAMP_WIMAX_SF_CS_MAX_VAL;
    }

    *peCsType = eCsType;

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                  "WIMAX_QA_DataPlanTx: WIMAX_QA_CsExam : the cstype is %d.\n",*peCsType);

    if (PacketDropFlag ==0)
        return eAMP_RET_SUCCESS;
    else
        return eAMP_RET_FAIL;


}

/*********************************************************************
 *
 * Description: Status QA_Cssdu(AMP_WIMAX_SF_CS_TYPE_E eCsType,
                        UINT8 bUseEhdu,
                        UINT16 *pu16Offset,
                        UINT16 * pu16Qid,
                        UINT16 * pu16EhduLen,
                        AMP_WIMAX_MAC_EHDU_S ** ppu8Ehdu)
 *
 * Arguments (IN/OUT):
 *                              @param[in]  AMP_WIMAX_SF_CS_TYPE_E eCsType
 *                              @param[in]  UINT8 bUseEhdu
 *                              @param[out] UINT16 *pu16Offset - Offset lenngth the driver should adjust its socket buffer pointers
 *                              @param[out] UINT16 * pu16Qid - Qid that this packet shd be sent on
 *                              @param[out] *ppu8Ehdu - pointer to the EHDU structure
 *                              @param[out] UINT16 * pu16EhduLen - Length of the EHDU structure
 *
 *********************************************************************/

AMP_RET_CODE_E QA_Cssdu(AMP_WIMAX_SF_CS_TYPE_E eCsType, UINT16 *pu16Offset)
{


    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
   AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                  "WIMAX_QA_DataPlanTx: QA_Cssdu entered this fn \n");

    AMP_ASSERT(pu16Offset, "WIMAX_QA_DataPlanTx: QA_Cssdu: null pointer\n");

    switch (eCsType)
    {
    case eAMP_WIMAX_SF_CS_RESERVED:
    case eAMP_WIMAX_SF_CS_PACKET_802_3_ETHERNET:
    case eAMP_WIMAX_SF_CS_PACKET_IPV4_802_3_ETHERNET:
    case eAMP_WIMAX_SF_CS_PACKET_IPV6_802_3_ETHERNET:
        *pu16Offset = 0;
        break;

    case eAMP_WIMAX_SF_CS_PACKET_IPV4:
    case eAMP_WIMAX_SF_CS_PACKET_IPV6:
            *pu16Offset = ETH_HLEN;       // a magic number for now, fix later
        break;
    default:
        // not supported yet
        *pu16Offset = 0;
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR, "WIMAX_QA_DataPlanTx: unsupported CS %d.\n", eCsType);
            eRet =  eAMP_RET_FAIL;
    }
    return eRet;

}

static unsigned char badmac[ETH_ALEN] = {0, 0, 0, 0, 0, 0};
AMP_RET_CODE_E QA_Cla(struct sk_buff * psSkb, UINT8 * pEps_id)
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    tCLASSIFIER_TABLE_ENTRY *p;
    INT8 ct_index = MAX_PDNS;

    AMP_ASSERT(psSkb, " QA_Cla: null pointer\n");
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                  "QA_Cla: entered this fn\n");

    eRet =  eAMP_RET_WIMAX_QOS_PKT_DROP;


    if (g_lte_qa_global_flags & LTEQA_GLOBAL_FLAG_BRIDGE_MODE)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
          "Using MAC address lookup for PDN\n");
         // Use MAC address to identify PDN
        for (ct_index = 0; ct_index < MAX_PDNS; ct_index++)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
                "[ EPS id %d]: mac_addr_lookup:%02x:%02x:%02x:%02x:%02x:%02x \n",
                gsQa_lte[ct_index].tDefaultBearer.u8EpsBearerId,
                (int) gsQa_lte[ct_index].u8SrcMacAddr[0],
                (int) gsQa_lte[ct_index].u8SrcMacAddr[1],
                (int) gsQa_lte[ct_index].u8SrcMacAddr[2],
                (int) gsQa_lte[ct_index].u8SrcMacAddr[3],
                (int) gsQa_lte[ct_index].u8SrcMacAddr[4],
                (int) gsQa_lte[ct_index].u8SrcMacAddr[5]);
            if ( !memcmp(gsQa_lte[ct_index].u8SrcMacAddr, badmac, ETH_ALEN) )
            {
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                 " Bad Mac for PDN %d , default bearer %d\n", ct_index,
                 gsQa_lte[ct_index].tDefaultBearer.u8EpsBearerId );
            }
            else if (!memcmp(gsQa_lte[ct_index].u8SrcMacAddr, gsQa.sPacketResult.pu8SourceMacAddr, ETH_ALEN))
            {
                break;
            }
        }
    }
    if (ct_index == MAX_PDNS)
    {
        for (ct_index = 0; ct_index < MAX_PDNS; ct_index++)
        {
            if ( gsQa.sPacketResult.u8IpVersion == 4)
            {
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
                 "IPv4 Src Addr %x %x %x %x\n",
                 gsQa_lte[ct_index].u8PdnAddress[0],
                 gsQa_lte[ct_index].u8PdnAddress[1],
                 gsQa_lte[ct_index].u8PdnAddress[2],
                 gsQa_lte[ct_index].u8PdnAddress[3] );
                if ( (gsQa_lte[ct_index].u8PdnType == 1)  &&
                  !memcmp(gsQa_lte[ct_index].u8PdnAddress,
                        gsQa.sPacketResult.pu8IpSourceAddr, 4))
	            {
                     break;
                }
                else if ( (gsQa_lte[ct_index].u8PdnType == 3) &&
                         !memcmp((UINT8 *)(gsQa_lte[ct_index].u8PdnAddress + 8),
                          gsQa.sPacketResult.pu8IpSourceAddr, 4))
                {
                    break;
                }
            }
            else // if( gsQa.sPacketResult.u8IpVersion == 6)
            {
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
                  "IPv6 Src Addr %x %x %x %x %x %x %x %x\n",
                  gsQa_lte[ct_index].u8PdnAddress[0],
                  gsQa_lte[ct_index].u8PdnAddress[1],
                  gsQa_lte[ct_index].u8PdnAddress[2],
                  gsQa_lte[ct_index].u8PdnAddress[3],
                  gsQa_lte[ct_index].u8PdnAddress[4],
                  gsQa_lte[ct_index].u8PdnAddress[5],
                  gsQa_lte[ct_index].u8PdnAddress[6],
                  gsQa_lte[ct_index].u8PdnAddress[7] );
                // Compare only the interface Id as the prefix may be different
                // depending on link local / global
                if ( ((gsQa_lte[ct_index].u8PdnType == 2) ||
                     (gsQa_lte[ct_index].u8PdnType == 3)) &&
                     !memcmp(gsQa_lte[ct_index].u8PdnAddress,
                        gsQa.sPacketResult.pu8IpSourceAddr+8, 8) )
                {
                    break;
                }
            }
        }
    }

    if (ct_index == MAX_PDNS)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                "QA_Cla: classifier table not found \n");
        ct_index = 0;
    }
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                "QA_Cla: Using classifier table %d\n", ct_index);

    spin_lock_irqsave(&mr_lock, flags);
    p = gsQa_lte[ct_index].tClassifierTable;
    while (p)
    {
        // Only examine filters applicable to uplink
        if ((p->tPacketFilter.u8Direction & DIRECTION_TFT_UL) ==
                                                        DIRECTION_TFT_UL)
        {
	        if (p->tPacketFilter.u32ClaMask == LTE_CLA_RULE_MATCH_ALL)
	        {
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                         "QA_Cla: LTE_CLA_RULE_MATCH_ALL\n");
                eRet = eAMP_RET_SUCCESS;
	        }
	        else
            {
		        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
                    "QA_Cla: Curerent filter : EpsID : %d ; PFID : %d ;"
                    " Precedence : %d \n",
					p->tPacketFilter.u8EpsId,
                    p->tPacketFilter.u8PacketFilterIdentifier,
                    p->tPacketFilter.u8PacketFilterPrecedence);
                eRet = QA_CmpRuleResults(&(p->tPacketFilter),
                                                    &gsQa.sPacketResult);
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                    "QA_Cla: QA_CmpRuleResults return value is %d\n", eRet);
            }

            if (eAMP_RET_SUCCESS == eRet)
            {
                *pEps_id = p->tPacketFilter.u8EpsId;
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
                    "QA_Cla:  EPS Id from classification %d \n", *pEps_id);
                break;
            }
        }
        p = p->tNextPF;
    }//end of while

    spin_unlock_irqrestore(&mr_lock, flags);

	// If no rules match packet will be dropped
    if (eRet != eAMP_RET_SUCCESS)
    {
        AMP_LOG_Info( "NO match found by classifiier\n" );
    }

    return eRet;
}


/*********************************************************************
 *
 * Description: Status QA_CmpRuleResults(AMP_WIMAX_MSG_QOS_CLA_DESC_S * cla_rule, AMP_WAQA_CLA_PACKET_ANALYSIS_RESULTS_S * psResults)
 *      This function is by-passed for WIMAX and is only called for LTE
 *
 * Arguments (IN/OUT):
 *                              IN  :   AMP_WIMAX_MSG_QOS_CLA_DESC_S * cla_rule - CLA rules negotiated between MS and BS and stored in SF tables
 *                                      AMP_WAQA_CLA_PACKET_ANALYSIS_RESULTS_S * psResults - Structure which has Packet inspection results for a data packet
 *                              OUT :
 *
 *********************************************************************/

static AMP_RET_CODE_E QA_CmpRuleResults(tPACKET_FILTER * cla_rule, AMP_WAQA_CLA_PACKET_ANALYSIS_RESULTS_S * psResults)
{
    // Local copy for faster access
    UINT32  present;
    UINT32  bitmap;
    int i;

    ASSERT (cla_rule->u32ClaMask);
    ASSERT (psResults->u16Present);

    present = psResults->u16Present;
    bitmap = cla_rule->u32ClaMask;

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                          "WIMAX_QA_DataPlanTx: QA_Cla: Cmp rules : Clarule->bitmap %x\n", bitmap);
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                          "WIMAX_QA_DataPlanTx: QA_Cla: Cmp rules : Packet results bitmap %x\n", psResults->u16Present);


    if (bitmap & LTE_CLA_RULE_IPV4_DESTINATION_ADDRESS_TYPE)
    {
        if (psResults->u16Present & eAMP_WIMAX_CS_CLA_FLAG_IPV4_HEADER)
        {
            // 11.13.19.3.4.5: An IP packet with IP destination address "ip-dst" matches this
            // parameter if dst = (ip-dst AND dmask)
	    // In Rule : 192.168.0.1 is stored as sIpv4DestAddrMask.sEntry[0].u8Entry1[0]= 192, u8Entry1[1]= 168, u8Entry1[2]= 0  u8Entry1[3]= 1
	    //  For packet: pu8IpDestAddr[0] = 192, pu8IpDestAddr[1] = 168...so on

            for (i = 3; i >= 0; i--)
            {
                if ((cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry1[i]) !=
                    (*(psResults->pu8IpDestAddr+i)
                     & (cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry2[i])))
                {
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
				"WIMAX_QA_DataPlanTx: QA_Cla: Cmp rules : IPv4 test failed rule: %d %d %d %d packet: %d %d %d %d mask: %d %d %d %d\n",
				cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry1[3], cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry1[2], cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry1[1], cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry1[0],
				 *(psResults->pu8IpDestAddr+3), *(psResults->pu8IpDestAddr+2), *(psResults->pu8IpDestAddr+1), *(psResults->pu8IpDestAddr+0), (cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry2[3]),
				 (cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry2[2]), (cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry2[1]), (cla_rule->sIpv4DestAddrMask.sEntry[0].u8Entry2[0]) );
                    return eAMP_RET_FAIL;
                }
            }
        }
        else
        {
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
				 "WIMAX_QA_DataPlanTx: QA_Cla: Cmp rules : Not eAMP_WIMAX_CS_CLA_FLAG_IPV4_HEADER\n");
            return eAMP_RET_FAIL;
        }
    }

    if (bitmap & LTE_CLA_RULE_IPV6_DESTINATION_ADDRESS_TYPE)
    {
        if (psResults->u16Present & eAMP_WIMAX_CS_CLA_FLAG_IPV6_HEADER)
        {
            // 11.13.19.3.4.5: An IP packet with IP destination address "ip-dst" matches this
            // parameter if dst = (ip-dst AND dmask)
            for (i = 15; i >= 0; i--)
            {
                if (cla_rule->sIpv6DestAddrMask.sEntry[0].u8Entry1[i] !=
                    (*(psResults->pu8IpDestAddr+i)
                     & (cla_rule->sIpv6DestAddrMask.sEntry[0].u8Entry2[i])))
                    return eAMP_RET_FAIL;
            }
        }
        else
            return eAMP_RET_FAIL;
    }

    if (bitmap & LTE_CLA_RULE_DESTINATION_PORT_RANGE_TYPE)
    {
        if (psResults->u16Present & eAMP_WIMAX_CS_CLA_FLAG_PORTS)
        {
            // 11.13.19.3.4.7: An IP packet with protocol port value "dst-port" matches this
            // parameter if dst-port is greater than or equal to dportlow (u16Entry1) and dst-port(u16Entry2) is less
            // than or equal to dporthigh.
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "WIMAX_QA_DataPlanTx: QA_Cla: Cmp rules : Destination port of pkt is %d, port range is from %d to %d \n",
	    			 psResults->u16DestPort, cla_rule->sIpDestPort.sEntry[0].u16Entry1, cla_rule->sIpDestPort.sEntry[0].u16Entry2);
	    if (psResults->u16DestPort < cla_rule->sIpDestPort.sEntry[0].u16Entry1)
                return  eAMP_RET_FAIL;
            if (psResults->u16DestPort > cla_rule->sIpDestPort.sEntry[0].u16Entry2)
                return  eAMP_RET_FAIL;
        }
        else
            return eAMP_RET_FAIL;
    }

    if (bitmap & LTE_CLA_RULE_SOURCE_PORT_RANGE_TYPE)
    {
        if (psResults->u16Present & eAMP_WIMAX_CS_CLA_FLAG_PORTS)
        {
            // 11.13.19.3.4.6: An IP packet with protocol port value "src-port" matches this
            // parameter if src-port is greater than or equal to sportlow and src-port is less
            // than or equal to sporthigh.
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "WIMAX_QA_DataPlanTx: QA_Cla: Cmp rules : Source port of pkt is %d, port range is from %d to %d \n",
	    				psResults->u16SourcePort, cla_rule->sIpSrcPort.sEntry[0].u16Entry1, cla_rule->sIpSrcPort.sEntry[0].u16Entry2);
            if (psResults->u16SourcePort < cla_rule->sIpSrcPort.sEntry[0].u16Entry1)
                return  eAMP_RET_FAIL;
            if (psResults->u16SourcePort > cla_rule->sIpSrcPort.sEntry[0].u16Entry2)
                return  eAMP_RET_FAIL;
        }
        else
            return eAMP_RET_FAIL;
    }

    if (bitmap & LTE_CLA_RULE_SINGLE_SOURCE_PORT_TYPE)
    {
        if (psResults->u16Present & eAMP_WIMAX_CS_CLA_FLAG_PORTS)
        {
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "WIMAX_QA_DataPlanTx: QA_Cla: Cmp rules : Source port of pkt is %d, port in the rules is is %d \n",
	    				psResults->u16SourcePort, cla_rule->sIpSrcPort.sEntry[0].u16Entry1);
            if (psResults->u16SourcePort != cla_rule->sIpSrcPort.sEntry[0].u16Entry1)
                return  eAMP_RET_FAIL;
	}
        else
            return eAMP_RET_FAIL;
    }

    if (bitmap & LTE_CLA_RULE_SINGLE_DESTINATION_PORT_TYPE)
    {
        if (psResults->u16Present & eAMP_WIMAX_CS_CLA_FLAG_PORTS)
        {
            if (psResults->u16DestPort != cla_rule->sIpDestPort.sEntry[0].u16Entry1)
	    {
	    	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "WIMAX_QA_DataPlanTx: QA_Cla: Cmp rules : Destination port of pkt is %d,  port in the rules is %d \n",
	    			 psResults->u16DestPort, cla_rule->sIpDestPort.sEntry[0].u16Entry1);
                return  eAMP_RET_FAIL;
	    }
	}
        else
            return eAMP_RET_FAIL;
    }

    if (bitmap & LTE_CLA_RULE_PROTOCOL_IDENTIFIER_TYPE)
    {
        if (psResults->u16Present & (eAMP_WIMAX_CS_CLA_FLAG_IPV4_HEADER | eAMP_WIMAX_CS_CLA_FLAG_IPV6_HEADER))
        {
            if (psResults->u8IpProtocol != cla_rule->sIpProt.u8Entry[0])
            {
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "WIMAX_QA_DataPlanTx: QA_Cla: Cmp rules : IpProt failed %d\n",
									psResults->u8IpProtocol);
                return  eAMP_RET_FAIL;
            }
        }
        else
            return eAMP_RET_FAIL;
    }

    if (bitmap & LTE_CLA_RULE_FLOW_LABEL_TYPE)
    {
        if (psResults->u16Present & eAMP_WIMAX_CS_CLA_FLAG_IPV6_HEADER)
        {
            UINT32 ipv6flowlabeld = cla_rule->u8Ipv6FlowLabel[0] | cla_rule->u8Ipv6FlowLabel[1] <<8 | cla_rule->u8Ipv6FlowLabel[2] <<16;
            if (psResults->u32Ipv6FlowLabel !=ipv6flowlabeld)
	    	 return eAMP_RET_FAIL;
        }
        else
            return eAMP_RET_FAIL;
    }

     if (bitmap & LTE_CLA_RULE_TYPE_OF_SERVICE_TYPE)//TOS for v4
    {
        if (psResults->u16Present & eAMP_WIMAX_CS_CLA_FLAG_IPV4_HEADER)
        {
            if ((psResults->u8IpTos & cla_rule->sDscp.u8Entry2) != cla_rule->sDscp.u8Entry1)
                return eAMP_RET_FAIL;
        }
        else
            return eAMP_RET_FAIL;
    }

    if (bitmap & LTE_CLA_RULE_TRAFFIC_CLASS_TYPE)//TOS for v6
    {
        if (psResults->u16Present & eAMP_WIMAX_CS_CLA_FLAG_IPV6_HEADER)
        {
            if ((psResults->u8IpTos & cla_rule->sDscp.u8Entry2) != cla_rule->sDscp.u8Entry1)
                return eAMP_RET_FAIL;
        }
        else
            return eAMP_RET_FAIL;
    }

   return eAMP_RET_SUCCESS;
} // cmp_rule_results

static AMP_RET_CODE_E QA_HandleUserMsg(AMP_HOST_MAC_MSG_S * psIpcMsg)
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_HandleUserMsg: Receive WiMAX message 0x%p.\n", psIpcMsg);
    AMP_ASSERT(psIpcMsg, "QA_HandleUserMsg: null pointer\n");

    switch(psIpcMsg->sHdr.u8MsgSubtype)
    {

	case eIPC_LTE_SP_QA_REGISTER_CNF:
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_HandleUserMsg: Receive LTE message LTE_SP_QA_REGISTER_CNF  psIpcMsg->sHdr.u16HostMsgType : 0x%x to  psIpcMsg->sHdr.u16HostMsgType : 0x%x.\n", psIpcMsg->sHdr.u16HostMsgType, psIpcMsg->sHdr.u8MacMsgType);
	    eRet =  QA_ProcessRegConf_Lte (psIpcMsg);
	    break;
	case eIPC_LTE_SP_QA_DETACH_CNF:
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_HandleUserMsg: Receive LTE message LTE_SP_QA_DETACH_CNF  psIpcMsg->sHdr.u16HostMsgType: 0x%x to  psIpcMsg->sHdr.u16HostMsgType 0x%x.\n", psIpcMsg->sHdr.u16HostMsgType, psIpcMsg->sHdr.u8MacMsgType);
	    eRet =  QA_ProcessDetachConf_Lte (psIpcMsg);
	    break;

#ifdef LTE92
	case SMR_PDN_CONN_REQ_ID:
#elif LTE93
	case ESM_PDN_CONN_REQ:
#endif
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_HandleUserMsg: Receive WiMAX message SMR_PDN_CONN_REQ from psIpcMsg->sHdr.u16HostMsgType: 0x%x to  psIpcMsg->sHdr.u16HostMsgType 0x%x.\n", psIpcMsg->sHdr.u16HostMsgType, psIpcMsg->sHdr.u8MacMsgType);
	    eRet = Qa_Process_Pdn_ConnReq (psIpcMsg);
	    break;

#ifdef LTE92
	case SMR_PDN_REL_REQ_ID:
#elif LTE93
	case ESM_BEARER_REL_REQ:
#endif
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_HandleUserMsg: Receive WiMAX message SMR_PDN_REL_REQ psIpcMsg->sHdr.u16HostMsgType: 0x%x to  psIpcMsg->sHdr.u16HostMsgType 0x%x.\n", psIpcMsg->sHdr.u16HostMsgType, psIpcMsg->sHdr.u8MacMsgType);
	    eRet = Qa_Process_Pdn_RelReq (psIpcMsg);
	    break;
#ifdef LTE92
	case SMR_BEARER_MOD_REQ_ID:
#elif LTE93
	 case ESM_BEARER_MOD_REQ:
#endif
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_HandleUserMsg: Receive WiMAX message ESM_BEARER_MOD_REQ psIpcMsg->sHdr.u16HostMsgType: 0x%x to  psIpcMsg->sHdr.u16HostMsgType 0x%x.\n", psIpcMsg->sHdr.u16HostMsgType, psIpcMsg->sHdr.u8MacMsgType);
	    eRet = Qa_Process_Bearer_ModReq (psIpcMsg);
	    break;
#ifdef LTE93
	case ESM_BEARER_ALLOC_REQ:
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_HandleUserMsg: Receive LTE message ESM_BEARER_ALLOC_REQ from psIpcMsg->sHdr.u16HostMsgType: 0x%x to  psIpcMsg->sHdr.u16HostMsgType 0x%x.\n", psIpcMsg->sHdr.u16HostMsgType, psIpcMsg->sHdr.u8MacMsgType);
	    eRet = Qa_Process_Bearer_ModReq (psIpcMsg);
	    break;
#endif

        default:
            break;
    };

    return eRet;
}

/*********************************************************************
 *
 * Description:                 AMP_RET_CODE_E QA_ProcessMacMsg(SAMP_IPC_MSG_S * psIpcMsg)
 *
 * Arguments (IN/OUT):
 *                              IN:  AMP_IPC_MSG_S * psIpcMsg, UINT8 * pbConnFlag, UINT32 u32BsidTableIndex
 *                              OUT: AMP_RET_CODE_E  Return code
 *
 * Revision:
 *
 *********************************************************************/

static AMP_RET_CODE_E QA_ProcessMacMsg(AMP_HOST_MAC_MSG_S * psIpcMsg)
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    AMP_HOST_MAC_MSG_S * psHm = NULL;
    tNAS_QA_MSG_TYPE_E eMacMsg;

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_ProcessMacMsg: enter IPC 0x%p.\n",
                   psIpcMsg);

    AMP_ASSERT(psIpcMsg, "QA_ProcessMacMsg: null pointer\n");

     psHm = psIpcMsg;
     eMacMsg = (tNAS_QA_MSG_TYPE_E)psHm->sHdr.u8MsgSubtype;
     AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_ProcessMacMsg: received MAC msg 0x%x.\n", eMacMsg);

    switch (eMacMsg)
    {

#ifdef CONFIG_LTE
	case ESM_PDN_CONN_CNF:
	{
	 	eRet = QA_Process_PdnConn_Cnf (psIpcMsg);
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, " RECEIVED ESM_PDN_CONN_CNF\n");
	}
	break;

	case ESM_PDN_CONN_REJ:
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "SMR_PDN_CONN_REJ_ID: PDN request has been rejected\n");
		//let the upper layers know that your PDN request has been rejected
		Qa_Process_PDNConnRej(psIpcMsg, ESM_PDN_CONN_REJ);
		break;


	}
	case ESM_PDN_CONN_REL_CNF:
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "SMR_PDN_REL_REJ_IND:SMR PDN release confirmation \n");
		//this is same as detach for a single PDN connection ; TO DO - multiple PDN
		eRet = QA_ProcessDetachConf_Lte (psIpcMsg);
		break;

	}
	case ESM_PDN_CONN_REL_REJ:
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "SMR_PDN_REL_REJ_IND:SMR PDN release request rejected\n");
		//send to IFC mgmt
		Qa_Process_PDNRelRej(psIpcMsg);
		break;
	}
	case ESM_BEARER_ALLOC_CNF://in response to indicator
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "ESM_BEARER_ALLOC_CNF is received\n");
		eRet = QA_Process_Bearer_Mod (psIpcMsg, ESM_BEARER_ALLOC_CNF);
		break;
	}
	case ESM_BEARER_ALLOC_IND:
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "ESM_BEARER_ALLOC_IND: bearer alloc ind\n");
		eRet = QA_Process_Bearer_Mod (psIpcMsg, ESM_BEARER_ALLOC_IND);
		break;
	}
	case ESM_BEARER_MOD_CNF:
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "ESM_BEARER_MOD_CNF is received\n");
		eRet = QA_Process_Bearer_Mod (psIpcMsg, ESM_BEARER_MOD_CNF);
		break;
	}
	case ESM_BEARER_MOD_IND:
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "ESM_BEARER_MOD_IND is received\n");
		eRet = QA_Process_Bearer_Mod (psIpcMsg, ESM_BEARER_MOD_IND);
		break;
	}

	case ESM_BEARER_ALLOC_REJ:
	{
		//I am using the same fn used in PDN connection request bcos we are processing both msgs in a similar fashin
		// we do not process anything in QA but let the higher layers know abt this with the right msg. ESM_BEARER_ALLOC_REJ, in this case.
		eRet =  Qa_Process_PDNConnRej(psIpcMsg, ESM_BEARER_ALLOC_REJ);
		break;
	}
	case ESM_BEARER_REL_CNF:
	{
		//release the particular dedicated bearer
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "ESM_BEARER_REL_CNF is received\n");
		eRet = QA_Process_Bearer_Rel_Cnf(psIpcMsg);
		break;
	}
	case ESM_BEARER_REL_IND:
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "ESM_BEARER_REL_IND is received\n");
		eRet = QA_Process_Bearer_Rel(psIpcMsg);
		break;
	}

	case ESM_BEARER_MOD_REJ:
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "ESM_BEARER_MOD_REJ: Bearer Mod request has been rejected\n");
		Qa_Process_BearerModRej(psIpcMsg);
		break;
	}


#endif //CONFIG_LTE
    default:
        break;
    } //switch (eMacMsg)
    return eRet;
}

static AMP_RET_CODE_E find_classifier_table(UINT8 bearer_id)
{
    UINT8 i;

    for (i=0; i<MAX_PDNS; i++)
    {
        if (gsQa_lte[i].tDefaultBearer.u8EpsBearerId == bearer_id)
	{
		return i;
        }
    }
    return eAMP_RET_FAIL;
}

static AMP_RET_CODE_E  Create_Match_All_Filter(INT8 ct_index, UINT8 bearer_id)
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    tCLASSIFIER_TABLE_ENTRY *pCft;
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Create_Matc_All_Filter is called %d %d\n", ct_index, bearer_id);

    gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0] = (tCLASSIFIER_TABLE_ENTRY *) kmalloc (sizeof(tCLASSIFIER_TABLE_ENTRY), GFP_KERNEL);
    gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8EpsId = bearer_id;
    gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8Direction = DIRECTION_TFT_BI;
    gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8PacketFilterPrecedence = 255; //will always be highest
    gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u32ClaMask = LTE_CLA_RULE_MATCH_ALL;
    gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tNextPF = NULL;
    gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPrevPF = NULL;

    pCft = gsQa_lte[ct_index].tClassifierTable;
    while (pCft!=NULL &&  pCft->tNextPF != NULL)
    {
        pCft = pCft->tNextPF;
    }
    if (pCft == NULL)
    {
        gsQa_lte[ct_index].tClassifierTable = gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0];
    }
    else
    {
        pCft->tNextPF = gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0];
        gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPrevPF = pCft;
    }

    gsQa_lte[ct_index].u8MatchAll = 1;
    return eRet;
}

#ifdef CONFIG_LTE
static AMP_RET_CODE_E QA_Process_PdnConn_Cnf (AMP_HOST_MAC_MSG_S * psIpcMsgRsp)
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    AMP_HOST_MAC_MSG_S * psHm;
    tPDN_CONN_CNF_S * pdn_cla;
    UINT8 * pu8pdn_cla;
    UINT8 u8QosLen, u8Apnsize;
    tTFT* pTFT;
    INT8 ct_index, j;
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_Process_PdnConn_Cnf is called\n");
    AMP_ASSERT(psIpcMsgRsp, "QA_Process_PdnConn_Cnf: null pointer\n");

    ct_index = find_classifier_table(0);


    if (ct_index < 0)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR, "QA_Process_PdnConn_Cnf: No More Classifier tables available - Cannot create PDN\n");
        return eAMP_RET_FAIL;
    }

    psHm = (AMP_HOST_MAC_MSG_S *) psIpcMsgRsp;

    pdn_cla = (tPDN_CONN_CNF_S *)psHm->u8Data;
    pu8pdn_cla = (UINT8 *)psHm->u8Data;

    pu8pdn_cla ++;
    u8QosLen = *pu8pdn_cla;

    gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0] = (tCLASSIFIER_TABLE_ENTRY *) kmalloc (sizeof(tCLASSIFIER_TABLE_ENTRY), GFP_KERNEL);


    spin_lock_irqsave(&mr_lock, flags);

    //gsQa_lte global variable
    gsQa_lte[ct_index].tDefaultBearer.u8EpsBearerId = pdn_cla->u8BearerId;
    memset((UINT8 *)&gsQa_lte[ct_index].tDefaultBearer.tHifQos, 0, sizeof (gsQa_lte[ct_index].tDefaultBearer.tHifQos));
    gsQa_lte[ct_index].tDefaultBearer.u8NumFilters = 1;

    pTFT = &gsQa_lte[ct_index].tDefaultBearer;

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,  "Qos Length %d u8QosLen%d\n", *pu8pdn_cla, u8QosLen);
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,  "Qos field\n");

    pu8pdn_cla = pu8pdn_cla + u8QosLen ;
    //pu8pdn_cla now points to qos field

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,  "pdn_length %d\n",  *(pu8pdn_cla+1));

   // No tag on pdn address
   gsQa_lte[ct_index].u8PdnType = *(pu8pdn_cla + 2);
   for (j=0; j< *(pu8pdn_cla+1)-1; j++)
   {
	gsQa_lte[ct_index].u8PdnAddress[j] = pu8pdn_cla[3+j];
   }

   AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Create ct_index %d PDN type %d   PDN IP address %x %x %x %x\n", ct_index,
                          gsQa_lte[ct_index].u8PdnType, gsQa_lte[ct_index].u8PdnAddress[0],
                          gsQa_lte[ct_index].u8PdnAddress[1], gsQa_lte[ct_index].u8PdnAddress[2],
                          gsQa_lte[ct_index].u8PdnAddress[3]);


  AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_Process_PdnConn_Cnf: gsQa_lte.tDefaultBearer.u8EpsBearerId %d\n",
                             gsQa_lte[ct_index].tDefaultBearer.u8EpsBearerId);
  gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8EpsId = pdn_cla->u8BearerId;
  gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8Direction = DIRECTION_TFT_BI;
  gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8PacketFilterPrecedence = 255; //will always be highest
  gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u32ClaMask = LTE_CLA_RULE_MATCH_ALL;
  gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tNextPF = NULL;
  gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0]->tPrevPF = NULL;
  gsQa_lte[ct_index].u8NumDedicatedBearers = 0;
  gsQa_lte[ct_index].u8MatchAll = 1;
  gsQa_lte[ct_index].tClassifierTable = gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0];

  spin_unlock_irqrestore(&mr_lock, flags);


  AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_Process_PdnConn_Cnf: the head of the LL :%p\n",  gsQa_lte[ct_index].tClassifierTable);
  QA_PrintFilterList(ct_index);
  return eRet;
}



#ifdef LTE93
static AMP_RET_CODE_E QA_Process_Bearer_Mod(AMP_HOST_MAC_MSG_S * psIpcMsgRsp, tNAS_QA_MSG_TYPE_E eMsgId)
{

	AMP_HOST_MAC_MSG_S * psHm;
	AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
	tBEARER_MOD_IND_S * bearer_mod;
	tTFT_RECVD *ptft_recvd;
    	UINT8 ebit, junk, no_filters, j, bearerid, u8Qoslen = 0, u8TFT_Type = 0;
	UINT8 *pu8Data, *verify, *pnum_filters_TFT, *u8Qos_Info = NULL;
	UINT8 **ppu8Data;
    	UINT8 tft_length = 0, num_filters_TFT = 0;
	UINT16 offsetlen, u16ReadIndex = 0;
	UINT16 *poffsetlen, *pu16ReadIndex;
	tTFT * pTFT = NULL;
	tHOST_RSP_MSG_S * pHostRsp;
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_Process_Bearer_Mod called \n");
	AMP_ASSERT(psIpcMsgRsp, "Qa_BearerMod: null pointer\n");

	psHm = (AMP_HOST_MAC_MSG_S *) psIpcMsgRsp;
	//bearer_mod points to the TFT (Qos and filter) data coming from BS. (all hdrs are stripped off)
    	bearer_mod = (tBEARER_MOD_IND_S *)psHm->u8Data;
    	//TFT_recvd (global struct) is populated by parsing filter tlvs; this does not have Qos information
	memset (&tft_recvd, 0, sizeof (tTFT_RECVD));
   	ptft_recvd = &tft_recvd;


	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_Process_Bearer_Mod:  bearerid is %d, LinkedBearerId is %d\n",  bearer_mod->u8BearerId, bearer_mod->u8LinkedBearerId);

	j= 20;
	verify = (UINT8 *) psHm->u8Data;
	while (j > 0)
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "QA_Process_Bearer_Mod: bit : %d ; value : %d\n",j , *verify);
		j--;
		verify ++;
	}

	bearerid = bearer_mod->u8BearerId;
	if (bearer_mod->u8BearerId == bearer_mod->u8LinkedBearerId && (eMsgId == ESM_BEARER_ALLOC_IND || eMsgId == ESM_BEARER_ALLOC_CNF))
	{
		/** Default Bearer **/
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_Process_Bearer_Mod: Creating default bearer\n");
                QA_ProcessRegConf_Lte(psIpcMsgRsp);

	}
        else
	{
		INT8 ct_index;

		// Find classifier table index
		ct_index = find_classifier_table(bearer_mod->u8LinkedBearerId);
		if (ct_index < 0)
                {
	    		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR, "QA_Process_Bearer_Mod: Classifier table not found\n");
			return eAMP_RET_FAIL;
                }
		//this is a dedicated bearer
		//u need length of the TFT to parse i.e., (payload length of the NAS-HOST msg) - (u8BearerId + u8LinkedBearerId + tQOS_S tQos)
		//offsetlen wud be the length we have to parse to populate tTFT_RECVD struct.
		#ifdef LTE93
			psHm->sHdr.u16Length = be16_to_cpu (psHm->sHdr.u16Length);
		#endif


		tft_recvd.u8EpsBearerId = bearer_mod->u8BearerId;
    	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_Process_Bearer_Mod: dedicated bearer Length of Payload is : %d, tft_recvd.u8EpsBearerId: %d\n",
				psHm->sHdr.u16Length, tft_recvd.u8EpsBearerId);

		pu8Data = &(bearer_mod->u8LinkedBearerId);
		pu8Data++;

		/*****pu8Data point to QOS if its present*****/
		if (eMsgId == ESM_BEARER_MOD_CNF || eMsgId == ESM_BEARER_MOD_IND)
		{
			//QOS is an optional field in ESM_BEARER_MOD_CNF msg, it will be in TLV format
			//Check if the type is QoS
			if (*pu8Data == 0x05)
			{
				pu8Data++; //skip the type field
				//pu8Data now points to the length of the QoS
				u8Qos_Info = pu8Data;
				u8Qoslen = *pu8Data;
			    pu8Data++; //skipping the length of QoS
				pu8Data = pu8Data + u8Qoslen;
		        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                    "QA_Process_Bearer_Mod(5): The first byte of Qos is %d",
                    *u8Qos_Info);
			}
		}
		else
		{
			u8Qos_Info = pu8Data;
			u8Qoslen = *pu8Data;
			pu8Data++; //skipping the length of QoS
			pu8Data = pu8Data + u8Qoslen;//skipping the data of QoS
		    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3,
                        "QA_Process_Bearer_Mod: The first byte of Qos is %d",
                        *u8Qos_Info);
		}

		/******TFT STARTS ************/
		if (eMsgId == ESM_BEARER_ALLOC_IND || eMsgId == ESM_BEARER_MOD_CNF || eMsgId == ESM_BEARER_MOD_IND)
		{
			u8TFT_Type = *pu8Data;
			//TFT is an optional field in bearer ALLOC_IND and MOD msg, it will be in TLV format
			pu8Data++; //skip the type field

		}
		if (eMsgId == ESM_BEARER_ALLOC_CNF)
		{
			//TFT is mandatory field in ESM_BEARER_ALLOC_IND - so it will be in LV format
		}

		if (u8TFT_Type == 0x06 || eMsgId == ESM_BEARER_ALLOC_CNF)
		{
			//there is some TFT info to process

			/********NOW THE TFT DATA PART STARTS*************/
			//pu8Data++; //skip the IEI field

			tft_length = (*pu8Data); //length of the whole TFT
			offsetlen = tft_length;
			u16ReadIndex = u16ReadIndex + 1; //u16readindex will maintain the index from length of TFT i.e., u16ReadIndex = 1;
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_Process_Bearer_Mod: length : %d\n", tft_length);


			pu8Data++;
			//pointing to TFT opcode + e-bit + no.of pkt filters
			junk = (*pu8Data);
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_Process_Bearer_Mod: TFT Opcode %d\n", junk);

			//first 3 bits is the opcode
			tft_recvd.u8TFTOpCode = junk >> 5;
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_Process_Bearer_Mod: TFT Opcode %d\n", tft_recvd.u8TFTOpCode);

			// lower 4 bits is no of filters
			no_filters = (*pu8Data)& 0x0F;
			tft_recvd.u8NumFilters = no_filters;
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_Process_Bearer_Mod: No of filters to process %d\n", tft_recvd.u8NumFilters);

			ebit = (*pu8Data)& 0x10; //access the the 5th LSB
			ebit = ebit >> 4;
			tft_recvd.u8EBit = ebit;
			u16ReadIndex++;	//u16ReadIndex = 2 at this time

            pnum_filters_TFT = &num_filters_TFT;

			switch (tft_recvd.u8TFTOpCode)
			{
			    case TD_TFT_OP_CODE_CREATE_NEW_TFT:
			    {
			    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_Process_Bearer_Mod: Creating new TFT ; TFT length to parse (offsetlen) : %d, u16ReadIndex: %d\n", tft_length, u16ReadIndex);
				//operate on the next dedicated bearer and increment the number of dedicated bearers

				spin_lock_irqsave(&mr_lock, flags);

                if (bearer_mod->u8BearerId == bearer_mod->u8LinkedBearerId)
                {
                    pTFT = &gsQa_lte[ct_index].tDefaultBearer;
				    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_Process_Bearer_Mod: Create New Filter For Default Bearer  %d \n", bearer_mod->u8BearerId);
                }
                else
                {
				    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_Process_Bearer_Mod: gsQa_lte.u8NumDedicatedBearers %d  \n", gsQa_lte[ct_index].u8NumDedicatedBearers);
				    gsQa_lte[ct_index].tDedicatedBearer[gsQa_lte[ct_index].u8NumDedicatedBearers].u8EpsBearerId = bearer_mod->u8BearerId;
				    //gsQa_lte.tDedicatedBearer[gsQa_lte.u8NumDedicatedBearers].u8NumFilters = no_filters;

			    	pTFT = &gsQa_lte[ct_index].tDedicatedBearer[gsQa_lte[ct_index].u8NumDedicatedBearers];
                }

				//u8Qos_Info points to the Length field of Qos structure (LV)
				if (u8Qoslen > 0)
                {
				    Qa_BearerMod_Process_QOS(pTFT, u8Qos_Info);
                }
                if (bearer_mod->u8BearerId != bearer_mod->u8LinkedBearerId)
                {
				    gsQa_lte[ct_index].u8NumDedicatedBearers++;
                }

				spin_unlock_irqrestore(&mr_lock, flags);


				//parse the packet filter TLVs
				pu8Data ++;
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_Process_Bearer_Mod: pu8Data b4 process _filter is %d ", (*pu8Data));
				ppu8Data = &pu8Data;

				poffsetlen = &offsetlen;
				pu16ReadIndex = &u16ReadIndex;
				//tPACKET_FILTER * ptPacket_filter = &tft_recvd.packet_filters.tPacketfilters[num_pkt_filters];

				//pu8Data points to packet filter list - points to the PFID (first byte of list)
				//u16ReadIndex is the index(number) of where the pu8Data is now pointing to starting from length of TFT IE
				//offsetlen is the length of the whole TFT
				Qa_BearerMod_Process_Filter (offsetlen, u16ReadIndex, pu8Data, pnum_filters_TFT, bearer_mod);

				//at this time we will have all the packet filter contents stored in tTFT_RECVD.tPACKET_FILTER or tft_recvd.tPacketFilters[num]
				//now have to do create a linked list in the order of precedence based on the tPACKET_FILTER tPacketFilters[X] information.
				//Only copy if the direction is bi or uplink;
				//To DO - May have to consider maintaining state for the other filters for query.
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "CREATE_NEW_TFT:num_filters_TFT %d \n", num_filters_TFT);
			    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "CREATE_NEW_TFT: Bearer index: gsQa_lte.u8NumDedicatedBearers: %d\n", gsQa_lte[ct_index].u8NumDedicatedBearers);
                if (bearer_mod->u8BearerId != bearer_mod->u8LinkedBearerId)
                {

                    Qa_Add_SortTFT(&tft_recvd, (gsQa_lte[ct_index].u8NumDedicatedBearers-1), ct_index, 0);
                }
                else
                {
                    AMP_LOG_Info("Adding TFT for default bearer on PDN %d\n", ct_index);
                    Qa_Add_SortTFT(&tft_recvd, (gsQa_lte[ct_index].u8NumDedicatedBearers-1), ct_index, 1);
                }


			}//end of case TD_TFT_OP_CODE_CREATE_NEW_TFT:
			break;

			case TD_TFT_OP_CODE_DELETE_EXISTING_TFT:
			{
				Qa_Process_Bearer_Delete(&tft_recvd, bearer_mod->u8LinkedBearerId, ct_index);
				break;
			}

			case TD_TFT_OP_CODE_ADD_PKT_FLTR_TO_EXISTING_TFT:
			{
				UINT8 i=0; //ded_bearer_index
				//identify which dedicated bearer is being modified

                if (bearer_mod->u8BearerId != bearer_mod->u8LinkedBearerId)
                {
				    for (i=0; i< gsQa_lte[ct_index].u8NumDedicatedBearers; i++)
				    {
				        if (gsQa_lte[ct_index].tDedicatedBearer[i].u8EpsBearerId == bearer_mod->u8BearerId)
				        {
				    	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Process_Bearer_mod: ADD_PKT_FLTR  : Bearer index is %d", i);
				       	    break;
				        }
				    }
				//add the additional filters to the already existing ones
				    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Process_Bearer_Mod: ADD_PKT_FLTR :i is the dedicated bearer index to operate %d\n", i);

				    spin_lock_irqsave(&mr_lock, flags);

				    pTFT = &gsQa_lte[ct_index].tDedicatedBearer[i];

				    spin_unlock_irqrestore(&mr_lock, flags);
                } else
                {
                    pTFT = &gsQa_lte[ct_index].tDefaultBearer;
                }

				pu8Data++;
				ppu8Data = &pu8Data;
				pu16ReadIndex = &u16ReadIndex;
				poffsetlen = &offsetlen;
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Process_Bearer_Mod: ADD_PKT_FLTR : u16ReadIndex %d, offsetlen %d\n", u16ReadIndex, offsetlen);

				Qa_BearerMod_Process_Filter (offsetlen, u16ReadIndex, pu8Data, pnum_filters_TFT, bearer_mod);
				//review again whether ptft_recvd is the right pointer. I think it is.

				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Process_Bearer_Mod: ADD_PKT_FLTR : bearer_index %d\n", i);
                if (bearer_mod->u8BearerId == bearer_mod->u8LinkedBearerId)
                {
                    AMP_LOG_Info("Adding TFT for default bearer on PDN %d\n", ct_index);
                    Qa_Add_SortTFT (ptft_recvd, 0, ct_index, 1); //i is the bearer index to update the PDT_classifier table and serves the as bearer_index
                }
                else
                {
				    Qa_Add_SortTFT (ptft_recvd, i, ct_index, 0); //i is the bearer index to update the PDT_classifier table and serves the as bearer_index
                }
				break;

			}
			case TD_TFT_OP_CODE_REPLACE_PKT_FLTR_TO_EXISTING_TFT:
			{
				UINT8 i=0;
				//identify which dedicated bearer is being modified
                if (bearer_mod->u8BearerId != bearer_mod->u8LinkedBearerId)
                {
				    for (i=0; i< gsQa_lte[ct_index].u8NumDedicatedBearers; i++)
				    {
				        if (gsQa_lte[ct_index].tDedicatedBearer[i].u8EpsBearerId == bearer_mod->u8BearerId)
				        {
                            break;
                        }
				    }
				    pTFT = &gsQa_lte[ct_index].tDedicatedBearer[i];
				}
                else
                {
				    pTFT = &gsQa_lte[ct_index].tDefaultBearer;
                }

				pu8Data++;
				ppu8Data = &pu8Data;
				pu16ReadIndex = &u16ReadIndex;
				poffsetlen = &offsetlen;
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "REPLACE_PKT_FLTR : u16ReadIndex %d, offsetlen %d\n", u16ReadIndex, offsetlen);
				Qa_BearerMod_Process_Filter (offsetlen, u16ReadIndex, pu8Data, pnum_filters_TFT, bearer_mod);

                if (bearer_mod->u8BearerId == bearer_mod->u8LinkedBearerId)
                {
                    AMP_LOG_Info("Replace PKT FILTER to existing TFT for default bearer on PDN %d\n", ct_index);
				    Qa_Replace_Filters (&tft_recvd, i, no_filters, ct_index, 1);
                }
                else
                {
				    Qa_Replace_Filters (&tft_recvd, i, no_filters, ct_index, 0);
                }

				break;
			}

			case TD_TFT_OP_CODE_DELETE_PKT_FLTR_FROM_EXISTING_TFT:
			{
				UINT8 i=0;
				//identify which dedicated bearer is being modified

                if (bearer_mod->u8BearerId != bearer_mod->u8LinkedBearerId)
                {
				    for (i=0; i< gsQa_lte[ct_index].u8NumDedicatedBearers; i++)
				    {
				        if (gsQa_lte[ct_index].tDedicatedBearer[i].u8EpsBearerId == bearer_mod->u8BearerId)
				        {
				    	    break;
				        }
                    }
				}

				//store the Qos info in gsQa_lte DS
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "DELETE_PKT_FLTR : u16ReadIndex %d, offsetlen %d\n", u16ReadIndex, offsetlen);

				pu8Data++;
				ppu8Data = &pu8Data;
				pu16ReadIndex = &u16ReadIndex;
				poffsetlen = &offsetlen;
				//store the info into tft_recvd data structure
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "DELETE_PKT_FLTR : u16ReadIndex %d, offsetlen %d, no_filters %d\n", u16ReadIndex, offsetlen, no_filters);
				Qa_BearerMod_Process_Filter_Del (poffsetlen, pu16ReadIndex , ppu8Data, no_filters);
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "DELETE_PKT_FLTR : bearer_index %d, no_filters %d", i, no_filters);
                if (bearer_mod->u8BearerId == bearer_mod->u8LinkedBearerId)
                {
                    AMP_LOG_Info("Deleting Filter for default bearer on PDN %d\n", ct_index);
                    Qa_Delete_Filter (&tft_recvd, i, no_filters, ct_index,1);
                }
                else
                {
      				Qa_Delete_Filter (&tft_recvd, i, no_filters, ct_index,0);
                }

				break;
			}
			case TD_TFT_OP_CODE_NO_TFT_OPERATION:
			{
				break ; 	//do nothing
			}
			default:
			break;
		   }//end of switch
		} //if offset length

		// if LinkedBearerId is not equal to default bearer id update the default bearer id as per SWDN061
	}
	if ((eMsgId == ESM_BEARER_ALLOC_IND) || (eMsgId == ESM_BEARER_MOD_IND))
	{
		AMP_HOST_MAC_MSG_S * psHmiSend = NULL;
		psHmiSend = kmalloc (sizeof(AMP_HOST_MAC_MSG_S) + sizeof (tHOST_RSP_MSG_S), GFP_KERNEL);
		if (psHmiSend)
		{
                        lte_qa_dev_t* pdev;
                        long pfval;

			/* Send a ESM_BEARER_ALLOC_RSP of ESM_BEARER_MOD_RSP response back to MAC */
			psHmiSend->sHdr.u16HostMsgType = cpu_to_be16 (eMAC_HDR_HOST_MSG_LTE_NAS_TFT_MSG);//host module ID
			psHmiSend->sHdr.u8MacMsgType = LTE_HMM_NAS;//BP module ID
        		psHmiSend->sHdr.u16Length = cpu_to_be16 (sizeof (tHOST_RSP_MSG_S));
			psHmiSend->sHdr.u16Context = cpu_to_be16 (psIpcMsgRsp->sHdr.u16Context);
			if (eMsgId == ESM_BEARER_ALLOC_IND)
				psHmiSend->sHdr.u8MsgSubtype = ESM_BEARER_ALLOC_RSP;
			else if (eMsgId == ESM_BEARER_MOD_IND)
				psHmiSend->sHdr.u8MsgSubtype = ESM_BEARER_MOD_RSP;

			pHostRsp = (tHOST_RSP_MSG_S *) psHmiSend->u8Data;
			memset(psHmiSend->u8Data, 0, sizeof (tHOST_RSP_MSG_S));
			pHostRsp->u8BearerId = bearerid;
			//for now send success /accepted always
			pHostRsp->u8EsmCause = 0;
			pdev = &s_g_pdev;

			AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_ERROR, " Berarer Mod function sending a RSP message back, %p\n", psHmiSend);

    			pfval = PTR_FIFO_ERROR_NONE;
			pfval = ptr_fifo_insert(pdev->h_tx_mgmt_fifo, psHmiSend);
                	if (pfval == PTR_FIFO_ERROR_NONE)
                   		 pdev->tx_mgmt_packet_write_cnt++;
                	else
                	{
                    		AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_ERROR, " Berarer Mod function ERROR sending a RSP message back, insert error on RX mgmt FIFO, dropping packet:%p\n",
                        		psHmiSend);
                    	}
		}
	}

	return eRet;
}
#endif



static AMP_RET_CODE_E Qa_Process_Bearer_Delete(tTFT_RECVD * ptft_recvd, UINT8 LinkedBearerId, INT8 ct_index)
{
	AMP_RET_CODE_E eRet;
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_Process_Bearer_Delete called \n");
	eRet = Qa_Bearer_Delete (ptft_recvd->u8EpsBearerId, LinkedBearerId, ct_index);
	return eRet;
}

#ifdef LTE93
static AMP_RET_CODE_E QA_Process_Bearer_Rel_Cnf(AMP_HOST_MAC_MSG_S * psIpcMsgRsp)
{
	AMP_RET_CODE_E eRet = eAMP_RET_FAIL;
	AMP_HOST_MAC_MSG_S * psHm = NULL;
	tPDN_BEARER_REL_CNF_S * bearer_del;
	UINT8 i; UINT8 * dump;
        INT8 ct_index=0;

    	psHm = (AMP_HOST_MAC_MSG_S *) psIpcMsgRsp;
	//bearer_mod points to the TFT (Qos and filter) data coming from BS. (all hdrs are stripped off)
    	bearer_del = (tPDN_BEARER_REL_CNF_S *)psHm->u8Data;
	dump = (UINT8 *) psHm->u8Data;
	for ( i = 0; i< 10; i++)
	{
		if (dump)
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "QA_Process_Bearer_Rel_Cnf: byte : %d value : %d\n", i+1, *dump);
			dump++;
		}
	}
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Process_Bearer_Rel_Cnf: Bearer deletion: u8BearerId: %d\n", bearer_del->u8BearerId);

// Loop through all as linked bearer ID is not available in this message
        while (eRet == eAMP_RET_FAIL && ct_index < MAX_PDNS)
        {
            if (gsQa_lte[ct_index].tDefaultBearer.u8EpsBearerId != 0)
            {
	    	eRet = Qa_Bearer_Delete (bearer_del->u8BearerId, gsQa_lte[ct_index].tDefaultBearer.u8EpsBearerId, ct_index);
            }
            ct_index++;
        }

	return eRet;

}


static AMP_RET_CODE_E QA_Process_Bearer_Rel(AMP_HOST_MAC_MSG_S * psIpcMsgRsp)
{
	AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
	AMP_HOST_MAC_MSG_S * psHm = NULL;
	tBEARER_REL_IND_S * bearer_del;
	UINT8 i, bearerid = 0;
	UINT8 * dump;
	INT8 ct_index;
	tHOST_RSP_MSG_S * pHostRsp;
	AMP_HOST_MAC_MSG_S * psHmiSend;
   	long pfval;
	lte_qa_dev_t* pdev = &s_g_pdev;

        psHm = (AMP_HOST_MAC_MSG_S *) psIpcMsgRsp;
	//bearer_mod points to the TFT (Qos and filter) data coming from BS. (all hdrs are stripped off)
   	bearer_del = (tBEARER_REL_IND_S *)psHm->u8Data;
	dump = (UINT8 *) psHm->u8Data;

	for ( i = 0; i< 10; i++)
	{
		if (dump)
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Process_Bearer_Rel: byte : %d value : %d\n", i+1, *dump);
			dump++;
		}
	}

	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Process_Bearer_Rel: Bearer deletion: u8BearerId: %d, u8LinkedBearerId: %d\n", bearer_del->u8BearerId, bearer_del->u8LinkedBearerId);

       	ct_index = find_classifier_table(bearer_del->u8LinkedBearerId);
	if (ct_index < 0)
        {
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR, "QA_Process_Bearer_Rel: Classifier table not found\n");
	    return eAMP_RET_FAIL;
        }

	eRet = Qa_Bearer_Delete (bearer_del->u8BearerId, bearer_del->u8LinkedBearerId, ct_index);
	psHmiSend = NULL;
	psHmiSend = kmalloc (sizeof(AMP_HOST_MAC_MSG_S) + sizeof (tHOST_RSP_MSG_S), GFP_KERNEL);
	if (psHmiSend )
	{
		/* Send a ESM_BEARER_ALLOC_RSP of ESM_BEARER_MOD_RSP response back to MAC */

		psHmiSend->sHdr.u16HostMsgType = cpu_to_be16 (eMAC_HDR_HOST_MSG_LTE_NAS_TFT_MSG);//host module ID
		psHmiSend->sHdr.u8MacMsgType = LTE_HMM_NAS;//BP module ID
        	psHmiSend->sHdr.u16Length = cpu_to_be16 (sizeof (tHOST_RSP_MSG_S));
		psHmiSend->sHdr.u16Context =  cpu_to_be16 (psIpcMsgRsp->sHdr.u16Context);

		psHmiSend->sHdr.u8MsgSubtype = ESM_BEARER_REL_RSP;
		pHostRsp = (tHOST_RSP_MSG_S *) psHmiSend->u8Data;
		memset(psHmiSend->u8Data, 0, sizeof (tHOST_RSP_MSG_S));
		pHostRsp->u8BearerId = bearerid;
		//for now send success /accepted always
		pHostRsp->u8EsmCause = 0;

		pfval = PTR_FIFO_ERROR_NONE;
		pfval = ptr_fifo_insert(pdev->h_tx_mgmt_fifo, psHmiSend);
               	if (pfval == PTR_FIFO_ERROR_NONE)
         		 pdev->tx_mgmt_packet_write_cnt++;
               	else
               	{
              		AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_ERROR, "ERROR in QA_Process_Bearer_Rel, insert error on RX mgmt FIFO, dropping packet:%p\n",
                       		psHmiSend);
              	}
	}

	return eRet;
}
#endif

static AMP_RET_CODE_E Qa_Bearer_Delete ( UINT8 bearer_id, UINT8 linked_bearer_id, INT8 ct_index)
{
	AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
	INT8 bearer_index = -1;
	UINT8 i, j;
	tCLASSIFIER_TABLE_ENTRY  *temp;
	tCLASSIFIER_TABLE_ENTRY  *p = NULL;
	tCLASSIFIER_TABLE_ENTRY  *q = NULL;
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_Bearer_Delete : bearer_id :%d, linked_bearer_id : %d classifier table index %d \n",bearer_id, linked_bearer_id, ct_index);

	if (( gsQa_lte[ct_index].tDefaultBearer.u8EpsBearerId == bearer_id) || (bearer_id == 0))
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_Bearer_Delete : You need to delete the  default bearer id \n");
	 	//you need to delete the default bearer id and all the dedicated bearer id associated with default bearer id.

		spin_lock_irqsave(&mr_lock, flags);

		memset(&gsQa_lte[ct_index].tDefaultBearer, 0, sizeof (tTFT));

		for (i =0; i < gsQa_lte[ct_index].u8NumDedicatedBearers; i++)
		{
			memset(&gsQa_lte[ct_index].tDedicatedBearer[i], 0, sizeof (tTFT));
		}

		gsQa_lte[ct_index].u8NumDedicatedBearers = 0;
		//free the memory allocated for every node in the linked list - Do we need to do that again ? TO DO

		p = gsQa_lte[ct_index].tClassifierTable;
		q = p;
		while (p!= NULL)
		{
			q = p->tNextPF;
			kfree (p);
			p = q;
		}
		gsQa_lte[ct_index].tClassifierTable = NULL;
		p = NULL;
		q = NULL;

		spin_unlock_irqrestore(&mr_lock, flags);
	}

	else
	{
	//delete the dedicated bearer
		for (i = 0; i < gsQa_lte[ct_index].u8NumDedicatedBearers; i++)
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Bearer_delete: dedicated bearer id to delete is %d, bearer_id is %d\n", gsQa_lte[ct_index].u8NumDedicatedBearers, bearer_id);
			if (gsQa_lte[ct_index].tDedicatedBearer[i].u8EpsBearerId == bearer_id)
			{
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Bearer_delete: dedicated bearer id to delete is %d \n", bearer_id);
				bearer_index = i;
				break;
			}
		}

		if (bearer_index == -1) {
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Bearer_delete: No bearer found to delete\n");
			return eAMP_RET_FAIL;
		}
	// identify the bearer to delete using bearer_index; after identifying the bearer -
	//delete the filters for the dedicated bearer through dedicated bearer pointers

	for (j = 0; j< MAX_FILTERS_PER_TFT; j++)
	{
		spin_lock_irqsave(&mr_lock, flags);

		temp = gsQa_lte[ct_index].tDedicatedBearer[bearer_index].tPacketFilters[j];
		if (temp == NULL)
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_Bearer_Delete: Skipping filter position %d  \n", j);
		}
		else if (temp == gsQa_lte[ct_index].tClassifierTable)
		{
			p = temp;
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_Bearer_Delete: Have to delete the head of the LL \n");
			p= p->tNextPF;
			p->tPrevPF = NULL;
			gsQa_lte[ct_index].tClassifierTable = p;
		}
		else
		{
			if (temp->tNextPF == NULL)
			{
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_Bearer_Delete- this is the last node \n");
				p = temp->tPrevPF;
				p->tNextPF = NULL;
			}
			else
			{
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_Bearer_Delete- the node is neither the first nor the last node \n");
				p=temp->tNextPF;
				q = temp->tPrevPF;
				q->tNextPF = p;
				p->tPrevPF = q;
			}
		}
		spin_unlock_irqrestore(&mr_lock, flags);
        
        if (temp != NULL)
        {
		    kfree (temp);
		    gsQa_lte[ct_index].tDedicatedBearer[bearer_index].tPacketFilters[j] = NULL;
        }
	}//end of for
	memset(&gsQa_lte[ct_index].tDedicatedBearer[bearer_index], 0, sizeof (tTFT));
	gsQa_lte[ct_index].u8NumDedicatedBearers = gsQa_lte[ct_index].u8NumDedicatedBearers -1;

	} //end of else ( gsQa_lte.tDefaultBearer.u8EpsBearerId == bearer_id)
	return eRet;
}

static AMP_RET_CODE_E Qa_BearerMod_Process_Filter_Del ( UINT16 *poffsetlen, UINT16 *pu16ReadIndex, UINT8 **ppu8Data, UINT8 num_filters)
{

	//UINT8 offsetlen_z = *poffsetlen;
	//UINT8 u16ReadIndex_z = *pu16ReadIndex;
	UINT8 num = 0;
	UINT8 num_del_filters = num_filters;
	UINT8 *pu8Data = *ppu8Data;
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_BearerMod_Process_Filter_Del is called \n");
	AMP_ASSERT(poffsetlen && pu16ReadIndex && ppu8Data, "Qa_BearerMod_Process_Filter_Del: null pointer\n");

	for (num=0; num<num_del_filters; num++)
	{
		tft_recvd.packet_filters.u8PfidList[num] = *pu8Data++;
	}
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_BearerMod_Process_Filter_Del: No. of filters to be deleted is %d \n", num_del_filters);
	return eAMP_RET_SUCCESS;

}


static AMP_RET_CODE_E Qa_Delete_Filter_Replace (tTFT_RECVD * ptft_recvd,
                        UINT8 bearer_index, UINT8 num_filters, INT8 ct_index,
                        bool writeData, UINT8 default_filter)
{
	UINT8 i, j;
	tCLASSIFIER_TABLE_ENTRY *p = NULL, *q = NULL, *temp;
    tTFT  *pTft;
	AMP_ASSERT(ptft_recvd, "Qa_Delete_Filter_Replace: null pointer \n");


    if (default_filter == 1)
    {
        pTft = &(gsQa_lte[ct_index].tDefaultBearer);
    }
    else
    {
        pTft = &(gsQa_lte[ct_index].tDedicatedBearer[bearer_index]);
    }

	spin_lock_irqsave(&mr_lock, flags);

    for (i = 0; i<num_filters; i++)
    {
	    for (j = 0;
             j< MAX_FILTERS_PER_TFT ;
             j++)
        {
			if (  (pTft->tPacketFilters[j] != NULL)  &&
                  ( ptft_recvd->packet_filters.tPacketFilters[i].u8PacketFilterIdentifier ==
                    pTft->tPacketFilters[j]->tPacketFilter.u8PacketFilterIdentifier )
               )
			{
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
                        "Qa_Delete_Filter_Replace: Filter index to "
                        "delete is %d in bearer %d\n", j, bearer_index);
				break;
			}
		}
		if (j <  MAX_FILTERS_PER_TFT)
		{
			//TEMP points to the packet filter that needs to be replaced
		    temp = pTft->tPacketFilters[j];
		    if (temp == NULL)
		    {
		        spin_unlock_irqrestore(&mr_lock, flags);
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
                   "Qa_Delete_Filter_Replace: There is no filter to delete\n");
				return eAMP_RET_FAIL;
		    }

            if (true == writeData)
            {
                if (temp == gsQa_lte[ct_index].tClassifierTable)
                {
                    p = temp;
                    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                       "Qa_Delete_Filter_Replace: Have to delete the head "
                        "of the LL\n");
                    p= p->tNextPF;
                    if (p != NULL) 
                    {
                        p->tPrevPF = NULL;
                    }
                    gsQa_lte[ct_index].tClassifierTable = p;
                }
                else
                {
                    if (temp->tNextPF == NULL)
                    {
                        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA,
                                      AMP_LOG_LEVEL_DEBUG1,
                            "Qa_Delete_Filter_Replace- Filter to be replaced "
                            "is the last node \n");
                        p = temp->tPrevPF;
                        p->tNextPF = NULL;
                    }
                    else
                    {
                        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA,
                                        AMP_LOG_LEVEL_DEBUG1,
                            "Qa_Delete_Filter_Replace- the node is neither "
                            "the first nor the last node \n");
                        p=temp->tNextPF;
                        q = temp->tPrevPF;
                        q->tNextPF = p;
                        p->tPrevPF = q;
				    }
		        }
		        /* I only delete the link of node in the linked list. I do
                    not free the memory Instead, I copy the filter
                    information of TFT_Recvd into the same index of
                    dedicated bearers for e.g., if I am trying to replace
                    filter indentifier 2 of bearer 1,  I copy the TFT info
                    into filter indentifier 2.  */
		        pTft-> tPacketFilters[j]->tNextPF = NULL;
		        pTft->tPacketFilters[j]->tPrevPF = NULL;
		        memcpy ((UINT8 *) &(pTft->tPacketFilters[j]-> tPacketFilter),
                        (UINT8 *)&(ptft_recvd->packet_filters.tPacketFilters[i]),
                        sizeof (tPACKET_FILTER));

		        // you do not fill in the tNextPF and tPrevPF now, since we know
                // which dedicated bearer/filter to process, in this case
                // gsQa_lte.tDedicatedBearer[bearer_index].tPacketFilters[j]
		        // the the tNextPF and tPrevPF is added while sorting the linked
                // list with the replaced filters
		        // gsQa_lte.tDedicatedBearer[bearer_index].u8NumFilters --;
		        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                    "Qa_Delete_Filter_Replace- filter[%d] of bearer [%d] "
                    "is deleted \n", j, bearer_index);

		        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                 "Qa_Delete_Filter_Replace- No of filters now in bearer "
                 "%d is %d\n", bearer_index,
                gsQa_lte[ct_index].tDedicatedBearer[bearer_index].u8NumFilters);
            } // end of if write data
        }//end of if filter found
        else
        {
            spin_unlock_irqrestore(&mr_lock, flags);
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
                "%s: Filter %d not found.\n", __FUNCTION__,
                ptft_recvd->packet_filters.tPacketFilters[i].
                                            u8PacketFilterIdentifier);
            return eAMP_RET_FAIL;
        }
	} // for num_filters
    spin_unlock_irqrestore(&mr_lock, flags);
	return eAMP_RET_SUCCESS;
}


static AMP_RET_CODE_E Qa_Replace_Filters (tTFT_RECVD *ptft_recvd,
            UINT8 bearer_index, UINT8 num_filters, INT8 ct_index, UINT8 default_filter)
{
	AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
	//To replace a filter - We need to delete the corresponding filter and
    // add them again;
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                "Qa_Replace_Filters is called  for PDN: %d , bearer_index: %d , default_filter flag: %d\n",
                ct_index, bearer_index, default_filter );

    // Before modifying the table, make sure the request can be handled.
	eRet = Qa_Delete_Filter_Replace (ptft_recvd, bearer_index, num_filters,
                ct_index, false, default_filter);
    if (eAMP_RET_SUCCESS == eRet)
    {
	    (void)Qa_Delete_Filter_Replace (ptft_recvd, bearer_index, num_filters,
                ct_index, true, default_filter);
    }

	spin_lock_irqsave(&mr_lock, flags);

	eRet = Qa_Replace_SortTFT(ptft_recvd, bearer_index, ct_index, default_filter);

	spin_unlock_irqrestore(&mr_lock, flags);

	return eRet;
}


static AMP_RET_CODE_E Qa_Delete_Filter(tTFT_RECVD * ptft_recvd, UINT8 bearer_index, UINT8 num_filters, INT8 ct_index, UINT8 default_filter)
{
	AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
	UINT8 i, j, u8filter_found_flag = 0;
	tCLASSIFIER_TABLE_ENTRY * p, *q, *temp;
        tTFT *pTft;

	p = NULL;
	q= NULL;
	AMP_ASSERT(ptft_recvd, "Qa_Delete_Filter: null pointer \n");

    if (default_filter == 1)
    {
        pTft = &(gsQa_lte[ct_index].tDefaultBearer);
    }
    else
    {
        pTft = &(gsQa_lte[ct_index].tDedicatedBearer[bearer_index]);
    }

	for (j = 0; j< MAX_FILTERS_PER_TFT; j++)
	{
        if (pTft->tPacketFilters[j] == NULL)
        {
             continue;
        }
		for (i = 0; i<num_filters; i++)
		{
			if (ptft_recvd->packet_filters.u8PfidList[i] == pTft->tPacketFilters[j]->tPacketFilter.u8PacketFilterIdentifier)
			{
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_Delete_Filter: Filter index to delete is %d", j);
				u8filter_found_flag = 1;
				break;
			}
		}
		//p = gsQa_lte.tClassifierTable;
	    if (u8filter_found_flag ==1)
	    {

		spin_lock_irqsave(&mr_lock, flags);

		temp = pTft->tPacketFilters[j];

		if (temp == gsQa_lte[ct_index].tClassifierTable)
		{
			p = temp;
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_Delete_Filter: Have to delete the head of the LL \n");
			p= p->tNextPF;
            if ( p != NULL)
            {
			    p->tPrevPF = NULL;
            }
			gsQa_lte[ct_index].tClassifierTable = p;
		}
		else
		{
			if (temp->tNextPF == NULL)
			{
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_Delete_Filter- this is the last node \n");
				p = temp->tPrevPF;
		                p->tNextPF = NULL;
			}
			else
			{
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_Delete_Filter- the node is neither the first nor the last node \n");
				p=temp->tNextPF;
				q = temp->tPrevPF;
				q->tNextPF = p;
				p->tPrevPF = q;
			}
		}
		spin_unlock_irqrestore(&mr_lock, flags);

		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_Delete_Filter- filter[%d] of bearer [%d] is deleted \n",j, bearer_index);
		memset ((UINT8 *) &(gsQa_lte[ct_index].tDedicatedBearer[bearer_index].tPacketFilters[j]->tPacketFilter), 0, sizeof (tPACKET_FILTER));
		kfree (temp);

		spin_lock_irqsave(&mr_lock, flags);

		pTft->tPacketFilters[j] = NULL;

        pTft->u8NumFilters --;
        if (default_filter && pTft->u8NumFilters == 0)
        {

            Create_Match_All_Filter(ct_index, gsQa_lte[ct_index].tDefaultBearer.u8EpsBearerId);
         // insert match all filter
        }
		spin_unlock_irqrestore(&mr_lock, flags);

		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_Delete_Filter- No of filters now in bearer %d is %d", bearer_index, gsQa_lte[ct_index].tDedicatedBearer[bearer_index].u8NumFilters);
	    }
	    u8filter_found_flag = 0;
	}
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_Delete_Filter : The order of the LL is \n");
        QA_PrintFilterList(ct_index);
	return eRet;
}


static AMP_RET_CODE_E Qa_BearerMod_Process_Filter ( UINT16 offsetlen, UINT16 u16ReadIndex, UINT8 *pu8Data, UINT8 *pnum_pkt_filter, tBEARER_MOD_IND_S * bearer_mod)
{
	AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
	UINT8 i;
 	UINT8 num = 0;
	UINT8 pktfilter_con_len;
	UINT8 pftype_id = 0;
	UINT16 offsetlen_z = offsetlen -1;
	UINT16 u16ReadIndex_z = 0; //*pu16ReadIndex;
	tPACKET_FILTER * ptPacket_filter;
	AMP_ASSERT(pu8Data && pnum_pkt_filter && bearer_mod, "Qa_BearerMod_Process_Filter: null pointer\n");
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_BearerMod_Process_Filter: tft_len %d, u16ReadIndex_z%d \n", offsetlen_z, u16ReadIndex_z);
	while ((offsetlen_z - u16ReadIndex_z)>0)
	{
	        UINT8 junk1;

		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: offsetlen_z is %d ;  u16ReadIndex_z is %d\n", offsetlen_z, u16ReadIndex_z);

		ptPacket_filter = &tft_recvd.packet_filters.tPacketFilters[num];
		ptPacket_filter->u8EpsId =  bearer_mod->u8BearerId;
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: first byte is  %d\n", (*pu8Data));

		junk1 = (*pu8Data)& 0x30;
		ptPacket_filter->u8Direction = junk1 >> 4;//direction
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: Direction is %d\n", ptPacket_filter->u8Direction);

		ptPacket_filter->u8PacketFilterIdentifier = (*pu8Data) & 0x0F;//PFID
		pu8Data++;
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: PFID is %d\n", ptPacket_filter->u8PacketFilterIdentifier);
		ptPacket_filter->u8PacketFilterPrecedence = *pu8Data++;//precedence

		//*pu8data now gives the length
		pktfilter_con_len = *pu8Data++;
		u16ReadIndex_z = u16ReadIndex_z + 3 /* pkt_id + pkt_precedence + length of pkt filter */ + pktfilter_con_len;
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: pktfilter_con_len is %d, %d\n",pktfilter_con_len, u16ReadIndex_z);

		//pu8Data now points to the packet filter contents of a pkt
		while (pktfilter_con_len > 0)//if length of the present pkt filter content
		{
			pftype_id = (*pu8Data) & 0xff;
			pu8Data++;
			pktfilter_con_len--;
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_BearerMod_Process_Filter: pftype_id is %d\n", pftype_id);
		switch (pftype_id)
		{
		     case IPV4_SOURCE_ADDRESS_TYPE:
		     {
		     	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: pftype is %d \n", IPV4_SOURCE_ADDRESS_TYPE);
				pktfilter_con_len = pktfilter_con_len - 8;
				ptPacket_filter->u32ClaMask = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_IPV4_DESTINATION_ADDRESS_TYPE;
				//192.168.0.1 is stored as u8Entry1[0]= 192, u8Entry1[1]= 168, u8Entry1[2]= 0  u8Entry1[4]= 1
				for (i=0; i<4; i++)
				{
					ptPacket_filter->sIpv4DestAddrMask.sEntry[0].u8Entry1[i] = *pu8Data++;
					AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_BearerMod_Process_Filter: the %d byte of IP addr is %d \n", i+1, ptPacket_filter->sIpv4DestAddrMask.sEntry[0].u8Entry1[i]);
				}
				//mask is stored in Entry2
				for (i=0; i<4; i++)
				{
					ptPacket_filter->sIpv4DestAddrMask.sEntry[0].u8Entry2[i] = *pu8Data++;
					AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: the %d byte of IP mask is %d \n", i+1, ptPacket_filter->sIpv4DestAddrMask.sEntry[0].u8Entry2[i]);
				}
				break;
		     }
		     case IPV6_SOURCE_ADDRESS_TYPE:
		     {
		        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: pftye is %d \n", IPV6_SOURCE_ADDRESS_TYPE);
				pktfilter_con_len = pktfilter_con_len -32;
				ptPacket_filter->u32ClaMask  = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_IPV6_DESTINATION_ADDRESS_TYPE;
				for (i=0; i<16; i++)
				{
					ptPacket_filter->sIpv6DestAddrMask.sEntry[0].u8Entry1[i] = *pu8Data++;
					AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_BearerMod_Process_Filter: Dest addr  is %d", ptPacket_filter->sIpv6DestAddrMask.sEntry[0].u8Entry1[i]);
				}
				for (i=0; i<16; i++)
				ptPacket_filter->sIpv6DestAddrMask.sEntry[0].u8Entry2[i] = *pu8Data++;
				break;
		     }
		    case PROTOCOL_IDENTIFIER_TYPE:
		    {
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: pftye is %d \n", PROTOCOL_IDENTIFIER_TYPE);
				pktfilter_con_len = pktfilter_con_len -1;
				ptPacket_filter->u32ClaMask = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_PROTOCOL_IDENTIFIER_TYPE;
				ptPacket_filter->sIpProt.u32NumOfEntry = 1;
				ptPacket_filter->sIpProt.u8Entry[0]= *pu8Data++;
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: protocol identifier is %d \n", ptPacket_filter->sIpProt.u8Entry[0]);
				break;
		    }
			case SINGLE_SOURCE_PORT_TYPE:
			{
		  		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: pftye is %d \n", SINGLE_SOURCE_PORT_TYPE);
				pktfilter_con_len = pktfilter_con_len -2;
				ptPacket_filter->u32ClaMask  = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_SINGLE_SOURCE_PORT_TYPE;
				ptPacket_filter->sIpSrcPort.u32NumOfEntry = 1;
				ptPacket_filter->sIpSrcPort.sEntry[0].u16Entry1 = *pu8Data++;
				ptPacket_filter->sIpSrcPort.sEntry[0].u16Entry1 = ptPacket_filter->sIpSrcPort.sEntry[0].u16Entry1 <<8 | *pu8Data++;
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_BearerMod_Process_Filter: Source port is %d",ptPacket_filter->sIpSrcPort.sEntry[0].u16Entry1);
				break;
			}
			case SINGLE_DESTINATION_PORT_TYPE:
			{
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: pftye is %d \n", SINGLE_DESTINATION_PORT_TYPE);
				pktfilter_con_len = pktfilter_con_len -2;
				ptPacket_filter->u32ClaMask  = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_SINGLE_DESTINATION_PORT_TYPE;
				ptPacket_filter->sIpDestPort.u32NumOfEntry = 1;
				ptPacket_filter->sIpDestPort.sEntry[0].u16Entry1 = *pu8Data++;
				ptPacket_filter->sIpDestPort.sEntry[0].u16Entry1 = ptPacket_filter->sIpDestPort.sEntry[0].u16Entry1 <<8 | *pu8Data++;
				AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Qa_BearerMod_Process_Filter: Dest port is %d \n", ptPacket_filter->sIpDestPort.sEntry[0].u16Entry1);
				break;
			}
		 case DESTINATION_PORT_RANGE_TYPE:
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: pftye is %d \n", DESTINATION_PORT_RANGE_TYPE);
			pktfilter_con_len = pktfilter_con_len -4;
			ptPacket_filter->u32ClaMask  = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_DESTINATION_PORT_RANGE_TYPE;

			ptPacket_filter->sIpDestPort.u32NumOfEntry = 2;
			ptPacket_filter->sIpDestPort.sEntry[0].u16Entry1 = *pu8Data++;
			ptPacket_filter->sIpDestPort.sEntry[0].u16Entry1 = ptPacket_filter->sIpDestPort.sEntry[0].u16Entry1 <<8 | *pu8Data++;
			ptPacket_filter->sIpDestPort.sEntry[0].u16Entry2 = *pu8Data++;
			ptPacket_filter->sIpDestPort.sEntry[0].u16Entry2 = ptPacket_filter->sIpDestPort.sEntry[1].u16Entry1 <<8 | *pu8Data++;
			break;
		}

		case SOURCE_PORT_RANGE_TYPE:
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: pftye is %d \n", SOURCE_PORT_RANGE_TYPE);
			pktfilter_con_len = pktfilter_con_len -4;
			ptPacket_filter->u32ClaMask  = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_SOURCE_PORT_RANGE_TYPE;
			ptPacket_filter->sIpSrcPort.u32NumOfEntry = 2;
			ptPacket_filter->sIpSrcPort.sEntry[0].u16Entry1 = *pu8Data++;
			ptPacket_filter->sIpSrcPort.sEntry[0].u16Entry1 = ptPacket_filter->sIpSrcPort.sEntry[0].u16Entry1 <<8 | *pu8Data++;
			ptPacket_filter->sIpSrcPort.sEntry[0].u16Entry2 = *pu8Data++;
			ptPacket_filter->sIpSrcPort.sEntry[0].u16Entry2 = ptPacket_filter->sIpSrcPort.sEntry[1].u16Entry1 <<8 | *pu8Data++;
			break;
		}
		case SECURITY_PARAMETER_INDEX_TYPE:
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_BearerMod_Process_Filter: pftye is %d \n", SECURITY_PARAMETER_INDEX_TYPE);
			pktfilter_con_len = pktfilter_con_len -4;
			ptPacket_filter->u32ClaMask  = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_SECURITY_PARAMETER_INDEX_TYPE;
			ptPacket_filter->sSpi.u32NumOfEntry = 1;
			ptPacket_filter->sSpi.u32Entry[0] = *pu8Data++;
			ptPacket_filter->sSpi.u32Entry[0] =ptPacket_filter->sSpi.u32Entry[0] << 8 | *pu8Data++;
			ptPacket_filter->sSpi.u32Entry[0] =ptPacket_filter->sSpi.u32Entry[0] << 8 | *pu8Data++;
			ptPacket_filter->sSpi.u32Entry[0] =ptPacket_filter->sSpi.u32Entry[0] << 8 | *pu8Data++;
			break;
		}

		case TYPE_OF_SERVICE_TYPE:
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_BearerMod_Process_Filter: pftye is %d \n", SECURITY_PARAMETER_INDEX_TYPE);
			pktfilter_con_len = pktfilter_con_len -2;
			ptPacket_filter->u32ClaMask  = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_TYPE_OF_SERVICE_TYPE;
			ptPacket_filter->sDscp.u8Entry1 = *pu8Data++; //type of service class field
			ptPacket_filter->sDscp.u8Entry2 = *pu8Data++; // type of service mask field
			break;
		}
		case TRAFFIC_CLASS_TYPE:
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_BearerMod_Process_Filter: pftye is %d \n", SECURITY_PARAMETER_INDEX_TYPE);
			pktfilter_con_len = pktfilter_con_len -2;
			ptPacket_filter->u32ClaMask  = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_TRAFFIC_CLASS_TYPE;
			ptPacket_filter->sDscp.u8Entry1 = *pu8Data++; //type of service class field
			ptPacket_filter->sDscp.u8Entry2 = *pu8Data++; // type of service mask field
			break;
		}
		case FLOW_LABEL_TYPE:
		{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_BearerMod_Process_Filter: pftye is %d \n", SECURITY_PARAMETER_INDEX_TYPE);
			pktfilter_con_len = pktfilter_con_len -3;
			ptPacket_filter->u32ClaMask  = ptPacket_filter->u32ClaMask | LTE_CLA_RULE_FLOW_LABEL_TYPE;
			//MSB is transmitted first
			ptPacket_filter->u8Ipv6FlowLabel[2] = *pu8Data++;
			ptPacket_filter->u8Ipv6FlowLabel[1]= *pu8Data++;
			ptPacket_filter->u8Ipv6FlowLabel[0]= *pu8Data++;
			break;
		}
		default:
		break;
	     }//end of switch case
		 //pu8Data = pu8Data + pktfilter_con_len;
	}//end of while pktfilter_con_len > 0
	num++;
}//end of while ((offsetlen - u16ReadIndex)>0)
*pnum_pkt_filter = num;
return eRet;
}



#ifdef LTE93
static AMP_RET_CODE_E Qa_BearerMod_Process_QOS(tTFT * pTFT1, UINT8 *bearer_mod1)
{
	AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
	UINT8 len;
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_BearerMod_Process_QOS called\n");
	AMP_ASSERT(pTFT1 && bearer_mod1, "Qa_BearerMod_Process_QOS: null pointer\n");
	len = *bearer_mod1;

	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_BearerMod_Process_QOS : len of qos is %d \n", len);
	pTFT1->tHifQos.u8Len =  len;
	bearer_mod1 = bearer_mod1 + len;

	if (len >= 9)
	{
		pTFT1->tHifQos.uGaurenBitRateDownlnkExt = *bearer_mod1;
		bearer_mod1--;
	}
	if (len >= 8)
	{
		pTFT1->tHifQos.uGaurenBitRateUplnkExt = *bearer_mod1;
		bearer_mod1--;
	}
	if (len >= 7)
	{
		pTFT1->tHifQos.uMaxBitRateDownlnkExt = *bearer_mod1;
		bearer_mod1--;
	}
	if (len >= 6)
	{
		pTFT1->tHifQos.uMaxBitRateUplnkExt = *bearer_mod1;
		bearer_mod1--;
	}
	if (len >= 5)
	{
		pTFT1->tHifQos.uGaurenBitRateDownlnk = *bearer_mod1;
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "Qa_BearerMod_Process_QOS : len of qos is %d \n", *bearer_mod1);
		bearer_mod1--;
	}
	if (len >= 4)
	{
		pTFT1->tHifQos.uGaurenBitRateUplnk = *bearer_mod1;
		bearer_mod1--;
	}
	if (len >= 3)
	{
		pTFT1->tHifQos.uMaxBitRateDownlnk = *bearer_mod1;
		bearer_mod1--;
	}
	if (len >= 2)
	{
		pTFT1->tHifQos.uMaxBitRateUplnk = *bearer_mod1;
		bearer_mod1--;
	}
	if (len >= 1)
	{
		pTFT1->tHifQos.uQciLabel = *bearer_mod1;
		bearer_mod1--;
	}

	return eRet;
}
#endif



static AMP_RET_CODE_E Qa_Replace_SortTFT(tTFT_RECVD * ptft_recvd,
                            UINT8 bearer_index, INT8 ct_index, UINT8 default_filter)
{
    // bearer index here is the index you will need to point
    // gsqa_lte.tdedicatedbearer.tCLASSIFIER_TABLE_ENTRY to the
    // appropriate filter.
    UINT8 i, j;
    tCLASSIFIER_TABLE_ENTRY * temp1 = NULL;
    tCLASSIFIER_TABLE_ENTRY *p = NULL;
    tCLASSIFIER_TABLE_ENTRY *q = NULL;
    UINT8 no_filters = ptft_recvd->u8NumFilters;
    tTFT *pTft;

    AMP_ASSERT(ptft_recvd, "Qa_Replace_SortTFT: null pointer\n");
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
        "Qa_Replace_SortTFT: no_filters to add is %d to the bearer index %d\n",
        no_filters, bearer_index);

    if (default_filter == 1)
    {
        pTft = &(gsQa_lte[ct_index].tDefaultBearer);
    }
    else
    {
        pTft = &(gsQa_lte[ct_index].tDedicatedBearer[bearer_index]);
    }

    for (i=0; i < no_filters; i++)
    {
	    for (j = 0;
             j< MAX_FILTERS_PER_TFT;
             j++)
	    {
            if (  ( pTft->tPacketFilters[j] != NULL)  &&
                  ( pTft->tPacketFilters[j]->tPacketFilter.u8PacketFilterIdentifier ==
                ptft_recvd->packet_filters.tPacketFilters[i].u8PacketFilterIdentifier)
               )
		    {
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
                    "Qa_Replace_SortTFT: Filter with PFID %d is replaced "
                    "in the bearer %d\n", gsQa_lte[ct_index].
                    tDedicatedBearer[bearer_index].tPacketFilters[j]->
                    tPacketFilter.u8PacketFilterIdentifier, bearer_index);
                //or temp1 = ptft_recvd->packet_filters.tPacketFilters[i]
			    temp1 = pTft->tPacketFilters[j];
                break;
            }
        }
	    if (NULL != temp1)
	    {
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                    "Qa_Replace_SortTFT: Packet filter precedence of gsQa and"
                    " ptftrecvd are %d and %d", ptft_recvd->packet_filters.
                        tPacketFilters[i].u8PacketFilterPrecedence,
                        temp1->tPacketFilter.u8PacketFilterPrecedence);
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                    "Qa_Replace_SortTFT:  i is %d\n", i);
            if ( (gsQa_lte[ct_index].tClassifierTable != NULL ) && 
                  ptft_recvd->packet_filters.tPacketFilters[i].u8PacketFilterPrecedence <
                  gsQa_lte[ct_index].tClassifierTable->tPacketFilter.u8PacketFilterPrecedence
                )
            { 
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                    "Qa_Replace_SortTFT: replacing a node at the start of "
                    "the list \n");
			    temp1->tNextPF = gsQa_lte[ct_index].tClassifierTable;
			    temp1->tPrevPF = NULL;
			    gsQa_lte[ct_index].tClassifierTable->tPrevPF = temp1;
			    gsQa_lte[ct_index].tClassifierTable = temp1;
            }
            else
            {
                //traverse to the place where this filter shd be inserted
                //p points to the head of the classifier table
                p = gsQa_lte[ct_index].tClassifierTable;
                q = NULL;
                while ( (p != NULL)  && ptft_recvd->packet_filters.tPacketFilters[i].
                                            u8PacketFilterPrecedence >
                        p->tPacketFilter.u8PacketFilterPrecedence)
			    {
                    q = p;
                    p = p->tNextPF;
			    }
			    if ((NULL != p) && (NULL != q)) //if q is not the last node
			    {
                    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                        "Qa_Replace_SortTFT: temp1 should be inserted between "
                        "q and p\n");

				    temp1->tNextPF = p;
				    temp1->tPrevPF = q;
				    q->tNextPF = temp1;
				    p->tPrevPF = temp1;
			    }

			    //temp1 shd be the last node in the list
			    if ((NULL == p) && (NULL != q))
			    {
                    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                        "Qa_Replace_SortTFT: temp1 shd be the last node "
                        "in the list \n");
                    temp1->tNextPF = p;
                    temp1->tPrevPF = q;
                    q->tNextPF = temp1;
			    }

                if ( p == NULL  && q == NULL)
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                        "Qa_Replace_SortTFT: Insert first node in classifier table \n");
				    temp1->tNextPF = p;
				    temp1->tPrevPF = q;
			        gsQa_lte[ct_index].tClassifierTable = temp1;
                }
                    
            } //end of else
	    }//end of if filter found
        else
        {
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
                "%s: Filter %d not found; continuing.\n", __FUNCTION__,
                ptft_recvd->packet_filters.tPacketFilters[i].
                                                u8PacketFilterIdentifier);
        }
	} //end of filter loop

	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
            "Qa_Replace_SortTFT : The order of the LL is:\n");
	QA_PrintFilterList(ct_index);
    return eAMP_RET_SUCCESS;
}// Qa_Replace_SortTFT()


/*
 *  Called when adding TFT to default bearer after attach - delete the match all rule
 *
 */
static AMP_RET_CODE_E  Qa_Delete_Default_Bearer_MatchAll_TFT(INT8 ct_index)
{
	AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
	INT8 bearer_index = -1;
	UINT8 i, j;
	tCLASSIFIER_TABLE_ENTRY  *temp;
	tCLASSIFIER_TABLE_ENTRY  *p = NULL;
	tCLASSIFIER_TABLE_ENTRY  *q = NULL;

    spin_lock_irqsave(&mr_lock, flags);
    temp = gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0];
    if (temp == NULL)
    {
        spin_unlock_irqrestore(&mr_lock, flags);
        AMP_LOG_Error("Qa_Delete_Default_Bearer_MatchAll_TFT: There is no filter to delete \n");
        return eAMP_RET_FAIL;
	}
	else if (temp == gsQa_lte[ct_index].tClassifierTable)
    {
        p = temp;
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_Delete_Default_Bearer_MatchAll_TFT: Delete the first node \n");
        p = p->tNextPF;
        if (p != NULL)
        {
            p->tPrevPF = NULL;
        }
        gsQa_lte[ct_index].tClassifierTable = p;
    }
    else
    {
        if (temp->tNextPF == NULL)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_Delete_Default_Bearer_MatchAll_TFT - Delete  the last node \n");
            p = temp->tPrevPF;
            p->tNextPF = NULL;
        }
        else
        {
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Qa_Delete_Default_Bearer_MatchAll_TFT- Delete interior node \n");
            p=temp->tNextPF;
            q = temp->tPrevPF;
            q->tNextPF = p;
            p->tPrevPF = q;
        }
    }
    spin_unlock_irqrestore(&mr_lock, flags);
    kfree (temp);
    gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[0] = NULL;
    gsQa_lte[ct_index].tDefaultBearer.u8NumFilters = 0;
    gsQa_lte[ct_index].u8MatchAll = 0;
    return eRet;
}


static AMP_RET_CODE_E Qa_Add_SortTFT(tTFT_RECVD * ptft_recvd,
                                    UINT8 bearer_index, INT8 ct_index, UINT8 u8DefaultBearerFlag)
{
    // bearer number here is the index you will need to point
    // gsqa_lte.tdedicatedbearer.tCLASSIFIER_TABLE_ENTRY to the
    // appropriate filter.
    UINT8 i, j;
    tCLASSIFIER_TABLE_ENTRY * temp1 = NULL;
    tCLASSIFIER_TABLE_ENTRY *p = NULL;
    tCLASSIFIER_TABLE_ENTRY *q = NULL;
    UINT8 no_filters = ptft_recvd->u8NumFilters;
    AMP_ASSERT(ptft_recvd, "Qa_Add_SortTFT: null pointer\n");
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
        "Qa_Add_SortTFT: no_filters to add is %d\n to the bearer index %d\n",
        no_filters, bearer_index);


    if  (u8DefaultBearerFlag == 1 && gsQa_lte[ct_index].u8MatchAll == 1)
    {
       // Need to delete Match All Filter if default bearer has any TFTs
       Qa_Delete_Default_Bearer_MatchAll_TFT(ct_index);
    }


    //we need to prioritize the filters within the bearer creation
    for (i=0; i < no_filters; i++)
    {
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
                        "Qa_Add_SortTFT:  i is %d\n", i);
	    if (gsQa_lte[ct_index].tClassifierTable == NULL)
	    {
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
                        "Qa_Add_SortTFT: there are no filters present\n");
	    }
	    else
	    {
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                    "Qa_Add_SortTFT: New filter precedense %d, "
                    "Head of LL filter precedence %d\n",
                    ptft_recvd->packet_filters.tPacketFilters[i].
                    u8PacketFilterPrecedence, gsQa_lte[ct_index].
                    tClassifierTable->tPacketFilter.u8PacketFilterPrecedence);
	    }

        if (
            gsQa_lte[ct_index].tClassifierTable == NULL ||
            (ptft_recvd->packet_filters.tPacketFilters[i].
                                        u8PacketFilterPrecedence <=
            gsQa_lte[ct_index].tClassifierTable->tPacketFilter.
                                                u8PacketFilterPrecedence))
	    {
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                "Qa_Add_SortTFT: Adding a node at the start of the list \n");

		    temp1 = (tCLASSIFIER_TABLE_ENTRY *)kmalloc(
                            sizeof(tCLASSIFIER_TABLE_ENTRY), GFP_KERNEL);

		    if (temp1 ==NULL)
		    {
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
                                "Qa_Add_SortTFT: memory alloc failed \n");
                return eAMP_RET_FAIL;
		    }
		    spin_lock_irqsave(&mr_lock, flags);

		    memcpy (&temp1->tPacketFilter,
                    &ptft_recvd->packet_filters.tPacketFilters[i],
                    sizeof(tPACKET_FILTER));

		    temp1->tNextPF = gsQa_lte[ct_index].tClassifierTable;
		    temp1->tPrevPF = NULL;

            if (gsQa_lte[ct_index].tClassifierTable != NULL)
            {
		         gsQa_lte[ct_index].tClassifierTable->tPrevPF = temp1;
            }
		    gsQa_lte[ct_index].tClassifierTable = temp1;

            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                "Qa_Add_SortTFT: Added a node at the start of the list \n");

		    spin_unlock_irqrestore(&mr_lock, flags);
	    }
	    else
	    {
            //traverse to the place where this filter shd be inserted
            //p points to the head of the classifier table
            p = gsQa_lte[ct_index].tClassifierTable;
            q = NULL;
            if (p != NULL)
		    {
			    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                    "Qa_Add_SortTFT: first pkt filter precedence is %d\n",
                    p->tPacketFilter.u8PacketFilterPrecedence);
		        while (p &&
                       ptft_recvd->packet_filters.tPacketFilters[i].u8PacketFilterPrecedence >
                        p->tPacketFilter.u8PacketFilterPrecedence)
		        {
                    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
                                    "Qa_Add_SortTFT: traverse node \n");
                    q = p;
                    p = p->tNextPF;
		        }
		    }
		    temp1 = (tCLASSIFIER_TABLE_ENTRY *)kmalloc(
                                sizeof(tCLASSIFIER_TABLE_ENTRY), GFP_KERNEL);

		    if (temp1 == NULL)
            {
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
                    "Qa_Add_SortTFT: memory alloc failed \n");
                return eAMP_RET_FAIL;
            }

            spin_lock_irqsave(&mr_lock, flags);

		    if ((NULL != p) && (NULL != q)) //if q is not the last node
		    {
			    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                 "Qa_Add_SortTFT: temp1 should be inserted between q and p \n");
                memcpy (&(temp1->tPacketFilter),
                        &ptft_recvd->packet_filters.tPacketFilters[i],
                        sizeof(tPACKET_FILTER));
			    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                  "Qa_Add_SortTFT: p %p , q %p, q->tNextPF %p, p->tPrevPF %p\n",
                    p, q, q->tNextPF, p->tPrevPF);
			    temp1->tNextPF = p;
			    temp1->tPrevPF = q;
			    q->tNextPF = temp1;
			    p->tPrevPF = temp1;
            }

            //temp1 shd be the last node in the list
		    if ((NULL == p) && (NULL != q))
		    {
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                   "Qa_Add_SortTFT: temp1 shd be the last node in the list \n");
                memcpy (&temp1->tPacketFilter,
                        &ptft_recvd->packet_filters.tPacketFilters[i],
                        sizeof(tPACKET_FILTER));
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                   "Qa_Add_SortTFT: p %p, q %p, q->tNextPF %p\n",
                    p, q, q->tNextPF);
			    temp1->tNextPF = p;
			    temp1->tPrevPF = q;
			    q->tNextPF = temp1;
		    }
            spin_unlock_irqrestore(&mr_lock, flags);
	    } //end of else

        spin_lock_irqsave(&mr_lock, flags);
        if  (u8DefaultBearerFlag == 1)
        {
            for (j=0; j<MAX_FILTERS_PER_TFT; j++)
            {
                if (gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[j] == NULL)
                {
                    break;
                }
            }
            if (j== MAX_FILTERS_PER_TFT)
            {
                AMP_LOG_Error("Qa_Add_SortTFT: No more filter positions available - for default bearer\n");
                spin_unlock_irqrestore(&mr_lock, flags);
                return eAMP_RET_FAIL;
            }
            else
            {
               gsQa_lte[ct_index].tDefaultBearer.tPacketFilters[j] = temp1;
               gsQa_lte[ct_index].tDefaultBearer.u8NumFilters++;
               AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                "Qa_Add_SortTFT: Filter %d is added to DEFAULT bearer\n",
                gsQa_lte[ct_index].tDefaultBearer.u8NumFilters
                );
            }
        }
        else
        {
            for (j=0; j<MAX_FILTERS_PER_TFT; j++)
            {
                if (gsQa_lte[ct_index].tDedicatedBearer[bearer_index].tPacketFilters[j] == NULL)
                {
                    break;
                }
            }
            if (j== MAX_FILTERS_PER_TFT)
            {
                AMP_LOG_Error("Qa_Add_SortTFT: No more filter positions available \n");
                spin_unlock_irqrestore(&mr_lock, flags);
                return eAMP_RET_FAIL;
            }
            else
            {
                gsQa_lte[ct_index].tDedicatedBearer[bearer_index].tPacketFilters[j] =
                                                                          temp1;
                gsQa_lte[ct_index].tDedicatedBearer[bearer_index].u8NumFilters++;
                AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1,
                "Qa_Add_SortTFT: Filter %d is added to bearer %d at position %d\n",
                gsQa_lte[ct_index].tDedicatedBearer[bearer_index].u8NumFilters,
                bearer_index, j);
            }
        }

        spin_unlock_irqrestore(&mr_lock, flags);

    }//end of for
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2,
            "Qa_Add_SortTFT : Filter List in sorted order is:\n");
     QA_PrintFilterList(ct_index);
     return eAMP_RET_SUCCESS;

}//Qa_Add_SortTFT()

static void QA_PrintFilterList(INT8 ct_index)
{
	tCLASSIFIER_TABLE_ENTRY *p = NULL;
	UINT8 i = 0;
	p = gsQa_lte[ct_index].tClassifierTable;

	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_PrintFilterList : In priority order for classifier table %d\n", ct_index);
	while (p)
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_PrintFilterList :  Node : %d ; EpsID : %d ; PFID : %d ; Precedence : %d \n",
		 	i+1, p->tPacketFilter.u8EpsId, p->tPacketFilter.u8PacketFilterIdentifier, p->tPacketFilter.u8PacketFilterPrecedence);
		p = p->tNextPF;
		i++;
	}
}


static AMP_RET_CODE_E QA_ProcessDetachConf_Lte (AMP_HOST_MAC_MSG_S * psIpcMsgRsp)
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    UINT8 n32Index;
    INT8 ct_index;
    UINT8 bearer_id;
	//AMP_WAQA_SF_TRANSACTION_S * psTransaction = NULL;
    //AMP_HOST_MAC_MSG_S * psHm;
    AMP_ASSERT(psIpcMsgRsp, "QA_ProcessDetachConf_Lte: null pointer\n");
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_ProcessDetachConf_Lte: .\n");



    bearer_id = *((UINT8 *) psIpcMsgRsp->u8Data);
    ct_index = find_classifier_table(bearer_id);


    if (ct_index < 0)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR, "QA_ProcessDetachConf_Lte: Cannot find Classifier tables to release - Cannot delete PDN\n");
        return eAMP_RET_FAIL;
    }

	//bearer_mod points to the TFT (Qos and filter) data coming from BS. (all hdrs are stripped off)

	//AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "QA_Process_Bearer_Rel: Bearer deletion: u8BearerId: %d, u8LinkedBearerId: %d\n", bearer_del->u8BearerId, bearer_del->u8LinkedBearerId);
	eRet = Qa_Bearer_Delete (bearer_id, bearer_id, ct_index);


	memset(&gsQa_Tran.sSfTransactionTable, 0, sizeof(gsQa_Tran.sSfTransactionTable));
    for (n32Index=0; n32Index<AMP_WIMAX_MAX_SF_TRANSACTION; n32Index++)
   	{
        	gsQa_Tran.sSfTransactionTable[n32Index].u16TransactionId = n32Index;
        //gsQa.sSfInfo.sSfTransactionTable[n32Index].n32QosId = -1;
	}

	return eRet;

}


#ifdef LTE93
static AMP_RET_CODE_E QA_ProcessRegConf_Lte(AMP_HOST_MAC_MSG_S * psIpcMsgRsp)
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    UINT8 *pdn_cla;
    UINT8 *pdn_cla_end;
    AMP_HOST_MAC_MSG_S * psHm;
    UINT8 qos_len;
    UINT8 bearer_id;
    UINT8 val;

    AMP_ASSERT(psIpcMsgRsp, "QA_ProcessRegConf_Lte: null pointer\n");
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_ProcessRegConf_Lte: Add default bearer\n");

    psHm = (AMP_HOST_MAC_MSG_S *) psIpcMsgRsp;
    pdn_cla = psHm->u8Data;
    pdn_cla_end = psHm->u8Data + psHm->sHdr.u16Length;
    // +2 for bearer id / linked bearer id
    pdn_cla+= 2;
    qos_len = (*pdn_cla);
    bearer_id = ((tBEARER_MOD_IND_S *) psHm->u8Data)->u8BearerId;

    pdn_cla += qos_len + 1;

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_ProcessRegConf_Lte: pdn_cla->u8BearerId is %d, qos len is %d\n", bearer_id, qos_len);
    gsQa_lte[0].tDefaultBearer.u8EpsBearerId = bearer_id;

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_ProcessRegConf_Lte:len of qos is %d", qos_len);

    if ( (pdn_cla < pdn_cla_end) && (*pdn_cla) == TAG_TFA )
    {
      AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "TAG_TFA found");
      pdn_cla +=  *(pdn_cla+1) + 2;
    }
    val = (*pdn_cla);

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "PDN Type == %x\n",val);


    if (pdn_cla < pdn_cla_end)
    {
      AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Searching for PDN Address \n ");
      if( (*pdn_cla) == TAG_PDN_ADDRESS_TYPE )
      {
        gsQa_lte[0].u8PdnType = *(pdn_cla+2);
        memcpy (&(gsQa_lte[0].u8PdnAddress), pdn_cla + 3, *(pdn_cla+1)-1);
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Create default PDN type %d   PDN IP address %x %x %x %x\n", gsQa_lte[0].u8PdnType, gsQa_lte[0].u8PdnAddress[0],gsQa_lte[0].u8PdnAddress[1], gsQa_lte[0].u8PdnAddress[2], gsQa_lte[0].u8PdnAddress[3]);
      }
      else
      {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "TAG type didn't match ");

      }
   }
   else
   {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "Create default PDN PDN IP address NOT present\n");
   }

    // Populate first classifier table
    gsQa_lte[0].tDefaultBearer.tPacketFilters[0] = (tCLASSIFIER_TABLE_ENTRY *) kmalloc (sizeof(tCLASSIFIER_TABLE_ENTRY), GFP_KERNEL);


    spin_lock_irqsave(&mr_lock, flags);


    gsQa_lte[0].tDefaultBearer.u8NumFilters = 1;
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_ProcessRegConf_Lte: default EpsBearerId is %d, Qos len is %d\n",gsQa_lte[0].tDefaultBearer.u8EpsBearerId, gsQa_lte[0].tDefaultBearer.tHifQos.u8Len);
    gsQa_lte[0].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8EpsId = bearer_id;
    gsQa_lte[0].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8Direction =
                                                              DIRECTION_TFT_BI;
    //gsQa_lte.tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8PacketFilterIdentifier = 15; //fill in the value
    gsQa_lte[0].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u8PacketFilterPrecedence = 255; //will always be highest
    gsQa_lte[0].tDefaultBearer.tPacketFilters[0]->tPacketFilter.u32ClaMask = LTE_CLA_RULE_MATCH_ALL;
    gsQa_lte[0].tDefaultBearer.tPacketFilters[0]->tNextPF = NULL;
    gsQa_lte[0].tDefaultBearer.tPacketFilters[0]->tPrevPF = NULL;

    gsQa_lte[0].u8NumDedicatedBearers = 0;
    gsQa_lte[0].u8MatchAll = 1;
    gsQa_lte[0].tClassifierTable = gsQa_lte[0].tDefaultBearer.tPacketFilters[0];

    spin_unlock_irqrestore(&mr_lock, flags);
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "QA_ProcessRegConf_Lte: the head of the LL :%p",  gsQa_lte[0].tClassifierTable);


    return eRet;
}
#endif



static AMP_RET_CODE_E Qa_Process_Pdn_ConnReq (AMP_HOST_MAC_MSG_S * psIpcMsgReq)
{

    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    return eRet;
}


static AMP_RET_CODE_E Qa_Process_Pdn_RelReq (AMP_HOST_MAC_MSG_S * psIpcMsgReq)
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    return eRet;
}


static AMP_RET_CODE_E Qa_Process_Bearer_ModReq (AMP_HOST_MAC_MSG_S * psIpcMsgReq)
{

    return 0;
}

#ifdef LTE92
static AMP_RET_CODE_E Qa_Process_PDNConnRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp)
#elif LTE93
static AMP_RET_CODE_E Qa_Process_PDNConnRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp, UINT16 msgid)
#endif
{
    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    return eRet;
}

static AMP_RET_CODE_E Qa_Process_BearerModRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp)
{

    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;
    return eRet;
}


static AMP_RET_CODE_E Qa_Process_PDNRelRej(AMP_HOST_MAC_MSG_S * psIpcMsgRsp)
{

    AMP_RET_CODE_E eRet = eAMP_RET_SUCCESS;

    return eRet;
}


#endif

//cedar point

/**
 * char device read method (register function).
 *  This function reads either TX mgmt or TX data from the TX fifos.
 *  The payload returned is wrapped in a HIF "driver" header that is
 *  slightly different than the standard HIF header
 *
 * @param struct file* filp
 * @param char __user * buf
 * @param size_t count
 * @param loff_t* f_pos
 *
 * @return ssize_t, bytes read
 */
ssize_t lte_qa_dev_read(struct file* filp, char __user * buf, size_t count, loff_t* f_pos)
{
    char* func_name = "lte_qa_dev_read";
    lte_qa_dev_t* pdev = (lte_qa_dev_t*) filp->private_data;
    long pfval = PTR_FIFO_ERROR_NONE;
    mgmt_packet_t* pmgmt_packet;
    UINT8 j;
    UINT8 *verify;

     AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
        "%s - filp:%p; buf:%p; count:%d; f_pos:%p\n", func_name, filp, buf, (int) count, f_pos);

    // make sure that we have at least the HIF data header size in bytes
    if (count <= sizeof (AMP_HOST_MAC_MSG_HDR_S))
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
            "%s - minimum data size %d requirement not met\n",
            func_name, sizeof (AMP_HOST_MAC_MSG_HDR_S));
        return -EFAULT;
    }

    if (down_interruptible(&pdev->sem))
        return -ERESTARTSYS;

    while (pdev->tx_mgmt_packet_write_cnt == pdev->tx_mgmt_packet_read_cnt)
    {
        // nothing to read, release the lock
        up(&pdev->sem);
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "%s - going to sleep\n", func_name);

        // wait until something is available in the tx data or mgmt fifos
        if (wait_event_interruptible(pdev->txq, pdev->tx_mgmt_packet_write_cnt != pdev->tx_mgmt_packet_read_cnt))
            return -ERESTARTSYS;

        // otherwise loop, but first reacquire the lock
        if (down_interruptible(&pdev->sem))
            return -ERESTARTSYS;
    }
	//tx_mgmt_packet_avail = (int) (pdev->tx_mgmt_packet_write_cnt != pdev->tx_mgmt_packet_read_cnt);
	//to modify
     	pmgmt_packet = NULL;
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "lte_qa_dev_read: b4 ptr fifo \n");
        pfval = ptr_fifo_remove(pdev->h_tx_mgmt_fifo, (void**) &pmgmt_packet);
	pdev->tx_mgmt_packet_read_cnt++;

	j= 20;
    	verify = (UINT8 *)pmgmt_packet;
   	while (j > 0)
   	{
	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "lte_qa_dev_read: after ptr_fifo_remove: %d ; value : %d\n",j , *verify);
	    j--;
	    verify ++;
   	}
        if (pfval == PTR_FIFO_ERROR_NONE)
        {
	    /*
	    // initialize our local HIF header prior to kern-2-user copy
      	    INIT_HIF_HEADER_DRV(phif_hdr_drv, HIF_ENDPOINT_MGMT_OUT,  pmgmt_packet->len);
     	     // copy the header first
      	    if (copy_to_user(buf, phif_hdr_drv, hif_hdr_drv_size))
      	    {
		free_mgmt_payload(pmgmt_packet);
        	return -EFAULT;
      	    }
	    // actual available tx data payload size is total minus the header
            data_payload_size = count - hif_hdr_drv_size;
	    // check for buffer overrun
            count = min(data_payload_size, pmgmt_packet->len);
	    */

	    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "lte_qa_dev_read: after copy_to_user count is %d \n ", count);
	    // copy the pmgmt data
	    //if (copy_to_user(buf + hif_hdr_drv_size, pmgmt_packet->data, count))
            if (copy_to_user(buf, pmgmt_packet, count))
            {
                free_mgmt_payload(pmgmt_packet);
                up(&pdev->sem);
                return -EFAULT;
            }
	    j= 15;
    	    verify = (UINT8 *)buf;

   	    while (j > 0)
   	    {
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "lte_qa_dev_read: after copy_to_user: %d ; value : %d\n",j , *verify);
		j--;
		verify ++;
   	    }
	    	//free_mgmt_payload(pmgmt_packet);
		//*f_pos += count;
	}
	    up(&pdev->sem);
	    return count;
 }


ssize_t lte_qa_dev_write(struct file* filp, const char __user * buf, size_t count, loff_t* f_pos)
{
    char* func_name = "lte_qa_dev_write";
    lte_qa_dev_t* pdev = (lte_qa_dev_t*) filp->private_data;
    size_t data_payload_size = 0;
    UINT8 j;
    UINT8 * verify;
    long pfval;
    mgmt_packet_t* pmgmt_packet;
    AMP_RET_CODE_E eRet;
    AMP_IPC_MSG_S * psIpcMsg;
    mgmt_packet_t* pmgmt_packet1;

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
        "%s - filp:%p; buf:%p; count:%d; f_pos:%p\n",
        func_name, filp, buf, (int) count, f_pos);

   j= 20;
   verify = (UINT8 *)buf;
   while (j > 0)
   {
	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "lte_qa_dev_write: Msg received thru write from IFC bit : %d ; value : %d\n",j , *verify);
	j--;
	verify ++;
    }
    // make sure that we have at least the HIF data header size in bytes
    if (count <= sizeof (AMP_HOST_MAC_MSG_HDR_S))
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
            "%s - minimum data size %d requirement not met\n",
            func_name, sizeof (AMP_HOST_MAC_MSG_HDR_S));
        return -EFAULT;
    }


    // actual data payload size is total minus the header
    data_payload_size = count - sizeof (AMP_HOST_MAC_MSG_HDR_S);


    // now route the payload according to the end point
    pfval = PTR_FIFO_ERROR_NONE;
    pmgmt_packet = NULL;
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING, "%s - MGMT_IN: payload size:%d\n",
                    func_name, data_payload_size);

    if (down_interruptible(&pdev->sem))
	return -ERESTARTSYS;

	// allocate a mgmt packet for this payload
	pmgmt_packet = alloc_mgmt_payload(sizeof (AMP_HOST_MAC_MSG_HDR_S) + data_payload_size);
	if (!pmgmt_packet)
        {
             AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR, "%s - ERROR allocating mgmt packet, dropping RX mgmt packet\n",
                        func_name);
              up(&pdev->sem);
              return -EFAULT;
        }
	// copy payload data portion to mgmt packet
	if (copy_from_user(pmgmt_packet->data, buf, (sizeof (AMP_HOST_MAC_MSG_HDR_S) + data_payload_size)))
        {
        	free_mgmt_payload(pmgmt_packet);
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR, "%s - ERROR (copy_from_user dropping RX mgmt packet\n",
                        func_name);
                up(&pdev->sem);
                return -EFAULT;
	}

	j= 20;
	verify = (UINT8 *)pmgmt_packet->data;
	while (j > 0)
	{
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "lte_qa_dev_write: after copy_from_user : %d ; value : %d\n",j , *verify);
		j--;
		verify ++;
	}

        pfval = ptr_fifo_insert(pdev->h_rx_mgmt_fifo, pmgmt_packet);

        if (pfval == PTR_FIFO_ERROR_NONE)
         	pdev->rx_mgmt_packet_write_cnt++;
       else
       {
		AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR, "%s - ERROR, insert error on RX mgmt FIFO, dropping packet:%p\n",
    	                    func_name, pmgmt_packet);
                count = -EFAULT;
		//free_mgmt_payload(pmgmt_packet);
	}
       up(&pdev->sem);
       // wake anyone waiting for rx data
	wake_up_interruptible(&pdev->rxq);


	/******** PROCESS THE PACKET ACCORDINGLY ********/
	eRet = eAMP_RET_SUCCESS;
    	psIpcMsg = NULL;


    	AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_thread: enter work thread.\n");

	pmgmt_packet1 = NULL;
	ptr_fifo_remove(pdev->h_rx_mgmt_fifo, (void**) &pmgmt_packet1);
	pdev->rx_mgmt_packet_read_cnt++;
        if (pmgmt_packet1)
        {
                AMP_HOST_MAC_MSG_S *psHm;
		j= 15;
	   	verify = (UINT8 *)pmgmt_packet1->data;
	  	while (j > 0)
	   	{
			AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "QA_thread: Msg received thru write from IFC bit : %d ; value : %d\n",j , *verify);
			j--;
			verify ++;
	    	}
            	psHm = NULL;
	    	psHm =  (AMP_HOST_MAC_MSG_S *)(pmgmt_packet1->data);
	    	switch (psHm->sHdr.u8MacMsgType)
            	{
	       		case LTE_HMM_NAS:
            			eRet = QA_ProcessMacMsg(psHm);
                		break;
               		default:
               			eRet = QA_HandleUserMsg(psHm);
                		break;
           	 }
          }//end of (pmgmt_packet1)
          else
          {
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG3, "QA_thread: Null WiMAX message.\n");
          }

      AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, "QA_thread: exit work thread.\n");
      return count;
}
/**
 * Frees a fake management payload
 *
 * @param size_t packet_size
 *
 * @return void
 */
static long free_mgmt_payload(mgmt_packet_t* pmgmt_packet)
{

    if (pmgmt_packet)
    {
        if (pmgmt_packet->data)
            kfree(pmgmt_packet->data);

        kfree(pmgmt_packet);
    }

    return 0;
}

/**
 * Allocates a management packet
 *
 * @param size_t packet_size
 *
 * @return void
 */
static mgmt_packet_t* alloc_mgmt_payload(size_t payload_size)
{
    mgmt_packet_t* pmgmt_packet = NULL;

    // Its late, this is only a test, the two allocs will be removed,
    // yes it is wasteful

    // allocate the structure
    pmgmt_packet = kmalloc(sizeof(mgmt_packet_t), GFP_KERNEL);
    if (!pmgmt_packet)
        return NULL;

    memset(pmgmt_packet, 0x00, sizeof(mgmt_packet_t));
    // allocate the payload
    pmgmt_packet->data = kmalloc(payload_size, GFP_KERNEL);
    if (!pmgmt_packet->data)
    {
        kfree(pmgmt_packet);
        return NULL;
    }

    pmgmt_packet->len = payload_size;

    return pmgmt_packet;
}


/**
 * char device open method (register function)
 *
 * @param struct inode* inode
 * @param struct file* filp
 *
 * @return int, error code
 */
int lte_qa_dev_open(struct inode* inode, struct file* filp)
{
    char* func_name = "lte_qa_dev_open";
    lte_qa_dev_t* pdev = NULL;

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
        "%s - inode:%p; filp:%p\n", func_name, inode, filp);

    // get pointer to our structure
    pdev = (lte_qa_dev_t*) container_of(inode->i_cdev, lte_qa_dev_t, cdev);
    filp->private_data = pdev;

    // perform any other initialization here

    return 0;
}


/**
 * char device release method (register function)
 *
 * @param struct inode* inode
 * @param struct file* filp
 *
 * @return int, error code
 */
int lte_qa_dev_release(struct inode* inode, struct file* filp)
{
    char* func_name = "lte_qa_dev_release";


    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
        "%s - inode:%p; filp:%p\n", func_name, inode, filp);


    return 0;
}

static long lte_qa_flush_tx_mgmt_fifo(lte_qa_dev_t* pdev)
{
    char* func_name = "lte_qa_flush_tx_mgmt_fifo";
    mgmt_packet_t* pmgmt_packet = NULL;
    long pfval = PTR_FIFO_ERROR_NONE;


    // drain the tx mgmt fifo
    while(1)
    {
        pfval = ptr_fifo_remove(pdev->h_tx_mgmt_fifo, (void**) &pmgmt_packet);
        if (pfval != PTR_FIFO_ERROR_NONE)
            break;

        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
            "%s - deleting tx mgmt packet:%p\n", func_name, pmgmt_packet);

        if (pmgmt_packet)
           free_mgmt_payload(pmgmt_packet);
    }

    pdev->tx_mgmt_packet_write_cnt = 0;
    pdev->tx_mgmt_packet_read_cnt = 0;

    return AMP_ERROR_NONE;
}


/**
 * Flushes the rx mgmt fifo
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return long, error code
 */
static long lte_qa_flush_rx_mgmt_fifo(lte_qa_dev_t* pdev)
{
    char* func_name = "lte_qa_flush_rx_mgmt_fifo";
    mgmt_packet_t* pmgmt_packet = NULL;
    long pfval = PTR_FIFO_ERROR_NONE;


    // drain the tx mgmt fifo
    while(1)
    {
        pfval = ptr_fifo_remove(pdev->h_rx_mgmt_fifo, (void**) &pmgmt_packet);
        if (pfval != PTR_FIFO_ERROR_NONE)
            break;

        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
            "%s - deleting rx mgmt packet:%p\n", func_name, pmgmt_packet);

        if (pmgmt_packet)
           free_mgmt_payload(pmgmt_packet);
    }

    pdev->rx_mgmt_packet_write_cnt = 0;
    pdev->rx_mgmt_packet_read_cnt = 0;

    return AMP_ERROR_NONE;
}
struct file_operations g_lte_qos_fops =
{
    .owner =    THIS_MODULE,
    //.llseek =   lte_qa_dev_llseek,
    .read =     lte_qa_dev_read,
    .write =    lte_qa_dev_write,
    .open =     lte_qa_dev_open,
    .release =  lte_qa_dev_release,
};




/**
 * Set the log level for the QOS driver
 *
 * @param short log_level - new log level to use
 *
 * @return long, AMP error code
 */
void QA_SetLogLevel(short log_level)
{
    char* func_name = "QA_SetLogLevel";


    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
        "%s - Using new log level:%d\n",
        func_name, (int) log_level);

    AMP_LOG_SetLogLevel(AMP_LOG_ADDR_WIMAX_QA, log_level);

    return;

}
EXPORT_SYMBOL(QA_SetLogLevel);

/*********************************************************************
 *
 * Description:                 void WIMAX_QA_cleanup_module(void)
 *
 * Arguments (IN/OUT):
 *                              IN:
 *                              OUT:
 *
 * Revision:
 *
 *********************************************************************/
void WIMAX_QA_cleanup_module(void)
{

    dev_t devno = MKDEV(g_dev_major, g_dev_minor);
    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG1, " QA cleanup module\n");
    gsQa.sQaStatus.nEnabled = 0;

if (g_pdev)
{
	// flush and then delete the ptr tx mgmt fifo object
        if (g_pdev->h_tx_mgmt_fifo)
        {
            lte_qa_flush_tx_mgmt_fifo(g_pdev);
            ptr_fifo_free(&g_pdev->h_tx_mgmt_fifo);
        }

        // flush and then delete the ptr rx mgmt fifo object
        if (g_pdev->h_rx_mgmt_fifo)
        {
            lte_qa_flush_rx_mgmt_fifo(g_pdev);
            ptr_fifo_free(&g_pdev->h_rx_mgmt_fifo);
        }
//cedar point
#if defined (DO_DYNAMIC_DEV)
        if (lte_qa_class)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
                "WIMAX_QA_cleanup_module - removing %s class\n", DEVICE_NAME);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24)
            device_destroy(lte_qa_class, devno);
#else
            class_device_destroy(lte_qa_class, devno);
#endif
            class_destroy(lte_qa_class);
        }
#endif

}
	// delete the char device
	cdev_del(&g_pdev->cdev);
        //kfree(g_pdev);
        g_pdev = NULL;
    // now unregister the device
    unregister_chrdev_region(devno, 1);

    return;
}

//// kernel module init

/*********************************************************************
 *
 * Description:                 int WIMAX_QA_init_module(void)
 *
 * Arguments (IN/OUT):
 *                              IN:
 *                              OUT: int
 *
 * Revision:
 *
 *********************************************************************/
int __init WIMAX_QA_init_module(void)
{

    int ret = 0;
    int err = 0;
    int devno = 0;
    dev_t dev = 0;
    long pfval = PTR_FIFO_ERROR_NONE;
    NET_DRV_QOS_INTERFACE qos_interface = {0, 0};

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "WIMAX_QA_init_module: start\n");

    memset((UINT8*)&gsQa_lte, 0, sizeof(gsQa_lte));


    // 1st: register our QOS TX handler, leave the RX handler zero
    // (zero specifies to use the default handler)
    memset(&qos_interface, 0x00, sizeof(qos_interface));
    qos_interface.process_tx_packet_qos = WIMAX_QA_DataPlanTx;
    net_drv_qos_register(qos_interface);

    // 2nd: now set the mode to use our registered handlers
    net_drv_set_qos_mode(NET_DRV_QOS_MODE_QOS_MODULE);


// For Cedar Point

    if (g_dev_major)
    {
        dev = MKDEV(g_dev_major, g_dev_minor);
        ret = register_chrdev_region(dev, 1, DEVICE_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&dev, g_dev_minor, 1, DEVICE_NAME);
        g_dev_major = MAJOR(dev);
    }
    if (ret < 0)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
            " - can't get major %d\n",g_dev_major);
        return ret;
    }

    memset(g_pdev, 0x00, sizeof(lte_qa_dev_t));
    // finish initializing our device structure
    g_pdev->signature = LTE_QA_SIGNATURE;
    g_pdev->size = sizeof(lte_qa_dev_t);
    init_MUTEX(&g_pdev->sem);
    init_waitqueue_head(&g_pdev->txq);
    init_waitqueue_head(&g_pdev->rxq);

    // create a FIFO object for our TX management packets
    pfval = ptr_fifo_alloc(LTE_QA_TX_MGMT_PACKET_FIFO_SIZE, PTR_FIFO_FLAG_NONE, &g_pdev->h_tx_mgmt_fifo);
    if (pfval != PTR_FIFO_ERROR_NONE)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
            " error %d while creating TX mgmt FIFO object\n",(int) pfval);
        ret = -ENOMEM;
        goto INIT_FAIL;
    }

    // create a FIFO object for our RX management packets
    pfval = ptr_fifo_alloc(LTE_QA_RX_MGMT_PACKET_FIFO_SIZE, PTR_FIFO_FLAG_NONE, &g_pdev->h_rx_mgmt_fifo);
    if (pfval != PTR_FIFO_ERROR_NONE)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
            " - error %d while creating RX mgmt FIFO object\n",
             (int) pfval);
        ret = -ENOMEM;
        goto INIT_FAIL;

    }

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
        "Creating cdev: name:%s; major:%d; minor:%d\n",
        DEVICE_NAME, g_dev_major, g_dev_minor);
    devno = MKDEV(g_dev_major, g_dev_minor);
    cdev_init(&g_pdev->cdev, &g_lte_qos_fops);
    g_pdev->cdev.owner = THIS_MODULE;
    g_pdev->cdev.ops = &g_lte_qos_fops;
    err = cdev_add(&g_pdev->cdev, devno, 1);
    if (err)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
            " - error %d while adding cdev: name:%s; major:%d; minor:%d\n",
             err, DEVICE_NAME, g_dev_major, g_dev_minor);
        ret = -ENOMEM;
        goto INIT_FAIL;
    }

// if the following code is not enabled, then the mknod command must
// be run in order to create the device special file

#if defined (DO_DYNAMIC_DEV)
    // create a class structure to be used with creating the device special file
    // that is located under the /dev folder
    lte_qa_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(lte_qa_class))
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
            " Error in creating lte_qa_class class\n");
        ret = -ENOMEM;
        goto INIT_FAIL;
    }
// create the actual device specific file in /dev/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24)
    if(IS_ERR(device_create(lte_qa_class, NULL, devno, "%s", DEVICE_NAME_DEV1)))
#else
    if(IS_ERR(class_device_create(lte_qa_class, NULL, devno, NULL, "%s", DEVICE_NAME_DEV1)))
#endif
    {
        AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_ERROR,
            " Error in creating device special file: %s\n", DEVICE_NAME_DEV1);
        ret = -ENOMEM;
        goto INIT_FAIL;
    }
#endif


    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_DEBUG2, "Classifier_init_module: successful\n");

    return 0;

INIT_FAIL:
    WIMAX_QA_cleanup_module();

    AMP_LOG_Print(AMP_LOG_ADDR_WIMAX_QA, AMP_LOG_LEVEL_WARNING,
        "Classifier_init_module - leave, error:%d\n",  ret);
    return ret;


}

/* Add an EPS ID to MAC address mapping for UL packets
 *
 * @param unsigned char eps_id - the EPS id to use on this interface
 * @param unsigned char* mac_addr - the MAC address to use.  If NULL, then an default
 *  destination MAC address will be used
 *
 * @return long, AMP error code
 */
long lte_qa_set_eps_mac_map(unsigned char eps_id, unsigned char* mac_addr)
{
    char* func_name = "lte_qa_set_eps_mac_map";
    long retval = AMP_ERROR_NONE;
    INT8 ct_index;


    // if the EPS ID is out of range, then error
    if (eps_id > NETDRV_MAX_EPS_ID)
    {
        AMP_LOG_Error("%s - invalid EPS ID specified\n", func_name);
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }

    ct_index = find_classifier_table(eps_id);

    if (ct_index == eAMP_RET_FAIL)
    {
        AMP_LOG_Error ("%s EPS ID %d not found as default bearer ID for any PDN -  Ignored\n", func_name, eps_id);
    }
    else
    if (!mac_addr)
    {
	// rm the mac address mapping
        memset(gsQa_lte[ct_index].u8SrcMacAddr, 0, ETH_ALEN);
    } else
    {
        memcpy(gsQa_lte[ct_index].u8SrcMacAddr, mac_addr, ETH_ALEN);
    }
    // dump the entire table as a reference
    {
        int i;

        for (i = 0; i < MAX_PDNS; i++)
        {
            AMP_LOG_Info(
                "EPS ID to UL MAC Table[ EPS id %d]: mac_addr:%02x:%02x:%02x:%02x:%02x:%02x \n",
                gsQa_lte[i].tDefaultBearer.u8EpsBearerId,
                (int) gsQa_lte[i].u8SrcMacAddr[0],
                (int) gsQa_lte[i].u8SrcMacAddr[1],
                (int) gsQa_lte[i].u8SrcMacAddr[2],
                (int) gsQa_lte[i].u8SrcMacAddr[3],
                (int) gsQa_lte[i].u8SrcMacAddr[4],
                (int) gsQa_lte[i].u8SrcMacAddr[5]);
        }
    }

    return AMP_ERROR_NONE;

}
EXPORT_SYMBOL(lte_qa_set_eps_mac_map);

/**
 * Used to enable or disable UL PDN MAC lookup  mode.  If enabled, allows EPS ID to MAC address
 *  mapping for determining PDN of UL packets.
 *
 * @param unsigned long enable - 0 to disable or != 0 to enable
 *
 * @return long, AMP error code
 */
long lte_qa_set_bridge_mode(unsigned long enable)
{
    int i;

    if (enable)
        g_lte_qa_global_flags |= LTEQA_GLOBAL_FLAG_BRIDGE_MODE;
    else
        g_lte_qa_global_flags &= ~LTEQA_GLOBAL_FLAG_BRIDGE_MODE;


    // if enabled, then dump the entire table as a reference
    if (enable)
    {
        for (i = 0; i < MAX_PDNS; i++)
        {
            AMP_LOG_Info(
                "EPS ID to UL MAC Table[ EPS id %d]: mac_addr:%02x:%02x:%02x:%02x:%02x:%02x \n",
                gsQa_lte[i].tDefaultBearer.u8EpsBearerId,
                (int) gsQa_lte[i].u8SrcMacAddr[0],
                (int) gsQa_lte[i].u8SrcMacAddr[1],
                (int) gsQa_lte[i].u8SrcMacAddr[2],
                (int) gsQa_lte[i].u8SrcMacAddr[3],
                (int) gsQa_lte[i].u8SrcMacAddr[4],
                (int) gsQa_lte[i].u8SrcMacAddr[5]);
        }
    }

    return AMP_ERROR_NONE;
}
EXPORT_SYMBOL(lte_qa_set_bridge_mode);

/**
 * Used to query the bridge mode.
 *
 * @param unsigned long* penabled - 0 is disabled or 1 if enabled
 *
 * @return long, AMP error code
 */
long lte_qa_get_bridge_mode(unsigned long* penabled)
{

    if (!penabled)
    {
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }

    *penabled = (g_lte_qa_global_flags & LTEQA_GLOBAL_FLAG_BRIDGE_MODE) ? 1 : 0;

    return AMP_ERROR_NONE;
}
EXPORT_SYMBOL(lte_qa_get_bridge_mode);

module_init(WIMAX_QA_init_module);
module_exit(WIMAX_QA_cleanup_module);

MODULE_DESCRIPTION("Motorola LTE QoS Agent");
MODULE_LICENSE("GPL");
