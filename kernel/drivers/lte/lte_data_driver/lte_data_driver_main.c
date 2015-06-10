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

#include <AMP_typedef.h>
#include <AMP_lte_log.h>
#include <AMP_lte_error.h>
#include <ptr_fifo.h>
#include <time_calc.h>

#include "net_driver.h"
#include "lte_data_driver.h"

#define DEVICE_NAME "lte_dd"
#define DEVICE_NAME_DEV "lte_dd0"

#define LTE_DD_SIGNATURE                        0xBAADBEEF
#define LTE_DD_TX_DATA_PACKET_FIFO_SIZE         8
#define LTE_DD_TX_MGMT_PACKET_FIFO_SIZE         8
#define LTE_DD_RX_MGMT_PACKET_FIFO_SIZE         8


typedef struct
{

    unsigned long signature;                        // our modules signature
    unsigned long size;                             // this structure's size
    void* pnet;                                     // net driver handle returned from the net driver probe call
    NET_DRV_BUS_INTERFACE net_drv_bus_interface;    // interface between this device and a net device
    PTR_FIFO_HANDLE h_tx_data_fifo;                 // handle to a ptr fifo object for TX data packets
    PTR_FIFO_HANDLE h_tx_mgmt_fifo;                 // handle to a ptr fifo object for TX mgmt packets
    PTR_FIFO_HANDLE h_rx_mgmt_fifo;                 // handle to a ptr fifo object for RX mgmt packets
    unsigned long tx_data_packet_write_cnt;         // total tx data packet write count
    unsigned long tx_data_packet_read_cnt;          // total tx data packet read count
    unsigned long rx_data_packet_write_cnt;         // total rx data packet write count
    unsigned long rx_data_packet_read_cnt;          // total rx data packet read count
    unsigned long tx_mgmt_packet_write_cnt;         // total tx mgmt packet write count
    unsigned long tx_mgmt_packet_read_cnt;          // total tx mgmt packet read count
    unsigned long rx_mgmt_packet_write_cnt;         // total rx mgmt packet write count
    unsigned long rx_mgmt_packet_read_cnt;          // total rx mgmt packet read count
    unsigned long tx_data_throttle;                 // throttle tx data
    unsigned long tx_mgmt_throttle;                 // throttle tx mgmt
    unsigned long mgmt_msg_tx_time;                 // the time the last tx mgmt packet was sent (read)
    wait_queue_head_t txq;                          // tx wait queue
    wait_queue_head_t rxq;                          // rx wait queue
    struct semaphore sem;                           // mutual exclusion semaphore
    struct cdev cdev;                               // char device structure
    
} lte_dd_dev_t;


static int g_dev_major = 0;
static int g_dev_minor = 0;
static lte_dd_dev_t* g_pdev = NULL;
static struct proc_dir_entry* g_proc_entry = NULL;


#define DO_DYNAMIC_DEV
#if defined (DO_DYNAMIC_DEV)
struct class* lte_dd_class = NULL;
#endif


/**
 * Validates the device pointer as ours
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return long, error code
 */ 
static long lte_dd_private_valid(lte_dd_dev_t* pdev)
{
    
    if (!pdev || pdev->signature != LTE_DD_SIGNATURE || pdev->size != sizeof(lte_dd_dev_t))
        return AMP_ERROR_LTEDD_PRIVATE_INVALID;

    return AMP_ERROR_NONE;
}


/**
 * Prints data buffer.
 *
 * @param pbanner - log banner.
 * @param pbuffer - buffer to print.
 * @param buf_size - buffer size.
 *
 * @return void
 */
