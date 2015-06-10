/*
 * Copyright (C) 2007 - 2009 Motorola, Inc.
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

#include <linux/errno.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/spi/cpcap.h>
#include <linux/spi/cpcap-regbits.h>

struct cpcap_usb_adptr_data {
	struct cpcap_device *cpcap;
#if defined(CONFIG_CHARGER_BQ24113A)	
	struct platform_device *bq24113a_charger_dev;
#endif	
};

static int __init cpcap_usb_adptr_det_probe(struct platform_device *pdev)
{
	int retval;
	struct cpcap_usb_adptr_data *data;
	unsigned short value = -EFAULT;

	if (pdev->dev.platform_data == NULL) {
		dev_err(&pdev->dev, "no platform_data\n");
		return -EINVAL;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->cpcap = pdev->dev.platform_data;
	platform_set_drvdata(pdev, data);

	dev_info(&pdev->dev, "USB adaptor detection device probed\n");

	/*Initialize CPCAP GPIO3 as input operating at PRISPI VCC with Pullup enabled*/
	retval = cpcap_regacc_write(data->cpcap,
				CPCAP_REG_GPIO3,
				CPCAP_BIT_GPIO3PUEN,
				CPCAP_BIT_GPIO3PUEN | CPCAP_BIT_GPIO3DIR);
	/*Initialize CPCAP GPIO4 as output low operating at PRISPI VCC with Pullup disabled*/
	retval |= cpcap_regacc_write(data->cpcap,
				CPCAP_REG_GPIO4,
				CPCAP_BIT_GPIO4DIR,
				CPCAP_BIT_GPIO4DRV| CPCAP_BIT_GPIO4DIR| CPCAP_BIT_GPIO4PUEN);

	retval |= cpcap_regacc_read(data->cpcap, CPCAP_REG_GPIO3, &value);
	if(retval != 0)
	{
		retval = -EFAULT;
		goto free_mem;
	}

	if(!(value & CPCAP_BIT_GPIO3S)) {
		/* USB dongle Present */
		dev_info(&pdev->dev, "USB adaptor connected\n");
#if defined(CONFIG_CHARGER_BQ24113A)
		data->bq24113a_charger_dev = platform_device_alloc("bq24113a_charger", -1);
		data->bq24113a_charger_dev->dev.platform_data = data->cpcap;
		platform_device_add(data->bq24113a_charger_dev);
#endif

	} else {
		dev_info(&pdev->dev, "USB adaptor not connected\n");
		retval = cpcap_regacc_write(data->cpcap,
					CPCAP_REG_GPIO3,
					0,
					CPCAP_BIT_GPIO3PUEN | CPCAP_BIT_GPIO3DIR);
		retval |= cpcap_regacc_write(data->cpcap,
					CPCAP_REG_GPIO4,
					0,
					CPCAP_BIT_GPIO4DIR| CPCAP_BIT_GPIO4PUEN);
		if(retval != 0)
		{
			retval = -EFAULT;
			goto free_mem;
		}
	}
	return 0;

free_mem:
	kfree(data);

	return retval;
}

static int __exit cpcap_usb_adtr_det_remove(struct platform_device *pdev)
{
	struct cpcap_usb_adptr_data *data = platform_get_drvdata(pdev);

#if defined(CONFIG_CHARGER_BQ24113A)
	if (data->bq24113a_charger_dev != NULL)
		platform_device_del(data->bq24113a_charger_dev);
#endif	

	kfree(data);
	return 0;
}


static struct platform_driver cpcap_usb_adptr_det_driver = {
	.probe		= cpcap_usb_adptr_det_probe,
	.remove		= __exit_p(cpcap_usb_adptr_det_remove),
	.driver		= {
		.name	= "cpcap_usb_adptr_det",
		.owner	= THIS_MODULE,
	},
};

static int __init cpcap_usb_adptr_det_init(void)
{
	return platform_driver_register(&cpcap_usb_adptr_det_driver);
}
module_init(cpcap_usb_adptr_det_init);

static void __exit cpcap_usb_adptr_det_exit(void)
{
	platform_driver_unregister(&cpcap_usb_adptr_det_driver);
}
module_exit(cpcap_usb_adptr_det_exit);

MODULE_ALIAS("platform:cpcap_usb_adptr_det");
MODULE_DESCRIPTION("CPCAP USB adaptor detection driver");
MODULE_LICENSE("GPL");
