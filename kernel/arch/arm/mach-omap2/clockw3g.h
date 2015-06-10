/*
 * OMAPW3G clock framework
 *
 * Copyright (C) 2009-2010 Motorola, Inc.
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Written by Paul Walmsley
 * With many device clock fixes by Kevin Hilman and Jouni Högander
 * DPLL bypass clock support added by Roman Tereshonkov
 *
 */

/*
 * Virtual clocks are introduced as convenient tools.
 * They are sources for other clocks and not supposed
 * to be requested from drivers directly.
 */

#ifndef __ARCH_ARM_MACH_OMAP2_CLOCKW3G_H
#define __ARCH_ARM_MACH_OMAP2_CLOCKW3G_H

#include <mach/control.h>
#include <mach/omap-pm.h>
#include <mach/prcm_34xx.h>

#include "clock.h"
#include "cm.h"
#include "cm-regbits-w3g.h"
#include "prm.h"
#include "prm-regbits-w3g.h"

static void omap3_dpll_recalc(struct clk *clk, unsigned long parent_rate,
			      u8 rate_storage);
static void omap3_clkoutx2_recalc(struct clk *clk, unsigned long parent_rate,
			      u8 rate_storage);
static void omap3_dpll_allow_idle(struct clk *clk);
static void omap3_dpll_deny_idle(struct clk *clk);
static u32 omap3_dpll_autoidle_read(struct clk *clk);
static int omap3_noncore_dpll_enable(struct clk *clk);
static void omap3_noncore_dpll_disable(struct clk *clk);
static int omap3_noncore_dpll_set_rate(struct clk *clk, unsigned long rate);
static int omap3_core_dpll_m2_set_rate(struct clk *clk, unsigned long rate);

/* W3G Override Functions */
int omapw3g_clk_enable(struct clk *clk);
void omapw3g_clk_disable(struct clk *clk);
int omapw3g_clk_enable_mute(struct clk *clk);
void omapw3g_clk_disable_mute(struct clk *clk);
int omapw3g_clk_mcbsp1_enable(struct clk *clk);
void omapw3g_clk_mcbsp1_disable(struct clk *clk);
int omapw3g_clk_set_rate(struct clk *clk, unsigned long rate);
int omapw3g_clk_set_parent(struct clk *clk, struct clk *new_parent);

/* Maximum DPLL multiplier, divider values for OMAP3 */
#define OMAP3_MAX_DPLL_MULT		2048
#define OMAP3_MAX_DPLL_DIV		128

/*
 * DPLL1 supplies clock to the MPU.
 * DPLL2 supplies clock to the DSP.
 * DPLL3 supplies CORE domain clocks.
 * DPLL4 supplies peripheral clocks.
 */

/* Forward declarations for DPLL bypass clocks */
static struct clk dpll1_fck;

/* CM_CLKEN_PLL*.EN* bit values - not all are available for every DPLL */
#define DPLL_LOW_POWER_STOP		0x1
#define DPLL_LOW_POWER_BYPASS		0x5
#define DPLL_LOCKED			0x7

/* PRM CLOCKS */

/* According to timer32k.c, this is a 32768Hz clock, not a 32000Hz clock. */
static struct clk omap_32k_fck = {
	.name		= "omap_32k_fck",
	.rate		= 32768,
	.flags		= CLOCK_IN_OMAP343X | ALWAYS_ENABLED,
	.clkdm		= { .name = "prm_clkdm" },
};

static const struct clksel_rate div2_rates[] = {
	{ .div = 1, .val = 1, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 2, .val = 2, .flags = RATE_IN_343X },
	{ .div = 0 }
};


