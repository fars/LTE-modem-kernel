/*
 * linux/arch/arm/mach-omap2/pm34xx.c
 *
 * OMAP3 Power Management Routines
 *
 * Copyright (C) 2009 Motorola Inc.
 *
 * Copyright (C) 2006-2008 Nokia Corporation
 * Tony Lindgren <tony@atomide.com>
 * Jouni Hogander
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 * Rajendra Nayak <rnayak@ti.com>
 *
 * Copyright (C) 2005 Texas Instruments, Inc.
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * Based on pm.c for omap1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/ipc_vr_shortmsg.h>

#include <mach/gpio.h>
#include <mach/sram.h>
#include <mach/prcm.h>
#include <mach/clockdomain.h>
#include <mach/powerdomain.h>
#include <mach/resource.h>
#include <mach/serial.h>
#include <mach/control.h>
#include <mach/serial.h>
#include <mach/gpio.h>
#include <mach/sdrc.h>
#include <mach/dma.h>
#include <mach/gpmc.h>
#include <mach/dma.h>
#include <mach/dmtimer.h>
#include <mach/system.h>

#include <asm/tlbflush.h>

#include <linux/delay.h>

#include "cm.h"
/* TODO: Remove the 34xx.h definitions..... */
#include "cm-regbits-34xx.h"
#include "prm-regbits-34xx.h"
#include "cm-regbits-w3g.h"
#include "prm-regbits-w3g.h"

#include "prm.h"
#include "pm.h"
#include "smartreflex.h"
#include "sdrc.h"

static int regset_save_on_suspend;

/* Scratchpad offsets */
#define OMAP343X_TABLE_ADDRESS_OFFSET	   0x31
#define OMAP343X_TABLE_VALUE_OFFSET	   0x30
#define OMAP343X_CONTROL_REG_VALUE_OFFSET  0x32

struct power_state {
	struct powerdomain *pwrdm;
	u32 next_state;
#ifdef CONFIG_SUSPEND
	u32 saved_state;
#endif
	struct list_head node;
};

static LIST_HEAD(pwrst_list);

static int (*_omap_save_secure_sram) (u32 *addr);

static struct powerdomain *mpu_pwrdm;
static struct powerdomain *cam_pwrdm, *dss_pwrdm;
static struct powerdomain *core_pwrdm;

static void omapw3g_pm_idle(void)
{
	local_irq_disable();
	local_fiq_disable();

	if (!omap3_can_sleep())
		goto out;

	if (omap_irq_pending() || need_resched())
		goto out;

	arch_idle();

out:
	local_fiq_enable();
	local_irq_enable();
}

static struct prm_setup_vc prm_setup = {
	.clksetup = 0xff,
	.voltsetup_time1 = 0xfff,
	.voltsetup_time2 = 0xfff,
	.voltoffset = 0xff,
	.voltsetup2 = 0xff,
	.vdd0_on = 0x30,
	.vdd0_onlp = 0x1e,
	.vdd0_ret = 0x1e,
	.vdd0_off = 0x30,
	.vdd1_on = 0x2c,
	.vdd1_onlp = 0x1e,
	.vdd1_ret = 0x1e,
	.vdd1_off = 0x2c,
	.i2c_slave_ra = (R_SRI2C_SLAVE_ADDR << OMAP3430_SMPS_SA1_SHIFT) |
	    (R_SRI2C_SLAVE_ADDR << OMAP3430_SMPS_SA0_SHIFT),
	.vdd_vol_ra = (R_VDD2_SR_CONTROL << OMAP3430_VOLRA1_SHIFT) |
	    (R_VDD1_SR_CONTROL << OMAP3430_VOLRA0_SHIFT),
	/* vdd_vol_ra controls both cmd and vol, set the address equal */
	.vdd_cmd_ra = (R_VDD2_SR_CONTROL << OMAP3430_VOLRA1_SHIFT) |
	    (R_VDD1_SR_CONTROL << OMAP3430_VOLRA0_SHIFT),
	.vdd_ch_conf = OMAP3430_CMD1 | OMAP3430_RAV1,
	.vdd_i2c_cfg = OMAP3430_MCODE_SHIFT | OMAP3430_HSEN | OMAP3430_SREN,
};

