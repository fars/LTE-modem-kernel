/*
 * drivers/input/keyboard/w3g_keypad.c
 *
 * Copyright (C) 2009 Motorola, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/input.h>
#include <linux/workqueue.h>

#include <mach/hardware.h>
#include <mach/keypad.h>
#include <mach/io.h>
#include <mach/cpu.h>
#include <mach/control.h>

/* Keyboard controller registers*/
#define OMAPW3G_KBD_REVISION         0x0000
#define OMAPW3G_KBD_SYSCONFIG        0x0010
#define OMAPW3G_KBD_SYSSTATUS        0x0014
#define OMAPW3G_KBD_IRQSTATUS        0x0018
#define OMAPW3G_KBD_IRQENABLE        0x001C
#define OMAPW3G_KBD_WAKEUPENABLE     0x0020
#define OMAPW3G_KBD_PENDING          0x0024
#define OMAPW3G_KBD_CTRL             0x0028
#define OMAPW3G_KBD_DEBOUNCINGTIME   0x002C
#define OMAPW3G_KBD_KEYLONGTIME      0x0030
#define OMAPW3G_KBD_TIMEOUT          0x0034
#define OMAPW3G_KBD_STATEMACHINE     0x0038
#define OMAPW3G_KBD_ROWINPUTS        0x003C
#define OMAPW3G_KBD_COLUMNOUTPUTS    0x0040
#define OMAPW3G_KBD_FULLCODE31_0     0x0044
#define OMAPW3G_KBD_FULLCODE63_32    0x0048

/* keyboard controller register's bit map */
/* SYSCONFIG */
#define CLOCKACTIVITY       0x300
#define EMUFREE             0x020
#define IDELMODE            0x018
#define ENAWAKEUP           0x004
#define SOFTRESET           0x002
#define AUTOGATING          0x001

/* SYSSTATUS */
#define RESETDONE           0x001

/* IRQSTATUS */
#define MISS_EVENT          0x008
#define IT_TIMEOUT          0x004
#define IT_LONG_KEY         0x002
#define IT_EVENT            0x001

/* IRQENABLE */
#define IT_TIMEOUT_EN       0x004
#define IT_LONG_KEY_EN      0x002
#define IT_EVENT_EN         0x001

/* WAKUPENABLE */
#define WUP_TIMEOUT_ENA     0x004
#define WUP_LONG_KEY_ENA    0x002
#define WUP_EVENT_ENA       0x001

/* PENDING */
#define PEND_TIMEOUT        0x008
#define PEND_LONG_KEY       0x004
#define PEND_DEBOUNCEING    0x002
#define PEND_CTRL           0x001

/* CTRL */
#define REPEAT_MODE         0x100
#define TIMEOUT_LONG_KEY    0x080
#define TIMEOUT_EMPTY       0x040
#define LONG_KEY            0x020
#define PTV                 0x01C
#define NSOFTWARE_MODE      0x002

/* STATEMACHINE */
#define STATE_MACHINE       0x0F

/* ROWINPUTS */
#define KBR_LATCH           0xFF

/* COLUMNOUTPUTS */
#define KBC_REG             0xFF

/* Values to write to register */
#define CLOCKACTIVITY_VAL   0x000
#define IDLEMODE_VAL        0x010
#define AUTOGATING_VAL      0x001

/* SCM padconf register for keyboard controller */
#define CONTROL_PADCONF_STM_TXD3    0x01D4
#define CONTROL_PADCONF_KBD_R_1     0x01D8
#define CONTROL_PADCONF_KBD_R_3     0x01DC
#define CONTROL_PADCONF_KBD_R_5     0x01E0
#define CONTROL_PADCONF_KBD_R_7     0x01E4
#define CONTROL_PADCONF_KBD_C_1     0x01E8
#define CONTROL_PADCONF_KBD_C_3     0x01EC
#define CONTROL_PADCONF_KBD_C_5     0x01F0
#define CONTROL_PADCONF_KBD_C_7     0x01F4

/* SCM padconf register bit map*/
#define MUXMODE                     0x00
#define PULLUPENABLE                0x03
#define PULLTYPESELECT              0x04
#define LOADCONTROL_0               0x06
#define LOADCONTROL_1               0x07
#define INPUTENABLE                 0x08
#define STANDBYENABLE               0x09
#define STANDBYOUTENABLE            0x0A
#define STANDBYOUTVAL               0x0B
#define STANDBYPULLEDENABLE         0x0C
#define STANDBYPULLTYPESELECT       0x0D
#define WAKEUPENABLE                0x0E
#define WAKEUPEVENT                 0x0F

/* padconf values for keyboard */
#define KBD_MUXMODE         (0)
#define KBD_PULLUPENABLE    (1)
#define KBD_PULLTYPESELECT  (1)
#define KBD_INPUTENABLE     (1)
#define KBD_WAKEUPENABLE    (1)

