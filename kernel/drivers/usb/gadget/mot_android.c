/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include <linux/usb/android.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/io.h>

#include "f_mass_storage.h"
#include "f_adb.h"
#include "f_usbnet.h"
#include "f_acm.h"
#include "f_mtp.h"
#include "f_mot_android.h"
#include "u_serial.h"
#include "u_ether.h"
#include "gadget_chips.h"

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

MODULE_AUTHOR("Motorola");
MODULE_DESCRIPTION("Motorola Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static const char longname[] = "Gadget Android";

static DECLARE_WAIT_QUEUE_HEAD(device_mode_change_wait_q);

/* Default vendor and product IDs, overridden by platform data */
#define VENDOR_ID		0x22b8
#define PRODUCT_ID		0x41da
#define ADB_PRODUCT_ID		0x41da

#ifdef CONFIG_USB_MOT_ANDROID
struct device_pid_vid {
	char *name;
	u32 type;
	int vid;
	int pid;
	char *config_name;
	int class;
	int subclass;
	int protocol;
};

#define MAX_DEVICE_TYPE_NUM   20
#define MAX_DEVICE_NAME_SIZE  30

static struct device_pid_vid mot_android_vid_pid[MAX_DEVICE_TYPE_NUM] = {
	{"msc", MSC_TYPE_FLAG, 0x22b8, 0x41d9, "Motorola Config 14",
	 USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE,
	 USB_CLASS_PER_INTERFACE},
	{"cdrom", CDROM_TYPE_FLAG, 0x22b8, 0x41de, "Motorola CDROM Device",
	 USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE,
	 USB_CLASS_PER_INTERFACE},
	{"msc_adb", MSC_TYPE_FLAG | ADB_TYPE_FLAG, 0x22b8, 0x41db,
	 "Motorola Config 42", USB_CLASS_PER_INTERFACE,
	 USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},
	{"eth", ETH_TYPE_FLAG, 0x22b8, 0x41d4, "Motorola Config 13",
	 USB_CLASS_COMM, USB_CLASS_COMM, USB_CLASS_PER_INTERFACE},
	{"mtp", MTP_TYPE_FLAG, 0x22b8, 0x41D6, "Motorola Config 15",
	 USB_CLASS_PER_INTERFACE,
	 USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},
	{"acm", ACM_TYPE_FLAG, 0x22b8, 0x6422, "Motorola Config 1",
	 USB_CLASS_COMM, USB_CLASS_COMM, USB_CLASS_PER_INTERFACE},
	{"eth_adb", ETH_TYPE_FLAG | ADB_TYPE_FLAG, 0x22b8, 0x41d4,
	 "Motorola Android Composite Device"},
	{"acm_eth_mtp", ACM_TYPE_FLAG | ETH_TYPE_FLAG | MTP_TYPE_FLAG, 0x22b8,
	 0x41d8, "Motorola Config 30", USB_CLASS_VENDOR_SPEC,
	 USB_CLASS_VENDOR_SPEC, USB_CLASS_VENDOR_SPEC},
	{"mtp_adb", MTP_TYPE_FLAG | ADB_TYPE_FLAG, 0x22b8, 0x41dc,
	 "Motorola Config 32", USB_CLASS_VENDOR_SPEC,
	 USB_CLASS_VENDOR_SPEC, USB_CLASS_VENDOR_SPEC},
	{"acm_eth_mtp_adb",
	 ACM_TYPE_FLAG | ETH_TYPE_FLAG | MTP_TYPE_FLAG | ADB_TYPE_FLAG, 0x22b8,
	 0x41da, "Motorola Config 31", USB_CLASS_VENDOR_SPEC,
	 USB_CLASS_VENDOR_SPEC, USB_CLASS_VENDOR_SPEC},
	{"acm_eth_adb", ACM_TYPE_FLAG | ETH_TYPE_FLAG | ADB_TYPE_FLAG, 0x22b8,
	 0x41e2, "Motorola Android Composite Device"},
	{"msc_eth", MSC_TYPE_FLAG | ETH_TYPE_FLAG, 0x22b8, 0x41d4,
	 "Motorola Android Composite Device"},
	{"msc_adb_eth", MSC_TYPE_FLAG | ADB_TYPE_FLAG | ETH_TYPE_FLAG, 0x22b8,
	 0x41d4, "Motorola Android Composite Device"},
	{}
};

static atomic_t device_mode_change_excl;
#endif
static int g_device_type;

struct android_dev {
	struct usb_gadget *gadget;
	struct usb_composite_dev *cdev;

	int product_id;
	int adb_product_id;
	int version;

	int nluns;
};

static atomic_t adb_enable_excl;
static struct android_dev *_android_dev;

/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2
#define STRING_CONFIG_IDX		3

/* String Table */
static struct usb_string strings_dev[] = {
	/* These dummy values should be overridden by platform data */
	[STRING_MANUFACTURER_IDX].s = "Android",
	[STRING_PRODUCT_IDX].s = "Android",
	[STRING_SERIAL_IDX].s = "0123456789ABCDEF",
#if !defined (CONFIG_USB_GADGET_SINGLE_ECM) && \
    !defined (CONFIG_USB_GADGET_DUAL_ECM) && \
    !defined (CONFIG_USB_GADGET_FIVE_ECM) && \
    !defined (CONFIG_USB_GADGET_SINGLE_ACM) && \
    !defined (CONFIG_USB_GADGET_DUAL_ACM) && \
    defined (CONFIG_USB_GADGET_NETWORK_INTERFACE)
	[STRING_CONFIG_IDX].s = "Motorola Datacard Composite Configuration",
#else
	[STRING_CONFIG_IDX].s = "Motorola Datacard Composite Configuration",
#endif
	{}			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language = 0x0409,	/* en-us */
	.strings = strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength = sizeof(device_desc),
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = __constant_cpu_to_le16(0x0200),
#ifdef CONFIG_USB_MOT_ANDROID
	.bDeviceClass = USB_CLASS_VENDOR_SPEC,
	.bDeviceSubClass = USB_CLASS_VENDOR_SPEC,
	.bDeviceProtocol = USB_CLASS_VENDOR_SPEC,
#else
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
#endif
	.idVendor = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations = 1,
};

#define TRUE 1
#ifdef CONFIG_USB_GADGET_RAM_LOG
#define MAX_RAM_DATA_SIZE 256

enum log_address {
    DIFF_LOG_ADDR,
    SAME_LOG_ADDR,
    INITIAL_CONDITION
};
enum log_address log_addr_status = INITIAL_CONDITION;

static DECLARE_WAIT_QUEUE_HEAD(usb_thread_wait_q);
static DECLARE_WAIT_QUEUE_HEAD(usb_log_wait_q);

static atomic_t usb_log_excl;

struct task_struct *log_task;

struct usb_log_info {
        u32 usb_id;
        u32 usb_count_32;
        u32 usb_byte_count;
};

struct usb_log_info ram_log_details[MAX_RAM_DATA_SIZE];

/* increment on each logging */
unsigned int log_index;
/* to keep track of whether 50% of buffer filled or not */
unsigned int half_buff_fill_count;
/* total number of records to be sent to user space */
unsigned int count_to_user_space;

struct usb_log_info *previous_log_address = &ram_log_details[0];
struct usb_log_info *current_log_address = &ram_log_details[0];

unsigned int wakeup_q_flag;

u8 thread_started;

struct semaphore log_sema;
#endif

void android_usb_set_connected(int connected)
{
/*
	if (_android_dev && _android_dev->cdev && _android_dev->cdev->gadget) {
		if (connected)
			usb_gadget_connect(_android_dev->cdev->gadget);
		else
			usb_gadget_disconnect(_android_dev->cdev->gadget);
	}
*/
}

#ifdef CONFIG_USB_GADGET_RAM_LOG

void usb_ram_log(unsigned int id, unsigned int bcount)
{
        down(&log_sema);
        ram_log_details[log_index].usb_id = id;
        ram_log_details[log_index].usb_byte_count = bcount;
        ram_log_details[log_index].usb_count_32 = *((u32 *)(0xee1e0010));
        log_index = (log_index + 1) % (MAX_RAM_DATA_SIZE);
        half_buff_fill_count++;
        up(&log_sema);

        if (half_buff_fill_count == (MAX_RAM_DATA_SIZE/2)) { /* 50% */
                if (thread_started) {
                        wakeup_q_flag = 1;
                        wake_up_interruptible(&usb_thread_wait_q);
                }
        }
}

int usb_log_open(struct inode *inode, struct file *file)
{
        if (atomic_inc_return(&usb_log_excl) != 1) {
                atomic_dec(&usb_log_excl);
                return -EBUSY;
        }
    wake_up_process(log_task);
        return 0;
}

int usb_log_release(struct inode *inode, struct file *file)
{
        atomic_dec(&usb_log_excl);
        return 0;
}

static ssize_t usb_log_read(struct file *filp,  /* see include/linux/fs.h   */
                             char *buffer,      /* buffer to fill with data */
                             size_t length,     /* length of the buffer     */
                             loff_t *offset)
{
	int len = 0;
        int ret;
        if (previous_log_address > current_log_address) {
                len = (&ram_log_details[MAX_RAM_DATA_SIZE-1] -
                                previous_log_address);
                ret = copy_to_user(buffer, previous_log_address, len);
                previous_log_address = &ram_log_details[0];
                ret = copy_to_user(
                                buffer,
                                previous_log_address,
                                (count_to_user_space - len) *
                                (sizeof(struct usb_log_info)));
                len = count_to_user_space * (sizeof(struct usb_log_info));
                previous_log_address = current_log_address;
        } else {
                len = count_to_user_space * (sizeof(struct usb_log_info));
                ret = copy_to_user(buffer, previous_log_address, len);
                previous_log_address = current_log_address;
        }
        return len;
}

static unsigned int usb_log_poll(struct file *file,
                                        struct poll_table_struct *wait)
{
    enum log_address curr_flag;
    curr_flag = log_addr_status;

        poll_wait(file, &usb_log_wait_q, wait);

        if (!curr_flag) {
                log_addr_status = SAME_LOG_ADDR;
                return POLLIN | POLLRDNORM;
    }
        else
                return 0;
}

static int usb_log_thread(void *data)
{
        unsigned long time_delay;

        thread_started = 1;
        while (TRUE) {
                time_delay = (10 * HZ);
                wait_event_interruptible_timeout(
                                                usb_thread_wait_q,
                                                wakeup_q_flag != 0,
                                                time_delay);
                wakeup_q_flag = 0;

                down(&log_sema);
                count_to_user_space = half_buff_fill_count;
                current_log_address = &ram_log_details[log_index];
                half_buff_fill_count = 0;
                up(&log_sema);

                if (previous_log_address != current_log_address) {
                        log_addr_status = DIFF_LOG_ADDR;
                        wake_up_interruptible(&usb_log_wait_q);
                }
        }
        return 0;
}

static const struct file_operations usb_log_fops = {
    .owner = THIS_MODULE,
        .read = usb_log_read,
        .poll = usb_log_poll,
        .open = usb_log_open,
        .release = usb_log_release,
};


static struct miscdevice usb_logging_device = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "usblogging",
        .fops = &usb_log_fops,
};
#endif

