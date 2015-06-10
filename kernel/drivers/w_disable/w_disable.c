/*
 * Copyright (C) 2009-2011 Motorola and Motorola Mobility, Inc.
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
 * Motorola 2011-Mar-22 - Shutdown Datacard upon W_DISABLE assert.
 * Motorola 2009-Aug-26 - Workaroud to avoid BP over writing to the
 *                        IOMUX configuration register for GPIO_10
 * Motorola 2009-Jun-04 - W_DISABLE support for W3G
 *                        Initial creation.
 */

/*
 * This file includes all of the initialization and tear-down code for the W_DISABLE
 * low-level driver and the basic user-mode interface (open(), close(), poll(), etc.).
 */


#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/types.h>

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqflags.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/wait.h>

#include <linux/gpio.h>
#include <linux/w_disable.h>
#include <linux/spi/cpcap.h>

/******************************************************************************
 * Global Variables
 *****************************************************************************/


/******************************************************************************
* Constants
******************************************************************************/



/******************************************************************************
 * Local Macros
 *****************************************************************************/
/*
  W_DISABLE MODULE TRACEMSG MACRO

  IMPORTANT NOTE(S) :

  Do not enable logging by default. Only use this for debug purposes.
*/
#if 0
#define tracemsg(fmt,args...)  printk(fmt,##args)
#else
#define tracemsg(fmt,args...)
#endif

/* REGISTER ACCESS MACROS

  IMPORTANT NOTE(S) :

  1) to be used only for W_DISABLE module register access
*/
#define read_reg(reg)                   (*(reg))
#define write_reg(reg, value)           (*(reg) = (value))
#define write_reg_bits(reg, mask, bits) (*(reg) = ( *(reg) & ~(mask)) | (bits));


/* IOMUX reg for W_DISABLE */
#define CONTROL_PADCONF_GPIO_9   IO_ADDRESS(OMAPW3G_SCM_BASE + 0x0150)
#define CONTROL_PADCONF_GPIO_82   IO_ADDRESS(0x5C1821F0)

/* GPIO for W_DISABLE_INT */
#define GPIO_SIGNAL_W_DISABLE_INT 10

/* For GPIO_82 */
#define GPIO_SIGNAL_82            82

/* GPIO3 Clock */
#define GPIO3_FCK "gpio3_dbck"


/******************************************************************************
* Local function prototypes
******************************************************************************/
static int w_disable_open(struct inode *inode, struct file *file);
static int w_disable_free(struct inode *inode, struct file *file);
static int w_disable_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static unsigned int w_disable_poll(struct file *file, poll_table *wait);
static irqreturn_t w_disable_int_irq(int irq, void *dev_id);
static int w_disable_debounce_thread(void * unused);
#ifndef FEAT_GNPO_W_DISABLE
void w_disable_power_down(unsigned long unused);
int w_disable_prepare_for_pd(void);
#endif
static struct class *w_disable_class;
struct clk *w_disable_gpio3_clk;

/******************************************************************************
* Local Structures
******************************************************************************/

/*This structure defines the file operations for the w_disable device */
static struct file_operations w_disable_fops =
{
    .owner =    THIS_MODULE,
    .open =     w_disable_open,
    .release =  w_disable_free,
    .poll =     w_disable_poll,
    .ioctl =    w_disable_ioctl,
};


/******************************************************************************
* Local variables
******************************************************************************/
static int w_disable_module_major;
static unsigned int w_disable_event = W_DISABLE_DE_ASERT_EVENT;
static unsigned int previous_w_disable_event = 2;
static wait_queue_head_t w_disable_module_wait = __WAIT_QUEUE_HEAD_INITIALIZER(w_disable_module_wait);
static wait_queue_head_t w_disable_db_wait = __WAIT_QUEUE_HEAD_INITIALIZER(w_disable_db_wait);;
static unsigned char w_disable_num_opens = 0;
static DECLARE_MUTEX(w_disable_module_lock);
static unsigned char w_disable_db_pending = 0;
static int w_disable_unused = 0;
#ifndef FEAT_GNPO_W_DISABLE
static struct timer_list dereg_wdt;
#endif
/******************************************************************************
* Local Functions
******************************************************************************/
/*

DESCRIPTION:
    This function is the interrupt service routine that handles the interrupt of W_DISABLE.

INPUTS:
    int irq       : the interrupt request number
    void * dev_id : pointer to the device associated with the interrupt

OUTPUT:
    irqreturn_t status : a status indicating if an interrupt was handled successfully

IMPORTANT NOTES:
    None

*/
static irqreturn_t w_disable_int_irq(int irq, void *dev_id)
{
    /* Read the event from the GPIO_10, W_DISABLE is active low */
#ifndef FEAT_GNPO_W_DISABLE
    w_disable_db_pending = 1;
    /* wake up the debouncing thread */
    wake_up_interruptible(&w_disable_db_wait);
#endif
    return(IRQ_RETVAL(1));
}

