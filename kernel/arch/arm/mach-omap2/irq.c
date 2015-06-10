/*
 * linux/arch/arm/mach-omap2/irq.c
 *
 * Interrupt handler for OMAP2 boards.
 *
 * Copyright (C) 2005 Nokia Corporation
 * Author: Paul Mundt <paul.mundt@nokia.com>
 *
 * Copyright (C) 2009 Motorola
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <asm/mach/irq.h>
#include <mach/cpu.h>

/* selected INTC register offsets */

#define INTC_REVISION		0x0000
#define INTC_SYSCONFIG		0x0010
#define INTC_SYSSTATUS		0x0014
#define INTC_SIR		0x0040
#define INTC_CONTROL		0x0048
#define INTC_PROTECTION		0x004C
#define INTC_IDLE		0x0050
#define INTC_THRESHOLD		0x0068
#define INTC_SICR		0x006C
#define INTC_MIR0		0x0084
#define INTC_MIR_CLEAR0		0x0088
#define INTC_MIR_SET0		0x008c
#define INTC_PENDING_IRQ0	0x0098
#define INTC_ILR_REG0    	0x0100


/* Number of IRQ state bits in each MIR register */
#define IRQ_BITS_PER_REG	32

/*
 * OMAP2 has a number of different interrupt controllers, each interrupt
 * controller is identified as its own "bank". Register definitions are
 * fairly consistent for each bank, but not all registers are implemented
 * for each bank.. when in doubt, consult the TRM.
 */
static struct omap_irq_bank {
	void __iomem *base_reg;
	unsigned int nr_irqs;
} __attribute__ ((aligned(4))) irq_banks[] = {
	{
		/* MPU INTC */
		.base_reg	= 0,
		.nr_irqs	= 96,
	},
};

/* Structure to save interrupt controller context */
struct omap3_intc_regs {
	u32 sysconfig;
	u32 protection;
	u32 idle;
	u32 threshold;
	u32 ilr[INTCPS_NR_IRQS];
	u32 mir[INTCPS_NR_MIR_REGS];
};

static struct omap3_intc_regs intc_context[ARRAY_SIZE(irq_banks)];


/*
Earlier to this version, interrupt priorities were not set. All interrupt priorities were
set to defatult 0
In this version - Interrupt priorities are set (as follows)
system DMA, SIM, usb, mailbox, timer, GPIO, smart reflex?, I2C, SPI1, SPI2, 1-wire,
SDMA_IRQ_0 	= 5
SDMA_IRQ_1 	= 6
SDMA_IRQ_2 	= 7
SDMA_IRQ_3 	= 8
USIM_IRQ   	= 10
DSP_HOST_IRQ	= 12
MAIL_U0_MPU_IRQ	= 14
MAIL_U2_MPU_IRQ	= 15
SPI1_IRQ	= 17
SPI2_IRQ	= 18
I2C1_IRQ	= 20
GPIO1_MPU_IRQ	= 22
GPIO2_MPU_IRQ	= 23
GPIO3_MPU_IRQ	= 24
GPT1_IRQ	= 26
GPT2_IRQ	= 27
GPT3_IRQ	= 28
HSUSB_MC_IRQ	= 30
HSUSB_DMA_IRQ	= 31
KBD_CTL_IRQ	= 33
SR2_IRQ		= 35
SR1_IRQ		= 36
CM_MPU_IRQ	= 38
2G_DSM_IRQ	= 39
3G_DSM_IRQ	= 40
HDQ_IRQ		= 42
UART1_IRQ	= 44
UART2_IRQ	= 45
UART3_IRQ	= 46
DSS_IRQ		= 48
CAM_IRQ		= 49
CAM_CORE_IRQ	= 50
MMC1_IRQ	= 52

- From 0 to 4 - Not assigned. Kept for future (if required, can use them).
- The reserved, interrupts are marked as 65.
- The default interrupt priority (of other modules, which are not required and NOT reserved) are kept as 58
- Between each of these modules (DMA, USB, Mailbox etc) one interrupt priority vacant (can use them if required in future).
*/
int irq_priority [96] = {
58, 65, 65, 65, 65, 65, 58, 58, 65, 58, 58, 58, 5, 6, 7, 8, 65, 35, 36, 10, 58, 58, 65, 50,
49, 48, 14, 15, 65, 22, 23, 24, 39, 40, 38, 12, 58, 26, 27, 28, 58, 58, 58, 58, 65, 58, 58, 58,
58, 65, 58, 58, 58, 58, 65, 65, 20, 65, 42, 58, 58, 58, 58, 58, 58, 17, 18, 65, 65, 65, 65, 65,
44, 45, 46, 65, 65, 65, 58, 58, 65, 65, 65, 52, 65, 65, 58, 33, 65, 65, 65, 65, 30, 31, 58, 65
};