static int __init android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret;

	/* the same sequence as force_reenumeration() */
#ifdef CONFIG_USB_GADGET_MASS_STORAGE
	ret = mass_storage_function_add(dev->cdev, c, dev->nluns);
	if (ret)
		return ret;
#endif
#if defined(CONFIG_USB_GADGET_SINGLE_ECM) || \
	defined(CONFIG_USB_GADGET_DUAL_ECM)|| \
	defined(CONFIG_USB_GADGET_FIVE_ECM)
	ret = ecm_function_add(dev->cdev, c);
	if (ret)
		return ret;
#endif
#if defined(CONFIG_USB_GADGET_SINGLE_ACM) || \
	defined(CONFIG_USB_GADGET_DUAL_ACM)
	ret = acm_function_add(dev->cdev, c);
	if (ret)
		return ret;
#endif
#ifdef CONFIG_USB_GADGET_NETWORK_INTERFACE
	ret = usbnet_function_add(dev->cdev, c);
	if (ret)
		return ret;
#endif
#ifdef CONFIG_USB_GADGET_MTP
	ret = mtp_function_add(dev->cdev, c);
	if (ret)
		return ret;
#endif
#ifdef CONFIG_USB_GADGET_ADB
	ret = adb_function_add(dev->cdev, c);
