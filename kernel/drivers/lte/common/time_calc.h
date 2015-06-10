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
 * Cambridge, MA 02139, USA
 */
/**
 * @file time_calc.h
 * @brief Time calculation related functions.
 *
 *  Some of these functions are only available on linux
 *
 * Last Changed Revision: $Rev: 2955 $
 * Last Changed Date: $LastChangedDate: 2009-03-19 12:40:49 -0500 (Thu, 19 Mar 2009) $
 */
 
#ifndef _TIME_CALC_H_
#define _TIME_CALC_H_

#if defined(__KERNEL__)
unsigned long delta_time_jiffies(unsigned long start_time);
#endif

#endif // _TIME_CALC_H_