/* DESCRIPTION:
       The ioctl () handler for the W_DISABLE driver

   INPUTS:
       inode       inode pointer
       file        file pointer
       cmd         the ioctl() command
       arg         the ioctl() argument

   OUTPUTS:
       Returns status: the status of the request
       arg : the status of the GPIO pin

   IMPORTANT NOTES:
       None.
*/
static int w_disable_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int status = 0;
    unsigned char gpio_status;

    gpio_status = (0x1 & (~gpio_get_value(GPIO_SIGNAL_W_DISABLE_INT)));

    switch (cmd)
    {
        case W_DISABLE_IOCTL_GPIO_STATUS:
            status = copy_to_user ((unsigned char *)arg, &gpio_status, sizeof(gpio_status));
            break;
        case W_DISABLE_IOCTL_RF_OFF:
#ifndef FEAT_GNPO_W_DISABLE
	    w_disable_power_down(0);
#endif
            break;

        default:
            printk("Warning: Invalid request sent to the W_DISABLE driver.\n");
            status = -ENOTTY;
            break;
    }

    return status;
}
/* DESCRIPTION:
       The poll() handler for the W_DISABLE driver

   INPUTS:
       file        file pointer
       wait        poll table for this poll()

   OUTPUTS:
       Returns POLLIN if w_disable is asserted and retuns POLLOUT if w_disable is de-asserted.

   IMPORTANT NOTES:
       None.
*/
static unsigned int w_disable_poll(struct file *file, poll_table *wait)
{
    unsigned int retval = 0;

    if (w_disable_event == previous_w_disable_event)
    {
        /* Add our wait queue to the poll table */
        poll_wait(file, &w_disable_module_wait, wait);
    }

    if (w_disable_event != previous_w_disable_event)
    {
        if(w_disable_event == W_DISABLE_ASERT_EVENT)
            retval = POLLIN;
        if(w_disable_event == W_DISABLE_DE_ASERT_EVENT)
            retval = POLLOUT;

        previous_w_disable_event = w_disable_event;
    }

    return retval;
}


/* DESCRIPTION:
       The open() handler for the W_DISABLE device.

   INPUTS:
       inode       inode pointer
       file        file pointer

   OUTPUTS:
       Returns 0 if successful.

   IMPORTANT NOTES:
       None.
*/
static int w_disable_open(struct inode *inode, struct file *file)
{
    int status = -ENODEV;

    down(&w_disable_module_lock);

    w_disable_num_opens++;

    if (w_disable_num_opens == 1) {
      status = 0;
      printk("w_disable : w_disable_open, status -> %X\n", status);
    }

    up (&w_disable_module_lock);
    return status;
}

/*
 * This thread debounces the w_disable signal
 */