#endif

	return ret;
}

static int android_setup_config(struct usb_configuration *c,
				const struct usb_ctrlrequest *ctrl);
static int usb_device_cfg_flag;
static int usb_get_desc_flag;
static int usb_data_transfer_flag;

static struct usb_configuration android_config_driver = {
	.label = "android",
	.bind = android_bind_config,
	.setup = android_setup_config,
	.bConfigurationValue = 1,
	.bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower = 0xFA,	/* 500ma */
};
#ifdef CONFIG_USB_MOT_ANDROID
int get_func_thru_config(int mode)
{
	int i;
	char name[50];

	memset(name, 0, 50);
	sprintf(name, "Motorola Config %d", mode);
	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (!mot_android_vid_pid[i].config_name)
			break;
		if (!strcmp(mot_android_vid_pid[i].config_name, name))
			return i;
	}
	return -1;
}

static int pc_mode_switch_flag;

void mode_switch_cb(int mode)
{
	pc_mode_switch_flag = mode;
	wake_up_interruptible(&device_mode_change_wait_q);
}
static int android_generic_setup(struct usb_configuration *c,
				const struct usb_ctrlrequest *ctrl)
{
	int	value = -EOPNOTSUPP;
	u16     wIndex = le16_to_cpu(ctrl->wIndex);
	u16     wValue = le16_to_cpu(ctrl->wValue);
	u16     wLength = le16_to_cpu(ctrl->wLength);
	struct usb_composite_dev *cdev = c->cdev;
	struct usb_request	*req = cdev->req;

	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_VENDOR:
		switch (ctrl->bRequest) {
		case 1:
			if ((wValue == 0) && (wLength == 0)) {
				mode_switch_cb(wIndex);
				value = 0;
				req->zero = 0;
				req->length = value;
				if (usb_ep_queue(cdev->gadget->ep0, req,
					GFP_ATOMIC))
					printk(KERN_ERR "ep0 in queue failed\n");
			}
		break;
		default:
			break;
		}
	default:
		break;
	}
	return value;
}
#endif

