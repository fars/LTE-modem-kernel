/*
 * Copyright Motorola, Inc. 2009 - 2011
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
/**
 *  @File:  net_driver_dev.h
 *
 * Private network driver header file, items NOT exported to other kernel modules
 *
 */
#ifndef _NET_DRV_DEV_H_
#define _NET_DRV_DEV_H_


#include    <linux/version.h>
#include    <linux/smp_lock.h>

#include    "net_driver.h"


#define NETDRV_SIGNATURE                        0xDEADDEED

// net driver status bit
#define NETDRV_STATUS_INITIAL                   (1 << 0)
#define NETDRV_STATUS_LINKDOWN                  (1 << 1)
#define NETDRV_STATUS_REGISTERED                (1 << 2)
#define NETDRV_STATUS_QUEUE_STOPPED             (1 << 3)
#define NETDRV_STATUS_PERF_TEST                 (1 << 4)


// currently, these map directly
#define SPIN_LOCK_IRQ_SAVE(SL_PLOCK, SL_FLAGS)\
{\
    spin_lock_irqsave(SL_PLOCK, SL_FLAGS);\
}

#define SPIN_UNLOCK_IRQ_RESTORE(SL_PLOCK, SL_FLAGS)\
{\
   spin_unlock_irqrestore(SL_PLOCK, SL_FLAGS);\
}


/**
 * @struct NET_DRV_PRIVATE
 * @brief Private structure for the network driver
 *
 */
typedef struct _NET_DRV_PRIVATE
{

    unsigned long signature;                            // our modules signature
    unsigned long size;                                 // this structure's size
    struct net_device* pnet_dev;                        // ptr to linux net device
    struct net_device_stats net_stats;                  // driver statistics
    struct module* powner;                              // module id
    char dev_name[NETDRV_MAX_DEV_NAME];                 // net driver name
    void* pbus;                                         // pointer to bus object (USB, SDIO, etc)
    spinlock_t sl_lock;                                 // for locking our device
    unsigned char eth_hdrV4[ETH_HLEN];                  // IPV4 ether address
    unsigned char eth_hdrV6[ETH_HLEN];                  // IPV6 ether address
    unsigned long driver_status;                        // driver status
    unsigned char eps_id;                               // eps_id to use for TX
    NET_DRV_BUS_INTERFACE* pnet_drv_bus_interface;      // net driver bus interface functions
    
} NET_DRV_PRIVATE;


#endif // _NET_DRV_DEV_H_