/* INTC bank register get/set */

static void intc_bank_write_reg(u32 val, struct omap_irq_bank *bank, u16 reg)
{
	__raw_writel(val, bank->base_reg + reg);
}

static u32 intc_bank_read_reg(struct omap_irq_bank *bank, u16 reg)
{
	return __raw_readl(bank->base_reg + reg);
}

static int previous_irq;

/*
 * On 34xx we can get occasional spurious interrupts if the ack from
 * an interrupt handler does not get posted before we unmask. Warn about
 * the interrupt handlers that need to flush posted writes.
 */
static int omap_check_spurious(unsigned int irq)
{
	u32 sir, spurious;

	sir = intc_bank_read_reg(&irq_banks[0], INTC_SIR);
	spurious = sir >> 7;

	if (spurious) {
		printk(KERN_WARNING "Spurious irq %i: 0x%08x, please flush "
					"posted write for irq %i\n",
					irq, sir, previous_irq);
		return spurious;
	}

	return 0;
}

static void omap_irq_set_cfg(int irq, int fiq, int priority)
{
	unsigned long val, offset;
	val = (fiq & 0x01) | ((priority & 0x3f) << 2) ;
	offset = INTC_ILR_REG0 + (irq * 0x04);
        intc_bank_write_reg(val, &irq_banks[0], offset);
}

static void omap_global_enable(void)
{
	unsigned long tmp;
	tmp = intc_bank_read_reg(&irq_banks[0], INTC_SICR);
	tmp &= ~(1 << 6);
	intc_bank_write_reg(tmp, &irq_banks[0], INTC_SICR);
}

static void omap_ack_irq(unsigned int irq)
{
    unsigned long tmp;

    tmp = intc_bank_read_reg(&irq_banks[0], (INTC_ILR_REG0 + (irq * 0x04)));
    if(tmp & 0x01)
    {
        /* FIQ ACK*/
	intc_bank_write_reg(0x02, &irq_banks[0], INTC_CONTROL);
    }
    else
    {
        /*IRQ ACK*/
	intc_bank_write_reg(0x01, &irq_banks[0], INTC_CONTROL);
    }
}

static void omap_mask_irq(unsigned int irq)
{
	int offset = irq & (~(IRQ_BITS_PER_REG - 1));

	if (cpu_is_omap34xx() || cpu_is_omapw3g()) {
		int spurious = 0;

		/*
		 * INT_34XX_GPT12_IRQ is also the spurious irq. Maybe because
		 * it is the highest irq number?
		 */
		if (irq == INT_34XX_GPT12_IRQ)
			spurious = omap_check_spurious(irq);

		if (!spurious)
			previous_irq = irq;
	}

	irq &= (IRQ_BITS_PER_REG - 1);

	intc_bank_write_reg(1 << irq, &irq_banks[0], INTC_MIR_SET0 + offset);
}

static void omap_unmask_irq(unsigned int irq)
{
	int offset = irq & (~(IRQ_BITS_PER_REG - 1));

	irq &= (IRQ_BITS_PER_REG - 1);

	intc_bank_write_reg(1 << irq, &irq_banks[0], INTC_MIR_CLEAR0 + offset);
}

static void omap_disable_irq(unsigned int irq)
{
	omap_mask_irq(irq);
}

static void omap_mask_ack_irq(unsigned int irq)
{
	omap_mask_irq(irq);
	omap_ack_irq(irq);
}

static struct irq_chip omap_irq_chip = {
	.name   	= "INTC",
	.ack	    = omap_mask_ack_irq,
	.mask   	= omap_mask_irq,
	.unmask	    = omap_unmask_irq,
	.disable	= omap_disable_irq,
};

static void __init omap_irq_bank_init_one(struct omap_irq_bank *bank)
{
	unsigned long tmp;

	tmp = intc_bank_read_reg(bank, INTC_REVISION) & 0xff;
	printk(KERN_INFO "IRQ: Found an INTC at 0x%p "
			 "(revision %ld.%ld) with %d interrupts\n",
			 bank->base_reg, tmp >> 4, tmp & 0xf, bank->nr_irqs);

	tmp = intc_bank_read_reg(bank, INTC_SYSCONFIG);
	tmp |= 1 << 1;	/* soft reset */
	intc_bank_write_reg(tmp, bank, INTC_SYSCONFIG);

	while (!(intc_bank_read_reg(bank, INTC_SYSSTATUS) & 0x1))
		/* Wait for reset to complete */;

	/* Enable autoidle */
	intc_bank_write_reg(1 << 0, bank, INTC_SYSCONFIG);

        /* Enable protection mode */
        __raw_writel(1 << 0, bank->base_reg + INTC_PROTECTION);
	/* Disable all interrupts by masking them off.  They will be re-enabled when
	   a handler is registered for them. */
	for (tmp = 0; tmp < bank->nr_irqs; tmp += IRQ_BITS_PER_REG)
	{
	    intc_bank_write_reg(0xffffffff, bank,  INTC_MIR_SET0 + tmp);
	}
        /* Set the global enable */
        omap_global_enable();
}

