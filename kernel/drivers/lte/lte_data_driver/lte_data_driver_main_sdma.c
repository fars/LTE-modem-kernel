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
 
/**
 * @file lte_data_driver_main.c
 * @brief LTE data driver main file.
 *
 * This file contains the data driver functionality
 *
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <asm/checksum.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/in6.h>
#include <linux/ipv6.h>
#include <linux/icmpv6.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>

#include <mach/dma.h>
#include <asm/dma.h>

#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_lte_log.h>
#include <linux/lte/AMP_lte_error.h>
#include "../common/ptr_fifo.h"
#include "../common/time_calc.h"

#include "../net_driver/net_driver.h"
#include "lte_data_driver.h"
#include "lte_data_driver_cmds.h"
//#include "ipc_4ghif_stub.h"
#include <linux/ipc_4ghif.h>

//#define OMAP_DM_PROFILE
#if defined (OMAP_DM_PROFILE)
#include <mach/dmtimer.h>
#endif

#include "../include/lte_classifier.h"
#include "../common/time_stamp_log.h"


// IKCHUM-2783: ignore unused variables caused by disabling AMP_LOG_Print
#pragma GCC diagnostic ignored "-Wunused-variable"


//******************************************************************************
// defines
//******************************************************************************

#define DEVICE_NAME                             "lte_dd"
#define DEVICE_NAME_DEV                         "lte_dd0"
#define DEVICE_NAME_UL_PROFILE                  "lte_dd_ul_profile"
#define DEVICE_NAME_DL_PROFILE                  "lte_dd_dl_profile"

#define LTE_DD_PERSISTENT_IFACE_NAME            "lte0"

#define LTE_DD_SIGNATURE                        0xBAADBEEF
#define LTE_DD_TX_DATA_PACKET_FIFO_SIZE         64

// flags specific to the data driver
#define LTE_DD_FLAG_NONE                        0x00000000
#define LTE_DD_FLAG_TX_BUSY                     0x00000001
#define LTE_DD_FLAG_TX_THROTTLE                 0x00000002
#define LTE_DD_FLAG_RX_BUSY                     0x00000004
#define LTE_DD_FLAG_RX_THROTTLE                 0x00000008
#define LTE_DD_FLAG_RX_AVAIL                    0x00000010
#define LTE_DD_FLAG_MULTI_PACKET                0x00000020
#define LTE_DD_FLAG_LPBP                        0x00000040

// flags specific to a net info (net object)
#define LTE_NI_FLAG_NONE                        0x00000000
#define LTE_NI_FLAG_TX_BUSY                     0x00000001

#define UL_DL_WATCHDOG_TIMER_INTERVAL           (HZ * 2)
#define TX_BUSY_TIMEOUT                         (HZ * 1)
#define RX_BUSY_TIMEOUT                         (HZ * 1)

#define MAX_SDMA_ERR_MSG                        256

#define UL_TSL_NUM_ENTRIES                      1024
#define DL_TSL_NUM_ENTRIES                      1024

#define SEQ_SHOW_TIMESPEC

// UL TSL events
#define UL_TSL_EVENT_MIN                        1
#define UL_TSL_EVENT_4GHIF_DMA_THROTTLE_CLOSE   1
#define UL_TSL_EVENT_4GHIF_DMA_THROTTLE_OPEN    2
#define UL_TSL_EVENT_NETDRV_THROTTLE_CLOSE      3
#define UL_TSL_EVENT_NETDRV_THROTTLE_OPEN       4
#define UL_TSL_EVENT_LTEDD_TX_DRAIN             5
#define UL_TSL_EVENT_MAX                        5

// UL TSL time stamp indicies
#define UL_TSL_TSNDX_ENQUEUE                    0
#define UL_TSL_TSNDX_SUBMIT                     1
#define UL_TSL_TSNDX_COMPLETE                   2
#define UL_TSL_TSNDX_EVENT                      0

// DL TSL time stamp indicies
#define DL_TSL_TSNDX_SUBMIT                     0
#define DL_TSL_TSNDX_COMPLETE                   1

// offsets into skb "cb" field for personal storage:
// WARNING!!! The network driver has reserved the 1st 4 bytes of the "cb" field
// Refer to function net_drv_set_skb_eps_id() and SKB_USER_BYTE_OFFSET_EPS_ID
#define SKB_USER_BYTE_OFFSET_PROFILE_INDEX      4

// enable to get info on FIFO lookup
//#define LTE_DD_DBG_INFO_FIFO_LOOKUP

// enable to track physical byte alignment of data
#define LTE_DD_TRACK_HIF_PHYS_ADDR_ALIGNMENT


//******************************************************************************
// typedefs
//******************************************************************************


// structure used to manage each net object that is created by the data driver
typedef struct
{

    char ifname[NETDRV_MAX_DEV_NAME];               // net driver name
    unsigned char eps_id;                           // interface eps id
    unsigned char priority;                         // interface priority
    void* pnet;                                     // net driver handle returned from the net driver probe call
    NET_DRV_BUS_INTERFACE net_drv_bus_interface;    // interface between this device and a net device
    PTR_FIFO_HANDLE h_tx_data_fifo;                 // handle to a ptr fifo object for TX data packets
    unsigned long tx_data_packet_write_fifo_cnt;    // total tx data packets written to the UL fifo
    unsigned long tx_data_packet_read_fifo_cnt;     // total tx data packets read from the UL fifo
    unsigned long tx_throttle_close_cnt;            // number of times net object signals it has stopped its queue
    unsigned long tx_throttle_open_cnt;             // number of times net object signals it has started its queue
    unsigned long tx_drain_cnt;                     // number of times net object signals it wants to drain
    unsigned long ni_flags;                         // data driver flags

} lte_dd_net_info_t;


// structure associated with each UL and DL sdma transfer
typedef struct
{

    lte_dd_net_info_t*  pni;                        // the net info associated with an UL or DL xfer
    struct sk_buff*     pskb;                       // the skb associated with an UL or DL xfer
    sdma_packet         sdma;                       // the SDMA (4GHIF API) packet associated with an UL or DL xfer 

} lte_dd_sdma_xfer_t;


// the data driver device object
typedef struct
{

    unsigned long signature;                                        // our modules signature
    unsigned long size;                                             // this structure's size
    void* ptsl_ul;                                                  // time stamp log for UL data
    int ul_iterator;                                                // UL iterator
    unsigned long ul_entries;                                       // UL entries
    void* ptsl_dl;                                                  // time stamp log for DL data
    int dl_iterator;                                                // DL iterator
    unsigned long dl_entries;                                       // DL entries
    lte_dd_net_info_t ni_interfaces[NETDRV_MAX_NUM_DEVICES];        // net object array. 0 is reserved for lte0 which is persistent.
    unsigned long ni_interfaces_cnt;                                // the number of net interfaces created by lte_dd_create_net_iface
    lte_dd_sdma_xfer_t tx_sdma_xfer[IPC_4GHIF_MAX_NB_UL_FRAMES];    // tx sdma xfer variables
    long tx_sdma_xfer_cnt;                                          // number of items in tx_xfer
    sdma_packet* tx_sdma_packet[IPC_4GHIF_MAX_NB_UL_FRAMES];        // array of pointers passed to the 4GHIF API
    long tx_sdma_packet_cnt;                                        // number of valid pointers in tx_sdma_packet
    lte_dd_sdma_xfer_t rx_sdma_xfer[IPC_4GHIF_MAX_NB_DL_FRAMES];    // rx sdma xfer variables
    long rx_sdma_xfer_cnt;                                          // number of items in rx_xfer
    sdma_packet* rx_sdma_packet[IPC_4GHIF_MAX_NB_DL_FRAMES];        // array of pointers passed to the 4GHIF API
    long rx_sdma_packet_cnt;                                        // number of valid pointers in rx_sdma_packet
    union ktime tx_kt_submit;                                       // tx kernel time submit
    union ktime tx_kt_ack;                                          // tx kernel time ack
    union ktime tx_kt_delta;                                        // tx kernel time delta
    union ktime tx_kt_delta_min;                                    // tx kernel time delta min
    union ktime tx_kt_delta_max;                                    // tx kernel time delta max
    union ktime rx_kt_submit;                                       // rx kernel time submit
    union ktime rx_kt_ack;                                          // rx kernel time ack
    union ktime rx_kt_delta;                                        // rx kernel time delta
    union ktime rx_kt_delta_min;                                    // rx kernel time delta min
    union ktime rx_kt_delta_max;                                    // rx kernel time delta max
    union ktime txrx_kt_delta;                                      // txrx kernel time delta
    union ktime txrx_kt_delta_min;                                  // txrx kernel time delta min
    union ktime txrx_kt_delta_max;                                  // txrx kernel time delta max
    unsigned long tx_packet_cnt;                                    // tx number of packets sent
    unsigned long rx_packet_cnt;                                    // rx number of packets received
    unsigned long tx_data_packets_sdma_submit;                      // tx number of sdma submits
    unsigned long tx_data_packets_sdma_ack;                         // tx number of sdma acks
    unsigned long tx_data_packets_sdma_bytes;                       // tx total bytes sent to sdma
    unsigned long rx_data_packets_sdma_notify;                      // rx number of sdma notifies
    unsigned long rx_data_packets_sdma_submit;                      // rx number of sdma submits
    unsigned long rx_data_packets_sdma_ack;                         // rx number of sdma acks
    unsigned long rx_data_packets_sdma_bytes;                       // rx total bytes received from sdma
    unsigned long tx_data_packets_multi_avail;                      // tx number of times multiple packets were available
    unsigned long rx_data_packets_multi_avail;                      // rx number of times multiple packets were available
    unsigned long tx_throttle_close_cnt;                            // number of times submit_ul RESOURCES_FULL signal occurs
    unsigned long tx_throttle_open_cnt;                             // number of times ul_resume callback is invoked
    unsigned long tx_data_busy_start_time;                          // tx data busy start time in jiffies
    unsigned long tx_data_throttle_start_time;                      // tx data throttle start time in jiffies
    unsigned long rx_data_busy_start_time;                          // rx data busy start time in jiffies
    unsigned long rx_data_throttle_start_time;                      // rx data throttle start time in jiffies
    unsigned long rx_net_rx_drop_cnt;                               // rx number of times the net driver drops a RX packet
    unsigned long tx_physaddr_align8_cnt;                           // tx number of 8-bit aligned phys addrs 
    unsigned long tx_physaddr_align16_cnt;                          // tx number of 16-bit aligned phys addrs
    unsigned long tx_physaddr_align32_cnt;                          // tx number of 32-bit aligned phys addrs
    unsigned long rx_physaddr_align8_cnt;                           // rx number of 8-bit aligned phys addrs
    unsigned long rx_physaddr_align16_cnt;                          // rx number of 16-bit aligned phys addrs
    unsigned long rx_physaddr_align32_cnt;                          // rx number of 32-bit aligned phys addrs
    spinlock_t sl_lock;                                             // linux kernel lock
    unsigned long dd_flags;                                         // data driver flags
    char last_sdma_err_msg[MAX_SDMA_ERR_MSG + 1];                   // the last 4GHIF IPC error
    struct timer_list ul_dl_wd_timer;                               // UL and DL watchdog timer
    struct cdev cdev;                                               // char device structure
    
} lte_dd_dev_t;


//******************************************************************************
// data
//******************************************************************************


static int g_dev_major = 0;
static int g_dev_minor = 0;
static lte_dd_dev_t* g_pdev = NULL;
static struct proc_dir_entry* g_proc_entry = NULL;
static struct proc_dir_entry* g_proc_entry_ul_profile = NULL;
static struct proc_dir_entry* g_proc_entry_dl_profile = NULL;

#define DO_DYNAMIC_DEV
#if defined (DO_DYNAMIC_DEV)
struct class* lte_dd_class = NULL;
#endif


//******************************************************************************
// static functions
//******************************************************************************


/**
 * Validates the device pointer as ours
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return long, error code
 */ 
static inline long lte_dd_private_valid(lte_dd_dev_t* pdev)
{
    
    if (!pdev || pdev->signature != LTE_DD_SIGNATURE || pdev->size != sizeof(lte_dd_dev_t))
        return AMP_ERROR_LTEDD_PRIVATE_INVALID;

    return AMP_ERROR_NONE;
}


/**
 * Stores an index value in the skb 
 *
 * @param struct sk_buff* pskb - skb to receive the index
 * @param unsigned long index - index value to store
 *
 * @return void
 */ 
static inline void lte_dd_set_skb_profile_index(struct sk_buff* pskb, unsigned long index)
{
    *((unsigned long*) ((char*) pskb->cb + SKB_USER_BYTE_OFFSET_PROFILE_INDEX)) = index;
}


/**
 * Returns an index value that was stored by the lte_dd_set_skb_profile_index() function
 *
 * @param struct sk_buff* pskb - skb with index value
 *
 * @return unsigned long - index value
 */
static inline unsigned long lte_dd_get_skb_profile_index(struct sk_buff* pskb)
{
    return *((unsigned long*) ((char*) pskb->cb + SKB_USER_BYTE_OFFSET_PROFILE_INDEX));
}


// ***************************************************************************
// UL profile sequence functions - start
// ***************************************************************************

/**
 * sequence function for UL profile data
 *
 * @param struct seq_file *s
 * @param loff_t *pos
 *
 * @return void*, user pointer
 */
static void* lte_dd_ul_profile_seq_start(struct seq_file *s, loff_t *pos)
{
    char* func_name = "lte_dd_ul_profile_seq_start";


    if (*pos == 0)
    {
        // if zero, then start of file
        g_pdev->ul_iterator = 0;

        // lock the log so that new entries can't be added
        tsl_lock(g_pdev->ptsl_ul, 1);
    
        // prime the indicies for the tsl_log_get_entry() functions
        tsl_log_prime(g_pdev->ptsl_ul);

        // save the number of elements currently in the log
        tsl_log_get_num_entries(g_pdev->ptsl_ul, &(g_pdev->ul_entries));

        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
            "%s - s:%p pos:%d num UL entries:%ld\n",
            func_name, s, (int) *pos, g_pdev->ul_entries);
            
        if (g_pdev->ul_entries == 0)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
                "%s - unlocking log\n", func_name);

            // unlock this tsl so that new entries can be added
            tsl_lock(g_pdev->ptsl_ul, 0);

            return NULL;
        }
    }
    else
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
            "%s - s:%p pos:%d\n", func_name, s, (int) *pos);

        // if greater than what is currently locked, then we are done
        if (*pos >= g_pdev->ul_entries)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
                "%s - unlocking log\n", func_name);

            // unlock this tsl so that new entries can be added
            tsl_lock(g_pdev->ptsl_ul, 0);

            return NULL;
        }
    }

    return &(g_pdev->ul_iterator);
}


/**
 * sequence function for UL profile data
 *
 * @param struct seq_file *s
 * @param void *v
 * @param loff_t *pos
 *
 * @return void*, user pointer
 */
static void* lte_dd_ul_profile_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    char* func_name = "lte_dd_ul_profile_seq_next";


    (*pos)++;

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - s:%p pos:%d\n", func_name, s, (int) *pos);

    // if requesting a position greater than what is available, then this
    // should trigger a final stop
    if (*pos >= g_pdev->ul_entries)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
            "%s - unlocking log\n", func_name);

        // unlock this tsl so that new entries can be added
        tsl_lock(g_pdev->ptsl_ul, 0);

        return NULL;
    }

    g_pdev->ul_iterator = *pos;

    return &(g_pdev->ul_iterator);

}


/**
 * sequence function for UL profile data.
 *
 * @param struct seq_file *s
 * @param void *v
 *
 * @return void*, user pointer
 */
static void lte_dd_ul_profile_seq_stop(struct seq_file *s, void *v)
{
    char* func_name = "lte_dd_ul_profile_seq_stop";


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - s:%p v:%p\n", func_name, s, v);
}


/**
 * sequence function for UL profile data
 *  This is the main function for displaying UL profile data
 *
 * @param struct seq_file *s
 * @param void *v
 *
 * @return void*, user pointer
 */
static int lte_dd_ul_profile_seq_show(struct seq_file *s, void *v)
{
    char* func_name = "lte_dd_ul_profile_seq_show";
    int err = 0;
    tsl_entry_t* ptsl_entry = NULL;
    long retval = TSL_ERROR_ERROR_NONE;
    union ktime t0, t1, t2;

   
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - s:%p v:%p it:%d\n", func_name, s, v, (int) g_pdev->ul_iterator);

    if (g_pdev->ul_iterator == 0)
        seq_printf(s, "UL packet time stamps\nSeq Pointer Size FIFO-Enqueue SDMA-UL-Submit SDMA-UL-Complete\n");

    retval = tsl_log_get_entry_by_index(g_pdev->ptsl_ul, g_pdev->ul_iterator, &ptsl_entry);
    if ((retval == TSL_ERROR_ERROR_NONE || retval == TSL_WARNING_TSL_LAST_LOG_ENTRY))
    {
        // events have special pointer values
        if (ptsl_entry->puser >= ((void*)UL_TSL_EVENT_MIN) && 
            ptsl_entry->puser <= ((void*)UL_TSL_EVENT_MAX))
        {
            t0 = ptsl_entry->stamps[UL_TSL_TSNDX_EVENT];
            t1.tv64 = 0;
            t2.tv64 = 0;
        }
        else
        {
            // get the various timestamps
            t0 = ptsl_entry->stamps[UL_TSL_TSNDX_ENQUEUE];
            t1 = ptsl_entry->stamps[UL_TSL_TSNDX_SUBMIT];
            t2 = ptsl_entry->stamps[UL_TSL_TSNDX_COMPLETE];
        }

#if defined (SEQ_SHOW_TIMESPEC)
        {
            struct timespec ts0, ts1, ts2;

            ts0 = ktime_to_timespec(t0);
            ts1 = ktime_to_timespec(t1);
            ts2 = ktime_to_timespec(t2);

            seq_printf(s, "%ld 0x%p %ld %ld.%09ld %ld.%09ld %ld.%09ld\n",
                ptsl_entry->sequence,
                ptsl_entry->puser,
                ptsl_entry->user,
                ts0.tv_sec, ts0.tv_nsec,
                ts1.tv_sec, ts1.tv_nsec,
                ts2.tv_sec, ts2.tv_nsec);

        }
#else
        seq_printf(s, "%ld 0x%p %ld %d.%09d %d.%09d %d.%09d\n",
            ptsl_entry->sequence,
            ptsl_entry->puser,
            ptsl_entry->user,
            t0.tv.sec, t0.tv.nsec,
            t1.tv.sec, t1.tv.nsec,
            t2.tv.sec, t2.tv.nsec);
#endif

    }
    else
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
            "%s - tsl_log_get_entry_by_index returned error:%ld\n", func_name, retval);

        err = -1;
    }


    return err;        
}

  
// sequence data for UL profile
static struct seq_operations lte_dd_ul_profile_seq_ops = {
        .start = lte_dd_ul_profile_seq_start,
        .next  = lte_dd_ul_profile_seq_next,
        .stop  = lte_dd_ul_profile_seq_stop,
        .show  = lte_dd_ul_profile_seq_show
};


