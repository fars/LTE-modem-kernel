/*
 * Copyright (c) 2009,2011 Motorola, Inc, All Rights Reserved.
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
 *
 */

/*
 * Implementation of mailbox short message and virtual register handling.
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/ipc_api.h>
#include <linux/ipc_vr_shortmsg.h>
#include <linux/hw_mbox.h>
#include <linux/mutex.h>
#include <mach/io.h>
#include <asm/mach/map.h>
#include <mach/dmtimer.h>

/*
 * Define the structure to handle the shortmsg data
 */
struct ipc_vr_reg_data {
	/* A pointer to the address of the register or NULL
	   if it is not a real register. */
	volatile u32 *reg_p;
	/* A 1 in a bot position in this mask indicates
	   the AP controls the bit. */
	u32 ap_ctrl_mask;
	/* A 1 in a bit position in this mask indicates
	   the BP controls the bit. */
	u32 bp_ctrl_mask;
	/* Function used to write from the AP for registers
	   which are emulated. */
	void (*ap_write_fcn_p) (enum ipc_vr reg_id, u32 value);
	/* Function used to inform AP that the BP has written to
	   the virtual register. */
	void (*bp_write_fcn_p) (enum ipc_vr reg_id);
};

static void mailbox_vr_bp_write_dmtimer1_gpt_tisr(enum ipc_vr reg_id);
static void mailbox_vr_ap_write_dmtimer1_gpt_tisr(enum ipc_vr reg_id,
						  u32 value);
static void mailbox_vr_ap_write_dmtimer1_gpt_tcrr(enum ipc_vr reg_id,
						  u32 value);
static void mailbox_vr_ap_write_cm_clk(enum ipc_vr reg_id, u32 value);
static void mailbox_vr_ap_wdt(enum ipc_vr reg_id, u32 value);

/*
 * Flag used to determine if a BP panic happend while a virtual timer
 * was being processed by the BP.
 */
static bool bp_processing_timer = false;

/*
 * Flag used to track when the BP panicked.  This is used as opposed
 * to function calls to make the code as fast as possible.
 */
static bool bp_panicked = false;

/*
 *  Table of short message data
 */
const struct ipc_vr_reg_data ipc_vr_data_tb[IPC_VR_LAST] = {
	/* IPC_VR_AP_VERSION_REG */
	{
	 NULL,
	 0xffffffff,		/* The AP controls this register. */
	 0x00000000,
	 NULL,
	 NULL},
	/* IPC_VR_BP_VERSION_REG */
	{
	 NULL,
	 0x00000000,		/* The BP controls this register. */
	 0xffffffff,
	 NULL,
	 NULL},
	/* IPC_VR_DMTIMER1_GPT_TISR */
	{
	 NULL,
	 0xffffffff,
	 0x00000000,
	 mailbox_vr_ap_write_dmtimer1_gpt_tisr,
	 mailbox_vr_bp_write_dmtimer1_gpt_tisr},
	/* IPC_VR_DMTIMER1_GPT_TCRR */
	{
	 NULL,
	 0xffffffff,
	 0x00000000,
	 mailbox_vr_ap_write_dmtimer1_gpt_tcrr,
	 NULL},
	/* IPC_VR_CM_FCLKEN_CORE */
	{
	 NULL,
	 0x0000274E,		/* Clocks owned by the AP */
	 0x00000000,
	 mailbox_vr_ap_write_cm_clk,
	 NULL},
	/* IPC_VR_CM_CLKSEL_CORE */
	{
	 NULL,
	 0xFFFFFFFF,
	 0x00000000,
	 mailbox_vr_ap_write_cm_clk,
	 NULL},
	/* IPC_VR_CM_FCLKEN_AUD */
	{
	 NULL,
	 0x00000002,		/* Clocks owned by the AP */
	 0x00000000,
	 mailbox_vr_ap_write_cm_clk,
	 NULL},
	/* IPC_VR_AP_WDT */
	{
	 NULL,
	 0xffffffff,		/* (tttttsss) : t: timeout;  s: trigger */
	 0x00000000,
	 mailbox_vr_ap_wdt,
	 NULL},
	/* IPC_VR_MP_ADDR_REG */
	{
	 NULL,
	 0x00000000,		/* The AP controls this register. */
	 0xffffffff,
	 NULL,
	 NULL},
	/* IPC_VR_MP_UPDATE_TIMES */
	{
	 NULL,
	 0xffffffff,		/* The AP controls this register. */
	 0x00000000,
	 NULL,
	 NULL},
	/* IPC_VR_MP_USB_STATUS_REG */
	{
	 NULL,
	 0xffffffff,		/* The AP controls this register. */
	 0x00000000,
	 NULL,
	 NULL},

};

