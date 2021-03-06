#ifndef __ARCH_ASM_MACH_OMAP2_PRCM_COMMON_H
#define __ARCH_ASM_MACH_OMAP2_PRCM_COMMON_H

/*
 * OMAP2/3 PRCM base and module definitions
 *
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Written by Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/* Module offsets from both CM_BASE & PRM_BASE */

/*
 * Offsets that are the same on 24xx and 34xx
 *
 * Technically, in terms of the TRM, OCP_MOD is 34xx only; PLL_MOD is
 * CCR_MOD on 3430; and GFX_MOD only exists < 3430ES2.
 */
#define OCP_MOD						0x000
#define MPU_MOD						0x100
#define CORE_MOD					0x200
#define GFX_MOD						0x300
#define WKUP_MOD					0x400
#define PLL_MOD						0x500


/* Chip-specific module offsets */
#define OMAP24XX_GR_MOD					OCP_MOD
#define OMAP24XX_DSP_MOD				0x800

#define OMAPW3G_DSP_MOD                                 0x000
#define OMAPW3G_OCP_MOD                                 0x800
#define OMAPW3G_MPU_MOD                                 0x900
#define OMAPW3G_CORE_MOD                                0xA00
#define OMAPW3G_WKUP_MOD                                0xC00
#define OMAPW3G_CCR_MOD                                 0xD00
#define OMAPW3G_DSS_MOD                                 0xE00
#define OMAPW3G_CAM_MOD                                 0xF00
#define OMAPW3G_APP_MOD		                        0x1000
#define OMAPW3G_EMU_MOD				        0x1100
#define OMAPW3G_GR_MOD				        0x1200
#define OMAPW3G_WCDMA_MOD                               0x1300
#define OMAPW3G_R5R6_MOD                                0x1400
#define OMAPW3G_AUD_MOD                                 0x1500

#define OMAP2430_MDM_MOD				0xc00

/* IVA2 module is < base on 3430 */
#define OMAP3430_IVA2_MOD				-0x800
#define OMAP3430ES2_SGX_MOD				GFX_MOD
#define OMAP3430_CCR_MOD				PLL_MOD
#define OMAP3430_DSS_MOD				0x600
#define OMAP3430_CAM_MOD				0x700
#define OMAP3430_PER_MOD				0x800
#define OMAP3430_EMU_MOD				0x900
#define OMAP3430_GR_MOD					0xa00
#define OMAP3430_NEON_MOD				0xb00
#define OMAP3430ES2_USBHOST_MOD				0xc00


/* 24XX register bits shared between CM & PRM registers */

/* CM_FCLKEN1_CORE, CM_ICLKEN1_CORE, PM_WKEN1_CORE shared bits */
#define OMAP2420_EN_MMC_SHIFT				26
#define OMAP2420_EN_MMC					(1 << 26)
#define OMAP24XX_EN_UART2_SHIFT				22
#define OMAP24XX_EN_UART2				(1 << 22)
#define OMAP24XX_EN_UART1_SHIFT				21
#define OMAP24XX_EN_UART1				(1 << 21)
#define OMAP24XX_EN_MCSPI2_SHIFT			18
#define OMAP24XX_EN_MCSPI2				(1 << 18)
#define OMAP24XX_EN_MCSPI1_SHIFT			17
#define OMAP24XX_EN_MCSPI1				(1 << 17)
#define OMAP24XX_EN_MCBSP2_SHIFT			16
#define OMAP24XX_EN_MCBSP2				(1 << 16)
#define OMAP24XX_EN_MCBSP1_SHIFT			15
#define OMAP24XX_EN_MCBSP1				(1 << 15)
#define OMAP24XX_EN_GPT12_SHIFT				14
#define OMAP24XX_EN_GPT12				(1 << 14)
#define OMAP24XX_EN_GPT11_SHIFT				13
#define OMAP24XX_EN_GPT11				(1 << 13)
#define OMAP24XX_EN_GPT10_SHIFT				12
#define OMAP24XX_EN_GPT10				(1 << 12)
#define OMAP24XX_EN_GPT9_SHIFT				11
#define OMAP24XX_EN_GPT9				(1 << 11)
#define OMAP24XX_EN_GPT8_SHIFT				10
#define OMAP24XX_EN_GPT8				(1 << 10)
#define OMAP24XX_EN_GPT7_SHIFT				9
#define OMAP24XX_EN_GPT7				(1 << 9)
#define OMAP24XX_EN_GPT6_SHIFT				8
#define OMAP24XX_EN_GPT6				(1 << 8)
#define OMAP24XX_EN_GPT5_SHIFT				7
#define OMAP24XX_EN_GPT5				(1 << 7)
#define OMAP24XX_EN_GPT4_SHIFT				6
#define OMAP24XX_EN_GPT4				(1 << 6)
#define OMAP24XX_EN_GPT3_SHIFT				5
#define OMAP24XX_EN_GPT3				(1 << 5)
#define OMAP24XX_EN_GPT2_SHIFT				4
#define OMAP24XX_EN_GPT2				(1 << 4)
#define OMAP2420_EN_VLYNQ_SHIFT				3
#define OMAP2420_EN_VLYNQ				(1 << 3)