#define VAL_SET(bit)    (KBD_##bit << bit)

/* padconf value for rows and cols */
#define ROW_PADCONF_VAL (VAL_SET(MUXMODE) | VAL_SET(PULLUPENABLE) | VAL_SET(PULLTYPESELECT) \
                        VAL_SET(INPUTENABLE) | VAL_SET(WAKEUPENABLE))
#define COL_PADCONF_VAL (VAL_SET(MUXMODE))

#define PADCONF(reg)    (CONTROL_PADCONF_##reg)

/* Read/Write keyboard controller register*/
#define KBD_READ_REG(base, reg) __raw_readl((base) + OMAPW3G_KBD_##reg)
#define KBD_WRITE_REG(base, reg, val)   __raw_writel((val), (base) + OMAPW3G_KBD_##reg)

/* Defines */
#define KBD_INPUT_NAME "w3g-keypad"

struct w3g_kbd {
	struct input_dev *input_dev;
	const unsigned short *keymap;
	unsigned long phybase;
	void __iomem *virtbase;
	int irqno;
	char *input_name;

	unsigned int prev[2];

	struct work_struct irq_work;
	struct workqueue_struct *work_queue;
};

static int kbd_input_event(struct input_dev *ip_dev, unsigned int type,
                                         unsigned int code, int value)
{
    printk(KERN_ALERT "Input event recived\n");
    printk(KERN_ALERT "type: 0x%x, code: 0x%x, value: 0x%x\n", type, code, value);

    return 0;
}

static int kbd_input_open(struct input_dev *ip_dev)
{
    printk(KERN_ALERT "KBD open envoked\n");
    return 0;
}

static void kbd_input_close(struct input_dev *ip_dev)
{
    printk(KERN_ALERT "KBD close envoked\n");
    return;
}


static void report_key(struct w3g_kbd *pkbd, int keycode, int pressed)
{
	if (keycode == KEY_RESERVED) {
		printk(KERN_INFO "w3g_keypad %s: unmapped key pressed %d\n",
			__func__, keycode);
	} else {
		if (pressed)
			printk(KERN_DEBUG "w3g_keypad %s: key %d pressed\n",
				__func__, keycode);
		else
			printk(KERN_DEBUG "w3g_keypad %s: key %d unpressed\n",
				__func__, keycode);
		input_report_key(pkbd->input_dev, keycode, pressed);
	}
}


/* This function intialize driver with the input subsystem */
static int kbd_generic_init(struct w3g_kbd *pkbd, struct platform_device *pdev)
{
    struct input_dev *ip_dev = NULL;
    int i, ret = 0;

    /* Allocate the new input device */
    ip_dev = input_allocate_device();
    if(!ip_dev) {
        printk(KERN_ERR "w3g_keypad: Allocation of input device failed\n");
        return -ENOMEM;
    }

    /* Set the type of event and the bitmap of keys */
    set_bit(EV_KEY, ip_dev->evbit);
    for(i = 0; i < 64; i++) {
        if(pkbd->keymap[i]) {
            set_bit(pkbd->keymap[i] & KEY_MAX, ip_dev->keybit);
        }
    }

    ip_dev->name    = pkbd->input_name;
    ip_dev->open    = kbd_input_open;
    ip_dev->close   = kbd_input_close;
    ip_dev->event   = kbd_input_event;

    /* Initialize the bus type in input subsystem */
//    pkbd->input_dev->dev.parent = &pdev->dev;
//    pkbd->input_dev->id.bustype = BUS_HOST;

    input_set_drvdata(ip_dev, pkbd);
    pkbd->input_dev = ip_dev;

    /* Register with input sub-system */
    ret = input_register_device(ip_dev);
    if(ret) {
        printk(KERN_ERR "Registering with the input subsystem failed\n");
        input_free_device(ip_dev);
        return ret;
    }

    return 0;
}

static void kbd_generic_deinit(struct w3g_kbd *pkbd)
{
    /* Un-register with the input subsystem*/
    input_unregister_device(pkbd->input_dev);

    /* Free the input device allocated */
    input_free_device(pkbd->input_dev);
}


#ifdef CONFIG_PM
static int w3g_kbd_suspend(struct platform_device *pdev, pm_message_t state)
{
	unsigned long val = 0;
	unsigned long clk_addr = (unsigned long)IO_ADDRESS(0x5C184C00 + 0x10);
	struct w3g_kbd *pkbd = platform_get_drvdata(pdev);

	printk(KERN_INFO "%s: beginning\n", __func__);

	/* Disable the event interrupt in KBD controller */
	KBD_WRITE_REG(pkbd->virtbase, IRQENABLE, 0);

	/* Get the interface and functional clock and disable them */
	val = *(unsigned long *)clk_addr;
	val &= ~(1 << 10);
	*(unsigned long *)clk_addr = val;

	printk(KERN_INFO "%s: end\n", __func__);

	return 0;
}