#ifdef IPC_DEBUG
#define DEBUG(args...) printk(args)
#else
#define DEBUG(args...)
#endif

/*
 *  Define the table for irq handlers
 */
static void (*ipc_vr_irq_tb[IPC_VR_LAST]) (u32 data);

/*
 * Handle BP isr for dmtime1
 * This function is called by interrupt handler to process the BP interrupt.
 */
static void mailbox_vr_bp_write_dmtimer1_gpt_tisr(enum ipc_vr reg_id)
{
	u32 value;
	unsigned long flags;

	value = ipc_virt_reg_bank[reg_id];

	/* If the timer has expired, call the registered interrupt handler. */
	if (value & 0x00000002) {
		/* Clear the virtual interrupt status register */
		mailbox_vr_write(IPC_VR_DMTIMER1_GPT_TISR, 0x00000002);
		if (ipc_vr_irq_tb[reg_id] != NULL) {
			ipc_vr_irq_tb[reg_id] (value);

			local_irq_save(flags);
			bp_processing_timer = false;
			local_irq_restore(flags);
		}
	}
}

/*
 * Clear virtual register status bit
 * This function is used to clear the virtual interrupt status register.
 */
static void mailbox_vr_ap_write_dmtimer1_gpt_tisr(enum ipc_vr reg_id, u32 value)
{
	/* A write to the tisr when a bit is set, clears the bit.
	 * There is a risk of a race condition with the BP here.
	 * However, the AP should not have requested a new timer value when
	 * this is called from the timer interupt handler.  If this
	 * changes this code will need to be modified.
	 */
	ipc_virt_reg_bank[reg_id] ^= value;
}

/*
 *  Write timer to virtual register.
 * This function is called when it needs to ask BP to start the virtual timer.
 */
static void mailbox_vr_ap_write_dmtimer1_gpt_tcrr(enum ipc_vr reg_id, u32 value)
{
	unsigned long flags;

	local_irq_save(flags);
	bp_processing_timer = true;
	ipc_virt_reg_bank[reg_id] = value;
	local_irq_restore(flags);

	/* Tell the BP that the value has changed,
	   but do not wait for a response. */
	mailbox_short_msg_tx(IPC_VR_SHORT_MSG_VIRT_REG_WRITE, (u16) reg_id);
}

/*
 * Update AP controlled fclk enable bit in the virtual register then notify the
 * BP of the that there is a new setting.
 */
static void mailbox_vr_ap_write_cm_clk(enum ipc_vr reg_id, u32 value)
{
	value &= ipc_vr_data_tb[reg_id].ap_ctrl_mask;
	ipc_virt_reg_bank[reg_id] &= ~(ipc_vr_data_tb[reg_id].ap_ctrl_mask);
	ipc_virt_reg_bank[reg_id] |= value;

	mailbox_short_msg_tx(IPC_VR_SHORT_MSG_VIRT_REG_WRITE, (u16) reg_id);
}

static void mailbox_vr_ap_wdt(enum ipc_vr reg_id, u32 value)
{
	value &= ipc_vr_data_tb[reg_id].ap_ctrl_mask;
	ipc_virt_reg_bank[reg_id] &= ~(ipc_vr_data_tb[reg_id].ap_ctrl_mask);
	ipc_virt_reg_bank[reg_id] |= value;

	mailbox_short_msg_tx(IPC_VR_SHORT_MSG_VIRT_REG_WRITE, (u16) reg_id);
}

