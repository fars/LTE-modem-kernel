/*
 * Copyright (C) 2009 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#include <linux/err.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/leds-cpcap-bt.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/spi/cpcap.h>
#include <linux/spi/cpcap-regbits.h>

#ifdef CONFIG_ARM_OF
#include <mach/dt_path.h>
#include <asm/prom.h>
#endif

struct bt_led {
	struct cpcap_device *cpcap;
	struct led_classdev cpcap_bt_dev;
	struct regulator *regulator;
	int regulator_state;
};

static void cpcap_bt_led_set(struct led_classdev *led_cdev,
			  enum led_brightness value)
{
	int status = 0;
	struct cpcap_platform_data *data;
	struct spi_device *spi;
	struct cpcap_leds *leds;

	struct bt_led *bt_led =
	    container_of(led_cdev, struct bt_led,
			 cpcap_bt_dev);

	spi = bt_led->cpcap->spi;
	data = (struct cpcap_platform_data *)spi->controller_data;
	leds = data->leds;

	if (value > LED_OFF) {
		if ((bt_led->regulator) &&
		    (bt_led->regulator_state == 0)) {
			regulator_enable(bt_led->regulator);
			bt_led->regulator_state = 1;
		}
		status = cpcap_regacc_write(bt_led->cpcap,
					    leds->bt_led.bt_reg,
					    leds->bt_led.bt_on,
					    leds->bt_led.bt_mask);
	} else {
		status = cpcap_regacc_write(bt_led->cpcap,
					    leds->bt_led.bt_reg,
					    0x0001,
					    leds->bt_led.bt_mask);
		status = cpcap_regacc_write(bt_led->cpcap,
					    leds->bt_led.bt_reg,
					    0x0000,
					    leds->bt_led.bt_mask);
		if ((bt_led->regulator) &&
		    (bt_led->regulator_state == 1)) {
			regulator_disable(bt_led->regulator);
			bt_led->regulator_state = 0;
		}
	}
	if (status < 0)
		pr_err("%s: Writing to the register failed for %i\n",
		       __func__, status);

	return;
}

static int cpcap_bt_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct bt_led *led;

	if (pdev == NULL) {
		pr_err("%s: platform data required %d\n", __func__, -ENODEV);
		return -ENODEV;

	}
	led = kzalloc(sizeof(struct bt_led), GFP_KERNEL);
	if (led == NULL) {
		pr_err("%s: Unable to allocate memory %d\n", __func__, -ENOMEM);
		ret = -ENOMEM;
	}

	led->cpcap = pdev->dev.platform_data;
	platform_set_drvdata(pdev, led);

	led->regulator = regulator_get(&pdev->dev, "sw5");
	if (IS_ERR(led->regulator)) {
		pr_err("%s: Cannot get %s regulator\n", __func__, "sw5");
		ret = PTR_ERR(led->regulator);
		goto exit_request_reg_failed;
	}

	led->regulator_state = 0;

	led->cpcap_bt_dev.name = CPCAP_BT_DEV;
	led->cpcap_bt_dev.brightness_set = cpcap_bt_led_set;
	ret = led_classdev_register(&pdev->dev, &led->cpcap_bt_dev);
	if (ret) {
		printk(KERN_ERR "Register bt led class failed %d\n", ret);
		goto err_reg_bt_failed;
	}

	return ret;

err_reg_bt_failed:
	if (led->regulator)
		regulator_put(led->regulator);

exit_request_reg_failed:
	kfree(led);
	return ret;
}

static int cpcap_bt_led_remove(struct platform_device *pdev)
{
	struct bt_led *led = platform_get_drvdata(pdev);

	if (led->regulator)
		regulator_put(led->regulator);

	led_classdev_unregister(&led->cpcap_bt_dev);
	kfree(led);
	return 0;
}

static struct platform_driver cpcap_bt_driver = {
	.probe   = cpcap_bt_led_probe,
	.remove  = cpcap_bt_led_remove,
	.driver  = {
		.name   = CPCAP_BT_DEV,
        .owner  = THIS_MODULE,
	},
};

#ifdef CONFIG_ARM_OF
static int lights_of_init(void)
{
	u8 device_available;
	struct device_node *node;
	const void *prop;

	node = of_find_node_by_path(DT_NOTIFICATION_LED);
	if (node == NULL) {
		pr_err("Unable to read node %s from device tree!\n",
			DT_NOTIFICATION_LED);
		return -ENODEV;
	}

	prop = of_get_property(node, "bt_led", NULL);
	if (prop)
		device_available = *(u8 *)prop;
	else {
		pr_err("Read property %s error!\n", DT_PROP_BT_LED);
		of_node_put(node);
		return -ENODEV;
	}

	of_node_put(node);
	return device_available;
}
#endif

static int __init cpcap_bt_led_init(void)
{
#ifdef CONFIG_ARM_OF
#if 0
	int err = lights_of_init();
	if (err <= 0) {
		pr_err("BT led device declared unavailable: %d\n",
			err);
		return err;
	}
#endif    
#endif
	return platform_driver_register(&cpcap_bt_driver);
}

static void __exit cpcap_bt_led_shutdown(void)
{
	platform_driver_unregister(&cpcap_bt_driver);
}

module_init(cpcap_bt_led_init);
module_exit(cpcap_bt_led_shutdown);

MODULE_DESCRIPTION("Icon/bt Lighting Driver");
MODULE_AUTHOR("Motorola");
MODULE_LICENSE("GNU");






