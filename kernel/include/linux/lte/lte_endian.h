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

#ifndef _LTE_ENDIAN_H_
#define _LTE_ENDIAN_H_

#define	cpu_to_le32(a)		(a)
#define cpu_to_le16(a)		(a)
#define le16_to_cpu(a)		(a)
#define	le32_to_cpu(a)		(a)
#define cpu_to_be32(a)		(((a)&0xff000000)>>24 | ((a)&0x00ff0000)>>8 | ((a)&0x0000ff00)<< 8 | ((a)&0x000000ff)<< 24)
#define be32_to_cpu(a)		cpu_to_be32(a)
#define cpu_to_be16(a)		(((a)&0xff00)>>8 | ((a)&0x00ff)<<8)
#define be16_to_cpu(a)		cpu_to_be16(a)
#ifdef LTE93
#define cpu_to_target16(a)      cpu_to_be16(a)
#define cpu_to_target32(a)      cpu_to_be32(a)
#define target_to_cpu16(a)      be16_to_cpu(a)
#define target_to_cpu32(a)      be32_to_cpu(a)
#else
#define cpu_to_target16(a)      cpu_to_le16(a)
#define cpu_to_target32(a)      cpu_to_le32(a)
#define target_to_cpu16(a)      le16_to_cpu(a)
#define target_to_cpu32(a)      le32_to_cpu(a)
#endif

#endif // _LTE_ENDIAN_H_

