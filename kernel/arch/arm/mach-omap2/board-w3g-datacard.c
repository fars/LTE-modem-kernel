/*
 * linux/arch/arm/mach-omap2/board-w3g-datacard.c
 *
 * Copyright (C) 2005 Nokia Corporation
 * Author: Paul Mundt <paul.mundt@nokia.com>
 *
 * Modified from mach-omap/omap1/board-generic.c
 *
 * Copyright (C) 2009 Motorola, Inc.
 * Modified from mach-omap/omap1/board-generic.c
 *
 * Code for Motorola Wrigley 3G based data card products.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Revision History:
 *
 * Date         Author   Comment
 * -----------  -------  -----------------------------------
 * 24-Aug-2009  Motorola Support for W3G architecture
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <linux/memory.h>

#include <mach/gpio.h>
#include <mach/mux.h>
#include <mach/usb.h>
#include <mach/board.h>
#include <mach/common.h>
#include <mach/omap24xx.h>

/* #include "board-genericphone-timing.h" */

#include <linux/spi/spi.h>
#include <mach/mcspi.h>

#include <linux/usb/android.h>
#include "omap3-opp.h"

#define DIE_ID_REG_BASE                 (OMAPW3G_SCM_BASE + 0x300)
#define DIE_ID_REG_OFFSET               0x134
#define MAX_USB_SERIAL_NUM              17
#define BPWAKE_STROBE_GPIO	        157   /* TODO - Investigate this value and if it is vaild for a data card. */

/* TODO - Remove this once Mux Framework is supported for W3G */
#ifndef CONFIG_OMAP_MUX
#define write_reg_bits(reg, mask, bits) (*(reg) = ( *(reg) & ~(mask)) | (bits));
#define CONTROL_PADCONF_GPIO_9   IO_ADDRESS(OMAPW3G_SCM_BASE + 0x0150)
#endif

static char device_serial[MAX_USB_SERIAL_NUM];

#ifdef CONFIG_MACH_W3G_LTE_DATACARD
char *bp_model = "LTE";
#endif 

#define PRCM_INTERRUPT_MASK (1<<11);

static struct omap_opp w3g_datacard_mpu_rate_table[] = {
	{0, 0, 0},
	/*OPP1 */
	{S125M, VDD1_OPP1, 0x20},
	/*OPP2 */
	{S250M, VDD1_OPP2, 0x27},
	/*OPP3 */
	{S500M, VDD1_OPP3, 0x32},
	/*OPP4 */
	{S550M, VDD1_OPP4, 0x38},
	/*OPP5 */
	{S600M, VDD1_OPP5, 0x3E},
};

static struct omap_opp w3g_datacard_l3_rate_table[] = {
	{0, 0, 0},
	/*OPP1 */
	{0, VDD2_OPP1, 0x20},
	/*OPP2 */
	{S83M, VDD2_OPP2, 0x27},
	/*OPP3 */
	{S166M, VDD2_OPP3, 0x2E},
};

static struct omap_opp w3g_datacard_dsp_rate_table[] = {
	{0, 0, 0},
	/*OPP1 */
	{S90M, VDD1_OPP1, 0x20},
	/*OPP2 */
	{S180M, VDD1_OPP2, 0x27},
	/*OPP3 */
	{S360M, VDD1_OPP3, 0x32},
	/*OPP4 */
	{S400M, VDD1_OPP4, 0x38},
	/*OPP5 */
	{S430M, VDD1_OPP5, 0x3E},
};

void __init w3g_datacard_spi_init(void);

static void __init w3g_datacard_init_irq(void)
{
	omap2_init_common_hw(NULL,
			     w3g_datacard_mpu_rate_table,
			     w3g_datacard_dsp_rate_table,
			     w3g_datacard_l3_rate_table);
	omap_init_irq();
	omap_gpio_init();
}

static __init void w3g_datacard_gpio_iomux_init(void)
{
	/* configure omapw3g IOMUX pad registers to initial state */
	/* mot_scm_init();  - Initialize the SCM along with the msuspend registers.  */
	/* It does not appear that the msuspend registers are initialized on K29. */
	/* The IOMUX is initialized in the bootloader. */
#ifdef CONFIG_OMAP_MUX
	omap_cfg_reg(W3G_GPIO_9);
#else
	/* TODO - Remove this once Mux Framework is supported for W3G */
	write_reg_bits((volatile unsigned int *)CONTROL_PADCONF_GPIO_9, (0x0000FFFF), (0x00000118));
#endif
}

