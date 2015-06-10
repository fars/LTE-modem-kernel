/*
 * Copyright (C) 2011 Motorola Mobility, Inc.
 */

/*
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
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/ipc_vr_shortmsg.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <mach/dma.h>

#include "modem_profiler.h"

/******************************************************************************
 * Global Variables
 *****************************************************************************/

/******************************************************************************
* Constants
******************************************************************************/

/******************************************************************************
 * Local Macros
 *****************************************************************************/
/* Configure DMA data transfer ( memory to memory ) */
#define OMAP_DMA_MEM2MEM_RCV(ch, src, dest, len)                        \
    omap_set_dma_transfer_params(ch, OMAP_DMA_DATA_TYPE_S32, len, 1, OMAP_DMA_SYNC_FRAME, 0, 0); \
    omap_set_dma_dest_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_src_burst_mode(ch, OMAP_DMA_DATA_BURST_32); \
    omap_set_dma_write_mode(ch,OMAP_DMA_WRITE_LAST_NON_POSTED); \
    omap_set_dma_prefetch_mode(ch, OMAP_DMA_PREFETCH_EN); \
    omap_set_dma_src_data_pack(ch, 0); \
    omap_set_dma_dest_data_pack(ch, 0); \
    omap_set_dma_dest_params(ch, 0, OMAP_DMA_AMODE_POST_INC, dest, 0, 0);     \
    omap_set_dma_src_params(ch, 0, OMAP_DMA_AMODE_POST_INC, src, 0, 0);       \
    omap_set_dma_src_endian_type(ch,OMAP_DMA_BIG_ENDIAN);\
    omap_set_dma_dst_endian_type(ch,OMAP_DMA_LITTLE_ENDIAN);

/******************************************************************************
* Local function prototypes
******************************************************************************/
static int modem_profiler_open(struct inode *inode, struct file *file);
static int modem_profiler_free(struct inode *inode, struct file *file);
static int modem_profiler_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static unsigned int modem_profiler_poll(struct file *file, poll_table *wait);
void modem_profiler_get_state_times (void);
void modem_profiler_dma_callback(int lch, u16 ch_status, void *data);
/******************************************************************************
* Local Structures
******************************************************************************/

/*This structure defines the file operations for the Modem Profiler device */
static struct file_operations modem_profiler_fops =
{
    .owner =    THIS_MODULE,
    .open =     modem_profiler_open,
    .release =  modem_profiler_free,
    .poll =     modem_profiler_poll,
    .ioctl =    modem_profiler_ioctl,
};

typedef struct {
	unsigned long * buffer;
	dma_addr_t dma_buffer;
} MODEM_PROFILER_BUFFER_DATA;
/******************************************************************************
* Local variables
******************************************************************************/
static int modem_profiler_module_major = 0;
static int modem_profiler_event = MODEM_PROFILER_EVENT_NONE;
static unsigned char modem_profiler_num_opens = 0;
static wait_queue_head_t modem_profiler_wait = __WAIT_QUEUE_HEAD_INITIALIZER(modem_profiler_wait);
static wait_queue_head_t modem_profiler_db_wait = __WAIT_QUEUE_HEAD_INITIALIZER(modem_profiler_db_wait);
static DECLARE_MUTEX(modem_profiler_module_lock);
static DECLARE_MUTEX(modem_profiler_event_lock);
static struct class *modem_profiler_class;
static unsigned long modem_profiler_state_times[MODEM_PROFILER_MODEM_STATES_END] = { 0 };
static MODEM_PROFILER_USB_STATE modem_profiler_usb_state = MODEM_PROFILER_USB_STATE_ACTIVE;
static int modem_profiler_dma_channel = 0;
static  MODEM_PROFILER_BUFFER_DATA modem_profiler_buffer;
static bool  modem_profiler_dma_complete = false;
static u16 modem_profiler_dma_error = 0;
static struct timespec modem_profiler_last_update;
/******************************************************************************
* Local Functions
******************************************************************************/
/* DESCRIPTION:
       The ioctl () handler for the Modem Profiler driver

   INPUTS:
       inode       inode pointer
       file        file pointer
       cmd         the ioctl() command
       arg         the ioctl() argument

   OUTPUTS:
       Returns status: the status of the request
 
   IMPORTANT NOTES:
       None.
*/
static int modem_profiler_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int status = 0;

    switch (cmd)
    {
        case MODEM_PROFILER_GET_STATE_TIME_VALUES:
   	   modem_profiler_get_state_times();
	   status = copy_to_user ((unsigned long *)arg, modem_profiler_state_times, 
				   (MODEM_PROFILER_MODEM_STATES_END * 4));
	   break;
        default:
           printk("Invalid request sent to the Modem Profiler driver.\n");
           status = -ENOTTY;
           break;
    }

    return status;
}
/* DESCRIPTION:
       The poll() handler for the Modem Profiler driver

   INPUTS:
       file        file pointer
       wait        poll table for this poll()

   OUTPUTS:
    
   IMPORTANT NOTES:
       None.
*/
static unsigned int modem_profiler_poll(struct file *file, poll_table *wait)
{
    unsigned int retval = 0;
    if (modem_profiler_event == MODEM_PROFILER_EVENT_NONE) {
       printk ("Modem Profiler waiting ... \n");
       /* Add our wait queue to the poll table */
       poll_wait(file, &modem_profiler_wait, wait);
    }

    if (modem_profiler_event != MODEM_PROFILER_EVENT_NONE) {
       printk ("Modem Profiler event %d received \n", modem_profiler_event);
       modem_profiler_event = MODEM_PROFILER_EVENT_NONE;
       retval = POLLIN;
    }
    return retval;
}