/* CM_FCLKEN2_CORE, CM_ICLKEN2_CORE, PM_WKEN2_CORE shared bits */
#define OMAP2430_EN_GPIO5_SHIFT				10
#define OMAP2430_EN_GPIO5				(1 << 10)
#define OMAP2430_EN_MCSPI3_SHIFT			9
#define OMAP2430_EN_MCSPI3				(1 << 9)
#define OMAP2430_EN_MMCHS2_SHIFT			8
#define OMAP2430_EN_MMCHS2				(1 << 8)
#define OMAP2430_EN_MMCHS1_SHIFT			7
#define OMAP2430_EN_MMCHS1				(1 << 7)
#define OMAP24XX_EN_UART3_SHIFT				2
#define OMAP24XX_EN_UART3				(1 << 2)
#define OMAP24XX_EN_USB_SHIFT				0
#define OMAP24XX_EN_USB					(1 << 0)

/* CM_ICLKEN2_CORE, PM_WKEN2_CORE shared bits */
#define OMAP2430_EN_MDM_INTC_SHIFT			11
#define OMAP2430_EN_MDM_INTC				(1 << 11)
#define OMAP2430_EN_USBHS_SHIFT				6
#define OMAP2430_EN_USBHS				(1 << 6)

/* CM_IDLEST1_CORE, PM_WKST1_CORE shared bits */
#define OMAP2420_ST_MMC_SHIFT				26
#define OMAP2420_ST_MMC_MASK				(1 << 26)
#define OMAP24XX_ST_UART2_SHIFT				22
#define OMAP24XX_ST_UART2_MASK				(1 << 22)
#define OMAP24XX_ST_UART1_SHIFT				21
#define OMAP24XX_ST_UART1_MASK				(1 << 21)
#define OMAP24XX_ST_MCSPI2_SHIFT			18
#define OMAP24XX_ST_MCSPI2_MASK				(1 << 18)
#define OMAP24XX_ST_MCSPI1_SHIFT			17
#define OMAP24XX_ST_MCSPI1_MASK				(1 << 17)
#define OMAP24XX_ST_GPT12_SHIFT				14
#define OMAP24XX_ST_GPT12_MASK				(1 << 14)
#define OMAP24XX_ST_GPT11_SHIFT				13
#define OMAP24XX_ST_GPT11_MASK				(1 << 13)
#define OMAP24XX_ST_GPT10_SHIFT				12
#define OMAP24XX_ST_GPT10_MASK				(1 << 12)
#define OMAP24XX_ST_GPT9_SHIFT				11
#define OMAP24XX_ST_GPT9_MASK				(1 << 11)
#define OMAP24XX_ST_GPT8_SHIFT				10
#define OMAP24XX_ST_GPT8_MASK				(1 << 10)
#define OMAP24XX_ST_GPT7_SHIFT				9
#define OMAP24XX_ST_GPT7_MASK				(1 << 9)
#define OMAP24XX_ST_GPT6_SHIFT				8
#define OMAP24XX_ST_GPT6_MASK				(1 << 8)
#define OMAP24XX_ST_GPT5_SHIFT				7
#define OMAP24XX_ST_GPT5_MASK				(1 << 7)
#define OMAP24XX_ST_GPT4_SHIFT				6
#define OMAP24XX_ST_GPT4_MASK				(1 << 6)
#define OMAP24XX_ST_GPT3_SHIFT				5
#define OMAP24XX_ST_GPT3_MASK				(1 << 5)
#define OMAP24XX_ST_GPT2_SHIFT				4
#define OMAP24XX_ST_GPT2_MASK				(1 << 4)
#define OMAP2420_ST_VLYNQ_SHIFT				3
#define OMAP2420_ST_VLYNQ_MASK				(1 << 3)