static void omap3_enable_io_chain(void)
{
	int timeout = 0;

	if (omap_rev() >= OMAP3430_REV_ES3_1) {
		prm_set_mod_reg_bits(OMAP3430_EN_IO_CHAIN, WKUP_MOD, PM_WKEN);
		/* Do a readback to assure write has been done */
		prm_read_mod_reg(WKUP_MOD, PM_WKEN);

		while (!(prm_read_mod_reg(WKUP_MOD, PM_WKST) &
			 OMAP3430_ST_IO_CHAIN)) {
			timeout++;
			if (timeout > 1000) {
                               printk(KERN_ERR "Wake up daisy chain "
                                      "activation failed.\n");
                               return;
			}

			prm_set_mod_reg_bits(OMAP3430_ST_IO_CHAIN,
					     WKUP_MOD, PM_WKST);
		}
	}
}

static void omap3_disable_io_chain(void)
{
	if (omap_rev() >= OMAP3430_REV_ES3_1)
		prm_clear_mod_reg_bits(OMAP3430_EN_IO_CHAIN, WKUP_MOD, PM_WKEN);
}

/*
 * FIXME: This function should be called before entering off-mode after
 * OMAP3 secure services have been accessed. Currently it is only called
 * once during boot sequence, but this works as we are not using secure
 * services.
 */
static void omap3_save_secure_ram_context(u32 target_mpu_state)
{
	u32 ret;

	if (omap_type() != OMAP2_DEVICE_TYPE_GP) {
		/*
		 * MPU next state must be set to POWER_ON temporarily,
		 * otherwise the WFI executed inside the ROM code
		 * will hang the system.
		 */
		pwrdm_set_next_pwrst(mpu_pwrdm, PWRDM_POWER_ON);
		/*              ret = _omap_save_secure_sram((u32 *)
		 *                              __pa(omap3_secure_ram_storage));
		 */
		pwrdm_set_next_pwrst(mpu_pwrdm, target_mpu_state);
		/* Following is for error tracking, it should not happen */
		if (ret) {
			printk(KERN_ERR "save_secure_sram() returns %08x\n",
			       ret);
			while (1)
				;
		}
	}
}

/*
 * PRCM Interrupt Handler Helper Function
 *
 * The purpose of this function is to clear any wake-up events latched
 * in the PRCM PM_WKST_x registers. It is possible that a wake-up event
 * may occur whilst attempting to clear a PM_WKST_x register and thus
 * set another bit in this register. A while loop is used to ensure
 * that any peripheral wake-up events occurring while attempting to
 * clear the PM_WKST_x are detected and cleared.
 */
static void prcm_clear_mod_irqs(s16 module, u16 wkst_off,
				u16 iclk_off, u16 fclk_off)
{
	u32 wkst, fclk, iclk;

	wkst = prm_read_mod_reg(module, wkst_off);
	if (wkst) {
		iclk = cm_read_mod_reg(module, iclk_off);
		fclk = cm_read_mod_reg(module, fclk_off);
		while (wkst) {
			cm_set_mod_reg_bits(wkst, module, iclk_off);
			cm_set_mod_reg_bits(wkst, module, fclk_off);
			prm_write_mod_reg(wkst, module, wkst_off);
			wkst = prm_read_mod_reg(module, wkst_off);
		}
		cm_write_mod_reg(iclk, module, iclk_off);
		cm_write_mod_reg(fclk, module, fclk_off);
	}
}

