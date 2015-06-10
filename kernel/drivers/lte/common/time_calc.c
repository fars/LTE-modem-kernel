/*
 * Copyright Motorola, Inc. 2007-2008
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
 * @file time_calc.c
 * @brief Time calculation related functions.
 *
 *  Some of these functions are only available on linux
 *
 */


#if defined(__KERNEL__)
#include    <linux/kernel.h>
#include    <linux/module.h>
#include    <linux/init.h>
#include    <linux/version.h>
#include    <linux/jiffies.h>
#include    <time_calc.h>
#endif


#if defined(__KERNEL__)
/**
 * Calulates the delta time in jiffies. It handles timer wrap condition
 *
 * @param unsigned long start_time - The start time in jiffies
 *
 * @return unsigned long, delta time in jiffies
 */
unsigned long delta_time_jiffies(unsigned long start_time)
{
    unsigned long current_time = jiffies;
    unsigned long delta_time = 0;
    
    // check for timer wrap
    if (current_time < start_time)
        delta_time = (0xFFFFFFFF - start_time) + current_time;
    else
        delta_time = current_time - start_time;

    return delta_time;
}
#endif


