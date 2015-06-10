/*
 * linux/arch/arm/mach-omap2/board-hpm1200l-sensors.c
 *
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/sfh7743.h>
#include <linux/bu52014hfv.h>
#include <linux/lis331dlh.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/vib-gpio.h>

#include <mach/mux.h>
#include <mach/gpio.h>
#include <mach/keypad.h>

#define PROX_INT_GPIO		17
#define VIBRATOR_GPIO		24

static struct regulator *hpm1200l_vibrator_regulator;
static int hpm1200l_vibrator_initialization(void)
{
	struct regulator *reg;
	reg = regulator_get(NULL, "vvib");
	if (IS_ERR(reg))
		return PTR_ERR(reg);
	hpm1200l_vibrator_regulator = reg;
	return 0;
}

static void hpm1200l_vibrator_exit(void)
{
	regulator_put(hpm1200l_vibrator_regulator);
}

static int hpm1200l_vibrator_power_on(void)
{
	regulator_set_voltage(hpm1200l_vibrator_regulator, 3000000, 3000000);
	return regulator_enable(hpm1200l_vibrator_regulator);
}

static int hpm1200l_vibrator_power_off(void)
{
	if (hpm1200l_vibrator_regulator)
		return regulator_disable(hpm1200l_vibrator_regulator);
	return 0;
}

static struct vib_gpio_platform_data hpm1200l_vib_gpio_data = {
	.gpio = VIBRATOR_GPIO,
	.max_timeout = 15000,
	.active_low = 0,
	.initial_vibrate = 0,

	.init = hpm1200l_vibrator_initialization,
	.exit = hpm1200l_vibrator_exit,
	.power_on = hpm1200l_vibrator_power_on,
	.power_off = hpm1200l_vibrator_power_off,
};

static struct platform_device hpm1200l_vib_gpio = {
	.name           = "vib-gpio",
	.id             = -1,
	.dev            = {
		.platform_data  = &hpm1200l_vib_gpio_data,
	},
};

static struct sfh7743_platform_data hpm1200l_sfh7743_data = {
	.gpio = PROX_INT_GPIO,
};

static void __init hpm1200l_sfh7743_init(void)
{
	gpio_request(PROX_INT_GPIO, "sfh7743 proximity int");
	gpio_direction_input(PROX_INT_GPIO);
	omap_cfg_reg(W3G_GPIO_17);
}

struct platform_device sfh7743_platform_device = {
	.name = "sfh7743",
	.id = -1,
	.dev = {
		.platform_data = &hpm1200l_sfh7743_data,
	},
};

static void hpm1200l_vibrator_init(void)
{
	gpio_request(VIBRATOR_GPIO, "vibrator");
	gpio_direction_output(VIBRATOR_GPIO, 0);
	omap_cfg_reg(W3G_GPIO_24);
}

static struct platform_device *hpm1200l_sensors[] __initdata = {
	&sfh7743_platform_device,
	&hpm1200l_vib_gpio,
};

void __init hpm1200l_sensors_init(void)
{
	hpm1200l_sfh7743_init();
	hpm1200l_vibrator_init();
	platform_add_devices(hpm1200l_sensors, ARRAY_SIZE(hpm1200l_sensors));
}