static int w_disable_debounce_thread(void * unused)
{
     unsigned char num_matching_samples = 0;
     unsigned long previous_db_event = 0xFF;
     unsigned long current_db_event = 0;
#ifndef FEAT_GNPO_W_DISABLE
     int status;
#endif

     while (!kthread_should_stop())
     {
         /* wait for an interrupt event */
         wait_event_interruptible(w_disable_db_wait, (w_disable_db_pending == 1));

	 tracemsg ("w_disable : debouncing the signal\n");

	 do
	 {
	     current_db_event = (0x1 & gpio_get_value(GPIO_SIGNAL_W_DISABLE_INT));

	     if (current_db_event == previous_db_event)
	     {
		 num_matching_samples++;
	     }
	     else
	     {
		 num_matching_samples = 0;
	     }

	     previous_db_event = current_db_event;

	     msleep (10);
	 } while (num_matching_samples < 20) ;

	 w_disable_event = current_db_event;

	 /* if the status reported to user space is now different than
            was previously reported to user space... */
	 if (w_disable_event != previous_w_disable_event)
	 {

#ifndef FEAT_GNPO_W_DISABLE
	     /* this function needs to executed before notifying user space
                to ensure it is executed before user space can run.
	     */
	     status = w_disable_prepare_for_pd();
#endif
	     /* wake up user space */
	     wake_up_interruptible(&w_disable_module_wait);

#ifndef FEAT_GNPO_W_DISABLE
	     if (status < 0) {
	       sys_sync();
	       panic ("w_disable failed to prepare for power down: status = %d", status);
	     }

	     /* start the 3 second deregistration watchdog timer */
	     setup_timer (&dereg_wdt, w_disable_power_down, 0);
	     status = mod_timer (&dereg_wdt, jiffies + msecs_to_jiffies(3000));

	     /* if the timer setup fails ... */
	     if (status < 0)
	     {
	         sys_sync();
		 panic ("w_disable error : deregistration watchdog failed to initialize. force a panic. status = %d", status);
	     }
#endif
	 }

	 tracemsg ("w_disable : completed debouncing the signal -> event : %X\n", w_disable_event);

	 previous_db_event = 0xFF;
	 current_db_event = 0;
	 num_matching_samples = 0;

	 w_disable_db_pending = 0;
     }
     /* should not reach this */
     return 0;
}

#ifndef FEAT_GNPO_W_DISABLE
/* DESCRIPTION:
       This function powers down the device in response to w_disable

   INPUTS:
       unsigned long unused : required for timer callback functions,
                              but the data passed in is not needed
			      in this function.

   OUTPUTS:
       None

   IMPORTANT NOTES:
       1. This function is run in interrupt context and task context
             be careful about making changes. They must also function
             in interrupt context.
       2. This function does not return. It will power down the device.
       3. cpcap_vbusswitch_set must be called before executing this function for
            to reduce current drain while powered down due to leakage from CPCAP.
*/
void w_disable_power_down(unsigned long unused)
{
     /* Configure for GPIO_82 for Shutdown */
     write_reg_bits((volatile unsigned int *)CONTROL_PADCONF_GPIO_82, (0xFFFFFFFF), (0x00020002));

     sys_sync();

     clk_enable(w_disable_gpio3_clk);

     /* gpio_82 powers down the device by driving sys_npwrdown low */
     gpio_set_value(GPIO_SIGNAL_82,0);

     while (1) {
       /* wait for power down */
     }
}

/* DESCRIPTION:
       This function prepares the system for powering down

   INPUTS:
       None.

   OUTPUTS:
       int status : the status of the powerdown preparation
                    returns a value < 0 if it failed.

   IMPORTANT NOTES:
       1. This function is not safe to execute in interrupt context
       2. This function must be called before calling
          w_disable_power_down.
*/
int w_disable_prepare_for_pd()
{
   int status;
   /* Open the VBUS switch in CPCAP to eliminate current drain
      from voltage comparators. Must be executed here because
      this function will crash in interrupt context due to
      the scheduler being called in interrupt context in the
      SPI driver when the CPCAP write is done. Also, this must
      be called before starting the timer to ensure the CPCAP
      update went through before the device is powered down.
   */
   cpcap_vbusswitch_set(0);

   /* get the gpio3 clock, and configure the sys_npwrdown GPIO. Done
      in task context because gpio_direction_output has functions
      executed from it that will call the scheduler.
   */
   w_disable_gpio3_clk = clk_get(NULL, GPIO3_FCK);
   clk_enable(w_disable_gpio3_clk);

   status = gpio_request(GPIO_SIGNAL_82, NULL);
   if (status == 0) {
     status = gpio_direction_output(GPIO_SIGNAL_82, 1);
   }

   clk_disable(w_disable_gpio3_clk);

   return status;
}
#endif

/* DESCRIPTION:
       The close() handler for the W_DISABLE device

   INPUTS:
       inode       inode pointer
       file        file pointer

   OUTPUTS:
       Returns 0 if successful.

   IMPORTANT NOTES:
       None.
*/
static int w_disable_free(struct inode *inode, struct file *file)
{
    down (&w_disable_module_lock);

    w_disable_num_opens--;

    if (w_disable_num_opens == 0)
    {
        free_irq(OMAP_GPIO_IRQ(GPIO_SIGNAL_W_DISABLE_INT), NULL);
        gpio_free(GPIO_SIGNAL_W_DISABLE_INT);
        tracemsg("w_disable : w_disable_free()\n");
    }

    up (&w_disable_module_lock);
    return 0;
}


