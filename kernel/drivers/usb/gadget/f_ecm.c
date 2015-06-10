/*
 * f_ecm.c -- USB CDC Ethernet (ECM) link function driver
 *
 * Copyright (C) 2003-2005,2008 David Brownell
 * Copyright (C) 2008 Nokia Corporation
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
 */

/* #define VERBOSE_DEBUG */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/etherdevice.h>
#include <linux/inetdevice.h>

#include "u_ether.h"

#define USB_ENCAP_MSGLEN 12
#define USB_DEFAULT_IP 0x0214a8c0
#define USB_DEFAULT_SUBNET 0xf8ffffff
#define FALSE 0
#define TRUE 1

static u8 ecm_vendor_ipset_flag = FALSE;
static DECLARE_MUTEX(ecm_inet_sema);

int ecm_set_inet_addr(int cmd, struct gether *geth);

/*
 * This function is a "CDC Ethernet Networking Control Model" (CDC ECM)
 * Ethernet link.  The data transfer model is simple (packets sent and
 * received over bulk endpoints using normal short packet termination),
 * and the control model exposes various data and optional notifications.
 *
 * ECM is well standardized and (except for Microsoft) supported by most
 * operating systems with USB host support.  It's the preferred interop
 * solution for Ethernet over USB, at least for firmware based solutions.
 * (Hardware solutions tend to be more minimalist.)  A newer and simpler
 * "Ethernet Emulation Model" (CDC EEM) hasn't yet caught on.
 *
 * Note that ECM requires the use of "alternate settings" for its data
 * interface.  This means that the set_alt() method has real work to do,
 * and also means that a get_alt() method is required.
 */
#define CONTROL_INTERFACE_INDEX 3
struct ecm_ep_descs {
	struct usb_endpoint_descriptor	*in;
	struct usb_endpoint_descriptor	*out;
	struct usb_endpoint_descriptor	*notify;
};

enum ecm_notify_state {
	ECM_NOTIFY_NONE,		/* don't notify */
	ECM_NOTIFY_CONNECT,		/* issue CONNECT next */
	ECM_NOTIFY_SPEED,		/* issue SPEED_CHANGE next */
	ECM_NOTIFY_RESPONSE,		/* issue RESPONSE for the encapsulated command */
};

enum ecm_encap_resp {
	ECM_IFRESP_SUCCESS,			/* IP details set successfully */
	ECM_IFRESP_SETADDR_FAIL,		/* set IP address failed */
	ECM_IFRESP_SETNETMASK_FAIL,		/* set IP netmask failed */
	ECM_IFRESP_SETBCAST_FAIL,		/* set IP broadcast failed */
	ECM_IFRESP_SETIFFLAGS_FAIL,		/* set interface flags failed */
};

struct ecm_ifconfig {
	u32 ip_addr;
	u32 subnet_mask;
	enum ecm_encap_resp encap_ipresp;
	struct work_struct ecm_vendor_ip_wq;	/* workqueue to handle vendor-ip */
	struct work_struct ecm_default_ip_wq;	/* workqueue to handle default-ip */
	struct net_device *ecm_net_dev;
};

struct f_ecm {
	struct gether			port;
	u8				ctrl_id, data_id;

	char				ethaddr[14];

	struct ecm_ep_descs		fs;
	struct ecm_ep_descs		hs;

	struct usb_ep			*notify;
	struct usb_endpoint_descriptor	*notify_desc;
	struct usb_request		*notify_req;
	u8				notify_state;
	bool				is_open;
	int 				ecm_device_count;
	/* FIXME is_open needs some irq-ish locking
	 * ... possibly the same as port.ioport
	 */
	struct ecm_ifconfig ecm_ifcfg;
};

static inline struct f_ecm *func_to_ecm(struct usb_function *f)
{
	return container_of(f, struct f_ecm, port.func);
}

static inline struct f_ecm *vendor_ip_workq_to_ecm(struct work_struct *ecm_config_wq)
{
	return container_of(ecm_config_wq, struct f_ecm, ecm_ifcfg.ecm_vendor_ip_wq);
}

static inline struct f_ecm *default_ip_workq_to_ecm(struct work_struct *ecm_config_wq)
{
	return container_of(ecm_config_wq, struct f_ecm, ecm_ifcfg.ecm_default_ip_wq);
}

/* peak (theoretical) bulk transfer rate in bits-per-second */
static inline unsigned ecm_bitrate(struct usb_gadget *g)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return 13 * 512 * 8 * 1000 * 8;
	else
		return 19 *  64 * 1 * 1000 * 8;
}

/*-------------------------------------------------------------------------*/