/* CM_IDLEST2_CORE, PM_WKST2_CORE shared bits */
#define OMAP2430_ST_MDM_INTC_SHIFT			11
#define OMAP2430_ST_MDM_INTC_MASK			(1 << 11)
#define OMAP2430_ST_GPIO5_SHIFT				10
#define OMAP2430_ST_GPIO5_MASK				(1 << 10)
#define OMAP2430_ST_MCSPI3_SHIFT			9
#define OMAP2430_ST_MCSPI3_MASK				(1 << 9)
#define OMAP2430_ST_MMCHS2_SHIFT			8
#define OMAP2430_ST_MMCHS2_MASK				(1 << 8)
#define OMAP2430_ST_MMCHS1_SHIFT			7
#define OMAP2430_ST_MMCHS1_MASK				(1 << 7)
#define OMAP2430_ST_USBHS_SHIFT				6
#define OMAP2430_ST_USBHS_MASK				(1 << 6)
#define OMAP24XX_ST_UART3_SHIFT				2
#define OMAP24XX_ST_UART3_MASK				(1 << 2)
#define OMAP24XX_ST_USB_SHIFT				0
#define OMAP24XX_ST_USB_MASK				(1 << 0)

/* CM_FCLKEN_WKUP, CM_ICLKEN_WKUP, PM_WKEN_WKUP shared bits */
#define OMAP24XX_EN_GPIOS_SHIFT				2
#define OMAP24XX_EN_GPIOS				(1 << 2)
#define OMAP24XX_EN_GPT1_SHIFT				0
#define OMAP24XX_EN_GPT1				(1 << 0)

/* PM_WKST_WKUP, CM_IDLEST_WKUP shared bits */
#define OMAP24XX_ST_GPIOS_SHIFT				(1 << 2)
#define OMAP24XX_ST_GPIOS_MASK				2
#define OMAP24XX_ST_GPT1_SHIFT				(1 << 0)
#define OMAP24XX_ST_GPT1_MASK				0

/* CM_IDLEST_MDM and PM_WKST_MDM shared bits */
#define OMAP2430_ST_MDM_SHIFT				(1 << 0)

/* W3G register bits shared between CM & PRM registers */
/* CM_REVISION, PRM_REVISION shared bits */
#define OMAPW3G_REV_SHIFT				0
#define OMAPW3G_REV_MASK				(0xff << 0)

/* CM_SYSCONFIG, PRM_SYSCONFIG shared bits */
#define OMAPW3G_AUTOIDLE				(1 << 0)

/* CM_FCLKEN_CORE, CM_ICLKEN_CORE, PM_WKEN_CORE shared bits */
#define OMAPW3G_EN_RCI					(1 << 19)
#define OMAPW3G_EN_RCI_SHIFT				19
#define OMAPW3G_EN_USIMOCP				(1 << 10)
#define OMAPW3G_EN_USIMOCP_SHIFT			10
#define OMAPW3G_EN_MMCSD2				(1 << 9)
#define OMAPW3G_EN_MMCSD2_SHIFT				9
#define OMAPW3G_EN_MMCSD1				(1 << 8)
#define OMAPW3G_EN_MMCSD1_SHIFT				8
#define OMAPW3G_EN_SPI1					(1 << 6)
#define OMAPW3G_EN_SPI1_SHIFT				6
#define OMAPW3G_EN_FSOTGUSB				(1 << 5)
#define OMAPW3G_EN_FSOTGUSB_SHIFT			5
#define OMAPW3G_EN_UART3				(1 << 3)
#define OMAPW3G_EN_UART3_SHIFT				3
#define OMAPW3G_EN_UART2				(1 << 2)
#define OMAPW3G_EN_UART2_SHIFT				2
#define OMAPW3G_EN_UART1				(1 << 1)
#define OMAPW3G_EN_UART1_SHIFT				1
#define OMAPW3G_EN_MCBSP2				(1 << 0)
#define OMAPW3G_EN_MCBSP2_SHIFT				0

/* CM_ICLKEN1_CORE, PM_WKEN1_CORE shared bits */
#define OMAPW3G_EN_SDIOSLAVE				(1 << 18)
#define OMAPW3G_EN_SDIOSLAVE_SHIFT			18
#define OMAPW3G_EN_HSOTGUSB				(1 << 4)
#define OMAPW3G_EN_HSOTGUSB_SHIFT			4

