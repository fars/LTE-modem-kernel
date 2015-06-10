#ifndef __ARCH_ARM_MACH_OMAP2_PRM_REGBITS_W3G_H
#define __ARCH_ARM_MACH_OMAP2_PRM_REGBITS_W3G_H

/*
 * Wrigley Power/Reset Management register bits
 *
 * Copyright (C) 2009 Motorola, Inc.
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Written by Scott DeBates
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "prm.h"

/* Shared register bits */

/* PM_WKDEP shared bits */
#define OMAPW3G_WKDEP_EN_WCDMA_SHIFT		9
#define OMAPW3G_WKDEP_EN_WCDMA_MASK		(1 << 9)
#define OMAPW3G_WKDEP_EN_AUD_SHIFT		8
#define OMAPW3G_WKDEP_EN_AUD_MASK			(1 << 8)
#define OMAPW3G_WKDEP_EN_DSS_SHIFT		7
#define OMAPW3G_WKDEP_EN_DSS_MASK			(1 << 7)
#define OMAPW3G_WKDEP_EN_WKUP_SHIFT		6
#define OMAPW3G_WKDEP_EN_WKUP_MASK		(1 << 6)
#define OMAPW3G_WKDEP_EN_DSP_SHIFT		5
#define OMAPW3G_WKDEP_EN_DSP_MASK			(1 << 5)
#define OMAPW3G_WKDEP_EN_MPU_SHIFT		4
#define OMAPW3G_WKDEP_EN_MPU_MASK			(1 << 4)
#define OMAPW3G_WKDEP_EN_APP_SHIFT		2
#define OMAPW3G_WKDEP_EN_APP_MASK			(1 << 2)
#define OMAPW3G_WKDEP_EN_CORE_SHIFT		0
#define OMAPW3G_WKDEP_EN_CORE_MASK		(1 << 0)

/* PRM_VC_CMD_VAL_0, PRM_VC_CMD_VAL_1 shared bits */
#define OMAPW3G_ON_SHIFT				24
#define OMAPW3G_ON_MASK 				(0xff << 24)
#define OMAPW3G_ONLP_SHIFT				16
#define OMAPW3G_ONLP_MASK				(0xff << 16)
#define OMAPW3G_RET_SHIFT				8
#define OMAPW3G_RET_MASK				(0xff << 8)

/* PRM_VP1_CONFIG, PRM_VP2_CONFIG shared bits */
#define OMAPW3G_ERROROFFSET_SHIFT			24
#define OMAPW3G_ERROROFFSET_MASK			(0xff << 24)
#define OMAPW3G_ERRORGAIN_SHIFT 			16
#define OMAPW3G_ERRORGAIN_MASK				(0xff << 16)
#define OMAPW3G_INITVOLTAGE_SHIFT			8
#define OMAPW3G_INITVOLTAGE_MASK			(0xff << 8)
#define OMAPW3G_TIMEOUTEN				(1 << 3)
#define OMAPW3G_INITVDD 		 		(1 << 2)
#define OMAPW3G_FORCEUPDATE				(1 << 1)
#define OMAPW3G_VPENABLE				(1 << 0)

/* PRM_VP1_VSTEPMIN, PRM_VP2_VSTEPMIN shared bits */
#define OMAPW3G_SMPSWAITTIMEMIN_SHIFT			8
#define OMAPW3G_SMPSWAITTIMEMIN_MASK			(0xffff << 8)
#define OMAPW3G_VSTEPMIN_SHIFT				0
#define OMAPW3G_VSTEPMIN_MASK				(0xff << 0)

/* PRM_VP1_VSTEPMAX, PRM_VP2_VSTEPMAX shared bits */
#define OMAPW3G_SMPSWAITTIMEMAX_SHIFT			8
#define OMAPW3G_SMPSWAITTIMEMAX_MASK			(0xffff << 8)
#define OMAPW3G_VSTEPMAX_SHIFT				0
#define OMAPW3G_VSTEPMAX_MASK				(0xff << 0)

/* PRM_VP1_VLIMITTO, PRM_VP2_VLIMITTO shared bits */
#define OMAPW3G_VDDMAX_SHIFT				24
#define OMAPW3G_VDDMAX_MASK				(0xff << 24)
#define OMAPW3G_VDDMIN_SHIFT				16
#define OMAPW3G_VDDMIN_MASK				(0xff << 16)
#define OMAPW3G_TIMEOUT_SHIFT				0
#define OMAPW3G_TIMEOUT_MASK				(0xffff << 0)

/* PRM_VP1_VOLTAGE, PRM_VP2_VOLTAGE shared bits */
#define OMAPW3G_VPVOLTAGE_SHIFT 			0
#define OMAPW3G_VPVOLTAGE_MASK				(0xff << 0)

/* PRM_VP1_STATUS, PRM_VP2_STATUS shared bits */
#define OMAPW3G_VPINIDLE				(1 << 0)

/* PRM_IRQSTATUS_MPU shared bits */
#define OMAPW3G_WKUP_ST                                (1 << 0)

/* PRM_IRQENABLE_MPU shared bits */
#define OMAPW3G_WKUP_EN                                        (1 << 0)



/* RM_RSTST_DSP, RM_RSTST_MPU, RM_RSTST_APP,
 * RM_RSTST_WCDMA,RM_RSTST_R5R6, RM_PW_RSTST_CAM
 * RM_RSTST_AUD, RM_RSTST_EMU, RM_RSTST_DSS
 * RM_RSTST_CORE, RM_RSTST_CUST_EFUSE
 * shared bits
 */
#define OMAPW3G_DOMAINWKUP_RST  			(1 << 2)
#define OMAPW3G_GLOBALWARM_RST   			(1 << 1)
#define OMAPW3G_GLOBALCOLD_RST  	   		(1 << 0)