static int android_setup_config(struct usb_configuration *c,
				const struct usb_ctrlrequest *ctrl)
{
	int i, ret = -EOPNOTSUPP;
#ifdef CONFIG_USB_MOT_ANDROID
	ret = android_generic_setup(c, ctrl);
	if (ret >= 0)
		return ret;
#endif
	for (i = 0; i < android_config_driver.next_interface_id; i++)
		if (android_config_driver.interface[i]->setup) {
			ret =
			    android_config_driver.interface[i]->
			    setup(android_config_driver.interface[i], ctrl);
			if (ret >= 0)
				return ret;
		}
	return ret;
}

void usb_data_transfer_callback(void)
{
	if (usb_data_transfer_flag == 0) {
		usb_data_transfer_flag = 1;
		usb_get_desc_flag = 1;
		wake_up_interruptible(&device_mode_change_wait_q);
	}
}

void usb_interface_enum_cb(int flag)
{
	usb_device_cfg_flag |= flag;
	if (usb_device_cfg_flag == g_device_type)
		wake_up_interruptible(&device_mode_change_wait_q);
}

static int __init android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget *gadget = cdev->gadget;
	int gcnum;
	int id;
	int ret, loop_count;

	dev->gadget = gadget;
	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	device_desc.iSerialNumber = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_CONFIG_IDX].id = id;
	android_config_driver.iConfiguration = id;

	if (gadget->ops->wakeup)
		android_config_driver.bmAttributes |= USB_CONFIG_ATT_WAKEUP;

