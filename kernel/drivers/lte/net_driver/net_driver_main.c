/*
 * Copyright Motorola, Inc. 2009 - 2011
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
 
/*
 * @file net_driver_main.c
 * @brief Network driver main entry file.
 *
 * This file contains the traditional network driver device entry.
 *
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/etherdevice.h>
#include <asm/processor.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/in.h>
#include <linux/ip.h>

#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_lte_log.h>
#include <linux/lte/AMP_lte_error.h>
#include <../common/time_calc.h>

//#define OMAP_DM_PROFILE
#if defined (OMAP_DM_PROFILE)
#include <mach/dmtimer.h>
#endif

#include "net_driver.h"
#include "net_driver_dev.h"
#include "net_driver_config.h"


// IKCHUM-2783: ignore unused variables caused by disabling AMP_LOG_Print
#pragma GCC diagnostic ignored "-Wunused-variable"


//******************************************************************************
// defines
//******************************************************************************

/* Destination (device) MAC address */
static unsigned char default_dev_mac_addr[ETH_ALEN] =
                                        {0x00, 0x14, 0x9A, 0xA5, 0x0A, 0xFE};

/* Source base MAC address (new addresses increment byte 5) */
#define DEFAULT_BAS_MAC_ADDR            {0x00, 0x14, 0x9A, 0xBB, 0xBB, 0xFE}

#define COERCE_VAL(val,min,max)\
    if (val < min)\
        val = min;\
    else if (val > max)\
        val = max;

#define SKB_USER_BYTE_OFFSET_EPS_ID     0

#define NETDRV_GLOBAL_FLAG_NONE         0x00000000
#define NETDRV_GLOBAL_FLAG_BRIDGE_MODE  0x00000001
#define NETDRV_GLOBAL_FLAG_DISABLE_UL  0x00000002

//******************************************************************************
// typedefs
//******************************************************************************

typedef struct
{

    unsigned long busy;
    unsigned long duration;
    unsigned long packet_size;
    unsigned long packet_delay;
    unsigned long tx_count;
    unsigned long rx_count;
    unsigned long tx_bytes;
    unsigned long rx_bytes;
    
} net_drv_perf_thread_data_t;

typedef struct
{

    unsigned char mac_addr[ETH_ALEN];
    unsigned long packet_cnt;

} net_drv_mac_addr_entry_t;

//******************************************************************************
// data
//******************************************************************************


static NET_DRV_PRIVATE* g_net_dev_list[NETDRV_MAX_NUM_DEVICES] = {0};

static NET_DRV_QOS_MODE g_net_drv_qos_mode = NET_DRV_QOS_MODE_IPTABLES;

static NET_DRV_QOS_INTERFACE g_net_drv_qos_interface_default = {0, 0};

static NET_DRV_QOS_INTERFACE g_net_drv_qos_interface_active = {0, 0};

static NET_DRV_QOS_INTERFACE g_net_drv_qos_interface_registered = {0, 0};

static net_drv_perf_thread_data_t g_net_ptd = {0, 0, 0, 0, 0, 0, 0};

static net_drv_mac_addr_entry_t g_net_eps_id_to_mac_addr_table[NETDRV_MAX_NUM_EPS_ID];

static unsigned long g_net_drv_global_flags = NETDRV_GLOBAL_FLAG_NONE;

//******************************************************************************
// static functions
//******************************************************************************


/**
 * Stub QOS function for TX data packets
 *
 * @param struct sk_buff *pskb
 * @param struct unsigned short* poffset
 * @param struct unsigned char* peps_id
 *
 * @return long, error code
 */ 
static long net_drv_process_tx_packet_qos(struct sk_buff *pskb, unsigned short* poffset, unsigned char* peps_id)
{

    if (!pskb || !poffset || !peps_id)
        return AMP_ERROR_NET_DRV_INVALID_PARAM;

    *poffset = ETH_HLEN;
    *peps_id = NETDRV_DEFAULT_EPS_ID;
    
    return AMP_ERROR_NONE;
}


/**
 * Stub QOS function for RX data packets
 *
 * @param struct sk_buff *pskb
 * @param unsigned char* pheader
 * @param struct unsigned short* plength
 *
 *  The buffer passed in is already primed with a correct dst:src:proto ether
 *
 * @return long, error code
 */ 
static long net_drv_process_rx_packet_qos(struct sk_buff *pskb, unsigned char* pheader, unsigned short* plength)
{
    unsigned char ipheader[32];
    unsigned long octet = 0;
    struct iphdr* ih = NULL;
   
    
    if (!pskb || !pheader || !plength)
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    
    if (pskb->len < 20 || *plength < ETH_HLEN)
        return AMP_ERROR_NET_DRV_BUF_TO_SMALL;
    
    // copy first 20 bytes of ip header
    memcpy(ipheader, pskb->data, 20);
    
    ih = (struct iphdr*) ipheader;
    if (ih->version == 4)
    {    
        octet =  ((unsigned long)ipheader[16]) & 0xff;
    
        if ((octet >= 224) && (octet <= 239)) // multicast range
        {
            // multicast ip is (01,00,53, followed by one bit 0 and 23 lower bits of ip address of the packet
            pheader[0] = 0x01;
            pheader[1] = 0x00;
            pheader[2] = 0x5e;
            pheader[3] = ipheader[17] & 0xef; // ef coz the high bit should be zero
            pheader[4] = ipheader[18];
            pheader[5] = ipheader[19];
        }
    }
    
    *plength = ETH_HLEN;
    
    return AMP_ERROR_NONE;
}


/**
 * Detects the IP version (IPV4 or IPV6) of a given IP payload
 *
 * @param pbuffer - pointer to IP header
 * @param buf_size - buffer size
 *
 * @return long, <0 error code, otherwise 4 or 6
 */
static inline long net_drv_detect_IP4_IPV6_packet(char* pbuffer, unsigned long buf_size)
{
    char* func_name = "detect_IP4_IPV6_packet";
    struct iphdr* ih = NULL;
    char ip_version = 0;
    

    if (buf_size < 4)
    {
        AMP_LOG_Error(
            "%s - ERROR: buf_size:%d is less 4 bytes, not a valid IP header\n",
            func_name, (int) buf_size);
        return -1;
    }
    
    // cast to an IP4 header, this is safe since we are only going to dereference the
    // version field which is common between IPV4 and IPV6
    ih = (struct iphdr*)(pbuffer);
    
    // if this is an ip header, then ip version is in the first 4 bytes
    ip_version = (char) ih->version;
    if (ip_version != 4 && ip_version != 6)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - Not a valid IP4 or IPV6 header, IP version detected as:%d\n",
            func_name, (int) ip_version);
        return -1;
    }
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - detected IPV%d packet\n",
        func_name, (int) ip_version);
    
    return (long) ip_version;
}


/**
 * Validates the private pointer as ours
 *
 * @param NET_DRV_PRIVATE* ppriv
 *
 * @return long, error code
 */ 
static inline long net_drv_private_valid(NET_DRV_PRIVATE* ppriv)
{
    
    if (!ppriv || ppriv->signature != NETDRV_SIGNATURE || ppriv->size != sizeof(NET_DRV_PRIVATE))
        return AMP_ERROR_NET_DRV_PRIVATE_INVALID;

    return AMP_ERROR_NONE;
}


/**
 * Adds net device to internal list
 *
 * @param NET_DRV_PRIVATE* ppriv
 *
 * @return long, error code
 */ 
static long net_drv_device_list_add(NET_DRV_PRIVATE* ppriv)
{
    int i = 0;
    
    
    // scan table looking for empty slot
    for (i = 0; i < NETDRV_MAX_NUM_DEVICES; i++)
    {
        if (g_net_dev_list[i] == NULL)
        {
            g_net_dev_list[i] = ppriv;
            return AMP_ERROR_NONE;
        }
    }

    return AMP_ERROR_NET_DRV_MAX_DEVICES;
}


/**
 * Removes net device from internal list
 *
 * @param NET_DRV_PRIVATE* ppriv
 *
 * @return long, error code
 */ 
static long net_drv_device_list_remove(NET_DRV_PRIVATE* ppriv)
{
    int i = 0;
    
    
    // scan table looking for match
    for (i = 0; i < NETDRV_MAX_NUM_DEVICES; i++)
    {
        if (g_net_dev_list[i] == ppriv)
        {
            g_net_dev_list[i] = NULL;
            return AMP_ERROR_NONE;
        }
    }

    return AMP_ERROR_NET_DRV_DEVICE_NOT_FOUND;
}


/**
 * Searches the internal list for the supplied device private
 *
 * @param NET_DRV_PRIVATE* ppriv - search criteria
 *
 * @return int, -1 if NOT found, else an index (0 - NETDRV_MAX_NUM_DEVICES)
 */ 
static int net_drv_device_list_get_entry(NET_DRV_PRIVATE* ppriv)
{
    int i = 0;
    NET_DRV_PRIVATE* pentry = NULL;
    
    // scan table
    for (i = 0; i < NETDRV_MAX_NUM_DEVICES; i++)
    {
        pentry = g_net_dev_list[i];
        if (pentry != NULL)
        {
            if (pentry == ppriv)
            {
                return i;
            }
        }
    }
       
    return -1;
}