/* PM_MPUGRPSEL_CORE, PM_DSPGRPSEL1_CORE shared bits */
#define OMAPW3G_GRPSEL_RCI				(1 << 19)
#define OMAPW3G_GRPSEL_SDIOSLAVE       			(1 << 18)
#define OMAPW3G_GRPSEL_I2C				(1 << 12)
#define OMAPW3G_GRPSEL_USIMOCP				(1 << 10)
#define OMAPW3G_GRPSEL_MMCSD2				(1 << 9)
#define OMAPW3G_GRPSEL_MMCSD1				(1 << 8)
#define OMAPW3G_GRPSEL_SPI1				(1 << 6)
#define OMAPW3G_GRPSEL_FSOTGUSB				(1 << 5)
#define OMAPW3G_GRPSEL_HSOTGUSB 			(1 << 4)
#define OMAPW3G_GRPSEL_UART3				(1 << 3)
#define OMAPW3G_GRPSEL_UART2				(1 << 2)
#define OMAPW3G_GRPSEL_UART1    			(1 << 1)
#define OMAPW3G_GRPSEL_MCBSP2				(1 << 0)

/* PM_MPUGRPSEL_APP, PM_DSPGRPSEL_APP shared bits */
#define OMAPW3G_GRPSEL_SPI2				(1 << 1)
#define OMAPW3G_GRPSEL_GPT3	       		(1 << 0)

/* PM_MPUGRPSEL_WKUP, PM_DSPGRPSEL_WKUP shared bits */
#define OMAPW3G_GRPSEL_IO				(1 << 15)
#define OMAPW3G_GRPSEL_SR2				(1 << 14)
#define OMAPW3G_GRPSEL_SR1				(1 << 13)
#define OMAPW3G_GRPSEL_2GDSM				(1 << 11)
#define OMAPW3G_GRPSEL_KBC				(1 << 10)
#define OMAPW3G_GRPSEL_GPIO1				(1 << 3)
#define OMAPW3G_GRPSEL_DMTIMER1				(1 << 0)

/* PM_MPUGRPSEL_AUD, PM_DSPGRPSEL_AUD shared bits */
#define OMAPW3G_GRPSEL_MCBSP1				(1 << 3)
#define OMAPW3G_GRPSEL_DMTIMER2				(1 << 2)
#define OMAPW3G_GRPSEL_GPIO3				(1 << 1)
#define OMAPW3G_GRPSEL_GPIO2				(1 << 0)

/* PM_WKDEP_DSP, PM_WKDEP_MPU, PM_WKDEP_DSS,
 * PM_WKDEP_CAM, PM_WKDEP_AUD  shared bits
 */
#define OMAPW3G_EN_WKUP_SHIFT				6
#define OMAPW3G_EN_WKUP_MASK				(1 << 6)

/* PM_PWSTCTRL_DSP, PM_PWSTCTRL_MPU, PM_PWSTCTRL_APP,
 * PM_PWSTCTRL_WCDMA,PM_PWSTCTRL_R5R6, PM_PW_PWSTCTRL_CAM
 * PM_PWSTCTRL_AUD, PM_PWSTCTRL_EMU, PM_PWSTRCTRL_DSS
 * RM_PWSTCTRL_CUST_EFUSE  shared bits
 */
#define OMAPW3G_L1CACHEONSTATE_SHIFT	        	16
#define OMAPW3G_L1CACHEONSTATE_MASK			(0x3 << 16)
#define OMAPW3G_L1CACHERETSTATE 			(1 << 8)
#define OMAPW3G_MEMORYCHANGE				(1 << 3)
#define OMAPW3G_LOGICRETSTATE   			(1 << 2)
#define OMAPW3G_POWERSTATE_SHIFT			0
#define OMAPW3G_POWERSTATE_MASK 			(0x3 << 0)

/* PM_PWSTST_DSP, PM_PWSTST_MPU, PM_PWSTST_CORE,
 * PM_PWSTST_APP, PM_PWSTST_WCDMA, PM_PWSTST_R5R6,
 * PM_PWSTST_AUD, PM_PWSTST_CUST_EFUSE  PM_PWSTST_CAM
 * shared bits
 */
#define OMAPW3G_LOGICSTATEST				(1 << 2)
#define OMAPW3G_POWERSTATEST_SHIFT      		0
#define OMAPW3G_POWERSTATEST_MASK	        	(0x3 << 0)

/* PM_PWSTST_MPU, PM_PWSTST_CORE,PM_PWSTST_CUST_EFUSE
 * PM_PWSTST_APP, PM_PWSTST_WCDMA, PM_PWSTST_R5R6,
 * PM_PWSTST_AUD, PM_PWSTST_DSS  shared bits
 */
#define OMAPW3G_INTRANSTION             	       	(1 << 20)

/* PM_PREPWSTST_DSP, PM_PREPWSTST_MPU, PM_PREPWSTST_CORE,
 * PM_PREPWSTST_APP, PM_PREPWSTST_WCDMA, PM_PREPWSTST_R5R6,
 * PM_PREPWSTST_AUD, PM_PREPWSTST_CUST_EFUS  shared bits
 */
#define OMAPW3G_LASTLOGICSTATEENTERED			(1 << 2)
#define OMAPW3G_LASTPOWERSTATEENTERED_SHIFT    		0
#define OMAPW3G_LASTPOWERSTATEENTERED_MASK     		(0x3 << 0)