/**
 * sequence function for UL profile data
 *  Make an open method which sets up the sequence operators
 *
 * @param struct inode *inode
 * @param struct file *file
 *
 * @return int, seq_open value
 */
static int lte_dd_ul_profile_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &lte_dd_ul_profile_seq_ops);
}


// file operations for UL profile
static struct file_operations lte_dd_ul_profile_proc_ops = {
    .owner   = THIS_MODULE,
    .open    = lte_dd_ul_profile_proc_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release
};


// ***************************************************************************
// DL profile sequence functions - start
// ***************************************************************************


/**
 * sequence function for DL profile data
 *
 * @param struct seq_file *s
 * @param loff_t *pos
 *
 * @return void*, user pointer
 */
static void* lte_dd_dl_profile_seq_start(struct seq_file *s, loff_t *pos)
{
    char* func_name = "lte_dd_dl_profile_seq_start";


    if (*pos == 0)
    {
        // if zero, then start of file
        g_pdev->dl_iterator = 0;

        // lock the log so that new entries can't be added
        tsl_lock(g_pdev->ptsl_dl, 1);
    
        // prime the indicies for the tsl_log_get_entry() functions
        tsl_log_prime(g_pdev->ptsl_dl);

        // save the number of elements currently in the log
        tsl_log_get_num_entries(g_pdev->ptsl_dl, &(g_pdev->dl_entries));

        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
            "%s - s:%p pos:%d num DL entries:%ld\n",
            func_name, s, (int) *pos, g_pdev->dl_entries);

        if (g_pdev->dl_entries == 0)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
                "%s - unlocking log\n", func_name);

            // unlock this tsl so that new entries can be added
            tsl_lock(g_pdev->ptsl_dl, 0);

            return NULL;
        }
    }
    else
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
            "%s - s:%p pos:%d\n", func_name, s, (int) *pos);

        // if greater than what is currently locked, then we are done
        if (*pos >= g_pdev->dl_entries)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
                "%s - unlocking log\n", func_name);

            // unlock this tsl so that new entries can be added
            tsl_lock(g_pdev->ptsl_dl, 0);

            return NULL;
        }
    }

    return &(g_pdev->dl_iterator);
}


/**
 * sequence function for DL profile data
 *
 * @param struct seq_file *s
 * @param void *v
 * @param loff_t *pos
 *
 * @return void*, user pointer
 */
static void* lte_dd_dl_profile_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    char* func_name = "lte_dd_dl_profile_seq_next";


    (*pos)++;

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - s:%p pos:%d\n", func_name, s, (int) *pos);

    // if requesting a position greater than what is available, then this
    // should trigger a final stop
    if (*pos >= g_pdev->dl_entries)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
            "%s - unlocking log\n", func_name);

        // unlock this tsl so that new entries can be added
        tsl_lock(g_pdev->ptsl_dl, 0);

        return NULL;
    }

    g_pdev->dl_iterator = *pos;

    return &(g_pdev->dl_iterator);
}


/**
 * sequence function for DL profile data.
 *
 * @param struct seq_file *s
 * @param void *v
 *
 * @return void*, user pointer
 */
static void lte_dd_dl_profile_seq_stop(struct seq_file *s, void *v)
{
    char* func_name = "lte_dd_dl_profile_seq_stop";


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - s:%p v:%p\n", func_name, s, v);
}


/**
 * sequence function for DL profile data
 *  This is the main function for displaying UL profile data
 *
 * @param struct seq_file *s
 * @param void *v
 *
 * @return void*, user pointer
 */
static int lte_dd_dl_profile_seq_show(struct seq_file *s, void *v)
{
    char* func_name = "lte_dd_dl_profile_seq_show";
    int err = 0;
    tsl_entry_t* ptsl_entry = NULL;
    long retval = TSL_ERROR_ERROR_NONE;
    union ktime t0, t1;

   
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - s:%p v:%p it:%d\n", func_name, s, v, (int) g_pdev->dl_iterator);

    if (g_pdev->dl_iterator == 0)
        seq_printf(s, "DL packet time stamps\nSeq Pointer Size SDMA-DL-Submit SDMA-DL-Complete\n");

    retval = tsl_log_get_entry_by_index(g_pdev->ptsl_dl, g_pdev->dl_iterator, &ptsl_entry);
    if ((retval == TSL_ERROR_ERROR_NONE || retval == TSL_WARNING_TSL_LAST_LOG_ENTRY))
    {
        // calculate the various timestamps
        t0 = ptsl_entry->stamps[DL_TSL_TSNDX_SUBMIT];
        t1 = ptsl_entry->stamps[DL_TSL_TSNDX_COMPLETE];

#if defined (SEQ_SHOW_TIMESPEC)
        {
            struct timespec ts0, ts1;

            ts0 = ktime_to_timespec(t0);
            ts1 = ktime_to_timespec(t1);

            seq_printf(s, "%ld 0x%p %ld %ld.%09ld %ld.%09ld\n",
                ptsl_entry->sequence,
                ptsl_entry->puser,
                ptsl_entry->user,
                ts0.tv_sec, ts0.tv_nsec,
                ts1.tv_sec, ts1.tv_nsec);

        }
#else
        seq_printf(s, "%ld 0x%p %ld %d.%09d %d.%09d\n",
            ptsl_entry->sequence,
            ptsl_entry->puser,
            ptsl_entry->user,
            t0.tv.sec, t0.tv.nsec,
            t1.tv.sec, t1.tv.nsec);
#endif

    }
    else
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
            "%s - tsl_log_get_entry_by_index returned error:%ld\n", func_name, retval);

        err = -1;
    }


    return err;
        
}

  
// sequence data for DL profile
static struct seq_operations lte_dd_dl_profile_seq_ops = {
        .start = lte_dd_dl_profile_seq_start,
        .next  = lte_dd_dl_profile_seq_next,
        .stop  = lte_dd_dl_profile_seq_stop,
        .show  = lte_dd_dl_profile_seq_show
};


/**
 * sequence function for DL profile data
 *  Make an open method which sets up the sequence operators
 *
 * @param struct inode *inode
 * @param struct file *file
 *
 * @return int, seq_open value
 */
static int lte_dd_dl_profile_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &lte_dd_dl_profile_seq_ops);
}


// file operations for UL profile
static struct file_operations lte_dd_dl_profile_proc_ops = {
    .owner   = THIS_MODULE,
    .open    = lte_dd_dl_profile_proc_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release
};


// ***************************************************************************
// UL/DL profile sequence functions - end
// ***************************************************************************


/**
 * Prints data buffer.
 *
 * @param pbanner - log banner.
 * @param pbuffer - buffer to print.
 * @param buf_size - buffer size.
 * @param force - force print.
 *
 * @return void
 */
static inline void print_buf(char* pbanner, char* pbuffer, unsigned long buf_size, int force)
{
#if defined (AMP_LOG_ENABLE_LOG_PRINT)
    int i;
    short log_level = AMP_LOG_LEVEL_WARNING;


    AMP_LOG_GetLogLevel(AMP_LOG_ADDR_LTE_DATA_DRIVER, log_level);
    
    if (force || log_level >= AMP_LOG_LEVEL_DEBUG2)
    {  
        printk("%s", pbanner);
        printk("%p:%d\n", pbuffer, (int) buf_size);
        for (i = 0; i < buf_size; i++)
        {
            if ((0 == (i % 16)) && i != 0)
                printk("\n");

            printk("%02x ", (unsigned char)pbuffer[i]);
        }
        printk("\n");
    }
#endif
}


/**
 * Generates a loopback (ping, udp, etc) given an ip payload
 *
 * @param pbuffer - pointer to IP header
 * @param buf_size - buffer size
 *
 * @return long, error code
 */
static long generate_loopback_reply(char* pbuffer, unsigned long buf_size)
{
    char* func_name = "generate_loopback_reply";
    struct iphdr* ih = NULL;
    struct icmphdr* icmp = NULL;
    struct udphdr* udp = NULL;
    struct tcphdr* tcp = NULL;
    u32 saddr = 0;
    char ip_version = 0;
    struct ipv6hdr* ihv6 = NULL;
    struct icmp6hdr* icmpv6 = NULL;
    struct in6_addr saddrv6;
    int payload_len = 0;

    
    if (buf_size < 4)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
            "%s - buf_size:%d is less 4 bytes, not a valid IP header\n",
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
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
            "%s - Not a valid IP4 or IPV6 header, IP version detected as:%d\n",
            func_name, (int) ip_version);
        return -1;
    }
    
    if (ip_version == 4)
    {
        // swizzle based on IPV4
        if ((size_t) buf_size < sizeof(struct iphdr))
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                "%s - buf_size:%d is less than the IPV4 header size:%d\n",
                func_name, (int) buf_size, (int) sizeof(struct iphdr));
            return -1;
        }
        
        // cast to an IP4 header
        ih = (struct iphdr*)(pbuffer);
        payload_len = ntohs(ih->tot_len) - (ih->ihl << 2);

        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
            "%s - IPV4 header detected - protocol(0x%x); payload_len:%d\n",
            func_name, (int) ih->protocol, payload_len);

        switch(ih->protocol)
        {
            case IPPROTO_ICMP:
                
                if ((size_t) buf_size < (sizeof(struct iphdr) + sizeof(struct icmphdr)))
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                        "%s - buf_size:%d is less than the IP header size:%d + ICMP header size:%d\n",
                        func_name, (int) buf_size, (int) sizeof(struct iphdr), (int) sizeof(struct icmphdr));
                    return -2;
                }

                // cast to a ICMP header
                icmp = (struct icmphdr*) (pbuffer + sizeof(struct iphdr));

                // check to see if this ICMP is a ping request, if not then return
                if (icmp->type != ICMP_ECHO)
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                        "%s - ICMP type:0x%x is not ICMP_ECHO(0x%x)\n",
                        func_name, (int) icmp->type, (int) ICMP_ECHO);
                    return -3;
                }

                // swap the src and destination address
                saddr = ih->saddr;
                ih->saddr = ih->daddr;
                ih->daddr = saddr;

                // recacluate the IP header checksum
                ih->check = 0;
                ih->check = ip_fast_csum((unsigned char*) ih, ih->ihl);
                
                // modify the ICMP header to be a ping reply and recacluate the ICMP header checksum
                icmp->type = ICMP_ECHOREPLY;

                // recalcuate the ICMP checksum
//#define USE_IPV4_TCPUDP_MAGIC_CHECKSUM_CALC 
#if defined (USE_IPV4_TCPUDP_MAGIC_CHECKSUM_CALC)
                icmp->checksum = 0;
                icmp->checksum = csum_tcpudp_magic(ih->saddr, ih->daddr,
                    payload_len, ih->protocol, csum_partial(icmp, payload_len, 0));
#else
                icmp->checksum = 0;
                icmp->checksum = ip_compute_csum(icmp, payload_len);
#endif

                break;

            
            case IPPROTO_UDP:
            
                if ((size_t) buf_size < (sizeof(struct iphdr) + sizeof(struct udphdr)))
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                        "%s - buf_size:%d is less than the IP header size:%d + UDP header size:%d\n",
                        func_name, (int) buf_size, (int) sizeof(struct iphdr), (int) sizeof(struct udphdr));
                    return -2;
                }
                
                // cast to a UDP header
                udp = (struct udphdr*) (pbuffer + sizeof(struct iphdr));
                
                // swap the src and destination address
                saddr = ih->saddr;
                ih->saddr = ih->daddr;
                ih->daddr = saddr;
                
                // recacluate the IP header checksum
                ih->check = 0;
                ih->check = ip_fast_csum((unsigned char*) ih, ih->ihl);
                
                // Do NOT swap the source and destination ports since we are simply looping back

#if defined (USE_IPV4_TCPUDP_MAGIC_CHECKSUM_CALC)
                // recalcuate the UDP header checksum
                udp->check = 0;
                udp->check = csum_tcpudp_magic(ih->saddr, ih->daddr,
                    payload_len, ih->protocol, csum_partial(udp, payload_len, 0));
#endif

                break;
                
                
            case IPPROTO_TCP:
            
                if ((size_t) buf_size < (sizeof(struct iphdr) + sizeof(struct tcphdr)))
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                        "%s - buf_size:%d is less than the IP header size:%d + TCP header size:%d\n",
                        func_name, (int) buf_size, (int) sizeof(struct iphdr), (int) sizeof(struct tcphdr));
                    return -2;
                }
                
                // cast to a TCP header
                tcp = (struct tcphdr*) (pbuffer + sizeof(struct iphdr));
                
                // swap the src and destination address
                saddr = ih->saddr;
                ih->saddr = ih->daddr;
                ih->daddr = saddr;
                
                // recacluate the IP header checksum
                ih->check = 0;
                ih->check = ip_fast_csum((unsigned char*) ih, ih->ihl);
                
                // Do NOT swap the source and destination ports since we are simply looping back
                
#if defined (USE_IPV4_TCPUDP_MAGIC_CHECKSUM_CALC)
                // recalcuate the TCP header checksum
                tcp->check = 0;
                tcp->check = csum_tcpudp_magic(ih->saddr, ih->daddr,
                    payload_len, ih->protocol, csum_partial(tcp, payload_len, 0));
#endif

                break;
                
        }
    }
    else
    {
        // swizzle based on IPV6
        if ((size_t) buf_size < sizeof(struct ipv6hdr))
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                "%s - buf_size:%d is less than the IPV6 header size:%d\n",
                func_name, (int) buf_size, (int) sizeof(struct ipv6hdr));
            return -1;
        }
        
        memset(&saddrv6, 0x00, sizeof(struct in6_addr));

        // cast to an IP6 header
        ihv6 = (struct ipv6hdr*)(pbuffer);
        payload_len = ihv6->payload_len;
            
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
            "%s - IPV6 header detected - protocol(0x%x); payload_len:%d\n",
            func_name, (int) ihv6->nexthdr, payload_len);
        
        // specifies the next encapsulated protocol, the values are compatible with
        // those specified for the IPv4 protocol field.
        switch(ihv6->nexthdr)
        {
            case IPPROTO_ICMPV6:
                
                if ((size_t) buf_size < (sizeof(struct ipv6hdr) + sizeof(struct icmp6hdr)))
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                        "%s - buf_size:%d is less than the IPV6 header size:%d + ICMPV6 header size:%d\n",
                        func_name, (int) buf_size, (int) sizeof(struct ipv6hdr), (int) sizeof(struct icmp6hdr));
                    return -2;
                }

                // cast to a ICMPV6 header
                icmpv6 = (struct icmp6hdr*) (pbuffer + sizeof(struct ipv6hdr));

                // check to see if this ICMP is a ping request, if not then return
                if (icmpv6->icmp6_type != ICMPV6_ECHO_REQUEST)
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                        "%s - ICMPV6 type:0x%x is not ICMPV6_ECHO_REQUEST(0x%x)\n",
                        func_name, (int) icmpv6->icmp6_type, (int) ICMPV6_ECHO_REQUEST);
                    return -3;
                }
                
                // swap the source and destination address of IPV6 header
                saddrv6 = ihv6->saddr;
                ihv6->saddr = ihv6->daddr;
                ihv6->daddr = saddrv6;
                
                // modify the ICMPV6 header to be a ping reply
                icmpv6->icmp6_type = ICMPV6_ECHO_REPLY;
                
                // recalcuate the ICMPV6 checksum
#define USE_IPV6_MAGIC_CHECKSUM_CALC
#if defined (USE_IPV6_MAGIC_CHECKSUM_CALC)
                icmpv6->icmp6_cksum = 0;
                icmpv6->icmp6_cksum = csum_ipv6_magic(&ihv6->saddr, &ihv6->daddr,
                    payload_len, ihv6->nexthdr, csum_partial(icmpv6, payload_len, 0));
#else
                icmpv6->icmp6_cksum = 0;
                icmpv6->icmp6_cksum = ip_compute_csum(ihv6, sizeof(struct ipv6hdr) + sizeof(struct icmp6hdr));
#endif
                
                break;


            case IPPROTO_UDP:
            
                if ((size_t) buf_size < (sizeof(struct ipv6hdr) + sizeof(struct udphdr)))
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                        "%s - buf_size:%d is less than the IPV6 header size:%d + UDP header size:%d\n",
                        func_name, (int) buf_size, (int) sizeof(struct ipv6hdr), (int) sizeof(struct udphdr));
                    return -2;
                }
                
                // swap the source and destination address of IPV6 header
                saddrv6 = ihv6->saddr;
                ihv6->saddr = ihv6->daddr;
                ihv6->daddr = saddrv6;
                
                // cast to a UDP header
                udp = (struct udphdr*) (pbuffer + sizeof(struct ipv6hdr));
            
                // Do NOT swap the source and destination ports since we are simply looping back
                
#if defined (USE_IPV6_MAGIC_CHECKSUM_CALC)
                // recalcuate the UDP header checksum
                udp->check = 0;
                udp->check = csum_ipv6_magic(&ihv6->saddr, &ihv6->daddr,
                    payload_len, ihv6->nexthdr, csum_partial(udp, payload_len, 0));