void mailbox_short_msg_tx(enum ipc_vr_short_msg msg_type, u16 data)
{
	HW_MBOX_MsgWrite((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),
			 HW_MBOX_ID_7, ((u32) msg_type << 16) | (u32) data);
}
EXPORT_SYMBOL(mailbox_short_msg_tx);

void mailbox_short_msg_tx_and_wait(enum ipc_vr_short_msg msg_type, u16 data)
{
	mailbox_short_msg_tx(msg_type, data);
	mailbox_short_msg_tx_wait();
}
EXPORT_SYMBOL(mailbox_short_msg_tx_and_wait);

int mailbox_short_msg_tx_wait(void)
{
	u32 irq_status;
	u32 retry = 0xffff;

	do {
		HW_MBOX_EventStatus((const u32)IO_ADDRESS(IPC_VR_MBOXES_BASE),
				    HW_MBOX_ID_6,
				    HW_MBOX_U0_ARM,
				    HW_MBOX_INT_NEW_MSG, &irq_status);
	} while ((irq_status) && (--retry) && (!bp_panicked));

	if (retry == 0)
		return -ETIMEDOUT;

	if (bp_panicked)
		return -EBUSY;

	return 0;
}
EXPORT_SYMBOL(mailbox_short_msg_tx_wait);

extern void usb_ipc_exchange_endian16(u16 *data);
extern void usb_ipc_exchange_endian32(u32 *data);

void mailbox_short_msg_vr_write_proc(enum ipc_vr_short_msg msg_type, u16 reg_id)
{
	/* Make sure the AP and BP versions are in sync, if not do nothing. */
	if (ipc_virt_reg_bank[IPC_VR_BP_VERSION_REG] == IPC_VR_AP_VERSION) {
		if (reg_id < sizeof(ipc_vr_data_tb) /
		    sizeof(ipc_vr_data_tb[0])) {

			/* If there is special processing for this
			   register handle it. */
			if (ipc_vr_data_tb[reg_id].bp_write_fcn_p != NULL) {
				ipc_vr_data_tb[reg_id].bp_write_fcn_p
				    ((enum ipc_vr)reg_id);
			} else if (ipc_vr_data_tb[reg_id].reg_p != NULL) {
				*ipc_vr_data_tb[reg_id].reg_p =
				    (((*ipc_vr_data_tb[reg_id].reg_p) &
				      ipc_vr_data_tb[reg_id].ap_ctrl_mask) |
				     (ipc_virt_reg_bank[reg_id] &
				      ipc_vr_data_tb[reg_id].bp_ctrl_mask));
			}
		}
	}
}

int mailbox_vr_read(enum ipc_vr reg_id, u32 *read_p)
{
	/* Make sure the AP and BP versions are in sync,
	   if not do nothing. */
	if ((ipc_virt_reg_bank[IPC_VR_BP_VERSION_REG] == IPC_VR_AP_VERSION) &&
	    (!bp_panicked)){
		if (reg_id < sizeof(ipc_vr_data_tb) /
		    sizeof(ipc_vr_data_tb[0])) {
			if (ipc_vr_data_tb[reg_id].reg_p != NULL) {
				*read_p = *ipc_vr_data_tb[reg_id].reg_p;
				return 0;
			} else {
				*read_p = ipc_virt_reg_bank[reg_id];
			}
		}
		DEBUG("Invalid register.\n");
		return -EINVAL;
	}
	DEBUG("AP is not ready\n");
	return -EINVAL;
}
EXPORT_SYMBOL(mailbox_vr_read);