/* PRM_IRQSTATUS_MCP PRM_IRQSTATUS_DSP shared bits */
#define OMAPW3G_VC_TIMEOUTERR_ST			(1 << 24)
#define OMAPW3G_VC_RAERR_ST				(1 << 23)
#define OMAPW3G_VC_SAERR_ST				(1 << 22)
#define OMAPW3G_VP2_TRANXDONE_ST			(1 << 21)
#define OMAPW3G_VP2_EQVALUE_ST				(1 << 20)
#define OMAPW3G_VP2_NOSMPSACK_ST			(1 << 19)
#define OMAPW3G_VP2_MAXVDD_ST				(1 << 18)
#define OMAPW3G_VP2_MINVDD_ST				(1 << 17)
#define OMAPW3G_VP2_OPPCHANGEDONE_ST			(1 << 16)
#define OMAPW3G_VP1_TRANXDONE_ST			(1 << 15)
#define OMAPW3G_VP1_EQVALUE_ST				(1 << 14)
#define OMAPW3G_VP1_NOSMPSACK_ST			(1 << 13)
#define OMAPW3G_VP1_MAXVDD_ST				(1 << 12)
#define OMAPW3G_VP1_MINVDD_ST				(1 << 11)
#define OMAPW3G_VP1_OPPCHANGEDONE_ST			(1 << 10)
#define OMAPW3G_IO_ST					(1 << 9)
#define OMAPW3G_DPLL4_RECAL_ST          		(1 << 8)
#define OMAPW3G_DPLL4_RECAL_ST_SHIFT            	8
#define OMAPW3G_DPLL3_RECAL_ST				(1 << 7)
#define OMAPW3G_DPLL3_RECAL_ST_SHIFT			7
#define OMAPW3G_DPLL2_RECAL_ST				(1 << 6)
#define OMAPW3G_DPLL2_RECAL_ST_SHIFT			6
#define OMAPW3G_DPLL1_RECAL_ST				(1 << 5)
#define OMAPW3G_DPLL1_RECAL_ST_SHIFT			5
#define OMAPW3G_TRANSITION_ST				(1 << 4)
#define OMAPW3G_USB_WKUP_ST				(1 << 1)
#define OMAPW3G_GRP_WKUP_ST				(1 << 0)

/* PRM_IRQENABLE_MCP PRM_IRQENABLE_DSP shared bits */
#define OMAPW3G_VC_TOERR_EN	        		(1 << 24)
#define OMAPW3G_VC_RAERR_EN				(1 << 23)
#define OMAPW3G_VC_SAERR_EN				(1 << 22)
#define OMAPW3G_VP2_TRANXDONE_EN			(1 << 21)
#define OMAPW3G_VP2_EQVALUE_EN				(1 << 20)
#define OMAPW3G_VP2_NOSMPSACK_EN			(1 << 19)
#define OMAPW3G_VP2_MAXVDD_EN				(1 << 18)
#define OMAPW3G_VP2_MINVDD_EN				(1 << 17)
#define OMAPW3G_VP2_OPPCHANGEDONE_EN			(1 << 16)
#define OMAPW3G_VP1_TRANXDONE_EN			(1 << 15)
#define OMAPW3G_VP1_EQVALUE_EN				(1 << 14)
#define OMAPW3G_VP1_NOSMPSACK_EN			(1 << 13)
#define OMAPW3G_VP1_MAXVDD_EN				(1 << 12)
#define OMAPW3G_VP1_MINVDD_EN				(1 << 11)
#define OMAPW3G_VP1_OPPCHANGEDONE_EN			(1 << 10)
#define OMAPW3G_IO_EN					(1 << 9)
#define OMAPW3G_DPLL4_RECAL_EN          		(1 << 8)
#define OMAPW3G_DPLL4_RECAL_EN_SHIFT            	8
#define OMAPW3G_DPLL3_RECAL_EN				(1 << 7)
#define OMAPW3G_DPLL3_RECAL_EN_SHIFT			7
#define OMAPW3G_DPLL2_RECAL_EN				(1 << 6)
#define OMAPW3G_DPLL2_RECAL_EN_SHIFT			6
#define OMAPW3G_DPLL1_RECAL_EN				(1 << 5)
#define OMAPW3G_DPLL1_RECAL_EN_SHIFT			5
#define OMAPW3G_TRANSITION_EN				(1 << 4)
#define OMAPW3G_USB_WKUP_EN				(1 << 1)
#define OMAPW3G_GRP_WKUP_EN				(1 << 0)



/* Bits specific to each register */

/* RM_RSTCTRL_DSP */
#define OMAPW3G_RST_DSP 				(1 << 0)

/* RM_RSTST_DSP specific bits */
#define OMAPW3G_EMULATION_RST   			(1 << 11)
#define OMAPW3G_DSP_SW_RST				(1 << 8)

/* PM_WKDEP_DSP specific bits */
#define OMAPW3G_WKDEP_DSP_EN_WCDMA		       	(1 << 9)
#define OMAPW3G_WKDEP_DSP_EN_AUD		       	(1 << 8)
#define OMAPW3G_WKDEP_DSP_EN_DSS		       	(1 << 7)
#define OMAPW3G_WKDEP_DSP_EN_MPU    			(1 << 4)
#define OMAPW3G_WKDEP_DSP_EN_APP	       		(1 << 2)
#define OMAPW3G_WKDEP_DSP_EN_CORE	       		(1 << 0)

/* PM_PWSTCTRL_DSP specific bits */
#define OMAPW3G_EMUBANKONSTATE_SHIFT       		24
#define OMAPW3G_EMUBANKONSTATE_MASK			(0x3 << 24)
#define OMAPW3G_FLATBANK2ONSTATE_SHIFT     		22
#define OMAPW3G_FLATBANK2ONSTATE_MASK      		(0x3 << 22)
#define OMAPW3G_FLATBANK1ONSTATE_SHIFT  		20
#define OMAPW3G_FLATBANK1ONSTATE_MASK   		(0x3 << 20)
#define OMAPW3G_L2CACHEONSTATE_SHIFT       		18
#define OMAPW3G_L2CACHEONSTATE_MASK			(0x3 << 18)
#define OMAPW3G_EMUBANKRETSTATE 			(1 << 12)
#define OMAPW3G_FLATBANK2RETSTATE			(1 << 11)
#define OMAPW3G_FLATBANK1RETSTATE       		(1 << 10)
#define OMAPW3G_L2CACHERETSTATE 			(1 << 9)