#endif
                
                break;


            case IPPROTO_TCP:
            
                if ((size_t) buf_size < (sizeof(struct ipv6hdr) + sizeof(struct tcphdr)))
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                        "%s - buf_size:%d is less than the IPV6 header size:%d + TCP header size:%d\n",
                        func_name, (int) buf_size, (int) sizeof(struct ipv6hdr), (int) sizeof(struct tcphdr));
                    return -2;
                }
                
                // swap the source and destination address of IPV6 header
                saddrv6 = ihv6->saddr;
                ihv6->saddr = ihv6->daddr;
                ihv6->daddr = saddrv6;
                
                // cast to a TCP header
                tcp = (struct tcphdr*) (pbuffer + sizeof(struct ipv6hdr));
            
                // Do NOT swap the source and destination ports since we are simply looping back

#if defined (USE_IPV6_MAGIC_CHECKSUM_CALC)
                // recalcuate the TCP header checksum
                tcp->check = 0;
                tcp->check = csum_ipv6_magic(&ihv6->saddr, &ihv6->daddr,
                    payload_len, ihv6->nexthdr, csum_partial(tcp, payload_len, 0));
#endif
                break; 
        
        }
    
    }

    return 0;
}


/**
 * Returns a net info structure that contains the pnet device
 *
 * @param void* pnet - network object returned from probe call
 *
 * @return lte_dd_net_info_t*, 0 or valid pointer
 */ 
static inline lte_dd_net_info_t* lte_dd_ni_interface_find_pnet(void* pnet)
{
    lte_dd_net_info_t* pni = NULL;
    int i = 0;
    
    
    if (!pnet)
        return NULL;

    // scan table looking for matching pnet, only return if non-zero
    for (i = 0; i < (sizeof(g_pdev->ni_interfaces) / sizeof(lte_dd_net_info_t)); i++)
    {
        pni = &(g_pdev->ni_interfaces[i]);
        if (pni->pnet && pni->pnet == pnet)
            return pni;
    }
    
    return NULL;
}


/**
 * Returns a net info structure that contains the pnet device
 *
 * @param char* ifname - interface name
 *
 * @return lte_dd_net_info_t*, 0 or valid pointer
 */ 
static inline lte_dd_net_info_t* lte_dd_ni_interface_find_ifname(char* ifname)
{
    lte_dd_net_info_t* pni = NULL;
    int i = 0;
    
    
    if (!ifname)
        return NULL;

    // scan table looking for matching ifname
    for (i = 0; i < (sizeof(g_pdev->ni_interfaces) / sizeof(lte_dd_net_info_t)); i++)
    {
        pni = &(g_pdev->ni_interfaces[i]);
        if (pni->pnet && strcasecmp(pni->ifname, ifname) == 0)
            return pni;
    }
    
    return NULL;
}


/**
 * Returns a net info structure at the specified index
 *
 * @param int index - net info index
 *
 * @return lte_dd_net_info_t*, 0 or valid pointer
 */ 
static inline lte_dd_net_info_t* lte_dd_ni_interface_get_at_index(int index)
{
    lte_dd_net_info_t* pni = NULL;
    
    
    if (index < 0 || index >= (sizeof(g_pdev->ni_interfaces) / sizeof(lte_dd_net_info_t)))
        return NULL;

    pni = &(g_pdev->ni_interfaces[index]);
    
    return pni;
}


/**
 * Returns the first available net info structure.  This is any structure whose
 *  pnet field is zero
 *
 * @param none
 *
 * @return lte_dd_net_info_t*, 0 or valid pointer
 */ 
static lte_dd_net_info_t* lte_dd_ni_interface_alloc(void)
{
    lte_dd_net_info_t* pni = NULL;
    int i = 0;
 

    // scan table looking for the first non-null pnet field
    for (i = 0; i < (sizeof(g_pdev->ni_interfaces) / sizeof(lte_dd_net_info_t)); i++)
    {
        pni = &(g_pdev->ni_interfaces[i]);
        if (!pni->pnet)
            return pni;
    }
    
    return NULL;
}


/**
 * Resets the fields in the net info structure to zero, thus freeing it
 *
 * @param none
 *
 * @return lte_dd_net_info_t*, 0 or valid pointer
 */ 
static void lte_dd_ni_interface_free(lte_dd_net_info_t* pni)
{

    memset(pni, 0x00, sizeof(lte_dd_net_info_t));

}


/**
 * Returns the net info structure for the persistent interface lte0
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return lte_dd_net_info_t*, 0 or valid pointer
 */
static inline lte_dd_net_info_t* lte_dd_ni_interface_get_persistent(lte_dd_dev_t* pdev)
{
    lte_dd_net_info_t* pni = NULL;

    // lte0 is always at index zero
    pni = &(g_pdev->ni_interfaces[0]);
    
    return pni;
}


/**
 * Performs lookup of the availaible net interfaces and checks to see which interface
 * should be used for an UL transfer based on packet availability, priority, etc
 *
 * @param lte_dd_dev_t* pdev - device object
 * @param long* pnum_fifo_elements - receives the number of FIFO elements
 *
 * @return lte_dd_net_info_t*, 0 or valid pointer
 */
static inline lte_dd_net_info_t* lte_dd_ni_interface_fifo_lookup(lte_dd_dev_t* pdev, long* pnum_fifo_elements)
{
    lte_dd_net_info_t* pni = NULL;
    static unsigned long running_index = 0;
    unsigned long array_index = 0;
    int num_elements = 0;
    int i = 0;

 
    // validate params
    if (!pnum_fifo_elements)
        return NULL;
    
    *pnum_fifo_elements = 0;

    // if only one interface, then check the persistent interface which is lte0
    if (pdev->ni_interfaces_cnt == 1)
    {
        pni = lte_dd_ni_interface_get_persistent(pdev);
        ptr_fifo_num_elements(pni->h_tx_data_fifo, &num_elements);
        if (num_elements > 0)
        {
            // if not in multi packet mode, then limit the count to 1
            if (!(pdev->dd_flags & LTE_DD_FLAG_MULTI_PACKET))
                num_elements = 1;
                            
            *pnum_fifo_elements = (long) num_elements;
        
            return pni;
        }
    }
    else
    {
        // scan through the entire net info list and find an interface that
        // has been created AND has at least one element in its FIFO
        // for now, ignore the net info priority and use a round robin method
        for (i = 0; i < (sizeof(pdev->ni_interfaces) / sizeof(lte_dd_net_info_t)); i++)
        {
            array_index =  running_index % (sizeof(pdev->ni_interfaces) / sizeof(lte_dd_net_info_t));
            pni = &(pdev->ni_interfaces[array_index]);
            running_index++;
            
            // only check the FIFO if an interface has been created
            if (pni->pnet != 0)
            {
                // get the number of FIFO elements 
                ptr_fifo_num_elements(pni->h_tx_data_fifo, &num_elements);
                if (num_elements > 0)
                {
                             
#if defined (LTE_DD_DBG_INFO_FIFO_LOOKUP)
                    AMP_LOG_Info(
                        "using array index:%lu; ifname:%s; num_elements:%d\n",
                        "lte_dd_ni_interface_fifo_lookup",
                        array_index, pni->ifname, num_elements);
#endif

                    // if not in multi packet mode, then limit the count to 1
                    if (!(pdev->dd_flags & LTE_DD_FLAG_MULTI_PACKET))
                        num_elements = 1;
                                
                    *pnum_fifo_elements = (long) num_elements;
                    
                    return pni;
               }
            }
        }
    }
    
    return NULL;
}


/**
 * zeros the TX arrays and counts
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return none
 */
static inline void lte_dd_zero_tx_arrays(lte_dd_dev_t* pdev)
{

    // reset arrays and counts
    pdev->tx_sdma_xfer_cnt = 0;
    memset(pdev->tx_sdma_xfer, 0x00, sizeof(pdev->tx_sdma_xfer));
    pdev->tx_sdma_packet_cnt = 0;
    memset(pdev->tx_sdma_packet, 0x00, sizeof(pdev->tx_sdma_packet));

}


/**
 * zeros the RX arrays and counts
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return none
 */
static inline void lte_dd_zero_rx_arrays(lte_dd_dev_t* pdev)
{

    // reset arrays and counts
    pdev->rx_sdma_xfer_cnt = 0;
    memset(pdev->rx_sdma_xfer, 0x00, sizeof(pdev->rx_sdma_xfer));
    pdev->rx_sdma_packet_cnt = 0;
    memset(pdev->rx_sdma_packet, 0x00, sizeof(pdev->rx_sdma_packet));
    
}
          

/**
 * Constructs a SDMA xfer list.
 *
 *  NOTICE!!!!  This function can allocate fewer buffers than resuested.  This is NOT an
 *  error condition.  The caller must look at the "psdma_xfer_cnt" variable for the actual
 *  number of buffers allocated.  This function is used for DL packets only
 *
 * @param lte_dd_dev_t* pdev - device object
 * @param lte_dd_net_info_t* pni - network interface pointer
 * @param packet_info pi[IPC_4GHIF_MAX_NB_DL_FRAMES] - description of packets to be created
 * @param long pi_cnt - items currently populated in "pi"
 * @param lte_dd_sdma_xfer_t sdma_xfer[IPC_4GHIF_MAX_NB_DL_FRAMES] - xfer structures to be populated
 * @param long* psdma_xfer_cnt - items populated in "sdma_xfer"
 *
 * @return long, error code
 */ 
static inline long lte_dd_alloc_sdma_xfer_list(lte_dd_dev_t* pdev, lte_dd_net_info_t* pni,
    packet_info pi[IPC_4GHIF_MAX_NB_DL_FRAMES], long pi_cnt,
    lte_dd_sdma_xfer_t sdma_xfer[IPC_4GHIF_MAX_NB_DL_FRAMES], long* psdma_xfer_cnt)
{
    char* func_name = "lte_dd_alloc_sdma_xfer_list";
    int i = 0;
    short log_level = AMP_LOG_LEVEL_WARNING;
    struct sk_buff* pskb = NULL;
    packet_info* ppi = NULL;
    lte_dd_sdma_xfer_t* psdma_xfer = NULL;

    
    // validate params
    if (!pdev || !pni || !psdma_xfer_cnt || pi_cnt < 1 || pi_cnt > IPC_4GHIF_MAX_NB_DL_FRAMES)
    {
        AMP_LOG_Error(
            "%s - ERROR: invalid parameter was supplied: pdev:%p; pni:%p; pxfer_cnt:%p; pi_cnt:%d;\n",
            func_name, pdev, pni, psdma_xfer_cnt, (int) pi_cnt);
            
        return AMP_ERROR_LTEDD_INVALID_PARAM;
    }
    
    *psdma_xfer_cnt = 0;
    memset(sdma_xfer, 0x00, sizeof(sdma_xfer));
    
    AMP_LOG_GetLogLevel(AMP_LOG_ADDR_LTE_DATA_DRIVER, log_level);
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG2,
                "%s - cnt:%d packet info:\n", func_name, (int) pi_cnt);
    
    for (i = 0; i < (int) pi_cnt; i++)
    {
        ppi = &(pi[i]);
        psdma_xfer = &(sdma_xfer[i]);
        
        if (log_level >= AMP_LOG_LEVEL_DEBUG2)
            printk("    type:%d buf_size:%d\n", ppi->packet_type, ppi->buf_size);
            
        // NOTE: if called within a spinlock, GFP_ATOMIC must be used
        // if outside a spinlock, then GFP_KERNEL can be used since it may sleep.
        // call the net driver to allocate a skb for this payload
        pskb = pni->net_drv_bus_interface.rx_start(pni->pnet, pdev, (int) ppi->buf_size,
            GFP_ATOMIC);
        
        if (!pskb)
        {
            if (*psdma_xfer_cnt == 0)
            {
                // if we get an alloc failure on the first element, then this is a fatal error
                AMP_LOG_Error(
                    "%s - ERROR: allocating skb, fatal error\n",
                    func_name);
                
                return AMP_ERROR_LTEDD_RESOURCE_ALLOC;
            }
            else
            {
                AMP_LOG_Error(
                    "%s - WARNING: allocating skb, unable to allocate total number requested. request:%d actual:%d\n",
                    func_name, (int) pi_cnt, (int) *psdma_xfer_cnt);
                    
                return AMP_ERROR_NONE;
            }
        }
        else
        {
            *psdma_xfer_cnt += 1;
            
            psdma_xfer->pni = pni;
            psdma_xfer->pskb = pskb;
            psdma_xfer->sdma.packet_type = ppi->packet_type;
            psdma_xfer->sdma.pbuf = pskb->data;
            psdma_xfer->sdma.buf_size = ppi->buf_size;
            psdma_xfer->sdma.transferred_status = 0;
            psdma_xfer->sdma.bearerID = 0;   
            psdma_xfer->sdma.user = (unsigned long) i;
            psdma_xfer->sdma.puser = psdma_xfer;
            
            
#if defined (LTE_DD_TRACK_HIF_PHYS_ADDR_ALIGNMENT)
            {
                unsigned long phys_addr = virt_to_phys(psdma_xfer->sdma.pbuf);
            
                if      (phys_addr & 1) pdev->rx_physaddr_align8_cnt++;
                else if (phys_addr & 2) pdev->rx_physaddr_align16_cnt++;
                else                    pdev->rx_physaddr_align32_cnt++;
            }
#endif
            
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                "%s - ifname:%s; sk_ptr:%p; sk_data_ptr:%p; sk_data_size:%d; sdma_data_ptr:%p sdma_data_size:%d;\n",
                func_name,
                pni->ifname,
                pskb,
                pskb->data,
                (int) pskb->len,
                psdma_xfer->sdma.pbuf,
                (int) psdma_xfer->sdma.buf_size);
        }
            
    }
    
    return AMP_ERROR_NONE;
    
}


/**
 * Frees skbuf if allocated and resets the sdma xfer structure to all zeros
 *  This function does NOT free the psdma_xfer pointer
 *
 * @param lte_dd_sdma_xfer_t* psdma_xfer - the sdma structire to release
 *
 * @return long, error code
 */
static inline long lte_dd_free_sdma_xfer(lte_dd_sdma_xfer_t* psdma_xfer)
{

    // validate param
    if (!psdma_xfer)            
        return AMP_ERROR_LTEDD_INVALID_PARAM;

    if (psdma_xfer->pskb)
        dev_kfree_skb_any(psdma_xfer->pskb);
                    
    memset(psdma_xfer, 0x00, sizeof(lte_dd_sdma_xfer_t));

    return AMP_ERROR_NONE;
}


/**
 * Starts a TX SDMA transfer (UL)
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return long, error code
 */ 