/* CM_FCLKEN_CORE, CM_ICLKEN_CORE shared bits */
#define OMAPW3G_EN_2GL1T				(1 << 11)
#define OMAPW3G_EN_2GL1T_SHIFT				11

/* PM_WKST1_CORE, CM_IDLEST1_CORE shared bits */
#define OMAPW3G_ST_RCI					(1 << 19)
#define OMAPW3G_ST_RCI_SHIFT				19
#define OMAPW3G_ST_SDIOSLAVE				(1 << 18)
#define OMAPW3G_ST_SDIOSLAVE_SHIFT			18
#define OMAPW3G_ST_I2C					(1 << 12)
#define OMAPW3G_ST_I2C_SHIFT				12
#define OMAPW3G_ST_2GL1T				(1 << 11)
#define OMAPW3G_ST_2GL1T_SHIFT				11
#define OMAPW3G_ST_USIMOCP				(1 << 10)
#define OMAPW3G_ST_USIMOCP_SHIFT			10
#define OMAPW3G_ST_MMCSD2				(1 << 9)
#define OMAPW3G_ST_MMCSD2_SHIFT				9
#define OMAPW3G_ST_MMCSD1				(1 << 8)
#define OMAPW3G_ST_MMCSD1_SHIFT				8
#define OMAPW3G_ST_SPI1					(1 << 6)
#define OMAPW3G_ST_SPI1_SHIFT				6
#define OMAPW3G_ST_FSOTGUSB				(1 << 5)
#define OMAPW3G_ST_FSOTGUSB_SHIFT			5
#define OMAPW3G_ST_HSOTGUSB				(1 << 4)
#define OMAPW3G_ST_HSOTGUSB_SHIFT			4
#define OMAPW3G_ST_UART3				(1 << 3)
#define OMAPW3G_ST_UART3_SHIFT				3
#define OMAPW3G_ST_UART2				(1 << 2)
#define OMAPW3G_ST_UART2_SHIFT				2
#define OMAPW3G_ST_UART1				(1 << 0)
#define OMAPW3G_ST_UART1_SHIFT				1
#define OMAPW3G_ST_MCBSP2				(1 << 0)
#define OMAPW3G_ST_MCBSP2_SHIFT				0

/* CM_FCLKEN_WKUP, CM_ICLKEN_WKUP, PM_WKEN_WKUP shared bits */
#define OMAPW3G_EN_GPIO1				(1 << 3)
#define OMAPW3G_EN_GPIO1_SHIFT				3
#define OMAPW3G_EN_GPT1				(1 << 0)
#define OMAPW3G_EN_GPT1_SHIFT			0

/* CM_FCLKEN_WKUP, PM_WKEN_WKUP shared bits */
#define OMAPW3G_EN_SR2					(1 << 14)
#define OMAPW3G_EN_SR2_SHIFT				14
#define OMAPW3G_EN_SR1					(1 << 13)
#define OMAPW3G_EN_SR1_SHIFT				13

/* CM_ICLKEN_WKUP, PM_WKEN_WKUP shared bits */
#define OMAPW3G_EN_KBC					(1 << 10)
#define OMAPW3G_EN_KBC_SHIFT				10

/* CM_FCLKEN_WKUP, CM_ICLKEN_WKUP shared bits */
#define OMAPW3G_EN_WDT2					(1 << 5)
#define OMAPW3G_EN_WDT2_SHIFT				5

/* CM_IDLEST_WKUP, PM_WKST_WKUP shared bits */
#define OMAPW3G_ST_SR2					(1 << 14)
#define OMAPW3G_ST_SR2_SHIFT				14
#define OMAPW3G_ST_SR1					(1 << 13)
#define OMAPW3G_ST_SR1_SHIFT				13
#define OMAPW3G_ST_2GDSM				(1 << 11)
#define OMAPW3G_ST_2GDSM_SHIFT				11
#define OMAPW3G_ST_KBC					(1 << 10)
#define OMAPW3G_ST_KBC_SHIFT				10
#define OMAPW3G_ST_GPIO1				(1 << 3)
#define OMAPW3G_ST_GPIO1_SHIFT				3
#define OMAPW3G_ST_GPT1				(1 << 0)
#define OMAPW3G_ST_GPT1_SHIFT			0


