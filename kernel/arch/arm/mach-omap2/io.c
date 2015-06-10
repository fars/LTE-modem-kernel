/*
 * linux/arch/arm/mach-omap2/io.c
 *
 * OMAP2 I/O mapping code
 *
 * Copyright (C) 2005 Nokia Corporation
 * Copyright (C) 2007 Texas Instruments
 * Copyright (C) 2009 Motorola Inc.
 *
 * Author:
 *      Juha Yrjola <juha.yrjola@nokia.com>
 *      Syed Khasim <x0khasim@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/omapfb.h>
#include <linux/io.h>
#include <linux/clk.h>

#include <asm/tlb.h>

#include <asm/mach/map.h>
#include <mach/mux.h>
#include <mach/sram.h>
#include <mach/sdrc.h>
#include <mach/gpmc.h>
#include <mach/vram.h>

#include "omapdev-common.h"

#include "clock.h"

#include <mach/powerdomain.h>

#include "powerdomains.h"

#include <mach/clockdomain.h>
#include "clockdomains.h"

#include <mach/omap-pm.h>

#include <dspbridge/host_os.h>

/*
 * The machine specific code may provide the extra mapping besides the
 * default mapping provided here.
 */

#ifdef CONFIG_ARCH_OMAP24XX
#ifdef CONFIG_ARCH_OMAPW3G
static struct map_desc w3g_io_desc[] __initdata= {
	{
        	.virtual    = L3_W3G_VIRT,
        	.pfn        = __phys_to_pfn(L3_W3G_PHYS),
        	.length     = L3_W3G_SIZE,
        	.type       = MT_DEVICE
	},
	{   
        	.virtual    = L3_W3G_INC_VIRT,
        	.pfn        = __phys_to_pfn(L3_W3G_INC_PHYS),
        	.length     = L3_W3G_INC_SIZE,
       		.type       = MT_DEVICE
	},
	{
        	.virtual    = W3G_IPC_SHARED_MEMORY_VIRT,
        	.pfn        = __phys_to_pfn(W3G_IPC_SHARED_MEMORY_PHYS),
        	.length     = W3G_IPC_SHARED_MEMORY_SIZE,
        	.type       = MT_DEVICE
	},
#ifdef CONFIG_MACH_W3G_LTE_DATACARD   
        {
                .virtual    = W3G_4GHIF_IPC_SHARED_MEMORY_VIRT,
                .pfn        = __phys_to_pfn(W3G_4GHIF_IPC_SHARED_MEMORY_PHYS),
                .length     = W3G_4GHIF_IPC_SHARED_MEMORY_SIZE,
                .type       = MT_DEVICE
        },   
#endif    	
	{
        	.virtual    = L4_W3G_APPS_VIRT,
        	.pfn        = __phys_to_pfn(L4_W3G_APPS_PHYS),
        	.length     = L4_W3G_APPS_SIZE,
        	.type       = MT_DEVICE
	},
	{
        	.virtual    = L3_MDMDSP_VIRT,
        	.pfn        = __phys_to_pfn(L3_MDMDSP_PHYS),
        	.length     = L3_MDMDSP_SIZE,
        	.type       = MT_DEVICE
	},
	{
        	.virtual    = L3_L4_PER_VIRT,
        	.pfn        = __phys_to_pfn(L3_L4_PER_PHYS),
        	.length     = L3_L4_PER_SIZE,
        	.type       = MT_DEVICE
	},
	{
        	.virtual    = L4_3G_BASE_VIRT,
        	.pfn        = __phys_to_pfn(L4_3G_BASE_PHYS),
        	.length     = L4_3G_BASE_SIZE,
        	.type       = MT_DEVICE
	},
	{
        	.virtual    = L4_EMU_BASE_VIRT,
        	.pfn        = __phys_to_pfn(L4_EMU_BASE_PHYS),
        	.length     = L4_EMU_BASE_SIZE,
        	.type       = MT_DEVICE
	},
};
#else

static struct map_desc omap24xx_io_desc[] __initdata = {
	{
		.virtual	= L3_24XX_VIRT,
		.pfn		= __phys_to_pfn(L3_24XX_PHYS),
		.length		= L3_24XX_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= L4_24XX_VIRT,
		.pfn		= __phys_to_pfn(L4_24XX_PHYS),
		.length		= L4_24XX_SIZE,
		.type		= MT_DEVICE
	},
};
#endif

#ifdef CONFIG_ARCH_OMAP2420
static struct map_desc omap242x_io_desc[] __initdata = {
	{
		.virtual	= DSP_MEM_24XX_VIRT,
		.pfn		= __phys_to_pfn(DSP_MEM_24XX_PHYS),
		.length		= DSP_MEM_24XX_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= DSP_IPI_24XX_VIRT,
		.pfn		= __phys_to_pfn(DSP_IPI_24XX_PHYS),
		.length		= DSP_IPI_24XX_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= DSP_MMU_24XX_VIRT,
		.pfn		= __phys_to_pfn(DSP_MMU_24XX_PHYS),
		.length		= DSP_MMU_24XX_SIZE,
		.type		= MT_DEVICE
	},
};

#endif

#ifdef CONFIG_ARCH_OMAP2430
static struct map_desc omap243x_io_desc[] __initdata = {
	{
		.virtual	= L4_WK_243X_VIRT,
		.pfn		= __phys_to_pfn(L4_WK_243X_PHYS),
		.length		= L4_WK_243X_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= OMAP243X_GPMC_VIRT,
		.pfn		= __phys_to_pfn(OMAP243X_GPMC_PHYS),
		.length		= OMAP243X_GPMC_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= OMAP243X_SDRC_VIRT,
		.pfn		= __phys_to_pfn(OMAP243X_SDRC_PHYS),
		.length		= OMAP243X_SDRC_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= OMAP243X_SMS_VIRT,
		.pfn		= __phys_to_pfn(OMAP243X_SMS_PHYS),
		.length		= OMAP243X_SMS_SIZE,
		.type		= MT_DEVICE
	},
};
#endif
#endif

