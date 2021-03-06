/*
 * linux/include/asm-arm/arch-omap/board-hpm1200l.h
 *
 * Hardware definitions for OMAP3430-based Motorola reference design.
 *
 * Copyright (C) 2009 Motorola, Inc.
 *
 * Derived from include/asm-arm/arch-omap/board-3430sdp.h
 * Initial creation by Syed Mohammed Khasim
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __ASM_ARCH_OMAP_HPM1200L_H
#define __ASM_ARCH_OMAP_HPM1200L_H

#include <linux/init.h>
#include "board-hpm1200l-padconf.h"

extern void __init hpm1200l_usb_init(void);
extern void __init hpm1200l_flash_init(void);
extern void __init hpm1200l_panel_init(void);
extern void __init hpm1200l_sensors_init(void);
extern void __init hpm1200l_spi_init(void);
extern void __init hpm1200l_flash_init(void);
extern void __init hpm1200l_padconf_init(void);
extern void __init hpm1200l_hsmmc_init(void);
extern void __init hpm1200l_gpio_mapping_init(void);
extern void __init hpm1200l_camera_init(void);
extern void __init hpm1200l_mmcprobe_init(void);

#if defined(CONFIG_VIDEO_MT9P012) || defined(CONFIG_VIDEO_MT9P012_MODULE)
extern struct mt9p012_platform_data hpm1200l_mt9p012_platform_data;
#endif
#ifdef CONFIG_VIDEO_OMAP3_HPLENS
extern struct hplens_platform_data hpm1200l_hplens_platform_data;
#endif

#define GPIO_MT9P012_STANDBY		58
#define GPIO_MT9P012_RESET		98
#define GPIO_SILENCE_KEY		100
#define GPIO_SLIDER			177

#define is_cdma_phone() (!strcmp("CDMA", bp_model))
extern char *bp_model;

#endif /*  __ASM_ARCH_OMAP_HPM1200L_H */