/* 3430 register bits shared between CM & PRM registers */

/* CM_REVISION, PRM_REVISION shared bits */
#define OMAP3430_REV_SHIFT				0
#define OMAP3430_REV_MASK				(0xff << 0)

/* CM_SYSCONFIG, PRM_SYSCONFIG shared bits */
#define OMAP3430_AUTOIDLE				(1 << 0)

/* CM_FCLKEN1_CORE, CM_ICLKEN1_CORE, PM_WKEN1_CORE shared bits */
#define OMAP3430_EN_MMC2				(1 << 25)
#define OMAP3430_EN_MMC2_SHIFT				25
#define OMAP3430_EN_MMC1				(1 << 24)
#define OMAP3430_EN_MMC1_SHIFT				24
#define OMAP3430_EN_MCSPI4				(1 << 21)
#define OMAP3430_EN_MCSPI4_SHIFT			21
#define OMAP3430_EN_MCSPI3				(1 << 20)
#define OMAP3430_EN_MCSPI3_SHIFT			20
#define OMAP3430_EN_MCSPI2				(1 << 19)
#define OMAP3430_EN_MCSPI2_SHIFT			19
#define OMAP3430_EN_MCSPI1				(1 << 18)
#define OMAP3430_EN_MCSPI1_SHIFT			18
#define OMAP3430_EN_I2C3				(1 << 17)
#define OMAP3430_EN_I2C3_SHIFT				17
#define OMAP3430_EN_I2C2				(1 << 16)
#define OMAP3430_EN_I2C2_SHIFT				16
#define OMAP3430_EN_I2C1				(1 << 15)
#define OMAP3430_EN_I2C1_SHIFT				15
#define OMAP3430_EN_UART2				(1 << 14)
#define OMAP3430_EN_UART2_SHIFT				14
#define OMAP3430_EN_UART1				(1 << 13)
#define OMAP3430_EN_UART1_SHIFT				13
#define OMAP3430_EN_GPT11				(1 << 12)
#define OMAP3430_EN_GPT11_SHIFT				12
#define OMAP3430_EN_GPT10				(1 << 11)
#define OMAP3430_EN_GPT10_SHIFT				11
#define OMAP3430_EN_MCBSP5				(1 << 10)
#define OMAP3430_EN_MCBSP5_SHIFT			10
#define OMAP3430_EN_MCBSP1				(1 << 9)
#define OMAP3430_EN_MCBSP1_SHIFT			9
#define OMAP3430_EN_FSHOSTUSB				(1 << 5)
#define OMAP3430_EN_FSHOSTUSB_SHIFT			5
#define OMAP3430_EN_D2D					(1 << 3)
#define OMAP3430_EN_D2D_SHIFT				3

/* CM_ICLKEN1_CORE, PM_WKEN1_CORE shared bits */
#define OMAP3430_EN_HSOTGUSB				(1 << 4)
#define OMAP3430_EN_HSOTGUSB_SHIFT				4