/* Latency: this clock is only enabled after PRM_CLKSETUP.SETUP_TIME */
/* Feeds DPLLs - divided first by PRM_CLKSRC_CTRL.SYSCLKDIV? */
static struct clk sys_ck = {
	.name		= "sys_ck",
	.rate           = 26000000,
	.prcm_mod	= OMAPW3G_GR_MOD | CLK_REG_IN_PRM,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= OMAP3_PRM_CLKSRC_CTRL_OFFSET,
	.clksel_mask	= OMAP_SYSCLKDIV_MASK,
	.flags		= CLOCK_IN_OMAP343X | ALWAYS_ENABLED,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

/*
 * Optional external clock input for some McBSPs
 * Apparently this is not really in prm_clkdm, but rather is fed into
 * both CORE and PER separately.
 */
static struct clk mcbsp_clks = {
	.name		= "mcbsp_clks",
	.flags		= CLOCK_IN_OMAP343X | ALWAYS_ENABLED,
	.clkdm		= { .name = "prm_clkdm" },
};

/* PRM EXTERNAL CLOCK OUTPUT */

static struct clk sys_clkout1 = {
	.name		= "sys_clkout1",
	.parent		= &sys_ck,
	.prcm_mod	= OMAPW3G_CCR_MOD | CLK_REG_IN_PRM,
	.enable_reg	= OMAP3_PRM_CLKOUT_CTRL_OFFSET,
	.enable_bit	= OMAPW3G_CLKOUT_EN_SHIFT,
	.flags		= CLOCK_IN_OMAP343X,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};

/* DPLLS */

/* CM CLOCKS */

static const struct clksel_rate div16_dpll_rates[] = {
	{ .div = 1, .val = 1, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 2, .val = 2, .flags = RATE_IN_343X },
	{ .div = 3, .val = 3, .flags = RATE_IN_343X },
	{ .div = 4, .val = 4, .flags = RATE_IN_343X },
	{ .div = 5, .val = 5, .flags = RATE_IN_343X },
	{ .div = 6, .val = 6, .flags = RATE_IN_343X },
	{ .div = 7, .val = 7, .flags = RATE_IN_343X },
	{ .div = 8, .val = 8, .flags = RATE_IN_343X },
	{ .div = 9, .val = 9, .flags = RATE_IN_343X },
	{ .div = 10, .val = 10, .flags = RATE_IN_343X },
	{ .div = 11, .val = 11, .flags = RATE_IN_343X },
	{ .div = 12, .val = 12, .flags = RATE_IN_343X },
	{ .div = 13, .val = 13, .flags = RATE_IN_343X },
	{ .div = 14, .val = 14, .flags = RATE_IN_343X },
	{ .div = 15, .val = 15, .flags = RATE_IN_343X },
	{ .div = 16, .val = 16, .flags = RATE_IN_343X },
	{ .div = 0 }
};

/* DPLL1 */
/* MPU clock source */
/* Type: DPLL */
static struct dpll_data dpll1_dd = {
	.mult_div1_reg	= OMAP3430_CM_CLKSEL1_PLL,
	.mult_mask	= OMAPW3G_MPU_DPLL_MULT_MASK,
	.div1_mask	= OMAPW3G_MPU_DPLL_DIV_MASK,
	.freqsel_mask	= OMAPW3G_MPU_DPLL_FREQSEL_MASK,
	.control_reg	= OMAP3430_CM_CLKEN_PLL,
	.enable_mask	= OMAPW3G_EN_MPU_DPLL_MASK,
	.modes		= (1 << DPLL_LOW_POWER_BYPASS) | (1 << DPLL_LOCKED),
	.auto_recal_bit	= OMAPW3G_EN_MPU_DPLL_DRIFTGUARD_SHIFT,
	.recal_en_bit	= OMAPW3G_MPU_DPLL_RECAL_EN_SHIFT,
	.recal_st_bit	= OMAPW3G_MPU_DPLL_ST_SHIFT,
	.autoidle_reg	= OMAP3430_CM_AUTOIDLE_PLL,
	.autoidle_mask	= OMAPW3G_AUTO_MPU_DPLL_MASK,
	.idlest_reg	= OMAP3430_CM_IDLEST_PLL,
	.idlest_mask	= OMAPW3G_ST_MPU_CLK_MASK,
	.bypass_clk	= &dpll1_fck,
	.max_multiplier = OMAP3_MAX_DPLL_MULT,
	.min_divider	= 1,
	.max_divider	= OMAP3_MAX_DPLL_DIV,
	.rate_tolerance = DEFAULT_DPLL_RATE_TOLERANCE
};

static struct clk dpll1_ck = {
	.name		= "dpll1_ck",
	.parent		= &sys_ck,
	.prcm_mod	= OMAPW3G_MPU_MOD,
	.dpll_data	= &dpll1_dd,
	.flags		= CLOCK_IN_OMAP343X | ALWAYS_ENABLED | RECALC_ON_ENABLE,
	.round_rate	= &omap2_dpll_round_rate,
	.set_rate	= &omap3_noncore_dpll_set_rate,
	.clkdm		= { .name = "dpll1_clkdm" },
	.recalc		= &omap3_dpll_recalc,
};

/*
 * This virtual clock provides the CLKOUTX2 output from the DPLL if the
 * DPLL isn't bypassed.
 */
static struct clk dpll1_x2_ck = {
	.name		= "dpll1_x2_ck",
	.parent		= &dpll1_ck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "dpll1_clkdm" },
	.recalc		= &omap3_clkoutx2_recalc,
};

/* On DPLL1, unlike other DPLLs, the divider is downstream from CLKOUTX2 */
static const struct clksel div16_dpll1_x2m2_clksel[] = {
	{ .parent = &dpll1_x2_ck, .rates = div16_dpll_rates },
	{ .parent = NULL }
};

/*
 * Does not exist in the TRM - needed to separate the M2 divider from
 * bypass selection in mpu_ck
 */
