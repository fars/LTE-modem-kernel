/*
 * Copyright Motorola, Inc. 2009
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 *  Cambridge, MA 02139, USA
 */
 
/**
 * @File  net_driver_config.c
 * @brief LTE network driver configuration (moid) handling.
 *
 * This module contains the configuration (moid) setting handling for the LTE system.
 *
 * Last Changed Revision: $Rev$
 * Last Changed Date: $LastChangedDate$
 *
 **************************************************************
 */
#include    <linux/kernel.h>
#include    <linux/module.h>
#include    <linux/init.h>
#include    <linux/version.h>
#include    <linux/slab.h>
#include    <linux/sched.h>
#include    <linux/proc_fs.h>
#include    <linux/netdevice.h>
#include    <linux/skbuff.h>

#include    "net_driver_config.h"
#include    "net_driver_dev.h"