/*
 * Include the status endpoint if we can, even though it's optional.
 *
 * Use wMaxPacketSize big enough to fit CDC_NOTIFY_SPEED_CHANGE in one
 * packet, to simplify cancellation; and a big transfer interval, to
 * waste less bandwidth.
 *
 * Some drivers (like Linux 2.4 cdc-ether!) "need" it to exist even
 * if they ignore the connect/disconnect notifications that real aether
 * can provide.  More advanced cdc configurations might want to support
 * encapsulated commands (vendor-specific, using control-OUT).
 */

#define LOG2_STATUS_INTERVAL_MSEC	5	/* 1 << 5 == 32 msec */
#define ECM_STATUS_BYTECOUNT		16	/* 8 byte header + data */
#define ECM_IP_DETAILS			1	/* message for IP settings */

/* interface descriptor: */

static struct usb_interface_descriptor ecm_control_intf __initdata = {
	.bLength =		sizeof ecm_control_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	/* .bInterfaceNumber = DYNAMIC */
	/* status endpoint is optional; this could be patched later */
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_COMM,
	.bInterfaceSubClass =	USB_CDC_SUBCLASS_ETHERNET,
	.bInterfaceProtocol =	USB_CDC_PROTO_NONE,
	/* .iInterface = DYNAMIC */
};

static struct usb_cdc_header_desc ecm_header_desc __initdata = {
	.bLength =		sizeof ecm_header_desc,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_HEADER_TYPE,

	.bcdCDC =		__constant_cpu_to_le16(0x0110),
};

static struct usb_cdc_union_desc ecm_union_desc __initdata = {
	.bLength =		sizeof(ecm_union_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_UNION_TYPE,
	/* .bMasterInterface0 =	DYNAMIC */
	/* .bSlaveInterface0 =	DYNAMIC */
};

static struct usb_cdc_ether_desc ecm_desc __initdata = {
	.bLength =		sizeof ecm_desc,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_ETHERNET_TYPE,

	/* this descriptor actually adds value, surprise! */
	/* .iMACAddress = DYNAMIC */
	.bmEthernetStatistics =	__constant_cpu_to_le32(0), /* no statistics */
	.wMaxSegmentSize =	__constant_cpu_to_le16(ETH_FRAME_LEN),
	.wNumberMCFilters =	__constant_cpu_to_le16(0),
	.bNumberPowerFilters =	0,
};

/* the default data interface has no endpoints ... */

static struct usb_interface_descriptor ecm_data_nop_intf __initdata = {
	.bLength =		sizeof ecm_data_nop_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bInterfaceNumber =	1,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_CDC_DATA,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	0,
	/* .iInterface = DYNAMIC */
};

/* ... but the "real" data interface has two bulk endpoints */

static struct usb_interface_descriptor ecm_data_intf __initdata = {
	.bLength =		sizeof ecm_data_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bInterfaceNumber =	1,
	.bAlternateSetting =	1,
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_CDC_DATA,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	0,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor fs_ecm_notify_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(ECM_STATUS_BYTECOUNT),
	.bInterval =		1 << LOG2_STATUS_INTERVAL_MSEC,
};

static struct usb_endpoint_descriptor fs_ecm_in_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_ecm_out_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *ecm_fs_function[] __initdata = {
	/* CDC ECM control descriptors */
	(struct usb_descriptor_header *) &ecm_control_intf,
	(struct usb_descriptor_header *) &ecm_header_desc,
	(struct usb_descriptor_header *) &ecm_union_desc,
	(struct usb_descriptor_header *) &ecm_desc,
	/* NOTE: status endpoint might need to be removed */
	(struct usb_descriptor_header *) &fs_ecm_notify_desc,
	/* data interface, altsettings 0 and 1 */
	(struct usb_descriptor_header *) &ecm_data_nop_intf,
	(struct usb_descriptor_header *) &ecm_data_intf,
	(struct usb_descriptor_header *) &fs_ecm_in_desc,
	(struct usb_descriptor_header *) &fs_ecm_out_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor hs_ecm_notify_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(ECM_STATUS_BYTECOUNT),
	.bInterval =		LOG2_STATUS_INTERVAL_MSEC + 4,
};
static struct usb_endpoint_descriptor hs_ecm_in_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_ecm_out_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};