/**
 * Returns the free avail in the internal list
 *
 * @param void
 
 * @return int, count of available
 */ 
static int net_drv_device_list_free_avail(void)
{
    int i = 0;
    int avail = 0;


    // scan table looking for empty slot
    for (i = 0; i < NETDRV_MAX_NUM_DEVICES; i++)
        if (g_net_dev_list[i] == NULL)
            avail++;
    
    return avail;
}


/**
 * Sets the active QOS handlers based on the QOS mode
 *
 * @param NET_DRV_QOS_MODE net_drv_qos_mode - the mode to use
 *
 * @return long, error code
 */
static long net_drv_set_active_qos_handers(NET_DRV_QOS_MODE net_drv_qos_mode)
{
    char* func_name = "net_drv_set_active_qos_handers";
    long retval = AMP_ERROR_NONE;


    switch(net_drv_qos_mode)
    {
        case NET_DRV_QOS_MODE_QOS_MODULE:
        
            AMP_LOG_Info(
                "%s - setting QOS handlers for QOS module\n", func_name);
            
            // TODO - add a lock acquire here
            
            // the QOS module uses the registered handlers
            g_net_drv_qos_interface_active = g_net_drv_qos_interface_registered;
            
            // TODO - add a lock release here
            
            break;
                
        case NET_DRV_QOS_MODE_IPTABLES:
        
            AMP_LOG_Info(
                "%s - setting QOS handlers for IP tables\n", func_name);
            
            // TODO - add a lock acquire here
                
            // the IP table mode uses the default handlers
            g_net_drv_qos_interface_active = g_net_drv_qos_interface_default;
    
            // TODO - add a lock release here
            
            break;
    
        default:
        
            AMP_LOG_Error(
                "%s - ERROR: Invalid parameter passed:%d\n", func_name, (int) net_drv_qos_mode);
            
            return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }

   
    return retval;
}


/**
 * Stops the NET queue
 *
 * @param NET_DRV_PRIVATE* ppriv
 *
 * @return long, error code
 */
static inline long net_drv_stop_queue(NET_DRV_PRIVATE* ppriv)
{

    if (!(ppriv->driver_status & NETDRV_STATUS_PERF_TEST))
    {
        netif_stop_queue(ppriv->pnet_dev);
    }
    
    ppriv->driver_status |= NETDRV_STATUS_QUEUE_STOPPED;
        
    return AMP_ERROR_NONE;
}


/**
 * Starts the NET queue
 *
 * @param NET_DRV_PRIVATE* ppriv
 *
 * @return long, error code
 */
static inline long net_drv_start_queue(NET_DRV_PRIVATE* ppriv)
{
    
    if (!(ppriv->driver_status & NETDRV_STATUS_PERF_TEST))
    {
        netif_start_queue(ppriv->pnet_dev);
    }
    
    ppriv->driver_status &= ~NETDRV_STATUS_QUEUE_STOPPED;
    
    return AMP_ERROR_NONE;
}


/**
 * Wakes the NET queue
 *
 * @param NET_DRV_PRIVATE* ppriv
 *
 * @return long, error code
 */
static inline long net_drv_wake_queue(NET_DRV_PRIVATE* ppriv)
{
    
    if (!(ppriv->driver_status & NETDRV_STATUS_PERF_TEST))
    {
        netif_wake_queue(ppriv->pnet_dev);
    }
    
    ppriv->driver_status &= ~NETDRV_STATUS_QUEUE_STOPPED;
    
    return AMP_ERROR_NONE;
}


/**
 * Returns the queue stop status of the driver
 *
 * @param NET_DRV_PRIVATE* ppriv
 * @param long* pstopped
 *
 * @return long, error code
 */
static inline long net_drv_is_queue_stopped(NET_DRV_PRIVATE* ppriv, long* pstopped)
{
        
    *pstopped = (ppriv->driver_status & NETDRV_STATUS_QUEUE_STOPPED) ? 1 : 0;
    
    return AMP_ERROR_NONE;
}


/**
 * Generates a fake source MAC addr.
 *
 * @param mac_addr - buffer to receive fake source MAC address.
 *
 * @return long, error code
 */
static long net_drv_gen_mac_addr(unsigned char mac_addr[ETH_ALEN])
{
    static unsigned char default_mac_addr[ETH_ALEN] = DEFAULT_BAS_MAC_ADDR;
    
    // bump the 5th field
    default_mac_addr[4] += 1;
    
    memcpy(mac_addr, default_mac_addr, ETH_ALEN);
    
    return AMP_ERROR_NONE;
}


/**
 * Used to construct ether header (register function)
 * This function is called to fill up an eth header, since ARP is
 * not available on the interface
 *
 * @param struct net_device* pdev
 *
 * @return int, error code
 */ 
static int net_drv_dev_rebuild_header(struct sk_buff* pskb)
{
    struct ethhdr* peth = (struct ethhdr *) pskb->data;
    struct net_device* pdev = pskb->dev;

    memcpy(peth->h_source, pdev->dev_addr, pdev->addr_len);
    memcpy(peth->h_dest, pdev->dev_addr, pdev->addr_len);
    peth->h_dest[ETH_ALEN-1] ^= 0x01;   /* dest is us xor 1 */
    
    return 0;
}
 

/**
 * Used to construct ether header (register function)
 * This function is called to fill up an eth header, since ARP is
 * not available on the interface
 *
 * @param struct net_device* pdev
 *
 * @return int, error code
 */ 
static int net_drv_dev_header(struct sk_buff* pskb, struct net_device* pdev,
    unsigned short type, const void* pdaddr, const void* psaddr,
    unsigned int len)
{
    struct ethhdr* peth = (struct ethhdr *) skb_push(pskb, ETH_HLEN);

    peth->h_proto = htons(type);
    memcpy(peth->h_source, psaddr ? psaddr : pdev->dev_addr, pdev->addr_len);
    memcpy(peth->h_dest,   pdaddr ? pdaddr : pdev->dev_addr, pdev->addr_len);
    peth->h_dest[ETH_ALEN-1] ^= 0x01;   /* dest is us xor 1 */
    
    return (pdev->hard_header_len);
}
 

static const struct header_ops g_net_drv_dev_header_ops =
{
    .create  = net_drv_dev_header,
    .rebuild = net_drv_dev_rebuild_header,
    .cache   = NULL,  // disable caching
};


/**
 * Net device open method (register function)
 *
 * @param struct net_device* pdev
 *
 * @return int, error code
 */ 
static int net_drv_dev_open(struct net_device* pdev)
{
    char* func_name = "net_drv_dev_open";
    NET_DRV_PRIVATE* ppriv = netdev_priv(pdev);
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - enter: iface:%s\n", func_name, ppriv->dev_name);
        
    // start xmit
    if (netif_queue_stopped(pdev))
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_WARNING,
            "%s - linkup, start the data queue.\n", func_name);

        net_drv_start_queue(ppriv);
    }
    
    return 0;
}


/**
 * Net device release method (register function)
 *
 * @param struct net_device* pdev
 *
 * @return int, error code
 */ 
static int net_drv_dev_release(struct net_device* pdev)
{
    char* func_name = "net_drv_dev_release";
    NET_DRV_PRIVATE* ppriv = netdev_priv(pdev);
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - enter: iface:%s\n", func_name, ppriv->dev_name);

    // stop xmit
    net_drv_stop_queue(ppriv);
    
    return 0;
}


/**
 * Net device config method (register function)
 *
 * @param struct net_device* pdev
 * @param struct ifmap* pmap
 *
 * @return int, error code
 */ 
static int net_drv_dev_config(struct net_device* pdev, struct ifmap* pmap)
{
    char* func_name = "net_drv_dev_config";
    NET_DRV_PRIVATE* ppriv = netdev_priv(pdev);

    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - enter: iface:%s\n", func_name, ppriv->dev_name);
        
    // can't act on a running interface
    if (pdev->flags & IFF_UP) 
        return -EBUSY;

    // ignore all other fields
    return 0;
}


/**
 * Net device tx method (register function)
 *
 * @param struct sk_buff* pskb
 * @param struct net_device* pdev
 *
 * @return int, error code
 */