/*
 * PRCM Interrupt Handler
 *
 * The PRM_IRQSTATUS_MPU register indicates if there are any pending
 * interrupts from the PRCM for the MPU. These bits must be cleared in
 * order to clear the PRCM interrupt. The PRCM interrupt handler is
 * implemented to simply clear the PRM_IRQSTATUS_MPU in order to clear
 * the PRCM interrupt. Please note that bit 0 of the PRM_IRQSTATUS_MPU
 * register indicates that a wake-up event is pending for the MPU and
 * this bit can only be cleared if the all the wake-up events latched
 * in the various PM_WKST_x registers have been cleared. The interrupt
 * handler is implemented using a do-while loop so that if a wake-up
 * event occurred during the processing of the prcm interrupt handler
 * (setting a bit in the corresponding PM_WKST_x register and thus
 * preventing us from clearing bit 0 of the PRM_IRQSTATUS_MPU register)
 * this would be handled.
 */
static irqreturn_t prcm_interrupt_handler(int irq, void *dev_id)
{
	u32 irqstatus_mpu;

	do {
		prcm_clear_mod_irqs(OMAPW3G_WKUP_MOD, PM_WKST,
				    CM_ICLKEN, CM_FCLKEN);
		prcm_clear_mod_irqs(OMAPW3G_CORE_MOD, PM_WKST,
				    CM_ICLKEN, CM_ICLKEN);
		prcm_clear_mod_irqs(OMAPW3G_APP_MOD, PM_WKST,
				    CM_ICLKEN, CM_FCLKEN);

		irqstatus_mpu = prm_read_mod_reg(
					OMAPW3G_OCP_MOD,
					OMAP2_PRM_IRQSTATUS_MPU_OFFSET);
		prm_write_mod_reg(irqstatus_mpu, OMAPW3G_OCP_MOD,
				  OMAP2_PRM_IRQSTATUS_MPU_OFFSET);

	} while (prm_read_mod_reg(OMAPW3G_OCP_MOD, OMAP2_PRM_IRQSTATUS_MPU_OFFSET));

	return IRQ_HANDLED;
}

static void restore_control_register(u32 val)
{
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0" : : "r"(val));
}

/* Function to restore the table entry that was modified for enabling MMU */
static void restore_table_entry(void)
{
	u32 *scratchpad_address;
	u32 previous_value, control_reg_value;
	u32 *address;
	scratchpad_address = OMAP2_IO_ADDRESS(OMAP343X_SCRATCHPAD);
	/* Get address of entry that was modified */
	address = (u32 *) __raw_readl(scratchpad_address
				      + OMAP343X_TABLE_ADDRESS_OFFSET);
	/* Get the previous value which needs to be restored */
	previous_value = __raw_readl(scratchpad_address
				     + OMAP343X_TABLE_VALUE_OFFSET);
	address = __va(address);
	*address = previous_value;
	flush_tlb_all();
	control_reg_value = __raw_readl(scratchpad_address
					+ OMAP343X_CONTROL_REG_VALUE_OFFSET);
	/* This will enable caches and prediction */
	restore_control_register(control_reg_value);
}

