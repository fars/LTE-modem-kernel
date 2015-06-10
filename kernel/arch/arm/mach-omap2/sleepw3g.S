/*
 * linux/arch/arm/mach-omap2/sleep.S
 *
 * (C) Copyright 2007
 * Texas Instruments
 * Karthik Dasu <karthik-dp@ti.com>
 *
 * (C) Copyright 2004
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR /PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/io.h>
#include <mach/control.h>

#include "cm.h"
#include "prm.h"
#include "sdrc.h"

#define PM_PREPWSTST_CORE_V	OMAP34XX_PRM_REGADDR(CORE_MOD, \
				PM_PREPWSTST)
#define PM_PREPWSTST_CORE_P	0x48306AE8
#define PM_PREPWSTST_MPU_V	OMAP34XX_PRM_REGADDR(MPU_MOD, \
				PM_PREPWSTST)
#define CM_IDLEST1_CORE_V	OMAP34XX_CM_REGADDR(CORE_MOD, CM_IDLEST1)
#define SDRC_SCRATCHPAD_SEM_V	0xd800291C

/*
 * This is the physical address of the register as specified
 * by the _P. To be used while the MMU is still disabled.
 */
#define PM_PWSTCTRL_MPU_P	(OMAP3430_PRM_BASE + MPU_MOD + PM_PWSTCTRL)
#define SRAM_BASE_P		0x40200000
#define CONTROL_STAT		0x480022F0
#define SCRATCHPAD_MEM_OFFS	0x310 /* Move this as correct place is
				       * available */
#define SCRATCHPAD_BASE_P	(OMAP343X_CTRL_BASE + OMAP343X_CONTROL_MEM_WKUP\
						+ SCRATCHPAD_MEM_OFFS)
#define SDRC_POWER_V		OMAP34XX_SDRC_REGADDR(SDRC_POWER)
#define SDRC_SYSCONFIG_P	        (OMAPW3G_SDRC_BASE + SDRC_SYSCONFIG)
#define SDRC_MR_0_P		(OMAPW3G_SDRC_BASE + SDRC_MR_0)
#define SDRC_EMR2_0_P		(OMAPW3G_SDRC_BASE + SDRC_EMR2_0)
#define SDRC_MANUAL_0_P		(OMAPW3G_SDRC_BASE + SDRC_MANUAL_0)
#define SDRC_MR_1_P		(OMAPW3G_SDRC_BASE + SDRC_MR_1)
#define SDRC_EMR2_1_P		(OMAPW3G_SDRC_BASE + SDRC_EMR2_1)
#define SDRC_MANUAL_1_P		(OMAPW3G_SDRC_BASE + SDRC_MANUAL_1)
#define SDRC_DLLA_STATUS_V	OMAP34XX_SDRC_REGADDR(SDRC_DLLA_STATUS)
#define SDRC_DLLA_CTRL_V	OMAP34XX_SDRC_REGADDR(SDRC_DLLA_CTRL)

        .text
/* Function to aquire the semaphore in scratchpad */
ENTRY(lock_scratchpad_sem)
	stmfd	sp!, {lr}	@ save registers on stack
wait_sem:
	mov	r0,#1
	ldr	r1, sdrc_scratchpad_sem
wait_loop:
	ldr	r2, [r1]	@ load the lock value
	cmp	r2, r0		@ is the lock free ?
	beq	wait_loop	@ not free...
	swp	r2, r0, [r1]	@ semaphore free so lock it and proceed
	cmp	r2, r0		@ did we succeed ?
	beq	wait_sem	@ no - try again
	ldmfd	sp!, {pc}	@ restore regs and return
sdrc_scratchpad_sem:
        .word SDRC_SCRATCHPAD_SEM_V
ENTRY(lock_scratchpad_sem_sz)
        .word   . - lock_scratchpad_sem

        .text
/* Function to release the scratchpad semaphore */
ENTRY(unlock_scratchpad_sem)
	stmfd	sp!, {lr}	@ save registers on stack
	ldr	r3, sdrc_scratchpad_sem
	mov	r2,#0
	str	r2,[r3]
	ldmfd	sp!, {pc}	@ restore regs and return