static int w3g_kbd_resume(struct platform_device *pdev)
{
	unsigned long val = 0;
	unsigned long clk_addr = (unsigned long)IO_ADDRESS(0x5C184C00 + 0x10);
	struct w3g_kbd *pkbd = platform_get_drvdata(pdev);

	printk(KERN_INFO "%s: beginning\n", __func__);

	/* Get the interface and functional clock and enable them */
	val = *(unsigned long *)clk_addr;
	val |= (1 << 10);
	*(unsigned long *)clk_addr = val;

	/* Reset the keyboard controller */
	KBD_WRITE_REG(pkbd->virtbase, SYSCONFIG, SOFTRESET);
	/* Wait for the reset to complete */
	while(!(KBD_READ_REG(pkbd->virtbase, SYSSTATUS) & RESETDONE));

	/* configure the keyboard controller */
	val = (CLOCKACTIVITY_VAL | IDLEMODE_VAL | AUTOGATING_VAL);
	KBD_WRITE_REG(pkbd->virtbase, SYSCONFIG, val);

	/* Enable the event interrupt in KBD controller */
	KBD_WRITE_REG(pkbd->virtbase, IRQENABLE, IT_EVENT_EN);

	printk(KERN_INFO "%s: end\n", __func__);

	return 0;
}
#else
#define w3g_kbd_suspend NULL
#define w3g_kbd_resume  NULL
#endif

static irqreturn_t kbd_irq_handler(int irqno, void *dev_id)
{
	struct w3g_kbd *pkbd = (struct w3g_kbd *)dev_id;

	disable_irq_nosync(pkbd->irqno);
        queue_work(pkbd->work_queue, &pkbd->irq_work);

        return IRQ_HANDLED;
}


void w3g_keypad_irq_work_func(struct work_struct *work)
{
	unsigned int val[2] = {0};
        struct w3g_kbd *pkbd = container_of(work, struct w3g_kbd, irq_work);
	unsigned int i, intr;
	unsigned short keyref = 0;
	unsigned short bit;

	intr = KBD_READ_REG(pkbd->virtbase, IRQSTATUS);

	val[0] = KBD_READ_REG(pkbd->virtbase, FULLCODE31_0);
	val[1] = KBD_READ_REG(pkbd->virtbase, FULLCODE63_32);

	for (i=0; i<2; i++) {
		for (bit=0; bit<32; bit++) {
			if (((1<<bit) & val[i]) &&
				(((1<<bit) & pkbd->prev[i])==0)) {
				report_key(pkbd, pkbd->keymap[keyref], 1);
			} else if ((((1<<bit) & val[i])==0) &&
					((1<<bit) & pkbd->prev[i])) {
				report_key(pkbd, pkbd->keymap[keyref], 0);
			}
			keyref++;
		}
		pkbd->prev[i] = val[i];
	}

	/* Reset the interrupt status bits */
	KBD_WRITE_REG(pkbd->virtbase, IRQSTATUS, intr);

	enable_irq(pkbd->irqno);
}