static struct clk dpll1_x2m2_ck = {
	.name		= "dpll1_x2m2_ck",
	.parent		= &dpll1_x2_ck,
	.prcm_mod	= OMAPW3G_MPU_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= OMAP3430_CM_CLKSEL2_PLL,
	.clksel_mask	= OMAPW3G_MPU_DPLL_CLKOUT_DIV_MASK,
	.clksel		= div16_dpll1_x2m2_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "dpll1_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

/* DPLL2 */


/*
 * DPLL3
 * Source clock for all interfaces and for some device fclks
 * REVISIT: Also supports fast relock bypass - not included below
 */
static struct dpll_data dpll3_dd = {
	.mult_div1_reg	= CM_CLKSEL1,
	.mult_mask	= OMAPW3G_CORE_DPLL_MULT_MASK,
	.div1_mask	= OMAPW3G_CORE_DPLL_DIV_MASK,
	.freqsel_mask	= OMAPW3G_CORE_DPLL_FREQSEL_MASK,
	.control_reg	= CM_CLKEN,
	.enable_mask	= OMAPW3G_EN_CORE_DPLL_MASK,
	.auto_recal_bit	= OMAPW3G_EN_CORE_DPLL_DRIFTGUARD_SHIFT,
	.recal_en_bit	= OMAPW3G_CORE_DPLL_RECAL_EN_SHIFT,
	.recal_st_bit	= OMAPW3G_CORE_DPLL_ST_SHIFT,
	.autoidle_reg	= CM_AUTOIDLE,
	.autoidle_mask	= OMAPW3G_AUTO_CORE_DPLL_MASK,
	.idlest_reg	= CM_IDLEST,
	.idlest_mask	= OMAPW3G_ST_CORE_CLK_MASK,
	.bypass_clk	= &sys_ck,
	.max_multiplier = OMAP3_MAX_DPLL_MULT,
	.min_divider	= 1,
	.max_divider	= OMAP3_MAX_DPLL_DIV,
	.rate_tolerance = DEFAULT_DPLL_RATE_TOLERANCE
};

static struct clk dpll3_ck = {
	.name		= "dpll3_ck",
	.parent		= &sys_ck,
	.prcm_mod	= OMAPW3G_CCR_MOD,
	.dpll_data	= &dpll3_dd,
	.flags		= CLOCK_IN_OMAP343X | ALWAYS_ENABLED | RECALC_ON_ENABLE,
	.round_rate	= &omap2_dpll_round_rate,
	.clkdm		= { .name = "dpll3_clkdm" },
	.recalc		= &omap3_dpll_recalc,
};

static const struct clksel_rate div31_dpll3_rates[] = {
	{ .div = 1, .val = 1, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 2, .val = 2, .flags = RATE_IN_343X },
	{ .div = 3, .val = 3, .flags = RATE_IN_343X },
	{ .div = 4, .val = 4, .flags = RATE_IN_343X },
	{ .div = 5, .val = 5, .flags = RATE_IN_343X },
	{ .div = 6, .val = 6, .flags = RATE_IN_343X },
	{ .div = 7, .val = 7, .flags = RATE_IN_343X },
	{ .div = 8, .val = 8, .flags = RATE_IN_343X },
	{ .div = 9, .val = 9, .flags = RATE_IN_343X },
	{ .div = 10, .val = 10, .flags = RATE_IN_343X },
	{ .div = 11, .val = 11, .flags = RATE_IN_343X },
	{ .div = 12, .val = 12, .flags = RATE_IN_343X },
	{ .div = 13, .val = 13, .flags = RATE_IN_343X },
	{ .div = 14, .val = 14, .flags = RATE_IN_343X },
	{ .div = 15, .val = 15, .flags = RATE_IN_343X },
	{ .div = 16, .val = 16, .flags = RATE_IN_343X },
	{ .div = 17, .val = 17, .flags = RATE_IN_343X },
	{ .div = 18, .val = 18, .flags = RATE_IN_343X },
	{ .div = 19, .val = 19, .flags = RATE_IN_343X },
	{ .div = 20, .val = 20, .flags = RATE_IN_343X },
	{ .div = 21, .val = 21, .flags = RATE_IN_343X },
	{ .div = 22, .val = 22, .flags = RATE_IN_343X },
	{ .div = 23, .val = 23, .flags = RATE_IN_343X },
	{ .div = 24, .val = 24, .flags = RATE_IN_343X },
	{ .div = 25, .val = 25, .flags = RATE_IN_343X },
	{ .div = 26, .val = 26, .flags = RATE_IN_343X },
	{ .div = 27, .val = 27, .flags = RATE_IN_343X },
	{ .div = 28, .val = 28, .flags = RATE_IN_343X },
	{ .div = 29, .val = 29, .flags = RATE_IN_343X },
	{ .div = 30, .val = 30, .flags = RATE_IN_343X },
	{ .div = 31, .val = 31, .flags = RATE_IN_343X },
	{ .div = 0 },
};

static const struct clksel div31_dpll3m2_clksel[] = {
	{ .parent = &dpll3_ck, .rates = div31_dpll3_rates },
	{ .parent = NULL }
};

/* DPLL3 output M2 - primary control point for CORE speed */
static struct clk dpll3_m2_ck = {
	.name		= "dpll3_m2_ck",
	.parent		= &dpll3_ck,
	.prcm_mod	= OMAPW3G_CCR_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= CM_CLKSEL1,
	.clksel_mask	= OMAPW3G_CORE_DPLL_CLKOUT_DIV_MASK,
	.clksel		= div31_dpll3m2_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "dpll3_clkdm" },
	.round_rate	= &omap2_clksel_round_rate,
	.set_rate	= &omap3_core_dpll_m2_set_rate,
	.recalc		= &omap2_clksel_recalc,
};

static struct clk core_ck = {
	.name		= "core_ck",
	.parent		= &dpll3_m2_ck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk dpll3_m2x2_ck = {
	.name		= "dpll3_m2x2_ck",
	.parent		= &dpll3_m2_ck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "dpll3_clkdm" },
	.recalc		= &omap3_clkoutx2_recalc,
};

static const struct clksel div16_dpll3_clksel[] = {
	{ .parent = &dpll3_ck, .rates = div16_dpll_rates },
	{ .parent = NULL }
};

/* DPLL4 */
/* Supplies 96MHz, 54Mhz TV DAC, DSS fclk, CAM sensor clock, emul trace clk */
/* Type: DPLL */
static struct dpll_data dpll4_dd = {
	.mult_div1_reg	= CM_CLKSEL2,
	.mult_mask	= OMAPW3G_PERIPH_DPLL_MULT_MASK,
	.div1_mask	= OMAPW3G_PERIPH_DPLL_DIV_MASK,
	.freqsel_mask	= OMAPW3G_PERIPH_DPLL_FREQSEL_MASK,
	.control_reg	= CM_CLKEN,
	.enable_mask	= OMAPW3G_EN_PERIPH_DPLL_MASK,
	.modes		= (1 << DPLL_LOW_POWER_STOP) | (1 << DPLL_LOCKED),
	.auto_recal_bit	= OMAPW3G_EN_PERIPH_DPLL_DRIFTGUARD_SHIFT,
	.recal_en_bit	= OMAPW3G_PERIPH_DPLL_RECAL_EN_SHIFT,
	.recal_st_bit	= OMAPW3G_PERIPH_DPLL_ST_SHIFT,
	.autoidle_reg	= CM_AUTOIDLE,
	.autoidle_mask	= OMAPW3G_AUTO_PERIPH_DPLL_MASK,
	.idlest_reg	= CM_IDLEST,
	.idlest_mask	= OMAPW3G_ST_PERIPH_CLK_MASK,
	.bypass_clk	= &sys_ck,
	.max_multiplier = OMAP3_MAX_DPLL_MULT,
	.min_divider	= 1,
	.max_divider	= OMAP3_MAX_DPLL_DIV,
	.rate_tolerance = DEFAULT_DPLL_RATE_TOLERANCE
};

static struct clk dpll4_ck = {
	.name		= "dpll4_ck",
	.parent		= &sys_ck,
	.prcm_mod	= OMAPW3G_CCR_MOD,
	.dpll_data	= &dpll4_dd,
	.flags		= CLOCK_IN_OMAP343X | RECALC_ON_ENABLE,
	.round_rate	= &omap2_dpll_round_rate,
	.set_rate	= &omap3_noncore_dpll_set_rate,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.clkdm		= { .name = "dpll4_clkdm" },
	.recalc		= &omap3_dpll_recalc,
};

static const struct clksel div16_dpll4_clksel[] = {
	{ .parent = &dpll4_ck, .rates = div16_dpll_rates },
	{ .parent = NULL }
};