/* PM_PWSTST_DSP specific bits */
#define OMAPW3G_DSP_INTRANSTION             	       	(1 << 22)
#define OMAPW3G_EMUBANKSTATEST_SHIFT			12
#define OMAPW3G_EMUBANKSTATEST_MASK			(0x3 << 12)
#define OMAPW3G_FLATBANK2STATEST_SHIFT			10
#define OMAPW3G_FLATBANK2STATEST_MASK			(0x3 << 10)
#define OMAPW3G_FLATBANK1STATEST_SHIFT  	       	8
#define OMAPW3G_FLATBANK1STATEST_MASK   		(0x3 << 8)
#define OMAPW3G_L2CACHESTATEST_SHIFT			6
#define OMAPW3G_L2CACHESTATEST_MASK			(0x3 << 6)
#define OMAPW3G_DSP_L1CACHESTATEST_SHIFT    		4
#define OMAPW3G_DSP_L1CACHESTATEST_MASK      		(0x3 << 4)

/* PM_PREPWSTST_DSP specific bits */
#define OMAPW3G_LASTEMUBANKSTATEENTERED_SHIFT	       	12
#define OMAPW3G_LASTEMUBANKSTATEENTERED_MASK		(0x3 << 12)
#define OMAPW3G_LASTFLATBANK2STATEENTERED_SHIFT		10
#define OMAPW3G_LASTFLATBANK2STATEENTERED_MASK 		(0x3 << 10)
#define OMAPW3G_LASTFLATBANK1STATEENTERED_SHIFT 	8
#define OMAPW3G_LASTFLATBANK1STATEENTERED_MASK		(0x3 << 8)
#define OMAPW3G_LASTL2CACHESTATEENTERED_SHIFT		6
#define OMAPW3G_LASTL2CACHESTATEENTERED_MASK	       	(0x3 << 6)
#define OMAPW3G_DSP_LASTL1CACHESTATEENTERED_SHIFT   	4
#define OMAPW3G_DSP_LASTL1CACHESTATEENTERED_MASK   	(0x3 << 4)

/* PRM_REVISION specific bits */
#define OMAPW3G_REV_SHIFT                		0
#define OMAPW3G_REV_MASK                 	     	(0xff << 0)

/* PRM_IRQSTATUS_MCP specific bits */
#define OMAPW3G_VC_TIMEOUTERR_ST				(1 << 24)
#define OMAPW3G_VC_RAERR_ST					(1 << 23)
#define OMAPW3G_VC_SAERR_ST					(1 << 22)
#define OMAPW3G_VP2_TRANXDONE_ST				(1 << 21)
#define OMAPW3G_VP2_EQVALUE_ST					(1 << 20)
#define OMAPW3G_VP2_NOSMPSACK_ST				(1 << 19)
#define OMAPW3G_VP2_MAXVDD_ST					(1 << 18)
#define OMAPW3G_VP2_MINVDD_ST					(1 << 17)
#define OMAPW3G_VP2_OPPCHANGEDONE_ST				(1 << 16)
#define OMAPW3G_VP1_TRANXDONE_ST				(1 << 15)
#define OMAPW3G_VP1_EQVALUE_ST					(1 << 14)
#define OMAPW3G_VP1_NOSMPSACK_ST				(1 << 13)
#define OMAPW3G_VP1_MAXVDD_ST					(1 << 12)
#define OMAPW3G_VP1_MINVDD_ST					(1 << 11)
#define OMAPW3G_VP1_OPPCHANGEDONE_ST				(1 << 10)
#define OMAPW3G_IO_ST						(1 << 9)
#define OMAPW3G_PERIPH_DPLL_ST					(1 << 8)
#define OMAPW3G_PERIPH_DPLL_ST_SHIFT				8
#define OMAPW3G_CORE_DPLL_ST					(1 << 7)
#define OMAPW3G_CORE_DPLL_ST_SHIFT				7
#define OMAPW3G_DSP_DPLL_ST					(1 << 6)
#define OMAPW3G_DSP_DPLL_ST_SHIFT				6
#define OMAPW3G_MPU_DPLL_ST					(1 << 5)
#define OMAPW3G_MPU_DPLL_ST_SHIFT				5
#define OMAPW3G_TRANSITION_ST					(1 << 4)
#define OMAPW3G_EVGENOFF_ST			        	(1 << 3)
#define OMAPW3G_EVGENON_ST			        	(1 << 2)
#define OMAPW3G_USB_WKUP_ST			        	(1 << 1)
#define OMAPW3G_GRP_WKUP_ST			        	(1 << 0)

/* PRM_IRQENABLE_MCP specific bits */
#define OMAPW3G_VC_TIMEOUTERR_EN				(1 << 24)
#define OMAPW3G_VC_RAERR_EN					(1 << 23)
#define OMAPW3G_VC_SAERR_EN					(1 << 22)
#define OMAPW3G_VP2_TRANXDONE_EN				(1 << 21)
#define OMAPW3G_VP2_EQVALUE_EN					(1 << 20)
#define OMAPW3G_VP2_NOSMPSACK_EN				(1 << 19)
#define OMAPW3G_VP2_MAXVDD_EN					(1 << 18)
#define OMAPW3G_VP2_MINVDD_EN					(1 << 17)
#define OMAPW3G_VP2_OPPCHANGEDONE_EN				(1 << 16)
#define OMAPW3G_VP1_TRANXDONE_EN				(1 << 15)
#define OMAPW3G_VP1_EQVALUE_EN					(1 << 14)
#define OMAPW3G_VP1_NOSMPSACK_EN				(1 << 13)
#define OMAPW3G_VP1_MAXVDD_EN					(1 << 12)
#define OMAPW3G_VP1_MINVDD_EN					(1 << 11)
#define OMAPW3G_VP1_OPPCHANGEDONE_EN				(1 << 10)
#define OMAPW3G_IO_EN						(1 << 9)
#define OMAPW3G_PERIPH_DPLL_RECAL_EN				(1 << 8)
#define OMAPW3G_PERIPH_DPLL_RECAL_EN_SHIFT			8
#define OMAPW3G_CORE_DPLL_RECAL_EN				(1 << 7)
#define OMAPW3G_CORE_DPLL_RECAL_EN_SHIFT			7
#define OMAPW3G_DSP_DPLL_RECAL_EN				(1 << 6)
#define OMAPW3G_DSP_DPLL_RECAL_EN_SHIFT			6
#define OMAPW3G_MPU_DPLL_RECAL_EN				(1 << 5)
#define OMAPW3G_MPU_DPLL_RECAL_EN_SHIFT			5
#define OMAPW3G_TRANSITION_EN					(1 << 4)
#define OMAPW3G_EVGENOFF_EN			        	(1 << 3)
#define OMAPW3G_EVGENON_EN			        	(1 << 2)
#define OMAPW3G_USB_WKUP_EN			        	(1 << 1)
#define OMAPW3G_GRP_WKUP_EN			        	(1 << 0)

