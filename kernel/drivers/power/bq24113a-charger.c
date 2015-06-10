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

struct bq24113a_charger_data {
	struct cpcap_device *cpcap;
	unsigned int		mA;
	struct work_struct	work;
};

static struct bq24113a_charger_data *bq24113a_data = NULL;

static void bq24113a_charger_work(struct work_struct *work)
{
	struct cpcap_device *cpcap = bq24113a_data->cpcap;

	if(500 == bq24113a_data->mA)
	{
		cpcap_regacc_write(cpcap,
			CPCAP_REG_GPIO3,
			CPCAP_BIT_GPIO3DRV | CPCAP_BIT_GPIO3DIR,
			CPCAP_BIT_GPIO3DRV | CPCAP_BIT_GPIO3DIR | CPCAP_BIT_GPIO3PUEN);

		cpcap_regacc_write(cpcap,
			CPCAP_REG_GPIO4,
			CPCAP_BIT_GPIO4DIR,
			CPCAP_BIT_GPIO4DRV | CPCAP_BIT_GPIO4DIR |
			CPCAP_BIT_GPIO4PUEN);
	}
	else if(0 == bq24113a_data->mA)
	{
		cpcap_regacc_write(cpcap,
			CPCAP_REG_GPIO4,
			CPCAP_BIT_GPIO4DIR |CPCAP_BIT_GPIO4DRV ,
			CPCAP_BIT_GPIO4DRV | CPCAP_BIT_GPIO4DIR |
			CPCAP_BIT_GPIO4PUEN);
	}
	else
	{
		cpcap_regacc_write(cpcap,
			CPCAP_REG_GPIO3,
			CPCAP_BIT_GPIO3DIR,
			CPCAP_BIT_GPIO3DRV | CPCAP_BIT_GPIO3DIR |
			CPCAP_BIT_GPIO3PUEN);

		cpcap_regacc_write(cpcap,
			CPCAP_REG_GPIO4,
			CPCAP_BIT_GPIO4DIR,
			CPCAP_BIT_GPIO4DRV | CPCAP_BIT_GPIO4DIR |
			CPCAP_BIT_GPIO4PUEN);
	}

}

void bq24113a_charger_set_usb_prop_curr(struct cpcap_device *cpcap, unsigned int curr)
{
	if(!bq24113a_data)
		return;

	bq24113a_data->mA = curr;
	schedule_work(&bq24113a_data->work);
}

EXPORT_SYMBOL(bq24113a_charger_set_usb_prop_curr);

static int __init bq24113a_charger_probe(struct platform_device *pdev)
{
	if (pdev->dev.platform_data == NULL) {
		dev_err(&pdev->dev, "no platform_data\n");
		return -EINVAL;
	}

	bq24113a_data = kzalloc(sizeof(*bq24113a_data), GFP_KERNEL);
	if (!bq24113a_data)
		return -ENOMEM;

	bq24113a_data->cpcap = pdev->dev.platform_data;

	INIT_WORK(&bq24113a_data->work, bq24113a_charger_work);

	bq24113a_charger_set_usb_prop_curr(bq24113a_data->cpcap, 100);

	return 0;
}

static int __exit bq24113a_charger_remove(struct platform_device *pdev)
{
	flush_scheduled_work();
	kfree(bq24113a_data);
	bq24113a_data = NULL;
	return 0;
}


static struct platform_driver bq24113a_charger_driver = {
	.probe		= bq24113a_charger_probe,
	.remove		= __exit_p(bq24113a_charger_remove),
	.driver		= {
		.name	= "bq24113a_charger",
		.owner	= THIS_MODULE,
	},
};

static int __init bq24113a_charger_init(void)
{
	return platform_driver_register(&bq24113a_charger_driver);
}
module_init(bq24113a_charger_init);

static void __exit bq24113a_charger_exit(void)
{
	platform_driver_unregister(&bq24113a_charger_driver);
}
module_exit(bq24113a_charger_exit);

MODULE_ALIAS("platform:bq24113a_charger");
MODULE_DESCRIPTION("bq24113a battery charger driver");
MODULE_AUTHOR("Motorola");
MODULE_LICENSE("GPL");
