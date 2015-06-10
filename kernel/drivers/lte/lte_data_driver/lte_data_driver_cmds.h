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
 * @File: lte_data_driver_cmds.h
 * @brief LTE data driver commands
 *
 * Last Changed Revision: $Rev$
 * Last Changed Date: $LastChangedDate$
 */
 
#ifndef _LTE_DATA_DRIVER_CMDS_H_
#define _LTE_DATA_DRIVER_CMDS_H_

#include "../net_driver/net_driver.h"
#include "lte_data_driver.h"


#define LTE_DD_MIN_EPS              0
#define LTE_DD_MAX_EPS              15


#define COERCE_VAL(val,min,max)\
    if (val < min)\
        val = min;\
    else if (val > max)\
        val = max;

#define CMD_STR_NET_NEW                 "net_new"
#define CMD_STR_NET_DELETE              "net_delete"
#define CMD_STR_NET_EPS                 "net_eps"
#define CMD_STR_NET_EPS_MAC_MAP_ADD_DL     "net_eps_mac_map_add_dl"
#define CMD_STR_NET_EPS_MAC_MAP_RM_DL      "net_eps_mac_map_rm_dl"
#define CMD_STR_NET_BRIDGE_MODE_DL         "net_bridge_mode_dl"
#define CMD_STR_NET_EPS_MAC_MAP_ADD_UL     "net_eps_mac_map_add_ul"
#define CMD_STR_NET_EPS_MAC_MAP_RM_UL      "net_eps_mac_map_rm_ul"
#define CMD_STR_NET_BRIDGE_MODE_UL         "net_bridge_mode_ul"
#define CMD_STR_NET_PRIORITY            "net_priority"
#define CMD_STR_NET_LOG                 "net_log"
#define CMD_STR_NET_PERF_TEST           "net_perf_test"
#define CMD_STR_DD_LOG                  "dd_log"
#define CMD_STR_DD_UL_PACKET_PROFILE    "dd_ul_packet_profile"
#define CMD_STR_DD_DL_PACKET_PROFILE    "dd_dl_packet_profile"
#define CMD_STR_DD_MULTIPACKET          "dd_multipacket"
#define CMD_STR_DD_LPBP_MODE            "dd_lpbp_mode"
#define CMD_STR_DD_DISABLE_UL           "dd_disable_ul"
#define CMD_STR_QOS_LOG                 "qos_log"
#define CMD_STR_QOS_MODE                "qos_mode"
#define CMD_STR_HIF_CMD                 "hif_cmd"

#define MAX_DD_CMD_ARRAY                64

typedef enum
{

    lte_dd_cmd_net_new = 0,
    lte_dd_cmd_net_delete,
    lte_dd_cmd_net_eps,
    lte_dd_cmd_net_eps_mac_map_add_dl,
    lte_dd_cmd_net_eps_mac_map_rm_dl,
    lte_dd_cmd_net_bridge_mode_dl,
    lte_dd_cmd_net_eps_mac_map_add_ul,
    lte_dd_cmd_net_eps_mac_map_rm_ul,
    lte_dd_cmd_net_bridge_mode_ul,
    lte_dd_cmd_net_priority,
    lte_dd_cmd_net_log,
    lte_dd_cmd_net_perf_test,
    lte_dd_cmd_dd_log,
    lte_dd_cmd_dd_ul_packet_profile,
    lte_dd_cmd_dd_dl_packet_profile,
    lte_dd_cmd_dd_multipacket,
    lte_dd_cmd_dd_lpbp_mode,
    lte_dd_cmd_dd_disable_ul,
    lte_dd_cmd_qos_log,
    lte_dd_cmd_qos_mode,
    lte_dd_cmd_hif_cmd

} lte_dd_cmd_id_t;


/**
 * @struct lte_dd_cmd_net_new_t
 * @brief data driver command - net_new
 *
 * Used in the creation of a network object
 */
typedef struct
{

    char ifname[NETDRV_MAX_DEV_NAME];               // the net object name
    unsigned char eps_id;                           // EPS id
    unsigned char priority;                         // the interface UL priority
    
} lte_dd_cmd_net_new_t;


/**
 * @struct lte_dd_cmd_net_delete_t
 * @brief data driver command - net_delete
 *
 * Used in the destruction of a network object
 */
typedef struct
{

    char ifname[NETDRV_MAX_DEV_NAME];               // the net object name
    
} lte_dd_cmd_net_delete_t;


/**
 * @struct lte_dd_cmd_net_eps_t
 * @brief data driver command - net_eps
 *
 * Used in setting the epsid of a network object
 */