static long lte_dd_sdma_tx(lte_dd_dev_t* pdev)
{
    char* func_name = "lte_dd_sdma_tx";
    int i = 0;
    long num_fifo_elements = 0;
    long num_packets_to_tx = 0;
    u8 num_sdma_accepted = 0;
    struct sk_buff* pskb = NULL;
    long pfval = PTR_FIFO_ERROR_NONE;
    lte_dd_net_info_t* pni = NULL;
    lte_dd_sdma_xfer_t* psdma_xfer = NULL;
    IPC_4GHIF_STATUS ipc4_status = IPC_4GHIF_STATUS_OK;
    unsigned long entry_index = 0;
    unsigned long sl_flags = 0;
    
    

    spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
    
    // check to make sure that we are not already busy OR throttled
    if (pdev->dd_flags & LTE_DD_FLAG_TX_BUSY || pdev->dd_flags & LTE_DD_FLAG_TX_THROTTLE)
    {
        spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
        return AMP_ERROR_NONE;
    }
      
    // now scan all the net info fifo's and pick a net info to use for FIFO dequeue
    // if all of the FIFO's are empty, then this function returns NULL
    pni = lte_dd_ni_interface_fifo_lookup(pdev, &num_fifo_elements);
    if (!pni)
    {
        spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
        return AMP_ERROR_LTEDD_NET_INFO;
    }

    // set the busy flags, these flags are cleared in the
    // 4GHIF_ul_complete callback
    pdev->dd_flags |= LTE_DD_FLAG_TX_BUSY;
    pni->ni_flags |= LTE_NI_FLAG_TX_BUSY;
    pdev->tx_data_busy_start_time = jiffies;
    
    spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
    
    
    // construct the tx sdma xfer list and make sure not to exceed the array
    pdev->tx_sdma_xfer_cnt = 0;
    memset(pdev->tx_sdma_xfer, 0x00, sizeof(pdev->tx_sdma_xfer));
    num_packets_to_tx = min(num_fifo_elements, (long) (sizeof(pdev->tx_sdma_xfer) / sizeof(lte_dd_sdma_xfer_t)));
    for (i = 0; i < (int) num_packets_to_tx; i++)
    {
        pfval = ptr_fifo_peek_offset(pni->h_tx_data_fifo, i, (void**) &pskb);
        if (pfval != PTR_FIFO_ERROR_NONE && i == 0)
        {
            spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
            
            pdev->dd_flags &= ~LTE_DD_FLAG_TX_BUSY;
            pni->ni_flags &= ~LTE_NI_FLAG_TX_BUSY;
                
            spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
            
            return AMP_ERROR_LTEDD_FIFO_REMOVE;
        }
        
        // drop zero length packets, the 4GHIF API will return an error for zero length.
        // remember
        if (pskb->len == 0)
        {
            // We should add a fail-safe check here for zero length packets (ZLP).
            // Currently, this isn't an issue because the network driver drops ZLPs
            // so they are never enqueued into the fifo. If this fail-safe method is
            // implemented, it needs to make sure that:
            //
            //  1. This ZLP is not submitted to the 4GHIF API
            //  2. If mutiple packets are available (ie: num_packets_to_tx > 1),
            //     that this packet is released during the UL complete callback and
            //     that the correct number of FIFO elements are removed.
            //  3. If only a single packet is available, then the skbuf should be
            //     removed at this time and freed.  Then this function should
            //     simply return.
            
            AMP_LOG_Error(
                "%s - ERROR: zero length packet encountered... network driver should NOT enqueue ZLPs into fifo\n",
                func_name);
        }
 
        psdma_xfer = &(pdev->tx_sdma_xfer[pdev->tx_sdma_xfer_cnt]);
    
        psdma_xfer->pni = pni;
        psdma_xfer->pskb = pskb;
        psdma_xfer->sdma.packet_type = PKT_TYPE_UL_DATA;
        psdma_xfer->sdma.pbuf = pskb->data;
        psdma_xfer->sdma.buf_size = (size_t) pskb->len; 
        
        psdma_xfer->sdma.transferred_status = 0;
        psdma_xfer->sdma.bearerID = net_drv_get_skb_eps_id(pskb);
        psdma_xfer->sdma.user = (unsigned long) pdev->tx_sdma_xfer_cnt;
        psdma_xfer->sdma.puser = psdma_xfer;
        
        pdev->tx_sdma_xfer_cnt += 1;

        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
            "%s - ifname:%s; sk_ptr:%p; sk_data_ptr:%p; sk_data_size:%d; sdma_data_ptr:%p sdma_data_size:%d; eps_id:%d;\n",
            func_name,
            pni->ifname,
            pskb,
            pskb->data,
            (int) pskb->len,
            psdma_xfer->sdma.pbuf,
            (int) psdma_xfer->sdma.buf_size,
            (int) psdma_xfer->sdma.bearerID);

#if defined (LTE_DD_TRACK_HIF_PHYS_ADDR_ALIGNMENT)
        {
            unsigned long phys_addr = virt_to_phys(psdma_xfer->sdma.pbuf);
        
            if      (phys_addr & 1) pdev->tx_physaddr_align8_cnt++;
            else if (phys_addr & 2) pdev->tx_physaddr_align16_cnt++;
            else                    pdev->tx_physaddr_align32_cnt++;
        }
#endif
        
    }
    
    // if more than one packet, keep count of the number of times this condition occurs
    if (pdev->tx_sdma_xfer_cnt > 1)
        pdev->tx_data_packets_multi_avail++;
    
    // now construct an array of "sdma_packet" pointers for the 4GHIF API
    pdev->tx_sdma_packet_cnt = pdev->tx_sdma_xfer_cnt;
    memset(pdev->tx_sdma_packet, 0x00, sizeof(pdev->tx_sdma_packet));
    for (i = 0; i < (int) pdev->tx_sdma_xfer_cnt; i++)
    {
        pdev->tx_sdma_packet[i] = &(pdev->tx_sdma_xfer[i].sdma);
        pskb = pdev->tx_sdma_xfer[i].pskb;
        
#if defined (LTE_DD_TSL_PROFILE)
        // fetch the index TSL entry index that was saved with this skb
        entry_index = lte_dd_get_skb_profile_index(pskb);

        // add the submit time stamp
        tsl_add_time_stamp(pdev->ptsl_ul, entry_index, UL_TSL_TSNDX_SUBMIT);
#endif
        
#if defined (OMAP_DM_PROFILE)
        omap_dm_timer_log_evt(OMAP_DM_TIMER_DEBUG_ID('l', 't', 'e', '4'), (u32) pskb);
#endif
    }
    
    // save the tx submit start time
    pdev->tx_kt_submit = ktime_get();

    // call the SDMA API to send the packets
    ipc4_status = IPC_4GHIF_submit_ul(pdev->tx_sdma_packet_cnt, pdev->tx_sdma_packet, &num_sdma_accepted);
    pdev->tx_data_packets_sdma_submit++;
        
    // cache the last error code if an error
    if (ipc4_status != IPC_4GHIF_STATUS_OK)
        IPC_4GHIF_get_error_string(ipc4_status, pdev->last_sdma_err_msg, sizeof(pdev->last_sdma_err_msg));

    switch (ipc4_status)
    {
        case IPC_4GHIF_STATUS_OK:            
            pdev->tx_packet_cnt += (unsigned long) num_sdma_accepted;
            break;
        
        // for this case, call a SDMA API cancel function to clear things
        case IPC_4GHIF_STATUS_BUSY:             // previous UL transferred not ended
        
            // this is an internal error, we should be able to clear this case
            AMP_LOG_Error(
                "%s - ipc4_status:%d(%s); internal error, IPC should NOT be busy\n",
                func_name, ipc4_status, pdev->last_sdma_err_msg);
                
            break;
        
        // for this case, wait for callback to restart the UL
        case IPC_4GHIF_STATUS_RESOURCES_FULL:   // cannot transmit the packet because shared memory (DATA_UL or FIFO_UL) is full
        
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                "%s - ipc4_status:%d(%s); now waiting for lte_dd_4ghif_ul_resume_cb callback to restart UL\n",
                func_name, ipc4_status, pdev->last_sdma_err_msg);
            
            // verify that the number accepted is the same as what we requested
            if ((long) num_sdma_accepted < pdev->tx_sdma_packet_cnt)
            {
                // we need to reset our counters to reflect the correct number of packets in this transaction
                AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                    "%s - WARNING: SDMA UL accepted fewer packets than requested. requested:%d; accepted:%d\n",
                    func_name, (int) pdev->tx_sdma_packet_cnt, (int) num_sdma_accepted);
                
                // reset the counts for both the packet and xfer arrays
                pdev->tx_sdma_packet_cnt = (long) num_sdma_accepted;
                pdev->tx_sdma_xfer_cnt = (long) num_sdma_accepted;
            }
            
            pdev->tx_packet_cnt += (unsigned long) num_sdma_accepted;
            
            spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
            
            // set the throttle flag
            pdev->dd_flags |= LTE_DD_FLAG_TX_THROTTLE;
            pdev->tx_throttle_close_cnt++;
            pdev->tx_data_throttle_start_time = jiffies;
            
            // only clear the busy flags if none were accepted
            if (num_sdma_accepted == 0)
            {
                // clear the busy flags
                pdev->dd_flags &= ~LTE_DD_FLAG_TX_BUSY;
                pni->ni_flags &= ~LTE_NI_FLAG_TX_BUSY;
            }
            
#if defined (LTE_DD_TSL_PROFILE)
            // get the next available index and prime it with special event signature
            tsl_get_entry_index_autoinc(pdev->ptsl_ul, pni->eps_id,
                (void*) UL_TSL_EVENT_4GHIF_DMA_THROTTLE_CLOSE, &entry_index);

            // set the time stamp for this event
            tsl_add_time_stamp(pdev->ptsl_ul, entry_index, UL_TSL_TSNDX_EVENT);
#endif
            
            spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
            
            break;
        
        // for these cases, simply drop the packets since no callback will be invoked
        case IPC_4GHIF_STATUS_BP_NOT_READY:     // BP did not reported yet it is ready
        case IPC_4GHIF_STATUS_CLOSED:           // API is already closed
        default:
        
            // drop these packets
            for (i = 0; i < (int) pdev->tx_sdma_xfer_cnt; i++)
            {
                // remove this packet from the FIFO and release it
                pfval = ptr_fifo_remove(pni->h_tx_data_fifo, (void**) &pskb);
                if (pfval == PTR_FIFO_ERROR_NONE)
                {
                    int fifo_count = 0;
                    
                    AMP_LOG_Error(
                        "%s - ERROR: ipc4_status:%d(%s); dropping packet:%p; packet size:%d\n",
                        func_name, ipc4_status, pdev->last_sdma_err_msg, pskb, pskb->len);
                    
                    ptr_fifo_num_elements(pni->h_tx_data_fifo, &fifo_count);
                    pni->tx_data_packet_read_fifo_cnt++;
                    pni->net_drv_bus_interface.tx_complete(pni->pnet, pdev, pskb,
                        LTE_DD_TX_DATA_PACKET_FIFO_SIZE, (long) fifo_count);
                }
            }
            
            // reset our "tx sdma packet" and "tx sdma xfer" arrays
            lte_dd_zero_tx_arrays(pdev);
            
            spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
            
            // clear the busy flags
            pdev->dd_flags &= ~LTE_DD_FLAG_TX_BUSY;
            pni->ni_flags &= ~LTE_NI_FLAG_TX_BUSY;
            
            spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
            
            break;
            
    }

    
    return AMP_ERROR_NONE;
}


/**
 * Starts a RX SDMA transfer (DL)
 *
 * @param lte_dd_dev_t* pdev
 * @param packet_info frame_info[IPC_4GHIF_MAX_NB_DL_FRAMES] - frame info array
 * @param long frame_info_cnt - number of populated items in frame
 *
 * @return long, error code
 */ 
static long lte_dd_sdma_rx(lte_dd_dev_t* pdev, packet_info pi[IPC_4GHIF_MAX_NB_DL_FRAMES], long pi_cnt)
{
    char* func_name = "lte_dd_sdma_rx";
    long retval = AMP_ERROR_NONE;
    int i = 0;
    struct sk_buff* pskb = NULL;
    u8 num_sdma_accepted = 0;
    lte_dd_net_info_t* pni = NULL;
    lte_dd_sdma_xfer_t* psdma_xfer = NULL;
    IPC_4GHIF_STATUS ipc4_status = IPC_4GHIF_STATUS_OK;
    unsigned long entry_index = 0;
    unsigned long sl_flags = 0;


   
    spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
    
    // check to make sure that we are not already busy
    if (pdev->dd_flags & LTE_DD_FLAG_RX_BUSY || pi_cnt == 0)
    {
        spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
        return AMP_ERROR_NONE;
    }
    
    // set the busy flag, this flag is cleared in the
    // 4GHIF_ul_complete callback
    pdev->dd_flags |= LTE_DD_FLAG_RX_BUSY;
    pdev->rx_data_busy_start_time = jiffies;
  
    spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
    
    // get the persistent interface which is lte0. all DL traffic is routed to this interface
    pni = lte_dd_ni_interface_get_persistent(pdev);

    // alloc the necessary skb's and return in xfer list
    retval = lte_dd_alloc_sdma_xfer_list(pdev, pni, pi, pi_cnt,
        pdev->rx_sdma_xfer, &(pdev->rx_sdma_xfer_cnt));
    if (retval != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: allocating skb, fatal error\n",
            func_name);
        
        spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
        
        pdev->dd_flags &= ~LTE_DD_FLAG_RX_BUSY;
        pdev->dd_flags |= LTE_DD_FLAG_RX_AVAIL;
        
        spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
        
        return AMP_ERROR_LTEDD_RESOURCE_ALLOC;
    }
    
    // now construct an array of "sdma_packet" pointers for the 4GHIF API
    pdev->rx_sdma_packet_cnt = pdev->rx_sdma_xfer_cnt;
    memset(pdev->rx_sdma_packet, 0x00, sizeof(pdev->rx_sdma_packet));
    for (i = 0 ; i < (int) pdev->rx_sdma_xfer_cnt; i++)
    {
        pdev->rx_sdma_packet[i] = &(pdev->rx_sdma_xfer[i].sdma);
        pskb = pdev->rx_sdma_xfer[i].pskb;
        
#if defined (LTE_DD_TSL_PROFILE)
        // get the next available index and prime it
        tsl_get_entry_index_autoinc(pdev->ptsl_dl, pskb->len, pskb->data, &entry_index);

        // now save this index in the SKB so that it may be used for profile logging
        lte_dd_set_skb_profile_index(pskb, entry_index);

        // add the submit time stamp
        tsl_add_time_stamp(pdev->ptsl_dl, entry_index, DL_TSL_TSNDX_SUBMIT);
#endif

#if defined (OMAP_DM_PROFILE)
        omap_dm_timer_log_evt(OMAP_DM_TIMER_DEBUG_ID('l', 't', 'e', '6'), (u32) pskb);
#endif
    }
 
    // save the rx submit start time
    pdev->rx_kt_submit = ktime_get();

    // call the SDMA API to recv the packet(s)
    ipc4_status = IPC_4GHIF_submit_dl((u8) pdev->rx_sdma_packet_cnt, pdev->rx_sdma_packet, &num_sdma_accepted);
    pdev->rx_data_packets_sdma_submit++;
    
    // cache the last error code if an error
    if (ipc4_status != IPC_4GHIF_STATUS_OK)
        IPC_4GHIF_get_error_string(ipc4_status, pdev->last_sdma_err_msg, sizeof(pdev->last_sdma_err_msg));

    switch (ipc4_status)
    {
        case IPC_4GHIF_STATUS_OK:

            // verify that the number accepted is the same as what we requested
            if ((long) num_sdma_accepted < pdev->rx_sdma_packet_cnt)
            {
                // we must delete the skbufs that were not accepted, they should be picked up the
                // next go around. we also update the array elements for these freed skb's
                AMP_LOG_Error(
                    "%s - WARNING: SDMA DL accepted fewer packets than requested. requested:%d; accepted:%d\n",
                    func_name, (int) pdev->rx_sdma_packet_cnt, (int) num_sdma_accepted);
                
                for (i = num_sdma_accepted; i < (int) pdev->rx_sdma_xfer_cnt; i++)
                {
                    psdma_xfer = &(pdev->rx_sdma_xfer[i]); 
                    lte_dd_free_sdma_xfer(psdma_xfer);
                    pdev->rx_sdma_packet[i] = 0;
                }
                
                // reset the counts for both the packet and xfer arrays
                pdev->rx_sdma_packet_cnt = (long) num_sdma_accepted;
                pdev->rx_sdma_xfer_cnt = (long) num_sdma_accepted;
                
                // this condition should never occur but if it does, then clear the RX
                // busy flag because a DL complete will never occur
                if (num_sdma_accepted == 0)
                {
                     AMP_LOG_Error(
                        "%s - ERROR: SDMA UL reported 0 accepted when %d was requested\n",
                        func_name, (int) pdev->rx_sdma_packet_cnt);
                    
                    spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
                    
                    pdev->dd_flags &= ~LTE_DD_FLAG_RX_BUSY;
                    
                    spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
                }
                
            }
            
            break;
        
        
        // for these cases, free the skb
        case IPC_4GHIF_STATUS_BUSY:             // previous DL transferred not ended
        case IPC_4GHIF_STATUS_INVALID_PARAM:    // NULL pointer, etc
        case IPC_4GHIF_STATUS_NO_FRAME:         // No DL frame available
        case IPC_4GHIF_STATUS_BP_NOT_READY:     // BP did not reported yet it is ready
        case IPC_4GHIF_STATUS_CLOSED:           // API is already closed
        default:
        
            // free the skb's and reset the arrays
            for (i = 0; i < (int) pdev->rx_sdma_xfer_cnt; i++)
            {
                psdma_xfer = &(pdev->rx_sdma_xfer[i]);
                
                AMP_LOG_Error(
                    "%s - ERROR: ipc4_status:%d(%s); dropping packet:%p; packet size:%d\n",
                    func_name, ipc4_status, pdev->last_sdma_err_msg, psdma_xfer->pskb, psdma_xfer->sdma.buf_size);
                
                lte_dd_free_sdma_xfer(psdma_xfer);
            }
            
            // reset our "rx sdma packet" and "rx sdma xfer" arrays
            lte_dd_zero_rx_arrays(pdev);
            
            spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
            
            pdev->dd_flags &= ~LTE_DD_FLAG_RX_BUSY;
            
            spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
            
            break;            
    }
    
    
    return AMP_ERROR_NONE;
}


/**
 * UL and DL watchdog timer.  Used to monitir UL and DL traffic and check
 *  for missing DMA interrupts or stalled conditions
 *
 * @param unsigned long ptr
 *
 * @return void
 */ 
static void ul_dl_watchdog_func(unsigned long ptr)
{
    char* func_name = "ul_dl_watchdog_func";
    IPC_4GHIF_STATUS ipc4_status = IPC_4GHIF_STATUS_OK;
    lte_dd_dev_t* pdev = g_pdev;
    unsigned long delta_time = 0;
    int restart_tx = 0;
    int restart_rx = 0;
    unsigned long sl_flags = 0;
    int i = 0;
    lte_dd_sdma_xfer_t* psdma_xfer = NULL;
    packet_info rx_pi[IPC_4GHIF_MAX_NB_DL_FRAMES];
    u8 rx_pi_cnt = 0;
    
    
    if (pdev)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
            "%s - fired;\n", func_name);
        
        spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
        
        // check TX SDMA timeout
        if (pdev->dd_flags & LTE_DD_FLAG_TX_BUSY)
        {
            delta_time = delta_time_jiffies(pdev->tx_data_busy_start_time);
            if (delta_time >= TX_BUSY_TIMEOUT)
            {
                // if a timeout, then call the UL clear function
                // and then clear the TX_BUSY
                
                AMP_LOG_Error(
                    "%s - ERROR: UL DMA has stalled, calling SDMA cancel_ul - seconds:%ld\n",
                    func_name, delta_time / HZ);
                
                IPC_4GHIF_cancel_ul();
                pdev->dd_flags &= ~LTE_DD_FLAG_TX_BUSY;
                restart_tx = 1;
            }
        }
        
        // check RX SDMA timeout
        if (pdev->dd_flags & LTE_DD_FLAG_RX_BUSY)
        {
            delta_time = delta_time_jiffies(pdev->rx_data_busy_start_time);
            if (delta_time >= RX_BUSY_TIMEOUT)
            {
                // if a timeout, then call the UL clear function
                // and then clear the RX_BUSY
                // we assume that the UL complete function will never be called at this point
                
                AMP_LOG_Error(
                    "%s - ERROR: DL DMA has stalled, calling SDMA cancel_dl - seconds:%ld\n",
                    func_name, delta_time / HZ);
                
                IPC_4GHIF_cancel_dl();

                AMP_LOG_Error(
                    "%s - ERROR: DL DMA has stalled, dropping all packets in current DL transaction\n",
                    func_name);
                    
                // free all of the skb's in the current transaction
                for (i = 0; i < pdev->rx_sdma_xfer_cnt; i++)
                {
                    psdma_xfer = &(pdev->rx_sdma_xfer[i]);
                     
                    if (psdma_xfer->pskb)
                        dev_kfree_skb_any(psdma_xfer->pskb);
                }
                
                // reset our "rx sdma packet" and "rx sdma xfer" arrays
                lte_dd_zero_rx_arrays(pdev);

                pdev->dd_flags &= ~LTE_DD_FLAG_RX_BUSY;
                restart_rx = 1;
            }
        }
        else
        {
            // if not a timeout, then check for RX_AVAIL.
            // this condition can occur if:
            //      1. a skb alloc fails.
            //      2. RX throttle was cleared
                
            // if we are not throttled, and RX is available
            if (!(pdev->dd_flags & LTE_DD_FLAG_RX_THROTTLE) &&
                 (pdev->dd_flags & LTE_DD_FLAG_RX_AVAIL))
            {
                AMP_LOG_Error(
                    "%s - WARNING: DL available, will restart SDMA DL\n", func_name);
                restart_rx = 1;
            }
        }
        
        spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);

        // do we need to restart UL?
        if (restart_tx)
        {
            AMP_LOG_Info(
                "%s - UL packet retry, restarting SDMA UL\n", func_name);
            lte_dd_sdma_tx(pdev);
        }
        
        // do we need to restart DL?
        if (restart_rx)
        {
            AMP_LOG_Info(
                "%s - DL packet retry, restarting SDMA DL\n", func_name);

            ipc4_status = IPC_4GHIF_get_dl_frames_nb(&rx_pi_cnt, rx_pi);
            switch (ipc4_status)
            {
                case IPC_4GHIF_STATUS_OK:
                    if (rx_pi_cnt > 0)
                    {
                        // if not in multi packet mode, then limit the count to 1
                        if (!(pdev->dd_flags & LTE_DD_FLAG_MULTI_PACKET))
                            rx_pi_cnt = 1;
                            
                        // if more than one packet, keep count the number of times
                        // this condition occurs
                        if (rx_pi_cnt > 1)
                            pdev->rx_data_packets_multi_avail++;

                        lte_dd_sdma_rx(pdev, rx_pi, rx_pi_cnt);
                    }
                    else
                    {
                        AMP_LOG_Info(
                            "%s - No DL packets available for SDMA DL restart\n", func_name);
                    }
                    break;
                    
                    
                default:
                    AMP_LOG_Error(
                        "%s - ERROR: Function IPC_4GHIF_get_dl_frames_nb returned error:%d\n",
                        func_name, ipc4_status);
                    break;
            }
        }
        
        // reschedule our watchdog timer
        pdev->ul_dl_wd_timer.expires = jiffies + UL_DL_WATCHDOG_TIMER_INTERVAL;
        add_timer(&(pdev->ul_dl_wd_timer));
    }
    
    return;
}