static int net_drv_dev_tx(struct sk_buff* pskb, struct net_device* pdev)
{
    char* func_name = "net_drv_dev_tx";
    NET_DRV_PRIVATE* ppriv = netdev_priv(pdev);
    unsigned long sl_flags = 0;
    long busval = 0;
    unsigned short offset = 0;
    unsigned char eps_id = 0;

 
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - enter: iface:%s\n", func_name, ppriv->dev_name);

    // if QOS function is available, then process the tx packet
    if (g_net_drv_qos_interface_active.process_tx_packet_qos)
    {
        long qosval = 0;


#if defined (OMAP_DM_PROFILE)
        omap_dm_timer_log_evt(OMAP_DM_TIMER_DEBUG_ID('l', 't', 'e', '1'), (u32)pskb);
#endif

        qosval = g_net_drv_qos_interface_active.process_tx_packet_qos(pskb, &offset, &eps_id);

#if defined (OMAP_DM_PROFILE)
        omap_dm_timer_log_evt(OMAP_DM_TIMER_DEBUG_ID('l', 't', 'e', '2'), (u32)pskb);
#endif

        // if the QOS mode is IP tables, then override the eps_id that is returned
        if (g_net_drv_qos_mode == NET_DRV_QOS_MODE_IPTABLES)
        {
            eps_id = ppriv->eps_id;
        }

        if (!qosval) 
        {
            // should always be 14
            if ((offset > 10) && (offset < 15))
            {
                // remove 14 bytes of the header
                skb_pull(pskb, (unsigned long) offset);
             
                // For data card, we store the EPS id in the "cb" data field of the skb.
                // We do this because when capturing with tcpdump, the ether type
                // field would be displayed as corrupt (unknown).  tcpdump obviously
                // captures the packet when the skb is released and this has caused
                // some confusion.
                net_drv_set_skb_eps_id(pskb, eps_id);
            }
        }
        else
        {
            AMP_LOG_Error(
                "%s - ERROR: iface:%s: QOS process TX packet failed with error %d, dropping packet\n",
                func_name, ppriv->dev_name, (int) qosval);
            
            ppriv->net_stats.tx_errors++;
            ppriv->net_stats.tx_dropped++;
            
            dev_kfree_skb_any(pskb);
            
            return NETDEV_TX_OK;
        }
    }

    if (g_net_drv_global_flags & NETDRV_GLOBAL_FLAG_DISABLE_UL)
    {
        ppriv->net_stats.tx_errors++;
        ppriv->net_stats.tx_dropped++;
            
        dev_kfree_skb_any(pskb);
        
        return NETDEV_TX_OK;
    }


    // drop zero length packets, the data driver can't handle zero length packets at this time
    if (pskb->len == 0)
    {
        AMP_LOG_Error(
            "%s - ERROR: iface:%s: zero length packet received, dropping packet\n",
            func_name, ppriv->dev_name);
        
        ppriv->net_stats.tx_errors++;
        ppriv->net_stats.tx_dropped++;
            
        dev_kfree_skb_any(pskb);
        
        return NETDEV_TX_OK;
    }


    spin_lock_irqsave(&ppriv->sl_lock, sl_flags);
    
    // call the BUS TX start function, on completion the BUS object will call
    // the NET drivers tx_complete function.  If the bus object returns an error
    // here due to FIFO full (sk_buff is not enqueued), device error, etc, we
    // will return an error to the stack. In this case, the kernel will retry
    // at a later time
    busval = ppriv->pnet_drv_bus_interface->tx_start(ppriv, ppriv->pbus, pskb);
    if (busval != AMP_ERROR_NONE)
    {
        // if the FIFO is full, then stop queue, log event and then attempt drain
        if (busval == AMP_ERROR_LTEDD_FIFO_INSERT)
        {
            // stop the queue
            net_drv_stop_queue(ppriv);
        
            // log this stop event with the data driver           
            ppriv->pnet_drv_bus_interface->bus_control(ppriv, ppriv->pbus,
                NETDRV_BUS_CONTROL_LOG_NETQUEUE_EVENT, 1);
        
            spin_unlock_irqrestore(&ppriv->sl_lock, sl_flags);
            
            // force a drain of TX packets           
            ppriv->pnet_drv_bus_interface->bus_control(ppriv, ppriv->pbus,
                NETDRV_BUS_CONTROL_TX_DRAIN, 0);

            // we must restore the header to the original state prior
            // to returning NETDEV_TX_BUSY
            skb_push(pskb, (unsigned long) offset); 

            return NETDEV_TX_BUSY;
        }
        else
        {
            spin_unlock_irqrestore(&ppriv->sl_lock, sl_flags);
             
            AMP_LOG_Error(
                "%s - ERROR: iface:%s: tx_start failed with error %d, dropping packet\n",
                func_name, ppriv->dev_name, (int) busval);
            
            ppriv->net_stats.tx_dropped++;
            ppriv->net_stats.tx_errors++;
            
            dev_kfree_skb_any(pskb);
            
            return NETDEV_TX_OK;
        }
    }
    
    spin_unlock_irqrestore(&ppriv->sl_lock, sl_flags);
       
    return NETDEV_TX_OK;
}


/**
 * Net device ioctl method (register function)
 *
 * @param struct sk_buff* pskb
 * @param struct net_device *dev
 *
 * @return int, error code
 */
static int net_drv_dev_ioctl(struct net_device* pdev, struct ifreq* prq, int cmd)
{
    char* func_name = "net_drv_dev_ioct";
    NET_DRV_PRIVATE* ppriv = netdev_priv(pdev);
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - enter: iface:%s\n", func_name, ppriv->dev_name);
        
    return 0;
}


/**
 * Net device dev status method (register function)
 *
 * @param struct net_device* pdev
 *
 * @return int, error code
 */
static struct net_device_stats* net_drv_dev_stats(struct net_device* pdev)
{
    char* func_name = "net_drv_dev_stats";
    NET_DRV_PRIVATE* ppriv = netdev_priv(pdev);
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - enter: iface:%s\n", func_name, ppriv->dev_name);
    
    return &(ppriv->net_stats);
}


/**
 * Net device dev change mtu method (register function)
 *  The "change_mtu" method is usually not needed.
 *
 * @param struct net_device* pdev
 *
 * @return int, error code
 */
static int net_drv_dev_change_mtu(struct net_device* pdev, int new_mtu)
{
    char* func_name = "net_drv_dev_change_mtu";
    NET_DRV_PRIVATE* ppriv = netdev_priv(pdev);
    unsigned long sl_flags = 0;


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - enter: iface:%s; new_mtu:%d\n", func_name, ppriv->dev_name, new_mtu);
        
    // check ranges
    if ((new_mtu < 68) || (new_mtu > NETDRV_MTU))
        return -EINVAL;

    // do anything you need, and the accept the value
    spin_lock_irqsave(&ppriv->sl_lock, sl_flags);
    pdev->mtu = new_mtu;
    spin_unlock_irqrestore(&ppriv->sl_lock, sl_flags);
    
    return 0;
}


/**
 * Net device dev tx timeout method (register function)
 *
 * @param struct net_device* pdev
 *
 * @return int, error code
 */
static void net_drv_dev_tx_timeout(struct net_device* pdev)
{
    char* func_name = "net_drv_dev_tx_timeout";
    NET_DRV_PRIVATE* ppriv = netdev_priv(pdev);


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - enter: iface:%s\n", func_name, ppriv->dev_name);
          
    return;
}


/**
 * Net device tx complete function (BUS interface function)
 *  All of the following parameters should be the same exact ones that were
 *  passed to the BUS interface function tx_start
 *
 * @param void* pnet - net private data
 * @param void* pbus - bus object
 * @param struct sk_buff* pskb - the skbuff submitted to tx_start
 * @param long fifo_size - FIFO element size (packets)
 * @param long fifo_count - number of elements currently in the FIFO
 *
 * @return long, error code
 */