ENTRY(unlock_scratchpad_sem_sz)
        .word   . - unlock_scratchpad_sem

	.text
/* Function call to get the restore pointer for resume from OFF */
ENTRY(get_restore_pointer)
        stmfd   sp!, {lr}     @ save registers on stack
	adr	r0, restore
        ldmfd   sp!, {pc}     @ restore regs and return
ENTRY(get_restore_pointer_sz)
        .word   . - get_restore_pointer

	.text
/* Function call to get the restore pointer for for ES3 to resume from OFF */
ENTRY(get_es3_restore_pointer)
	stmfd	sp!, {lr}	@ save registers on stack
	adr	r0, restore_es3
	ldmfd	sp!, {pc}	@ restore regs and return
ENTRY(get_es3_restore_pointer_sz)
	.word	. - get_es3_restore_pointer

ENTRY(es3_sdrc_fix)
	ldr	r4, sdrc_syscfg		@ get config addr
	ldr	r5, [r4]		@ get value
	tst	r5, #0x100		@ is part access blocked
	it	eq
	biceq	r5, r5, #0x100		@ clear bit if set
	str	r5, [r4]		@ write back change
	ldr	r4, sdrc_mr_0		@ get config addr
	ldr	r5, [r4]		@ get value
	str	r5, [r4]		@ write back change
	ldr	r4, sdrc_emr2_0		@ get config addr
	ldr	r5, [r4]		@ get value
	str	r5, [r4]		@ write back change
	ldr	r4, sdrc_manual_0	@ get config addr
	mov	r5, #0x2		@ autorefresh command
	str	r5, [r4]		@ kick off refreshes
	ldr	r4, sdrc_mr_1		@ get config addr
	ldr	r5, [r4]		@ get value
	str	r5, [r4]		@ write back change
	ldr	r4, sdrc_emr2_1		@ get config addr
	ldr	r5, [r4]		@ get value
	str	r5, [r4]		@ write back change
	ldr	r4, sdrc_manual_1	@ get config addr
	mov	r5, #0x2		@ autorefresh command
	str	r5, [r4]		@ kick off refreshes
	bx	lr
sdrc_syscfg:
	.word	SDRC_SYSCONFIG_P
sdrc_mr_0:
	.word	SDRC_MR_0_P
sdrc_emr2_0:
	.word	SDRC_EMR2_0_P
sdrc_manual_0:
	.word	SDRC_MANUAL_0_P
sdrc_mr_1:
	.word	SDRC_MR_1_P
sdrc_emr2_1:
	.word	SDRC_EMR2_1_P
sdrc_manual_1:
	.word	SDRC_MANUAL_1_P
ENTRY(es3_sdrc_fix_sz)
	.word	. - es3_sdrc_fix

/* Function to call rom code to save secure ram context */
ENTRY(save_secure_ram_context)
	stmfd	sp!, {r1-r12, lr}	@ save registers on stack