static struct usb_descriptor_header *ecm_hs_function[] __initdata = {
	/* CDC ECM control descriptors */
	(struct usb_descriptor_header *) &ecm_control_intf,
	(struct usb_descriptor_header *) &ecm_header_desc,
	(struct usb_descriptor_header *) &ecm_union_desc,
	(struct usb_descriptor_header *) &ecm_desc,
	/* NOTE: status endpoint might need to be removed */
	(struct usb_descriptor_header *) &hs_ecm_notify_desc,
	/* data interface, altsettings 0 and 1 */
	(struct usb_descriptor_header *) &ecm_data_nop_intf,
	(struct usb_descriptor_header *) &ecm_data_intf,
	(struct usb_descriptor_header *) &hs_ecm_in_desc,
	(struct usb_descriptor_header *) &hs_ecm_out_desc,
	NULL,
};

/* string descriptors: */

static struct usb_string ecm_string_defs[] = {
	[0].s = "Motorola PLAN Interface",
	[1].s = NULL /* DYNAMIC */,
	[2].s = "CDC Ethernet Data",
#if defined ( CONFIG_USB_GADGET_DUAL_ECM) || \
    defined ( CONFIG_USB_GADGET_FIVE_ECM)
	[3].s = "Motorola WWAN Interface ",
	[4].s = NULL,
	[5].s = "CDC Ethernet Data",
#ifdef CONFIG_USB_GADGET_FIVE_ECM
	[6].s = "Motorola WWAN2 Interface ",
	[7].s = NULL,
	[8].s = "CDC Ethernet Data",
	[9].s = "Motorola WWAN3 Interface ",
	[10].s = NULL,
	[11].s = "CDC Ethernet Data",
	[12].s = "Motorola WWAN4 Interface ",
	[13].s = NULL,
	[14].s = "CDC Ethernet Data",
#endif
#endif
	{  } /* end of list */
};

static struct usb_gadget_strings ecm_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		ecm_string_defs,
};

static struct usb_gadget_strings *ecm_strings[] = {
	&ecm_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/

static void ecm_do_notify(struct f_ecm *ecm)
{
	struct usb_request		*req = ecm->notify_req;
	struct usb_cdc_notification	*event;
	struct usb_composite_dev	*cdev = ecm->port.func.config->cdev;
	__le32				*data;
	int				status;

	/* notification already in flight? */
	if (!req)
		return;

	event = req->buf;
	switch (ecm->notify_state) {
	case ECM_NOTIFY_NONE:
		return;

	case ECM_NOTIFY_CONNECT:
		event->bNotificationType = USB_CDC_NOTIFY_NETWORK_CONNECTION;
		if (ecm->is_open)
			event->wValue = cpu_to_le16(1);
		else
			event->wValue = cpu_to_le16(0);
		event->wLength = 0;
		req->length = sizeof *event;

		DBG(cdev, "notify connect %s\n",
				ecm->is_open ? "true" : "false");
		ecm->notify_state = ECM_NOTIFY_SPEED;
		break;

	case ECM_NOTIFY_SPEED:
		event->bNotificationType = USB_CDC_NOTIFY_SPEED_CHANGE;
		event->wValue = cpu_to_le16(0);
		event->wLength = cpu_to_le16(8);
		req->length = ECM_STATUS_BYTECOUNT;

		/* SPEED_CHANGE data is up/down speeds in bits/sec */
		data = req->buf + sizeof *event;
		data[0] = cpu_to_le32(ecm_bitrate(cdev->gadget));
		data[1] = data[0];

		DBG(cdev, "notify speed %d\n", ecm_bitrate(cdev->gadget));
		ecm->notify_state = ECM_NOTIFY_NONE;
		break;

	case ECM_NOTIFY_RESPONSE:
		event->bNotificationType = USB_CDC_NOTIFY_RESPONSE_AVAILABLE;
		event->wValue = 0;
		event->wLength = 0;
		req->length = sizeof *event;

		DBG(cdev, "notify response available\n");
		ecm->notify_state = ECM_NOTIFY_NONE;
		break;
	}
	event->bmRequestType = 0xA1;
	event->wIndex = cpu_to_le16(ecm->ctrl_id);

	ecm->notify_req = NULL;
	status = usb_ep_queue(ecm->notify, req, GFP_ATOMIC);
	if (status < 0) {
		ecm->notify_req = req;
		DBG(cdev, "notify --> %d\n", status);
	}
}

static void ecm_notify(struct f_ecm *ecm)
{
	/* NOTE on most versions of Linux, host side cdc-ethernet
	 * won't listen for notifications until its netdevice opens.
	 * The first notification then sits in the FIFO for a long
	 * time, and the second one is queued.
	 */
	ecm->notify_state = ECM_NOTIFY_CONNECT;
	ecm_do_notify(ecm);
}

static void ecm_command_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_ecm *ecm = req->context;
	u32 cmd_type, msg_len = 0;
	__le32 *data_buff;