/* PM_WKST1_CORE, CM_IDLEST1_CORE shared bits */
#define OMAP3430_ST_MMC2_SHIFT				25
#define OMAP3430_ST_MMC2_MASK				(1 << 25)
#define OMAP3430_ST_MMC1_SHIFT				24
#define OMAP3430_ST_MMC1_MASK				(1 << 24)
#define OMAP3430_ST_MCSPI4_SHIFT			21
#define OMAP3430_ST_MCSPI4_MASK				(1 << 21)
#define OMAP3430_ST_MCSPI3_SHIFT			20
#define OMAP3430_ST_MCSPI3_MASK				(1 << 20)
#define OMAP3430_ST_MCSPI2_SHIFT			19
#define OMAP3430_ST_MCSPI2_MASK				(1 << 19)
#define OMAP3430_ST_MCSPI1_SHIFT			18
#define OMAP3430_ST_MCSPI1_MASK				(1 << 18)
#define OMAP3430_ST_I2C3_SHIFT				17
#define OMAP3430_ST_I2C3_MASK				(1 << 17)
#define OMAP3430_ST_I2C2_SHIFT				16
#define OMAP3430_ST_I2C2_MASK				(1 << 16)
#define OMAP3430_ST_I2C1_SHIFT				15
#define OMAP3430_ST_I2C1_MASK				(1 << 15)
#define OMAP3430_ST_UART2_SHIFT				14
#define OMAP3430_ST_UART2_MASK				(1 << 14)
#define OMAP3430_ST_UART1_SHIFT				13
#define OMAP3430_ST_UART1_MASK				(1 << 13)
#define OMAP3430_ST_GPT11_SHIFT				12
#define OMAP3430_ST_GPT11_MASK				(1 << 12)
#define OMAP3430_ST_GPT10_SHIFT				11
#define OMAP3430_ST_GPT10_MASK				(1 << 11)
#define OMAP3430_ST_MCBSP5_SHIFT			10
#define OMAP3430_ST_MCBSP5_MASK				(1 << 10)
#define OMAP3430_ST_MCBSP1_SHIFT			9
#define OMAP3430_ST_MCBSP1_MASK				(1 << 9)
#define OMAP3430ES1_ST_FSHOSTUSB_SHIFT			5
#define OMAP3430ES1_ST_FSHOSTUSB_MASK			(1 << 5)
#define OMAP3430ES1_ST_HSOTGUSB_SHIFT			4
#define OMAP3430ES1_ST_HSOTGUSB_MASK			(1 << 4)
#define OMAP3430ES2_ST_HSOTGUSB_IDLE_SHIFT		5
#define OMAP3430ES2_ST_HSOTGUSB_IDLE_MASK		(1 << 5)
#define OMAP3430ES2_ST_HSOTGUSB_STDBY_SHIFT		4
#define OMAP3430ES2_ST_HSOTGUSB_STDBY_MASK		(1 << 4)
#define OMAP3430_ST_D2D_SHIFT				3
#define OMAP3430_ST_D2D_MASK				(1 << 3)

/* CM_FCLKEN_WKUP, CM_ICLKEN_WKUP, PM_WKEN_WKUP shared bits */
#define OMAP3430_EN_GPIO1				(1 << 3)
#define OMAP3430_EN_GPIO1_SHIFT				3
#define OMAP3430_EN_GPT12				(1 << 1)
#define OMAP3430_EN_GPT12_SHIFT				1
#define OMAP3430_EN_GPT1				(1 << 0)
#define OMAP3430_EN_GPT1_SHIFT				0

/* CM_FCLKEN_WKUP, PM_WKEN_WKUP shared bits */
#define OMAP3430_EN_SR2					(1 << 7)
#define OMAP3430_EN_SR2_SHIFT				7
#define OMAP3430_EN_SR1					(1 << 6)
#define OMAP3430_EN_SR1_SHIFT				6

/* CM_ICLKEN_WKUP, PM_WKEN_WKUP shared bits */
#define OMAP3430_EN_GPT12				(1 << 1)
#define OMAP3430_EN_GPT12_SHIFT				1

/* CM_IDLEST_WKUP, PM_WKST_WKUP shared bits */
#define OMAP3430_ST_SR2_SHIFT				7
#define OMAP3430_ST_SR2_MASK				(1 << 7)
#define OMAP3430_ST_SR1_SHIFT				6
#define OMAP3430_ST_SR1_MASK				(1 << 6)
#define OMAP3430_ST_GPIO1_SHIFT				3
#define OMAP3430_ST_GPIO1_MASK				(1 << 3)
#define OMAP3430_ST_GPT12_SHIFT				1
#define OMAP3430_ST_GPT12_MASK				(1 << 1)
#define OMAP3430_ST_GPT1_SHIFT				0
#define OMAP3430_ST_GPT1_MASK				(1 << 0)

/*
 * CM_SLEEPDEP_GFX, CM_SLEEPDEP_DSS, CM_SLEEPDEP_CAM,
 * CM_SLEEPDEP_PER, PM_WKDEP_IVA2, PM_WKDEP_GFX,
 * PM_WKDEP_DSS, PM_WKDEP_CAM, PM_WKDEP_PER, PM_WKDEP_NEON shared bits
 */
#define OMAP3430_EN_MPU					(1 << 1)
#define OMAP3430_EN_MPU_SHIFT				1