#if defined(CONFIG_USB_GADGET_SINGLE_ECM) || \
	defined(CONFIG_USB_GADGET_DUAL_ECM) || \
	defined(CONFIG_USB_GADGET_FIVE_ECM)
	for (loop_count = 0; loop_count < MAX_NUM_ECM_DEVICES; loop_count++)
		gether_setup(gadget , hostaddr[loop_count]);
#endif
	/* double check to move this call to f_acm.c??? */
#if defined(CONFIG_USB_GADGET_SINGLE_ACM) || \
	defined(CONFIG_USB_GADGET_DUAL_ACM)
	gserial_setup(gadget, NUM_ACM_PORTS);
#endif

	/* register our configuration */
	ret = usb_add_config(cdev, &android_config_driver);
	if (ret) {
		printk(KERN_ERR "usb_add_config failed\n");
		return ret;
	}

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so just warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("%s: controller '%s' not recognized\n",
			   longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	usb_gadget_set_selfpowered(gadget);
	dev->cdev = cdev;

	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name = "android_usb",
	.dev = &device_desc,
	.strings = dev_strings,
	.bind = android_bind,
};
#ifdef CONFIG_USB_MOT_ANDROID
static void get_device_pid_vid(int type, int *pid, int *vid)
{
	int i;

	*vid = 0;
	*pid = 0;
	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (mot_android_vid_pid[i].type == type) {
			*pid = mot_android_vid_pid[i].pid;
			*vid = mot_android_vid_pid[i].vid;
			break;
		}
	}
}

int get_func_thru_type(int type)
{
	int i;

	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (mot_android_vid_pid[i].type == type)
			return i;
	}
	return -1;
}

static void force_reenumeration(struct android_dev *dev, int dev_type)
{
	int vid, pid, i, temp_enabled;
	struct usb_function *f;

	/* using other namespace ??? */
	usb_device_cfg_flag = 0;
	usb_get_desc_flag   = 0;
	usb_data_transfer_flag = 0;
	pc_mode_switch_flag = 0;

	get_device_pid_vid(dev_type, &pid, &vid);
	device_desc.idProduct = __constant_cpu_to_le16(pid);
	device_desc.idVendor = __constant_cpu_to_le16(vid);

	for (i = 0; i < MAX_CONFIG_INTERFACES; i++)
		android_config_driver.interface[i] = 0;

	/* clear all intefaces */
	android_config_driver.next_interface_id = 0;

#ifdef CONFIG_USB_GADGET_MASS_STORAGE
	temp_enabled = dev_type & (MSC_TYPE_FLAG | CDROM_TYPE_FLAG);
	f = msc_function_enable(temp_enabled,
				android_config_driver.next_interface_id);
	if (temp_enabled) {
		android_config_driver.interface[android_config_driver.
						next_interface_id] = f;
		android_config_driver.next_interface_id++;
	}
#endif
#if defined(CONFIG_USB_GADGET_SINGLE_ACM) || \
	defined(CONFIG_USB_GADGET_DUAL_ACM)
	temp_enabled = dev_type & ACM_TYPE_FLAG;
	f = acm_function_enable(temp_enabled,
				android_config_driver.next_interface_id);
	if (temp_enabled) {
		android_config_driver.interface[android_config_driver.
						next_interface_id] = f;
		android_config_driver.next_interface_id++;
		android_config_driver.interface[android_config_driver.
						next_interface_id] = f;
		android_config_driver.next_interface_id++;
	}
#endif
#ifdef CONFIG_USB_GADGET_NETWORK_INTERFACE
	temp_enabled = dev_type & ETH_TYPE_FLAG;
	f = usbnet_function_enable(temp_enabled,
				   android_config_driver.next_interface_id);
	if (temp_enabled) {
		android_config_driver.interface[android_config_driver.
						next_interface_id] = f;
		android_config_driver.next_interface_id++;
	}
#endif
#ifdef CONFIG_USB_GADGET_MTP
	temp_enabled = dev_type & MTP_TYPE_FLAG;
	f = mtp_function_enable(temp_enabled,
				android_config_driver.next_interface_id);
	if (temp_enabled) {
		android_config_driver.interface[android_config_driver.
						next_interface_id] = f;
		android_config_driver.next_interface_id++;
	}
#endif
#ifdef CONFIG_USB_GADGET_ADB
	temp_enabled = dev_type & ADB_TYPE_FLAG;
	f = adb_function_enable_id(temp_enabled,
				   android_config_driver.next_interface_id);
	if (temp_enabled) {
		android_config_driver.interface[android_config_driver.
						next_interface_id] = f;
		android_config_driver.next_interface_id++;
	}
#endif
	if (dev->cdev) {
		dev->cdev->desc.idProduct = device_desc.idProduct;
		dev->cdev->desc.idVendor = device_desc.idVendor;
		i = get_func_thru_type(dev_type);
		if (i != -1) {
			dev->cdev->desc.bDeviceClass =
				mot_android_vid_pid[i].class;
			dev->cdev->desc.bDeviceSubClass =
				mot_android_vid_pid[i].subclass;
			dev->cdev->desc.bDeviceProtocol =
				mot_android_vid_pid[i].protocol;
		}

	}

	if (dev->cdev && dev->cdev->gadget)  {
		/* dev->cdev->gadget->speed != USB_SPEED_UNKNOWN ? */
		usb_gadget_disconnect(dev->cdev->gadget);
		msleep(10);
		usb_gadget_connect(dev->cdev->gadget);
	}
}
#endif
static int adb_mode_changed_flag;
#ifdef CONFIG_USB_GADGET_ADB