/* DESCRIPTION:
       The W_DISABLE intialization function.

   INPUTS:
       None.

   OUTPUTS:
       Returns 0 if successful and returns an error code upon failure.

   IMPORTANT NOTES:
       None.
*/

int __init w_disable_init(void)
{
    int ret = 0;
    struct device *temp_class;

    w_disable_module_major = register_chrdev(0, W_DISABLE_DEV_NAME, &w_disable_fops);
    if (w_disable_module_major < 0) {
      tracemsg("w_disable_init: Unable to get a major for w_disable driver\n");
      return w_disable_module_major;
    }

    w_disable_class = class_create(THIS_MODULE, W_DISABLE_DEV_NAME);
    if (IS_ERR(w_disable_class)) {
      unregister_chrdev(w_disable_module_major, W_DISABLE_DEV_NAME);
      printk("w_disable_init: Error creating w_disable class.\n");
      ret = PTR_ERR(w_disable_class);
      return ret;
    }

    temp_class = device_create(w_disable_class, NULL, MKDEV(w_disable_module_major, 0), NULL, W_DISABLE_DEV_NAME);

    if (IS_ERR(temp_class)) {
      class_destroy(w_disable_class);
      unregister_chrdev(w_disable_module_major, W_DISABLE_DEV_NAME);
      printk("w_disable_init: Error creating w_disable class device.\n");
      ret = PTR_ERR(temp_class);
      return ret;
    }

    /* Request GPIO */
    ret = gpio_request(GPIO_SIGNAL_W_DISABLE_INT, NULL);
    if (ret != 0) {
      printk("Error : The  W_DISABLE driver failed to request a GPIO. GPIO -> %X Error -> %X\n", GPIO_SIGNAL_W_DISABLE_INT, ret);
    }
    else {
#ifdef FEAT_GNPO_W_DISABLE
      /* Configure GPIO_10 for W_DISABLE */
      write_reg_bits((volatile unsigned int *)CONTROL_PADCONF_GPIO_9, (0xFFFF0000), (0x41180000));

      /* Set GPIO Direction */
      gpio_direction_input(GPIO_SIGNAL_W_DISABLE_INT );
#endif
      /* set irq type Enable for both Edge */
      set_irq_type(OMAP_GPIO_IRQ(GPIO_SIGNAL_W_DISABLE_INT), IRQ_TYPE_EDGE_BOTH);

      /* request irq */
      ret = request_irq(OMAP_GPIO_IRQ(GPIO_SIGNAL_W_DISABLE_INT), w_disable_int_irq, (IRQF_DISABLED | IRQF_NOBALANCING), W_DISABLE_DEV_NAME, 0);

      if ( ret != 0 ) {
	printk("Error: the W_DISABLE driver failed to request an IRQ. IRQ -> %X Error -> %X\n", GPIO_SIGNAL_W_DISABLE_INT, ret);
      }
      else {
	/* Read the initial status of GPIO_10, W_DISABLE is active low  */
	w_disable_event = (0x1 & gpio_get_value(GPIO_SIGNAL_W_DISABLE_INT));
	kthread_run(w_disable_debounce_thread, (void *)&w_disable_unused, "w_disable_debounce");
      }
    }

    tracemsg("w_disable_init: w_disable Module successfully initialized \n");
    return ret;
}


/* DESCRIPTION:
       The W_DISABLE device cleanup function

   INPUTS:
       None.

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       None.
*/
static void __exit w_disable_exit(void)
{
    device_destroy(w_disable_class, MKDEV(w_disable_module_major, 0));
    class_destroy(w_disable_class);
    unregister_chrdev(w_disable_module_major, W_DISABLE_DEV_NAME);
    gpio_free(GPIO_SIGNAL_W_DISABLE_INT);
    tracemsg("w_disable_exit: driver successfully unloaded.\n");
}


/*
 * Module entry points
 */
module_init(w_disable_init);
module_exit(w_disable_exit);

MODULE_DESCRIPTION("W_DISABLE driver");
MODULE_AUTHOR("Motorola");
MODULE_LICENSE("GPL");
