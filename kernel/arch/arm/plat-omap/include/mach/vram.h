/*
 * File: arch/arm/plat-omap/include/mach/vram.h
 *
 * Copyright (C) 2009 Nokia Corporation
 * Author: Tomi Valkeinen <tomi.valkeinen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __OMAPVRAM_H
#define __OMAPVRAM_H

#include <asm/types.h>

extern int omap_vram_add_region(unsigned long paddr, size_t size);
extern int omap_vram_free(unsigned long paddr, size_t size);
extern int omap_vram_alloc(int mtype, size_t size, unsigned long *paddr);
extern int omap_vram_reserve(unsigned long paddr, size_t size);
extern void omap2_set_sdram_vram(u32 size, u32 start);
extern void omap2_set_sram_vram(u32 size, u32 start);

# if defined(CONFIG_FB_OMAP) || defined(CONFIG_FB_OMAP2)
extern void __init omapfb_reserve_sdram(void);
extern unsigned long __init omapfb_reserve_sram(unsigned long sram_pstart,
						unsigned long sram_vstart,
						unsigned long sram_size,
						unsigned long pstart_avail,
						unsigned long size_avail);
# else
#  define omapfb_reserve_sdram() (0)
#  define omapfb_reserve_sram(sram_pstart,  \
			     sram_vstart,  \
			     sram_size,    \
			     pstart_avail, \
			     size_avail) ((unsigned long)(0))
# endif
#endif
