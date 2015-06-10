/*
 * Copyright (C) 2010 Motorola, Inc.
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
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/types.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/version.h>

#include <mach/gpio.h>
#include <mach/io.h>
#include <mach/omap24xx.h>

/*
 * This file provides an interface to read OMAP W3G GPIO from user space
 */
/* MACROS */
#if 0
# define tracemsg(fmt,args...)  printk(fmt,##args)
#else
# define tracemsg(fmt,args...)
#endif

#define write_reg_bits(reg, mask, bits) (*(reg) = ( *(reg) & ~(mask)) | (bits));

/* CONSTANTS */
#define MOT_GPIO_IOCTL_GPIO_READ 1
#define MOT_GPIO_IOCTL_GPIO_SET  2

#define MOT_GPIO_8_PADCONF IO_ADDRESS(OMAPW3G_SCM_BASE + 0x014C)
#define MOT_GPIO_9_PADCONF IO_ADDRESS(OMAPW3G_SCM_BASE + 0x0150)

/* ENUMS and TYPDEFs*/

/* LOCAL FUNCTION PROTOTYPES */
static int mot_gpio_open(struct inode *inode, struct file *file);
static int mot_gpio_free(struct inode *inode, struct file *file);
static int mot_gpio_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);

/* LOCAL STRUCTURES */
/*This structure defines the file operations for the mot_tcmd_gpio device */
static struct file_operations mot_gpio_fops =
{
    .owner =    THIS_MODULE,
    .open =     mot_gpio_open,
    .release =  mot_gpio_free,
    .ioctl =    mot_gpio_ioctl,
};

/* LOCAL VARIABLES */
static int mot_gpio_module_major;
static struct class *mot_gpio_class;

/* LOCAL FUNCTIONS */

/* DESCRIPTION:
       The open() handler for the mot_gpio device.
 
   INPUTS:
       inode       inode pointer
       file        file pointer

   OUTPUTS:
       Returns 0 if successful.

   IMPORTANT NOTES:
       None.   
*/
static int mot_gpio_open(struct inode *inode, struct file *file)
{
  return 0;
}

/* DESCRIPTION:
       The close() handler for the mot_gpio device
 
   INPUTS:
       inode       inode pointer
       file        file pointer

   OUTPUTS:
       Returns 0 if successful.

   IMPORTANT NOTES:
       None.   
*/
static int mot_gpio_free(struct inode *inode, struct file *file)
{
  return 0;
}

/* DESCRIPTION:
       The mot_gpio intialization function.
 
   INPUTS:
       None.

   OUTPUTS:
       Returns 0 if successful and returns an error code upon failure.

   IMPORTANT NOTES:
       None.   
*/

int __init mot_gpio_init(void)
{    
    int ret = 0;
    struct device *temp_class;
    
    mot_gpio_module_major = register_chrdev(0, "mot_gpio", &mot_gpio_fops);

    if (mot_gpio_module_major < 0) 
    {
        tracemsg("mot_gpio_init: Unable to get a major for mot gpio driver\n");
        return mot_gpio_module_major;
    }

    mot_gpio_class = class_create(THIS_MODULE, "mot_gpio");

    if (IS_ERR(mot_gpio_class)) 
    {
        unregister_chrdev(mot_gpio_module_major, "mot_gpio");
        tracemsg("mot_gpio_init: Error creating mot_gpio class.\n");
        ret = PTR_ERR(mot_gpio_class);
        return ret;
    }

    temp_class = device_create(mot_gpio_class, NULL, MKDEV(mot_gpio_module_major, 0), NULL, "mot_gpio");
    
    if (IS_ERR(temp_class)) 
    {
        class_destroy(mot_gpio_class);
        unregister_chrdev(mot_gpio_module_major, "mot_gpio");
        tracemsg("mot_gpio_init: Error creating mot_gpio class device.\n");
        ret = PTR_ERR(temp_class);
        return ret;
    }
    
        
    tracemsg("mot_gpio_init: mot_gpio Module successfully initialized \n");
    return ret;    
}


/* DESCRIPTION:
       The mot_gpio device cleanup function
 
   INPUTS:
       None. 

   OUTPUTS:
       None.

   IMPORTANT NOTES:
       None.   
*/
static void __exit mot_gpio_exit(void)
{
    device_destroy(mot_gpio_class, MKDEV(mot_gpio_module_major, 0));
    class_destroy(mot_gpio_class);
    unregister_chrdev(mot_gpio_module_major, "mot_gpio");

    tracemsg("mot_gpio_exit: driver successfully unloaded.\n");    
}

/* DESCRIPTION:
       The ioctl () handler for the MOT GPIO driver
 
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
static int mot_gpio_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int status = 0;
    unsigned char args_kernel[2] = {0, 0};
       
    status = copy_from_user(args_kernel, (unsigned char *) arg, sizeof(args_kernel));

    /* configure the I/O pad for GPIO 8 and 9 */
    write_reg_bits ((volatile unsigned int *)MOT_GPIO_8_PADCONF, 0xFFFF0000, 0x01000000);
    write_reg_bits ((volatile unsigned int *)MOT_GPIO_9_PADCONF, 0x0000FFFF, 0x00000000);

    switch (cmd)
    {
        case MOT_GPIO_IOCTL_GPIO_READ:
        /* request the gpio */
    	status = gpio_request(args_kernel[0], NULL);
    	if (status != 0){
        	tracemsg("Warning: the mot_gpio driver failed to request a GPIO. GPIO -> %d Error -> %X\n", args_kernel[0], status); 
	}   
            /* Set GPIO Direction */
            gpio_direction_input(args_kernel[0]);
	    args_kernel[1] = (gpio_get_value(args_kernel[0]) ? 1 : 0);
	    status = copy_to_user ((unsigned char *)arg, &args_kernel, sizeof(unsigned char) * 2);
	    
	    break;

	case MOT_GPIO_IOCTL_GPIO_SET:
	     gpio_direction_output(args_kernel[0], 0);
	     gpio_set_value(args_kernel[0], args_kernel[1]);

	    break;

        default:
	  tracemsg("Warning: Invalid request sent to the MOT GPIO driver.\n");
	  status = -ENOTTY;
	  break;
    }
 
    tracemsg ("mot gpio -> cmd %X, arg 1 0x%X, arg 2 0x%X, status: %d\n", 
	      cmd, args_kernel[0], args_kernel[1], status);

    return status;
}

/* Module Entry / Exit Points */
module_init (mot_gpio_init)
module_exit (mot_gpio_exit)

MODULE_DESCRIPTION("MOT GPIO DRIVER")
MODULE_AUTHOR("Motorola")
MODULE_LICENSE("GPL")