/**
 * 4GHIF IPC callback - BP ready.  Called when BP 4GHIF IPC is ready. 
 *  This callback may be invoked in response to a call to IPC_4GHIF_open() OR
 *  when an eIPC_MSG_TYPE_INIT_REQ message is received by the API
 *
 * @param void
 *
 * @return void
 */
static void lte_dd_4ghif_bp_ready_cb(void)
{
    char* func_name = "lte_dd_4ghif_bp_ready_cb";


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - enter\n", func_name);
                
    // now signal to the 4GHIF API that the AP (this driver) is ready
    IPC_4GHIF_ready();

    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - leave\n", func_name);
}


/**
 * 4GHIF IPC callback - Receive DL.  Called when new DL frame(s) from BP was received
 *
 * @param u8 packet_nb - number of packets in frame info
 * @param packet_info frame_info - frame info array
 *
 * @return void
 */
static void lte_dd_4ghif_receive_dl_cb(u8 packet_nb, packet_info frame_info[])
{
    char* func_name = "lte_dd_4ghif_receive_dl_cb";
    lte_dd_dev_t* pdev = g_pdev;
    packet_info rx_pi[IPC_4GHIF_MAX_NB_DL_FRAMES];
    u8 rx_pi_cnt = 0;
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - enter: packet_nb:%d\n", func_name, (int) packet_nb);

    if (pdev)
    {
        if (packet_nb > 0)
        {
            // if not in multi packet mode, then limit the count to 1
            if (!(pdev->dd_flags & LTE_DD_FLAG_MULTI_PACKET))
                packet_nb = 1;
        
            // if more than one packet, keep count of the number of times this condition occurs
            if (packet_nb > 1)
                pdev->rx_data_packets_multi_avail++;

            rx_pi_cnt = min((int) packet_nb, (int) (sizeof(rx_pi) / sizeof(packet_info)));
            memset(rx_pi, 0x00, sizeof(rx_pi));
            memcpy(rx_pi, frame_info, rx_pi_cnt * sizeof(packet_info));

            pdev->rx_data_packets_sdma_notify++;
            lte_dd_sdma_rx(pdev, rx_pi, rx_pi_cnt);
        }
        else
        {
            AMP_LOG_Error(
                "%s - ERROR: Received notification that DL packets are available, but the count is:%d\n",
                func_name, packet_nb);
        }
    }
    
        
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - leave\n", func_name);
}


/**
 * 4GHIF IPC callback - DL complete.  Called when a DL frame has been transferred from
 *  shared memory to user skb via DMA
 *
 * @param sdma_packet* packet[] - array of pointers that was supplied to "IPC_4GHIF_submit_dl"
 *
 * @return void
 */
static void lte_dd_4ghif_dl_complete_cb(sdma_packet* packet[])
{
    char* func_name = "lte_dd_4ghif_dl_complete_cb";
    int i = 0;
    lte_dd_dev_t* pdev = g_pdev;
    lte_dd_net_info_t* pni = NULL;
    struct sk_buff* pskb_sdma = NULL;
    lte_dd_sdma_xfer_t* psdma_xfer = NULL;
    sdma_packet* psdma_packet = NULL;
    unsigned long entry_index = 0;
    unsigned long sl_flags = 0;
    long netval = 0;
    

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - enter\n", func_name);
    
    // simple pointer checks
    if (!pdev || !packet)
    {
        AMP_LOG_Error(
            "%s - ERROR: invalid pointers - pdev:%p; packet:%p\n", func_name, pdev, packet);
        return;
    }
    
    pdev->rx_data_packets_sdma_ack++;
    
    // calculate RX statistics
    pdev->rx_kt_ack = ktime_get();
    pdev->rx_kt_delta = ktime_sub(pdev->rx_kt_ack, pdev->rx_kt_submit);
    pdev->rx_kt_delta_min.tv64 = min(pdev->rx_kt_delta.tv64, pdev->rx_kt_delta_min.tv64);
    pdev->rx_kt_delta_max.tv64 = max(pdev->rx_kt_delta.tv64, pdev->rx_kt_delta_max.tv64);

    // now caclulate the delta between the last TX submit and this RX ack, this is only useful
    // when performing pings using the loopback BP
    pdev->txrx_kt_delta = ktime_sub(pdev->rx_kt_ack, pdev->tx_kt_submit);
    pdev->txrx_kt_delta_min.tv64 = min(pdev->txrx_kt_delta.tv64, pdev->txrx_kt_delta_min.tv64);
    pdev->txrx_kt_delta_max.tv64 = max(pdev->txrx_kt_delta.tv64, pdev->txrx_kt_delta_max.tv64);

    // verify that this is the same rx sdma packet array that was supplied to the UL
    if (packet != pdev->rx_sdma_packet)
    {
        AMP_LOG_Error(
            "%s - ERROR: rx data packet array:%p is invalid\n", func_name, packet);
        return;
    }
    
    // grab our spin lock 
    spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
 
    // if we are not busy, then most likely spurious callback
    if (!(pdev->dd_flags & LTE_DD_FLAG_RX_BUSY))
    {
        AMP_LOG_Error(
            "%s - ERROR: RX_BUSY not set\n", func_name);
        spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
        return;
    }
    
    // release the spin lock
    spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
    
    // keep track of DL packets received
    pdev->rx_packet_cnt += (unsigned long) pdev->rx_sdma_packet_cnt;
    
    // this callback is provided the same array of pointers that was passed to the "IPC_4GHIF_submit_dl"
    // function.  for each "sdma_packet" pointer, the puser filed contains a pointer to a unique
    // lte_dd_sdma_xfer_t structure that contains the network interface as well as the skb for this
    // SDMA packet transfer
    for (i = 0; i < (int) pdev->rx_sdma_packet_cnt; i++)
    {
        psdma_packet = packet[i];
        if (psdma_packet)
        {         
            // verify that the DL complete flags is set
            if (!psdma_packet->transferred_status)
            {
                AMP_LOG_Error(
                    "%s - WARNING: psdma_packet:%p transferred_status does not indicate DL complete; status:%d\n",
                    func_name, psdma_packet, (int) psdma_packet->transferred_status);
                
                // frees the skb and reset the structure
                lte_dd_free_sdma_xfer(psdma_xfer);
                
                continue;
            } 

            // track the number of bytes
            pdev->rx_data_packets_sdma_bytes += psdma_packet->buf_size;
            
            // get pointer to the unique lte_dd_sdma_xfer_t structure to access the
            // pni and pskb fields
            psdma_xfer = (lte_dd_sdma_xfer_t*) psdma_packet->puser;
            pni = psdma_xfer->pni;
            pskb_sdma = psdma_xfer->pskb;
        
            // if not NULL, then forward to the network stack
            if (pskb_sdma != NULL)
            {
                // make sure that the size reported matches the skb length
                if (psdma_packet->buf_size != pskb_sdma->len)
                {
                    AMP_LOG_Error(
                        "%s - ERROR: DMA buffer size:%d does not match skbuf size:%d. Dropping DL skb:%p\n",
                        func_name, psdma_packet->buf_size, pskb_sdma->len, pskb_sdma);
                    
                    // frees the skb and reset the structure
                    lte_dd_free_sdma_xfer(psdma_xfer);
                    
                    continue;
                }
                
                AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                    "%s - received packet type:%d; packet len:%d; pskb len:%d;\n",
                    func_name, psdma_packet->packet_type, psdma_packet->buf_size, pskb_sdma->len);

                print_buf("lte_dd_4ghif_dl_complete_cb\n", pskb_sdma->data, pskb_sdma->len, 0);

#if defined (LTE_DD_TSL_PROFILE)
                // fetch the index TSL entry index that was saved with this skb
                entry_index = lte_dd_get_skb_profile_index(pskb_sdma);

                // add the submit time stamp
                tsl_add_time_stamp(pdev->ptsl_dl, entry_index, DL_TSL_TSNDX_COMPLETE);
#endif

#if defined (OMAP_DM_PROFILE)
                omap_dm_timer_log_evt(OMAP_DM_TIMER_DEBUG_ID('l', 't', 'e', '7'), (u32) pskb_sdma);
#endif

                AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
                    "%s - DL packet received - packet type:%d; len:%d; eps id:%d\n",
                    func_name, (int) psdma_packet->packet_type, (int) psdma_packet->buf_size, (int) psdma_packet->bearerID);
                            
                // now pass this SKB to the net net driver if the packet type is data or mon
                netval = 0;
                switch(psdma_packet->packet_type)
                {
                    case PKT_TYPE_DL_DATA:
                    
                        // if loopback mode is enabled, then generate ping reply if possible
                        if (pdev->dd_flags & LTE_DD_FLAG_LPBP)
                            generate_loopback_reply(pskb_sdma->data, pskb_sdma->len);

                        netval = pni->net_drv_bus_interface.rx_complete(pni->pnet, pdev, pskb_sdma,
                            (unsigned char) psdma_packet->bearerID, 0);
                        
                        break;

                    case PKT_TYPE_DL_MON:
                    
                        netval = pni->net_drv_bus_interface.rx_complete(pni->pnet, pdev, pskb_sdma,
                            (unsigned char) psdma_packet->bearerID, 1);
                        
                        break;
                        
                    default:
                    
                        AMP_LOG_Error(
                            "%s - ERROR: received packet type:%d; len:%d; invalid for data driver, dropping packet\n",
                            func_name, psdma_packet->packet_type, psdma_packet->buf_size);
                            
                        // frees the skb and reset the structure
                        lte_dd_free_sdma_xfer(psdma_xfer);
                        
                        break;
                }
                
                // keep stats of dropped RX packets
                if (netval == AMP_ERROR_NET_DRV_PACKET_DROPPED)
                    pdev->rx_net_rx_drop_cnt++;
                     
            }
        }
    }


    // reset our "rx sdma packet" and "rx sdma xfer" arrays
    lte_dd_zero_rx_arrays(pdev);
    
    // grab our spin lock 
    spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
 
    // clear our busy flag
    pdev->dd_flags &= ~LTE_DD_FLAG_RX_BUSY;
   
    // release the spin lock
    spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - leave\n", func_name);
}


/**
 * 4GHIF IPC callback - UL complete.  Called when a UL frame has been transferred
 *  from user skb to shared memory.  It is safe to call IPC_4GHIF_submit_ul from
 *  within this call
 *
 * @param sdma_packet* packet[] - array of pointers that was supplied to "IPC_4GHIF_submit_ul"
 *
 * @return void
 */
static void lte_dd_4ghif_ul_complete_cb(sdma_packet* packet[])
{
    char* func_name = "lte_dd_4ghif_ul_complete_cb";
    int i = 0;
    lte_dd_dev_t* pdev = g_pdev;
    lte_dd_net_info_t* pni = NULL;
    struct sk_buff* pskb_sdma = NULL;
    struct sk_buff* pskb_fifo = NULL;
    long pfval = PTR_FIFO_ERROR_NONE;
    lte_dd_sdma_xfer_t* psdma_xfer = NULL;
    sdma_packet* psdma_packet = NULL;
    unsigned long entry_index = 0;
    unsigned long sl_flags = 0;
    

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - enter\n", func_name);

    // simple pointer checks
    if (!pdev || !packet)
    {
        AMP_LOG_Error(
            "%s - ERROR: invalid pointers - pdev:%p; packet:%p\n", func_name, pdev, packet);
        return;
    }
    
    pdev->tx_data_packets_sdma_ack++;

    // calculate TX statistics
    pdev->tx_kt_ack = ktime_get();
    pdev->tx_kt_delta = ktime_sub(pdev->tx_kt_ack, pdev->tx_kt_submit);
    pdev->tx_kt_delta_min.tv64 = min(pdev->tx_kt_delta.tv64, pdev->tx_kt_delta_min.tv64);
    pdev->tx_kt_delta_max.tv64 = max(pdev->tx_kt_delta.tv64, pdev->tx_kt_delta_max.tv64);


    // verify that this is the same tx sdma packet array that was supplied to the UL
    if (packet != pdev->tx_sdma_packet)
    {
        AMP_LOG_Error(
            "%s - ERROR: tx data packet array:%p is invalid\n", func_name, packet);
        return;
    }
    
    // grab our spin lock 
    spin_lock_irqsave(&(pdev->sl_lock), sl_flags);

    // if we are not busy, then most likely spurious callback
    if (!(pdev->dd_flags & LTE_DD_FLAG_TX_BUSY))
    {
        AMP_LOG_Error(
            "%s - ERROR: TX_BUSY not set\n", func_name);
        spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
        return;
    }
    
    // release the spin lock
    spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
    
    // this callback is provided the same array of pointers that was passed to the "IPC_4GHIF_submit_ul"
    // function.  for each "sdma_packet" pointer, the puser filed contains a pointer to a unique
    // lte_dd_sdma_xfer_t structure that contains the network interface as well as the skb for this
    // SDMA packet transfer
    for (i = 0; i < pdev->tx_sdma_packet_cnt; i++)
    {
        psdma_packet = packet[i];
        if (psdma_packet)
        {
            // verify that the UL complete flags is set
            if (!psdma_packet->transferred_status)
            {
                AMP_LOG_Error(
                    "%s - WARNING: psdma_packet:%p transferred_status does not indicate UL complete; status:%d\n",
                    func_name, psdma_packet, (int) psdma_packet->transferred_status);
            } 
            
            pdev->tx_data_packets_sdma_bytes += psdma_packet->buf_size;
            
            // get pointer to the unique lte_dd_sdma_xfer_t structure to access the
            // pni and pskb fields
            psdma_xfer = (lte_dd_sdma_xfer_t*) psdma_packet->puser;
            pni = psdma_xfer->pni;
            pskb_sdma = psdma_xfer->pskb;

            // if not NULL, check for FIFO match    
            if (pskb_sdma != NULL)
            {
                int fifo_count = 0;


#if defined (LTE_DD_TSL_PROFILE)
                // fetch the index TSL entry index that was saved with this skb
                entry_index = lte_dd_get_skb_profile_index(pskb_sdma);

                // add the submit time stamp
                tsl_add_time_stamp(pdev->ptsl_ul, entry_index, UL_TSL_TSNDX_COMPLETE);
#endif
                
#if defined (OMAP_DM_PROFILE)
                omap_dm_timer_log_evt(OMAP_DM_TIMER_DEBUG_ID('l', 't', 'e', '5'), (u32) pskb_sdma);
#endif
        
                // now verify that the packet that was just submitted to DMA
                // is the same packet that is specified in this callback.  If
                // so, then remove from the FIFO
                pfval = ptr_fifo_peek_offset(pni->h_tx_data_fifo, 0, (void**) &pskb_fifo);
                if (pfval == PTR_FIFO_ERROR_NONE)
                {
                    // if the same, then remove
                    if (pskb_sdma == pskb_fifo)
                    {
                        pfval = ptr_fifo_remove(pni->h_tx_data_fifo, (void**) &pskb_fifo);
                        pni->tx_data_packet_read_fifo_cnt++;
                    }
                    else
                    {
                        AMP_LOG_Error(
                            "%s - WARNING: pskb_sdma:%p is not the packet we expected:%p for UL complete, psdma_xfer:%p\n",
                            func_name, pskb_sdma, pskb_fifo, psdma_xfer);
                    }
                }
                
                // release the skb that was supplied in the callback
                ptr_fifo_num_elements(pni->h_tx_data_fifo, &fifo_count);
                pni->net_drv_bus_interface.tx_complete(pni->pnet, pdev, pskb_sdma,
                    LTE_DD_TX_DATA_PACKET_FIFO_SIZE, (long) fifo_count);
            }
        }
    }
   
    // reset our "tx sdma packet" and "tx sdma xfer" arrays
    lte_dd_zero_tx_arrays(pdev);
    
    // grab our spin lock
    spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
    
    // clear our busy flags
    pdev->dd_flags &= ~LTE_DD_FLAG_TX_BUSY;

    /* The pni pointer should have been set in the for loop. */
    if (NULL != pni)
    {
        pni->ni_flags &= ~LTE_NI_FLAG_TX_BUSY;
    }

    // release the spin lock
    spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
    
    // attempt a new SDMA UL
    lte_dd_sdma_tx(pdev);
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - leave\n", func_name);
        
}


/**
 * 4GHIF IPC callback - Resume.  Called when resources are available again after
 *  UL fails due to FIFO full, etc
 *
 * @param void
 *
 * @return void
 */
