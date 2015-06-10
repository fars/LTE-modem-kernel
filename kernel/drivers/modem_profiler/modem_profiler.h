/*
 * Copyright (C) 2011 Motorola Mobility, Inc.
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this list of conditions
 *   and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, this list of
 *   conditions and the following disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * - Neither the name of Motorola nor the names of its contributors may be used to endorse or
 *   promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#ifndef __MODEM_PROFILER_H__
#define __MODEM_PROFILER_H__

#include <linux/ioctl.h>
#include <linux/types.h>

/* Global Constants */
#define MODEM_PROFILER_DEV_NAME "modem_profiler"

/* enums and typdefs */
enum
{
    MODEM_PROFILER_CONN_RX_OFF_TX_OFF_OPP125,
    MODEM_PROFILER_CONN_RX_ON_TX_ON_AVG_OPP125,
    MODEM_PROFILER_CONN_RX_ON_TX_ON_HIGH_OPP125,
    MODEM_PROFILER_CONN_RX_ON_TX_ON_LOW_OPP125,
    MODEM_PROFILER_CONN_RX_ON_TX_OFF_OPP125,
    MODEM_PROFILER_IDLE_USB_SUSPENDED_OPP125,
    MODEM_PROFILER_IDLE_USB_NOT_SUSPENDED_OPP125,
    MODEM_PROFILER_IDLE_PASSIVE_USB_SUSPENDED_OPP125,
    MODEM_PROFILER_IDLE_PASSIVE_USB_NOT_SUSPENDED_OPP125,
    MODEM_PROFILER_DISABLED_USB_SUSPENDED_OPP125,
    MODEM_PROFILER_DISABLED_USB_NOT_SUSPENDED_OPP125,
    MODEM_PROFILER_CONN_RX_OFF_TX_OFF_OPP100,
    MODEM_PROFILER_CONN_RX_ON_TX_ON_AVG_OPP100,
    MODEM_PROFILER_CONN_RX_ON_TX_ON_HIGH_OPP100,
    MODEM_PROFILER_CONN_RX_ON_TX_ON_LOW_OPP100,
    MODEM_PROFILER_CONN_RX_ON_TX_OFF_OPP100,
    MODEM_PROFILER_IDLE_USB_SUSPENDED_OPP100,
    MODEM_PROFILER_IDLE_USB_NOT_SUSPENDED_OPP100,
    MODEM_PROFILER_IDLE_PASSIVE_USB_SUSPENDED_OPP100,
    MODEM_PROFILER_IDLE_PASSIVE_USB_NOT_SUSPENDED_OPP100,
    MODEM_PROFILER_DISABLED_USB_SUSPENDED_OPP100,
    MODEM_PROFILER_DISABLED_USB_NOT_SUSPENDED_OPP100,

    /* only add new states above */
    MODEM_PROFILER_MODEM_STATES_END,
    MODEM_PROFILER_MODEM_STATES_UNKNOWN
};
typedef unsigned int MODEM_PROFILER_MODEM_STATES;

enum
{
    MODEM_PROFILER_USB_STATE_SUSPEND,
    MODEM_PROFILER_USB_STATE_ACTIVE
};
typedef unsigned char MODEM_PROFILER_USB_STATE;

/* Modem Profiler events to wake the modem profiler wait queue */
#define MODEM_PROFILER_EVENT_NONE            0x00000000
#define MODEM_PROFILER_EVENT_GET_DATA        0x00000001
#define MODEM_PROFILER_EVENT_SEND_DATA       0x00000002

/* IOCTL Commands */
#define MODEM_PROFILER_GET_STATE_TIME_VALUES   (0x00)
#define MODEM_PROFILER_USB_STATUS_CHANGE       (0x01)

#ifdef __KERNEL__
void modem_profiler_set_event (int event);
void modem_profiler_set_usb_state (bool state);
void modem_profiler_usb_data_notify (void);
void modem_profiler_state_update (MODEM_PROFILER_MODEM_STATES state);
#endif

#endif /* __MODEM_PROFILER_H__ */