static void print_buf(char* pbanner, char* pbuffer, unsigned long buf_size)
{
    int i;
    short log_level = AMP_LOG_LEVEL_WARNING;


    AMP_LOG_GetLogLevel(AMP_LOG_ADDR_LTE_DATA_DRIVER, log_level);
    
    if (log_level >= AMP_LOG_LEVEL_DEBUG3)
    {  
        printk("%s", pbanner);
        for (i = 0; i < buf_size; i++)
        {
            if ((0 == (i % 16)) && i != 0)
                printk("\n");

            printk("%02x ", (unsigned char)pbuffer[i]);
        }
        printk("\n");
    }
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
 * Flushes the tx data fifo
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return long, error code
 */ 
static long lte_dd_flush_tx_data_fifo(lte_dd_dev_t* pdev)
{
    char* func_name = "lte_dd_flush_tx_data_fifo";
    struct sk_buff* pskb = NULL;
    long pfval = PTR_FIFO_ERROR_NONE;
    
              
    // drain the tx data fifo
    while(1)
    {
        pfval = ptr_fifo_remove(pdev->h_tx_data_fifo, (void**) &pskb);
        if (pfval != PTR_FIFO_ERROR_NONE)
            break;
       
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
            "%s - deleting pskp:%p\n", func_name, pskb);
        
        if (pskb)
            dev_kfree_skb_any(pskb);
    }
    
    pdev->tx_data_packet_write_cnt = 0;
    pdev->tx_data_packet_read_cnt = 0;

    return AMP_ERROR_NONE;
}


/**
 * Flushes the tx mgmt fifo
 *
 * @param lte_dd_dev_t* pdev
 *
 * @return long, error code
 */ 