static int adb_enable_open(struct inode *ip, struct file *fp)
{
	if (atomic_inc_return(&adb_enable_excl) != 1) {
		atomic_dec(&adb_enable_excl);
		return -EBUSY;
	}
	adb_mode_changed_flag = 1;
	wake_up_interruptible(&device_mode_change_wait_q);

	return 0;
}

static int adb_enable_release(struct inode *ip, struct file *fp)
{
	atomic_dec(&adb_enable_excl);
	adb_mode_changed_flag = 1;
	wake_up_interruptible(&device_mode_change_wait_q);

	return 0;
}

static const struct file_operations adb_enable_fops = {
	.owner = THIS_MODULE,
	.open = adb_enable_open,
	.release = adb_enable_release,
};

static struct miscdevice adb_enable_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "android_adb_enable",
	.fops = &adb_enable_fops,
};
#endif

#ifdef CONFIG_USB_MOT_ANDROID
/*
 * Device is used for USB mode switch
 */
static int device_mode_change_open(struct inode *ip, struct file *fp)
{
	if (atomic_inc_return(&device_mode_change_excl) != 1) {
		atomic_dec(&device_mode_change_excl);
		return -EBUSY;
	}

	return 0;
}

static int device_mode_change_release(struct inode *ip, struct file *fp)
{
	atomic_dec(&device_mode_change_excl);
	return 0;
}

static ssize_t
device_mode_change_write(struct file *file, const char __user *buffer,
			 size_t count, loff_t *ppos)
{
	unsigned char cmd[MAX_DEVICE_NAME_SIZE + 1];
	int cnt = MAX_DEVICE_NAME_SIZE;
	int i, temp_device_type;

	if (count <= 0)
		return 0;

	if (cnt > count)
		cnt = count;

	if (copy_from_user(cmd, buffer, cnt))
		return -EFAULT;
	cmd[cnt] = 0;

	printk(KERN_DEBUG "%s cmd=%s\n", __func__, cmd);

	/* USB cable detached Command */
	if (strncmp(cmd, "usb_cable_detach", 16) == 0) {
		usb_data_transfer_flag = 0;
		g_device_type = 0;
		usb_device_cfg_flag = 0;
		usb_get_desc_flag   = 0;
		usb_gadget_disconnect(_android_dev->gadget);
		return count;
	}

	/* USB connect/disconnect Test Commands */
	if (strncmp(cmd, "usb_connect", 11) == 0) {
		usb_gadget_connect(_android_dev->gadget);
		return count;
	}
	if (strncmp(cmd, "usb_disconnect", 14) == 0) {
		usb_gadget_disconnect(_android_dev->gadget);
		return count;
	}

	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (mot_android_vid_pid[i].name == NULL)
			return count;
		if (strlen(mot_android_vid_pid[i].name) > cnt)
			continue;
		if (strncmp(cmd, mot_android_vid_pid[i].name, cnt - 1) == 0) {
			temp_device_type = mot_android_vid_pid[i].type;
			strings_dev[STRING_CONFIG_IDX].s =
			mot_android_vid_pid[i].config_name;
			break;
		}
	}

	if (i == MAX_DEVICE_TYPE_NUM)
		return count;

	if (temp_device_type == g_device_type)
		return count;

	g_device_type = temp_device_type;
	force_reenumeration(_android_dev, g_device_type);
	return count;
}