extern void omap_udelay(u32 seconds);
void omap_sram_idle(void)
{
	int core_next_state = PWRDM_POWER_ON;

	cm_clear_mod_reg_bits(OMAPW3G_EN_GPT3,
			OMAPW3G_APP_MOD,
			CM_FCLKEN);


	pwrdm_clear_all_prev_pwrst(mpu_pwrdm);
	pwrdm_clear_all_prev_pwrst(cam_pwrdm);
	pwrdm_clear_all_prev_pwrst(dss_pwrdm);

	pwrdm_pre_transition();

	if (pwrdm_read_pwrst(cam_pwrdm) == PWRDM_POWER_ON)
		omap2_clkdm_deny_idle(mpu_pwrdm->pwrdm_clkdms[0]);

	/* CORE */
	omap_uart_prepare_idle(0);
	omap_uart_prepare_idle(1);
	omap_uart_prepare_idle(2);

	core_next_state = pwrdm_read_next_pwrst(core_pwrdm);
	if (core_next_state < PWRDM_POWER_ON) {
		/* Enable IO-PAD and IO-CHAIN wakeups */
		/* TODO: ????? */
		prm_set_mod_reg_bits(OMAP3430_EN_IO, WKUP_MOD, PM_WKEN);
		omap3_enable_io_chain();
	}

	if (regset_save_on_suspend)
		pm_dbg_regset_save(1);

	/*
	 * omap3_arm_context is the location where ARM registers
	 * get saved. The restore path then reads from this
	 * location and restores them back.
	 */
	/* _omap_sram_idle(omap3_arm_context, save_state); */
	omapw3g_pm_idle();

	cpu_init();

	/* Restore table entry modified during MMU restoration */
	/* TODO: ???? */
#if 0
	if (pwrdm_read_prev_pwrst(mpu_pwrdm) == PWRDM_POWER_OFF)
		restore_table_entry();
#endif

	/* CORE */
	omap_uart_resume_idle(0);
	omap_uart_resume_idle(1);
	omap_uart_resume_idle(2);

	cm_set_mod_reg_bits(OMAPW3G_EN_GPT3,
			OMAPW3G_APP_MOD,
			CM_FCLKEN);
	omap_udelay(500);


#if 0
	/* Disable IO-PAD and IO-CHAIN wakeup */
	if (core_next_state < PWRDM_POWER_ON) {
		prm_clear_mod_reg_bits(OMAP3430_EN_IO, WKUP_MOD, PM_WKEN);
		omap3_disable_io_chain();
	}
#endif

	pwrdm_post_transition();

	omap2_clkdm_allow_idle(mpu_pwrdm->pwrdm_clkdms[0]);
}

int omap3_can_sleep(void)
{
	if (!enable_dyn_sleep)
		return 0;
	if (!omap_uart_can_sleep())
		return 0;
	if (atomic_read(&sleep_block) > 0)
		return 0;
	return 1;
}

/* This sets pwrdm state (other than mpu & core. Currently only ON &
 * RET are supported. Function is assuming that clkdm doesn't have
 * hw_sup mode enabled. */
int set_pwrdm_state(struct powerdomain *pwrdm, u32 state)
{
	u32 cur_state;
	int sleep_switch = 0;
	int ret = 0;

	if (pwrdm == NULL || IS_ERR(pwrdm))
		return -EINVAL;

	while (!(pwrdm->pwrsts & (1 << state))) {
		if (state == PWRDM_POWER_OFF)
			return ret;
		state--;
	}

	cur_state = pwrdm_read_next_pwrst(pwrdm);
	if (cur_state == state)
		return ret;

	if (pwrdm_read_pwrst(pwrdm) < PWRDM_POWER_ON) {
		omap2_clkdm_wakeup(pwrdm->pwrdm_clkdms[0]);
		sleep_switch = 1;
		pwrdm_wait_transition(pwrdm);
	}

	ret = pwrdm_set_next_pwrst(pwrdm, state);
	if (ret) {
		printk(KERN_ERR "Unable to set state of powerdomain: %s\n",
		       pwrdm->name);
		goto err;
	}

	if (sleep_switch) {
		omap2_clkdm_allow_idle(pwrdm->pwrdm_clkdms[0]);
		pwrdm_wait_transition(pwrdm);
		pwrdm_state_switch(pwrdm);
	}

err:
	return ret;
}


#ifdef CONFIG_SUSPEND
static void (*saved_idle) (void);
static suspend_state_t suspend_state;

static void omap2_pm_wakeup_on_timer(u32 seconds)
{
	u32 tick_rate, cycles;

	if (!seconds)
		return;

	tick_rate = clk_get_rate(omap_dm_timer_get_fclk(gptimer_wakeup));
	cycles = tick_rate * seconds;
	omap_dm_timer_stop(gptimer_wakeup);
	omap_dm_timer_set_load_start(gptimer_wakeup, 0, 0xffffffff - cycles);

	pr_info("PM: Resume timer in %d secs (%d ticks at %d ticks/sec.)\n",
		seconds, cycles, tick_rate);
}

static int omap3_pm_prepare(void)
{
	saved_idle = pm_idle;
	pm_idle = NULL;
	return 0;
}