static long net_drv_tx_complete(void* pnet, void* pbus, struct sk_buff* pskb, long fifo_size, long fifo_count)
{
    char* func_name = "net_drv_tx_complete";
    NET_DRV_PRIVATE* ppriv = (NET_DRV_PRIVATE*) pnet;
    long stopped = 0;
    unsigned long sl_flags = 0;
    long retval = AMP_ERROR_NONE;


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - enter: iface:%s \n", func_name, ppriv->dev_name);
            
    if ((retval = net_drv_private_valid(ppriv)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: private data is invalid\n", func_name);
        return retval;
    }

    // increment some stats and free the skb
    ppriv->net_stats.tx_bytes += pskb->len;
    ppriv->net_stats.tx_packets++;
    dev_kfree_skb_any(pskb);
    
    spin_lock_irqsave(&ppriv->sl_lock, sl_flags);
    
    // get the queue stop status
    net_drv_is_queue_stopped(ppriv, &stopped);
    
    // if the queue was stopped, then check to see if we should restart the queue
    if (stopped)
    {
        long threshold = fifo_size  / 2;   // 50%
        
        // if we are currently stopped and the FIFO has drained past the
        // threshold, then restart the queue
        if (fifo_count <= threshold)
        {
            // log this wake event with the data driver           
            ppriv->pnet_drv_bus_interface->bus_control(ppriv, ppriv->pbus,
                NETDRV_BUS_CONTROL_LOG_NETQUEUE_EVENT, 0);
        
            // wake the queue, sets the flag as well
            net_drv_wake_queue(ppriv);
        }
    }

    spin_unlock_irqrestore(&ppriv->sl_lock, sl_flags);

    return AMP_ERROR_NONE;    
}


/**
 * Net device rx start function (BUS interface function). The bus object
 *  will call this function prior to receiving RX data to allocate and skb
 *
 * @param void* pnet - net private data
 * @param void* pbus - bus object
 * @param unsigned long buf_size - memory size requested
 * @param gfp_t mem_flags - alloc flags
 *
 * @return sk_buff*
 */
 static struct sk_buff* net_drv_rx_start(void* pnet, void* pbus, int buf_size, gfp_t mem_flags)
 {
    char* func_name = "net_drv_rx_start";
    NET_DRV_PRIVATE* ppriv = (NET_DRV_PRIVATE*) pnet;
    struct sk_buff* pskb = NULL;
    int data_len = buf_size;
    long retval = AMP_ERROR_NONE;
    
    
    if ((retval = net_drv_private_valid(ppriv)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: private data is invalid\n", func_name);     
        return NULL;
    }
    
    // the +2 head padding allows for the IP payload to be 32-bit aligned
    // without the +2, alignment will be 16-bit
    
    data_len += ETH_HLEN + 0;
    
    // make sure we don't exceed the MTU size
    if (data_len > (NETDRV_MTU + ETH_HLEN + 0))
    {
        AMP_LOG_Error(
            "%s - ERROR: iface:%s wrong packet size: %d, packet dropped\n",
            func_name, ppriv->dev_name, data_len);
        
        ppriv->net_stats.rx_dropped++;
        ppriv->net_stats.rx_errors++;

        return NULL;
    }

#if defined (NETDRV_FORCE_MIN_SIZE_SKB)
    if (data_len < ETH_ZLEN)
        data_len = ETH_ZLEN;
#endif

    if ((pskb = alloc_skb(data_len, mem_flags)) == NULL)
    {
        AMP_LOG_Error(
            "%s - ERROR: iface:%s alloc_skb failed, packet dropped\n",
            func_name, ppriv->dev_name);
        
        ppriv->net_stats.rx_dropped++;
        ppriv->net_stats.rx_errors++;

        return NULL;
    }
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - iface:%s allocated pskb:%p\n", func_name, ppriv->dev_name, pskb);

#if defined (NETDRV_FORCE_MIN_SIZE_SKB)
    memset(pskb->data, 0, ETH_ZLEN);
#else
    memset(pskb->data, 0, data_len);
#endif

    pskb->dev = ppriv->pnet_dev;
    data_len -= (ETH_HLEN + 0);
    skb_reserve(pskb, (ETH_HLEN + 0));  // for the IP boundary
    skb_put(pskb, data_len);
    
    return pskb;
 }
 
 
/**
 * Net device rx complete function (BUS interface function)
 *
 * @param void* pnet - net private data
 * @param void* pbus - bus object
 * @param struct sk_buff* pskb - net buffer
 * @param unsigned char eps_id - eps id for packet
 * @param int mm_packet - non zero if monitor mode packet
 *
 * @return long, error code
 */
 static long net_drv_rx_complete(void* pnet, void* pbus, struct sk_buff* pskb, unsigned char eps_id, int mm_packet)
 {
    char* func_name = "net_drv_rx_complete";
    NET_DRV_PRIVATE* ppriv = (NET_DRV_PRIVATE*) pnet;
    long retval = AMP_ERROR_NONE;
    unsigned short length = ETH_HLEN;
    unsigned char eth_hdr[ETH_HLEN * 2];
    int netval = NET_RX_SUCCESS;
    long ip_version = 0;

    
    if ((retval = net_drv_private_valid(ppriv)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: private data is invalid\n", func_name);    
        return retval;
    }

    if (mm_packet)
    {
        // MM packet

        // get the cached V4 eth_hdr copy
        memcpy(eth_hdr, ppriv->eth_hdrV4, ETH_HLEN);
    
        // modify the ether type field
        eth_hdr[12] = 0xFA;
        eth_hdr[13] = 0xDE;
        
        memcpy(skb_push(pskb, length), eth_hdr, length);
    }
    else
    {
        // data packet

        // detect if an IPV4 or IPV6 packet
        ip_version = net_drv_detect_IP4_IPV6_packet(pskb->data, (unsigned long) pskb->len);
        switch(ip_version)
        {
            case 4:
                // get the cached eth_hdrV4 copy
                memcpy(eth_hdr, ppriv->eth_hdrV4, ETH_HLEN);
                break;
                
            case 6:
                // get the cached eth_hdrV6 copy
                memcpy(eth_hdr, ppriv->eth_hdrV6, ETH_HLEN);
                break;
            
            default:
                AMP_LOG_Error(
                    "%s - ERROR: iface:%s: Payload not IPV4 or IPV6; IP version:0x%x, dropping packet\n",
                    func_name, ppriv->dev_name, (int) ip_version);
                
                ppriv->net_stats.rx_errors++;
                ppriv->net_stats.rx_dropped++;
            
                dev_kfree_skb_any(pskb);
                return AMP_ERROR_NONE;
        }

        // now if in bridge mode, substitue the dest MAC address with what is in the lookup table
        if (g_net_drv_global_flags & NETDRV_GLOBAL_FLAG_BRIDGE_MODE)
        {
            if (eps_id > NETDRV_MAX_EPS_ID)
            {
                AMP_LOG_Error(
                    "%s - ERROR: Bridge Mode - iface:%s: Invalid EPS ID %d received, using default %d\n",
                    func_name, ppriv->dev_name, (int) eps_id, (int) NETDRV_DEFAULT_EPS_ID);
                
                // in error case, use default
                eps_id = NETDRV_DEFAULT_EPS_ID;
            }
        
            // perform look up and replace the destination MAC address based on the EPS ID
            memcpy(eth_hdr, g_net_eps_id_to_mac_addr_table[eps_id].mac_addr, ETH_ALEN);
            g_net_eps_id_to_mac_addr_table[eps_id].packet_cnt++;
        }
        
        // if QOS function is available, then process the rx packet
        if (g_net_drv_qos_interface_active.process_rx_packet_qos)
        {           
            long qosval = 0;
             
            // allow qos to modify the primed header
            qosval = g_net_drv_qos_interface_active.process_rx_packet_qos(pskb, eth_hdr, &length);
            if (!qosval)
            {
                memcpy(skb_push(pskb, length), eth_hdr, length);    
            }
            else
            {
                AMP_LOG_Error(
                    "%s - ERROR: iface:%s: QOS process RX packet failed with error %d, dropping packet\n",
                    func_name, ppriv->dev_name, (int) qosval);
                
                ppriv->net_stats.rx_errors++;
                ppriv->net_stats.rx_dropped++;
            
                dev_kfree_skb_any(pskb);
                return AMP_ERROR_NONE;
            } 
        }
    }
    
    
    // This function extracts the protocol identifier from the ethernet
    // header; it also assigns skb->mac.raw, removes the hardware header
    // from packet data (with skb_pull), and sets skb->pkt_type
    pskb->protocol = eth_type_trans(pskb, ppriv->pnet_dev);
    pskb->ip_summed = CHECKSUM_UNNECESSARY;
    
    
    if (!(ppriv->driver_status & NETDRV_STATUS_PERF_TEST))
    {
        // notify the kernel (including at interrupt time) -- netif_rx_ni(pskb);
        netval = netif_rx(pskb);        
    }
    else
    {
        // if in performance test mode, just increment counters and free skb
        g_net_ptd.rx_count += 1;
        g_net_ptd.rx_bytes += pskb->len;
        dev_kfree_skb_any(pskb);
    }
    
    if (netval == NET_RX_DROP)
    {
        ppriv->net_stats.rx_errors++;
        ppriv->net_stats.rx_dropped++;
        retval = AMP_ERROR_NET_DRV_PACKET_DROPPED;
    }
    else
    {
        ppriv->net_stats.rx_bytes += pskb->len;
        ppriv->net_stats.rx_packets++;
    }

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - iface:%s submitted pskb:%p (size:%d; mm_packet:%d) to kernel net\n",
        func_name, ppriv->dev_name, pskb, pskb->len, mm_packet);
    
    
    return retval;
 }
 

/**
 * Net device control function (BUS interface function)
 *
 * @param void* pnet - net private data
 * @param void* pbus - bus object
 * @param unsigned long cmd - control command
 * @param unsigned long cmd_data - control command data
 *
 * @return long, error code
 */
 static long net_drv_net_control(void* pnet, void* pbus, unsigned long cmd, unsigned long cmd_data)
 {
    char* func_name = "net_drv_net_control";
    NET_DRV_PRIVATE* ppriv = (NET_DRV_PRIVATE*) pnet;
    unsigned long sl_flags = 0;
    long retval = AMP_ERROR_NONE;
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - pnet:%p; pbus:%p; cmd:0x%x\n",
        func_name, pnet, pbus, (int) cmd);
        
    // validate net object
    if ((retval = net_drv_private_valid(ppriv)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: private data is invalid\n", func_name);
        return retval;
    }
    
    // perform operation
    switch(cmd)
    {
        case NETDRV_NET_CONTROL_SUSPEND:
            spin_lock_irqsave(&ppriv->sl_lock, sl_flags);
            net_drv_stop_queue(ppriv);
            spin_unlock_irqrestore(&ppriv->sl_lock, sl_flags);
            break;
        
        case NETDRV_NET_CONTROL_RESUME:
            spin_lock_irqsave(&ppriv->sl_lock, sl_flags);
            net_drv_wake_queue(ppriv);
            spin_unlock_irqrestore(&ppriv->sl_lock, sl_flags);
            break;
            
        case NETDRV_NET_CONTROL_RESET:
            // nop
            break;
            
        case NETDRV_NET_CONTROL_FLUSH:
            // no FIFO's at this time
            break;

    }
    
    return AMP_ERROR_NONE;
 }


//******************************************************************************
// exported functions
//******************************************************************************


/**
 * Returns the first free avail index, used to aid in the 
 *  device name generation that is passed to the net_drv_probe
 *  function.  call this function to get the index and then
 *  specify lte[index] as the name to the probe function
 *
 * @param void
 
 * @return int, -1 in none avail, otherwise the index
 */ 
int net_drv_device_list_first_avail(void)
{
    int i = 0;


    // scan table looking for empty slot
    for (i = 0; i < NETDRV_MAX_NUM_DEVICES; i++)
        if (g_net_dev_list[i] == NULL)
            return i;
    
    return -1;
}
EXPORT_SYMBOL(net_drv_device_list_first_avail);


/**
 * Returns net device private from from internal list that matches the
 *  supplied criteria
 *
 * @param char* pdev_name - search criteria
 * @param void* pbus - search criteria, if NULL, then use only name
 *
 * @return void*, 0 or valid pointer
 */ 
void* net_drv_device_list_find_name(char* pdev_name, void* pbus)
{
    int i = 0;
    NET_DRV_PRIVATE* ppriv = NULL;
    
    
    if (!pdev_name)
        return NULL;
        
    // scan table looking for match
    for (i = 0; i < NETDRV_MAX_NUM_DEVICES; i++)
    {
        ppriv = g_net_dev_list[i];
        if (ppriv != NULL)
        {
            if (strcmp(pdev_name, ppriv->dev_name) == 0)
            {
                if (pbus == NULL)
                    return ppriv;
                else
                {
                    if (pbus == ppriv->pbus)
                        return ppriv;
                }
            }
        }
    }

    return NULL;
}
EXPORT_SYMBOL(net_drv_device_list_find_name);


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
static const struct net_device_ops g_net_drv_netdev_ops =
{
    .ndo_open = net_drv_dev_open,
    .ndo_stop = net_drv_dev_release,    
    .ndo_start_xmit = net_drv_dev_tx,
    .ndo_do_ioctl = net_drv_dev_ioctl,
    .ndo_set_config = net_drv_dev_config,
    .ndo_change_mtu = net_drv_dev_change_mtu,
    .ndo_tx_timeout = net_drv_dev_tx_timeout,
    .ndo_get_stats = net_drv_dev_stats
};
#endif


/**
 * hardware probe function
 *
 * @param char* dev_name - name of device to create
 * @param void* pbus - bus object
 * @param NET_DRV_BUS_INTERFACE* pnet_drv_bus_interface - interface functions between NET and BUS
 * @param void** ppnet - receives net driver object handle
 *
 * WARNING!!!!! - the NET_DRV_BUS_INTERFACE* storage must be manage by the object that calls this
 *  function.  Therefore, care must be taken to ensure that this net object is destroyed via the
 *  net_drv_disconnect() function prior releasing the memory/code of the calling/managing object.
 * 
 * @return long, AMP error code
 */ 
long net_drv_probe(char* pdev_name, void* pbus, NET_DRV_BUS_INTERFACE* pnet_drv_bus_interface, void** ppnet)
{
    char* func_name = "net_drv_probe";
    NET_DRV_PRIVATE* ppriv = NULL;
    struct net_device* pnet_dev = NULL;
    unsigned char src_mac_addr[ETH_ALEN] = {0};
    int avail = 0;
    int regval = 0;
    long retval = AMP_ERROR_NONE;
    long addval = AMP_ERROR_NONE;
    
    
    // validate params
    if (!pdev_name || !pbus || !pnet_drv_bus_interface || !ppnet)
    {
        AMP_LOG_Error(
            "%s - ERROR: invalid parameters passed\n", func_name);
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }
    
    *ppnet = NULL;
    
    // first check to see if we can add another device
    avail = net_drv_device_list_free_avail();
    if (!avail)
    {
        AMP_LOG_Error(
            "%s - ERROR: the maximum number of devices has been allocated\n", func_name);
        return AMP_ERROR_NET_DRV_MAX_DEVICES;
    }
    
    // verify that the BUS functions have been initialized.  The BUS
    // object that calls this probe function MUST initialized the
    // following functions
    if (!(pnet_drv_bus_interface->tx_start)     ||
        !(pnet_drv_bus_interface->bus_control))
    {
        AMP_LOG_Error(
            "%s - ERROR: invalid BUS supplied interface functions - tx_start%p; bus_control:%p\n",
            func_name, 
            pnet_drv_bus_interface->tx_start,
            pnet_drv_bus_interface->bus_control);
            
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }
        
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - creating network device using name:%s; bus:%p\n", func_name, pdev_name, pbus);
    
    // allocate a net device using specified name, use stock ether_setup function
    if (!(pnet_dev = alloc_netdev(sizeof(NET_DRV_PRIVATE), pdev_name, ether_setup)))
    {
        AMP_LOG_Error(
            "%s - ERROR: error creating network device using name:%s\n", func_name, pdev_name);
        return AMP_ERROR_NET_DRV_RESOURCE_ALLOC;
    }
    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
    pnet_dev->netdev_ops = &g_net_drv_netdev_ops;
    pnet_dev->header_ops = &g_net_drv_dev_header_ops;
#else
    // initialize the function pointers
    pnet_dev->open = net_drv_dev_open;
    pnet_dev->stop = net_drv_dev_release;
    pnet_dev->set_config = net_drv_dev_config;
    pnet_dev->hard_start_xmit = net_drv_dev_tx;
    pnet_dev->do_ioctl = net_drv_dev_ioctl;
    pnet_dev->get_stats = net_drv_dev_stats;
    pnet_dev->change_mtu = net_drv_dev_change_mtu;
    pnet_dev->tx_timeout = net_drv_dev_tx_timeout;
    pnet_dev->header_ops = &g_net_drv_dev_header_ops;
#endif

    // initialize data members
    pnet_dev->watchdog_timeo = (10 * HZ);
    pnet_dev->mtu = NETDRV_MTU; 
    pnet_dev->flags |= IFF_NOARP;
    pnet_dev->features |= NETIF_F_NO_CSUM;
    
    // initialize our private data
    ppriv = netdev_priv(pnet_dev);
    memset(ppriv, 0x00, sizeof(NET_DRV_PRIVATE));
    
    ppriv->signature = NETDRV_SIGNATURE;
    ppriv->size = sizeof(NET_DRV_PRIVATE);
    ppriv->pnet_dev = pnet_dev;
    ppriv->pbus = pbus;
    ppriv->pnet_drv_bus_interface = pnet_drv_bus_interface;
    ppriv->powner = THIS_MODULE;
    ppriv->driver_status |= (NETDRV_STATUS_INITIAL | NETDRV_STATUS_LINKDOWN);
    ppriv->eps_id = NETDRV_DEFAULT_EPS_ID;
    strncpy(ppriv->dev_name, pdev_name, NETDRV_MAX_DEV_NAME - 1);
    
    // generate a source MAC address
    net_drv_gen_mac_addr(src_mac_addr);

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - using MAC address:%02x:%02x:%02x:%02x:%02x:%02x\n",
        func_name,
        src_mac_addr[0], src_mac_addr[1], src_mac_addr[2],
        src_mac_addr[3], src_mac_addr[4], src_mac_addr[5]);
 
    memcpy(pnet_dev->dev_addr, default_dev_mac_addr, ETH_ALEN);

    // cache a copy of a ether header for default qos RX processing
    // dst: a constant value that denotes the target device
    // src: a variable value generated based on the interface
    // proto: IP header
    memcpy(ppriv->eth_hdrV4, default_dev_mac_addr, ETH_ALEN);
    memcpy(ppriv->eth_hdrV4 + ETH_ALEN, src_mac_addr, ETH_ALEN);
    ppriv->eth_hdrV4[ETH_ALEN + ETH_ALEN] = 0x08;
    ppriv->eth_hdrV4[ETH_ALEN + ETH_ALEN + 1] = 0x00;
    memcpy(ppriv->eth_hdrV6, ppriv->eth_hdrV4, ETH_HLEN);
    ppriv->eth_hdrV6[ETH_ALEN + ETH_ALEN] = 0x86;
    ppriv->eth_hdrV6[ETH_ALEN + ETH_ALEN + 1] = 0xDD;

    // for locking device
    spin_lock_init(&ppriv->sl_lock);
    
    // now register this network device
    if ((regval = register_netdev(pnet_dev)))
    {
        AMP_LOG_Error(
            "%s - ERROR: error %d registering device", func_name, regval);
        
        retval = AMP_ERROR_NET_DRV_REGISTRATION;
        goto ERROR_EXIT;
    }
    
    // flag as registered
    ppriv->driver_status |= NETDRV_STATUS_REGISTERED;
    
    // now add to our internal list
    if ((addval = net_drv_device_list_add(ppriv)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: error %d adding device to internal list", func_name, (int) addval);
            
        retval = AMP_ERROR_NET_DRV_INTERNAL;
        goto ERROR_EXIT;
    }

    // now finish our part of the BUS interface functions
    ppriv->pnet_drv_bus_interface->tx_complete = net_drv_tx_complete;
    ppriv->pnet_drv_bus_interface->rx_start = net_drv_rx_start;
    ppriv->pnet_drv_bus_interface->rx_complete = net_drv_rx_complete;
    ppriv->pnet_drv_bus_interface->net_control = net_drv_net_control;

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - BUS interface functions - tx_start:%p; tx_complete:%p; rx_start:%p; rx_complete:%p; net_control:%p; bus_control:%p\n",
        func_name, 
        ppriv->pnet_drv_bus_interface->tx_start,
        ppriv->pnet_drv_bus_interface->tx_complete,
        ppriv->pnet_drv_bus_interface->rx_start,
        ppriv->pnet_drv_bus_interface->rx_complete,
        ppriv->pnet_drv_bus_interface->net_control,
        ppriv->pnet_drv_bus_interface->bus_control);

    
    // link up at this point
    ppriv->driver_status &= ~NETDRV_STATUS_LINKDOWN;
    netif_carrier_on(pnet_dev);

    // now return the private as the net driver handle
    *ppnet = ppriv;

    return retval;


ERROR_EXIT:

    if (ppriv && ppriv->pnet_dev && ppriv->driver_status & NETDRV_STATUS_REGISTERED)
    {
        unregister_netdev(ppriv->pnet_dev);
        ppriv->driver_status &= ~NETDRV_STATUS_REGISTERED;
    }

    if (ppriv && ppriv->pnet_dev)
    {
        free_netdev(ppriv->pnet_dev);
        ppriv->pnet_dev = NULL;
    }


    return retval;  
}
EXPORT_SYMBOL(net_drv_probe);


/**
 * hardware disconnect function
 *
 * @param void* pnet - net driver object retured from net_drv_probe
 *
 * @return long, AMP error code
 */ 
long net_drv_disconnect(void* pnet)
{
    char* func_name = "net_drv_disconnect";
    NET_DRV_PRIVATE* ppriv = (NET_DRV_PRIVATE*) pnet;
    long retval = AMP_ERROR_NONE;
    int entry = -1;
    
    
    // validate params
    if (!ppriv)
    {
        AMP_LOG_Error(
            "%s - ERROR: invalid parameters passed\n", func_name);
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }
    
    // validate net driver object
    if ((retval = net_drv_private_valid(ppriv)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: private data ppriv:%p is invalid\n", func_name, ppriv);
        return retval;
    }
    
    // now see if it is in our list
    entry = net_drv_device_list_get_entry(ppriv);
    if (entry < 0)
    {
        AMP_LOG_Error(
            "%s - ERROR: no network device found that matches ppriv:%p\n",
            func_name, ppriv);
        return AMP_ERROR_NET_DRV_DEVICE_NOT_FOUND;    
    }
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - releasing network device name:%s; bus:%p\n",
        func_name, ppriv->dev_name, ppriv->pbus);
 
    // shutdown the network link first
    ppriv->driver_status |= NETDRV_STATUS_LINKDOWN;
    
    if (ppriv->pnet_dev)
    {
        // last piece is to remove the network interface
        if (ppriv->driver_status & NETDRV_STATUS_REGISTERED)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                "%s - calling unregister_netdev()\n", func_name);
                
            netif_device_detach(ppriv->pnet_dev);
            net_drv_stop_queue(ppriv);
            netif_carrier_off(ppriv->pnet_dev);
            
            unregister_netdev(ppriv->pnet_dev);
            
            ppriv->driver_status &= ~NETDRV_STATUS_REGISTERED;
        }
            
        //
        // RTWBUG - do we need a delay here?
        //
    
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
            "%s - calling free_netdev()\n", func_name);

        free_netdev(ppriv->pnet_dev);
    }

    // remove from our internal list
    net_drv_device_list_remove(ppriv);
    

    return AMP_ERROR_NONE;
    
}
EXPORT_SYMBOL(net_drv_disconnect);