static long lte_dd_flush_tx_mgmt_fifo(lte_dd_dev_t* pdev)
{
    char* func_name = "lte_dd_flush_tx_mgmt_fifo";
    mgmt_packet_t* pmgmt_packet = NULL;
    long pfval = PTR_FIFO_ERROR_NONE;
    
              
    // drain the tx mgmt fifo
    while(1)
    {
        pfval = ptr_fifo_remove(pdev->h_tx_mgmt_fifo, (void**) &pmgmt_packet);
        if (pfval != PTR_FIFO_ERROR_NONE)
            break;
       
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
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
static long lte_dd_flush_rx_mgmt_fifo(lte_dd_dev_t* pdev)
{
    char* func_name = "lte_dd_flush_rx_mgmt_fifo";
    mgmt_packet_t* pmgmt_packet = NULL;
    long pfval = PTR_FIFO_ERROR_NONE;
    
              
    // drain the tx mgmt fifo
    while(1)
    {
        pfval = ptr_fifo_remove(pdev->h_rx_mgmt_fifo, (void**) &pmgmt_packet);
        if (pfval != PTR_FIFO_ERROR_NONE)
            break;
       
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
            "%s - deleting rx mgmt packet:%p\n", func_name, pmgmt_packet);
        
        if (pmgmt_packet)
           free_mgmt_payload(pmgmt_packet);
    }
    
    pdev->rx_mgmt_packet_write_cnt = 0;
    pdev->rx_mgmt_packet_read_cnt = 0;

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
loff_t lte_dd_dev_llseek(struct file* filp, loff_t off, int whence)
{
    char* func_name = "lte_dd_dev_llseek";
    //lte_dd_dev_t* pdev = (lte_dd_dev_t*) filp->private_data;
    loff_t newpos = 0;


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
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
ssize_t lte_dd_dev_read(struct file* filp, char __user * buf, size_t count, loff_t* f_pos)
{
    char* func_name = "lte_dd_dev_read";
    lte_dd_dev_t* pdev = (lte_dd_dev_t*) filp->private_data; 
    long pfval = PTR_FIFO_ERROR_NONE;
    int tx_data_packet_avail = 0;
    int tx_mgmt_packet_avail = 0;
    int process_data_packet = 0;
    int process_mgmt_packet = 0;
    unsigned long elapsed_mgmt_tx_time = 0;
    hif_header_drv_t hif_hdr_drv = {0};
    hif_header_drv_t* phif_hdr_drv = &(hif_hdr_drv);
    size_t hif_hdr_drv_size = sizeof(hif_header_drv_t);
    size_t data_payload_size = 0;
    

 
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
        "%s - filp:%p; buf:%p; count:%d; f_pos:%p\n",
        func_name, filp, buf, (int) count, f_pos);    
    
    
    // make sure that we have at least the HIF data header size in bytes
    if (count <= hif_hdr_drv_size)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - minimum data size %d requirement not met\n",
            func_name, hif_hdr_drv_size);
        return -EFAULT;
    }
    
    
    if (down_interruptible(&pdev->sem))
        return -ERESTARTSYS;
 
    // while the tx write and tx read counts are the same
    while (pdev->tx_data_packet_write_cnt == pdev->tx_data_packet_read_cnt &&
           pdev->tx_mgmt_packet_write_cnt == pdev->tx_mgmt_packet_read_cnt)
    {
        // nothing to read, release the lock
        up(&pdev->sem);
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
            "%s - going to sleep\n", func_name);

        // wait until something is available in the tx data or mgmt fifos
        if (wait_event_interruptible(pdev->txq,
            (pdev->tx_data_packet_write_cnt != pdev->tx_data_packet_read_cnt) ||
            (pdev->tx_mgmt_packet_write_cnt != pdev->tx_mgmt_packet_read_cnt)))
            return -ERESTARTSYS; // signal: tell the fs layer to handle it
        
        // otherwise loop, but first reacquire the lock
        if (down_interruptible(&pdev->sem))
            return -ERESTARTSYS;
    }

    // we will now determine what to send, either tx data or tx mgmt
    tx_data_packet_avail = (int) (pdev->tx_data_packet_write_cnt != pdev->tx_data_packet_read_cnt);
    tx_mgmt_packet_avail = (int) (pdev->tx_mgmt_packet_write_cnt != pdev->tx_mgmt_packet_read_cnt);    
    
    // based on throttle, avail and delta time
    if (!(pdev->tx_data_throttle) && (tx_data_packet_avail))
    {
        // there is also tx mgmt to send
        if (!(pdev->tx_mgmt_throttle) && (tx_mgmt_packet_avail))
        {
#define MAC_MSG_TX_TIMEOUT (((HZ * 1) / 100) + 1)
            // check the elapsed time since the last mgmt msg was sent (HZ == 250 for our build)
            elapsed_mgmt_tx_time = delta_time_jiffies(pdev->mgmt_msg_tx_time);
            if (elapsed_mgmt_tx_time >= MAC_MSG_TX_TIMEOUT)
            {
                AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
                    "%s: elapsed time of %d (ms) exceeded for waiting mgmt packet, sending mgmt packet instead of data packet\n",
                    func_name, (int) ((MAC_MSG_TX_TIMEOUT / (double) HZ) * 1000));
                
                process_mgmt_packet = 1;
            }
            else
            {
                process_data_packet = 1;
            }
        }
        else
        {
            process_data_packet = 1;
        }
    }
    else if (!(pdev->tx_mgmt_throttle) && (tx_mgmt_packet_avail))
    {
        process_mgmt_packet = 1;
    }
    

    //
    // WARNING!!!! - only one of the following flags should be set
    //

    // a tx data packet is now available
    if (process_data_packet)
    {
        struct sk_buff* pskb = NULL;
        
        pfval = ptr_fifo_remove(pdev->h_tx_data_fifo, (void**) &pskb);
        pdev->tx_data_packet_read_cnt++;
        if (pfval == PTR_FIFO_ERROR_NONE)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
                "%s - TX data packet available - pskb:%p; data:%p; len:%d\n",
                func_name, pskb, pskb->data, pskb->len);
            
            print_buf("lte_dd_dev_read - tx data:\n", pskb->data, pskb->len);
    
            // initialize our local HIF header prior to kern-2-user copy
            INIT_HIF_HEADER_DRV(phif_hdr_drv, HIF_ENDPOINT_DATA_OUT,  pskb->len);
            
            // copy the header first
            if (copy_to_user(buf, phif_hdr_drv, hif_hdr_drv_size))
            {
                pdev->net_drv_bus_interface.tx_complete(pdev->pnet, pdev, pskb);
                up(&pdev->sem);
                return -EFAULT;
            }
            
            // actual available tx data payload size is total minus the header
            data_payload_size = count - hif_hdr_drv_size;
    
            // check for buffer overrun
            count = min(data_payload_size, pskb->len);

            // copy the pskb data
            if (copy_to_user(buf + hif_hdr_drv_size, pskb->data, count))
            {
                pdev->net_drv_bus_interface.tx_complete(pdev->pnet, pdev, pskb);
                up(&pdev->sem);
                return -EFAULT;
            }
            
            pdev->net_drv_bus_interface.tx_complete(pdev->pnet, pdev, pskb);
            *f_pos += count;
        }
    }

    // a tx mgmt packet is now available
    if (process_mgmt_packet)
    {
        mgmt_packet_t* pmgmt_packet = NULL;
        
        pdev->mgmt_msg_tx_time = jiffies;
        pfval = ptr_fifo_remove(pdev->h_tx_mgmt_fifo, (void**) &pmgmt_packet);
        pdev->tx_mgmt_packet_read_cnt++;
        if (pfval == PTR_FIFO_ERROR_NONE)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
                "%s - TX mgmt packet available - pskb:%p; data:%p; len:%d\n",
                func_name, pmgmt_packet, pmgmt_packet->data, pmgmt_packet->len);
            
            print_buf("lte_dd_dev_read - tx mgmt:\n", pmgmt_packet->data, pmgmt_packet->len);
    
            // initialize our local HIF header prior to kern-2-user copy
            INIT_HIF_HEADER_DRV(phif_hdr_drv, HIF_ENDPOINT_MGMT_OUT,  pmgmt_packet->len);
            
            // copy the header first
            if (copy_to_user(buf, phif_hdr_drv, hif_hdr_drv_size))
            {
                free_mgmt_payload(pmgmt_packet);
                up(&pdev->sem);
                return -EFAULT;
            }
            
            // actual available tx data payload size is total minus the header
            data_payload_size = count - hif_hdr_drv_size;
    
            // check for buffer overrun
            count = min(data_payload_size, pmgmt_packet->len);

            // copy the pskb data
            if (copy_to_user(buf + hif_hdr_drv_size, pmgmt_packet->data, count))
            {
                free_mgmt_payload(pmgmt_packet);
                up(&pdev->sem);
                return -EFAULT;
            }
            
            free_mgmt_payload(pmgmt_packet);
            *f_pos += count;
        }
    }
  
     
    up(&pdev->sem);

    // wake anyone waiting for rx data
    // wake_up_interruptible(&pdev->rxq);
    

    return count;
        
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
ssize_t lte_dd_dev_write(struct file* filp, const char __user * buf, size_t count, loff_t* f_pos)
{
    char* func_name = "lte_dd_dev_write";
    lte_dd_dev_t* pdev = (lte_dd_dev_t*) filp->private_data;
    hif_header_drv_t hif_hdr_drv = {0};
    size_t hif_hdr_drv_size = sizeof(hif_header_drv_t);
    size_t data_payload_size = 0;



    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
        "%s - filp:%p; buf:%p; count:%d; f_pos:%p\n",
        func_name, filp, buf, (int) count, f_pos); 

    
    // make sure that we have at least the HIF data header size in bytes
    if (count <= hif_hdr_drv_size)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - minimum data size %d requirement not met\n",
            func_name, hif_hdr_drv_size);
        return -EFAULT;
    }
        
    // copy header
    memset(&hif_hdr_drv, 0x00, hif_hdr_drv_size);
    if (copy_from_user(&hif_hdr_drv, buf, hif_hdr_drv_size))
        return -EFAULT;

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
        "%s - HIF header - signature:0x%X; endpoint:%d; length:%d\n",
        func_name, (int) hif_hdr_drv.signature, (int) hif_hdr_drv.hif.end_point,
        (int) hif_hdr_drv.hif.length); 
      
    // validate the header using signature
    if (hif_hdr_drv.signature != HIF_HEADER_SIGNATURE)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - hif header - invalid HIF header 0x%X signature\n",
            func_name, (int) hif_hdr_drv.signature);
        return -EFAULT; 
    }

    // validate the endpoint
    if (hif_hdr_drv.hif.end_point != HIF_ENDPOINT_DATA_IN &&
        hif_hdr_drv.hif.end_point != HIF_ENDPOINT_MGMT_IN)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - hif header - invalid endpoint %d specified\n",
            func_name, (int) hif_hdr_drv.hif.end_point);
        return -EFAULT; 
    }
    
    // actual data payload size is total minus the header
    data_payload_size = count - hif_hdr_drv_size;
    
    // validate the length
    if ((size_t)hif_hdr_drv.hif.length != data_payload_size)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - hif header - invalid lenth %d specified\n",
            func_name, (int) hif_hdr_drv.hif.length);
        return -EFAULT; 
    }

    // now route the payload according to the end point
    switch(hif_hdr_drv.hif.end_point)
    {
    
        case HIF_ENDPOINT_DATA_IN:
            {
                struct sk_buff* pskb = NULL;
                
                
                AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
                  "%s - DATA_IN: specifing %d bytes for pskb\n",
                  func_name, data_payload_size);
               
                 if (down_interruptible(&pdev->sem))
                    return -ERESTARTSYS;
        
                // call the net driver to allocate a skb for this payload
                pskb = pdev->net_drv_bus_interface.rx_start(pdev->pnet, pdev, (int) data_payload_size,
                    /* GFP_ATOMIC */ GFP_KERNEL);
                if (!pskb)
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
                        "%s - ERROR allocating skb, dropping RX data packet\n",
                        func_name);
                    
                    up(&pdev->sem);
                    return -EFAULT;
                }
                
                AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
                    "%s - actual allocated for pskb: %d bytes\n",
                    func_name, pskb->len); 
                
                // copy payload data portion to skb
                if (copy_from_user(pskb->data, buf + hif_hdr_drv_size, data_payload_size))
                {
                    dev_kfree_skb_any(pskb);
                    up(&pdev->sem);
                    return -EFAULT;
                }
                
                // now pass this SKB to the net net driver
                pdev->net_drv_bus_interface.rx_complete(pdev->pnet, pdev, pskb); 
            
                up(&pdev->sem);
    
                // wake anyone waiting for rx data
                wake_up_interruptible(&pdev->rxq);
                
            }
            break;
            
        case HIF_ENDPOINT_MGMT_IN:
            {
                long pfval = PTR_FIFO_ERROR_NONE;
                mgmt_packet_t* pmgmt_packet = NULL;
 
                
                AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
                  "%s - MGMT_IN: payload size:%d\n",
                    func_name, data_payload_size);
                  
                if (down_interruptible(&pdev->sem))
                    return -ERESTARTSYS;
                
                // allocate a mgmt packet for this payload
                pmgmt_packet = alloc_mgmt_payload(data_payload_size);
                if (!pmgmt_packet)
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
                        "%s - ERROR allocating mgmt packet, dropping RX mgmt packet\n",
                        func_name);
                        
                    up(&pdev->sem);
                    return -EFAULT;
                }
            
                // copy payload data portion to mgmt packet
                if (copy_from_user(pmgmt_packet->data, buf + hif_hdr_drv_size, data_payload_size))
                {
                    free_mgmt_payload(pmgmt_packet);
                    up(&pdev->sem);
                    return -EFAULT;
                }
                
                pfval = ptr_fifo_insert(pdev->h_rx_mgmt_fifo, pmgmt_packet);
                if (pfval == PTR_FIFO_ERROR_NONE)
                    pdev->rx_mgmt_packet_write_cnt++;
                else
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_ERROR,
                        "%s - ERROR, insert error on RX mgmt FIFO, dropping packet:%p\n",
                        func_name, pmgmt_packet);
                        
                    count = -EFAULT;
                    free_mgmt_payload(pmgmt_packet);
                }
            
                up(&pdev->sem);
    
                // wake anyone waiting for rx data
                wake_up_interruptible(&pdev->rxq);
                  
            }
            break;
    
    }
 
    
    return count; 
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
int lte_dd_dev_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
    char* func_name = "lte_dd_dev_ioctl";
    //lte_dd_dev_t* pdev = (lte_dd_dev_t*) filp->private_data;
    int ret = 0;

    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
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
int lte_dd_dev_open(struct inode* inode, struct file* filp)
{
    char* func_name = "lte_dd_dev_open";
    lte_dd_dev_t* pdev = NULL;

 
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
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
int lte_dd_dev_release(struct inode* inode, struct file* filp)
{
    char* func_name = "lte_dd_dev_open";


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
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

    
    print_buf("lte_dd_tx_start received\n", pskb->data, pskb->len);
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_WARNING,
        "%s - received socket buffer - pnet:%p; pbus:%p; pskb:%p len:%d\n",
        func_name, pnet, pbus, pskb, (int) pskb->len);
    
    
    // the net driver passes us our object back as the "bus" object
    // if we return an error, then the stack is notified and will retry
    // at a later time
    if ((retval = lte_dd_private_valid(pdev)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - ERROR private data is invalid\n", func_name);
        return retval;
    }

    // insert the pskb into FIFO, after the pskb is removed from the
    // FIFO and actually xmitted, the bus function tx_complete will be called
    // to notify the stack that the skb has been sent.  this takes place
    // in the read function
    pfval = ptr_fifo_insert(pdev->h_tx_data_fifo, pskb);
    if (pfval != PTR_FIFO_ERROR_NONE)
        return AMP_ERROR_LTEDD_FIFO_INSERT;
    
    // increment the tx write count
    pdev->tx_data_packet_write_cnt++;


//#define DO_FAKE_MGMT_PACKET
#if defined (DO_FAKE_MGMT_PACKET)
    {
        mgmt_packet_t* pmgmt_packet = NULL;
        size_t payload_size = 32;
        
        // as a test, every so often fabricate a mgmt packet
        if ((pdev->tx_data_packet_write_cnt % 4) == 0)
        {
            pmgmt_packet = alloc_mgmt_payload(payload_size);
            
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_WARNING,
                "%s - inserting fake management packet:%p\n", func_name, pmgmt_packet);
                
            if (pmgmt_packet)
            {
                memset(pmgmt_packet->data, 0x4D, payload_size);
                pfval = ptr_fifo_insert(pdev->h_tx_mgmt_fifo, pmgmt_packet);
                if (pfval == PTR_FIFO_ERROR_NONE)
                    pdev->tx_mgmt_packet_write_cnt++;
                else
                {
                    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_ERROR,
                        "%s - insert error on TX mgmt FIFO, dropping packet:%p\n",
                        func_name, pmgmt_packet);

                    free_mgmt_payload(pmgmt_packet);
                }
            }
        }
    }   
