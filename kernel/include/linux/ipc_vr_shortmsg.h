/*
 * Copyright (C) 2009, 2011 Motorola, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

/*
 * This is the short message handler. Short messages are messages sent
 * between the AP and BP.
 */
#ifndef __IPC_VR_SHORTMSG_H__
#define __IPC_VR_SHORTMSG_H__

#include <linux/interrupt.h>

#define IPC_VR_MBOXES_BASE 0x47022000
#define IPC_VR_SHARED_MEM (W3G_IPC_SHARED_MEMORY_VIRT + 0x000F8000)

/*
 * Define the virtual address of shortmsg share memory
 */
#define ipc_virt_reg_bank \
        ((volatile u32 *) (IPC_VR_SHARED_MEM + (0x00bff97c-0x00bf8000)))

/*!
 * @brief Define the AP initial version
 */
#define IPC_VR_AP_VERSION 0x00000001

enum ipc_vr_status {
	IPC_VR_STATUS_ERROR,
	IPC_VR_STATUS_OK,
	IPC_VR_STATUS_REG_UNDEF,
	IPC_VR_STATUS_LAST
};

enum ipc_vr_short_msg {
	IPC_VR_SHORT_MSG_PANIC,
	IPC_VR_SHORT_MSG_VIRT_REG_WRITE,
	IPC_VR_SHORT_MSG_LAST            /* This must always the the last entry. */
};

enum ipc_vr {
	IPC_VR_AP_VERSION_REG,         /* Programmed by the AP with its version barker. */
	IPC_VR_BP_VERSION_REG,         /* Programmed by the BP with its version barker. */
	IPC_VR_DMTIMER1_GPT_TISR,
	IPC_VR_DMTIMER1_GPT_TCRR,
	IPC_VR_CM_FCLKEN_CORE,
	IPC_VR_CM_CLKSEL_CORE,
	IPC_VR_CM_FCLKEN_AUD,
	IPC_VR_AP_WDT,
        IPC_VR_MP_ADDR_REG,            /* initialized by the BP with a physical address */
	IPC_VR_MP_UPDATE_TIMES,
        IPC_VR_MP_USB_STATUS_REG,      
	IPC_VR_LAST                    /* This must always the the last entry. */
};

/*
 * This function is used to initialize virtual AP_VERSION register.
 *
 */
void mailbox_vr_init(void);

/*
 * This function is used to read from share memory.
 */
int mailbox_vr_read(enum ipc_vr reg_id, u32 *read_p);

/*
 * This function is used for AP to write to share memory.
 */
int mailbox_vr_write(enum ipc_vr reg_id, u32 data);

/*
 * Must be called upon an BP panic to inform the VR code of a BP panic.
 */
void mailbox_vr_bp_panicked(void);

/*
 * This function is used check if AP version is initialized.
 */
int mailbox_vr_initialized(void);

/*
 * This function is used subscribe the interrupt handler.
 */
int mailbox_vr_irq_subscribe(void (*handler_fcn_p)(u32 data), enum ipc_vr reg_id);

/*
 * This function is the interrupt handler of short message from BP.
 */
void mailbox_short_msg_vr_write_proc(enum ipc_vr_short_msg msg_type, u16 reg_id);

/*
 * Used to send a short message to the BP.
 */
void mailbox_short_msg_tx(enum ipc_vr_short_msg msg_type, u16 data);

/*
 * Send message to BP and wait for the status register clean up.
 */
void mailbox_short_msg_tx_and_wait(enum ipc_vr_short_msg  msg_type, u16 data);

/*
 * This function is the interrupt handler of short message from BP.
 */
int mailbox_short_msg_tx_wait(void);

/*
 * This function is the handler to set USB status on the BP for power state
 */
void mailbox_vr_write_usb_status(bool status);

#endif