/**
 * QOS register function
 *
 * @param NET_DRV_QOS_INTERFACE net_drv_qos_interface - QOS callback handlers
 *
 * @return long, AMP error code
 */
long net_drv_qos_register(NET_DRV_QOS_INTERFACE net_drv_qos_interface)
{
    char* func_name = "net_drv_qos_register";

    
    AMP_LOG_Info(
        "%s - Registering new QOS callback functions\n",
        func_name);
        
    // TODO - add a lock acquire here
    
    // if the TX function pointer is set to NULL, then we use the built-in default handler
    if (net_drv_qos_interface.process_tx_packet_qos == 0)
        g_net_drv_qos_interface_registered.process_tx_packet_qos = net_drv_process_tx_packet_qos;
    else
        g_net_drv_qos_interface_registered.process_tx_packet_qos = net_drv_qos_interface.process_tx_packet_qos;
    
    // if the RX function pointer is set to NULL, then we use the built-in default handler
    if (net_drv_qos_interface.process_rx_packet_qos == 0)
        g_net_drv_qos_interface_registered.process_rx_packet_qos = net_drv_process_rx_packet_qos;
    else
        g_net_drv_qos_interface_registered.process_rx_packet_qos = net_drv_qos_interface.process_rx_packet_qos;

   // TODO - add a lock release here
  
    
    return AMP_ERROR_NONE;
    
}
EXPORT_SYMBOL(net_drv_qos_register);


