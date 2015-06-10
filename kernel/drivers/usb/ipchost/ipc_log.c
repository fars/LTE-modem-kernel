/*
 * Copyright (C) 2007-2009 Motorola, Inc
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
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/07/2007      Motorola        USB-IPC initial
 * 03/22/2007      Motorola        USB-IPC header support
 * 05/09/2008      Motorola        Change Copyright and Changelog
 * 05/20/2009      Motorola        Port for W3G to use Mailbox instead of USB
 * 05/28/2009      Motorola        Inv ipc buffer endianess (merged with previous memcpy used to copy ipc buffer to dirver buffer)
 * 
 */
 
/*!
 * @file drivers/usb/ipchost/ipc_log.c
 * @brief AP Char driver for Mailbox to userspace
 *
 * This is the generic portion of the MB-IPC-APCHAR driver.
 *
 * @ingroup IPCFunction
 */


/*
 *  mb_ipc_log.c
 *  MB IPC Log driver
 */

/*=============================================================================
................................INCLUDE FILES
=============================================================================*/
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/major.h>
#include <linux/device.h>
#include <linux/ipc_api.h>



/*=============================================================================
                        LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
=============================================================================*/
typedef struct {
	void *		  next;
	int 		  data_num;
	int 	  read_num;
	unsigned char *ptr;
} IPC_LOG_DATA_BUFFER;

struct {
	int 				 read_bufsize;	/* each receive buffer size, NOT less than MaxPacketSize */
	IPC_LOG_DATA_BUFFER  *read_buf; 	/* pointer to IPC_LOG_DATA_BUFFER for read function call*/
	IPC_LOG_DATA_BUFFER  *write_buf;	/* pointer to IPC_LOG_DATA_BUFFER for submit URB */
	IPC_LOG_DATA_BUFFER  *end_buf;		/* pointer to last IPC_LOG_DATA_BUFFER */
	int 				 buf_num;		/* totally IPC_LOG_DATA_BUFFER */

	int 				 isopen;	 /* whether open function is called */
} ipc_log_param;

typedef struct {
	unsigned long buff_virt_addr;
	unsigned int buff_length;
} IPC_BUFF_PARAM;

/*=============================================================================
                                LOCAL MACROS
=============================================================================*/


/*=============================================================================
                                LOCAL FUNCTIONS
=============================================================================*/
void mb_ipc_readtasklet_function(unsigned long);

void mb_ipc_log_readscheduler(unsigned long, unsigned int);

static int mb_ipc_log_open(struct inode *, struct file *);

static int mb_ipc_log_release(struct inode *, struct file *);

static unsigned int mb_ipc_log_poll(struct file *, struct poll_table_struct *);

static ssize_t mb_ipc_log_read(struct file *, char __user *, size_t, loff_t *);

int ipc_log_fill_user_buf(unsigned char *, int);

IPC_LOG_DATA_BUFFER * ipc_log_alloc_read_buffer(void);

static int __init mb_ipc_log_init(void);

static void __exit mb_ipc_log_exit(void);

/*=============================================================================
                                LOCAL CONSTANTS
=============================================================================*/

/**
 * Identifier of the driver. Used in all kernel APIs requiring a text
 * string to identify the driver, and in creating the devfs entry
 */
#define LOGGER_ID    "logger"

/* #define IPC_DEBUG */

#ifdef IPC_DEBUG
#define DEBUG(args...) printk(args)
#else
#define DEBUG(args...)
#endif

static IPC_BUFF_PARAM ipc_buff_addr_param;
static unsigned char buff_avl_event;

static DEFINE_SPINLOCK(ipc_log_spinlock);
static DECLARE_MUTEX(ipc_log_mutex);
static DECLARE_WAIT_QUEUE_HEAD(ipc_log_wait_q);
static DECLARE_TASKLET(mb_ipc_readtasklet, mb_ipc_readtasklet_function, (unsigned long) &ipc_buff_addr_param);


/*=============================================================================
                                GLOBAL VARIABLES
=============================================================================*/

/**
 * This structure exposes the available I/O
 * operations of this driver to the kernel.
 * Each member points to a corresponding function
 */
	static struct file_operations mb_ipc_log_fops = {
		.owner    =    THIS_MODULE,
		.open     =    mb_ipc_log_open,
		.release  =    mb_ipc_log_release,
		.read     =    mb_ipc_log_read,
		.poll     =    mb_ipc_log_poll,
	};