/* DESCRIPTION:
       The open() handler for the Modem Profiler device.

   INPUTS:
       inode       inode pointer
       file        file pointer

   OUTPUTS:
       Returns 0 if successful.

   IMPORTANT NOTES:
       None.
*/
static int modem_profiler_open(struct inode *inode, struct file *file)
{
    int status = -ENODEV;

    down(&modem_profiler_module_lock);

    modem_profiler_num_opens++;

    if (modem_profiler_num_opens == 1) {
      status = 0;
      printk("modem_profiler_open, status -> %X\n", status);
    }

    up (&modem_profiler_module_lock);
    return status;
}


/* DESCRIPTION:
       The close() handler for the Modem Profiler device

   INPUTS:
       inode       inode pointer
       file        file pointer

   OUTPUTS:
       Returns 0 if successful.

   IMPORTANT NOTES:
       None.
*/
static int modem_profiler_free(struct inode *inode, struct file *file)
{
    down (&modem_profiler_module_lock);

    dma_free_coherent(NULL, (MODEM_PROFILER_MODEM_STATES_END * 4),
		      modem_profiler_buffer.buffer, modem_profiler_buffer.dma_buffer);

    omap_stop_dma(modem_profiler_dma_channel);
    omap_free_dma(modem_profiler_dma_channel);

    modem_profiler_num_opens--;

    if (modem_profiler_num_opens == 0)
    {
         printk("modem_profiler_free()\n");
    }
    up (&modem_profiler_module_lock);
    return 0;
}

int __init modem_profiler_init(void)
{
    int ret = 0;
    struct device *temp_class;

    modem_profiler_last_update.tv_sec = 0;
    modem_profiler_last_update.tv_nsec = 0;

    modem_profiler_module_major = register_chrdev(0, MODEM_PROFILER_DEV_NAME, &modem_profiler_fops);
    if (modem_profiler_module_major < 0) {
      printk ("Unable to get a major for vr driver, %d\n", modem_profiler_module_major);
      return modem_profiler_module_major;
    }

    modem_profiler_class = class_create(THIS_MODULE, MODEM_PROFILER_DEV_NAME);

    if (IS_ERR(modem_profiler_class)) {
      unregister_chrdev(modem_profiler_module_major, MODEM_PROFILER_DEV_NAME);
      printk("Error creating vr class.\n");
      ret = PTR_ERR(modem_profiler_class);
      return ret;
    }

    temp_class = device_create(modem_profiler_class, NULL, MKDEV(modem_profiler_module_major, 0), NULL, MODEM_PROFILER_DEV_NAME);

    if (IS_ERR(temp_class)) {
      class_destroy(modem_profiler_class);
      unregister_chrdev(modem_profiler_module_major, MODEM_PROFILER_DEV_NAME);
      printk("Error creating vr class device.\n");
      ret = PTR_ERR(temp_class);
      return ret;
    }

    modem_profiler_buffer.buffer = (unsigned long *)dma_alloc_coherent(NULL, (MODEM_PROFILER_MODEM_STATES_END * 4),
								       &(modem_profiler_buffer.dma_buffer), 0);
    ret = omap_request_dma(0, MODEM_PROFILER_DEV_NAME, modem_profiler_dma_callback,
			    (void *) &(modem_profiler_buffer.dma_buffer), &modem_profiler_dma_channel);

    OMAP_DMA_MEM2MEM_RCV(modem_profiler_dma_channel,
			 (unsigned long) ipc_virt_reg_bank[IPC_VR_MP_ADDR_REG],
			 modem_profiler_buffer.dma_buffer, (MODEM_PROFILER_MODEM_STATES_END));

    if (ret == 0) {
        printk ("Modem Profiler module successfully initialized \n");
    }

    return ret;
}


