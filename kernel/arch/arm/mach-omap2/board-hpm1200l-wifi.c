/* linux/arch/arm/mach-omap2/board-hpm1200l-wifi.c
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/wifi_tiwlan.h>

#include <linux/debugfs.h>

#define WIFI_PMENA_GPIO	186
#define WIFI_IRQ_GPIO	65

static int hpm1200l_wifi_cd;	/* WIFI virtual 'card detect' status */
static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

int hpm1200l_wifi_status_register(void (*callback)(int card_present,
						void *dev_id), void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

int hpm1200l_wifi_status(int irq)
{
	return hpm1200l_wifi_cd;
}

int hpm1200l_wifi_set_carddetect(int val)
{
	printk("%s: %d\n", __func__, val);
	hpm1200l_wifi_cd = val;
	if (wifi_status_cb) {
		wifi_status_cb(val, wifi_status_cb_devid);
	} else
		printk(KERN_WARNING "%s: Nobody to notify\n", __func__);
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(hpm1200l_wifi_set_carddetect);
#endif

static int hpm1200l_wifi_power_state;

int hpm1200l_wifi_power(int on)
{
	printk("%s: %d\n", __func__, on);
	gpio_set_value(WIFI_PMENA_GPIO, on);
	hpm1200l_wifi_power_state = on;
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(hpm1200l_wifi_power);
#endif

static int hpm1200l_wifi_reset_state;
int hpm1200l_wifi_reset(int on)
{
	printk("%s: %d\n", __func__, on);
	hpm1200l_wifi_reset_state = on;
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(hpm1200l_wifi_reset);
#endif

struct wifi_platform_data hpm1200l_wifi_control = {
	.set_power	= hpm1200l_wifi_power,
	.set_reset	= hpm1200l_wifi_reset,
	.set_carddetect	= hpm1200l_wifi_set_carddetect,
};

#ifdef CONFIG_WIFI_CONTROL_FUNC
static struct resource hpm1200l_wifi_resources[] = {
	[0] = {
		.name		= "device_wifi_irq",
		.start		= OMAP_GPIO_IRQ(WIFI_IRQ_GPIO),
		.end		= OMAP_GPIO_IRQ(WIFI_IRQ_GPIO),
		.flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE,
	},
};

static struct platform_device hpm1200l_wifi_device = {
	.name           = "device_wifi",
	.id             = 1,
	.num_resources  = ARRAY_SIZE(hpm1200l_wifi_resources),
	.resource       = hpm1200l_wifi_resources,
	.dev            = {
		.platform_data = &hpm1200l_wifi_control,
	},
};
#endif

static int __init hpm1200l_wifi_init(void)
{
	int ret;

	printk("%s: start\n", __func__);
	ret = gpio_request(WIFI_IRQ_GPIO, "wifi_irq");
	if (ret < 0) {
		printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
			WIFI_IRQ_GPIO);
		goto out;
	}
	ret = gpio_request(WIFI_PMENA_GPIO, "wifi_pmena");
	if (ret < 0) {
		printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
			WIFI_PMENA_GPIO);
		gpio_free(WIFI_IRQ_GPIO);
		goto out;
	}
	gpio_direction_input(WIFI_IRQ_GPIO);
	gpio_direction_output(WIFI_PMENA_GPIO, 0);
#ifdef CONFIG_WIFI_CONTROL_FUNC
	ret = platform_device_register(&hpm1200l_wifi_device);
#endif
out:
	return ret;
}

device_initcall(hpm1200l_wifi_init);

#if defined(CONFIG_DEBUG_FS)

static int hpm1200lmmc_dbg_wifi_reset_set(void *data, u64 val)
{
	hpm1200l_wifi_reset((int) val);
	return 0;
}

static int hpm1200lmmc_dbg_wifi_reset_get(void *data, u64 *val)
{
	*val = hpm1200l_wifi_reset_state;
	return 0;
}

static int hpm1200lmmc_dbg_wifi_cd_set(void *data, u64 val)
{
	hpm1200l_wifi_set_carddetect((int) val);
	return 0;
}

static int hpm1200lmmc_dbg_wifi_cd_get(void *data, u64 *val)
{
	*val = hpm1200l_wifi_cd;
	return 0;
}

static int hpm1200lmmc_dbg_wifi_pwr_set(void *data, u64 val)
{
	hpm1200l_wifi_power((int) val);
	return 0;
}

static int hpm1200lmmc_dbg_wifi_pwr_get(void *data, u64 *val)
{
	*val = hpm1200l_wifi_power_state;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(hpm1200lmmc_dbg_wifi_reset_fops,
			hpm1200lmmc_dbg_wifi_reset_get,
			hpm1200lmmc_dbg_wifi_reset_set, "%llu\n");

DEFINE_SIMPLE_ATTRIBUTE(hpm1200lmmc_dbg_wifi_cd_fops,
			hpm1200lmmc_dbg_wifi_cd_get,
			hpm1200lmmc_dbg_wifi_cd_set, "%llu\n");

DEFINE_SIMPLE_ATTRIBUTE(hpm1200lmmc_dbg_wifi_pwr_fops,
			hpm1200lmmc_dbg_wifi_pwr_get,
			hpm1200lmmc_dbg_wifi_pwr_set, "%llu\n");

static int __init hpm1200lmmc_dbg_init(void)
{
	struct dentry *dent;

	dent = debugfs_create_dir("hpm1200l_mmc_dbg", 0);
	if (IS_ERR(dent))
		return PTR_ERR(dent);

	debugfs_create_file("wifi_reset", 0644, dent, NULL,
			    &hpm1200lmmc_dbg_wifi_reset_fops);
	debugfs_create_file("wifi_cd", 0644, dent, NULL,
			    &hpm1200lmmc_dbg_wifi_cd_fops);
	debugfs_create_file("wifi_pwr", 0644, dent, NULL,
			    &hpm1200lmmc_dbg_wifi_pwr_fops);
	return 0;
}

device_initcall(hpm1200lmmc_dbg_init);
#endif