/* PM_RSTST_MPU specific bits */
#define OMAPW3G_EMULATION_MPU_RST			(1 << 11)
#define OMAPW3G_ICECRUSHER_RST  			(1 << 10)

/* PM_WKDEP_MPU specific bits */
#define OMAPW3G_WKDEP_MPU_EN_WCMDA_SHIFT               	9
#define OMAPW3G_WKDEP_MPU_EN_WCDMA_MASK                	(1 << 9)
#define OMAPW3G_WKDEP_MPU_EN_AUD_SHIFT                 	8
#define OMAPW3G_WKDEP_MPU_EN_AUD_MASK                  	(1 << 8)
#define OMAPW3G_WKDEP_MPU_EN_DSS_SHIFT                 	7
#define OMAPW3G_WKDEP_MPU_EN_DSS_MASK                  	(1 << 7)
#define OMAPW3G_WKDEP_MPU_EN_WKUP_SHIFT                	5
#define OMAPW3G_WKDEP_MPU_EN_WKUP_MASK                 	(1 << 5)
#define OMAPW3G_WKDEP_MPU_EN_DSP_SHIFT                 	5
#define OMAPW3G_WKDEP_MPU_EN_DSP_MASK                  	(1 << 5)
#define OMAPW3G_WKDEP_MPU_EN_APP_SHIFT                 	2
#define OMAPW3G_WKDEP_MPU_EN_APP_MASK                  	(1 << 2)
#define OMAPW3G_WKDEP_MPU_EN_CORE_SHIFT                	0
#define OMAPW3G_WKDEP_MPU_EN_CORE_MASK                 	(1 << 0)

/* PM_EVGENCTRL_MPU */
#define OMAPW3G_OFFLOADMODE_SHIFT			3
#define OMAPW3G_OFFLOADMODE_MASK			(0x3 << 3)
#define OMAPW3G_ONLOADMODE_SHIFT			1
#define OMAPW3G_ONLOADMODE_MASK	        		(0x3 << 1)
#define OMAPW3G_ENABLE					(1 << 0)

/* PM_EVGENONTIM_MPU */
#define OMAPW3G_ONTIMEVAL_SHIFT 		       	0
#define OMAPW3G_ONTIMEVAL_MASK				(0xffffffff << 0)

/* PM_EVGENOFFTIM_MPU */
#define OMAPW3G_OFFTIMEVAL_SHIFT			0
#define OMAPW3G_OFFTIMEVAL_MASK 			(0xffffffff << 0)

/* PM_PWSTCTRL_MPU specific bits */

/* PM_PWSTST_MPU specific bits */
#define OMAPW3G_MPU_L1CACHESTATEST_SHIFT	       	6
#define OMAPW3G_MPU_L1CACHESTATEST_MASK			(0x3 << 6)

/* PM_PREPWSTST_MPU specific bits */
#define OMAPW3G_MPU_LASTL1CACHESTATEENTERED_SHIFT   	6
#define OMAPW3G_MPU_LASTL1CACHESTATEENTERED_MASK   	(0x3 << 6)

/* RM_RSTCTRL_CORE */
#define OMAPW3G_RM_RSTCTRL_CORE_2GL1T   		(1 << 0)

/* RM_RSTST_CORE specific bits */
#define OMAPW3G_RM_RSTST_CORE_DOMAIN2GL1T_RST		(1 << 3)

/* PM_WKEN_CORE specific bits */
#define OMAPW3G_EN_RCI	         			(1 << 19)
#define OMAPW3G_EN_SDIOSLAVE       			(1 << 18)
#define OMAPW3G_EN_I2C     				(1 << 12)
#define OMAPW3G_EN_USIMOCP	         	       	(1 << 10)
#define OMAPW3G_EN_MMCSD2	        	       	(1 << 9)
#define OMAPW3G_EN_MMCSD1  				(1 << 8)
#define OMAPW3G_EN_SPI1    				(1 << 6)
#define OMAPW3G_EN_FSOTGUSB     			(1 << 5)
#define OMAPW3G_EN_HSOTGUSB        			(1 << 4)
#define OMAPW3G_EN_UART3   				(1 << 3)
#define OMAPW3G_EN_UART2    				(1 << 2)
#define OMAPW3G_EN_UART1           			(1 << 1)
#define OMAPW3G_EN_MCBSP2  				(1 << 0)

/* PM_MPUGRPSEL_CORE specific bits */

/* PM_DSPGRPSEL_CORE specific bits */

/* PM_WKST_CORE specific bits */
#define OMAPW3G_ST_RCI	         			(1 << 19)
#define OMAPW3G_ST_SDIOSLAVE       			(1 << 18)
#define OMAPW3G_ST_I2C     				(1 << 12)
#define OMAPW3G_ST_USIMOCP	               		(1 << 10)
#define OMAPW3G_ST_MMCSD2	               		(1 << 9)
#define OMAPW3G_ST_MMCSD1  				(1 << 8)
#define OMAPW3G_ST_SPI1    				(1 << 6)
#define OMAPW3G_ST_FSOTGUSB     	       		(1 << 5)
#define OMAPW3G_ST_HSOTGUSB        			(1 << 4)
#define OMAPW3G_ST_UART3   				(1 << 3)
#define OMAPW3G_ST_UART2    				(1 << 2)
#define OMAPW3G_ST_UART1           			(1 << 1)
#define OMAPW3G_ST_MCBSP2  				(1 << 0)