/* CM_FCLKEN_PER, CM_ICLKEN_PER, PM_WKEN_PER shared bits */
#define OMAP3430_EN_GPIO6				(1 << 17)
#define OMAP3430_EN_GPIO6_SHIFT				17
#define OMAP3430_EN_GPIO5				(1 << 16)
#define OMAP3430_EN_GPIO5_SHIFT				16
#define OMAP3430_EN_GPIO4				(1 << 15)
#define OMAP3430_EN_GPIO4_SHIFT				15
#define OMAP3430_EN_GPIO3				(1 << 14)
#define OMAP3430_EN_GPIO3_SHIFT				14
#define OMAP3430_EN_GPIO2				(1 << 13)
#define OMAP3430_EN_GPIO2_SHIFT				13
#define OMAP3430_EN_UART3				(1 << 11)
#define OMAP3430_EN_UART3_SHIFT				11
#define OMAP3430_EN_GPT9				(1 << 10)
#define OMAP3430_EN_GPT9_SHIFT				10
#define OMAP3430_EN_GPT8				(1 << 9)
#define OMAP3430_EN_GPT8_SHIFT				9
#define OMAP3430_EN_GPT7				(1 << 8)
#define OMAP3430_EN_GPT7_SHIFT				8
#define OMAP3430_EN_GPT6				(1 << 7)
#define OMAP3430_EN_GPT6_SHIFT				7
#define OMAP3430_EN_GPT5				(1 << 6)
#define OMAP3430_EN_GPT5_SHIFT				6
#define OMAP3430_EN_GPT4				(1 << 5)
#define OMAP3430_EN_GPT4_SHIFT				5
#define OMAP3430_EN_GPT3				(1 << 4)
#define OMAP3430_EN_GPT3_SHIFT				4
#define OMAP3430_EN_GPT2				(1 << 3)
#define OMAP3430_EN_GPT2_SHIFT				3

/* CM_FCLKEN_PER, CM_ICLKEN_PER, PM_WKEN_PER, PM_WKST_PER shared bits */
/* XXX Possible TI documentation bug: should the PM_WKST_PER EN_* bits
 * be ST_* bits instead? */
#define OMAP3430_EN_MCBSP4				(1 << 2)
#define OMAP3430_EN_MCBSP4_SHIFT			2
#define OMAP3430_EN_MCBSP3				(1 << 1)
#define OMAP3430_EN_MCBSP3_SHIFT			1
#define OMAP3430_EN_MCBSP2				(1 << 0)
#define OMAP3430_EN_MCBSP2_SHIFT			0

/* CM_IDLEST_PER, PM_WKST_PER shared bits */
#define OMAP3430_ST_GPIO6_SHIFT				17
#define OMAP3430_ST_GPIO6_MASK				(1 << 17)
#define OMAP3430_ST_GPIO5_SHIFT				16
#define OMAP3430_ST_GPIO5_MASK				(1 << 16)
#define OMAP3430_ST_GPIO4_SHIFT				15
#define OMAP3430_ST_GPIO4_MASK				(1 << 15)
#define OMAP3430_ST_GPIO3_SHIFT				14
#define OMAP3430_ST_GPIO3_MASK				(1 << 14)
#define OMAP3430_ST_GPIO2_SHIFT				13
#define OMAP3430_ST_GPIO2_MASK				(1 << 13)
#define OMAP3430_ST_UART3_SHIFT				11
#define OMAP3430_ST_UART3_MASK				(1 << 11)
#define OMAP3430_ST_GPT9_SHIFT				10
#define OMAP3430_ST_GPT9_MASK				(1 << 10)
#define OMAP3430_ST_GPT8_SHIFT				9
#define OMAP3430_ST_GPT8_MASK				(1 << 9)
#define OMAP3430_ST_GPT7_SHIFT				8
#define OMAP3430_ST_GPT7_MASK				(1 << 8)
#define OMAP3430_ST_GPT6_SHIFT				7
#define OMAP3430_ST_GPT6_MASK				(1 << 7)
#define OMAP3430_ST_GPT5_SHIFT				6
#define OMAP3430_ST_GPT5_MASK				(1 << 6)
#define OMAP3430_ST_GPT4_SHIFT				5
#define OMAP3430_ST_GPT4_MASK				(1 << 5)
#define OMAP3430_ST_GPT3_SHIFT				4
#define OMAP3430_ST_GPT3_MASK				(1 << 4)
#define OMAP3430_ST_GPT2_SHIFT				3
#define OMAP3430_ST_GPT2_MASK				(1 << 3)

/* CM_SLEEPDEP_PER, PM_WKDEP_IVA2, PM_WKDEP_MPU, PM_WKDEP_PER shared bits */
#define OMAP3430_EN_CORE_SHIFT				0
#define OMAP3430_EN_CORE_MASK				(1 << 0)

#endif