	/* received data using USB_CDC_SEND_ENCAPSULATED_COMMAND */
	data_buff = (__le32 *)(req->buf);
	cmd_type = get_unaligned_le32(data_buff++);

	switch(cmd_type) {
		case ECM_IP_DETAILS:
			*data_buff += 1;
			ecm->ecm_ifcfg.ip_addr = get_unaligned_be32(data_buff++);
			ecm->ecm_ifcfg.subnet_mask = get_unaligned_be32(data_buff++);
			ecm->ecm_ifcfg.ecm_net_dev = (struct net_device *)&(ecm->port.ioport->net);
			schedule_work(&(ecm->ecm_ifcfg.ecm_vendor_ip_wq));
			break;

		default:
			printk(KERN_INFO "%s unknown command type %u of length %u\n", __func__, cmd_type, msg_len);
			break;
	}
}

static void ecm_response_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_ecm *ecm = req->context;
	ecm->ecm_ifcfg.encap_ipresp = ECM_IFRESP_SUCCESS;
}

static void ecm_notify_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_ecm			*ecm = req->context;
	struct usb_composite_dev	*cdev = ecm->port.func.config->cdev;
	struct usb_cdc_notification	*event = req->buf;

	switch (req->status) {
	case 0:
		/* no fault */
		break;
	case -ECONNRESET:
	case -ESHUTDOWN:
		ecm->notify_state = ECM_NOTIFY_NONE;
		break;
	default:
		DBG(cdev, "event %02x --> %d\n",
			event->bNotificationType, req->status);
		break;
	}
	ecm->notify_req = req;
	ecm_do_notify(ecm);
}