static int omap3_pm_suspend(void)
{
	struct power_state *pwrst;
	int state, ret = 0;

	if (wakeup_timer_seconds)
		omap2_pm_wakeup_on_timer(wakeup_timer_seconds);

	/* Read current next_pwrsts */
	list_for_each_entry(pwrst, &pwrst_list, node)
	    pwrst->saved_state = pwrdm_read_next_pwrst(pwrst->pwrdm);
	/* Set ones wanted by suspend */
	list_for_each_entry(pwrst, &pwrst_list, node) {
		if (set_pwrdm_state(pwrst->pwrdm, pwrst->next_state))
			goto restore;
		if (pwrdm_clear_all_prev_pwrst(pwrst->pwrdm))
			goto restore;
	}

	omap_uart_prepare_suspend();

	regset_save_on_suspend = 1;
	omap_sram_idle();
	regset_save_on_suspend = 0;

restore:
	/* Restore next_pwrsts */
	list_for_each_entry(pwrst, &pwrst_list, node) {
		state = pwrdm_read_prev_pwrst(pwrst->pwrdm);
		if (state > pwrst->next_state) {
			printk(KERN_INFO "Powerdomain (%s) didn't enter "
			       "target state %d\n",
			       pwrst->pwrdm->name, pwrst->next_state);
			ret = -1;
		}
		set_pwrdm_state(pwrst->pwrdm, pwrst->saved_state);
	}
	if (ret)
		printk(KERN_ERR "Could not enter target state in pm_suspend\n");
	else
		printk(KERN_INFO "Successfully put all powerdomains "
		       "to target state\n");

	return ret;
}