#endif
    
    // wake anyone waiting on tx data
    wake_up_interruptible(&pdev->txq);
    

    return AMP_ERROR_NONE;
}


/**
 * LTE data driver BUS function that is supplied to the network driver. This 
 *  is called when the stack has uplink data
 *
 * @param void* pnet
 * @param void* pbus
 * @param struct sk_buff* pskb
 *
 * @return long, error code
 */
static long lte_dd_bus_control(void* pnet, void* pbus, unsigned long cmd)
{
    char* func_name = "lte_dd_bus_control";
    lte_dd_dev_t* pdev = (lte_dd_dev_t*) pbus;
    long retval = AMP_ERROR_NONE;
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_ERROR,
        "%s - pnet:%p; pbus:%p; cmd:0x%x\n",
        func_name, pnet, pbus, (int) cmd);
    
    // the net driver passes us our object back as the "bus" object
    if ((retval = lte_dd_private_valid(pdev)) != AMP_ERROR_NONE)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_NET_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - ERROR private data is invalid\n", func_name);
        return retval;
    }
    
    // perform operation
    switch(cmd)
    {
        case NETDRV_BUS_CONTROL_SUSPEND:
            // RTWBUG - finish
            break;
        
        case NETDRV_BUS_CONTROL_RESUME:
            // RTWBUG - finish
            break;
            
        case NETDRV_BUS_CONTROL_RESET:
            // RTWBUG - finish
            break;
            
        case NETDRV_BUS_CONTROL_FLUSH:
            lte_dd_flush_tx_data_fifo(pdev);
            break;

    }
    
    return AMP_ERROR_NONE;
}