static int ecm_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_ecm		*ecm = func_to_ecm(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	/* composite driver infrastructure handles everything except
	 * CDC class messages; interface activation uses set_alt().
	 */
	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_SET_ETHERNET_PACKET_FILTER:
		/* see 6.2.30: no data, wIndex = interface,
		 * wValue = packet filter bitmap
		 */
		if (w_length != 0 || w_index != ecm->ctrl_id)
			goto invalid;
		DBG(cdev, "packet filter %02x\n", w_value);
		/* REVISIT locking of cdc_filter.  This assumes the UDC
		 * driver won't have a concurrent packet TX irq running on
		 * another CPU; or that if it does, this write is atomic...
		 */
		ecm->port.cdc_filter = w_value;
		value = 0;
		break;
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_SEND_ENCAPSULATED_COMMAND:

		if (w_length > req->length || w_value || w_index != ecm->ctrl_id) {
			printk("%s: Invalid len/value of send encap command\n", __func__);
			goto invalid;
		}

		/* read the request sent from the host, process it later */
		value = w_length;
		req->complete = ecm_command_complete;
		req->context = ecm;
		memset(req->buf, 0, USB_ENCAP_MSGLEN);
		break;

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_GET_ENCAPSULATED_RESPONSE:

		if (w_value || w_index != ecm->ctrl_id) {
			printk("%s: Invalid len/value of get encap command\n", __func__);
			goto invalid;
		}
		else {
			/* send response to the host */
			memcpy((void*)req->buf, (void*)&(ecm->ecm_ifcfg.encap_ipresp), sizeof(ecm->ecm_ifcfg.encap_ipresp));
			req->complete = ecm_response_complete;
			value = sizeof(ecm->ecm_ifcfg.encap_ipresp);
			/* else stalls.. According to spec avoid such scenario */
		}
		break;

	/* and optionally:
	 * case USB_CDC_SET_ETHERNET_MULTICAST_FILTERS:
	 * case USB_CDC_SET_ETHERNET_PM_PATTERN_FILTER:
	 * case USB_CDC_GET_ETHERNET_PM_PATTERN_FILTER:
	 * case USB_CDC_GET_ETHERNET_STATISTIC:
	 */

	default:
invalid:
		DBG(cdev, "invalid control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		DBG(cdev, "ecm req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "ecm req %02x.%02x response err %d\n",
					ctrl->bRequestType, ctrl->bRequest,
					value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}


static int ecm_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_ecm		*ecm = func_to_ecm(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	/* Control interface has only altsetting 0 */
	if (intf == ecm->ctrl_id) {
		if (alt != 0)
			goto fail;

		if (ecm->notify->driver_data) {
			VDBG(cdev, "reset ecm control %d\n", intf);
			usb_ep_disable(ecm->notify);
		} else {
			VDBG(cdev, "init ecm ctrl %d\n", intf);
			ecm->notify_desc = ep_choose(cdev->gadget,
					ecm->hs.notify,
					ecm->fs.notify);
		}
		usb_ep_enable(ecm->notify, ecm->notify_desc);
		ecm->notify->driver_data = ecm;

	/* Data interface has two altsettings, 0 and 1 */
	} else if (intf == ecm->data_id) {
		if (alt > 1)
			goto fail;

		if (ecm->port.in_ep->driver_data) {
			DBG(cdev, "reset ecm\n");
			gether_disconnect(&ecm->port);
		}

		if (!ecm->port.in) {
			DBG(cdev, "init ecm\n");
			ecm->port.in = ep_choose(cdev->gadget,
					ecm->hs.in, ecm->fs.in);
			ecm->port.out = ep_choose(cdev->gadget,
					ecm->hs.out, ecm->fs.out);
		}

		/* CDC Ethernet only sends data in non-default altsettings.
		 * Changing altsettings resets filters, statistics, etc.
		 */
		if (alt == 1) {
			struct net_device	*net;

			/* Enable zlps by default for ECM conformance;
			 * override for musb_hdrc (avoids txdma ovhead)
			 * and sa1100 (can't).
			 */
			ecm->port.is_zlp_ok = !(
				   gadget_is_sa1100(cdev->gadget)
				|| gadget_is_musbhdrc(cdev->gadget)
				);
			ecm->port.cdc_filter = DEFAULT_FILTER;
			DBG(cdev, "activate ecm\n");
			net = gether_connect(&ecm->port);
			if (IS_ERR(net))
				return PTR_ERR(net);
		}

		/* NOTE this can be a minor disagreement with the ECM spec,
		 * which says speed notifications will "always" follow
		 * connection notifications.  But we allow one connect to
		 * follow another (if the first is in flight), and instead
		 * just guarantee that a speed notification is always sent.
		 */
		ecm_notify(ecm);
	} else
		goto fail;

	return 0;
fail:
	return -EINVAL;
}

/* Because the data interface supports multiple altsettings,
 * this ECM function *MUST* implement a get_alt() method.
 */
static int ecm_get_alt(struct usb_function *f, unsigned intf)
{
	struct f_ecm		*ecm = func_to_ecm(f);

	if (intf == ecm->ctrl_id)
		return 0;
	return ecm->port.in_ep->driver_data ? 1 : 0;
}

static void ecm_disable(struct usb_function *f)
{
	struct f_ecm		*ecm = func_to_ecm(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	DBG(cdev, "ecm deactivated\n");

	if (ecm->port.in_ep->driver_data)
		gether_disconnect(&ecm->port);

	if (ecm->notify->driver_data) {
		usb_ep_disable(ecm->notify);
		ecm->notify->driver_data = NULL;
		ecm->notify_desc = NULL;
	}
}


static void ecm_suspend(struct usb_function *f)
{
	struct usb_composite_dev *cdev = f->config->cdev;

	DBG(cdev, "ecm suspended\n");

	/* Revisit: ECM interface suspend handling */
}

static void ecm_resume(struct usb_function *f)
{
	struct usb_composite_dev *cdev = f->config->cdev;

	DBG(cdev, "ecm resumed\n");
	/* Revisit: ECM interface resume handling */
}

/*-------------------------------------------------------------------------*/

/*
 * Callbacks let us notify the host about connect/disconnect when the
 * net device is opened or closed.
 *
 * For testing, note that link states on this side include both opened
 * and closed variants of:
 *
 *   - disconnected/unconfigured
 *   - configured but inactive (data alt 0)
 *   - configured and active (data alt 1)
 *
 * Each needs to be tested with unplug, rmmod, SET_CONFIGURATION, and
 * SET_INTERFACE (altsetting).  Remember also that "configured" doesn't
 * imply the host is actually polling the notification endpoint, and
 * likewise that "active" doesn't imply it's actually using the data
 * endpoints for traffic.
 */

static void ecm_open(struct gether *geth)
{
	struct f_ecm		*ecm = func_to_ecm(&geth->func);

	DBG(ecm->port.func.config->cdev, "%s\n", __func__);

	ecm->is_open = true;
	ecm_notify(ecm);
}

static void ecm_close(struct gether *geth)
{
	struct f_ecm		*ecm = func_to_ecm(&geth->func);

	DBG(ecm->port.func.config->cdev, "%s\n", __func__);

	ecm->is_open = false;
	ecm_notify(ecm);
}

/*-------------------------------------------------------------------------*/

/* ethernet function driver setup/binding */

static int __init
ecm_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_ecm		*ecm = func_to_ecm(f);
	int			status;
	struct usb_ep		*ep;

	/* allocate instance-specific interface IDs */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	ecm->ctrl_id = status;

	ecm_control_intf.bInterfaceNumber = status;
	ecm_union_desc.bMasterInterface0 = status;

	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	ecm->data_id = status;

	ecm_data_nop_intf.bInterfaceNumber = status;
	ecm_data_intf.bInterfaceNumber = status;
	ecm_union_desc.bSlaveInterface0 = status;

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &fs_ecm_in_desc);
	if (!ep)
		goto fail;
	ep->dma_required = 1;
	ecm->port.in_ep = ep;
	ep->driver_data = cdev;	/* claim */

	ep = usb_ep_autoconfig(cdev->gadget, &fs_ecm_out_desc);
	if (!ep)
		goto fail;
	ep->dma_required = 1;
	ecm->port.out_ep = ep;
	ep->driver_data = cdev;	/* claim */

	/* NOTE:  a status/notification endpoint is *OPTIONAL* but we
	 * don't treat it that way.  It's simpler, and some newer CDC
	 * profiles (wireless handsets) no longer treat it as optional.
	 */
	ep = usb_ep_autoconfig(cdev->gadget, &fs_ecm_notify_desc);
	if (!ep)
		goto fail;
	ep->dma_required = 0;
	ecm->notify = ep;
	ep->driver_data = cdev;	/* claim */

	status = -ENOMEM;

	/* allocate notification request and buffer */
	ecm->notify_req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!ecm->notify_req)
		goto fail;
	ecm->notify_req->buf = kmalloc(ECM_STATUS_BYTECOUNT, GFP_KERNEL);
	if (!ecm->notify_req->buf)
		goto fail;
	ecm->notify_req->context = ecm;
	ecm->notify_req->complete = ecm_notify_complete;

	/* copy descriptors, and track endpoint copies */
	f->descriptors = usb_copy_descriptors(ecm_fs_function);
	if (!f->descriptors)
		goto fail;

	ecm->fs.in = usb_find_endpoint(ecm_fs_function,
			f->descriptors, &fs_ecm_in_desc);
	ecm->fs.out = usb_find_endpoint(ecm_fs_function,
			f->descriptors, &fs_ecm_out_desc);
	ecm->fs.notify = usb_find_endpoint(ecm_fs_function,
			f->descriptors, &fs_ecm_notify_desc);

	/* support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		hs_ecm_in_desc.bEndpointAddress =
				fs_ecm_in_desc.bEndpointAddress;
		hs_ecm_out_desc.bEndpointAddress =
				fs_ecm_out_desc.bEndpointAddress;
		hs_ecm_notify_desc.bEndpointAddress =
				fs_ecm_notify_desc.bEndpointAddress;

		/* copy descriptors, and track endpoint copies */
		f->hs_descriptors = usb_copy_descriptors(ecm_hs_function);
		if (!f->hs_descriptors)
			goto fail;

		ecm->hs.in = usb_find_endpoint(ecm_hs_function,
				f->hs_descriptors, &hs_ecm_in_desc);
		ecm->hs.out = usb_find_endpoint(ecm_hs_function,
				f->hs_descriptors, &hs_ecm_out_desc);
		ecm->hs.notify = usb_find_endpoint(ecm_hs_function,
				f->hs_descriptors, &hs_ecm_notify_desc);
	}

	/* NOTE:  all that is done without knowing or caring about
	 * the network link ... which is unavailable to this code
	 * until we're activated via set_alt().
	 */

	ecm->port.open = ecm_open;
	ecm->port.close = ecm_close;

	DBG(cdev, "CDC Ethernet: %s speed IN/%s OUT/%s NOTIFY/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			ecm->port.in_ep->name, ecm->port.out_ep->name,
			ecm->notify->name);

	return 0;

