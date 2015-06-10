/*
 * OMAP34XX powerdomain definitions
 *
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Written by Paul Walmsley
 * Debugging and integration fixes by Jouni Högander
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARCH_ARM_MACH_OMAP2_POWERDOMAINSW3G
#define ARCH_ARM_MACH_OMAP2_POWERDOMAINSW3G

/*
 * N.B. If powerdomains are added or removed from this file, update
 * the array in mach-omap2/powerdomains.h.
 */

#include <mach/powerdomain.h>

#include "prcm-common.h"
#include "prm.h"
#include "prm-regbits-w3g.h"
#include "cm.h"
#include "cm-regbits-w3g.h"

/*
 * W3G-specific powerdomains, dependencies
 */

#ifdef CONFIG_ARCH_OMAPW3G

/* W3G: MPU and AUD only support RET and ON.  Further
 *      support could be expanded to save off context
 *      to reach OFF.
 */
#define PWRSTS_RET_ON	((1 << PWRDM_POWER_RET) | \
				 (1 << PWRDM_POWER_ON))


/*
 * W3G PM_WKDEP_MPU: APP, AUD, CORE, DSP, DSS, WCDMA, WKUP
 */
static struct pwrdm_dep mpu_wkdeps[] = {
	{
		.pwrdm_name = "app_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "aud_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "core_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "dsp_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "dss_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "wcdma_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "wkup_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{ NULL },
};

/* W3G PM_WKDEP_AUD: APP, CORE, DSP, MPU, WKUP */
static struct pwrdm_dep aud_wkdeps[] = {
	{
		.pwrdm_name = "app_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "core_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "dsp_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "mpu_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{
		.pwrdm_name = "wkup_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G)
	},
	{ NULL },
};


/*
 * Powerdomains
 */

static struct powerdomain dsp_pwrdm = {
	.name		  = "dsp_pwrdm",
	.prcm_offs	  = OMAPW3G_DSP_MOD,
	.omap_chip	  = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
	.dep_bit	  = OMAPW3G_WKDEP_EN_DSP_SHIFT,
};

static struct powerdomain mpu_pwrdm = {
	.name		  = "mpu_pwrdm",
	.prcm_offs	  = OMAPW3G_MPU_MOD,
	.omap_chip	  = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
	.dep_bit	  = OMAPW3G_WKDEP_EN_MPU_SHIFT,
	.wkdep_srcs	  = mpu_wkdeps,
	.pwrsts		  = PWRSTS_RET_ON,
	.pwrsts_logic_ret = PWRSTS_OFF_RET,
	.banks		  = 1,
	.pwrsts_mem_ret	  = {
		[0] = PWRSTS_OFF_RET,
	},
	.pwrsts_mem_on	  = {
		[0] = PWRSTS_OFF_ON,
	},
};

/* No wkdeps or sleepdeps apparently */
static struct powerdomain core_pwrdm = {
	.name		  = "core_pwrdm",
	.prcm_offs	  = OMAPW3G_CORE_MOD,
	.omap_chip	  = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
	.dep_bit	  = OMAPW3G_WKDEP_EN_CORE_SHIFT,
};

/* Another case of bit name collisions between several registers: EN_DSS */
static struct powerdomain dss_pwrdm = {
	.name		  = "dss_pwrdm",
	.omap_chip	  = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
	.prcm_offs	  = OMAPW3G_DSS_MOD,
	.dep_bit	  = OMAPW3G_WKDEP_EN_DSS_SHIFT,
};

static struct powerdomain cam_pwrdm = {
	.name		  = "cam_pwrdm",
	.omap_chip	  = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
	.prcm_offs	  = OMAPW3G_CAM_MOD,
};

static struct powerdomain app_pwrdm = {
	.name		  = "app_pwrdm",
	.prcm_offs	  = OMAPW3G_APP_MOD,
	.omap_chip	  = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
	.dep_bit	  = OMAPW3G_WKDEP_EN_APP_SHIFT,
};

static struct powerdomain aud_pwrdm = {
	.name		  = "aud_pwrdm",
	.prcm_offs	  = OMAPW3G_AUD_MOD,
	.omap_chip	  = OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
	.dep_bit	  = OMAPW3G_WKDEP_EN_AUD_SHIFT,
};

static struct powerdomain emu_pwrdm = {
	.name		= "emu_pwrdm",
	.prcm_offs	= OMAPW3G_EMU_MOD,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
};

static struct powerdomain dpll1_pwrdm = {
	.name		= "dpll1_pwrdm",
	.prcm_offs	= OMAPW3G_MPU_MOD,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
};

static struct powerdomain dpll2_pwrdm = {
	.name		= "dpll2_pwrdm",
	.prcm_offs	= OMAPW3G_DSP_MOD,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
};

static struct powerdomain dpll3_pwrdm = {
	.name		= "dpll3_pwrdm",
	.prcm_offs	= OMAPW3G_CCR_MOD,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
};

static struct powerdomain dpll4_pwrdm = {
	.name		= "dpll4_pwrdm",
	.prcm_offs	= OMAPW3G_CCR_MOD,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAPW3G),
};

#endif    /* CONFIG_ARCH_OMAPW3G */


#endif