/**
 * LTE data driver /proc write function
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

    // NOP writes

    return len;

}

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

    // for HIFd version, no stats to report at this time so just print dummy values as a test
    len = sprintf(page, "%s statistics\n  stat1:%d\n  stat2:%d\n  stat3:%d\n",
        DEVICE_NAME, (int) 44, (int) 12, (int) 109);

    *eof = 1;

    return len;

}


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
    
 
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
        "%s - enter\n", func_name);


    // release the proc entry    
    if (g_proc_entry != NULL)
    {
        remove_proc_entry(DEVICE_NAME, g_proc_entry);
    }

    // if allocated, then delete the device
    if (g_pdev)
    {
        // suspend the net object, this will stop the net driver queue and we
        // should not receive any additional tx packets
        if (g_pdev->pnet && g_pdev->net_drv_bus_interface.net_control)
            g_pdev->net_drv_bus_interface.net_control(g_pdev->pnet, g_pdev, NETDRV_NET_CONTROL_SUSPEND);
        
        // flush and then delete the ptr tx data fifo object
        if (g_pdev->h_tx_data_fifo)
        {
            lte_dd_flush_tx_data_fifo(g_pdev);
            ptr_fifo_free(&g_pdev->h_tx_data_fifo);
        }
        
        // TODO: we need a call here to tell the management path to suspend so
        // that we can drain the mgmt fifos
        
        // flush and then delete the ptr tx mgmt fifo object
        if (g_pdev->h_tx_mgmt_fifo)
        {
            lte_dd_flush_tx_mgmt_fifo(g_pdev);
            ptr_fifo_free(&g_pdev->h_tx_mgmt_fifo);
        }
        
        // flush and then delete the ptr rx mgmt fifo object
        if (g_pdev->h_rx_mgmt_fifo)
        {
            lte_dd_flush_rx_mgmt_fifo(g_pdev);
            ptr_fifo_free(&g_pdev->h_rx_mgmt_fifo);
        }
        
        // delete the net object
        if (g_pdev->pnet)
        {
            net_drv_disconnect(g_pdev->pnet);
            g_pdev->pnet = NULL;
        }
        
#if defined (DO_DYNAMIC_DEV)
        if (lte_dd_class)
        {
            AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
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
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
        "%s - leave\n", func_name);
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
    int first_available = -1;
    char net_dev_name[NETDRV_MAX_DEV_NAME] = {0};
    long retval = AMP_ERROR_NONE;
    long pfval = PTR_FIFO_ERROR_NONE;


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
        "%s - enter\n", func_name);
    
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
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - can't get major %d\n", func_name, g_dev_major);
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
    
    init_MUTEX(&g_pdev->sem);
    init_waitqueue_head(&g_pdev->txq);
    init_waitqueue_head(&g_pdev->rxq);
    
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
        "%s - creating TX FIFO: element size:%d\n", func_name, LTE_DD_TX_DATA_PACKET_FIFO_SIZE);
    
    // create a FIFO object for our TX data packets
    pfval = ptr_fifo_alloc(LTE_DD_TX_DATA_PACKET_FIFO_SIZE, PTR_FIFO_FLAG_NONE, &g_pdev->h_tx_data_fifo);
    if (pfval != PTR_FIFO_ERROR_NONE)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - error %d while creating TX data FIFO object\n",
            func_name, (int) pfval);
        ret = -ENOMEM;
        goto INIT_FAIL;
    
    }
    
    // create a FIFO object for our TX management packets
    pfval = ptr_fifo_alloc(LTE_DD_TX_MGMT_PACKET_FIFO_SIZE, PTR_FIFO_FLAG_NONE, &g_pdev->h_tx_mgmt_fifo);
    if (pfval != PTR_FIFO_ERROR_NONE)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - error %d while creating TX mgmt FIFO object\n",
            func_name, (int) pfval);
        ret = -ENOMEM;
        goto INIT_FAIL;
    
    }
    
    // create a FIFO object for our RX management packets
    pfval = ptr_fifo_alloc(LTE_DD_RX_MGMT_PACKET_FIFO_SIZE, PTR_FIFO_FLAG_NONE, &g_pdev->h_rx_mgmt_fifo);
    if (pfval != PTR_FIFO_ERROR_NONE)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - error %d while creating RX mgmt FIFO object\n",
            func_name, (int) pfval);
        ret = -ENOMEM;
        goto INIT_FAIL;
    
    }
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
        "%s - creating cdev: name:%s; major:%d; minor:%d\n",
        func_name, DEVICE_NAME, g_dev_major, g_dev_minor);
            
    devno = MKDEV(g_dev_major, g_dev_minor);
    cdev_init(&g_pdev->cdev, &g_lte_dd_fops);
    g_pdev->cdev.owner = THIS_MODULE;
    g_pdev->cdev.ops = &g_lte_dd_fops;
    err = cdev_add(&g_pdev->cdev, devno, 1);
    if (err)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - error %d while adding cdev: name:%s; major:%d; minor:%d\n",
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
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - Error in creating lte_dd_class class\n", func_name);
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
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - Error in creating device special file: %s\n", func_name, DEVICE_NAME_DEV);            
        ret = -ENOMEM;
        goto INIT_FAIL;
    }
#endif

    
    // now create a new net driver using the first available index
    first_available = net_drv_device_list_first_avail();
    if (first_available < 0)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - maximum number of net devices allocated\n", func_name);
        ret = -ENOMEM;
        goto INIT_FAIL;
    }
    
    // generate an interface name using this available index
    sprintf(net_dev_name, "lte%d", first_available);
    
    // initialize the "bus" functions were responsible for, the remainder
    // wil be initialized by the network driver probe function
    g_pdev->net_drv_bus_interface.tx_start = lte_dd_tx_data_start;
    g_pdev->net_drv_bus_interface.bus_control = lte_dd_bus_control;

    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
        "%s - creating net device using name:%s\n", func_name, net_dev_name);
    
    // create a new network object using our device as the "bus" object
    retval = net_drv_probe(net_dev_name, g_pdev, &(g_pdev->net_drv_bus_interface), &(g_pdev->pnet));
    if (!g_pdev->pnet)
    {
        AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_ERROR,
            "%s - error creating net device:%s\n", func_name, net_dev_name);
        ret = -ENOMEM;
        goto INIT_FAIL;
    }

    // now create a /proc entry for returning various statistics
    g_proc_entry = create_proc_entry(DEVICE_NAME, 0644, NULL);
    if (g_proc_entry != NULL)
    {
        g_proc_entry->read_proc = ltedd_proc_read;
        g_proc_entry->write_proc = ltedd_proc_write;
        g_proc_entry->owner = THIS_MODULE;
    }


    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
        "%s - leave\n", func_name);
        
    return 0;
    
 
  
 INIT_FAIL:
    lte_dd_cleanup_module();
    
    AMP_LOG_Print(AMP_LOG_ADDR_LTE_DATA_DRIVER, AMP_LOG_LEVEL_WARNING,
        "%s - leave, error:%d\n", func_name, ret);
    return ret;

}


// export the init and cleanup functions
module_init(lte_dd_init_module);
module_exit(lte_dd_cleanup_module);

MODULE_DESCRIPTION("Motorola LTE Data Driver");
MODULE_LICENSE("GPL");

