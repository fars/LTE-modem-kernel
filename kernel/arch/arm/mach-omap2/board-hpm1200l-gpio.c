/*
 * board-hpm1200l-gpio.c
 *
 * Copyright (C) 2009 Motorola, Inc.
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

/* Date	 Author	  Comment
 * ===========  ==============  ==============================================
 * Jun-23-2009  Motorola	Initial revision.
 */

#include <linux/module.h>

#ifdef CONFIG_GPIODEV
#include <linux/gpiodev.h>
#include <asm/prom.h>


#define GPIO_DEVICE_SIZE 20
#define GPIO_DEVICE_UNUSED 0xFFFF

static struct gpio_device gpio_devs[GPIO_DEVICE_SIZE] = {
/* TODO: Fix this for slide detect */
#if 0
	{
		111,
		"slide_interrupt",
		GPIODEV_CONFIG_INPUT | GPIODEV_CONFIG_INT_LLEV,
		GPIODEV_CONFIG_INVALID,
		GPIODEV_FLAG_CONFIGURABLE | GPIODEV_FLAG_LOWLEVELACCESS,
	},
#endif
	{
		7,
		"gps_reset",
		GPIODEV_CONFIG_OUTPUT_LOW,
		GPIODEV_CONFIG_INVALID,
		GPIODEV_FLAG_CONFIGURABLE | GPIODEV_FLAG_LOWLEVELACCESS,
	},
	{
		18,
		"gps_standby",
		GPIODEV_CONFIG_OUTPUT_LOW,
		GPIODEV_CONFIG_INVALID,
		GPIODEV_FLAG_CONFIGURABLE | GPIODEV_FLAG_LOWLEVELACCESS,
	},
	{
		20,
		"gps_interrupt",
		GPIODEV_CONFIG_INPUT | GPIODEV_CONFIG_INT_REDG,
		GPIODEV_CONFIG_INVALID,
		GPIODEV_FLAG_CONFIGURABLE | GPIODEV_FLAG_LOWLEVELACCESS,
	},
	{
		GPIO_DEVICE_UNUSED,
	},
};

static struct gpio_device_platform_data gpio_device_data = {
	.name = "hpm1200l-gpiodev",
	.info = gpio_devs,
};

static struct platform_device hpm1200l_gpiodev_device = {
	.name = GPIO_DEVICE_DEV_NAME,
	.id = 0,
	.dev = {
		.platform_data = &gpio_device_data,
	},
};

static int __init hpm1200l_init_gpiodev(void)
{
	int i;

	for (i = 0; i < GPIO_DEVICE_SIZE; i++) {
		if (gpio_devs[i].pin_nr == GPIO_DEVICE_UNUSED)
			break;
	}
	gpio_device_data.info_count = i;

	return platform_device_register(&hpm1200l_gpiodev_device);
}
device_initcall(hpm1200l_init_gpiodev);
#endif

void __init hpm1200l_gpio_mapping_init(void)
{
	printk(KERN_INFO "GPIO Mapping unused!\n");
}
