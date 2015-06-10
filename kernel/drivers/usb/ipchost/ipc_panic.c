/*
 * Copyright (C) 2009 Motorola, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * 
 * Changelog:
 * Date             Author          Comment
 * -----------------------------------------------------------------------------
 * 04-Jun-2009      Motorola        MB-IPC initial for W3G
 */
 
/*!
 * @file drivers/usb/ipchost/ipc_panic.c
 * @brief AP Char driver for Mailbox to userspace for panic logging
 *
 * This is the generic portion of the MB-IPC-PANIC driver.
 *
 * @ingroup IPCFunction
 */


/*=============================================================================
................................INCLUDE FILES
=============================================================================*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <asm/delay.h>
#include <linux/ipc_api.h>


/*=============================================================================
                                LOCAL CONSTANTS
=============================================================================*/

/**
 * Identifier of the driver. Used in all kernel APIs requiring a text
 * string to identify the driver, and in creating the devfs entry
 */
#define BPPANIC_ID    "bppanic"

/* #define IPC_DEBUG */

#ifdef IPC_DEBUG
#define DPRINTK(fmt, args...) printk(KERN_INFO "%s: " fmt, __FUNCTION__ ,\
                                       ## args)
#define DEBUG_PANIC_DRV_EXTRA(args...) printk(args)
#else
#define DEBUG_PANIC_DRV_EXTRA(args...)
#define DPRINTK(fmt, args...)
#endif


/*=============================================================================
                                LOCAL MACROS
=============================================================================*/
#define MAXFIFOSIZE 65535
#define PANIC_HEADER (0xABCD << 16)


/*=============================================================================
                        LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
=============================================================================*/

typedef enum {
	BPP_IDLE,   /* Initial state  */
	BPP_HEADER, /* Get the header */
	BPP_FILL,   /* Write in FIFO  */
} bpp_state;

/**
 * Internal representation of the bppanic driver.
 */
typedef struct {
	struct semaphore sem;		/**< Semaphore used to serialize I/O operations */
	unsigned char openFlag;		/**< Bit indicating the driver is being used by user-space */
	wait_queue_head_t waitQueue;	/**< Wait queue used for sleeping */
	unsigned char drv_buf[MAXFIFOSIZE];		/**< Drivers internal buffer */
	unsigned int resp_size;			/** size of current response */
	unsigned int data_num;			/** No. of bytes after current bytes have been copied */
	unsigned int read_num;          /* No. of bytes read by userspace */
	atomic_t state;

} BPPanicDriverType;


/*=============================================================================
                              LOCAL FUNCTION PROTOTYPES
=============================================================================*/

/**
 * \brief Initializes the bppanic module when it's loaded into kernel
 *
 * \param None
 *
 * \return Return Type \n
 * - Zero - Success. \n
 * - Non-Zero - An error has occurred while initializing the driver. \n
 */
static int __init bppanicInit(void);

/**
 * \brief Cleans up the bppanic module when it's unloaded from kernel
 *
 * \param None
 *
 * \return None
 *
 */
static void __exit bppanicExit(void);

/**
 * \brief Callback function when user-space invokes open()
 *
 * \param inode     - Pointer to inode structure
 * \param filp      - Pointer to open file structure
 *
 * \return Return Type \n
 * - Zero     - Success
 * - Non-Zero - An error has occurred
 */
static int bppanicOpen(struct inode *inode, struct file *filp);

/**
 * \brief Callback function when user-space invokes close()
 *
 * \param inode     - Pointer to inode structure
 * \param filp      - Pointer to open file structure
 *
 * \return Return Type \n
 * - Zero     - Success
 * - Non-Zero - An error has occurred
 */
static int bppanicRelease(struct inode *inode, struct file *filp);


/**
 * \brief Callback function when user-space invokes select()
 *
 * \param filp      - Pointer to open file structure
 * \param wait      - Opaque structure passed to kernel
 *
 * \return Return Type \n
 * - POLLIN - Data available for read
 * - 0 - No data available
 */
static unsigned int bppanicPoll(struct file *filp, poll_table * wait);

/**
 * \brief Callback function when user-space invokes read()
 *
 * \param filp      - Pointer to open file structure
 * \param buf      - destination userspace buffer address
 * \param count   - number of bytes userspace wants to read
 * \param l          - offset
 *
 * \return Return Type \n
 * - Success - No. of bytes read 
 * - Error - Error occured
 */
