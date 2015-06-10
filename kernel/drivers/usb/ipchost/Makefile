#
# Makefile for the IPC drivers.
#
# Copyright (C) 2010 Motorola, Inc
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# Changelog:
# Date               Author           Comment
# -----------------------------------------------------------------------------
# 15-jan-2010       Motorola         added 4ghif ul buddy memory manager
# 17-may-2010       Motorola         Test app for 4G HIF IPC low level driver on AP side
# 25-jun-2010       Motorola         Added 4G HIF IPC BP stub

obj-$(CONFIG_MAILBOX_IPC)	+= usb-ipc.o
usb-ipc-y :=	ipc_panic.o ipc_log.o ipc_api.o ipc_dma.o ipc_vr_shortmsg.o
usb-ipc-$(CONFIG_MACH_W3G_LTE_DATACARD) += ipc_4ghif.o ipc_4ghif_mm.o ipc_4ghif_stub_bp_loopback.o

obj-$(CONFIG_MAILBOX_IPC_TEST)   += usb-ipc-test.o
usb-ipc-test-objs :=	ipc_api_test.o

obj-$(CONFIG_IPC_4GHIF_TEST)   += lld-hif-test.o
lld-hif-test-objs :=	ipc_4ghif_test.o