static struct class *logger_class;
struct device *logger_device_class;
static int major_logger;



/******************************************************************************
 * Function Name: memcpy_w_endianess_inv
 * Description  : copy data between 2 buffers and change the endianess
 *****************************************************************************/
void memcpy_w_endianess_inv(unsigned char* dst_adr, unsigned char* src_adr, int len)
{
    unsigned char* pr;
    unsigned char* pw;
    unsigned int i;
    
    pr=src_adr;
    pw=dst_adr;
    
    for(i=0;i<len;i++)
    {
        pw[i]=pr[(i&0xfffffffc)|(3-(i&3))];
    }   
}

/******************************************************************************
 * Function Name: mb_ipc_readtasklet_function
 *****************************************************************************/
void mb_ipc_readtasklet_function(unsigned long data)
{
    IPC_BUFF_PARAM *ipc_buff_addr_paramp;
    IPC_LOG_DATA_BUFFER  *temp_buf;

	spin_lock(&ipc_log_spinlock);
	if(ipc_log_param.write_buf != NULL)   {
		spin_unlock(&ipc_log_spinlock);
		/* We have a bufffer allocated for writing */
		ipc_buff_addr_paramp = (IPC_BUFF_PARAM *)data;

		DEBUG("%s: %d\n",__FUNCTION__, ipc_buff_addr_paramp->buff_length);

		if(ipc_buff_addr_paramp->buff_length != 0)  {
		
			/* Read the data from the shared memory buffer into our current write buffer */
			//memcpy((void *)(ipc_log_param.write_buf->ptr),(void *)(ipc_buff_addr_paramp->buff_virt_addr), ipc_buff_addr_paramp->buff_length);
			/* Replaced to do endianess inversion in the same time */
			memcpy_w_endianess_inv(ipc_log_param.write_buf->ptr, (unsigned char*)(ipc_buff_addr_paramp->buff_virt_addr), ipc_buff_addr_paramp->buff_length);

			ipc_log_param.write_buf->read_num = 0;
			ipc_log_param.write_buf->data_num = ipc_buff_addr_paramp->buff_length;

			ipc_log_param.write_buf = ipc_log_param.write_buf->next;
			if( (ipc_log_param.write_buf == NULL) && (ipc_log_param.buf_num < MAX_LOG_BUF_NUM) ) {
				// "write_buf->next = NULL" is same with "write_buf = end_buf"
				DEBUG("%s: alloc read buffer \n", __FUNCTION__);
				temp_buf = ipc_log_alloc_read_buffer();
				if( temp_buf == NULL)  {  /* alloc new buffer is failed */
					DEBUG("%s: Error alloc read buffer\n", __FUNCTION__);
					wake_up_interruptible(&ipc_log_wait_q);
					return;
				}
			    /* add new buffer to buffer list .... */
				spin_lock(&ipc_log_spinlock);
				ipc_log_param.end_buf->next = temp_buf;
				ipc_log_param.end_buf		= temp_buf;
			    ipc_log_param.write_buf 	= temp_buf;
				spin_unlock(&ipc_log_spinlock);
			}
			wake_up_interruptible(&ipc_log_wait_q);
			}
		
		/* Send Ack to IPC */
		mailbox_dlog_ack();
	} else {
		/* Mark we were scheduled but out of buffer space so we need to return when we have space */
		buff_avl_event = 1;
		spin_unlock(&ipc_log_spinlock);
	}
}

/******************************************************************************
 * Function Name: mb_ipc_log_readscheduler
 *****************************************************************************/
void mb_ipc_log_readscheduler(unsigned long mb_buff_virt_addr, unsigned int mb_buff_length)
{
	/* Assign the virtual address provided to us by IPC callback */
    ipc_buff_addr_param.buff_virt_addr = mb_buff_virt_addr;
    ipc_buff_addr_param.buff_length = mb_buff_length;
	tasklet_schedule(&mb_ipc_readtasklet);
}
EXPORT_SYMBOL(mb_ipc_log_readscheduler);

/******************************************************************************
 * Function Name: ipc_log_fill_user_buf
 *****************************************************************************/