/* PM_WKDEP_CORE specific bits */
#define OMAPW3G_CORE_L4_EN_APP_L3	      		(1 << 18)
#define OMAPW3G_CORE_L4_EN_CORE_L3       		(1 << 16)

/* PM_PWSTCTRL_CORE specific bits */

/* PM_PWSTST_CORE specific bits */

/* PM_PREPWSTST_CORE specific bits */

/* PM_PWSTCTRL_CORE specific bits */

/* RM_RSTST_CUST_EFUSE */

/* PM_PWSTCTRL_CUST_EFUSE specific bits */

/* PM_PWSTST_CUST_EFUSE specific bits */

/* PM_PREPWSTST_CUST_EFUSE specific bits */

/* RM_RSTCTRL_WKUP specific bits */
#define OMAPW3G_RST_3GAO	       			(1 << 1)
#define OMAPW3G_RST_2GDSM				(1 << 0)

/* RM_RSTST_WKUP specific bits */
#define OMAPW3G_3GAO_RST	       			(1 << 4)
#define OMAPW3G_2GDSM_RST				(1 << 3)

/* PM_MPUGRPSEL_WKUP specific bits */

/* PM_DSPGRPSEL_WKUP specific bits */

/* PM_WKEN_WKUP specific bits */
#define OMAPW3G_EN_IO   				(1 << 15)
#define OMAPW3G_EN_SR2  				(1 << 14)
#define OMAPW3G_EN_SR1  				(1 << 13)
#define OMAPW3G_EN_2GDSM				(1 << 11)
#define OMAPW3G_EN_KBC  				(1 << 10)
#define OMAPW3G_EN_GPIO1    				(1 << 3)
#define OMAPW3G_EN_GPT1				(1 << 0)

/* PM_WKST_WKUP specific bits */
#define OMAPW3G_ST_IO   				(1 << 15)
#define OMAPW3G_ST_SR2  				(1 << 14)
#define OMAPW3G_ST_SR1  				(1 << 13)
#define OMAPW3G_ST_2GDSM				(1 << 11)
#define OMAPW3G_ST_KBC  				(1 << 10)
#define OMAPW3G_ST_GPIO1    				(1 << 3)
#define OMAPW3G_ST_GPT1				(1 << 0)

/* PRM_CLKOUT_CTRL */

#define OMAPW3G_CLKOUT_EN_SHIFT			        7
#define OMAPW3G_CLKOUT_EN_MASK				(1 << 7)
#define OMAPW3G_CLKOUT_POL_SHIFT       		        6
#define OMAPW3G_CLKOUT_POL_MASK				(1 << 6)
#define OMAPW3G_CLKOUT_DIV_SHIFT       		        3
#define OMAPW3G_CLKOUT_DIV_MASK				(1 << 3)

/* RM_RSTST_DSS specific bits */

/* PM_WKEN_DSS */
#define OMAPW3G_PM_WKEN_DSS_EN_DSS			(1 << 0)

/* PM_WKDEP_DSS specific bits */
#define OMAPW3G_WKDEP_DSS_EN_DSP    			(1 << 5)
#define OMAPW3G_WKDEP_DSS_EN_MPU	       		(1 << 4)
#define OMAPW3G_WKDEP_DSS_EN_APP_L3    			(1 << 2)
#define OMAPW3G_WKDEP_DSS_EN_CORE_L3	       		(1 << 0)

/* PM_PWSTCTRL_DSS specific bits */

/* PM_PWSTST_DSS specific bits */

/* PM_PREPWSTST_DSS specific bits */

/* RM_RSTST_CAM specific bits */

/* PM_WKDEP_CAM specific bits */
#define OMAPW3G_WKDEP_CAM_EN_DSP    			(1 << 5)
#define OMAPW3G_WKDEP_CAM_EN_MPU	       		(1 << 4)
#define OMAPW3G_WKDEP_CAM_EN_APP_L3    			(1 << 2)
#define OMAPW3G_WKDEP_CAM_EN_CORE_L3	       		(1 << 0)

/* PM_PWSTCTRL_CAM specific bits */

/* PM_PWSTST_CAM specific bits */

/* PM_PREPWSTST_CAM specific bits */

/* RM_RSTST_APP specific bits */

/* PM_MPUGRPSEL_APP specific bits */

/* PM_DSPGRPSEL_APP specific bits */

/* PM_WKEN_APP specific bits */
#define OMAPW3G_EN_SPI2 				(1 << 1)
#define OMAPW3G_EN_GPT3    	       		(1 << 0)

/* PM_WKST_APP specific bits */
#define OMAPW3G_ST_SPI2 				(1 << 1)
#define OMAPW3G_ST_GPT3    	       		(1 << 0)

/* PM_WKDEP_APP specific bits */
#define OMAPW3G_WKDEP_APP_L4_EN_WKUP			(1 << 22)
#define OMAPW3G_WKDEP_APP_L4_EN_DSP     	       	(1 << 21)
#define OMAPW3G_WKDEP_APP_L4_EN_APP_L3	       		(1 << 18)
#define OMAPW3G_WKDEP_APP_L4_EN_CORE_L3        		(1 << 16)
#define OMAPW3G_WKDEP_APP_L4_EN_WCDMA	       		(1 << 9)
#define OMAPW3G_WKDEP_APP_L3_EN_AUD			(1 << 8)
#define OMAPW3G_WKDEP_APP_L3_EN_WKUP    	       	(1 << 6)
#define OMAPW3G_WKDEP_APP_L3_EN_DSP	       		(1 << 5)
#define OMAPW3G_WKDEP_APP_L3_EN_CORE_L4        		(1 << 1)
#define OMAPW3G_WKDEP_APP_L3_EN_CORE_L3	       		(1 << 0)

