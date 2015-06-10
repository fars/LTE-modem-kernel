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
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/leds-cpcap-display.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/spi/cpcap.h>
#include <linux/spi/cpcap-regbits.h>
#include <linux/earlysuspend.h>

struct display_led {
	struct input_dev *idev;
	struct platform_device *pdev;
	struct cpcap_device *cpcap;
	struct led_classdev cpcap_display_dev;
	struct early_suspend suspend;
	struct cpcap_leds *leds;
	struct regulator *regulator;
	int regulator_state;
};

static void cpcap_display_led_set(struct led_classdev *led_cdev,
			  enum led_brightness value)
{
	int status = 0;
	unsigned short backlight_set = 0;
	struct cpcap_leds *leds;
	struct display_led *display_led =
	    container_of(led_cdev, struct display_led,
			 cpcap_display_dev);

	leds = display_led->leds;

	if (value > LED_OFF) {
		if ((display_led->regulator) &&
		    (display_led->regulator_state == 0)) {
			regulator_enable(display_led->regulator);
			display_led->regulator_state = 1;
		}
		backlight_set = (value / 2) << 5;
		backlight_set |= leds->display_led.display_init;
		status = cpcap_regacc_write(display_led->cpcap,
					  leds->display_led.display_reg,
					  backlight_set,
					  leds->display_led.display_mask);
	} else {
		status = cpcap_regacc_write(display_led->cpcap,
					  leds->display_led.display_reg,
					  leds->display_led.pwm_off,
					  leds->display_led.display_mask);
		status = cpcap_regacc_write(display_led->cpcap,
					  leds->display_led.display_reg,
					  leds->display_led.display_off,
					  leds->display_led.display_mask);
		if ((display_led->regulator) &&
		    (display_led->regulator_state == 1)) {
			regulator_disable(display_led->regulator);
			display_led->regulator_state = 0;
		}
	}
	if (status < 0)
		pr_err("%s: Writing to the register failed for %i\n",
		       __func__, status);
	return;
}

static int cpcap_display_led_suspend(struct platform_device *pdev,
				     pm_message_t state)
{
	struct display_led *d_led = platform_get_drvdata(pdev);

	if (d_led->regulator_state) {
		regulator_disable(d_led->regulator);
		d_led->regulator_state = 0;
	}
	return 0;
}

static int cpcap_display_led_resume(struct platform_device *pdev)
{
	struct display_led *d_led = platform_get_drvdata(pdev);

	if (!d_led->regulator_state) {
		regulator_enable(d_led->regulator);
		d_led->regulator_state = 1;
	}
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cpcap_display_led_early_suspend(struct early_suspend *h)
{
	struct display_led *d_led =
	  container_of(h, struct display_led, suspend);

	cpcap_display_led_suspend(d_led->pdev, PMSG_SUSPEND);
}

static void cpcap_display_led_late_resume(struct early_suspend *h)
{
	struct display_led *d_led =
	  container_of(h, struct display_led, suspend);

	cpcap_display_led_resume(d_led->pdev);
}

static struct early_suspend cpcap_display_early_suspend_handler = {
	.suspend = cpcap_display_led_early_suspend,
	.resume = cpcap_display_led_late_resume,
};
#endif

static int cpcap_display_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct display_led *d_led;

	struct cpcap_platform_data *data;
	struct spi_device *spi;

	if (pdev == NULL) {
		pr_err("%s: platform data required %d\n", __func__, -ENODEV);
		return -ENODEV;

	}
	d_led = kzalloc(sizeof(struct display_led), GFP_KERNEL);
	if (d_led == NULL) {
		pr_err("%s: Unable to allacate memory %d\n", __func__, -ENOMEM);
		ret = -ENOMEM;
	}

	d_led->pdev = pdev;
	d_led->cpcap = pdev->dev.platform_data;
	spi = d_led->cpcap->spi;
	data = (struct cpcap_platform_data *)spi->controller_data;
	d_led->leds = data->leds;
	platform_set_drvdata(pdev, d_led);

	d_led->idev = input_allocate_device();
	if (!d_led->idev) {
		ret = -ENOMEM;
		pr_err("%s: input device allocate failed: %d\n", __func__,
		       ret);
		goto err_input_allocate_failed;
	}

	d_led->idev->name = BACKLIGHT_ALS;
	input_set_capability(d_led->idev, EV_MSC, MSC_RAW);

	if (input_register_device(d_led->idev)) {
		pr_err("%s: input device register failed\n", __func__);
		goto err_input_register_failed;
	}

	d_led->regulator = regulator_get(&pdev->dev, "sw5");
	if (IS_ERR(d_led->regulator)) {
		pr_err("%s: Cannot get %s regulator\n", __func__, "sw5");
		ret = PTR_ERR(d_led->regulator);
		goto err_reg_request_failed;

	}
	d_led->regulator_state = 0;

	d_led->cpcap_display_dev.name = CPCAP_DISPLAY_DEV;
	d_led->cpcap_display_dev.brightness_set = cpcap_display_led_set;
	ret = led_classdev_register(&pdev->dev, &d_led->cpcap_display_dev);
	if (ret) {
		printk(KERN_ERR "Register display led class failed %d\n", ret);
		goto err_reg_led_class_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	d_led->suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	d_led->suspend.suspend = cpcap_display_led_early_suspend;
	d_led->suspend.resume = cpcap_display_led_late_resume;
	register_early_suspend(&d_led->suspend);
#endif
	return ret;

err_reg_led_class_failed:
	if (d_led->regulator)
		regulator_put(d_led->regulator);
err_reg_request_failed:
err_input_register_failed:
	input_free_device(d_led->idev);
err_input_allocate_failed:
	kfree(d_led);
	return ret;
}

static int cpcap_display_led_remove(struct platform_device *pdev)
{
	struct display_led *d_led = platform_get_drvdata(pdev);

	if (d_led->regulator)
		regulator_put(d_led->regulator);

	led_classdev_unregister(&d_led->cpcap_display_dev);
	unregister_early_suspend(&cpcap_display_early_suspend_handler);
	input_unregister_device(d_led->idev);
	kfree(d_led);
	return 0;
}

static struct platform_driver cpcap_display_led_driver = {
	.probe   = cpcap_display_led_probe,
	.remove  = cpcap_display_led_remove,
	.suspend = cpcap_display_led_suspend,
	.resume  = cpcap_display_led_resume,
	.driver  = {
		.name   = CPCAP_DISPLAY_DEV,
		.owner  = THIS_MODULE,
	},
};

static int __init cpcap_display_led_init(void)
{
	return platform_driver_register(&cpcap_display_led_driver);
}

static void __exit cpcap_display_led_shutdown(void)
{
	platform_driver_unregister(&cpcap_display_led_driver);
}

module_init(cpcap_display_led_init);
module_exit(cpcap_display_led_shutdown);

MODULE_DESCRIPTION("Display Lighting Driver");
MODULE_AUTHOR("Motorola");
MODULE_LICENSE("GNU");






