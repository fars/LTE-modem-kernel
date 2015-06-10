/*
 * linuxarch/arm/mach-omap2/board-hpm1200l-panel.c
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
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/omapfb.h>

#include <mach/display.h>
#include <mach/gpio.h>
#include <mach/mux.h>
#include <mach/resource.h>

#define DISPLAY_RESET_GPIO	13
#define RFBI_CHANNEL_0	0
#define RFBI_CHANNEL0_DATALINES 16

struct regulator *regulator_analog;

static int hpm1200l_panel_enable(struct omap_dss_device *dssdev)
{
/*
	if (!regulator_analog) {
		regulator_analog = regulator_get(NULL, "vhvio");
		if (IS_ERR(regulator_analog)) {
			printk(KERN_ERR
				"failed to get display's analog regulator\n");
			return PTR_ERR(regulator_analog);
		}
	}

	regulator_enable(display_regulator_analog);
	regulator_enable(display_regulator_logic);
*/

	gpio_direction_output(DISPLAY_RESET_GPIO, 1);
	gpio_set_value(DISPLAY_RESET_GPIO, 1);
	msleep(2);
	gpio_set_value(DISPLAY_RESET_GPIO, 0);
	msleep(10);
	gpio_set_value(DISPLAY_RESET_GPIO, 1);
	msleep(65);
	return 0;
}

static void hpm1200l_panel_disable(struct omap_dss_device *dssdev)
{
	msleep(35);
	gpio_direction_output(DISPLAY_RESET_GPIO, 1);
	gpio_set_value(DISPLAY_RESET_GPIO, 0);
	msleep(1);
	regulator_disable(regulator_analog);
}

static struct omapfb_platform_data hpm1200l_fb_data = {
	.mem_desc = {
		.region_cnt = 1,
		.region = {
			{
				.format = OMAPFB_COLOR_ARGB32,
				.format_used = 1,
			},
		},
	},
};

static struct omap_dss_device hpm1200l_lcd_device = {
	.type = OMAP_DISPLAY_TYPE_DBI,
	.name = "lcd",
	.driver_name = "hpm1200l-panel",
	.phy.rfbi.channel = RFBI_CHANNEL_0,
	.phy.rfbi.data_lines = RFBI_CHANNEL0_DATALINES,
	.reset_gpio = DISPLAY_RESET_GPIO,
	.platform_enable = hpm1200l_panel_enable,
	.platform_disable = hpm1200l_panel_disable,
};

static struct omap_dss_device *hpm1200l_dss_devices[] = {
	&hpm1200l_lcd_device,
};

static struct omap_dss_board_info hpm1200l_dss_data = {
	.num_devices = ARRAY_SIZE(hpm1200l_dss_devices),
	.devices = hpm1200l_dss_devices,
	.default_device = &hpm1200l_lcd_device,
};

struct platform_device hpm1200l_dss_device = {
	.name = "omapdss",
	.id = -1,
	.dev = {
		.platform_data = &hpm1200l_dss_data,
	},
};

void __init hpm1200l_panel_init(void)
{
	int ret;

	/* RFBI_WE */
	omap_cfg_reg(W3G_DSS_HSYNC);
	/* RFBI_A0 */
	omap_cfg_reg(W3G_DSS_ACBIAS);
	/* RFBI_CS0 */
	omap_cfg_reg(W3G_DSS_VSYNC);
	/* RFBI_RE */
	omap_cfg_reg(W3G_DSS_PCLK);
	/* RFBI_DATA<0..15> */
	omap_cfg_reg(W3G_DSS_DATA0);
	omap_cfg_reg(W3G_DSS_DATA1);
	omap_cfg_reg(W3G_DSS_DATA2);
	omap_cfg_reg(W3G_DSS_DATA3);
	omap_cfg_reg(W3G_DSS_DATA4);
	omap_cfg_reg(W3G_DSS_DATA5);
	omap_cfg_reg(W3G_DSS_DATA6);
	omap_cfg_reg(W3G_DSS_DATA7);
	omap_cfg_reg(W3G_DSS_DATA8);
	omap_cfg_reg(W3G_DSS_DATA9);
	omap_cfg_reg(W3G_DSS_DATA10);
	omap_cfg_reg(W3G_DSS_DATA11);
	omap_cfg_reg(W3G_DSS_DATA12);
	omap_cfg_reg(W3G_DSS_DATA13);
	omap_cfg_reg(W3G_DSS_DATA14);
	omap_cfg_reg(W3G_DSS_DATA15);
	/* DISPC_RESET */
	omap_cfg_reg(W3G_GPIO_13);

	omapfb_set_platform_data(&hpm1200l_fb_data);

	ret = gpio_request(DISPLAY_RESET_GPIO, "display reset");
	if (ret) {
		printk(KERN_ERR "failed to get display reset gpio\n");
		goto error;
	}
	platform_device_register(&hpm1200l_dss_device);
	return;
error:
	gpio_free(DISPLAY_RESET_GPIO);
}