/* PM_PWSTCTRL_APP specific bits */
#define OMAPW3G_MEM2ONSTATE_SHIFT       		18
#define OMAPW3G_MEM2ONSTATE_MASK        	       	(0x3 << 18)
#define OMAPW3G_MEM1ONSTATE_SHIFT       		16
#define OMAPW3G_MEM1ONSTATE_MASK			(0x3 << 16)
#define OMAPW3G_MEM2RETSTATE     			(1 << 9)
#define OMAPW3G_MEM1RETSTATE     			(1 << 8)

/* PM_PWSTST_APP specific bits */
#define OMAPW3G_MEM2ONSTATEST_SHIFT       		6
#define OMAPW3G_MEM2ONSTATEST_MASK        	       	(0x3 << 6)
#define OMAPW3G_MEM1ONSTATEST_SHIFT       		4
#define OMAPW3G_MEM1ONSTATEST_MASK			(0x3 << 4)

/* PM_PREPWSTST_APP specific bits */
#define OMAPW3G_LASTMEM2STATEENTERED_SHIFT		6
#define OMAPW3G_LASTMEM2STATEENTERED_MASK		(0x3 << 6)
#define OMAPW3G_LASTMEM1STATEENTERED_SHIFT		4
#define OMAPW3G_LASTMEM1STATEENTERED_MASK		(0x3 << 4)

/* RM_RSTST_EMU specific bits */

/* PM_PWSTST_EMU specific bits */

/* PM_PADST_EMU specific bits */
#define OMAPW3G_PAD_ST   				(1 << 0)

/* RM_RSTCTRL_WCMDA specific bits */
#define OMAPW3G_RST_WCDMA   				(1 << 0)

/* RM_RSTST_WCMDA specific bits */
#define OMAPW3G_WCDMA_SW_RST   				(1 << 8)

/* RM_WKEN_WCMDA specific bits */
#define OMAPW3G_EN_WCDMA   				(1 << 0)

/* RM_WKST_WCMDA specific bits */
#define OMAPW3G_ST_WCDMA   				(1 << 0)

/* PM_PWSTCTRL_WCMDA specific bits */
#define OMAPW3G_M2SRAMONSTATE_SHIFT       		16
#define OMAPW3G_M2SRAMONSTATE_MASK			(0x3 << 16)
#define OMAPW3G_M2SRAMRETSTATE     			(1 << 8)

/* PM_PWSTST_WCMDA specific bits */
#define OMAPW3G_M2SRAMSTATEST_SHIFT       		4
#define OMAPW3G_M2SRAMSTATEST_MASK			(0x3 << 4)

/* PM_PREPWSTST_WCMDA specific bits */
#define OMAPW3G_LASTM2SRAMSTATEENDERED_SHIFT           	4
#define OMAPW3G_LASTM2SRAMSTATEENDERED_MASK	       	(0x3 << 4)

/* RM_RSTCTRL_R5R6 specific bits */
#define OMAPW3G_RST_R5R6   				(1 << 0)

/* RM_RSTST_R5R6 specific bits */
#define OMAPW3G_R5R6_SW_RST   				(1 << 8)

/* PM_PWSTCTRL_R5R6 specific bits */

/* PM_PWSTST_R5R6 specific bits */

/* PM_PREPWSTST_R5R6 specific bits */

/* RM_RSTST_AUD specific bits */

/* PM_MPUGRPSEL_AUD specific bits */

/* PM_DSPGRPSEL_AUD specific bits */

/* PM_WKEN_AUD specific bits */
#define OMAPW3G_EN_MCBSP1				(1 << 3)
#define OMAPW3G_EN_DMTIMER2				(1 << 2)
#define OMAPW3G_EN_GPIO3				(1 << 1)
#define OMAPW3G_EN_GPIO2				(1 << 0)

/* PM_WKST_AUD specific bits */
#define OMAPW3G_ST_MCBSP1				(1 << 3)
#define OMAPW3G_ST_DMTIMER2				(1 << 2)
#define OMAPW3G_ST_GPIO3				(1 << 1)
#define OMAPW3G_ST_GPIO2				(1 << 0)

/* PM_WKDEP_AUD specific bits */
#define OMAPW3G_WKDEP_AUD_EN_DSP		       	(1 << 5)
#define OMAPW3G_WKDEP_AUD_EN_MPU    			(1 << 4)
#define OMAPW3G_WKDEP_AUD_EN_APP	       		(1 << 2)
#define OMAPW3G_WKDEP_AUD_EN_CORE	       		(1 << 0)

/* PM_PWSTCTRL_AUD specific bits */

/* PM_PWSTST_AUD specific bits */

/* PM_PREPWSTST_AUD specific bits */

/* PRM_VC_SMPS_SA specific bits */
#define OMAPW3G_SMPS_SA1_SHIFT				16
#define OMAPW3G_SMPS_SA1_MASK				(0x7f << 16)
#define OMAPW3G_SMPS_SA0_SHIFT				0
#define OMAPW3G_SMPS_SA0_MASK				(0x7f << 0)

/* PRM_VC_SMPS_VOL_RA specific bits */
#define OMAPW3G_VOLRA1_SHIFT				16
#define OMAPW3G_VOLRA1_MASK				(0xff << 16)
#define OMAPW3G_VOLRA0_SHIFT				0
#define OMAPW3G_VOLRA0_MASK				(0xff << 0)

/* PRM_VC_SMPS_CMD_RA specific bits */
#define OMAPW3G_CMDRA1_SHIFT				16
#define OMAPW3G_CMDRA1_MASK				(0xff << 16)
#define OMAPW3G_CMDRA0_SHIFT				0
#define OMAPW3G_CMDRA0_MASK				(0xff << 0)

/* PRM_VC_CMD_VAL_0 specific bits */

/* PRM_VC_CMD_VAL_1 specific bits */

/* PRM_VC_CH_CONF specific bits */
#define OMAPW3G_CMD1					(1 << 20)
#define OMAPW3G_RACEN1					(1 << 19)
#define OMAPW3G_RAC1					(1 << 18)
#define OMAPW3G_RAV1					(1 << 17)
#define OMAPW3G_SA1	                		(1 << 16)
#define OMAPW3G_CMD0					(1 << 4)
#define OMAPW3G_RACEN0					(1 << 3)
#define OMAPW3G_RAC0					(1 << 2)
#define OMAPW3G_RAV0					(1 << 1)
#define OMAPW3G_SA0		                	(1 << 0)