save_secure_ram_debug:
	/* b save_secure_ram_debug */	@ enable to debug save code
	adr	r3, api_params		@ r3 points to parameters
	str	r0, [r3,#0x4]		@ r0 has sdram address
	ldr	r12, high_mask
	and	r3, r3, r12
	ldr	r12, sram_phy_addr_mask
	orr	r3, r3, r12
	mov	r0, #25			@ set service ID for PPA
	mov	r12, r0			@ copy secure service ID in r12
	mov	r1, #0			@ set task id for ROM code in r1
	mov	r2, #4			@ set some flags in r2, r6
	mov	r6, #0xff
	mcr	p15, 0, r0, c7, c10, 4	@ data write barrier
	mcr	p15, 0, r0, c7, c10, 5	@ data memory barrier
	.word	0xE1600071		@ call SMI monitor (smi #1)
	nop
	nop
	nop
	nop
	ldmfd	sp!, {r1-r12, pc}
sram_phy_addr_mask:
	.word	SRAM_BASE_P
high_mask:
	.word	0xffff
api_params:
	.word	0x4, 0x0, 0x0, 0x1, 0x1
ENTRY(save_secure_ram_context_sz)
	.word	. - save_secure_ram_context

/*
 * Forces OMAP into idle state
 *
 * omap34xx_suspend() - This bit of code just executes the WFI
 * for normal idles.
 *
 * Note: This code get's copied to internal SRAM at boot. When the OMAP
 *	 wakes up it continues execution at the point it went to sleep.
 */
ENTRY(omap34xx_cpu_suspend)
	stmfd	sp!, {r0-r12, lr}		@ save registers on stack
loop:
	/*b	loop*/	@Enable to debug by stepping through code
	/* r0 contains restore pointer in sdram */
	/* r1 contains information about saving context */
#if 0
	ldr     r4, sdrc_power          @ read the SDRC_POWER register
	ldr     r5, [r4]                @ read the contents of SDRC_POWER
	orr     r5, r5, #0x40           @ enable self refresh on idle req
	str     r5, [r4]                @ write back to SDRC_POWER register
#endif
	cmp	r1, #0x0
	/* If context save is required, do that and execute wfi */
	bne	save_context_wfi
	/* Data memory barrier and Data sync barrier */
	mov	r1, #0
	mcr	p15, 0, r1, c7, c10, 4
	mcr     p15, 0, r1, c7, c0,  4  @ wait for interrupt

	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	bl wait_sdrc_ok

	ldmfd	sp!, {r0-r12, pc}		@ restore regs and return
restore_es3:
	/*b restore_es3*/		@ Enable to debug restore code
	ldr	r5, pm_prepwstst_core_p
	ldr	r4, [r5]
	and	r4, r4, #0x3
	cmp	r4, #0x0	@ Check if previous power state of CORE is OFF
	bne	restore
	adr	r0, es3_sdrc_fix
	ldr	r1, sram_base
	ldr	r2, es3_sdrc_fix_sz
	mov	r2, r2, ror #2
copy_to_sram:
	ldmia	r0!, {r3}	@ val = *src
	stmia	r1!, {r3}	@ *dst = val
	subs	r2, r2, #0x1	@ num_words--
	bne	copy_to_sram
	ldr	r1, sram_base
	blx	r1
restore:
	/* b restore*/  @ Enable to debug restore code
        /* Check what was the reason for mpu reset and store the reason in r9*/
        /* 1 - Only L1 and logic lost */
        /* 2 - Only L2 lost - In this case, we wont be here */
        /* 3 - Both L1 and L2 lost */
	ldr     r1, pm_pwstctrl_mpu
	ldr	r2, [r1]
	and     r2, r2, #0x3
	cmp     r2, #0x0	@ Check if target power state was OFF or RET
        moveq   r9, #0x3        @ MPU OFF => L1 and L2 lost
	movne	r9, #0x1	@ Only L1 and L2 lost => avoid L2 invalidation
	bne	logic_l1_restore
	ldr	r0, control_stat
	ldr	r1, [r0]
	and	r1, #0x700
	cmp	r1, #0x300
	beq	l2_inv_gp
	mov	r0, #40		@ set service ID for PPA
	mov	r12, r0		@ copy secure Service ID in r12
	mov	r1, #0		@ set task id for ROM code in r1
	mov	r2, #4		@ set some flags in r2, r6
	mov	r6, #0xff
	adr	r3, l2_inv_api_params	@ r3 points to dummy parameters
	mcr	p15, 0, r0, c7, c10, 4	@ data write barrier
	mcr	p15, 0, r0, c7, c10, 5	@ data memory barrier
	.word	0xE1600071		@ call SMI monitor (smi #1)
	/* Write to Aux control register to set some bits */
	mov	r0, #42		@ set service ID for PPA
	mov	r12, r0		@ copy secure Service ID in r12
	mov	r1, #0		@ set task id for ROM code in r1
	mov	r2, #4		@ set some flags in r2, r6
	mov	r6, #0xff
	adr	r3, write_aux_control_params	@ r3 points to parameters
	mcr	p15, 0, r0, c7, c10, 4	@ data write barrier
	mcr	p15, 0, r0, c7, c10, 5	@ data memory barrier
	.word	0xE1600071		@ call SMI monitor (smi #1)

	b	logic_l1_restore
l2_inv_api_params:
	.word   0x1, 0x00
write_aux_control_params:
	.word   0x1, 0x72
l2_inv_gp:
	/* Execute smi to invalidate L2 cache */
	mov r12, #0x1                         @ set up to invalide L2
smi:    .word 0xE1600070		@ Call SMI monitor (smieq)
	/* Write to Aux control register to set some bits */
	mov	r0, #0x72
	mov	r12, #0x3
	.word 0xE1600070	@ Call SMI monitor (smieq)
logic_l1_restore:
	mov	r1, #0
	/* Invalidate all instruction caches to PoU
	 * and flush branch target cache */
	mcr	p15, 0, r1, c7, c5, 0

	ldr	r4, scratchpad_base
	ldr	r3, [r4,#0xBC]
	ldmia	r3!, {r4-r6}
	mov	sp, r4
	msr	spsr_cxsf, r5
	mov	lr, r6

	ldmia	r3!, {r4-r9}
	/* Coprocessor access Control Register */
	mcr p15, 0, r4, c1, c0, 2

	/* TTBR0 */
	MCR p15, 0, r5, c2, c0, 0
	/* TTBR1 */
	MCR p15, 0, r6, c2, c0, 1
	/* Translation table base control register */
	MCR p15, 0, r7, c2, c0, 2
	/*domain access Control Register */
	MCR p15, 0, r8, c3, c0, 0
	/* data fault status Register */
	MCR p15, 0, r9, c5, c0, 0

	ldmia  r3!,{r4-r8}
	/* instruction fault status Register */
	MCR p15, 0, r4, c5, c0, 1
	/*Data Auxiliary Fault Status Register */
	MCR p15, 0, r5, c5, c1, 0
	/*Instruction Auxiliary Fault Status Register*/
	MCR p15, 0, r6, c5, c1, 1
	/*Data Fault Address Register */
	MCR p15, 0, r7, c6, c0, 0
	/*Instruction Fault Address Register*/
	MCR p15, 0, r8, c6, c0, 2
	ldmia  r3!,{r4-r7}

	/* user r/w thread and process ID */
	MCR p15, 0, r4, c13, c0, 2
	/* user ro thread and process ID */
	MCR p15, 0, r5, c13, c0, 3
	/*Privileged only thread and process ID */
	MCR p15, 0, r6, c13, c0, 4
	/* cache size selection */
	MCR p15, 2, r7, c0, c0, 0
	ldmia  r3!,{r4-r8}
	/* Data TLB lockdown registers */
	MCR p15, 0, r4, c10, c0, 0
	/* Instruction TLB lockdown registers */
	MCR p15, 0, r5, c10, c0, 1
	/* Secure or Nonsecure Vector Base Address */
	MCR p15, 0, r6, c12, c0, 0
	/* FCSE PID */
	MCR p15, 0, r7, c13, c0, 0
	/* Context PID */
	MCR p15, 0, r8, c13, c0, 1

	ldmia  r3!,{r4-r5}
	/* primary memory remap register */
	MCR p15, 0, r4, c10, c2, 0
	/*normal memory remap register */
	MCR p15, 0, r5, c10, c2, 1

	/* Restore cpsr */
	ldmia	r3!,{r4}	/*load CPSR from SDRAM*/
	msr	cpsr, r4	/*store cpsr */

	/* Enabling MMU here */
	mrc	p15, 0, r7, c2, c0, 2 /* Read TTBRControl */
	/* Extract N (0:2) bits and decide whether to use TTBR0 or TTBR1*/
	and	r7, #0x7
	cmp	r7, #0x0
	beq	usettbr0
ttbr_error:
	/* More work needs to be done to support N[0:2] value other than 0
	* So looping here so that the error can be detected
	*/
	b	ttbr_error
usettbr0:
	mrc	p15, 0, r2, c2, c0, 0
	ldr	r5, ttbrbit_mask
	and	r2, r5
	mov	r4, pc
	ldr	r5, table_index_mask
	and	r4, r5 /* r4 = 31 to 20 bits of pc */
	/* Extract the value to be written to table entry */
	ldr	r1, table_entry
	add	r1, r1, r4 /* r1 has value to be written to table entry*/
	/* Getting the address of table entry to modify */
	lsr	r4, #18
	add	r2, r4 /* r2 has the location which needs to be modified */
	/* Storing previous entry of location being modified */
	ldr	r5, scratchpad_base
	ldr	r4, [r2]
	str	r4, [r5, #0xC0]
	/* Modify the table entry */
	str	r1, [r2]
	/* Storing address of entry being modified
	 * - will be restored after enabling MMU */
	ldr	r5, scratchpad_base
	str	r2, [r5, #0xC4]

	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 4	@ Flush prefetch buffer
	mcr	p15, 0, r0, c7, c5, 6	@ Invalidate branch predictor array
	mcr	p15, 0, r0, c8, c5, 0	@ Invalidate instruction TLB
	mcr	p15, 0, r0, c8, c6, 0	@ Invalidate data TLB
	/* Restore control register  but dont enable caches here*/
	/* Caches will be enabled after restoring MMU table entry */
	ldmia	r3!, {r4}
	/* Store previous value of control register in scratchpad */
	str	r4, [r5, #0xC8]
	ldr	r2, cache_pred_disable_mask
	and	r4, r2
	mcr	p15, 0, r4, c1, c0, 0

	ldmfd	sp!, {r0-r12, pc}		@ restore regs and return
save_context_wfi:
        /*b     save_context_wfi*/      @ enable to debug save code
        mov     r8, r0 /* Store SDRAM address in r8 */
l1_logic_lost:
        /* Store sp and spsr to SDRAM */
        mov     r4, sp
        mrs     r5, spsr
        mov     r6, lr
        stmia   r8!, {r4-r6}

       /* Save all ARM registers */
        /* TTBR0, TTBR1 and Translation table base control */
        mrc     p15, 0, r4, c2, c0, 0	@Refer to page 46, section
						@2.3.3 of ARM926EJS_TRM
        stmia   r8!, {r4}

       /* Domain access control Data fault status register (DFSR),
        and instruction fault status register (IFSR) */
        mrc     p15, 0, r4, c3, c0, 0	@Read Domain access control register
        mrc     p15, 0, r5, c5, c0, 0	@Read DFSR - Refer to page 48,
						@section 2.3.6 of TRM
        mrc     p15, 0, r6, c5, c0, 1	@Read IFSR - Refer to page 49,
						@section 2.3.6 of TRM
        stmia   r8!, {r4-r6}

       /* Only Fault address register is there on ARM926 */
        mrc     p15, 0, r6, c6, c0, 0
        stmia   r8!, {r6}

       /* Data TLB lockdown, FCSE PID and Context PID  */
        mrc     p15, 0, r4, c10, c0, 0
        mrc     p15, 0, r5, c13, c0, 0
        mrc     p15, 0, r6, c13, c0, 1
        stmia   r8!, {r4}

        /* Store SP, LR, SPSR registers for SUP, FIQ, IRQ, ABORT and USER
        modes into SDRAM */

        /* move SDRAM address to r7 as r8 is banked in FIQ*/
        mov     r7, r8

        /* Save current mode */
        mrs     r2, cpsr
        /* FIQ mode */
        bic     r0, r2, #0x1F
        orr     r0, r0, #0x11
        msr     cpsr, r0 /* go to FIQ mode */
        stmia   r7!, {r8-r12}
        mov     r4, r13 /* move SP into r4*/
        mov     r5, r14
        mrs     r6, spsr
        stmia   r7!, {r4-r6}

        /* IRQ mode */
        bic     r0, r2, #0x1F
        orr     r0, r0, #0x12
        msr     cpsr, r0
        mov     r4, r13
        mov     r5, r14
        mrs     r6, spsr
        stmia   r7!, {r4-r6}

        /* Abort mode */
        bic     r0, r2, #0x1F
        orr     r0, r0, #0x17
        msr     cpsr, r0
        mov     r4, r13
        mov     r5, r14
        mrs     r6, spsr
        stmia   r7!, {r4-r6}

        /* UNDEF mode */
        bic     r0, r2, #0x1F
        orr     r0, r0, #0x1B
        msr     cpsr, r0
        mov     r4, r13
        mov     r5, r14
        mrs     r6, spsr
        stmia   r7!, {r4-r6}

        /* System (USER mode) */
        bic     r0, r2, #0x1F
        orr     r0, r0, #0x1F
        msr     cpsr, r0
        mov     r4, r13
        mov     r5, r14
        mrs     r6, spsr
        stmia   r7!, {r4-r6}

        /* Back to original mode */
        msr     cpsr, r2

        /* Store current cpsr*/
        stmia   r7!, {r2}

        mrc     p15, 0, r4, c1, c0, 0
        /* save control register */
        stmia   r7!, {r4}
clean_caches:
        /* There are only I-Cache and D-Cache present.
            mrc         p15, 0, r15, c7, c10, 3 @Refer to page 171,
						@section 9.2 of ARM926 TRM
            bne     clean_caches

            mcr      p15, 0, r0, c7, c10, 4     @drain write buffer
            mcr      p15, 0, r0, c7, c5, 0          @invalidate i-cache

        /* Data memory barrier */
        mov     r1, #0
        mcr     p15, 0, r1, c7, c10, 4

        mcr     p15, 0, r1, c7, c0, 4	@wait for interrupt. Refer to
						@page 182 in ARM926 TRM
        nop
        nop
        nop
        bl wait_sdrc_ok
        /* restore regs and return */
        ldmfd   sp!, {r0-r12, pc}

/* Make sure SDRC accesses are ok */
wait_sdrc_ok:
        ldr     r4, cm_idlest1_core
        ldr     r5, [r4]
        and     r5, r5, #0x80   /* bti field is changed from
		                       * 3430 (bit 1) to W3G (bit 7) */
        cmp     r5, #0
        bne     wait_sdrc_ok

wait_dll_lock:
        /* Is dll in lock mode? */
        ldr     r4, sdrc_dlla_ctrl
        ldr     r5, [r4]
        tst     r5, #0x4
        bxne    lr
        /* wait till dll locks */
        ldr     r4, sdrc_dlla_status
        ldr     r5, [r4]
        and     r5, r5, #0x4
        cmp     r5, #0x4
        bne     wait_dll_lock
        bx      lr

cm_idlest1_core:
	.word	CM_IDLEST1_CORE_V
sdrc_dlla_status:
	.word	SDRC_DLLA_STATUS_V
sdrc_dlla_ctrl:
	.word	SDRC_DLLA_CTRL_V
pm_prepwstst_core:
	.word	PM_PREPWSTST_CORE_V
pm_prepwstst_core_p:
	.word	PM_PREPWSTST_CORE_P
pm_prepwstst_mpu:
	.word	PM_PREPWSTST_MPU_V
pm_pwstctrl_mpu:
	.word	PM_PWSTCTRL_MPU_P
scratchpad_base:
	.word	SCRATCHPAD_BASE_P
sram_base:
	.word	SRAM_BASE_P + 0x8000
sdrc_power:
	.word SDRC_POWER_V
clk_stabilize_delay:
	.word 0x000001FF
assoc_mask:
	.word	0x3ff
numset_mask:
	.word	0x7fff
ttbrbit_mask:
	.word	0xFFFFC000
table_index_mask:
	.word	0xFFF00000
table_entry:
	.word	0x00000C02
cache_pred_disable_mask:
	.word	0xFFFFE7FB
control_stat:
	.word	CONTROL_STAT
ENTRY(omap34xx_cpu_suspend_sz)
	.word	. - omap34xx_cpu_suspend