static ssize_t bppanicRead(struct file *filip, char __user *buf, size_t count, loff_t *l);
/**
 * \brief Callback function when user-space invokes write()
 *
 * \param filp      - Pointer to open file structure
 * \param buf      - source userspace buffer address
 * \param count   - number of bytes userspace wants to read
 * \param l          - offset
 *
 * \return Return Type \n
 * - Success - No. of bytes read 
 * - Error - Error occured
 */

static ssize_t bppanicWrite(struct file *filip, const char __user *buf, size_t count, loff_t *l);


/*=============================================================================
                                GLOBAL VARIABLES
=============================================================================*/

/**
 * This structure exposes the available I/O
 * operations of this driver to the kernel.
 * Each member points to a corresponding function
 */
static struct file_operations bppanicFops = {
	.owner = THIS_MODULE,
	.open = bppanicOpen,
	.release = bppanicRelease,
	.read = bppanicRead,
	.write = bppanicWrite,
	.poll = bppanicPoll,
};

static struct class *bppanic_class;
struct device *bppanic_device_class;
static int major_bppanic;
static BPPanicDriverType bpp;	/**< Internal structure of bppanic driver */


/*=============================================================================
                                LOCAL FUNCTIONS
=============================================================================*/

static int __init bppanicInit(void)
{
	/* Allocate a major number for the character device */
	major_bppanic = register_chrdev(0, "bppanic", &bppanicFops);
	if (major_bppanic < 0) {
		printk(KERN_INFO "Unable to get a major for bppanic\n");
		return major_bppanic;
	}

	bppanic_class = class_create(THIS_MODULE, "bppanic");
	if (IS_ERR(bppanic_class)) {
		printk(KERN_INFO "Not able to do the class_create\n");
		unregister_chrdev(major_bppanic, "bppanic");
		return (int) bppanic_class;
	}

	bppanic_device_class =
	    device_create(bppanic_class, NULL, MKDEV(major_bppanic, 0),
			  NULL, "bppanic");
	if (IS_ERR(bppanic_device_class)) {
		class_destroy(bppanic_class);
		unregister_chrdev(major_bppanic, "bppanic");
		printk(KERN_INFO
		       "Not able to do the class_device_create\n");
		return (int) bppanic_device_class;
	}

	/* Initialize internal structures */

	/* Initialize the buffer */
	memset((void *)bpp.drv_buf, 0, MAXFIFOSIZE);

	bpp.openFlag = 0;
	bpp.resp_size = 0;
	bpp.data_num = 0;
	bpp.read_num = 0;

	atomic_set(&bpp.state, BPP_IDLE);
	init_MUTEX(&bpp.sem);
	init_waitqueue_head(&bpp.waitQueue);

	printk(KERN_INFO "Loaded BP Panic device driver\n");

	return 0;
}

static void __exit bppanicExit(void)
{
	if (major_bppanic >= 0) {
		device_destroy(bppanic_class, MKDEV(major_bppanic, 0));
		class_destroy(bppanic_class);
		unregister_chrdev(major_bppanic, "bppanic");
	}

	printk(KERN_INFO "Unloaded BP Panic device driver\n");
}

static int bppanicOpen(struct inode *inode, struct file *filp)
{
	/* Hold the semaphore */
	if (down_interruptible(&bpp.sem)) {
		return -ERESTARTSYS;
	}

	/* Checks if device is already opened */
	if (bpp.openFlag) {
		up(&bpp.sem);
		return -EBUSY;
	}
	DPRINTK("bppanic opened\n");
	/* Sets the flag to open */
	bpp.openFlag = 1;

	/* Release the semaphore and return */
	up(&bpp.sem);

	return 0;
}

static int bppanicRelease(struct inode *inode, struct file *filp)
{
	/* Hold the semaphore */
	if (down_interruptible(&bpp.sem)) {
		return -ERESTARTSYS;
	}
	DPRINTK("bppanic closed\n");
	/* Clears open flag */
	bpp.openFlag = 0;

	/* Release the semaphore and return */
	up(&bpp.sem);
	return 0;
}

static unsigned int bppanicPoll(struct file *filp, poll_table * wait)
{
        unsigned int mask = 0;
	
	DEBUG_PANIC_DRV_EXTRA("PDRVDBG: enter fct Poll\n");

	poll_wait(filp, &bpp.waitQueue, wait);

	switch (atomic_read(&bpp.state)) 
	{
            case BPP_FILL:
	    case BPP_HEADER:
		mask = POLLIN; // | POLLRDNORM; don't send POLLRDNORM, the unchaged code for  panic daemon uses this for mem panic
		break;
	    case BPP_IDLE:
	    default:
	        mask=0;
		break;
	}

	DEBUG_PANIC_DRV_EXTRA("PDRVDBG: ready fct Poll (mask=0x%x)\n",mask);

	return mask;
}