/* PRM_VC_I2C_CFG specific bits */
#define OMAPW3G_HSMASTER				(1 << 5)
#define OMAPW3G_SREN					(1 << 4)
#define OMAPW3G_HSEN					(1 << 3)
#define OMAPW3G_MCODE_SHIFT				0
#define OMAPW3G_MCODE_MASK				(0x7 << 0)

/* PRM_VC_BYPASS_VAL specific bits */
#define OMAPW3G_VALID					(1 << 24)
#define OMAPW3G_DATA_SHIFT				16
#define OMAPW3G_DATA_MASK				(0xff << 16)
#define OMAPW3G_REGADDR_SHIFT				8
#define OMAPW3G_REGADDR_MASK				(0xff << 8)
#define OMAPW3G_SLAVEADDR_SHIFT	        		0
#define OMAPW3G_SLAVEADDR_MASK				(0x7f << 0)

/* PRM_RSTCTRL specific bits */
#define OMAPW3G_GLOBAL_COLD_SW_RST     			(1 << 2)
#define OMAPW3G_GLOBAL_WARM_SW_RST	       		(1 << 1)

/* PRM_RSTTIME specific bits */
#define OMAPW3G_RSTTIME2_SHIFT				8
#define OMAPW3G_RSTTIME2_MASK				(0x1f << 8)
#define OMAPW3G_RSTTIME1_SHIFT				0
#define OMAPW3G_RSTTIME1_MASK				(0xff << 0)

/* PRM_RSTST specific bits */
#define OMAPW3G_ICECRUSHER_RST				(1 << 10)
#define OMAPW3G_ICEPICK_RST				(1 << 9)
#define OMAPW3G_VDD2_VOLTAGE_MANAGER_RST		(1 << 8)
#define OMAPW3G_VDD1_VOLTAGE_MANAGER_RST		(1 << 7)
#define OMAPW3G_EXTERNAL_WARM_RST			(1 << 6)
#define OMAPW3G_SECURE_WD_RST				(1 << 5)
#define OMAPW3G_MODEM_WD_RST				(1 << 4)
#define OMAPW3G_SECURITY_VIOL_RST			(1 << 3)
#define OMAPW3G_SW_GLOBAL_WARM_RST			(1 << 1)
#define OMAPW3G_GLOBAL_COLD_RST	        		(1 << 0)

/* PRM_VOLTCTRL specific bits */
#define OMAPW3G_VOLTAGE_CONFIG				(1 << 6)
#define OMAPW3G_VMODE_POL				(1 << 5)
#define OMAPW3G_SEL_VMODE				(1 << 4)
#define OMAPW3G_AUTO_CTRL_VDD2_SHIFT                    2
#define OMAPW3G_AUTO_CTRL_VDD2_MASK    			(0x3 << 2)
#define OMAPW3G_AUTO_CTRL_VDD1_SHIFT                    0
#define OMAPW3G_AUTO_CTRL_VDD1_MASK    			(0x3 << 0)

/* PRM_PSCON_PARAM specific bits */
#define OMAPW3G_PONOUT_2_PGOODIN_TIME_SHIFT    		08
#define OMAPW3G_PONOUT_2_PGOODIN_TIME_MASK     		(0xff << 0)
#define OMAPW3G_SRAM_PCHARGE_TIME_SHIFT			0
#define OMAPW3G_SRAM_PCHARGE_TIME_MASK			(0xff << 0)

/* PRM_CLKSRC_CTRL specific bits */
#define OMAPW3G_AUTO_CLKREQ_SHIFT			3
#define OMAPW3G_AUTO_CLKREQ_MODE_MASK			(0x3 << 1)
#define OMAPW3G_CLKREQ_POL				(1 << 0)

/* PRM_DEBUG specific bits */
#define OMAPW3G_FORCE_DSP_STATE_RET			(1 << 1)
#define OMAPW3G_FORCE_DSP_LOGIC_ON             		(1 << 0)

/* PRM_D2DPMCTRL specific bits */
#define OMAPW3G_DIE_CONFIG      			(1 << 2)
#define OMAPW3G_MODEM_MODE      			(1 << 1)
#define OMAPW3G_APE_WAKEUP	        		(1 << 0)

/* PRM_D2DPMST specific bits */
#define OMAPW3G_APE_IDLE_STATUS	        		(1 << 0)

/* PRM_VOLTSETUP1 specific bits */
#define OMAPW3G_SETUP_TIME2_SHIFT			16
#define OMAPW3G_SETUP_TIME2_MASK			(0xffff << 16)
#define OMAPW3G_SETUP_TIME1_SHIFT			0
#define OMAPW3G_SETUP_TIME1_MASK			(0xffff << 0)

/* PRM_VOLTOFFSET specific bits */
#define OMAPW3G_OFFSET_TIME_SHIFT			0
#define OMAPW3G_OFFSET_TIME_MASK			(0xffff << 0)

/* PRM_CLKSETUP specific bits */
#define OMAPW3G_SETUP_TIME_SHIFT			0
#define OMAPW3G_SETUP_TIME_MASK	         		(0xffff << 0)

/* PRM_VP1_CONFIG specific bits */

/* PRM_VP1_VSTEPMIN specific bits */

/* PRM_VP1_VSTEPMAX specific bits */

/* PRM_VP1_VLIMITTO specific bits */

/* PRM_VP1_VOLTAGE specific bits */

/* PRM_VP1_STATUS specific bits */

/* PRM_VP2_CONFIG specific bits */

/* PRM_VP2_VSTEPMIN specific bits */

/* PRM_VP2_VSTEPMAX specific bits */

/* PRM_VP2_VLIMITTO specific bits */

/* PRM_VP2_VOLTAGE specific bits */

/* PRM_VP2_STATUS specific bits */

#endif