/* DESCRIPTION:
       The Modem Profiler device cleanup function

   INPUTS:
       None.

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       None.
*/
static void __exit modem_profiler_exit(void)
{
    device_destroy(modem_profiler_class, MKDEV(modem_profiler_module_major, 0));
    class_destroy(modem_profiler_class);
    unregister_chrdev(modem_profiler_module_major, MODEM_PROFILER_DEV_NAME);
    printk("Modem Profiler driver successfully unloaded.\n");
}

/* DESCRIPTION:
       The Modem Profiler event set function. Called by the USB driver to set events to
       wake user space.

   INPUTS:
       int event - event number

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       Run from interrupt context. Must not use prink or block or schedule in
       this function.
*/
void modem_profiler_set_event(int event)
{
   struct timespec curr_time;

   curr_time = CURRENT_TIME;

   if (event != MODEM_PROFILER_EVENT_NONE) {
      /* we are limiting the rate, this function could be called frequently */
      if (((modem_profiler_last_update.tv_sec == 0) && (modem_profiler_last_update.tv_nsec == 0)) ||
	  ((curr_time.tv_sec - modem_profiler_last_update.tv_sec) >= 30))  {
         modem_profiler_last_update.tv_sec = curr_time.tv_sec;
         modem_profiler_event |= event;
         wake_up_interruptible(&modem_profiler_wait);
      }
   }
}

/* DESCRIPTION:
       The Modem Profiler set state function. Called by the USB driver to set the state to
       the new state.

   INPUTS:
       bool state - USB state (Suspended - 0, Active - 1)

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       Run from interrupt context. Must not use prink or block or schedule in
       this function.
*/
void modem_profiler_set_usb_state (bool state)
{
   mailbox_vr_write_usb_status(state);
   modem_profiler_usb_state = state;
   if (state == true) {
      modem_profiler_set_event(MODEM_PROFILER_EVENT_GET_DATA);
   }
}

/* DESCRIPTION:
       The Modem Profiler set usb data notification. This function is executed
       by the USB driver to notify modem profiler of data going out of usb.
       We only want to collect and push data when there are other reasons to
       wake the AP.

   INPUTS:
       None.

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       None.
*/
void modem_profiler_usb_data_notify ()
{
   modem_profiler_set_event(MODEM_PROFILER_EVENT_GET_DATA);
}


/* DESCRIPTION:
       The Modem Profiler get state time data function. Called via IOCTL by user space to get the time data

   INPUTS:
       None

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       None.
*/
void modem_profiler_get_state_times (void)
{
   int loop_count = 0;

   modem_profiler_dma_complete = false;
   modem_profiler_dma_error = 0;

   /* force an update of the current state on the C55 ... */
   mailbox_short_msg_tx_and_wait(IPC_VR_SHORT_MSG_VIRT_REG_WRITE, IPC_VR_MP_UPDATE_TIMES);

   omap_start_dma(modem_profiler_dma_channel);

   /* wait for DMA to complete */
   while ((modem_profiler_dma_complete == false) && (loop_count < 1000)) {
     msleep (1);
     loop_count++;
   }

   omap_stop_dma(modem_profiler_dma_channel);
}

/* DESCRIPTION:
       The Modem Profiler DMA callback function

   INPUTS:
       int lch - the logical channel which generated the interrupt
                 since we have only 1 transfer at a time this is
                 unused.
       u16 ch_status - the status of the logical channel
       void *data - extra data unused by this driver

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       None.
*/
void modem_profiler_dma_callback(int lch, u16 ch_status, void *data)
{
   if (ch_status == 32) {
      memcpy (modem_profiler_state_times,
	      modem_profiler_buffer.buffer, MODEM_PROFILER_MODEM_STATES_END * 4);
   }
   else {
      printk ("modem profiler dma channel error %d \n", ch_status);
   }

   modem_profiler_dma_complete = true;
   modem_profiler_dma_error = ch_status;
}
/*
 * Module entry points
 */
module_init(modem_profiler_init);
module_exit(modem_profiler_exit);

MODULE_DESCRIPTION("Modem Profiler driver");
MODULE_AUTHOR("Motorola");
MODULE_LICENSE("GPL");