typedef struct
{

    char ifname[NETDRV_MAX_DEV_NAME];               // the net object name
    unsigned char eps_id;                           // EPS id
    
} lte_dd_cmd_net_eps_t;


/**
 * @struct llte_dd_cmd_net_eps_mac_map_add_dl_t
 * @brief data driver command - net_eps_mac_map_add_dl
 *
 * Used to add an eps id to MAC address mapping for DL packets
 */
typedef struct
{

    unsigned char eps_id;                           // EPS id
    unsigned char mac_addr[ETH_ALEN];               // MAC address

} lte_dd_cmd_net_eps_mac_map_add_dl_t;

/**
 * @struct llte_dd_cmd_net_eps_mac_map_add_ul_t
 * @brief data driver command - net_eps_mac_map_add_ul
 *
 * Used to add an eps id to MAC address mapping for UL packets
 */
typedef struct
{

    unsigned char eps_id;                           // EPS id
    unsigned char mac_addr[ETH_ALEN];               // MAC address

} lte_dd_cmd_net_eps_mac_map_add_ul_t;

/**
 * @struct llte_dd_cmd_net_eps_mac_map_rm_dl_t
 * @brief data driver command - net_eps_mac_map_rm_dl
 *
 * Used to remove an eps id to MAC address mapping for DL packets
 */
typedef struct
{

    unsigned char eps_id;                           // EPS id

} lte_dd_cmd_net_eps_mac_map_rm_dl_t;

/**
 * @struct llte_dd_cmd_net_eps_mac_map_rm_ul_t
 * @brief data driver command - net_eps_mac_map_rm_ul
 *
 * Used to remove an eps id to MAC address mapping for UL packets
 */
typedef struct
{

    unsigned char eps_id;                           // EPS id

} lte_dd_cmd_net_eps_mac_map_rm_ul_t;


/**
 * @struct lte_dd_cmd_net_bridge_mode_dl_t
 * @brief data driver command - net_bridge_mode_dl
 *
 * Used to enable or disable the bridge mode in the network driver
 */
typedef struct
{

    long enable;                                    // enable
    
} lte_dd_cmd_net_bridge_mode_dl_t;

/**
 * @struct lte_dd_cmd_net_bridge_mode_ul_t
 * @brief data driver command - net_bridge_mode_ul
 *
 * Used to enable or disable the bridge mode in the lte classifier
 */
typedef struct
{

    long enable;                                    // enable
    
} lte_dd_cmd_net_bridge_mode_ul_t;


/**
 * @struct lte_dd_cmd_net_priority_t
 * @brief data driver command - net_priority
 *
 * Used in setting the UL priority of a network object
 */
typedef struct
{

    char ifname[NETDRV_MAX_DEV_NAME];               // the net object name
    unsigned char priority;                         // the interface UL priority
    
} lte_dd_cmd_net_priority_t;


/**
 * @struct lte_dd_cmd_net_log_t
 * @brief data driver command - net_log
 *
 * Used in the setting the log level of a network object
 */
typedef struct
{

    long log_level;                                 // log level 
    
} lte_dd_cmd_net_log_t;


/**
 * @struct lte_dd_cmd_net_perf_test_t
 * @brief data driver command - net_perf_test
 *
 * Used to start the performace test built into the network driver
 */
typedef struct
{

    long duration;                                 // test duration
    long packet_size;                              // skbuf size
    long packet_delay;                             // delay between packet generation
    
} lte_dd_cmd_net_perf_test_t;


/**
 * @struct lte_dd_cmd_dd_log_t
 * @brief data driver command - dd_log
 *
 * Used in the setting the log level of the data driver
 */
typedef struct
{

    long log_level;                                 // log level 
    
} lte_dd_cmd_dd_log_t;


/**
 * @struct lte_dd_cmd_dd_ul_packet_profile_t
 * @brief data driver command - dd_ul_packet_profile
 *
 * Used to enable UL packet profiling in the data driver
 */
typedef struct
{

    long enable;                                    // enable
    long reset;                                     // reset the log prior to enable 
    
} lte_dd_cmd_dd_ul_packet_profile_t;


/**
 * @struct lte_dd_cmd_dd_dl_packet_profile_t
 * @brief data driver command - dd_dl_packet_profile
 *
 * Used to enable DL packet profiling in the data driver
 */
typedef struct
{

    long enable;                                    // enable
    long reset;                                     // reset the log prior to enable 
    
} lte_dd_cmd_dd_dl_packet_profile_t;


/**
 * @struct lte_dd_cmd_dd_multipacket_t
 * @brief data driver command - dd_multipacket
 *
 * Used to enable or disable the multi packet mode in the data driver
 */