/* This virtual clock is the source for dpll4_m2x2_ck */
static struct clk dpll4_m2_ck = {
	.name		= "dpll4_m2_ck",
	.parent		= &dpll4_ck,
	.prcm_mod	= OMAPW3G_CCR_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= OMAP3430_CM_CLKSEL3,
	.clksel_mask	= OMAPW3G_DIV_96M_MASK,
	.clksel		= div16_dpll4_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "dpll4_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

/* This virtual clock is the source for dpll4_m3_ck */
static struct clk dpll4_m3_ck = {
	.name           = "dpll4_m3_ck",
	.parent         = &dpll4_ck,
	.prcm_mod       = OMAPW3G_CCR_MOD,
	.init           = &omap2_init_clksel_parent,
	.clksel_reg     = OMAP3430_CM_CLKSEL3,
	.clksel_mask    = OMAPW3G_DIV_60M_MASK,
	.clksel         = div16_dpll4_clksel,
	.flags          = CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm          = { .name = "dpll4_clkdm" },
	.recalc         = &omap2_clksel_recalc,
	.set_rate       = &omap2_clksel_set_rate,
	.round_rate     = &omap2_clksel_round_rate,
};


/* This virstual clock is the source for dpll4_m4x2_ck */
static struct clk dpll4_m4_ck = {
	.name		= "dpll4_m4_ck",
	.parent		= &dpll4_ck,
	.prcm_mod	= OMAPW3G_DSS_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= CM_CLKSEL,
	.clksel_mask	= OMAPW3G_CLKSEL_DSS1_MASK,
	.clksel		= div16_dpll4_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "dpll4_clkdm" },
	.recalc		= &omap2_clksel_recalc,
	.set_rate	= &omap2_clksel_set_rate,
	.round_rate	= &omap2_clksel_round_rate,
};