int ipc_log_fill_user_buf(unsigned char *buf, int count)
{
	int i, size, num, ret;
        IPC_LOG_DATA_BUFFER *read_buf;

	DEBUG("%s: Enter count=%d\n", __FUNCTION__, count);

	size = 0;
	spin_lock(&ipc_log_spinlock);
	/* loop all Buffer ... */
	for(i = 0; i < ipc_log_param.buf_num; i++) {
		/* all data in buffer is retrieved or input "buf" is full */
		if( (ipc_log_param.read_buf == ipc_log_param.write_buf) || (count <= 0) ) {
	                DEBUG("%s: break\n", __FUNCTION__);
			break;
		}
		/*  */
		num = count > (ipc_log_param.read_buf->data_num - ipc_log_param.read_buf->read_num) ? (ipc_log_param.read_buf->data_num - ipc_log_param.read_buf->read_num) : count;
		ret = copy_to_user((unsigned char *)((unsigned long)buf + size), (unsigned char *)&(ipc_log_param.read_buf->ptr[ipc_log_param.read_buf->read_num]), num);
		count -= num;
		ipc_log_param.read_buf->read_num += num;
		size += num;
		/* if the current buffer is empty, move it to the tail of buffer list */
		if( ipc_log_param.read_buf->data_num <= ipc_log_param.read_buf->read_num )  {
			DEBUG("%s: Move read buffer from head to the tail\n", __FUNCTION__);
			read_buf = ipc_log_param.read_buf;
			read_buf->read_num          = 0;
			read_buf->data_num          = 0;
			if(ipc_log_param.read_buf != ipc_log_param.end_buf) {
				ipc_log_param.read_buf      = read_buf->next;
				ipc_log_param.end_buf->next = read_buf;
				ipc_log_param.end_buf       = read_buf;
				read_buf->next              = 0;
			}
			if (ipc_log_param.write_buf == NULL)
				ipc_log_param.write_buf = read_buf;
		}
	}
	
	if(1 == buff_avl_event)  {
		/* Clear the event flag */
		buff_avl_event = 0;
		spin_unlock(&ipc_log_spinlock);
		/* We missed last write, schedule now with prev available buffer parameters */
		tasklet_schedule(&mb_ipc_readtasklet);
	} else
		spin_unlock(&ipc_log_spinlock);

	DEBUG("%s: return %d\n", __FUNCTION__, size);

	return size;
} 

/******************************************************************************
 * Function Name: ipc_log_alloc_read_buffer
 *****************************************************************************/
IPC_LOG_DATA_BUFFER * ipc_log_alloc_read_buffer(void)
{
	IPC_LOG_DATA_BUFFER *temp_buf;

	DEBUG("%s: Enter\n", __FUNCTION__);	

	temp_buf = kmalloc(sizeof(IPC_LOG_DATA_BUFFER), GFP_KERNEL);
	if(temp_buf == NULL) {
		return NULL;
	}
	temp_buf->ptr = kmalloc(ipc_log_param.read_bufsize, GFP_KERNEL);
	if(temp_buf->ptr == NULL)  {
		kfree(temp_buf);
		return NULL;
	}
	temp_buf->data_num = 0;
	temp_buf->read_num = 0;
	temp_buf->next     = 0;

	ipc_log_param.buf_num ++;

	DEBUG("%s alloc buffer %d, %d\n", __FUNCTION__, ipc_log_param.buf_num, ipc_log_param.read_bufsize);	
	return temp_buf;
}

/******************************************************************************
 * Function Name: mb_ipc_log_open
 *****************************************************************************/
static int mb_ipc_log_open(struct inode * inode, struct file * file)
{
    DEBUG("Enter %s\n", __FUNCTION__);
	/* Hold the semaphore */
	if (down_interruptible(&ipc_log_mutex)) {
		return -ERESTARTSYS;
	}

    if( ipc_log_param.isopen ) {
	printk("%s: MB IPC LOG open error\n", __FUNCTION__);
	up(&ipc_log_mutex);
	return -EBUSY;
    }
    
    ipc_log_param.isopen   = 1;
	
	/* Release the semaphore and return */
	up(&ipc_log_mutex);

    return 0;
}

/******************************************************************************
 * Function Name: mb_ipc_log_release
 *****************************************************************************/