fail:
	if (f->descriptors)
		usb_free_descriptors(f->descriptors);

	if (ecm->notify_req) {
		kfree(ecm->notify_req->buf);
		usb_ep_free_request(ecm->notify, ecm->notify_req);
	}

	/* we might as well release our claims on endpoints */
	if (ecm->notify)
		ecm->notify->driver_data = NULL;
	if (ecm->port.out)
		ecm->port.out_ep->driver_data = NULL;
	if (ecm->port.in)
		ecm->port.in_ep->driver_data = NULL;

	ERROR(cdev, "%s: can't bind, err %d\n", f->name, status);

	return status;
}

static void
ecm_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_ecm		*ecm = func_to_ecm(f);

	DBG(c->cdev, "ecm unbind\n");

	if (gadget_is_dualspeed(c->cdev->gadget))
		usb_free_descriptors(f->hs_descriptors);
	usb_free_descriptors(f->descriptors);

	kfree(ecm->notify_req->buf);
	usb_ep_free_request(ecm->notify, ecm->notify_req);

	ecm_string_defs[(CONTROL_INTERFACE_INDEX * ecm->ecm_device_count) + 1].s = NULL;
	kfree(ecm);
}

static void ecm_set_default_ip(struct work_struct *ecm_work_struct)
{
	struct f_ecm *ecm = default_ip_workq_to_ecm(ecm_work_struct);
	int err = -EINVAL;

	err = ecm_set_inet_addr(SIOCPRIVSETDEFIP, &ecm->port);

	if (!err) {
		ecm_notify(ecm);
	}
	else {
		printk(KERN_INFO "Error in default inet address configuration: %d\n", err);
	}
}