static struct android_usb_platform_data andusb_plat = {
	.vendor_id = 0x22b8,
#ifndef CONFIG_MACH_W3G_DATACARD
	.product_id = 0x41DA,
	.adb_product_id = 0x41DA,
#elif defined (CONFIG_USB_GADGET_DUAL_ECM)
	.product_id     = 0x4268,
	.adb_product_id = 0x4268,
#elif defined (CONFIG_USB_GADGET_FIVE_ECM)
	.product_id     = 0x4267,
	.adb_product_id = 0x4267,
#elif !defined (CONFIG_USB_GADGET_SINGLE_ECM) && \
    !defined (CONFIG_USB_GADGET_DUAL_ACM) && \
    !defined (CONFIG_USB_GADGET_SINGLE_ACM) && \
    defined (CONFIG_USB_GADGET_NETWORK_INTERFACE)
    .product_id = 0x4262,
    .adb_product_id = 0x4262,
#else
	.product_id     = 0x4264,
	.adb_product_id = 0x4264,
#endif
	.product_name = "Motorola LTE Datacard",
	.manufacturer_name = "Motorola",
	.serial_number = device_serial,
};

static struct platform_device androidusb_device = {
	.name = "android_usb",
	.id = -1,
	.dev = {
		.platform_data = &andusb_plat,
		},
};

static void __init w3g_datacard_gadget_init(void)
{
	unsigned int val[2];
	unsigned int reg;

	reg = DIE_ID_REG_BASE + DIE_ID_REG_OFFSET;
	val[0] = omap_readl(reg);
	val[1] = omap_readl(reg + 4);

	snprintf(device_serial, MAX_USB_SERIAL_NUM, "%08x%08x", val[1], val[0]);
	platform_device_register(&androidusb_device);
	/*platform_driver_register(&cpcap_usb_connected_driver); */
}

extern void __init w3g_datacard_flash_init(void);

/* Need to verify the need for this. */
static void w3g_mbox_ipc_init(void)
{
	omap_cfg_reg(W3G_GPMC_AD16);
	omap_cfg_reg(W3G_GPIO_17);
}

static struct omap_uart_config w3g_datacard_uart_config __initdata = {
	.enabled_uarts = 0,
};

static struct omap_board_config_kernel w3g_datacard_config[] __initdata = {
	{OMAP_TAG_UART, &w3g_datacard_uart_config},
};



static void __init w3g_datacard_serial_init(void)
{
#if defined(CONFIG_SERIAL_OMAP_CONSOLE)
	u32 val;

	omap_cfg_reg(W3G_UART3_TX_IRTX);
	omap_cfg_reg(W3G_UART3_RX_IRRX);
	omap_cfg_reg(W3G_UART3_RTS_SD);
	omap_cfg_reg(W3G_UART3_CTS_RCTX);

	/* Enable UART3 for console */
	w3g_datacard_uart_config.enabled_uarts |= (1<<2);

#endif

	/* Initialize the platform drivers */
	omap_serial_init(0,0);
}

static void __init w3g_datacard_init(void)
{
	omap_board_config = w3g_datacard_config;
	omap_board_config_size = ARRAY_SIZE(w3g_datacard_config);
	w3g_datacard_gpio_iomux_init();
	w3g_mbox_ipc_init();
	w3g_datacard_spi_init();

	w3g_datacard_serial_init();
	usb_musb_init();
	omap_register_i2c_bus(1, 100, NULL, 0);
	omap_register_i2c_bus(2, 100, NULL, 0);
	/* omapw3gphone_memory_timing_init(); - Memory is configured in the bootloader. */
	w3g_datacard_gadget_init();
	w3g_datacard_flash_init();
}

static void __init w3g_datacard_map_io(void)
{
	omap2_set_globals_w3g();	/* should be 242x, 243x, or 343x */
	omap2_map_common_io();
}

MACHINE_START(W3G_DATACARD, "Wrigley 3G Datacard")
    .phys_io = 0x48000000,
	.io_pg_offst =
    ((0xd8000000) >> 18) & 0xfffc,
	.boot_params = PHYS_OFFSET,
	.map_io = w3g_datacard_map_io,
	.init_irq = w3g_datacard_init_irq,
	.init_machine = w3g_datacard_init,
	.timer = &omap_timer,
MACHINE_END