static int event_pending(void)
{
	if ((usb_device_cfg_flag == g_device_type) && (g_device_type != 0))
		return 1;
	else if (adb_mode_changed_flag)
		return 1;
	else if (usb_get_desc_flag)
		return 1;
	else if (pc_mode_switch_flag)
		return 1;
	else
		return 0;
}

static unsigned int device_mode_change_poll(struct file *file,
					    struct poll_table_struct *wait)
{
	poll_wait(file, &device_mode_change_wait_q, wait);

	if (event_pending())
		return POLLIN | POLLRDNORM;
	else
		return 0;
}

static ssize_t device_mode_change_read(struct file *file, char *buf,
				       size_t count, loff_t *ppos)
{
	int ret, size, cnt;
	/* double check last zero */
	unsigned char no_changed[] = "none:\0";
	unsigned char adb_en_str[] = "adb_enable:\0";
	unsigned char adb_dis_str[] = "adb_disable:\0";
	unsigned char enumerated_str[] = "enumerated\0";
	unsigned char get_desc_str[] = "get_desc\0";
	unsigned char modswitch_str[50];

	/* Message format example:
	* none:adb_enable:enumerated
	*/

	if (!event_pending())
		return 0;

	/* append PC request mode */
	if (!pc_mode_switch_flag) {
		size = strlen(no_changed);
		ret = copy_to_user(buf, no_changed, size);
	} else {
		memset(modswitch_str, 0, 50);
		ret = get_func_thru_config(pc_mode_switch_flag);
		pc_mode_switch_flag = 0;
		if (ret == -1) {
			size = strlen(no_changed);
			ret = copy_to_user(buf, no_changed, size);
		} else {
			sprintf(modswitch_str, "%s",
				 mot_android_vid_pid[ret].name);
			strcat(modswitch_str, ":");
			size = strlen(modswitch_str);
			ret = copy_to_user(buf, modswitch_str, size);
		}
	}
	cnt = size;
	buf += size;

	/* append ADB status */
	if (!adb_mode_changed_flag) {
		size = strlen(no_changed);
		ret = copy_to_user(buf, no_changed, size);
	} else {
		if (atomic_read(&adb_enable_excl)) {
			size = strlen(adb_en_str);
			ret = copy_to_user(buf, adb_en_str, size);
		} else {
			size = strlen(adb_dis_str);
			ret = copy_to_user(buf, adb_dis_str, size);
		}
		adb_mode_changed_flag = 0;
	}
	cnt += size;
	buf += size;

	/* append USB enumerated state */
	if ((usb_device_cfg_flag == g_device_type) && (g_device_type != 0)) {
		usb_device_cfg_flag = 0;
		size = strlen(enumerated_str);
		ret += copy_to_user(buf, enumerated_str, size);

	} else {
		if (usb_get_desc_flag == 1) {
			usb_get_desc_flag = 0;
			size = strlen(get_desc_str);
			ret += copy_to_user(buf, get_desc_str, size);
		} else {
			size = strlen(no_changed) - 1;
			ret += copy_to_user(buf, no_changed, size);
		}
	}
	cnt += size;

	return cnt;
}

static const struct file_operations device_mode_change_fops = {
	.owner = THIS_MODULE,
	.open = device_mode_change_open,
	.write = device_mode_change_write,
	.poll = device_mode_change_poll,
	.read = device_mode_change_read,
	.release = device_mode_change_release,
};