typedef struct
{

    long enable;                                    // enable
    
} lte_dd_cmd_dd_multipacket_t;


/**
 * @struct lte_dd_cmd_dd_lpbp_mode_t
 * @brief data driver command - dd_lpbp_mode
 *
 * Used to enable or disable the loopback BP mode in the data driver.  This allows
 * ICMP ping requests to be converted to ping replies in the DL path prior to
 * being submitted to kenel via netif_rx()
 */
typedef struct
{

    long enable;                                    // enable
    
} lte_dd_cmd_dd_lpbp_mode_t;

/**
 * @struct lte_dd_cmd_dd_disable_ul_t
 * @brief data driver command - dd_disable_ul
 *
 * Used to indicate that the data driver should drop all uplink traffic
 */
typedef struct
{

    long enable;                                    // enable
    
} lte_dd_cmd_dd_disable_ul_t;

/**
 * @struct lte_dd_cmd_qos_log_t
 * @brief data driver command - qos_log
 *
 * Used in the setting the log level of the QOS classifier
 */
typedef struct
{

    long log_level;                                 // log level 
    
} lte_dd_cmd_qos_log_t;


/**
 * @struct lte_dd_cmd_qos_mode_t
 * @brief data driver command - qos_mode
 *
 * Used in the setting the log level of the QOS classifier
 */
typedef struct
{

    long mode;                                      // mode
    
} lte_dd_cmd_qos_mode_t;


/**
 * @struct lte_dd_cmd_hif_cmd_t
 * @brief data driver command - hif_cmd
 *
 * Used to issue a command to the 4GHIF API
 */
typedef struct
{

    long cmd;                                       // command
    
} lte_dd_cmd_hif_cmd_t;


/**
 * @union lte_dd_cmd_data_t
 * @brief union of data driver command data
 *
 * A union that contains all possible data driver command structures
 */
typedef union
{

    lte_dd_cmd_net_new_t                net_new;
    lte_dd_cmd_net_delete_t             net_delete;
    lte_dd_cmd_net_eps_t                net_eps;
    lte_dd_cmd_net_eps_mac_map_add_dl_t    net_eps_mac_map_add_dl;
    lte_dd_cmd_net_eps_mac_map_rm_dl_t     net_eps_mac_map_rm_dl;
    lte_dd_cmd_net_bridge_mode_dl_t        net_bridge_mode_dl;
    lte_dd_cmd_net_eps_mac_map_add_ul_t    net_eps_mac_map_add_ul;
    lte_dd_cmd_net_eps_mac_map_rm_ul_t     net_eps_mac_map_rm_ul;
    lte_dd_cmd_net_bridge_mode_ul_t        net_bridge_mode_ul;
    lte_dd_cmd_net_priority_t           net_priority;
    lte_dd_cmd_net_log_t                net_log;
    lte_dd_cmd_net_perf_test_t          net_perf_test;
    lte_dd_cmd_dd_log_t                 dd_log;
    lte_dd_cmd_dd_ul_packet_profile_t   dd_ul_packet_profile;
    lte_dd_cmd_dd_dl_packet_profile_t   dd_dl_packet_profile;
    lte_dd_cmd_dd_multipacket_t         dd_multipacket;
    lte_dd_cmd_dd_lpbp_mode_t           dd_lpbp_mode;
    lte_dd_cmd_dd_disable_ul_t           dd_disable_ul;
    lte_dd_cmd_qos_log_t                qos_log;
    lte_dd_cmd_qos_mode_t               qos_mode;
    lte_dd_cmd_hif_cmd_t                hif_cmd;

} lte_dd_cmd_data_t;

typedef void (*PARSINGFUNCPTR)(void *dest, char *src, long param1, long param2);
typedef struct
{
    PARSINGFUNCPTR parsingFunc;
    unsigned long paramDestOffset;
    long funcParam1;
    long funcParam2;
} PARSING_INFO;

typedef struct
{
    char *cmdString;
    lte_dd_cmd_id_t commandId;
    unsigned long numCmdParams;
    PARSING_INFO *cmdParams;
} COMMAND_INFO;

/**
 * @struct lte_dd_cmd_t
 * @brief data driver command
 *
 * A single data driver command
 */
typedef struct
{

    lte_dd_cmd_id_t             cmd_id;
    lte_dd_cmd_data_t           cmd_data;

} lte_dd_cmd_t;

// exported functions
extern long lte_dd_parse_dd_cmds(char* cmd_str, lte_dd_cmd_t cmd_array[MAX_DD_CMD_ARRAY], unsigned long* populated);

#endif  // _LTE_DATA_DRIVER_CMDS_H_