static void lte_dd_4ghif_ul_resume_cb(void)
{
    char* func_name = "lte_dd_4ghif_ul_resume_cb";
    lte_dd_dev_t* pdev = g_pdev;
    unsigned long entry_index = 0;
    unsigned long sl_flags = 0;

   
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - enter\n", func_name);
    
    // retry a UL
    if (pdev)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
            "%s - enter: Resume CB invoked, restarting SDMA TX\n", func_name);

        // clear the throttle flag first
        spin_lock_irqsave(&(pdev->sl_lock), sl_flags);

        pdev->dd_flags &= ~LTE_DD_FLAG_TX_THROTTLE;
        pdev->tx_throttle_open_cnt++;

#if defined (LTE_DD_TSL_PROFILE)
        // get the next available index and prime it with special event signature
        tsl_get_entry_index_autoinc(pdev->ptsl_ul, 0,
            (void*) UL_TSL_EVENT_4GHIF_DMA_THROTTLE_OPEN, &entry_index);

        // set the time stamp for this event
        tsl_add_time_stamp(pdev->ptsl_ul, entry_index, UL_TSL_TSNDX_EVENT);
#endif

        spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
        
        // restart sdma tx
        lte_dd_sdma_tx(pdev);
    }
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG3,
        "%s - leave\n", func_name);

}


/**
 * Flushes the tx data fifo
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return long, error code
 */ 
static long lte_dd_flush_tx_data_fifo(lte_dd_net_info_t* pni)
{
    char* func_name = "lte_dd_flush_tx_data_fifo";
    struct sk_buff* pskb = NULL;
    long pfval = PTR_FIFO_ERROR_NONE;
    
              
    // drain the tx data fifo
    while(1)
    {
        pfval = ptr_fifo_remove(pni->h_tx_data_fifo, (void**) &pskb);
        if (pfval != PTR_FIFO_ERROR_NONE)
            break;
       
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
            "%s - deleting pskp:%p\n", func_name, pskb);
        
        if (pskb)
            dev_kfree_skb_any(pskb);
    }
    
    pni->tx_data_packet_write_fifo_cnt = 0;
    pni->tx_data_packet_read_fifo_cnt = 0;

    return AMP_ERROR_NONE;
}


/**
 * char device seek method (register function)
 *
 * @param struct file* filp
 * @param loff_t off
 * @param int whence
 *
 * @return int, error code
 */ 
static loff_t lte_dd_dev_llseek(struct file* filp, loff_t off, int whence)
{
    char* func_name = "lte_dd_dev_llseek";
    //lte_dd_dev_t* pdev = (lte_dd_dev_t*) filp->private_data;
    loff_t newpos = 0;


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - filp:%p; off:%d; whence:%d\n",
        func_name, filp, (int) off, whence);
    
    switch(whence)
    {
        case 0: // SEEK_SET
            break;

        case 1: // SEEK_CUR
            break;

        case 2: // SEEK_END
            break;

        default: // can't happen
            return -EINVAL;
    }
    
    if (newpos < 0)
        return -EINVAL;
        
    filp->f_pos = newpos;

    return newpos;
}


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
static ssize_t lte_dd_dev_read(struct file* filp, char __user * buf, size_t count, loff_t* f_pos)
{
    char* func_name = "lte_dd_dev_read";
    lte_dd_dev_t* pdev = (lte_dd_dev_t*) filp->private_data;


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - filp:%p; buf:%p; count:%d; f_pos:%p; pdev:%p\n",
        func_name, filp, buf, (int) count, f_pos, pdev);
    
    
    return 0;
}


/**
 * char device write method (register function).
 *  This function writes either TX mgmt or TX data to the RX fifos.
 *  This function receives a payload that consists of [hif_header_drv][data]
 *  The only difference between the hif_header_drv and a hif_header is that the
 *  driver version is more robust in the sense that it has a signature that is
 *  checked
 *
 * @param struct file* filp
 * @param char __user * buf
 * @param size_t count
 * @param loff_t* f_pos
 *
 * @return ssize_t, bytes written
 */ 
static ssize_t lte_dd_dev_write(struct file* filp, const char __user * buf, size_t count, loff_t* f_pos)
{
    char* func_name = "lte_dd_dev_write";
    lte_dd_dev_t* pdev = (lte_dd_dev_t*) filp->private_data;

    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - filp:%p; buf:%p; count:%d; f_pos:%p; pdev:%p\n",
        func_name, filp, buf, (int) count, f_pos, pdev); 


    return 0; 
}


/**
 * char device ioctl method (register function)
 *
 * @param struct inode* inode
 * @param struct file* filp
 * @param unsigned int cmd
 * @param unsigned long arg
 *
 * @return int, error code
 */
static int lte_dd_dev_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
    char* func_name = "lte_dd_dev_ioctl";
    //lte_dd_dev_t* pdev = (lte_dd_dev_t*) filp->private_data;
    int ret = 0;

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - inode:%p; filp:%p; cmd:%d; arg:%d\n",
        func_name, inode, filp, (int) cmd, (int) arg); 
 
 
    return ret;
}


/**
 * char device open method (register function)
 *
 * @param struct inode* inode
 * @param struct file* filp
 *
 * @return int, error code
 */