int omap_irq_pending(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(irq_banks); i++) {
		struct omap_irq_bank *bank = irq_banks + i;
		int irq;

		for (irq = 0; irq < bank->nr_irqs; irq += IRQ_BITS_PER_REG) {
			int offset = irq & (~(IRQ_BITS_PER_REG - 1));

			if (intc_bank_read_reg(bank, (INTC_PENDING_IRQ0 +
						      offset)))
				return 1;
		}
	}

	return 0;
}

void __init omap_init_irq(void)
{
	unsigned long nr_of_irqs = 0;
	unsigned int nr_banks = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(irq_banks); i++) {
		struct omap_irq_bank *bank = irq_banks + i;

                if (cpu_is_omapw3g())
			bank->base_reg = OMAP2_IO_ADDRESS(OMAPW3G_IC_BASE);
		else if (cpu_is_omap24xx())
			bank->base_reg = OMAP2_IO_ADDRESS(OMAP24XX_IC_BASE);
		else if (cpu_is_omap34xx())
			bank->base_reg = OMAP2_IO_ADDRESS(OMAP34XX_IC_BASE);

		omap_irq_bank_init_one(bank);

		nr_of_irqs += bank->nr_irqs;
		nr_banks++;
	}

	printk(KERN_INFO "Total of %ld interrupts on %d active controller%s\n",
	       nr_of_irqs, nr_banks, nr_banks > 1 ? "s" : "");

	for (i = 0; i < nr_of_irqs; i++) {
		/* Regardless of reserved IRQ, the interrupt must be handled using kernel's main irq handler */
		omap_irq_set_cfg(i, 0, irq_priority[i]);
		set_irq_chip(i, &omap_irq_chip);
		set_irq_handler(i, handle_level_irq);
		set_irq_flags(i, IRQF_VALID);
	}
}

#ifdef CONFIG_ARCH_OMAP3
void omap3_intc_save_context(void)
{
	int ind = 0, i = 0;
	for (ind = 0; ind < ARRAY_SIZE(irq_banks); ind++) {
		struct omap_irq_bank *bank = irq_banks + ind;
		intc_context[ind].sysconfig =
			intc_bank_read_reg(bank, INTC_SYSCONFIG);
		intc_context[ind].protection =
			intc_bank_read_reg(bank, INTC_PROTECTION);
		intc_context[ind].idle =
			intc_bank_read_reg(bank, INTC_IDLE);
		intc_context[ind].threshold =
			intc_bank_read_reg(bank, INTC_THRESHOLD);
		for (i = 0; i < INTCPS_NR_IRQS; i++)
			intc_context[ind].ilr[i] =
				intc_bank_read_reg(bank, (0x100 + 0x4*i));
		for (i = 0; i < INTCPS_NR_MIR_REGS; i++)
			intc_context[ind].mir[i] =
				intc_bank_read_reg(&irq_banks[0], INTC_MIR0 +
				(0x20 * i));
	}
}

void omap3_intc_restore_context(void)
{
	int ind = 0, i = 0;

	for (ind = 0; ind < ARRAY_SIZE(irq_banks); ind++) {
		struct omap_irq_bank *bank = irq_banks + ind;
		intc_bank_write_reg(intc_context[ind].sysconfig,
					bank, INTC_SYSCONFIG);
		intc_bank_write_reg(intc_context[ind].sysconfig,
					bank, INTC_SYSCONFIG);
		intc_bank_write_reg(intc_context[ind].protection,
					bank, INTC_PROTECTION);
		intc_bank_write_reg(intc_context[ind].idle,
					bank, INTC_IDLE);
		intc_bank_write_reg(intc_context[ind].threshold,
					bank, INTC_THRESHOLD);
		for (i = 0; i < INTCPS_NR_IRQS; i++)
			intc_bank_write_reg(intc_context[ind].ilr[i],
				bank, (0x100 + 0x4*i));
		for (i = 0; i < INTCPS_NR_MIR_REGS; i++)
			intc_bank_write_reg(intc_context[ind].mir[i],
				 &irq_banks[0], INTC_MIR0 + (0x20 * i));
	}
	/* MIRs are saved and restore with other PRCM registers */
}
#endif /* CONFIG_ARCH_OMAP3 */