static void ecm_set_vendor_ip(struct work_struct *ecm_work_struct)
{
	struct f_ecm *ecm = vendor_ip_workq_to_ecm(ecm_work_struct);
	int err = -EINVAL;

	err = ecm_set_inet_addr(SIOCPRIVSETVENIP, &ecm->port);

	if (!err) {
		ecm_do_notify(ecm);
	}
	else {
		printk(KERN_ERR "Error in vendor inet addr configuration:%d\n", err);
	}
}

int ecm_schedule_set_addr(int cmd, struct gether *geth)
{
	struct f_ecm *ecm = func_to_ecm(&geth->func);

	schedule_work(&(ecm->ecm_ifcfg.ecm_default_ip_wq));
	return 0;
}

int ecm_set_inet_addr(int cmd, struct gether *geth)
{
	struct f_ecm *ecm = func_to_ecm(&geth->func);
	struct ifreq ifr;
	mm_segment_t saved_fs;
	int err = -EINVAL;
	struct sockaddr_in *sin;

	if (down_interruptible(&ecm_inet_sema) < 0) {
		printk(KERN_INFO "sema inet timeout\n");
		err = -EAGAIN;
		return err;
	}

	if (cmd == SIOCPRIVSETDEFIP && !ecm_vendor_ipset_flag) {
		/* Vendor IP has not yet arrived, set default IP */
		ecm->ecm_ifcfg.ip_addr = USB_DEFAULT_IP;
		ecm->ecm_ifcfg.subnet_mask = USB_DEFAULT_SUBNET;
		ecm->ecm_ifcfg.ecm_net_dev = (struct net_device *)ecm->port.ioport->net;
	}
	else if (cmd == SIOCPRIVSETDEFIP && ecm_vendor_ipset_flag) {
		/* Vendor IP has already arrived, interface is up */
		/* do nothing */
		err = 0;
		goto out;
	}

	if (cmd == SIOCPRIVSETVENIP) {
		/* Vendor IP has arrived, set a flag */
		ecm_vendor_ipset_flag = TRUE;
	}

	memset(&ifr, 0, sizeof(ifr));
	sin = (void *)&(ifr.ifr_ifru.ifru_addr);
	strncpy(ifr.ifr_ifrn.ifrn_name, "usb0", strlen("usb0") + 1);
	sin->sin_family = AF_INET;

	sin->sin_addr.s_addr = ecm->ecm_ifcfg.ip_addr;
	saved_fs = get_fs();
	set_fs(get_ds());

	err = devinet_ioctl(dev_net(ecm->ecm_ifcfg.ecm_net_dev), \
		SIOCSIFADDR, (void __user *)&ifr);
	if (err) {
		printk(KERN_INFO "%s: Error in SIOCSIFADDR\n", __func__);
		ecm->ecm_ifcfg.encap_ipresp = ECM_IFRESP_SETADDR_FAIL;
	}

	sin->sin_addr.s_addr = ecm->ecm_ifcfg.subnet_mask;
	err = devinet_ioctl(dev_net(ecm->ecm_ifcfg.ecm_net_dev), \
		SIOCSIFNETMASK, (void __user *) &ifr);
	if (err) {
		printk(KERN_INFO "%s: Error in SIOCSIFNETMASK = %d\n", __func__, err);
		ecm->ecm_ifcfg.encap_ipresp |= ECM_IFRESP_SETNETMASK_FAIL;
	}

	sin->sin_addr.s_addr = ecm->ecm_ifcfg.ip_addr | ~(ecm->ecm_ifcfg.subnet_mask);
	err = devinet_ioctl(dev_net(ecm->ecm_ifcfg.ecm_net_dev), \
		SIOCSIFBRDADDR, (void __user *) &ifr);
	if (err) {
		printk(KERN_INFO "%s: Error in SIOCSIFBRDADDR = %d\n", __func__, err);
		ecm->ecm_ifcfg.encap_ipresp |= ECM_IFRESP_SETBCAST_FAIL;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_ifrn.ifrn_name, "usb0", strlen("usb0") + 1);
	ifr.ifr_flags = ((ecm->ecm_ifcfg.ecm_net_dev->flags) | IFF_UP);
	err = devinet_ioctl(dev_net(ecm->ecm_ifcfg.ecm_net_dev), \
		SIOCSIFFLAGS, (void __user *) &ifr);
	if (err) {
		printk(KERN_INFO "%s: Error in SIOCSIFFLAGS = %d\n", __func__, err);
		ecm->ecm_ifcfg.encap_ipresp |= ECM_IFRESP_SETIFFLAGS_FAIL;
	}

	set_fs(saved_fs);

out:
	up(&ecm_inet_sema);

	return err;
}

