/*
 * Copyright Motorola, Inc. 2009
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
 *  @File:  net_driver.h
 *
 * Exported network driver "public" functionality to other kernel modules
 *
 * Last Changed Revision: $Rev$
 * Last Changed Date: $LastChangedDate$
 */
#ifndef _NET_DRV_H_
#define _NET_DRV_H_


#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

#define NETDRV_MTU                              8188

#define NETDRV_MAX_DEV_NAME                     128
#define NETDRV_MAX_NUM_DEVICES                  8

#define NETDRV_NET_CONTROL_NONE                 0x00000000
#define NETDRV_NET_CONTROL_SUSPEND              0x00000001
#define NETDRV_NET_CONTROL_RESUME               0x00000002
#define NETDRV_NET_CONTROL_RESET                0x00000003
#define NETDRV_NET_CONTROL_FLUSH                0x00000004

#define NETDRV_BUS_CONTROL_NONE                 0x00000000
#define NETDRV_BUS_CONTROL_SUSPEND              0x00000001
#define NETDRV_BUS_CONTROL_RESUME               0x00000002
#define NETDRV_BUS_CONTROL_RESET                0x00000003
#define NETDRV_BUS_CONTROL_FLUSH                0x00000004
#define NETDRV_BUS_CONTROL_TX_DRAIN             0x00000005
#define NETDRV_BUS_CONTROL_LOG_NETQUEUE_EVENT   0x00000006

#define NETDRV_DEFAULT_EPS_ID                   5
#define NETDRV_MIN_EPS_ID                       0
#define NETDRV_MAX_EPS_ID                       15
#define NETDRV_MAX_NUM_EPS_ID                   16


/**
 * @typedef NET_DRV_CLASSIFIER_MODE
 * @brief Network driver classifier mode
 *
 * Used to denote the classifier mode, legacy or iptables
 */
typedef enum _NET_DRV_QOS_MODE
{

    NET_DRV_QOS_MODE_QOS_MODULE,
    NET_DRV_QOS_MODE_IPTABLES
   
} NET_DRV_QOS_MODE;


// Initialized by BUS object. NET object calls this function to enqueue data
typedef long (*fp_tx_start)(void* pnet, void* pbus, struct sk_buff* pskb);
// Initialized by NET object. BUS object calls this function to signal TX complete
typedef long (*fp_tx_complete)(void* pnet, void* pbus, struct sk_buff* pskb, long fifo_size, long fifo_count);

// Initialized by NET object. BUS object calls this function to allocate data
typedef struct sk_buff* (*fp_rx_start)(void* pnet, void* pbus, int buf_size, gfp_t mem_flags);
// Initialized by NET object. BUS object calls this function to deliver RX payload
typedef long (*fp_rx_complete)(void* pnet, void* pbus, struct sk_buff* pskb, unsigned char eps_id, int mm_packet);

// Initialized by NET object. BUS object calls this function to pause/resume/reset/flush/etc
typedef long (*fp_net_control)(void* pnet, void* pbus, unsigned long cmd, unsigned long cmd_data);
// Initialized by BUS object. NET object calls this function to pause/resume/reset/flush/etc
typedef long (*fp_bus_control)(void* pnet, void* pbus, unsigned long cmd, unsigned long cmd_data);


/**
 * @struct NET_DRV_BUS_INTERFACE
 * @brief Network driver BUS interface functions
 *
 * This structure contains the interface functions between the network
 * object and a bus object (USB, SDIO, SDMA, etc)
 */
typedef struct _NET_DRV_BUS_INTERFACE
{
    
    fp_tx_start     tx_start;
    fp_tx_complete  tx_complete;
    fp_rx_start     rx_start;
    fp_rx_complete  rx_complete;
    fp_net_control  net_control;
    fp_bus_control  bus_control;
   
} NET_DRV_BUS_INTERFACE;


// QOS related callback functions
typedef long (*fp_process_tx_packet_qos)(struct sk_buff *pskb, unsigned short* poffset, unsigned char* peps_id);
typedef long (*fp_process_rx_packet_qos)(struct sk_buff *pskb, unsigned char* pheader, unsigned short* plength);


/**
 * @struct NET_DRV_QOS_INTERFACE
 * @brief Network driver QOS interface functions
 *
 * This structure contains the interface functions between the network
 * object and a QOS object
 */
typedef struct _NET_DRV_QOS_INTERFACE
{

    fp_process_tx_packet_qos    process_tx_packet_qos;
    fp_process_rx_packet_qos    process_rx_packet_qos;
    
} NET_DRV_QOS_INTERFACE;


// exported functions

// device "list" functions
int net_drv_device_list_first_avail(void);
void* net_drv_device_list_find_name(char* pdev_name, void* pbus);

// device create, destroy, and modify
long net_drv_probe(char* pdev_name, void* pbus, NET_DRV_BUS_INTERFACE* pnet_drv_bus_interface, void** ppnet);
long net_drv_disconnect(void* pnet);

// QOS registration
long net_drv_qos_register(NET_DRV_QOS_INTERFACE net_drv_qos_interface);

// utility
long net_drv_set_eps_id(void* pnet, unsigned char eps_id);
long net_drv_set_eps_mac_map(unsigned char eps_id, unsigned char* mac_addr);
long net_drv_set_bridge_mode(unsigned long enable);
long net_drv_get_bridge_mode(unsigned long* penabled);
long net_drv_set_log_level(short log_level);
long net_drv_set_qos_mode(NET_DRV_QOS_MODE net_drv_qos_mode);
long net_drv_set_disable_ul(unsigned long enable);
long net_drv_get_disable_ul(unsigned long *penabled);
NET_DRV_QOS_MODE net_drv_get_qos_mode(void);
char* net_drv_get_qos_mode_string(NET_DRV_QOS_MODE net_drv_qos_mode);
long net_drv_perf_test(unsigned long duration, unsigned long packet_size, unsigned long packet_delay);
void net_drv_set_skb_eps_id(struct sk_buff* pskb, unsigned char eps_id);
unsigned char net_drv_get_skb_eps_id(struct sk_buff* pskb);


#if 0
// add get/set attribute functions to replace the above "utility" functions
#define NETDRV_ATTR_EPS_ID                          0x00000001
#define NETDRV_ATTR_LOG_LEVEL                       0x00000002
#define NETDRV_ATTR_QOS_MODE                        0x00000003

long net_drv_set_attr(void* pnet, unsigned long attr, unsigned long val);
long net_drv_get_attr(void* pnet, unsigned long attr, unsigned long* pval);
#endif


#endif // _NET_DRV_H_