static int w3g_kbd_probe(struct platform_device *pdev)
{
    struct omap_kp_platform_data *pdata = NULL;
    struct w3g_kbd  *pkbd = NULL;
    struct resource *res = NULL;
    unsigned long val = 0;
    int ret;
    unsigned long clk_addr = (unsigned long)IO_ADDRESS(0x5C184C00 + 0x10);

    if(NULL == pdev) {
        printk(KERN_ERR "%s: not proper platform device\n", __func__);
        return -EINVAL;
    }

    pdata = pdev->dev.platform_data;

    /* check whether the platform recived data is proper */
    if(!pdata->keymap) {
        printk(KERN_ERR "%s: missing keymap\n", __func__);
        return -EINVAL;
    }

    pkbd =(struct w3g_kbd * )kzalloc(sizeof(struct w3g_kbd), GFP_KERNEL);
    if(!pkbd)
    {
        printk(KERN_ERR "%s: failed to allocate keypad memory\n", __func__);
        return -ENOMEM;
    }

    /* Populate the private data with platform data */
    pkbd->keymap = pdata->keymap;
    if(pdata->input_name)
        pkbd->input_name = pdata->input_name;
    else
        pkbd->input_name = KBD_INPUT_NAME;

    /* Find the IRQ number and the memory Base address */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    pkbd->irqno = platform_get_irq(pdev, 0);
    if(NULL == res || pkbd->irqno < 0) {
        printk(KERN_ERR "%s: resource not available\n", __func__);
        ret = -ENXIO;
        goto err_platform;
    }

    /* Allocate I/O memory region of keyboard controller*/
    if(!request_mem_region(res->start, res->end - res->start +1, pdev->name)) {
        printk(KERN_ERR "%s: could not get region\n", __func__);
        ret = -EBUSY;
        goto err_requestmem;
    }

    /* Do I/O remap region to kernel address space */
    pkbd->virtbase = ioremap(res->start, SZ_4K);
    if(!pkbd->virtbase)
    {
        printk(KERN_ERR "%s: failed to remap\n", __func__);
        ret = -ENOMEM;
        goto err_remap;
    }

	pkbd->work_queue = create_singlethread_workqueue("w3g_keypad_wq");
	if (!pkbd->work_queue) {
		ret = -ENOMEM;
		printk(KERN_ERR "%s: cannot create work queue %d\n", __func__, ret);
		goto error_create_wq_failed;
	}
	INIT_WORK(&pkbd->irq_work, w3g_keypad_irq_work_func);

	pkbd->prev[0] = 0;
	pkbd->prev[1] = 0;


    /* Register IRQ handler for keypad operations */
    ret = request_irq(pkbd->irqno, kbd_irq_handler, IRQF_DISABLED,
                    "keypad-irq", pkbd);
    if(ret < 0) {
        printk(KERN_ERR "%s: unable to register kbd IRQ handler\n", __func__);
        goto err_irq;
    }


    /* Get the interface and functional clock and enable them */
    val = *(unsigned long *)clk_addr;
    printk(KERN_ALERT "Interface CLK 0x%x\n", (unsigned int)val);
    val |= (1 << 10);
    *(unsigned long *)clk_addr = val;

    /* Reset the keyboard controller */
    KBD_WRITE_REG(pkbd->virtbase, SYSCONFIG, SOFTRESET);

    /* Wait for the reset to complete */
    while(!(KBD_READ_REG(pkbd->virtbase, SYSSTATUS) & RESETDONE));

    /* configure the keyboard controller */
    val = (CLOCKACTIVITY_VAL | IDLEMODE_VAL | AUTOGATING_VAL);
    KBD_WRITE_REG(pkbd->virtbase, SYSCONFIG, val);

    /* Enable software decoding mode*/
    //val = (KBD_READ_REG(pkbd->virtbase, CTRL) & (~NSOFTWARE_MODE));
    //KBD_WRITE_REG(pkbd->virtbase, CTRL, NSOFTWARE_MODE);

    /* Enable the event interrupt in KBD controller */
    KBD_WRITE_REG(pkbd->virtbase, IRQENABLE, IT_EVENT_EN);


    /* Do generic keyboard initilization for software scanning */
    ret = kbd_generic_init(pkbd, pdev);
    if(ret < 0) {
        printk(KERN_ERR "%s: keyboard generic init failed\n", __func__);
        goto err_kbd;
    }

    platform_set_drvdata(pdev, pkbd);

    printk(KERN_ALERT "%s: complete\n", __func__);

    return 0;

err_kbd:
    free_irq(pkbd->irqno, pkbd);

err_irq:
	destroy_workqueue(pkbd->work_queue);

error_create_wq_failed:
    iounmap(pkbd->virtbase);

err_remap:
    release_mem_region(res->start, res->end - res->start + 1);

err_requestmem:
err_platform:
    kfree(pkbd);

    return ret;
}

static int __devexit w3g_kbd_remove(struct platform_device *pdev)
{
    struct w3g_kbd *pkbd = platform_get_drvdata(pdev);
    struct resource *res = NULL;

    /*deinitalize the keyboard */
    kbd_generic_deinit(pkbd);

    free_irq(pkbd->irqno, pkbd);

    /* Unmap the mapped phyical address */
    iounmap(pkbd->virtbase);

    kfree(pkbd);

    /* Release the requested memory region */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    release_mem_region(res->start, res->end - res->start + 1);

    platform_set_drvdata(pdev, NULL);

    return 0;
}


static struct platform_driver w3g_kbd_drv = {
	.probe   = w3g_kbd_probe,
	.remove  = __devexit_p(w3g_kbd_remove),
	.suspend = w3g_kbd_suspend,
	.resume  = w3g_kbd_resume,
	.driver  = {
		.name   = "w3g-keypad",
		.owner  = THIS_MODULE,
	},
};


static int __init w3g_kbd_init(void)
{
	/* Register the keypad controller driver */
	printk(KERN_ALERT "w3g_kbd_init: registering keypad controller\n");
	return platform_driver_register(&w3g_kbd_drv);
}

static void __exit w3g_kbd_exit(void)
{
	/* Unregister the keypad Controller driver */
	platform_driver_unregister(&w3g_kbd_drv);
}
module_init(w3g_kbd_init);
module_exit(w3g_kbd_exit);

MODULE_AUTHOR("Motorola");
MODULE_DESCRIPTION("Wrigley 3G keypad controller driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:keypad-controller");