/**
 * Set EPS ID for the specified interface
 *
 * @param void* pnet - net driver object retured from net_drv_probe OR obtained
 *  from the query net object functions
 * @param unsigned char eps_id - the EPS id to use on this interface 
 *
 * @return long, AMP error code
 */ 
long net_drv_set_eps_id(void* pnet, unsigned char eps_id)
{
    char* func_name = "net_drv_set_eps_id";
    NET_DRV_PRIVATE* ppriv = (NET_DRV_PRIVATE*) pnet;
    long retval = AMP_ERROR_NONE;


    // validate net driver object
    if ((retval = net_drv_private_valid(ppriv)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: private data ppriv:%p is invalid\n", func_name, ppriv);
        return retval;
    }

    // if the EPS ID is out of range, then error
    if (eps_id > NETDRV_MAX_EPS_ID)
    {
        AMP_LOG_Error("%s - invalid EPS ID specified\n", func_name);
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }
    
    ppriv->eps_id = eps_id;

    AMP_LOG_Info(
        "%s - Using new EPS ID:%d for iface:%s\n",
        func_name, (int) eps_id, ppriv->dev_name);
    
    return AMP_ERROR_NONE;
    
}
EXPORT_SYMBOL(net_drv_set_eps_id);



/**
 * Add an EPS ID to MAC address mapping for DL packets
 *
 * @param unsigned char eps_id - the EPS id to use on this interface
 * @param unsigned char* mac_addr - the MAC address to use.  If NULL, then an default
 *  destination MAC address will be used
 *
 * @return long, AMP error code
 */ 
long net_drv_set_eps_mac_map(unsigned char eps_id, unsigned char* mac_addr)
{
    char* func_name = "net_drv_set_eps_mac_map";
    long retval = AMP_ERROR_NONE;


    // if the EPS ID is out of range, then error
    if (eps_id > NETDRV_MAX_EPS_ID)
    {
        AMP_LOG_Error("%s - invalid EPS ID specified\n", func_name);
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }
    
    // if NULL, then reset back to the default MAC address
    if (!mac_addr)
    {
        memcpy(g_net_eps_id_to_mac_addr_table[eps_id].mac_addr, default_dev_mac_addr, ETH_ALEN);
    }
    else
    {
        memcpy(g_net_eps_id_to_mac_addr_table[eps_id].mac_addr, mac_addr, ETH_ALEN);
    }
    
    // dump the entire table as a reference
    {
        int i;
        
        for (i = 0; i < sizeof(g_net_eps_id_to_mac_addr_table) / sizeof(net_drv_mac_addr_entry_t); i++)
        {
            AMP_LOG_Info(
                "EPS ID to MAC Table[%d]: mac_addr:%02x:%02x:%02x:%02x:%02x:%02x; packet_cnt:%lu\n",
                i, 
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[0],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[1],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[2],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[3],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[4],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[5],
                g_net_eps_id_to_mac_addr_table[i].packet_cnt);
        }
    }
    
    return AMP_ERROR_NONE;

}
EXPORT_SYMBOL(net_drv_set_eps_mac_map);


/**
 * Used to enable or disable bridge mode.  If enabled, allows EPS ID to MAC address
 *  mapping for DL packets.
 *
 * @param unsigned long enable - 0 to disable or != 0 to enable
 *
 * @return long, AMP error code
 */ 
long net_drv_set_bridge_mode(unsigned long enable)
{
    int i;
    
    if (enable)
        g_net_drv_global_flags |= NETDRV_GLOBAL_FLAG_BRIDGE_MODE;
    else
        g_net_drv_global_flags &= ~NETDRV_GLOBAL_FLAG_BRIDGE_MODE;


    // if enabled, then dump the entire table as a reference
    if (enable)
    {    
        for (i = 0; i < sizeof(g_net_eps_id_to_mac_addr_table) / sizeof(net_drv_mac_addr_entry_t); i++)
        {
            AMP_LOG_Info(
                "EPS ID to MAC Table[%d]: mac_addr:%02x:%02x:%02x:%02x:%02x:%02x; packet_cnt:%lu\n",
                i, 
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[0],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[1],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[2],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[3],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[4],
                (int) g_net_eps_id_to_mac_addr_table[i].mac_addr[5],
                g_net_eps_id_to_mac_addr_table[i].packet_cnt);
        }
    }

    return AMP_ERROR_NONE;
}
EXPORT_SYMBOL(net_drv_set_bridge_mode);



/**
 * Used to query the bridge mode.
 *
 * @param unsigned long* penabled - 0 is disabled or 1 if enabled
 *
 * @return long, AMP error code
 */ 
long net_drv_get_bridge_mode(unsigned long* penabled)
{
    
    if (!penabled)
    {
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }

    *penabled = (g_net_drv_global_flags & NETDRV_GLOBAL_FLAG_BRIDGE_MODE) ? 1 : 0;

    return AMP_ERROR_NONE;
}
EXPORT_SYMBOL(net_drv_get_bridge_mode);


/**
 * Used to enable or disable uplink disable.  If enabled, all uplink traffic is dropped.
 * 
 * @param unsigned long enable - 0 to disable or != 0 to enable
 *
 * @return long, AMP error code
 */ 
long net_drv_set_disable_ul(unsigned long enable)
{
    if (enable)
    {
        g_net_drv_global_flags |= NETDRV_GLOBAL_FLAG_DISABLE_UL;
    }
    else
    {
        g_net_drv_global_flags &= ~NETDRV_GLOBAL_FLAG_DISABLE_UL;
    }
    AMP_LOG_Info(
        "Disable UL flag set to %s \n", 
        (g_net_drv_global_flags & NETDRV_GLOBAL_FLAG_DISABLE_UL)? "ENABLED":"DISABLED");

    return AMP_ERROR_NONE;
}
EXPORT_SYMBOL(net_drv_set_disable_ul);


/**
 * Used to query the disable UL status.
 *
 * @param unsigned long* penabled - 0 is disabled or 1 if enabled
 *
 * @return long, AMP error code
 */ 

long net_drv_get_disable_ul(unsigned long* penabled)
{
    if (!penabled)
    {
        return AMP_ERROR_NET_DRV_INVALID_PARAM;
    }

    *penabled = (g_net_drv_global_flags & NETDRV_GLOBAL_FLAG_DISABLE_UL) ? 1 : 0;

    return AMP_ERROR_NONE;
}
EXPORT_SYMBOL(net_drv_get_disable_ul);




/**
 * Set the log level for the network driver
 *
 * @param short log_level - new log level to use
 *
 * @return long, AMP error code
 */ 
long net_drv_set_log_level(short log_level)
{
    char* func_name = "net_drv_set_log_level";


    AMP_LOG_Info(
        "%s - Using new log level:%d\n",
        func_name, (int) log_level);
    
    AMP_LOG_SetLogLevel(AMP_LOG_ADDR_LTE_NET_DRIVER, log_level);
    
    return AMP_ERROR_NONE;
    
}
EXPORT_SYMBOL(net_drv_set_log_level);


/**
 * Set the QOS mode for the network driver
 *
 * @param NET_DRV_QOS_MODE net_drv_qos_mode - new qos mode
 *
 * @return long, AMP error code
 */ 
long net_drv_set_qos_mode(NET_DRV_QOS_MODE net_drv_qos_mode)
{
    char* func_name = "net_drv_set_qos_mode";
    char* str_mode = NULL;


    // coerce value
    COERCE_VAL(net_drv_qos_mode, NET_DRV_QOS_MODE_QOS_MODULE, NET_DRV_QOS_MODE_IPTABLES);
    
    str_mode = net_drv_get_qos_mode_string(net_drv_qos_mode);
    AMP_LOG_Info(
        "%s - Using new QOS mode:%d - %s\n", func_name, (int) net_drv_qos_mode, str_mode);
    
    // set the active handlers based on the global QOS mode
    net_drv_set_active_qos_handers(net_drv_qos_mode);
    
    g_net_drv_qos_mode = net_drv_qos_mode;
    
    return AMP_ERROR_NONE;
    
}
EXPORT_SYMBOL(net_drv_set_qos_mode);


/**
 * Get the current QOS mode for the network driver
 *
 * @param void
 *
 * @return NET_DRV_QOS_MODE
 */ 
NET_DRV_QOS_MODE net_drv_get_qos_mode(void)
{
    
    return g_net_drv_qos_mode;
    
}
EXPORT_SYMBOL(net_drv_get_qos_mode);


/**
 * Get the string representation for the specified QOS mode.
 *
 * @param NET_DRV_QOS_MODE net_drv_qos_mode - qos mode
 *
 * @return char* - string representaion -  WARNING!!!!  Do not delete/free this string
 */ 
char* net_drv_get_qos_mode_string(NET_DRV_QOS_MODE net_drv_qos_mode)
{
    static char* str_qos_module = "QOS MODULE";
    static char* str_ip_tables = "IP TABLES";
    static char* str_unknown = "UNKNOWN";
    
    
    switch (net_drv_qos_mode)
    {
        case NET_DRV_QOS_MODE_QOS_MODULE:
            return str_qos_module;
            
        case NET_DRV_QOS_MODE_IPTABLES:
            return str_ip_tables;
            
        default:
            return str_unknown;
    
    }
    
}
EXPORT_SYMBOL(net_drv_get_qos_mode_string);



/**
 * The performance test thread
 *
 * @param net_drv_perf_thread_data_t* ptr - private thread data
 *
 * @return void
 */ 
static void net_drv_perf_thread(net_drv_perf_thread_data_t* ptr)
{
    char* func_name = "net_drv_perf_thread";
    net_drv_perf_thread_data_t* pptd = (net_drv_perf_thread_data_t*) ptr;
    unsigned long delta_time = 0;
    unsigned long wait_start_time = 0;
    unsigned long test_start_time = 0;
    unsigned long test_total_time = 0;
    unsigned long test_hog_cpu_start_time = 0;
    union ktime test_start_time_kt;
    union ktime test_end_time_kt;
    union ktime test_total_time_kt;
    NET_DRV_PRIVATE* ppriv = NULL;
    char* if_name = "lte0";
    struct sk_buff* pskb = NULL;
    int net_val = 0;
    unsigned long skb_packet_size = 0;
    unsigned long rx_B_s = 0;
    unsigned long rx_Mbit_s = 0;
    static unsigned char fake_eth_ip4_data[] = 
    {
        0x01,0x00,0x5e,0x00,0x00,0x02,0x00,0x0c,0xdb,0xf9,0x09,0x00,0x08,0x00,0x45,0xc0,
        0x00,0x30,0xe2,0x29,0x40,0x00,0xff,0x11,0x1e,0x7e,0x0a,0x55,0x8f,0xfd,0xe0,0x00,
        0x00,0x02,0x22,0xb8,0x22,0xb8,0x00,0x1c,0x3f,0xf1
    };
    
    
    if (!pptd)
    {
        AMP_LOG_Error(
            "%s - ERROR: null pointer passed for thread private data. Test aborted\n", func_name);
        return;
    }
    
    ppriv = (NET_DRV_PRIVATE*) net_drv_device_list_find_name(if_name, NULL);
    if (!ppriv)
    {
        AMP_LOG_Error(
            "%s - ERROR: interface %s does not exist. Test aborted\n", func_name, if_name);
        pptd->busy = 0;
        return;
    }
    
    // stop the queue - this is a real stop
    net_drv_stop_queue(ppriv);
    
    // set the mode to perf test
    ppriv->driver_status |= NETDRV_STATUS_PERF_TEST;
    
    // start the queue - this is a fake start
    net_drv_wake_queue(ppriv);
    
    skb_packet_size = pptd->packet_size + ETH_HLEN;
    
    test_start_time_kt = ktime_get();
    test_start_time = jiffies;
    test_hog_cpu_start_time = jiffies;
    while(1)
    {
        // allocate an skb
        if ((pskb = alloc_skb((int) skb_packet_size, GFP_ATOMIC)) == NULL)
        {
            AMP_LOG_Info(
                "%s - iface:%s alloc_skb failed, packet dropped\n",
                func_name, ppriv->dev_name);
        
            break;
        }
    
        // now fill in the packet with a fake packet data
        // WARNING - the minimum packet size needs to be 42 bytes, see fake packet data above
        pskb->dev = ppriv->pnet_dev;
        skb_put(pskb, (int) skb_packet_size);
        memcpy(pskb->data, fake_eth_ip4_data, sizeof(fake_eth_ip4_data));
        skb_set_mac_header(pskb, 0);
        skb_set_network_header(pskb, 14);
        __net_timestamp(pskb);

        
wait_throttle:
        // if throttled, then wait
        while(ppriv->driver_status & NETDRV_STATUS_QUEUE_STOPPED)
        {
            udelay(10);
        }
        
        // call driver TX entry point
        net_val = net_drv_dev_tx(pskb, ppriv->pnet_dev);

        // if full, then wait for throttle to clear        
        if (net_val == NETDEV_TX_BUSY)
            goto wait_throttle;

        pptd->tx_count += 1;
        pptd->tx_bytes += pptd->packet_size;
        
        // delay between packets if specified
        if (pptd->packet_delay)
            msleep(pptd->packet_delay);
        else
        {
#if defined (NETDRV_PERF_TEST_HOG_LIMITER)
            // if a delay is not specified, then every so often we need to sleep
            // or the kernel will start reporting "CPU:0 stuck for N seconds....."
            // for now, every 15 seconds, sleep for 1 ms
            delta_time = delta_time_jiffies(test_hog_cpu_start_time);
            if (delta_time >= (15 * HZ))
            {
                msleep(1);
                test_hog_cpu_start_time = jiffies;
            }
#endif
        }
        
        // check to see if we have run for duration
        delta_time = delta_time_jiffies(test_start_time);
        if (delta_time >= (pptd->duration * HZ))
        {   
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                "%s - test complete, ran for %ld seconds\n", func_name, (delta_time / HZ));
                
            break;
        }
    }
    
    // wait for all of the packets to loop back from either the loop-back BP
    // or the HI AP loop back
    wait_start_time = jiffies;
    while(1)
    {
        // were done
        if (pptd->rx_count == pptd->tx_count)
        {
            test_end_time_kt = ktime_get();
            test_total_time_kt = ktime_sub(test_end_time_kt, test_start_time_kt);
            test_total_time = delta_time_jiffies(test_start_time);
            
            break;
        }
                
        // check for a timeout on the loopback
        delta_time = delta_time_jiffies(wait_start_time);
        if (delta_time >= (10 * HZ))
        {
            test_end_time_kt = ktime_get();
            test_total_time_kt = ktime_sub(test_end_time_kt, test_start_time_kt);
            test_total_time = delta_time_jiffies(test_start_time);
            
            AMP_LOG_Error(
                "%s - ERROR: timed-out waiting for all packets to loop back\n", func_name);
            
            break;
        }
    }
    
#if 0
    http://en.wikipedia.org/wiki/Mbps#Megabit_per_second
    
    Megabit per second - A megabit per second (Mbit/s or Mb/s or Mbps) is a unit of data transfer rate equal to:

    * 1,000,000 bits per second or
    * 1,000 kilobits per second or
    * 125,000 bytes per second.

    Kibibit per second - A kibibit per second (Kibit/s or Kib/s) is a unit of data transfer rate
    equal to 1,024 bits per second. The word "kibibit" is not capitalized, but the abbreviation "Kibit" is.
    
    Mebibit per second - A mebibit per second (Mibit/s or Mib/s) is a unit of data transfer rate equal to:

        * 1,048,576 bits per second or
        * 1,024 kibibits per second.

    bytes_per_sec = total_bytes / duration;             // B/s
    megabits_per_sec = bytes_per_second / 125000;       // Mbit/s
    

#endif

    // rx bytes per second
    rx_B_s = pptd->rx_bytes / (test_total_time / HZ); /*/ pptd->duration;*/
    // rx mega bits per second
    rx_Mbit_s = rx_B_s / 125000;
    // now print the results
    AMP_LOG_Info(
        "%s - RESULTS: test_total_time(sec):%ld; test_total_time_kt(sec.nsec):%d.%09d; packet_size:%ld; tx_count:%ld; rx_count:%ld; tx_bytes:%ld; rx_bytes:%ld; rx-B/s:%ld; rx-Mbit/s:%ld\n",
        func_name,
        test_total_time / HZ, test_total_time_kt.tv.sec, test_total_time_kt.tv.nsec, 
        pptd->packet_size, pptd->tx_count, pptd->rx_count, pptd->tx_bytes, pptd->rx_bytes,
        rx_B_s, rx_Mbit_s);

    
    // stop the queue - this is a fake stop
    net_drv_stop_queue(ppriv);
    
    // clear the perf test mode
    ppriv->driver_status &= ~NETDRV_STATUS_PERF_TEST;
    
    // start the queue - this is a real start
    net_drv_wake_queue(ppriv);
    
    pptd->busy = 0;

}


