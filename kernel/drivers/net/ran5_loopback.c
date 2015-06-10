/*
 * Copyright Motorola, Inc. 2010
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
 * @file ran5_loopback.c
 * @brief Pseudo driver which loops back packets.
 *
 * This driver is used to perform RAN5 Loopback testing as specified in
 * 3GPP 36.509. A one shot timer can be set, when the first packet is received,
 * the timer is started and any packets received while the timer is running,
 * are buffered until the buffer is full.  When the timer expires, the buffered
 * packets are looped back.  Thereafter, packets are looped back without
 * buffering.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/rtnetlink.h>
#include <net/dst.h>
#include <net/rtnetlink.h>
#include <linux/percpu.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/in6.h>
#include <linux/if_arp.h>


#define MAX_BUFFERED_BYTES  3000


/* Used for the timer/buffering logic */
struct ran5_loopback_priv {
	uint16_t             buffered_bytes;
	bool                 buffer_ip_pdus;
	struct sk_buff_head  skbq;
	struct timer_list    ip_delay_timer;
	uint8_t              ip_delay_in_seconds;
	uint8_t              ttl_inc;
	/* Protects the priv data when accessed from timer callback */
	spinlock_t           lock;
};


/* Timer utilities */
static void start_ip_delay_timer(unsigned short ip_delay_timer_seconds,
				   struct ran5_loopback_priv *priv);
static void ip_delay_timedout(unsigned long ptr);




static int ran5_loopback_ioctl(struct net_device *dev,
			       struct ifreq *ifr,
			       int cmd);
static int ran5_loopback_xmit(struct sk_buff *skb, struct net_device *dev);

static int ran5_loopback_set_address(struct net_device *dev, void *p)
{
	struct sockaddr *sa = p;

	if (!is_valid_ether_addr(sa->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, sa->sa_data, ETH_ALEN);
	return 0;
}

/* fake multicast ability */
static void set_multicast_list(struct net_device *dev)
{
}

static const struct net_device_ops ran5_loopback_netdev_ops = {
	.ndo_start_xmit		= ran5_loopback_xmit,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_multicast_list = set_multicast_list,
	.ndo_set_mac_address	= ran5_loopback_set_address,
	.ndo_do_ioctl 		= ran5_loopback_ioctl
};

static void ran5_loopback_setup(struct net_device *dev)
{
	struct ran5_loopback_priv *priv;
	ether_setup(dev);

	/* Initialize the device structure. */
	dev->netdev_ops = &ran5_loopback_netdev_ops;
	dev->destructor = free_netdev;

	/* Fill in device structure with ethernet-generic values. */
	dev->tx_queue_len = 0;
	dev->flags = 0;
	dev->flags |= IFF_NOARP;
	dev->flags &= ~IFF_MULTICAST;
	random_ether_addr(dev->dev_addr);

	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct ran5_loopback_priv));
	skb_queue_head_init(&priv->skbq);
	spin_lock_init(&priv->lock);
}


static int ran5_loopback_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ran5_loopback_priv *priv;
	struct iphdr *ih;
	struct ipv6hdr *ipv6h   = NULL;
	bool discard_packet = false;
	bool loop_back = false;

	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	skb_orphan(skb);

	skb->protocol = eth_type_trans(skb, dev);

	ih = (struct iphdr *)skb_network_header(skb);
	ipv6h = (struct ipv6hdr *)skb_network_header(skb);

	if (skb->dst != NULL) {
		/* This needs to be done so kernel will put it through
		   routing logic again and forward. */
		dst_release(skb->dst);
		skb->dst = NULL;
	}

	/* Ignore the IPv6 ICMP router solicitation */
	if ((ih->version == 6) &&
		(ipv6h->nexthdr == IPPROTO_ICMPV6)) {
		discard_packet = true;
	}

	if (false == discard_packet) {
		priv = netdev_priv(dev);
		spin_lock_bh(&priv->lock);

		if (priv->ttl_inc > 0) {
			/* Restore the TTL/hop_limit since it gets
			decremented by kernel routing, 3GPP 36.509 spec
			says header shouldn't be modified. Ideally
			iptables would be used instead to mangle the ttl
			because of boundary condition */
			if (ih->version == 4) {
				if ((ih->ttl + priv->ttl_inc) <= 255) {
					ih->ttl += priv->ttl_inc;
					ih->check = 0;
					ih->check =
						ip_fast_csum(
							(unsigned char *)ih,
							 ih->ihl);
				}
			} else {
				if ((ipv6h->hop_limit + priv->ttl_inc) <= 255)
					ipv6h->hop_limit += priv->ttl_inc;
			}
		}

		if (true == priv->buffer_ip_pdus) {
			if ((priv->buffered_bytes + skb->len)
					< MAX_BUFFERED_BYTES) {
				skb_queue_tail(&priv->skbq, skb);
				priv->buffered_bytes += skb->len;

				if (!timer_pending(&priv->ip_delay_timer))
					start_ip_delay_timer(
					    priv->ip_delay_in_seconds, priv);
			} else {
				printk(KERN_INFO "ran5: packet too big for buffer!\n");
				discard_packet = true;
			}
		} else {
			loop_back = true;
		}
		spin_unlock_bh(&priv->lock);
	}

	if (true == discard_packet) {
		printk(KERN_INFO "ran5: discard packet\n");
		dev_kfree_skb(skb);
	} else if (true == loop_back) {
		netif_rx(skb);
	}

	return 0;
}