static int mb_ipc_log_release(struct inode * inode, struct file * filp)
{
    if( !ipc_log_param.isopen ) {
    	// not opened, directly return.
    	return 0;
    }
	
    ipc_log_param.isopen   = 0;
	
    return 0;
}

/******************************************************************************
 * Function Name: mb_ipc_log_poll
 *****************************************************************************/
static unsigned int mb_ipc_log_poll(struct file * file, struct poll_table_struct *wait)
{
    unsigned int mask = 0;

    DEBUG("Enter %s\n", __FUNCTION__);
    if( 0 == ipc_log_param.isopen) {
        printk("%s: MB IPC LOG calling poll error\n", __FUNCTION__);
        return -EBUSY;
    }
    
    poll_wait(file, &ipc_log_wait_q, wait); 
    
    if( ipc_log_param.read_buf != ipc_log_param.write_buf )
        mask = (POLLIN | POLLRDNORM);
    
    DEBUG("%s: return mask=%d\n", __FUNCTION__, mask);

    return mask;
}

/******************************************************************************
 * Function Name: mb_ipc_log_read
 *****************************************************************************/
static ssize_t mb_ipc_log_read(struct file * filp, char __user * buf, size_t count, loff_t * l)
{       
    int size;

    DEBUG("%s: Enter count=%d\n", __FUNCTION__, count);

    if( !ipc_log_param.isopen ) {
        printk("%s: Not Opend\n", __FUNCTION__);
    	return 0;
    }

    if(count <= 0) {   /* read buffer is 0 */
    	return 0;
    }

    /* get out the received data */
    size = ipc_log_fill_user_buf(buf, count);

    DEBUG("%s: return %d\n", __FUNCTION__, size);

    return size;
}


static int mb_ipc_log_init(void)
{
	IPC_LOG_DATA_BUFFER *temp_buf;
	/* Allocate a major number for the character device */
	major_logger = register_chrdev(0, "logger", &mb_ipc_log_fops);
	if (major_logger < 0) {
		printk(KERN_INFO "Unable to get a major for logger\n");
		return major_logger;
	}

	logger_class = class_create(THIS_MODULE, "logger");
	if (IS_ERR(logger_class)) {
		printk(KERN_INFO "Not able to do the class_create\n");
		unregister_chrdev(major_logger, "logger");
		return (int) logger_class;
	}

	logger_device_class =
	    device_create(logger_class, NULL, MKDEV(major_logger, 0),
			  NULL, "logger");
	if (IS_ERR(logger_device_class)) {
		class_destroy(logger_class);
		unregister_chrdev(major_logger, "logger");
		printk(KERN_INFO
		       "Not able to do the class_device_create\n");
		return (int) logger_device_class;
	}

	/* Initialize the buffer */
	memset((void *)&ipc_log_param, 0, sizeof(ipc_log_param));
	
	ipc_log_param.read_bufsize = MAX_LOG_BUF_SIZE;
	
	temp_buf = ipc_log_alloc_read_buffer();
	if(temp_buf == NULL)  {
		return -ENODEV;
	}
	ipc_log_param.read_buf  = temp_buf;
	ipc_log_param.end_buf   = temp_buf;
	ipc_log_param.write_buf = temp_buf;
	ipc_log_param.isopen    = 0;

	return 0;
}


static void mb_ipc_log_exit(void)
{
    int i = 0;
	IPC_LOG_DATA_BUFFER *temp_buf;
	if (major_logger >= 0) {
		device_destroy(logger_class, MKDEV(major_logger, 0));
		class_destroy(logger_class);
		unregister_chrdev(major_logger, "logger");
	}
	/* Free the resources (use this space later to free IRQs etc.) */

	/* free memory */
	for ( i = 0 ; i < ipc_log_param.buf_num; i++ ) {
		temp_buf = ipc_log_param.read_buf->next;
		kfree(ipc_log_param.read_buf->ptr);
		kfree(ipc_log_param.read_buf);
		ipc_log_param.read_buf = temp_buf;

	printk(KERN_INFO "Unloaded logger device driver\n");
}
}


module_init(mb_ipc_log_init);
module_exit(mb_ipc_log_exit);

MODULE_AUTHOR("Motorola");
MODULE_DESCRIPTION("Logger Driver");
MODULE_LICENSE("GPL");