/**
 * Runs the internal performance test
 *
 * @param unsigned long duration - duration to run in seconds
 * @param unsigned long packet_size - packet size in bytes
 *
 * @return long, AMP error code
 */ 
long net_drv_perf_test(unsigned long duration, unsigned long packet_size, unsigned long packet_delay)
{
    char* func_name = "net_drv_perf_test";


    if (g_net_ptd.busy)
    {
        AMP_LOG_Error(
            "%s - ERROR: performance test already in progress\n",
            func_name);
    }
    else
    {
        // reset the perf structure
        memset(&g_net_ptd, 0x00, sizeof(net_drv_perf_thread_data_t));
    
        // thread will clear this flag when done
        g_net_ptd.busy = 1;
        g_net_ptd.duration = duration;
        g_net_ptd.packet_size = packet_size;
        g_net_ptd.packet_delay = packet_delay;
        
        AMP_LOG_Info(
            "%s - Starting internal performance test - duration:%ld; packet_size:%ld; packet_delay:%ld\n",
            func_name, duration, packet_size, packet_delay);

        // start the performance thread
        kernel_thread((int (*)(void *))net_drv_perf_thread, (void *)&g_net_ptd, 0);
    }


    return AMP_ERROR_NONE;
    
}
EXPORT_SYMBOL(net_drv_perf_test);