/**
 * ecm_bind_config - add CDC Ethernet network link to a configuration
 * @c: the configuration to support the network link
 * @ethaddr: a buffer in which the ethernet address of the host side
 *	side of the link was recorded
 * Context: single threaded during gadget setup
 *
 * Returns zero on success, else negative errno.
 *
 * Caller must have called @gether_setup().  Caller is also responsible
 * for calling @gether_cleanup() before module unload.
 */
int __init ecm_bind_config(struct usb_configuration *c, u8 ethaddr[ETH_ALEN])
{
	struct f_ecm	*ecm;
	int		status;
	static int ecm_device_count=0;

	if (ecm_device_count >= MAX_NUM_ECM_DEVICES)
		return -EINVAL;

	if (!can_support_ecm(c->cdev->gadget) || !ethaddr)
		return -EINVAL;

	/* maybe allocate device-global string IDs */
	/* control interface label */
	status = usb_string_id(c->cdev);
	if (status < 0)
		return status;
	ecm_string_defs[CONTROL_INTERFACE_INDEX * ecm_device_count].id = status;
	ecm_control_intf.iInterface = status;

	/* data interface label */
	status = usb_string_id(c->cdev);
	if (status < 0)
		return status;
	ecm_string_defs[(CONTROL_INTERFACE_INDEX * ecm_device_count) + 2].id = status;
	ecm_data_intf.iInterface = status;

	/* MAC address */
	status = usb_string_id(c->cdev);
	if (status < 0)
		return status;
	ecm_string_defs[(CONTROL_INTERFACE_INDEX * ecm_device_count) + 1].id = status;
	ecm_desc.iMACAddress = status;

	/* allocate and initialize one new instance */
	ecm = kzalloc(sizeof *ecm, GFP_KERNEL);
	if (!ecm)
		return -ENOMEM;

	if (!ecm_device_count) {
		INIT_WORK(&(ecm->ecm_ifcfg.ecm_vendor_ip_wq), ecm_set_vendor_ip);
		INIT_WORK(&(ecm->ecm_ifcfg.ecm_default_ip_wq), ecm_set_default_ip);
	}

	/* export host's Ethernet address in CDC format */
	snprintf(ecm->ethaddr, sizeof ecm->ethaddr,
		"%02X%02X%02X%02X%02X%02X",
		ethaddr[0], ethaddr[1], ethaddr[2],
		ethaddr[3], ethaddr[4], ethaddr[5]);
	ecm_string_defs[(CONTROL_INTERFACE_INDEX * ecm_device_count) + 1].s = ecm->ethaddr;

        ecm->ecm_device_count = ecm_device_count;

	ecm->port.cdc_filter = DEFAULT_FILTER;

	ecm->port.func.name = "cdc_ethernet";
	ecm->port.func.strings = ecm_strings;
	/* descriptors are per-instance copies */
	ecm->port.func.bind = ecm_bind;

	ecm->port.func.unbind = ecm_unbind;

	ecm->port.func.set_alt = ecm_set_alt;
	ecm->port.func.get_alt = ecm_get_alt;
	ecm->port.func.setup = ecm_setup;
	ecm->port.func.disable = ecm_disable;
	ecm->port.func.suspend = ecm_suspend;
	ecm->port.func.resume = ecm_resume;

	status = usb_add_function(c, &ecm->port.func);
	if (status) {
		ecm_string_defs[(CONTROL_INTERFACE_INDEX * ecm_device_count) + 1].s = NULL;
		kfree(ecm);
	}
	else
		ecm_device_count++;

	return status;
}


int __init ecm_function_add(struct usb_composite_dev *cdev,
			struct usb_configuration *c)
{
	int ret, loop_count;

	for (loop_count = 0; loop_count < MAX_NUM_ECM_DEVICES; loop_count++) {
		ret = ecm_bind_config(c , hostaddr[loop_count]);
		if (ret)
			return ret;
	}
}