static ssize_t bppanicWrite(struct file * filp, const char __user * buf, size_t count, loff_t * l)
{
    /* We have a potential panic command */
    u32 panic_cmd = 0;

    DEBUG_PANIC_DRV_EXTRA("PDRVDBG: enter fct Write\n");

    /* check error and no data cases */
    if( !bpp.openFlag ) 
    {
        printk("%s: Not Opened\n", __FUNCTION__);
    	//ret = 0;
	return 0;
    }

    if(count!=4) return 0;

    /* empty the buffer */
    bpp.data_num = 0;
    bpp.read_num = 0;
    
    /* Get the command*/
    panic_cmd = *((u32 *)buf);
    /* we pass this command to MB without checking validity of command */
    /* no more needed to check the state and return -EBUSY if not in correct state */
    mailbox_panic_send_cmd(panic_cmd);
    /* we are not changing the state, it only changes during read */

    return 4;	
}


static ssize_t bppanicRead(struct file * filp, char __user * buf, size_t count, loff_t * l)
{
    int num = 0, ret = 0;
    unsigned int fifo_used = 0;
	
    DEBUG_PANIC_DRV_EXTRA("PDRVDBG: enter fct Read\n");

    /* check error and no data cases */
    if( !bpp.openFlag ) 
    {
        printk("%s: Not Opened\n", __FUNCTION__);
	return 0;
    }

    if(count<1) return 0;                   /* user want to read 0 bytes */

    fifo_used = bpp.data_num;               /* save data.num because it can change */ 

    if(fifo_used == 0) return 0;            /* no data in buffer     */
    if(fifo_used == bpp.read_num) return 0; /* no new data in buffer */
    
    /* Calculate the size of data to read */
    num = count > (fifo_used - bpp.read_num) ? (fifo_used - bpp.read_num) : count;
    /* Copy data (beware, return bytes nb that cannot be copied) */
    ret = copy_to_user((void __user *)buf, (void *)(&(bpp.drv_buf[bpp.read_num])), num);
    /* Actual data that is copied */
    num -= ret;
    /* Increment the number of data read */
    bpp.read_num += num;
		
    DEBUG_PANIC_DRV_EXTRA("PDRVDBG: read done : %d \n",num);

    return num;		
}

void mb_ipc_panic_copy_to_drv_buf (u32 msg)
{
	int i = 0;
	switch (atomic_read(&bpp.state)) {
	case BPP_IDLE:
		/* We might have panic event notification */
		printk("BP Panic Occured\n");
                bpp.data_num = 0;
                bpp.read_num = 0;
		/* But we'll process it as any normal response */
	case BPP_HEADER:
		/* check header first */
		if( PANIC_HEADER == (msg & 0xFFFF0000) ) 
		{
			/* We have the header, set the length of data to follow */
			bpp.resp_size = (msg & 0x0000FFFF);
			/* Change the state now as our command was verified by BP and its sending data */
			if (bpp.resp_size)
				atomic_set(&bpp.state, BPP_FILL);
		}
		else
			/* Added for easy debugging has to be replaced by panic since we can't recover */
			printk(KERN_ERR "BP Panic : Invalid state for this data\n");
		break;
			
	case BPP_FILL:
		/* we need to copy the data into buffer */
		/* Due to W3G mailboxes architecture, there are 4 bytes in each new mailbox message */
		/* 1 to 4 bytes are used according the padding */
		for(i=0; i < 4; i++)  
		{
			bpp.drv_buf[bpp.data_num++] = *(((unsigned char*)&msg) + (3-i) );   // 3-i to change the endianess

			if(bpp.resp_size!=0) bpp.resp_size--;
			
			if(bpp.resp_size==0)
		        {
				/* All the bytes have been copied from response, we should wake up daemon */
				atomic_set(&bpp.state, BPP_HEADER);
                                DEBUG_PANIC_DRV_EXTRA("PDRVDBG: Packet complete (unblock poll)\n");
				wake_up_interruptible(&bpp.waitQueue);
				break;
			}
		}
		break;
	}
}
EXPORT_SYMBOL(mb_ipc_panic_copy_to_drv_buf);



module_init(bppanicInit);
module_exit(bppanicExit);

MODULE_AUTHOR("Motorola");
MODULE_DESCRIPTION("BP Panic Driver");
MODULE_LICENSE("GPL");