static int omap3_pm_enter(suspend_state_t unused)
{
	int ret = 0;

	switch (suspend_state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		ret = omap3_pm_suspend();
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void omap3_pm_finish(void)
{
	pm_idle = saved_idle;
}

/* Hooks to enable / disable UART interrupts during suspend */
static int omap3_pm_begin(suspend_state_t state)
{
	suspend_state = state;
	omap_uart_enable_irqs(0);
	return 0;
}

static void omap3_pm_end(void)
{
	suspend_state = PM_SUSPEND_ON;
	omap_uart_enable_irqs(1);
	return;
}

static struct platform_suspend_ops omap_pm_ops = {
	.begin = omap3_pm_begin,
	.end = omap3_pm_end,
	.prepare = omap3_pm_prepare,
	.enter = omap3_pm_enter,
	.finish = omap3_pm_finish,
	.valid = suspend_valid_only_mem,
};
#endif /* CONFIG_SUSPEND */

static void __init prcm_setup_regs(void)
{
	/* XXX Reset all wkdeps. This should be done when initializing
	 * powerdomains. */
	prm_write_mod_reg(0, OMAPW3G_MPU_MOD, PM_WKDEP);
	prm_write_mod_reg(0, OMAPW3G_DSS_MOD, PM_WKDEP);
	prm_write_mod_reg(0, OMAPW3G_CAM_MOD, PM_WKDEP);

	cm_write_mod_reg(OMAPW3G_AUTO_DSS, OMAPW3G_DSS_MOD, CM_AUTOIDLE);
	cm_write_mod_reg(OMAPW3G_AUTO_CAM, OMAPW3G_CAM_MOD, CM_AUTOIDLE);

	/* Disables the HS OTG from wakeup */
	prm_rmw_mod_reg_bits(OMAPW3G_GRPSEL_HSOTGUSB,
			     0,
			     OMAPW3G_CORE_MOD,
			     OMAPW3G_PM_MPUGRPSEL);

	/*
	 * Set all plls to autoidle. This is needed until autoidle is
	 * enabled by clockfw
	 */
	cm_write_mod_reg(1 << OMAPW3G_AUTO_MPU_DPLL_SHIFT,
			 OMAPW3G_MPU_MOD, CM_AUTOIDLE2);

	/* No need to write EN_IO, that is always enabled */
	prm_write_mod_reg(OMAPW3G_EN_IO | OMAPW3G_EN_KBC,
			  OMAPW3G_WKUP_MOD, OMAPW3G_PM_MPUGRPSEL);

	/* TODO Can we validate if the IO EN bug exists in W3G? */
	/* For some reason IO doesn't generate wakeup event even if
	 * it is selected to mpu wakeup goup */
	prm_write_mod_reg(OMAPW3G_IO_EN | OMAPW3G_WKUP_EN,
			  OMAPW3G_OCP_MOD, OMAP2_PRM_IRQENABLE_MPU_OFFSET);


	/* TODO: Fix this properly in framework, simply sets the MPU
	 *       as sleep dependency of these three domains.
	 */
	cm_write_mod_reg(0x10, OMAPW3G_CAM_MOD, OMAP3430_CM_SLEEPDEP);
	cm_write_mod_reg(0x10, OMAPW3G_DSS_MOD, OMAP3430_CM_SLEEPDEP);

	/* Clear any pending 'reset' flags */
	prm_write_mod_reg(0xffffffff, OMAPW3G_MPU_MOD, RM_RSTST);
	prm_write_mod_reg(0xffffffff, OMAPW3G_DSS_MOD, RM_RSTST);
	prm_write_mod_reg(0xffffffff, OMAPW3G_CAM_MOD, RM_RSTST);

	/* Clear any pending PRCM interrupts */
	prm_write_mod_reg(0, OMAPW3G_OCP_MOD, OMAP2_PRM_IRQSTATUS_MPU_OFFSET);

	/* TODO: DMT3 stuff */
	prm_write_mod_reg(0x3, OMAPW3G_APP_MOD, OMAPW3G_PM_MPUGRPSEL);
	prm_write_mod_reg(0x3, OMAPW3G_APP_MOD, PM_WKEN);
	prm_set_mod_reg_bits(0x1, OMAPW3G_APP_MOD, PM_WKDEP);
	prm_set_mod_reg_bits(0x2, OMAPW3G_MPU_MOD, PM_WKDEP);

}

void omap3_pm_off_mode_enable(int enable)
{
	struct power_state *pwrst;
	u32 state;

	if (enable)
		state = PWRDM_POWER_OFF;
	else
		state = PWRDM_POWER_RET;

	list_for_each_entry(pwrst, &pwrst_list, node) {
		pwrst->next_state = state;
		set_pwrdm_state(pwrst->pwrdm, state);
	}
}

int omap3_pm_get_suspend_state(struct powerdomain *pwrdm)
{
	struct power_state *pwrst;

	list_for_each_entry(pwrst, &pwrst_list, node) {
		if (pwrst->pwrdm == pwrdm)
			return pwrst->next_state;
	}
	return -EINVAL;
}

int omap3_pm_set_suspend_state(struct powerdomain *pwrdm, int state)
{
	struct power_state *pwrst;

	list_for_each_entry(pwrst, &pwrst_list, node) {
		if (pwrst->pwrdm == pwrdm) {
			pwrst->next_state = state;
			return 0;
		}
	}
	return -EINVAL;
}

void omap3_set_prm_setup_vc(struct prm_setup_vc *setup_vc)
{
	prm_setup.clksetup = setup_vc->clksetup;
	prm_setup.voltsetup_time1 = setup_vc->voltsetup_time1;
	prm_setup.voltsetup_time2 = setup_vc->voltsetup_time2;
	prm_setup.voltoffset = setup_vc->voltoffset;
	prm_setup.voltsetup2 = setup_vc->voltsetup2;
	prm_setup.vdd0_on = setup_vc->vdd0_on;
	prm_setup.vdd0_onlp = setup_vc->vdd0_onlp;
	prm_setup.vdd0_ret = setup_vc->vdd0_ret;
	prm_setup.vdd0_off = setup_vc->vdd0_off;
	prm_setup.vdd1_on = setup_vc->vdd1_on;
	prm_setup.vdd1_onlp = setup_vc->vdd1_onlp;
	prm_setup.vdd1_ret = setup_vc->vdd1_ret;
	prm_setup.vdd1_off = setup_vc->vdd1_off;
	prm_setup.i2c_slave_ra = setup_vc->i2c_slave_ra;
	prm_setup.vdd_vol_ra = setup_vc->vdd_vol_ra;
	prm_setup.vdd_cmd_ra = setup_vc->vdd_cmd_ra;
	prm_setup.vdd_ch_conf = setup_vc->vdd_ch_conf;
	prm_setup.vdd_i2c_cfg = setup_vc->vdd_i2c_cfg;
}

static int __init pwrdms_setup(struct powerdomain *pwrdm, void *unused)
{
	struct power_state *pwrst;

	if (!pwrdm->pwrsts)
		return 0;

	pwrst = kmalloc(sizeof(struct power_state), GFP_KERNEL);
	if (!pwrst)
		return -ENOMEM;
	pwrst->pwrdm = pwrdm;
	pwrst->next_state = PWRDM_POWER_RET;
	list_add(&pwrst->node, &pwrst_list);

	if (pwrdm_has_hdwr_sar(pwrdm))
		pwrdm_enable_hdwr_sar(pwrdm);

	return set_pwrdm_state(pwrst->pwrdm, pwrst->next_state);
}

/*
 * Enable hw supervised mode for all clockdomains if it's
 * supported. Initiate sleep transition for other clockdomains, if
 * they are not used
 */
static int __init clkdms_setup(struct clockdomain *clkdm, void *unused)
{
	if (clkdm->flags & CLKDM_CAN_ENABLE_AUTO)
		omap2_clkdm_allow_idle(clkdm);
	else if (clkdm->flags & CLKDM_CAN_FORCE_SLEEP &&
		 atomic_read(&clkdm->usecount) == 0)
		omap2_clkdm_sleep(clkdm);
	return 0;
}

void omap_push_sram_idle(void)
{

}

int __init omapw3g_pm_init(void)
{
	struct power_state *pwrst, *tmp;
	int ret;

	printk(KERN_ERR "Power Management for TI OMAPW3G.\n");

	/* XXX prcm_setup_regs needs to be before enabling hw
	 * supervised mode for powerdomains */
	prcm_setup_regs();

	ret = request_irq(INT_W3G_PRCM_MPU_IRQ,
			  (irq_handler_t) prcm_interrupt_handler,
			  IRQF_DISABLED, "prcm", NULL);

	if (ret) {
		printk(KERN_ERR "request_irq failed to register for 0x%x\n",
		       INT_W3G_PRCM_MPU_IRQ);
		goto err1;
	}

	ret = pwrdm_for_each(pwrdms_setup, NULL);
	if (ret) {
		printk(KERN_ERR "Failed to setup powerdomains\n");
		goto err2;
	}

	(void)clkdm_for_each(clkdms_setup, NULL);

	mpu_pwrdm = pwrdm_lookup("mpu_pwrdm");
	if (mpu_pwrdm == NULL) {
		printk(KERN_ERR "Failed to get mpu_pwrdm\n");
		goto err2;
	}

	dss_pwrdm = pwrdm_lookup("dss_pwrdm");
	cam_pwrdm = pwrdm_lookup("cam_pwrdm");
	core_pwrdm = pwrdm_lookup("core_pwrdm");

#ifdef CONFIG_SUSPEND
	suspend_set_ops(&omap_pm_ops);
#endif /* CONFIG_SUSPEND */

	pm_idle = omapw3g_pm_idle;

	/* TODO: need to investigate usage of CPUIDLE framework, like MOPPS? */
	/*      omap3_idle_init(); */

err1:
	return ret;
err2:
	free_irq(INT_W3G_PRCM_MPU_IRQ, NULL);
	list_for_each_entry_safe(pwrst, tmp, &pwrst_list, node) {
		list_del(&pwrst->node);
		kfree(pwrst);
	}
	return ret;
}

static int __init omap3_pm_early_init(void)
{
	pm_dbg_regset_init(1);

	return 0;
}

arch_initcall(omap3_pm_early_init);