static int lte_dd_dev_open(struct inode* inode, struct file* filp)
{
    char* func_name = "lte_dd_dev_open";
    lte_dd_dev_t* pdev = NULL;

 
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - inode:%p; filp:%p\n", func_name, inode, filp);
    
    // get pointer to our structure
    pdev = (lte_dd_dev_t*) container_of(inode->i_cdev, lte_dd_dev_t, cdev);
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
static int lte_dd_dev_release(struct inode* inode, struct file* filp)
{
    char* func_name = "lte_dd_dev_open";


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - inode:%p; filp:%p\n", func_name, inode, filp);
    

    return 0;
}


/**
 * char device file operations (register table)
 *
 */
struct file_operations g_lte_dd_fops =
{
    .owner =    THIS_MODULE,
    .llseek =   lte_dd_dev_llseek,
    .read =     lte_dd_dev_read,
    .write =    lte_dd_dev_write,
    .ioctl =    lte_dd_dev_ioctl,
    .open =     lte_dd_dev_open,
    .release =  lte_dd_dev_release,
};


/**
 * LTE data driver BUS function that are supplied to the network driver. This 
 *  is called when the stack has uplink data
 *
 * @param void* pnet
 * @param void* pbus
 * @param struct sk_buff* pskb
 *
 * @return long, error code
 */
static long lte_dd_tx_data_start(void* pnet, void* pbus, struct sk_buff* pskb)
{
    char* func_name = "lte_dd_tx_data_start";
    lte_dd_dev_t* pdev = (lte_dd_dev_t*) pbus;
    long retval = AMP_ERROR_NONE;
    long pfval = PTR_FIFO_ERROR_NONE;
    lte_dd_net_info_t* pni = NULL;
    unsigned long entry_index = 0;

    
    // NOTE: There is no ethernet header, only the IP data
    print_buf("lte_dd_tx_data_start received\n", pskb->data, pskb->len, 0);
    
    // the net driver passes us our object back as the "bus" object
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - received socket buffer - pnet:%p; pbus:%p; pskb:%p len:%d\n",
        func_name, pnet, pbus, pskb, (int) pskb->len);

    // if we return an error, then the stack is notified and will retry
    // at a later time
    if ((retval = lte_dd_private_valid(pdev)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: private data is invalid\n", func_name);
        return retval;
    }

    // check the remaining parameters
    if (!pnet || !pskb)
    {
        AMP_LOG_Error(
            "%s - ERROR: invalid parameters - pnet:%p; pskb:%p\n", func_name, pnet, pskb);
        return AMP_ERROR_LTEDD_INVALID_PARAM;
    }

    // map this pnet object to a net info structure
    pni = lte_dd_ni_interface_find_pnet(pnet);
    if (!pni)
    {
        AMP_LOG_Error(
            "%s - ERROR: can't map net object to net info structure\n", func_name);
        return AMP_ERROR_LTEDD_NET_INFO;
    }

    // insert the pskb into FIFO, after the pskb is removed from the
    // FIFO and actually xmitted, the bus function tx_complete will be called
    // to notify the stack that the skb has been sent.  this takes place
    // in the read function
    pfval = ptr_fifo_insert(pni->h_tx_data_fifo, pskb);
    if (pfval != PTR_FIFO_ERROR_NONE)
        return AMP_ERROR_LTEDD_FIFO_INSERT;
    
#if defined (LTE_DD_TSL_PROFILE)
    // get the next available index and prime it
    tsl_get_entry_index_autoinc(pdev->ptsl_ul, pskb->len, pskb->data, &entry_index);

    // now save this index in the SKB so that it may be used for profile logging
    lte_dd_set_skb_profile_index(pskb, entry_index);

    // add the enqueue time stamp
    tsl_add_time_stamp(pdev->ptsl_ul, entry_index, UL_TSL_TSNDX_ENQUEUE);
#endif

#if defined (OMAP_DM_PROFILE)
    omap_dm_timer_log_evt(OMAP_DM_TIMER_DEBUG_ID('l', 't', 'e', '3'), (u32) pskb);
#endif

    // increment the tx write count
    pni->tx_data_packet_write_fifo_cnt++;

    // attempt SDMA UL 
    lte_dd_sdma_tx(pdev);
    
    
    return AMP_ERROR_NONE;
}


/**
 * LTE data driver BUS function that is supplied to the network driver. This 
 *  is our control function
 *
 * @param void* pnet
 * @param void* pbus
 * @param unsigned long cmd
 * @param unsigned long cmd_data - control command data
 *
 * @return long, error code
 */
static long lte_dd_bus_control(void* pnet, void* pbus, unsigned long cmd, unsigned long cmd_data)
{
    char* func_name = "lte_dd_bus_control";
    lte_dd_dev_t* pdev = (lte_dd_dev_t*) pbus;
    long retval = AMP_ERROR_NONE;
    lte_dd_net_info_t* pni = NULL;
    unsigned long entry_index = 0;
    unsigned long tsl_event = 0;
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - pnet:%p; pbus:%p; cmd:0x%x\n",
        func_name, pnet, pbus, (int) cmd);
    
    // the net driver passes us our object back as the "bus" object
    if ((retval = lte_dd_private_valid(pdev)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: private data is invalid\n", func_name);
        return retval;
    }
    
    // perform operation
    switch(cmd)
    {
        case NETDRV_BUS_CONTROL_SUSPEND:
            // TODO - finish
            break;
        
        case NETDRV_BUS_CONTROL_RESUME:
            // TODO - finish
            break;
            
        case NETDRV_BUS_CONTROL_RESET:
            // TODO - finish
            break;
            
        case NETDRV_BUS_CONTROL_FLUSH:
            // find the net info structure that contains this net object
            pni = lte_dd_ni_interface_find_pnet(pnet);
            if (pni)
            {
                lte_dd_flush_tx_data_fifo(pni);
            }
            break;
            
        case NETDRV_BUS_CONTROL_TX_DRAIN:
        
            // find the net info structure that contains this net object
            pni = lte_dd_ni_interface_find_pnet(pnet);
            
            if (pni)
            {
                pni->tx_drain_cnt++;

#if defined (LTE_DD_TSL_PROFILE)
                // get the next available index and prime it with special event signature
                tsl_get_entry_index_autoinc(pdev->ptsl_ul, pni->eps_id,
                    (void*) UL_TSL_EVENT_LTEDD_TX_DRAIN, &entry_index);

                // set the time stamp for this event
                tsl_add_time_stamp(pdev->ptsl_ul, entry_index, UL_TSL_TSNDX_EVENT);
#endif

                // attempt SDMA UL 
                lte_dd_sdma_tx(pdev);
            }
    
            break;
            
        case NETDRV_BUS_CONTROL_LOG_NETQUEUE_EVENT:
        
            // find the net info structure that contains this net object
            pni = lte_dd_ni_interface_find_pnet(pnet);
            
            if (pni)
            {
                // a non-zero value means to log a close event
                if (cmd_data)
                {
                    tsl_event = UL_TSL_EVENT_NETDRV_THROTTLE_CLOSE;
                    pni->tx_throttle_close_cnt++;
                }
                else
                {
                    tsl_event = UL_TSL_EVENT_NETDRV_THROTTLE_OPEN;
                    pni->tx_throttle_open_cnt++;
                }
                
#if defined (LTE_DD_TSL_PROFILE)
                // get the next available index and prime it with special event signature
                tsl_get_entry_index_autoinc(pdev->ptsl_ul, pni->eps_id,
                    (void*) tsl_event, &entry_index);

                // set the time stamp for this event
                tsl_add_time_stamp(pdev->ptsl_ul, entry_index, UL_TSL_TSNDX_EVENT);
#endif
            }
            
            break;

    }
    
    return AMP_ERROR_NONE;
}


/**
 * Create a network interface and all of the resources associated with it
 *
 * @param pdev - lte data driver object
 * @param ifname - network interface name
 * @param eps_id - network interface eps id
 * @param priority - network interface priority
 * @param pni - ptr to a net info structure
 *
 * @return long, error code
 */
static long lte_dd_create_net_iface(lte_dd_dev_t* pdev, char* ifname, unsigned char eps_id,
    unsigned char priority, lte_dd_net_info_t* pni)
{
    char* func_name = "lte_dd_create_net_iface";
    long retval = AMP_ERROR_NONE;
    long pfval = PTR_FIFO_ERROR_NONE;
    void* pnet = NULL;
    
    
    if (!pdev || !ifname || !pni)
        return AMP_ERROR_LTEDD_INVALID_PARAM;
    
    memset(pni, 0x00, sizeof(lte_dd_net_info_t));
    strncpy(pni->ifname, ifname, NETDRV_MAX_DEV_NAME - 1);
    
    pni->eps_id = eps_id;
    pni->priority = priority;

    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - creating TX FIFO: element size:%d\n", func_name, LTE_DD_TX_DATA_PACKET_FIFO_SIZE);
    
    // create a FIFO object for our TX data packets
    pfval = ptr_fifo_alloc(LTE_DD_TX_DATA_PACKET_FIFO_SIZE, PTR_FIFO_FLAG_NONE, &pni->h_tx_data_fifo);
    if (pfval != PTR_FIFO_ERROR_NONE)
    {
        AMP_LOG_Error(
            "%s - ERROR: error %d while creating TX data FIFO object\n",
            func_name, (int) pfval);
        retval = AMP_ERROR_LTEDD_RESOURCE_ALLOC;
        goto CREATE_FAIL;
    
    }

    // initialize the "bus" functions the data driver is responsible for, the
    // remainder will be initialized by the network driver probe function
    pni->net_drv_bus_interface.tx_start = lte_dd_tx_data_start;
    pni->net_drv_bus_interface.bus_control = lte_dd_bus_control;

    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - creating net device using name:%s\n", func_name, pni->ifname);
    
    // create a new network object using our device as the "bus" object
    net_drv_probe(pni->ifname, pdev, &(pni->net_drv_bus_interface), &pnet);
    if (!pnet)
    {
        AMP_LOG_Error(
            "%s - ERROR: error creating net device:%s\n", func_name, pni->ifname);
        retval = AMP_ERROR_LTEDD_RESOURCE_ALLOC;
        goto CREATE_FAIL;
    }

    // now set the EPS id for the net object. the net driver will prepend this
    // id to each TX packet it sends
    net_drv_set_eps_id(pnet, pni->eps_id);

    // update the net object in the net info structure
    pni->pnet = pnet;

    // keep track of the total count
    pdev->ni_interfaces_cnt++;

    AMP_LOG_Info(
        "%s - successfully created net device - ifname:%s eps_id:%d priority:%d\n",
        func_name, pni->ifname, (int) pni->eps_id, (int) pni->priority);


    return retval;
    
    
CREATE_FAIL:

    // delete the ptr tx data fifo object
    if (pni->h_tx_data_fifo)
    {
        ptr_fifo_free(&pni->h_tx_data_fifo);
    }

    return retval;
}


/**
 * Delete a network interface and all of the resources associated with it
 *
 * @param pdev - lte data driver object
 * @param pni - ptr to a net info structure
 *
 * @return long, error code
 */
static long lte_dd_delete_net_iface(lte_dd_dev_t* pdev, lte_dd_net_info_t* pni)
{
    char* func_name = "lte_dd_delete_net_iface";
    long retval = AMP_ERROR_NONE;
    
    
    if (!pni)
        return AMP_ERROR_LTEDD_INVALID_PARAM;
    
    // suspend the net object, this will stop the net driver queue and we
    // should not receive any additional tx packets
    if (pni->pnet && pni->net_drv_bus_interface.net_control)
        pni->net_drv_bus_interface.net_control(pni->pnet, pdev, NETDRV_NET_CONTROL_SUSPEND, 0);
    
    // flush and then delete the ptr tx data fifo object
    if (pni->h_tx_data_fifo)
    {
        lte_dd_flush_tx_data_fifo(pni);
        ptr_fifo_free(&pni->h_tx_data_fifo);
    }

    // delete the net object
    if (pni->pnet)
    {
        net_drv_disconnect(pni->pnet);
        
        // keep track of the total count
        pdev->ni_interfaces_cnt--;
    }
   
    AMP_LOG_Info(
        "%s - successfully deleted net device - ifname:%s\n",
        func_name, pni->ifname);
    
    // reinitialize this structure
    memset(pni, 0x00, sizeof(lte_dd_net_info_t));
   
    return retval;

}


/**
 * Process command structures - WARNING!! This code is not reentrant
 *
 * @param lte_dd_dev_t* pdev - data driver object
 * @param lte_dd_cmd_t cmd_array - array of command structures
 * @param unsigned long* populated - array elements populated
 *
 * @return long, error code
 */
static long lte_dd_process_dd_cmds(lte_dd_dev_t* pdev, lte_dd_cmd_t cmd_array[MAX_DD_CMD_ARRAY],
    unsigned long populated)
{
    char* func_name = "lte_dd_process_dd_cmds";
    unsigned long i = 0;
    lte_dd_cmd_t* pcmd = NULL;
    lte_dd_net_info_t* pni = NULL;
    long retval = AMP_ERROR_NONE;
    lte_dd_net_info_t ni;
    unsigned long sl_flags = 0;
    static unsigned char badmac[ETH_ALEN] = {0, 0, 0, 0, 0, 0};
    

    AMP_LOG_Info(
        "%s - request to process %d cmds\n", func_name, (int) populated);

    // validate pointers
    if (!pdev)
    {
        AMP_LOG_Error(
            "%s - ERROR: Invalid parameter pdev:%p passed to function\n", func_name, pdev);
        return AMP_ERROR_LTEDD_INVALID_PARAM;
    }
    
    pcmd = &(cmd_array[0]);
    for (i = 0; i < populated; i++)
    {
        switch(pcmd->cmd_id)
        {
            case lte_dd_cmd_net_new:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d ifname:%s eps_id:%d priority:%d\n",
                    func_name,
                    CMD_STR_NET_NEW,
                    (int) pcmd->cmd_id,
                    pcmd->cmd_data.net_new.ifname,
                    (int) pcmd->cmd_data.net_new.eps_id,
                    (int) pcmd->cmd_data.net_new.priority);
                
                        
                // see if this interface already exists
                pni = lte_dd_ni_interface_find_ifname(pcmd->cmd_data.net_new.ifname);
                if (pni)
                {
                    AMP_LOG_Error(
                        "%s - ERROR: Can't create interface:%s because it already exists\n",
                        func_name,
                        pcmd->cmd_data.net_new.ifname);
                }
                else
                {
                    // allocate a new net info for this interface
                    pni =  lte_dd_ni_interface_alloc();
                    if (!pni)
                    {
                        AMP_LOG_Error(
                            "%s - ERROR: Can't create interface:%s - net info alloc failure.  Too many interfaces??\n",
                            func_name,
                            pcmd->cmd_data.net_new.ifname);
                    }
                    else
                    {
                        // now create the interface
                        retval = lte_dd_create_net_iface(pdev, pcmd->cmd_data.net_new.ifname, pcmd->cmd_data.net_new.eps_id,
                            pcmd->cmd_data.net_new.priority, pni);
                        if (retval != AMP_ERROR_NONE)
                        {
                        
                            AMP_LOG_Error(
                                "%s - ERROR: Can't create interface:%s - create interface returned error:0x%x.\n",
                                func_name,
                                pcmd->cmd_data.net_new.ifname,
                                (int) retval);
                            
                            // release this net info structure
                            lte_dd_ni_interface_free(pni);
                        }
                        else
                        {
                            AMP_LOG_Info(
                                "%s - Successfully created interface - ifname:%s eps_id:%d priority:%d \n",
                                func_name,
                                pcmd->cmd_data.net_new.ifname,
                                (int) pcmd->cmd_data.net_new.eps_id,
                                (int) pcmd->cmd_data.net_new.priority);
                        }
                    }
                }
                
                break;
                
            case lte_dd_cmd_net_delete:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d ifname:%s\n",
                    func_name,
                    CMD_STR_NET_DELETE,
                    (int) pcmd->cmd_id,
                    pcmd->cmd_data.net_delete.ifname);
                

                // make sure to reject the persistent interface name which is lte0
                if (strcasecmp(pcmd->cmd_data.net_delete.ifname, LTE_DD_PERSISTENT_IFACE_NAME) == 0)
                {
                    AMP_LOG_Error(
                        "%s - ERROR: Can't delete interface:%s because it is persistent\n",
                        func_name,
                        pcmd->cmd_data.net_delete.ifname);
                }
                else
                {
                    // verify that this interface actually exists
                    pni = lte_dd_ni_interface_find_ifname(pcmd->cmd_data.net_delete.ifname);
                    if (!pni)
                    {
                        AMP_LOG_Error(
                            "%s - ERROR: Can't delete interface:%s because it doesn't exist\n",
                            func_name,
                            pcmd->cmd_data.net_delete.ifname);
                    }
                    else
                    {
                        // grab our spin lock 
                        spin_lock_irqsave(&(pdev->sl_lock), sl_flags);
                
                        // check and see if the interface is currently (BUSY) in an UL SDMA transaction
                        if (pni->ni_flags & LTE_NI_FLAG_TX_BUSY)
                        {
                            AMP_LOG_Error(
                                "%s - ERROR: Can't delete interface:%s - busy with UL traffic, bring down interface then delete.\n",
                                func_name,
                                pcmd->cmd_data.net_delete.ifname);
                                
                            // release the spin lock
                            spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
                        }
                        else
                        {
                            // save a local copy of the net info structure
                            ni = *pni;
                            
                            // set the pnet field in the net info structure to zero, this will remove it
                            // from consideration by the UL scheduler - lte_dd_ni_interface_fifo_lookup()
                            pni->pnet = 0;
                            
                            // release the spin lock
                            spin_unlock_irqrestore(&(pdev->sl_lock), sl_flags);
                
                            // delete what is saved in our local copy
                            retval = lte_dd_delete_net_iface(pdev, &ni);
                            if (retval != AMP_ERROR_NONE)
                            {
                                AMP_LOG_Error(
                                    "%s - ERROR: Can't delete interface:%s - delete interface returned error:0x%x.\n",
                                    func_name,
                                    pcmd->cmd_data.net_delete.ifname,
                                    (int) retval);
                            }
                            
                            // release this net info structure
                            lte_dd_ni_interface_free(pni);
                        }
                    }
                }
                   
                break;
                
            case lte_dd_cmd_net_eps:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d ifname:%s eps_id:%d\n",
                    func_name,
                    CMD_STR_NET_EPS,
                    (int) pcmd->cmd_id,
                    pcmd->cmd_data.net_eps.ifname,
                    (int) pcmd->cmd_data.net_eps.eps_id);
                    
                // verify that this interface exists
                pni = lte_dd_ni_interface_find_ifname(pcmd->cmd_data.net_eps.ifname);
                if (!pni || !pni->pnet)
                {
                    AMP_LOG_Error(
                        "%s - ERROR: Can't set eps id because interface %s doesn't exist\n",
                        func_name,
                        pcmd->cmd_data.net_eps.ifname);
                }
                else
                {
                    // save in our internal net info structure
                    pni->eps_id = pcmd->cmd_data.net_eps.eps_id;   
                    // set on the net object iteself
                    net_drv_set_eps_id(pni->pnet, pni->eps_id);
                }
                    
                break;

            case lte_dd_cmd_net_eps_mac_map_add_dl:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d eps_id:%d MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",
                    func_name,
                    CMD_STR_NET_EPS_MAC_MAP_ADD_DL,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.net_eps_mac_map_add_dl.eps_id,
                    (int) pcmd->cmd_data.net_eps_mac_map_add_dl.mac_addr[0],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_dl.mac_addr[1],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_dl.mac_addr[2],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_dl.mac_addr[3],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_dl.mac_addr[4],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_dl.mac_addr[5]);
                    
                // sanity check the MAC to see if the parser discarded it.
                if (memcmp(pcmd->cmd_data.net_eps_mac_map_add_dl.mac_addr, badmac, ETH_ALEN) == 0)
                {
                    AMP_LOG_Error("%s: Invalid MAC - discarding.\n", func_name);
                }
                else
                {
                    net_drv_set_eps_mac_map(pcmd->cmd_data.net_eps_mac_map_add_dl.eps_id,
                        pcmd->cmd_data.net_eps_mac_map_add_dl.mac_addr);
                }

                break;

            case lte_dd_cmd_net_eps_mac_map_rm_dl:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d eps_id:%d\n",
                    func_name,
                    CMD_STR_NET_EPS_MAC_MAP_RM_DL,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.net_eps_mac_map_rm_dl.eps_id);
                    
                // NULL MAC address signals to reset back to default MAC address
                net_drv_set_eps_mac_map(pcmd->cmd_data.net_eps_mac_map_rm_dl.eps_id,
                        NULL);

                break;

            case lte_dd_cmd_net_bridge_mode_dl:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d enable:%d\n",
                    func_name,
                    CMD_STR_NET_BRIDGE_MODE_DL,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.net_bridge_mode_dl.enable);
                    
                net_drv_set_bridge_mode(pcmd->cmd_data.net_bridge_mode_dl.enable);

                break;

            case lte_dd_cmd_net_eps_mac_map_add_ul:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d eps_id:%d MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",
                    func_name,
                    CMD_STR_NET_EPS_MAC_MAP_ADD_UL,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.net_eps_mac_map_add_ul.eps_id,
                    (int) pcmd->cmd_data.net_eps_mac_map_add_ul.mac_addr[0],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_ul.mac_addr[1],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_ul.mac_addr[2],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_ul.mac_addr[3],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_ul.mac_addr[4],
                    (int) pcmd->cmd_data.net_eps_mac_map_add_ul.mac_addr[5]);
                    
                // sanity check the MAC to see if the parser discarded it.
                if (memcmp(pcmd->cmd_data.net_eps_mac_map_add_ul.mac_addr, badmac, ETH_ALEN) == 0)
                {
                    AMP_LOG_Error("%s: Invalid MAC - discarding.\n", func_name);
                }
                else
                {
                    lte_qa_set_eps_mac_map(pcmd->cmd_data.net_eps_mac_map_add_ul.eps_id,
                        pcmd->cmd_data.net_eps_mac_map_add_ul.mac_addr);
                }

                break;

            case lte_dd_cmd_net_eps_mac_map_rm_ul:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d eps_id:%d\n",
                    func_name,
                    CMD_STR_NET_EPS_MAC_MAP_RM_UL,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.net_eps_mac_map_rm_ul.eps_id);
                    
                // NULL MAC address signals to reset back to NULL MAC address
                lte_qa_set_eps_mac_map(pcmd->cmd_data.net_eps_mac_map_rm_ul.eps_id,
                        NULL);

                break;

            case lte_dd_cmd_net_bridge_mode_ul:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d enable:%d\n",
                    func_name,
                    CMD_STR_NET_BRIDGE_MODE_UL,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.net_bridge_mode_ul.enable);
                    
                lte_qa_set_bridge_mode(pcmd->cmd_data.net_bridge_mode_ul.enable);

                break;

            case lte_dd_cmd_net_priority:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d ifname:%s priority:%d\n",
                    func_name,
                    CMD_STR_NET_PRIORITY,
                    (int) pcmd->cmd_id,
                    pcmd->cmd_data.net_priority.ifname,
                    (int) pcmd->cmd_data.net_priority.priority);
                    
                // verify that this interface exists
                pni = lte_dd_ni_interface_find_ifname(pcmd->cmd_data.net_priority.ifname);
                if (!pni || !pni->pnet)
                {
                    AMP_LOG_Error(
                        "%s - ERROR: Can't set priority because interface %s doesn't exist\n",
                        func_name,
                        pcmd->cmd_data.net_priority.ifname);
                }
                else
                {
                    // save in our internal net info structure
                    pni->priority = pcmd->cmd_data.net_priority.priority;
                }
                    
                break;
                
            case lte_dd_cmd_net_log:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d log_level:%d\n",
                    func_name,
                    CMD_STR_NET_LOG,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.net_log.log_level);
                    
                // sets for net driver module
                net_drv_set_log_level((short) pcmd->cmd_data.net_log.log_level);
                    
                break;
                
            case lte_dd_cmd_net_perf_test:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d duration:%d packet_size:%d packet_delay:%d\n",
                    func_name,
                    CMD_STR_NET_PERF_TEST,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.net_perf_test.duration,
                    (int) pcmd->cmd_data.net_perf_test.packet_size,
                    (int) pcmd->cmd_data.net_perf_test.packet_delay);
                
                // starts the net driver performance test, must be used with loopback BP or HIF AP loopback
                net_drv_perf_test(pcmd->cmd_data.net_perf_test.duration,
                    pcmd->cmd_data.net_perf_test.packet_size, 
                    pcmd->cmd_data.net_perf_test.packet_delay);
                    
                break;

            case lte_dd_cmd_dd_log:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d log_level:%d\n",
                    func_name,
                    CMD_STR_DD_LOG,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.dd_log.log_level);
                
                // sets for this module
                AMP_LOG_SetLogLevel(AMP_LOG_ADDR_LTE_DATA_DRIVER, pcmd->cmd_data.dd_log.log_level);
                
                break;

            case lte_dd_cmd_dd_ul_packet_profile:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d enable:%d reset:%d\n",
                    func_name,
                    CMD_STR_DD_UL_PACKET_PROFILE,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.dd_ul_packet_profile.enable,
                    (int) pcmd->cmd_data.dd_ul_packet_profile.reset);
                
                if (pcmd->cmd_data.dd_ul_packet_profile.reset)
                    tsl_reset(g_pdev->ptsl_ul);
                    
                tsl_enable(g_pdev->ptsl_ul, (long) pcmd->cmd_data.dd_ul_packet_profile.enable);
                
                break;

            case lte_dd_cmd_dd_dl_packet_profile:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d enable:%d reset:%d\n",
                    func_name,
                    CMD_STR_DD_DL_PACKET_PROFILE,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.dd_dl_packet_profile.enable,
                    (int) pcmd->cmd_data.dd_dl_packet_profile.reset);
                
                if (pcmd->cmd_data.dd_dl_packet_profile.reset)
                    tsl_reset(g_pdev->ptsl_dl);
                    
                tsl_enable(g_pdev->ptsl_dl, (long) pcmd->cmd_data.dd_dl_packet_profile.enable);
                
                break;
            
            case lte_dd_cmd_dd_multipacket:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d enable:%d\n",
                    func_name,
                    CMD_STR_DD_MULTIPACKET,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.dd_multipacket.enable);
                
                // enable or disable the multi packet mode
                if (pcmd->cmd_data.dd_multipacket.enable)
                    pdev->dd_flags |= LTE_DD_FLAG_MULTI_PACKET;
                else
                    pdev->dd_flags &= ~LTE_DD_FLAG_MULTI_PACKET;
                
                break;
            
            case lte_dd_cmd_dd_lpbp_mode:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d enable:%d\n",
                    func_name,
                    CMD_STR_DD_LPBP_MODE,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.dd_lpbp_mode.enable);
                
                // enable or disable the loopback BP mode, this will swizzle packets
                if (pcmd->cmd_data.dd_lpbp_mode.enable)
                    pdev->dd_flags |= LTE_DD_FLAG_LPBP;
                else
                    pdev->dd_flags &= ~LTE_DD_FLAG_LPBP;
                
                break;

            case lte_dd_cmd_dd_disable_ul:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d enable:%d\n",
                    func_name,
                    CMD_STR_DD_DISABLE_UL,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.dd_disable_ul.enable);

                net_drv_set_disable_ul( pcmd->cmd_data.dd_disable_ul.enable);
               
                break;
            
            case lte_dd_cmd_qos_log:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d log_level:%d\n",
                    func_name,
                    CMD_STR_QOS_LOG,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.qos_log.log_level);
            
                // sets for QOS module
                QA_SetLogLevel((short) pcmd->cmd_data.qos_log.log_level);
                
                break;
                        
            case lte_dd_cmd_qos_mode:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d mode:%d\n",
                    func_name,
                    CMD_STR_QOS_MODE,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.qos_mode.mode);
                
                // if non-zero, then the mode is IP tables, not the legacy QOS module
                if (pcmd->cmd_data.qos_mode.mode)
                    net_drv_set_qos_mode(NET_DRV_QOS_MODE_IPTABLES);
                else
                    net_drv_set_qos_mode(NET_DRV_QOS_MODE_QOS_MODULE);
                
                break;
            
            case lte_dd_cmd_hif_cmd:
                AMP_LOG_Info(
                    "%s - [%s] cmd_id:%d cmd:%d\n",
                    func_name,
                    CMD_STR_HIF_CMD,
                    (int) pcmd->cmd_id,
                    (int) pcmd->cmd_data.hif_cmd.cmd);
            
                IPC_4GHIF_debug_order((u32) pcmd->cmd_data.hif_cmd.cmd);
                
                break;
    
        }
        
        pcmd++;
    }


    return 0;
}


/**
 * LTE data driver /proc write function. for configuring the driver
 *
 * @param struct file* filp
 * @param const char __user* buff
 * @param unsigned long len
 * @param void* data
 *
 * @return ssize_t
 */
static ssize_t ltedd_proc_write(struct file* filp, const char __user* buff, unsigned long len, void* data)

{
    char* func_name = "ltedd_proc_write";
    char temp_buf[512];
    int bytes_to_copy = 0;
    static lte_dd_cmd_t cmd_array[MAX_DD_CMD_ARRAY];
    unsigned long populated = 0;
    

    if (len > 0)
    {
        memset(temp_buf, 0x00, sizeof(temp_buf));
        bytes_to_copy = min((unsigned long) (sizeof(temp_buf) -1), len);

        // copy configuration string from user space
        if (copy_from_user(&temp_buf, buff, bytes_to_copy))
        {
            AMP_LOG_Error(
                "%s - ERROR: Unable to copy user data; bytes_to_copy:%d\n", func_name, bytes_to_copy);
            return -EFAULT;
        }

        // parse the string and populate the array of command structures
        lte_dd_parse_dd_cmds(temp_buf, cmd_array, &populated);
        if (populated > 0)
        {
            // process the command structures
            lte_dd_process_dd_cmds(g_pdev, cmd_array, populated);
        }

    }

    return len;
}


#define LTE_DD_STATS_FORMAT "\n\
Driver:\n\
  tx_kt_delta_min sec:%d; nsec:%d\n\
  tx_kt_delta_max sec:%d; nsec:%d\n\
  rx_kt_delta_min sec:%d; nsec:%d\n\
  rx_kt_delta_max sec:%d; nsec:%d\n\
  txrx_kt_delta_min sec:%d; nsec:%d\n\
  txrx_kt_delta_max sec:%d; nsec:%d\n\
  tx_packet_cnt:%lu\n\
  rx_packet_cnt:%lu\n\
  tx_data_packets_sdma_submit:%lu\n\
  tx_data_packets_sdma_ack:%lu\n\
  tx_data_packets_sdma_bytes:%lu\n\
  rx_data_packets_sdma_notify:%lu\n\
  rx_data_packets_sdma_submit:%lu\n\
  rx_data_packets_sdma_ack:%lu\n\
  rx_data_packets_sdma_bytes:%lu\n\
  tx_data_packets_multi_avail:%lu\n\
  rx_data_packets_multi_avail:%lu\n\
  tx_throttle_close_cnt:%lu\n\
  tx_throttle_open_cnt:%lu\n\
  tx_data_busy_start_time:%lu\n\
  tx_data_throttle_start_time:%lu\n\
  rx_data_busy_start_time:%lu\n\
  rx_data_throttle_start_time:%lu\n\
  rx_net_rx_drop_cnt:%lu\n\
  tx_physaddr_align8_cnt:%lu\n\
  tx_physaddr_align16_cnt:%lu\n\
  tx_physaddr_align32_cnt:%lu\n\
  rx_physaddr_align8_cnt:%lu\n\
  rx_physaddr_align16_cnt:%lu\n\
  rx_physaddr_align32_cnt:%lu\n\
  dd_flags:0x%lx\n\
  dd_multipacket:%s\n\
  dd_lpbp_mode:%s\n\
  qos_mode:%s\n\
  bridge_mode_dl:%s\n\
  bridge_mode_ul:%s\n\
  dd_disable_ul:%s\n\