/* This virtual clock is the source for dpll4_m5_ck */
static struct clk dpll4_m5_ck = {
	.name		= "dpll4_m5_ck",
	.parent		= &dpll4_ck,
	.prcm_mod	= OMAPW3G_CAM_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= CM_CLKSEL,
	.clksel_mask	= OMAPW3G_CLKSEL_CAM_MASK,
	.clksel		= div16_dpll4_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "dpll4_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

/* This virtual clock is the source for dpll4_m6x2_ck */
static struct clk dpll4_m6_ck = {
	.name		= "dpll4_m6_ck",
	.parent		= &dpll4_ck,
	.prcm_mod	= OMAPW3G_EMU_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= CM_CLKSEL1,
	.clksel_mask	= OMAPW3G_DIV_DPLL4_MASK,
	.clksel		= div16_dpll4_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "dpll4_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

static struct clk omap_60m_fck = {
	.name		= "omap_60m_fck",
	.parent		= &dpll4_ck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &followparent_recalc,
};


static struct clk cm_96m_fck = {
	.name		= "cm_96m_fck",
	.parent		= &dpll4_m2_ck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk cm_48m_fck = {
	.name		= "cm_48m_fck",
	.parent		= &dpll4_m2_ck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk cm_12m_fck = {
	.name		= "cm_12m_fck",
	.parent		= &dpll4_ck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &followparent_recalc,
};

/* CM EXTERNAL CLOCK OUTPUTS */

static const struct clksel_rate clkout2_src_core_rates[] = {
	{ .div = 1, .val = 0, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 0 }
};

static const struct clksel_rate clkout2_src_sys_rates[] = {
	{ .div = 1, .val = 1, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 0 }
};

static const struct clksel_rate clkout2_src_96m_rates[] = {
	{ .div = 1, .val = 2, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 0 }
};

static const struct clksel_rate clkout2_src_54m_rates[] = {
	{ .div = 1, .val = 3, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 0 }
};

static const struct clksel clkout2_src_clksel[] = {
	{ .parent = &core_ck,		.rates = clkout2_src_core_rates },
	{ .parent = &sys_ck,		.rates = clkout2_src_sys_rates },
	{ .parent = &cm_96m_fck,	.rates = clkout2_src_96m_rates },
	{ .parent = NULL }
};

static struct clk clkout2_src_ck = {
	.name		= "clkout2_src_ck",
	.prcm_mod	= OMAPW3G_CCR_MOD,
	.init		= &omap2_init_clksel_parent,
	.enable_reg	= OMAP3430_CM_CLKOUT_CTRL_OFFSET,
	.enable_bit	= OMAPW3G_CLKOUT2_EN_SHIFT,
	.clksel_reg	= OMAP3430_CM_CLKOUT_CTRL_OFFSET,
	.clksel_mask	= OMAPW3G_CLKOUT2SOURCE_MASK,
	.clksel		= clkout2_src_clksel,
	.flags		= CLOCK_IN_OMAP343X,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

static const struct clksel_rate sys_clkout2_rates[] = {
	{ .div = 1, .val = 0, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 2, .val = 1, .flags = RATE_IN_343X },
	{ .div = 4, .val = 2, .flags = RATE_IN_343X },
	{ .div = 8, .val = 3, .flags = RATE_IN_343X },
	{ .div = 16, .val = 4, .flags = RATE_IN_343X },
	{ .div = 0 },
};

static const struct clksel sys_clkout2_clksel[] = {
	{ .parent = &clkout2_src_ck, .rates = sys_clkout2_rates },
	{ .parent = NULL },
};

static struct clk sys_clkout2 = {
	.name		= "sys_clkout2",
	.prcm_mod	= OMAPW3G_CCR_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= OMAP3430_CM_CLKOUT_CTRL_OFFSET,
	.clksel_mask	= OMAPW3G_CLKOUT2_DIV_MASK,
	.clksel		= sys_clkout2_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

/* CM OUTPUT CLOCKS */

static struct clk corex2_fck = {
	.name		= "corex2_fck",
	.parent		= &dpll3_m2x2_ck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &followparent_recalc,
};

/* DPLL power domain clock controls */

static const struct clksel_rate div4_rates[] = {
	{ .div = 1, .val = 1, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 2, .val = 2, .flags = RATE_IN_343X },
	{ .div = 0 }
};

static const struct clksel div4_core_clksel[] = {
	{ .parent = &core_ck, .rates = div4_rates },
	{ .parent = NULL }
};

static struct clk dpll1_fck = {
	.name		= "dpll1_fck",
	.parent		= &core_ck,
	.prcm_mod	= OMAPW3G_MPU_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= OMAP3430_CM_CLKSEL1_PLL,
	.clksel_mask	= OMAPW3G_MPU_CLK_SRC_MASK,
	.clksel		= div4_core_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

static struct clk mpu_ck = {
	.name		= "mpu_ck",
	.parent		= &dpll1_x2m2_ck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "mpu_clkdm" },
	.recalc		= &followparent_recalc,
};

/* arm_fck is divided by two when DPLL1 locked; otherwise, passthrough mpu_ck */
static const struct clksel_rate arm_fck_rates[] = {
	{ .div = 1, .val = 0, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 2, .val = 1, .flags = RATE_IN_343X },
	{ .div = 0 },
};

static const struct clksel arm_fck_clksel[] = {
	{ .parent = &mpu_ck, .rates = arm_fck_rates },
	{ .parent = NULL }
};

static struct clk arm_fck = {
	.name		= "arm_fck",
	.parent		= &mpu_ck,
	.prcm_mod	= OMAPW3G_MPU_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= OMAP3430_CM_IDLEST_PLL,
	.clksel_mask	= OMAPW3G_ST_MPU_CLK_MASK,
	.clksel		= arm_fck_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "mpu_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

/* Common interface clocks */

static const struct clksel div2_core_clksel[] = {
	{ .parent = &core_ck, .rates = div2_rates },
	{ .parent = NULL }
};

static struct clk l3_ick = {
	.name		= "l3_ick",
	.parent		= &core_ck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= CM_CLKSEL,
	.clksel_mask	= OMAPW3G_CLKSEL_L3_MASK,
	.clksel		= div2_core_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "core_l3_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

static const struct clksel div2_l3_clksel[] = {
	{ .parent = &l3_ick, .rates = div2_rates },
	{ .parent = NULL }
};

static struct clk l4_ick = {
	.name		= "l4_ick",
	.parent		= &core_ck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.init		= &omap2_init_clksel_parent,
	.clksel_reg	= CM_CLKSEL,
	.clksel_mask	= OMAPW3G_CLKSEL_L4_MASK,
	.clksel		= div2_l3_clksel,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &omap2_clksel_recalc,

};

/* CORE power domain */

static struct clk mad2d_ick = {
	.name		= "mad2d_ick",
	.parent		= &l3_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_MAD2D_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X,
	.clkdm		= { .name = "core_l3_clkdm" },
	.recalc		= &followparent_recalc,
};

/* CORE 96M FCLK-derived clocks */

static struct clk core_96m_fck = {
	.name		= "core_96m_fck",
	.parent		= &cm_96m_fck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk mmchs2_fck = {
	.name		= "mmchs_fck",
	.id		= 1,
	.parent		= &core_96m_fck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_MMCSD2_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.idlest_bit	= OMAPW3G_ST_MMCSD2_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk mmchs1_fck = {
	.name		= "mmchs_fck",
	.parent		= &core_96m_fck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_MMCSD1_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.idlest_bit	= OMAPW3G_ST_MMCSD1_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk i2c1_fck = {
	.name		= "i2c_fck",
	.id		= 1,
	.parent		= &core_96m_fck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_I2C_FCLKEN_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.idlest_bit	= OMAPW3G_ST_I2C_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

/*
 * MCBSP 1 & 5 get their 96MHz clock from core_96m_fck;
 * MCBSP 2, 3, 4 get their 96MHz clock from per_96m_fck.
 */
static const struct clksel_rate common_mcbsp_96m_rates[] = {
	{ .div = 1, .val = 0, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 0 }
};

static const struct clksel_rate mcbsp1_ext_rates[] = {
	{ .div = 1, .val = 0, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 0 }
};

static const struct clksel_rate mcbsp1_sys_rates[] = {
	{ .div = 1, .val = 1, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 0 }
};

static const struct clksel_rate mcbsp1_96m_rates[] = {
	{ .div = 1, .val = 2, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 0 }
};

/* TODO: 96m_fck is actually the AWON one.... */

/* The other rate is the 15.36MHz clock, but it is supplied
 * through the WCDMA domain's MDM_15M_CLK which cannot be
 * used by the AP.
 */
static const struct clksel mcbsp1_clksel[] = {
	{ .parent = &mcbsp_clks,   .rates = mcbsp1_ext_rates },
	{ .parent = &sys_ck,	   .rates = mcbsp1_sys_rates },
	{ .parent = &cm_96m_fck,   .rates = mcbsp1_96m_rates },
	{ .parent = NULL }
};

/* APP_48M_FCK-derived clocks */

static struct clk app_48m_fck = {
	.name		= "app_48m_fck",
	.parent		= &cm_48m_fck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk mcspi2_fck = {
	.name		= "mcspi_fck",
	.id		= 2,
	.parent		= &app_48m_fck,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_SPI2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_SPI2_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

/* CORE_48M_FCK-derived clocks */

static struct clk core_48m_fck = {
	.name		= "core_48m_fck",
	.parent		= &cm_48m_fck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};



static struct clk mcspi1_fck = {
	.name		= "mcspi_fck",
	.id		= 1,
	.parent		= &core_48m_fck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_SPI1_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.idlest_bit	= OMAPW3G_ST_SPI1_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};


static struct clk uart3_fck = {
	.name		= "uart3_fck",
	.parent		= &core_48m_fck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_UART3_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.idlest_bit	= OMAPW3G_ST_UART3_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk uart2_fck = {
	.name		= "uart2_fck",
	.parent		= &core_48m_fck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_UART2_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.idlest_bit	= OMAPW3G_ST_UART2_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk uart1_fck = {
	.name		= "uart1_fck",
	.parent		= &core_48m_fck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_UART1_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.idlest_bit	= OMAPW3G_ST_UART1_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

/* APP_12M_FCK based clocks */
static struct clk app_12m_fck = {
	.name		= "app_12m_fck",
	.parent		= &cm_12m_fck,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk hdq_fck = {
	.name		= "hdq_fck",
	.parent		= &app_12m_fck,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_HDQ_SHIFT,
	.idlest_bit	= OMAPW3G_ST_HDQ_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};


/* CORE_L3_ICK based clocks */

/*
 * XXX must add clk_enable/clk_disable for these if standard code won't
 * handle it
 */
static struct clk core_l3_ick = {
	.name		= "core_l3_ick",
	.parent		= &l3_ick,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "core_l3_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk hsotgusb_ick = {
	.name		= "hsotgusb_ick",
	.parent		= &core_l3_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_HSOTGUSB_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X,
	.clkdm		= { .name = "core_l3_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk sdrc_ick = {
	.name		= "sdrc_ick",
	.parent		= &core_l3_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_SDRC_SHIFT,
	.idlest_bit	= OMAPW3G_ST_SDRC_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | ENABLE_ON_INIT | WAIT_READY,
	.clkdm		= { .name = "core_l3_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk gpmc_fck = {
	.name		= "gpmc_fck",
	.parent		= &core_l3_ick,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK |
				ENABLE_ON_INIT,
	.clkdm		= { .name = "core_l3_clkdm" },
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.recalc		= &followparent_recalc,
};

/* SECURITY_L3_ICK based clocks */
static struct clk security_l3_ick = {
	.name		= "security_l3_ick",
	.parent		= &l3_ick,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "app_l3_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk pka_ick = {
	.name		= "pka_ick",
	.parent		= &security_l3_ick,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_ICLKEN2,
	.enable_bit	= OMAPW3G_EN_PKA_SHIFT,
	.idlest_bit	= OMAPW3G_ST_PKA_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l3_clkdm" },
	.recalc		= &followparent_recalc,
};

/* CORE_L4_ICK based clocks */
static struct clk core_l4_ick = {
	.name		= "core_l4_ick",
	.parent		= &l4_ick,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};



static struct clk usim_ick = {
	.name		= "usim_ick",
	.parent		= &core_l4_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_USIMOCP_SHIFT,
	.idlest_bit	= OMAPW3G_ST_USIMOCP_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk mmchs2_ick = {
	.name		= "mmchs_ick",
	.id		= 1,
	.parent		= &core_l4_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_MMCSD2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_MMCSD2_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk mmchs1_ick = {
	.name		= "mmchs_ick",
	.parent		= &core_l4_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_MMCSD1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_MMCSD1_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};


static struct clk mcspi1_ick = {
	.name		= "mcspi_ick",
	.id		= 1,
	.parent		= &core_l4_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_SPI1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_SPI1_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk i2c1_ick = {
	.name		= "i2c_ick",
	.id		= 1,
	.parent		= &core_l4_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_I2C_ICLKEN_SHIFT,
	.idlest_bit	= OMAPW3G_ST_I2C_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};


static struct clk uart3_ick = {
	.name		= "uart3_ick",
	.parent		= &core_l4_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_UART3_SHIFT,
	.idlest_bit	= OMAPW3G_ST_UART3_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk uart2_ick = {
	.name		= "uart2_ick",
	.parent		= &core_l4_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_UART2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_UART2_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk uart1_ick = {
	.name		= "uart1_ick",
	.parent		= &core_l4_ick,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_UART1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_UART1_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "core_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

/* APP_L4_ICK based clocks */
static struct clk app_l4_ick = {
	.name		= "app_l4_ick",
	.parent		= &l4_ick,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk hdq_ick = {
	.name		= "hdq_ick",
	.parent		= &app_l4_ick,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_HDQ_SHIFT,
	.idlest_bit	= OMAPW3G_ST_HDQ_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk mcspi2_ick = {
	.name		= "mcspi_ick",
	.id		= 2,
	.parent		= &app_l4_ick,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_SPI2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_SPI2_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};


/* APP_SECURE_L4_ICK based clocks */
static struct clk security_l4_ick = {
	.name		= "security_l4_ick",
	.parent		= &l4_ick,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk sha12_ick = {
	.name		= "sha12_ick",
	.parent		= &security_l4_ick,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_SHA2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_SHA2_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk des2_ick = {
	.name		= "des2_ick",
	.parent		= &security_l4_ick,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_DES3_SHIFT,
	.idlest_bit	= OMAPW3G_ST_DES3_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk aes1_ick = {
	.name		= "aes1_ick",
	.parent		= &security_l4_ick,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_ICLKEN2,
	.enable_bit	= OMAPW3G_EN_AES_SHIFT,
	.idlest_bit	= OMAPW3G_ST_AES_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk rng_ick = {
	.name		= "rng_ick",
	.parent		= &security_l4_ick,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_ICLKEN2,
	.enable_bit	= OMAPW3G_EN_RNG_SHIFT,
	.idlest_bit	= OMAPW3G_ST_RNG_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};


static const struct clksel omap343x_gpt_clksel[] = {
	{ .parent = &omap_32k_fck, .rates = gpt_32k_rates },
	{ .parent = &sys_ck,	   .rates = gpt_sys_rates },
	{ .parent = NULL}
};

/* GPT3 Clock */
static struct clk gpt3_fck = {
	.name		= "gpt3_fck",
	.prcm_mod	= OMAPW3G_APP_MOD,
	.init		= &omap2_init_clksel_parent,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_GPT3_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPT3_SHIFT,
	.clksel_reg	= CM_CLKSEL,
	.clksel_mask	= OMAPW3G_CLKSEL_GPT3_MASK,
	.clksel		= omap343x_gpt_clksel,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};


static struct clk gpt3_ick = {
	.name		= "gpt3_ick",
	.parent		= &app_l4_ick,
	.prcm_mod	= OMAPW3G_APP_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_GPT3_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPT3_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "app_l4_clkdm" },
	.recalc		= &followparent_recalc,
};


/* DSS */
static struct clk dss1_alwon_fck = {
	.name		= "dss1_alwon_fck",
	.parent		= &dpll4_m4_ck,
	.prcm_mod	= OMAPW3G_DSS_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_DSS1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_DSS_IDLE_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "dss_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk dss2_alwon_fck = {
	.name		= "dss2_alwon_fck",
	.parent		= &sys_ck,
	.prcm_mod	= OMAPW3G_DSS_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_DSS2_SHIFT,
	.flags		= CLOCK_IN_OMAP343X,
	.clkdm		= { .name = "dss_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk dss_ick = {
	/* Handles both L3 and L4 clocks */
	.name		= "dss_ick",
	.parent		= &l4_ick,
	.prcm_mod	= OMAPW3G_DSS_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_CM_ICLKEN_DSS_EN_DSS_SHIFT,
	.flags		= CLOCK_IN_OMAP343X,
	.clkdm		= { .name = "dss_clkdm" },
	.recalc		= &followparent_recalc,
};


/* CAM */

static struct clk cam_mclk = {
	.name		= "cam_mclk",
	.parent		= &dpll4_m5_ck,
	.prcm_mod	= OMAPW3G_CAM_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_CAM_SHIFT,
	.flags		= CLOCK_IN_OMAP343X,
	.clkdm		= { .name = "cam_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk cam_ick = {
	/* Handles both L3 and L4 clocks */
	.name		= "cam_ick",
	.parent		= &l4_ick,
	.prcm_mod	= OMAPW3G_CAM_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_CAM_SHIFT,
	.flags		= CLOCK_IN_OMAP343X,
	.clkdm		= { .name = "cam_clkdm" },
	.recalc		= &followparent_recalc,
};

/* WKUP */

static const struct clksel_rate usim_rates[] = {
	{ .div = 1,  .val = 1, .flags = RATE_IN_343X | DEFAULT_RATE },
	{ .div = 2,  .val = 2, .flags = RATE_IN_343X },
	{ .div = 4,  .val = 4, .flags = RATE_IN_343X },
	{ .div = 6,  .val = 6, .flags = RATE_IN_343X },
	{ .div = 8,  .val = 8, .flags = RATE_IN_343X },
	{ .div = 10, .val = 10, .flags = RATE_IN_343X },
	{ .div = 0 },
};

static const struct clksel usim_clksel[] = {
	{ .parent = &dpll4_m2_ck,      .rates = usim_rates },
	{ .parent = &dpll4_m3_ck,      .rates = usim_rates },
	{ .parent = &sys_ck,	       .rates = div2_rates },
	{ .parent = NULL },
};

/*
 * W3G: The enabling/disabling of this clock is tied
 *      into the USIM_FCK as well.
 */
static struct clk usim32k_fck = {
	.name		= "usim32k_fck",
	.parent		= &omap_32k_fck,
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_USIMOCP_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};

/* Placing in cm_clkdm since can be: 96, 60, or sys */
static struct clk usim_fck = {
	.name		= "usim_fck",
	.prcm_mod	= OMAPW3G_CORE_MOD,
	.init		= &omap2_init_clksel_parent,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_USIMOCP_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.idlest_bit	= OMAPW3G_ST_USIMOCP_SHIFT,
	.clksel_reg	= CM_CLKSEL,
	.clksel_mask	= OMAPW3G_CLKSEL_USIMOCP_SRC_MASK,
	.clksel		= usim_clksel,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "cm_clkdm" },
	.recalc		= &omap2_clksel_recalc,
	.set_rate       = &omap2_clksel_set_rate,
};


/* XXX should gpt1's clksel have wkup_32k_fck as the 32k opt? */
static struct clk gpt1_fck = {
	.name		= "gpt1_fck",
	.prcm_mod	= OMAPW3G_WKUP_MOD,
	.init		= &omap2_init_clksel_parent,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_GPT1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPT1_SHIFT,
	.clksel_reg	= CM_CLKSEL,
	.clksel_mask	= OMAPW3G_CLKSEL_GPT1_MASK,
	.clksel		= omap343x_gpt_clksel,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};

static struct clk wkup_32k_fck = {
	.name		= "wkup_32k_fck",
	.parent		= &omap_32k_fck,
	.flags		= CLOCK_IN_OMAP343X | ALWAYS_ENABLED,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk gpio1_dbck = {
	.name		= "gpio1_dbck",
	.parent		= &wkup_32k_fck,
	.prcm_mod	= OMAPW3G_WKUP_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_GPIO1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPIO1_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk wdt2_fck = {
	.name		= "wdt2_fck",
	.parent		= &wkup_32k_fck,
	.prcm_mod	= OMAPW3G_WKUP_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_WDT2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_WDT2_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk wkup_l4_ick = {
	.name		= "wkup_l4_ick",
	.parent		= &sys_ck,
	.flags		= CLOCK_IN_OMAP343X | ALWAYS_ENABLED,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};


static struct clk wdt2_ick = {
	.name		= "wdt2_ick",
	.parent		= &wkup_l4_ick,
	.prcm_mod	= OMAPW3G_WKUP_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_WDT2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_WDT2_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk wdt1_ick = {
	.name		= "wdt1_ick",
	.parent		= &wkup_l4_ick,
	.prcm_mod	= OMAPW3G_WKUP_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_WDT1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_WDT1_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk gpio1_ick = {
	.name		= "gpio1_ick",
	.parent		= &wkup_l4_ick,
	.prcm_mod	= OMAPW3G_WKUP_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_GPIO1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPIO1_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk gpt1_ick = {
	.name		= "gpt1_ick",
	.parent		= &wkup_l4_ick,
	.prcm_mod	= OMAPW3G_WKUP_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_GPT1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPT1_SHIFT,
	.enable         = &omapw3g_clk_enable_mute,
	.disable        = &omapw3g_clk_disable_mute,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};



/* AUD clock domain */
static struct clk aud_32k_alwon_fck = {
	.name		= "aud_32k_alwon_fck",
	.parent		= &omap_32k_fck,
	.clkdm		= { .name = "aud_clkdm" },
	.flags		= CLOCK_IN_OMAP343X | ALWAYS_ENABLED,
	.recalc		= &followparent_recalc,
};

static struct clk gpt2_fck = {
	.name		= "gpt2_fck",
	.prcm_mod	= OMAPW3G_AUD_MOD,
	.init		= &omap2_init_clksel_parent,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_GPT2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPT2_SHIFT,
	.clksel_reg	= CM_CLKSEL,
	.clksel_mask	= OMAPW3G_CLKSEL_GPT2_MASK,
	.clksel		= omap343x_gpt_clksel,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "aud_clkdm" },
	.recalc		= &omap2_clksel_recalc,
};


static struct clk mcbsp1_fck = {
	.name		= "mcbsp_fck",
	.id		= 1,
	.prcm_mod	= OMAPW3G_AUD_MOD,
	.init		= &omap2_init_clksel_parent,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_MCBSP1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_MCBSP1_SHIFT,
	.clksel_reg     = CM_CLKSEL,
	.clksel_mask    = OMAPW3G_CLKSEL_MCBSP1_MASK,
	.clksel         = &mcbsp1_clksel,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "aud_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk gpio3_dbck = {
	.name		= "gpio3_dbck",
	.parent		= &aud_32k_alwon_fck,
	.prcm_mod	= OMAPW3G_AUD_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_GPIO3_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPIO3_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "aud_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk gpio2_dbck = {
	.name		= "gpio2_dbck",
	.parent		= &aud_32k_alwon_fck,
	.prcm_mod	= OMAPW3G_AUD_MOD,
	.enable_reg	= CM_FCLKEN,
	.enable_bit	= OMAPW3G_EN_GPIO2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPIO2_SHIFT,
	.enable         = &omapw3g_clk_enable,
	.disable        = &omapw3g_clk_disable,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "aud_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk aud_l4_ick = {
	.name		= "aud_l4_ick",
	.parent		= &l4_ick,
	.flags		= CLOCK_IN_OMAP343X | PARENT_CONTROLS_CLOCK,
	.clkdm		= { .name = "aud_clkdm" },
	.recalc		= &followparent_recalc,
};

/* TODO: Sheldon should have these clock enable/disables
 *       "muted" as they don't have control.  GPIO2 is
 *       attempted to be controlled incorrectly.
 */
static struct clk gpio3_ick = {
	.name		= "gpio3_ick",
	.parent		= &aud_l4_ick,
	.prcm_mod	= OMAPW3G_AUD_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_GPIO3_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPIO3_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "aud_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk gpio2_ick = {
	.name		= "gpio2_ick",
	.parent		= &aud_l4_ick,
	.prcm_mod	= OMAPW3G_AUD_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_GPIO2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPIO2_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "aud_clkdm" },
	.recalc		= &followparent_recalc,
};


static struct clk gpt2_ick = {
	.name		= "gpt2_ick",
	.parent		= &aud_l4_ick,
	.prcm_mod	= OMAPW3G_AUD_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_GPT2_SHIFT,
	.idlest_bit	= OMAPW3G_ST_GPT2_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "aud_clkdm" },
	.recalc		= &followparent_recalc,
};



static struct clk mcbsp1_ick = {
	.name		= "mcbsp_ick",
	.id		= 1,
	.parent		= &aud_l4_ick,
	.prcm_mod	= OMAPW3G_AUD_MOD,
	.enable_reg	= CM_ICLKEN,
	.enable_bit	= OMAPW3G_EN_MCBSP1_SHIFT,
	.idlest_bit	= OMAPW3G_ST_MCBSP1_SHIFT,
	.flags		= CLOCK_IN_OMAP343X | WAIT_READY,
	.clkdm		= { .name = "aud_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk wdt1_fck = {
	.name		= "wdt1_fck",
	.parent		= &omap_32k_fck,
	.flags		= CLOCK_IN_OMAP343X | ALWAYS_ENABLED,
	.clkdm		= { .name = "prm_clkdm" },
	.recalc		= &followparent_recalc,
};

static struct clk *onchip_34xx_clks[] __initdata = {
	&omap_32k_fck,
	&sys_ck,
	&mcbsp_clks,
	&sys_clkout1,
	&dpll1_ck,
	&dpll1_x2_ck,
	&dpll1_x2m2_ck,
	&dpll3_ck,
	&core_ck,
	&dpll3_m2_ck,
	&dpll3_m2x2_ck,
	&dpll4_ck,
	&cm_96m_fck,
	&cm_48m_fck,
	&cm_12m_fck,
	&omap_60m_fck,
	&core_96m_fck,
	&core_48m_fck,
	&app_48m_fck,
	&app_12m_fck,
	&app_l4_ick,
	&dpll4_m2_ck,
	&dpll4_m3_ck,
	&dpll4_m4_ck,
	&dpll4_m5_ck,
	&dpll4_m6_ck,
	&clkout2_src_ck,
	&sys_clkout2,
	&corex2_fck,
	&dpll1_fck,
	&mpu_ck,
	&arm_fck,
	&l3_ick,
	&l4_ick,
	&mad2d_ick,
	&mmchs2_fck,
	&mmchs1_fck,
	&i2c1_fck,
	&mcbsp1_fck,
	&mcspi2_fck,
	&mcspi1_fck,
	&uart2_fck,
	&uart1_fck,
	&hdq_fck,
	&core_l3_ick,
	&hsotgusb_ick,
	&sdrc_ick,
	&gpmc_fck,
	&security_l3_ick,
	&pka_ick,
	&core_l4_ick,
	&sha12_ick,
	&des2_ick,
	&mmchs2_ick,
	&mmchs1_ick,
	&hdq_ick,
	&mcspi2_ick,
	&mcspi1_ick,
	&i2c1_ick,
	&uart2_ick,
	&uart1_ick,
	&mcbsp1_ick,
	&security_l4_ick,
	&aes1_ick,
	&rng_ick,
	&dss1_alwon_fck,
	&dss2_alwon_fck,
	&dss_ick,
	&cam_mclk,
	&cam_ick,
	&usim_fck,
	&gpt1_fck,
	&wkup_32k_fck,
	&gpio1_dbck,
	&wdt2_fck,
	&wkup_l4_ick,
	&usim_ick,
	&wdt2_ick,
	&wdt1_ick,
	&gpio1_ick,
	&gpt1_ick,
	&uart3_fck,
	&gpt2_fck,
	&gpt3_fck,
	&aud_32k_alwon_fck,
	&gpio3_dbck,
	&gpio2_dbck,
	&aud_l4_ick,
	&gpio3_ick,
	&gpio2_ick,
	&uart3_ick,
	&gpt3_ick,
	&gpt2_ick,
	&wdt1_fck,
	&usim32k_fck,
};

#endif