int mailbox_vr_write(enum ipc_vr reg_id, u32 value)
{
	u32 ap_changed;
	u32 bp_changed;
	u32 ap_mask;
	u32 bp_mask;

	/* Make sure the AP and BP versions are in sync, if not do nothing. */
	if ((ipc_virt_reg_bank[IPC_VR_BP_VERSION_REG] == IPC_VR_AP_VERSION) &&
	    (!bp_panicked)) {
		if (reg_id < sizeof(ipc_vr_data_tb) /
		    sizeof(ipc_vr_data_tb[0])) {
			bp_mask = ipc_vr_data_tb[reg_id].bp_ctrl_mask;
			ap_mask = ipc_vr_data_tb[reg_id].ap_ctrl_mask;
			ap_changed =
			    ((ipc_virt_reg_bank[reg_id] ^ value) & ap_mask);
			bp_changed =
			    ((ipc_virt_reg_bank[reg_id] ^ value) & bp_mask);
			/* If there is special processing for this register,
			   handle it now. */
			if (ipc_vr_data_tb[reg_id].ap_write_fcn_p != NULL) {
				ipc_vr_data_tb[reg_id].ap_write_fcn_p(reg_id,
								      value);
			} else {
				ipc_virt_reg_bank[reg_id] =
				    (value & ap_mask) |
				    (ipc_virt_reg_bank[reg_id] & bp_mask);

				if ((ap_changed) &&
				    (ipc_vr_data_tb[reg_id].reg_p != NULL)) {
					*ipc_vr_data_tb[reg_id].reg_p =
					    ipc_virt_reg_bank[reg_id];
				}
				if (bp_changed) {
					/* Format and send a short message to
					 * the processor which
					 *  owns the regsiter to do the update.
					 */
					mailbox_short_msg_tx_and_wait
					    (IPC_VR_SHORT_MSG_VIRT_REG_WRITE,
					     (u16) reg_id);
				}
			}
			if (bp_panicked) {
				DEBUG("Panic during virtual register write.")
				return -EBUSY;
			}
			return 0;
		}
		DEBUG("Invalid register.\n");
		return -EINVAL;
	}
	DEBUG("AP is not ready\n");
	return -EBUSY;
}
EXPORT_SYMBOL(mailbox_vr_write);

/*
 * Called to inform the virtual register system that the BP has paniced
 * and will no longer be processing VR requests.
 */
void mailbox_vr_bp_panicked(void)
{
	unsigned long flags;

	local_irq_save(flags);
	bp_panicked = true;
	if (bp_processing_timer) {
		mailbox_vr_bp_write_dmtimer1_gpt_tisr(IPC_VR_DMTIMER1_GPT_TISR);
		DEBUG("%s: BP panic while AP timer being processed by BP.\n", __func__);
	}
	local_irq_restore(flags);
}
EXPORT_SYMBOL(mailbox_vr_bp_panicked);

int mailbox_vr_irq_subscribe(void (*handler_fcn_p) (u32 data),
			     enum ipc_vr reg_id)
{
	if (reg_id < IPC_VR_LAST) {
		if (ipc_vr_irq_tb[reg_id] == NULL)
			ipc_vr_irq_tb[reg_id] = handler_fcn_p;
		else
			return -EINVAL;
	} else {
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(mailbox_vr_irq_subscribe);

void mailbox_vr_init(void)
{
	/* Clear out the interrupts. */
	memset(ipc_vr_irq_tb, sizeof(ipc_vr_irq_tb), 0);
	/* Program the version number. */
	ipc_virt_reg_bank[IPC_VR_AP_VERSION_REG] = IPC_VR_AP_VERSION;
}
EXPORT_SYMBOL(mailbox_vr_init);

int mailbox_vr_initialized(void)
{
	return (ipc_virt_reg_bank[IPC_VR_AP_VERSION_REG] ==
		IPC_VR_AP_VERSION) &&
	    (ipc_virt_reg_bank[IPC_VR_BP_VERSION_REG] == IPC_VR_AP_VERSION);
}
EXPORT_SYMBOL(mailbox_vr_initialized);

void mailbox_vr_write_usb_status(bool status)
{
        ipc_virt_reg_bank[IPC_VR_MP_USB_STATUS_REG] = (unsigned int)status;

	mailbox_short_msg_tx(IPC_VR_SHORT_MSG_VIRT_REG_WRITE, (u16) IPC_VR_MP_USB_STATUS_REG);
}
EXPORT_SYMBOL(mailbox_vr_write_usb_status);