#ifdef	CONFIG_ARCH_OMAP34XX
static struct map_desc omap34xx_io_desc[] __initdata = {
	{
		.virtual	= L3_34XX_VIRT,
		.pfn		= __phys_to_pfn(L3_34XX_PHYS),
		.length		= L3_34XX_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= L4_34XX_VIRT,
		.pfn		= __phys_to_pfn(L4_34XX_PHYS),
		.length		= L4_34XX_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= L4_WK_34XX_VIRT,
		.pfn		= __phys_to_pfn(L4_WK_34XX_PHYS),
		.length		= L4_WK_34XX_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= OMAP34XX_GPMC_VIRT,
		.pfn		= __phys_to_pfn(OMAP34XX_GPMC_PHYS),
		.length		= OMAP34XX_GPMC_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= OMAP343X_SMS_VIRT,
		.pfn		= __phys_to_pfn(OMAP343X_SMS_PHYS),
		.length		= OMAP343X_SMS_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= OMAP343X_SDRC_VIRT,
		.pfn		= __phys_to_pfn(OMAP343X_SDRC_PHYS),
		.length		= OMAP343X_SDRC_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= L4_PER_34XX_VIRT,
		.pfn		= __phys_to_pfn(L4_PER_34XX_PHYS),
		.length		= L4_PER_34XX_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= L4_EMU_34XX_VIRT,
		.pfn		= __phys_to_pfn(L4_EMU_34XX_PHYS),
		.length		= L4_EMU_34XX_SIZE,
		.type		= MT_DEVICE
	},
};
#endif

void __init omap2_map_common_io(void)
{
#if defined(CONFIG_ARCH_OMAPW3G)/* Wrigley Mapping */
    iotable_init(w3g_io_desc, ARRAY_SIZE(w3g_io_desc));
#else

#if defined(CONFIG_ARCH_OMAP2420)
	iotable_init(omap24xx_io_desc, ARRAY_SIZE(omap24xx_io_desc));
	iotable_init(omap242x_io_desc, ARRAY_SIZE(omap242x_io_desc));
#endif

#if defined(CONFIG_ARCH_OMAP2430)
	iotable_init(omap24xx_io_desc, ARRAY_SIZE(omap24xx_io_desc));
	iotable_init(omap243x_io_desc, ARRAY_SIZE(omap243x_io_desc));
#endif

#if defined(CONFIG_ARCH_OMAP34XX)
	iotable_init(omap34xx_io_desc, ARRAY_SIZE(omap34xx_io_desc));
#endif

#endif


	/* Normally devicemaps_init() would flush caches and tlb after
	 * mdesc->map_io(), but we must also do it here because of the CPU
	 * revision check below.
	 */
	local_flush_tlb_all();
	flush_cache_all();

	omap2_check_revision();
#ifndef CONFIG_ARCH_OMAPW3G
	omap_sram_init();
	dspbridge_reserve_sdram();
#endif

#ifdef CONFIG_OMAP_DISPLAY // Frame buffer not required since no display    
	omapfb_reserve_sdram();
        //MK:dspbridge_reserve_sdram();
#endif    
}

/*
 * omap2_init_reprogram_sdrc - reprogram SDRC timing parameters
 *
 * Sets the CORE DPLL3 M2 divider to the same value that it's at
 * currently.  This has the effect of setting the SDRC SDRAM AC timing
 * registers to the values currently defined by the kernel.  Currently
 * only defined for OMAP3; will return 0 if called on OMAP2.  Returns
 * -EINVAL if the dpll3_m2_ck cannot be found, 0 if called on OMAP2,
 * or passes along the return value of clk_set_rate().
 */
static int __init _omap2_init_reprogram_sdrc(void)
{
	struct clk *dpll3_m2_ck;
	int v = -EINVAL;

	if (!cpu_is_omap34xx())
		return 0;

	dpll3_m2_ck = clk_get(NULL, "dpll3_m2_ck");
	if (!dpll3_m2_ck)
		return -EINVAL;

	pr_info("Reprogramming SDRC\n");
	v = clk_set_rate(dpll3_m2_ck, clk_get_rate(dpll3_m2_ck));
	if (v)
		pr_err("dpll3_m2_clk rate change failed: %d\n", v);

	clk_put(dpll3_m2_ck);

	return v;
}

void __init omap2_init_common_hw(struct omap_sdrc_params *sp,
				 struct omap_opp *mpu_opps,
				 struct omap_opp *dsp_opps,
				 struct omap_opp *l3_opps)
{
	omap2_mux_init();
	/* The OPP tables have to be registered before a clk init */
	omap_pm_if_early_init(mpu_opps, dsp_opps, l3_opps);
	pwrdm_init(powerdomains_omap);
	clkdm_init(clockdomains_omap, clkdm_pwrdm_autodeps);
#ifndef CONFIG_ARCH_OMAPW3G
	omapdev_init(omapdevs);
#endif
	omap2_clk_init();
	omap_pm_if_init();
#ifndef CONFIG_ARCH_OMAPW3G
	omap2_sdrc_init(sp);
	_omap2_init_reprogram_sdrc();
#endif

	gpmc_init();
}