static int ran5_loopback_validate(struct nlattr *tb[], struct nlattr *data[])
{
	if (tb[IFLA_ADDRESS]) {
		if (nla_len(tb[IFLA_ADDRESS]) != ETH_ALEN)
			return -EINVAL;
		if (!is_valid_ether_addr(nla_data(tb[IFLA_ADDRESS])))
			return -EADDRNOTAVAIL;
	}
	return 0;
}

static int ran5_loopback_ioctl(struct net_device *dev,
			       struct ifreq *ifr,
			       int cmd)
{
	struct ran5_loopback_priv *priv = netdev_priv(dev);
	uint8_t                   data[2] = {0};

	if (copy_from_user(data, ifr->ifr_data, sizeof(data)) == 0) {
		spin_lock_bh(&priv->lock);
		priv->ip_delay_in_seconds = data[0];
		priv->ttl_inc = data[1];
		if (priv->ip_delay_in_seconds > 0)
			priv->buffer_ip_pdus = true;
		spin_unlock_bh(&priv->lock);
	}
	return 0;
}

static void start_ip_delay_timer(unsigned short ip_delay_timer_seconds,
				 struct ran5_loopback_priv *priv)
{
	printk(KERN_INFO "ran5: start_ip_delay_timer\n");
	/* init the timer structure */
	init_timer(&priv->ip_delay_timer);
	priv->ip_delay_timer.function = ip_delay_timedout;
	priv->ip_delay_timer.data = (unsigned long)priv;
	priv->ip_delay_timer.expires = jiffies + (HZ * ip_delay_timer_seconds);
	add_timer(&priv->ip_delay_timer);
}

static void ip_delay_timedout(unsigned long ptr)
{
	struct sk_buff *skb;
	struct ran5_loopback_priv *priv = (struct ran5_loopback_priv *)ptr;
	printk(KERN_INFO "ran5: ip_delay_timedout\n");

	spin_lock_bh(&priv->lock);
	priv->buffer_ip_pdus = false;

	while ((skb = skb_dequeue(&priv->skbq)) != NULL) {
		printk(KERN_INFO "ran5: loopback dequeue skb\n");
		netif_rx(skb);
	}

	priv->buffered_bytes = 0;
	spin_unlock_bh(&priv->lock);
}


static struct rtnl_link_ops ran5_loopback_link_ops __read_mostly = {
	.kind		= "ran5_loopback",
	.setup		= ran5_loopback_setup,
	.validate	= ran5_loopback_validate,
};


static int __init ran5_loopback_init_net(void)
{
	struct net_device *dev_ran5_loopback;
	int err;

	dev_ran5_loopback = alloc_netdev(sizeof(struct  ran5_loopback_priv),
					"ran5_loop", ran5_loopback_setup);
	if (!dev_ran5_loopback)
		return -ENOMEM;

	err = dev_alloc_name(dev_ran5_loopback, dev_ran5_loopback->name);
	if (err < 0)
		goto err;

	dev_ran5_loopback->rtnl_link_ops = &ran5_loopback_link_ops;
	err = register_netdevice(dev_ran5_loopback);
	if (err < 0)
		goto err;
	return 0;

err:
	free_netdev(dev_ran5_loopback);
	return err;
}

static int __init ran5_loopback_init_module(void)
{
	int err = 0;

	rtnl_lock();
	err = __rtnl_link_register(&ran5_loopback_link_ops);

	if (!err)
		err = ran5_loopback_init_net();

	if (err < 0)
		__rtnl_link_unregister(&ran5_loopback_link_ops);
	rtnl_unlock();

	return err;
}

static void __exit ran5_loopback_cleanup_module(void)
{
	rtnl_link_unregister(&ran5_loopback_link_ops);
}

module_init(ran5_loopback_init_module);
module_exit(ran5_loopback_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("ran5_loopback");