Interfaces:\n"


#define LTE_DD_NI_FORMAT "\
  name:%s\n\
    eps_id:%d\n\
    priority:%d\n\
    tx_data_packet_write_fifo_cnt:%lu\n\
    tx_data_packet_read_fifo_cnt:%lu\n\
    tx_throttle_close_cnt:%lu\n\
    tx_throttle_open_cnt:%lu\n\
    tx_drain_cnt:%lu\n"

/**
 * LTE data driver /proc read function
 *
 * @param char* page
 * @param char** start
 * @param off_t off
 * @param int* eof
 * @param void* data
 *
 * @return int - bytes written
 */
static int ltedd_proc_read(char* page, char** start, off_t off, int count, int* eof, void* data)
{
    int len = 0;
    int i = 0;
    char* dd_multipacket_mode_str = NULL;
    char* dd_lpbp_mode_str = NULL;
    char* qos_mode_str = NULL;
    char* bridge_mode_dl_str = NULL;
    char* bridge_mode_ul_str = NULL;
    char* dd_disable_ul_str = NULL;
    ipc_4ghif_stats ipc_stats;
    lte_dd_net_info_t* pni = NULL;
    unsigned long bridge_mode_dl = 0;
    unsigned long bridge_mode_ul = 0;
    unsigned long disable_ul = 0;
    unsigned long sl_flags = 0;
    char ni_buffer[512];
    

    // if not allocated, then print error
    if (!g_pdev)
    {
        len = sprintf(page, "ERROR: no %s object has been created\n",
            DEVICE_NAME);

        *eof = 1;

        return len;
    }
    
    // initialize various mode strings
    dd_multipacket_mode_str = (g_pdev->dd_flags & LTE_DD_FLAG_MULTI_PACKET) ? "Multiple" : "Single";
    dd_lpbp_mode_str = (g_pdev->dd_flags & LTE_DD_FLAG_LPBP) ? "BP loop-back" : "Normal";
    qos_mode_str = net_drv_get_qos_mode_string(net_drv_get_qos_mode());
    net_drv_get_bridge_mode(&bridge_mode_dl);
    lte_qa_get_bridge_mode(&bridge_mode_ul);
    net_drv_get_disable_ul(&disable_ul);
    bridge_mode_dl_str = (bridge_mode_dl) ? "Enabled" : "Disabled";
    bridge_mode_ul_str = (bridge_mode_ul) ? "Enabled" : "Disabled";
    dd_disable_ul_str = (disable_ul) ? "Enabled": "Disabled";
    
    memset(ni_buffer, 0x00, sizeof(ni_buffer));
    memset(&ipc_stats, 0x00, sizeof(ipc_stats));

    // prepend header and then fetch the 4GHIF string
    strcpy(page, "LTE DD Statistics\n");
    len = strlen(page);
    IPC_4GHIF_get_statistics_string(page + len, (u16) count - len);
    len = strlen(page);

    // format the stats
    len = sprintf(page + len, LTE_DD_STATS_FORMAT,

        // stats from this device
        g_pdev->tx_kt_delta_min.tv.sec, g_pdev->tx_kt_delta_min.tv.nsec,
        g_pdev->tx_kt_delta_max.tv.sec, g_pdev->tx_kt_delta_max.tv.nsec,
        g_pdev->rx_kt_delta_min.tv.sec, g_pdev->rx_kt_delta_min.tv.nsec,
        g_pdev->rx_kt_delta_max.tv.sec, g_pdev->rx_kt_delta_max.tv.nsec,
        g_pdev->txrx_kt_delta_min.tv.sec, g_pdev->txrx_kt_delta_min.tv.nsec,
        g_pdev->txrx_kt_delta_max.tv.sec, g_pdev->txrx_kt_delta_max.tv.nsec,
        g_pdev->tx_packet_cnt,
        g_pdev->rx_packet_cnt,
        g_pdev->tx_data_packets_sdma_submit,
        g_pdev->tx_data_packets_sdma_ack,
        g_pdev->tx_data_packets_sdma_bytes,
        g_pdev->rx_data_packets_sdma_notify,
        g_pdev->rx_data_packets_sdma_submit,
        g_pdev->rx_data_packets_sdma_ack,
        g_pdev->rx_data_packets_sdma_bytes,
        g_pdev->tx_data_packets_multi_avail,
        g_pdev->rx_data_packets_multi_avail,
        g_pdev->tx_throttle_close_cnt,
        g_pdev->tx_throttle_open_cnt,
        g_pdev->tx_data_busy_start_time,
        g_pdev->tx_data_throttle_start_time,
        g_pdev->rx_data_busy_start_time,
        g_pdev->rx_data_throttle_start_time,
        g_pdev->rx_net_rx_drop_cnt,
        g_pdev->tx_physaddr_align8_cnt,
        g_pdev->tx_physaddr_align16_cnt,
        g_pdev->tx_physaddr_align32_cnt,
        g_pdev->rx_physaddr_align8_cnt,
        g_pdev->rx_physaddr_align16_cnt,
        g_pdev->rx_physaddr_align32_cnt,
        g_pdev->dd_flags,
        dd_multipacket_mode_str,
        dd_lpbp_mode_str,
        qos_mode_str,
        bridge_mode_dl_str,
        bridge_mode_ul_str,
        dd_disable_ul_str);
   

    // grab our spin lock 
    spin_lock_irqsave(&(g_pdev->sl_lock), sl_flags);
    
    // now add all of the net object specific infomation
    for (i = 0; i < NETDRV_MAX_NUM_DEVICES; i++)
    {
        pni = lte_dd_ni_interface_get_at_index(i);
        if (pni && pni->pnet)
        {         
            sprintf(ni_buffer, LTE_DD_NI_FORMAT,
                pni->ifname,
                (int) pni->eps_id,
                (int) pni->priority,
                pni->tx_data_packet_write_fifo_cnt,
                pni->tx_data_packet_read_fifo_cnt,
                pni->tx_throttle_close_cnt,
                pni->tx_throttle_open_cnt,
                pni->tx_drain_cnt);
            
            // now cat onto the main buffer
            strcat(page, ni_buffer);
        }
    }
    
    // release the spin lock
    spin_unlock_irqrestore(&(g_pdev->sl_lock), sl_flags);
    
    // recalc the total length
    len = strlen(page);
        
    *eof = 1;


    return len;

}

//******************************************************************************
// exported functions
//******************************************************************************

/**
 * LTE data driver module cleanup routine. This function is invoked in
 *  response to rmmod
 *
 * @param void
 *
 * @return void
 */
void lte_dd_cleanup_module(void)
{
    char* func_name = "lte_dd_cleanup_module";
    dev_t devno = MKDEV(g_dev_major, g_dev_minor);
    lte_dd_net_info_t* pni = NULL;
    int i = 0;


    // release the UL profile proc entry
    if (g_proc_entry_ul_profile != NULL)
    {
        remove_proc_entry(DEVICE_NAME_UL_PROFILE, NULL);
    }

    // release the DL profile proc entry
    if (g_proc_entry_dl_profile != NULL)
    {
        remove_proc_entry(DEVICE_NAME_DL_PROFILE, NULL);
    }

    // release the proc entry
    if (g_proc_entry != NULL)
    {
        remove_proc_entry(DEVICE_NAME, NULL);
    }

    // if allocated, then delete the device
    if (g_pdev)
    {
        // kill the watchdog timer
        del_timer(&g_pdev->ul_dl_wd_timer);
        
        // close the 4GHIF API
        IPC_4GHIF_close();

        // delete all of the network interfaces that have been created
        for (i = 0; i < (sizeof(g_pdev->ni_interfaces) / sizeof(lte_dd_net_info_t)); i++)
        {
            pni = &(g_pdev->ni_interfaces[i]);
            lte_dd_delete_net_iface(g_pdev, pni);
        }
        
        // free the tsl's
        if (g_pdev->ptsl_ul)
            tsl_free(g_pdev->ptsl_ul);

        if (g_pdev->ptsl_dl)
            tsl_free(g_pdev->ptsl_dl);
    
        // finialize the tsl module
        tsl_finalize();

#if defined (DO_DYNAMIC_DEV)
        if (lte_dd_class)
        {
            AMP_LOG_Info(
                "%s - removing %s class\n", func_name, DEVICE_NAME);
            
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24)
            device_destroy(lte_dd_class, devno);
#else
            class_device_destroy(lte_dd_class, devno);
#endif
            class_destroy(lte_dd_class);
        }
#endif
        
        // delete the char device
        cdev_del(&g_pdev->cdev);    
        
        kfree(g_pdev);
        g_pdev = NULL;
    }
    
    // now unregister the device
    unregister_chrdev_region(devno, 1);

    AMP_LOG_Info(
        "%s - complete\n", func_name);

}


/**
 * LTE data driver module initialization routine.  This function is
 *  invoked in response to insmod
 *
 * @param void
 *
 * @return int
 */
int __init lte_dd_init_module(void)
{
    char* func_name = "lte_dd_init_module";
    int ret = 0;
    int err = 0;
    int devno = 0;
    dev_t dev = 0;
    long retval = AMP_ERROR_NONE;
    lte_dd_net_info_t* pni = NULL;
    IPC_4GHIF_STATUS ipc4_status = IPC_4GHIF_STATUS_OK;
    ipc_4ghif_callbacks ghif_callbacks = 
    {
        lte_dd_4ghif_bp_ready_cb,
        lte_dd_4ghif_receive_dl_cb,
        lte_dd_4ghif_dl_complete_cb,
        lte_dd_4ghif_ul_complete_cb,
        lte_dd_4ghif_ul_resume_cb
    };


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
        AMP_LOG_Error(
            "%s - ERROR: can't get major %d\n", func_name, g_dev_major);
        return ret;
    }
        
    // now allocate our single device 
    g_pdev = kmalloc(sizeof(lte_dd_dev_t), GFP_KERNEL);
    if (!g_pdev)
    {
        ret = -ENOMEM;
        goto INIT_FAIL;
    }
    
    memset(g_pdev, 0x00, sizeof(lte_dd_dev_t));
    
    // finish initializing our device structure
    g_pdev->signature = LTE_DD_SIGNATURE;
    g_pdev->size = sizeof(lte_dd_dev_t);

    // initialize tsl module    
    tsl_initialize();

    // alloc new tsl objects for UL and DL logs
    g_pdev->ptsl_ul = tsl_alloc(UL_TSL_NUM_ENTRIES);
    g_pdev->ptsl_dl = tsl_alloc(DL_TSL_NUM_ENTRIES);
    if (!g_pdev->ptsl_ul || !g_pdev->ptsl_dl)
    {
        AMP_LOG_Error(
            "%s - ERROR: unable to allocate time stamp logs: ul:%p; dl:%p\n",
            func_name, g_pdev->ptsl_ul, g_pdev->ptsl_dl);
        ret = -ENOMEM;
        goto INIT_FAIL;
    }

    // these are enabled by default, we want them disabled for now
    tsl_enable(g_pdev->ptsl_ul, 0);
    tsl_enable(g_pdev->ptsl_dl, 0);

    // init sync objects
    spin_lock_init(&g_pdev->sl_lock);
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_DEBUG1,
        "%s - creating cdev: name:%s; major:%d; minor:%d\n",
        func_name, DEVICE_NAME, g_dev_major, g_dev_minor);
            
    devno = MKDEV(g_dev_major, g_dev_minor);
    cdev_init(&g_pdev->cdev, &g_lte_dd_fops);
    g_pdev->cdev.owner = THIS_MODULE;
    g_pdev->cdev.ops = &g_lte_dd_fops;
    err = cdev_add(&g_pdev->cdev, devno, 1);
    if (err)
    {
        AMP_LOG_Error(
            "%s - ERROR: error %d while adding cdev: name:%s; major:%d; minor:%d\n",
            func_name, err, DEVICE_NAME, g_dev_major, g_dev_minor);
        ret = -ENOMEM;
        goto INIT_FAIL;
    }

    //    
    // we are now listed under /proc/devices, ie: "cat /proc/devices"
    //

// if the following code is not enabled, then the mknod command must
// be run in order to create the device special file
#if defined (DO_DYNAMIC_DEV)
    // create a class structure to be used with creating the device special file
    // that is located under the /dev folder
    lte_dd_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(lte_dd_class))
    {
        AMP_LOG_Error(
            "%s - ERROR: Error in creating lte_dd_class class\n", func_name);
        ret = -ENOMEM;
        goto INIT_FAIL;
    }

    // create the actual device specific file in /dev/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24)
    if(IS_ERR(device_create(lte_dd_class, NULL, devno, "%s", DEVICE_NAME_DEV)))
#else
    if(IS_ERR(class_device_create(lte_dd_class, NULL, devno, NULL, "%s", DEVICE_NAME_DEV)))
#endif
    {
        AMP_LOG_Error(
            "%s - ERROR: Error in creating device special file: %s\n", func_name, DEVICE_NAME_DEV);            
        ret = -ENOMEM;
        goto INIT_FAIL;
    }
#endif

    // now set default driver flag states
    g_pdev->dd_flags |= LTE_DD_FLAG_MULTI_PACKET;
    
    // now create the lte0 interface which is always persistent
    // the first index is always reserved for lte0
    pni = &(g_pdev->ni_interfaces[0]);
    retval = lte_dd_create_net_iface(g_pdev, LTE_DD_PERSISTENT_IFACE_NAME, NETDRV_DEFAULT_EPS_ID, 1, pni);
    if (retval != AMP_ERROR_NONE)
    {
        ret = -ENOMEM;
        goto INIT_FAIL;
    }
    
    // open the 4GHIF API
    ipc4_status = IPC_4GHIF_open(ghif_callbacks);
    if (ipc4_status != IPC_4GHIF_STATUS_OK)
    {
        AMP_LOG_Error(
            "%s - ERROR: error opening 4GHIF API - ipc4_status:%d\n", func_name, ipc4_status);
        ret = -ENOMEM;
        goto INIT_FAIL;   
    }

    // initialize and start our UL and DL watchdog timer
    init_timer(&g_pdev->ul_dl_wd_timer);
    g_pdev->ul_dl_wd_timer.function = ul_dl_watchdog_func;
    g_pdev->ul_dl_wd_timer.data = (unsigned long) 0;
    g_pdev->ul_dl_wd_timer.expires = jiffies + UL_DL_WATCHDOG_TIMER_INTERVAL;
    add_timer(&(g_pdev->ul_dl_wd_timer));
    
    // now create a /proc entry for returning various statistics
    g_proc_entry = create_proc_entry(DEVICE_NAME, 0644, NULL);
    if (g_proc_entry != NULL)
    {
        g_proc_entry->read_proc = ltedd_proc_read;
        g_proc_entry->write_proc = ltedd_proc_write;
        g_proc_entry->owner = THIS_MODULE;
    }

    // create a proc entry for UL profile data
    g_proc_entry_ul_profile = create_proc_entry(DEVICE_NAME_UL_PROFILE, 0644, NULL);
    if (g_proc_entry_ul_profile != NULL)
    {
        g_proc_entry_ul_profile->proc_fops = &lte_dd_ul_profile_proc_ops;
    }

    // create a proc entry for DL profile data
    g_proc_entry_dl_profile = create_proc_entry(DEVICE_NAME_DL_PROFILE, 0644, NULL);
    if (g_proc_entry_dl_profile != NULL)
    {
        g_proc_entry_dl_profile->proc_fops = &lte_dd_dl_profile_proc_ops;
    }

    // initialize statistics
    g_pdev->tx_kt_delta_min = ktime_set(0x7FFFFFFF, 0xFFFFFFFF);
    g_pdev->tx_kt_delta_max = ktime_set(0x00000000, 0x00000000);
    g_pdev->rx_kt_delta_min = ktime_set(0x7FFFFFFF, 0xFFFFFFFF);
    g_pdev->rx_kt_delta_max = ktime_set(0x00000000, 0x00000000);
    g_pdev->txrx_kt_delta_min = ktime_set(0x7FFFFFFF, 0xFFFFFFFF);
    g_pdev->txrx_kt_delta_max = ktime_set(0x00000000, 0x00000000);


#if defined (SHOW_TIME_GRAN)
    {
        union ktime ts1;
        union ktime ts2;
        union ktime tsd;
        long delay = 0;

        for (delay = 10000; delay < 200000; delay += 10000)
        {
            ts1 = ktime_get();
            ndelay(delay);
            ts2 = ktime_get();

            tsd = ktime_sub(ts2, ts1);

            AMP_LOG_Info(
                "%s - [ktime] delay:%ld TS1-sec:%ld nsec:%ld TS2:-sec:%ld nsec:%ld DELTA:-sec:%ld nsec:%ld\n",
                func_name, delay, ts1.tv.sec, ts1.tv.nsec, ts2.tv.sec, ts2.tv.nsec, tsd.tv.sec, tsd.tv.nsec);
        }

    }
#endif

    
    AMP_LOG_Info(
        "%s - complete\n", func_name);
    
    return 0;
    
 
  
 INIT_FAIL:
    lte_dd_cleanup_module();
    
    AMP_LOG_Error(
        "%s - ERROR: leave, ret:%d\n", func_name, ret);
    return ret;

}


// export the init and cleanup functions
module_init(lte_dd_init_module);
module_exit(lte_dd_cleanup_module);

MODULE_DESCRIPTION("Motorola LTE Data Driver");
MODULE_LICENSE("GPL");