/**
 * Stores EPS id in the skb 
 *
 * @param struct sk_buff* pskb - skb to receive the index
 * @param unsigned char - EPS ID value to store
 *
 * @return void
 */ 
void net_drv_set_skb_eps_id(struct sk_buff* pskb, unsigned char eps_id)
{
    *((unsigned char*) ((char*) pskb->cb + SKB_USER_BYTE_OFFSET_EPS_ID)) = eps_id;
}
EXPORT_SYMBOL(net_drv_set_skb_eps_id);


/**
 * Returns the EPS id value that was stored by the net_drv_set_skb_eps_id() function
 *
 * @param struct sk_buff* pskb - skb with index value
 *
 * @return unsigned long - EPS ID value
 */
unsigned char net_drv_get_skb_eps_id(struct sk_buff* pskb)
{
    return *((unsigned char*) ((char*) pskb->cb + SKB_USER_BYTE_OFFSET_EPS_ID));
}
EXPORT_SYMBOL(net_drv_get_skb_eps_id);


/**
 * LTE net driver module cleanup routine. This function is invoked in
 *  response to rmmod
 *
 * @param void
 *
 * @return void
 */
void net_drv_cleanup_module(void)
{
    char* func_name = "net_drv_cleanup_module";


    AMP_LOG_Info(
        "%s - complete\n", func_name);

}


/**
 * LTE net driver module initialization routine.  This function is
 *  invoked in response to insmod
 *
 * @param void
 *
 * @return int
 */
static int __init net_drv_init_module(void)
{
    char* func_name = "net_drv_init_module";
    int i;
    int ret = 0;


    // initialize the default handlers to the built-in handlers
    g_net_drv_qos_interface_default.process_tx_packet_qos = net_drv_process_tx_packet_qos;
    g_net_drv_qos_interface_default.process_rx_packet_qos = net_drv_process_rx_packet_qos;
    
    // set the registered to be the same as default on startup
    g_net_drv_qos_interface_registered = g_net_drv_qos_interface_default;
    
    // set the active handlers based on the global qos mode
    net_drv_set_active_qos_handers(g_net_drv_qos_mode);
    
    // initialize the EPS ID to MAC ADDR lookup table with the default MAC address
    for (i = 0; i < sizeof(g_net_eps_id_to_mac_addr_table) / sizeof(net_drv_mac_addr_entry_t); i++)
    {
        memcpy(g_net_eps_id_to_mac_addr_table[i].mac_addr, default_dev_mac_addr, ETH_ALEN);
        g_net_eps_id_to_mac_addr_table[i].packet_cnt = 0;
    }
    
    AMP_LOG_Info(
        "%s - complete\n", func_name);


    return ret;
}


module_init(net_drv_init_module);
module_exit(net_drv_cleanup_module);

MODULE_DESCRIPTION("Motorola LTE Network Driver");
MODULE_LICENSE("GPL");

