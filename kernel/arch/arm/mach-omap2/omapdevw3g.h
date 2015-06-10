/*
 * TI OCP devices present on OMAPW3G
 *
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Copyright (C) 2007-2008 Nokia Corporation
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef ARCH_ARM_MACH_OMAP2_OMAPDEV_W3G_H
#define ARCH_ARM_MACH_OMAP2_OMAPDEV_W3G_H

#include <linux/serial_8250.h>

#include <mach/cpu.h>
#include <mach/omapdev.h>
/* CORE */

static struct omapdev l4_per_3xxx_omapdev = {
	.name		= "l4_per_omapdev",
	.pwrdm		= { .name = "core_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

static struct omapdev l4_emu_3xxx_omapdev = {
	.name		= "l4_emu_omapdev",
	.pwrdm		= { .name = "core_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* XXX ICR is present on 2430 & 3430, but is in WKUP on 2430 */
static struct omapdev icr_3xxx_omapdev = {
	.name		= "icr_omapdev",
	.pwrdm		= { .name = "core_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* IVA2 interrupt controller - XXX 2430 also ? */
static struct omapdev wugen_3xxx_omapdev = {
	.name		= "wugen_omapdev",
	.pwrdm		= { .name = "core_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* XXX temperature sensor (what is the second one for ?) */

/* XXX CWT is on 2430 at least, what about 2420? */

static struct omapdev mad2d_3xxx_omapdev = {
	.name		= "mad2d_omapdev",
	.pwrdm		= { .name = "core_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* CONTROL/SCM moved into CORE pwrdm on 3430 */
static struct omapdev control_3xxx_omapdev = {
	.name		= "control_omapdev",
	.pwrdm		= { .name = "core_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* L4-WAKEUP */

static struct omapdev tap_3xxx_omapdev = {
	.name		= "tap_omapdev",
	.pwrdm		= { .name = "wkup_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};


/* L4-EMU */

static struct omapdev mpuemu_3xxx_omapdev = {
	.name		= "mpuemu_omapdev",
	.pwrdm		= { .name = "emu_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

static struct omapdev tpiu_3xxx_omapdev = {
	.name		= "tpiu_omapdev",
	.pwrdm		= { .name = "emu_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

static struct omapdev etb_3xxx_omapdev = {
	.name		= "etb_omapdev",
	.pwrdm		= { .name = "emu_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

static struct omapdev dapctl_3xxx_omapdev = {
	.name		= "dapctl_omapdev",
	.pwrdm		= { .name = "emu_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

static struct omapdev dap_3xxx_omapdev = {
	.name		= "dap_omapdev",
	.pwrdm		= { .name = "emu_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* USBHOST */

static struct omapdev usbhost_3xxx_omapdev = {
	.name		= "usbhost_omapdev",
	.pwrdm		= { .name = "usbhost_pwrdm" },
	.pdev_name	= "ehci-omap",
	.pdev_id	= 0,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_GE_OMAP3430ES2),
};

static struct omapdev usbotg_3xxx_omapdev = {
	.name		= "usbotg_omapdev",
	.pwrdm		= { .name = "usbhost_pwrdm" },
	.pdev_name	= "musb_hdrc",
	.pdev_id	= -1,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_GE_OMAP3430ES2),
};

static struct omapdev usbtll_3xxx_omapdev = {
	.name		= "usbtll_omapdev",
	.pwrdm		= { .name = "usbhost_pwrdm" },
	.pdev_name	= "ehci-omap",
	.pdev_id	= 0,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* DSS */

static struct omapdev dispc_3xxx_omapdev = {
	.name		= "dispc_omapdev",
	.pwrdm		= { .name = "dss_pwrdm" },
	.pdev_name	= "omapfb",
	.pdev_id	= -1,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

static struct omapdev rfbi_3xxx_omapdev = {
	.name		= "rfbi_omapdev",
	.pwrdm		= { .name = "dss_pwrdm" },
	.pdev_name	= "omapfb",
	.pdev_id	= -1,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* CAM */


static struct omapdev isp_3xxx_omapdev = {
	.name		= "isp_omapdev",
	.pwrdm		= { .name = "cam_pwrdm" },
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

#endif	   /* CONFIG_ARCH_OMAP3 */

#endif