static struct miscdevice mode_change_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "usb_device_mode",
	.fops = &device_mode_change_fops,
};
#endif

static int __init android_probe(struct platform_device *pdev)
{
	struct android_usb_platform_data *pdata = pdev->dev.platform_data;
	struct android_dev *dev = _android_dev;

	if (pdata) {
		if (pdata->vendor_id)
			device_desc.idVendor =
				__constant_cpu_to_le16(pdata->vendor_id);
		if (pdata->product_id) {
			dev->product_id = pdata->product_id;
			device_desc.idProduct =
				__constant_cpu_to_le16(pdata->product_id);
		}
		if (pdata->adb_product_id)
			dev->adb_product_id = pdata->adb_product_id;
		if (pdata->version)
			dev->version = pdata->version;

		if (pdata->product_name)
			strings_dev[STRING_PRODUCT_IDX].s = pdata->product_name;
		if (pdata->manufacturer_name)
			strings_dev[STRING_MANUFACTURER_IDX].s =
				pdata->manufacturer_name;
		if (pdata->serial_number)
			strings_dev[STRING_SERIAL_IDX].s = pdata->serial_number;
		dev->nluns = pdata->nluns;
	}

	return 0;
}

static struct platform_driver android_platform_driver = {
	.driver = {.name = "android_usb",},
	.probe = android_probe,
};

static int __init init(void)
{
	struct android_dev *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	_android_dev = dev;

	ret = platform_driver_register(&android_platform_driver);
	if (ret)
		return ret;
#ifdef CONFIG_USB_GADGET_ADB
	ret = misc_register(&adb_enable_device);
	if (ret) {
		platform_driver_unregister(&android_platform_driver);
		return ret;
	}
#endif
#ifdef CONFIG_USB_MOT_ANDROID
	ret = misc_register(&mode_change_device);
	if (ret) {
#ifdef CONFIG_USB_GADGET_ADB
		misc_deregister(&adb_enable_device);
#endif
		platform_driver_unregister(&android_platform_driver);
		return ret;
	}
#endif
#ifdef CONFIG_USB_GADGET_RAM_LOG
	ret = misc_register(&usb_logging_device);
	if (!ret) {
		log_task = kthread_create(usb_log_thread, NULL,
                                       "logging_thread");
       		if (!log_task)
                	printk(KERN_ERR "unable to start the kernel thread\n");
       		sema_init(&log_sema, 2);
	}
#endif
	ret = usb_composite_register(&android_usb_driver);
	if (ret) {
#ifdef CONFIG_USB_GADGET_RAM_LOG
		kfree(log_task);
		misc_deregister(&usb_logging_device);
#endif
#ifdef CONFIG_USB_GADGET_ADB
		misc_deregister(&adb_enable_device);
#endif
#ifdef CONFIG_USB_MOT_ANDROID
		misc_deregister(&mode_change_device);
#endif
		platform_driver_unregister(&android_platform_driver);
	}

	return ret;
}

module_init(init);

static void __exit cleanup(void)
{
	usb_composite_unregister(&android_usb_driver);
#ifdef CONFIG_USB_GADGET_RAM_LOG
	kfree(log_task);
	misc_deregister(&usb_logging_device);
#endif
#ifdef CONFIG_USB_GADGET_ADB
	misc_deregister(&adb_enable_device);
#endif
#ifdef CONFIG_USB_MOT_ANDROID
	misc_deregister(&mode_change_device);
#endif
	platform_driver_unregister(&android_platform_driver);
#if defined(CONFIG_USB_GADGET_SINGLE_ACM) || \
	defined(CONFIG_USB_GADGET_DUAL_ACM)
	gserial_cleanup();
#endif
#if defined(CONFIG_USB_GADGET_SINGLE_ECM) || \
	defined(CONFIG_USB_GADGET_DUAL_ECM) || \
	defined(CONFIG_USB_GADGET_FIVE_ECM)
	gether_cleanup();
#endif
	kfree(_android_dev);
	_android_dev = NULL;
}

module_exit(cleanup);
