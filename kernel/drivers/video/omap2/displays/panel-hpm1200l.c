#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>

#include <mach/display.h>
#include <mach/dma.h>

/* #define DEBUG */
#ifdef DEBUG
#define DBG(format, ...) (printk(KERN_DEBUG "hpm1200l-panel: " \
	format, ## __VA_ARGS__))
#else
#define DBG(format, ...)
#endif


#define COMMAND		0
#define DATA		1
#define DATA_READ	2
#define DELAY_MS	3


static struct omap_video_timings video_timings = {
	.x_res          = 240,
	.y_res          = 320,
};

/* timing values set according to  71014055001.pdf issue 01 of 05/19/09 */
/* values are set in "ps" to suit the future calculation with L4 set in KHz */
static struct rfbi_timings panel_timings_in_ps = {
	.cs_on_time     =   0 * 1000,
	.cs_off_time    =  70 * 1000,
	.we_on_time     =  10 * 1000,
	.we_off_time    =  40 * 1000, /* we_off_time = we_on_time + 30 */
	.re_on_time     =  10 * 1000,
	.re_off_time    = 160 * 1000, /* re_off_time = re_on_time + 150 */
	.we_cycle_time  =  75 * 1000,
	.re_cycle_time  = 300 * 1000,
	.cs_pulse_width =  70 * 1000,
	.access_time    = 170 * 1000, /* access time = re_on_time + 10 */
/*	.clk_div	= 1,*/
};


static void panel_setup_update(struct omap_dss_device *dssdev,
				      u16 x, u16 y, u16 w, u16 h);


struct  display_command {
	unsigned int		type;
	unsigned int		value;
};

struct  display_command w3g_display_enable[] = {
	DELAY_MS,	50,

	/* driver output control */
	COMMAND,	0x01,
	DATA,		0x0100,
	/* set inversion */
	COMMAND,	0x02,
	DATA,		0x0200,
	/* set entry mode */
	COMMAND,	0x03,
	DATA,		0x1030,
	/* set back & front porch */
	COMMAND,	0x08,
	DATA,		0x0202,
	/* set scan interval */
	COMMAND,	0x09,
	DATA,		0x0,
	/* set display control 1 */
	COMMAND,	0x0A,
	DATA,		0x0,
	/* set frame mark position */
	COMMAND,	0x0D,
	DATA,		0x0,
	/* set gate scan control */
	COMMAND,	0x60,
	DATA,		0x2700,
	/* Normally white */
	COMMAND,	0x61,
	DATA,		0x0001,
	/* set gate scan control */
	COMMAND,	0x6A,
	DATA,		0x0,

	DELAY_MS,	10,

	/* set BT, STB & SLP */
	COMMAND,	0x10,
	DATA,		0x1490,
	/* set VCI set up circuits */
	COMMAND,	0x11,
	DATA,		0x0667,

	DELAY_MS,	80,

	/* set VREGOUT1 */
	COMMAND,	0x12,
	DATA,		0x000A,

	DELAY_MS,	10,

	/* set VCOMAC */
	COMMAND,	0x13,
	DATA,		0x1900,
	/* set VCOMH */
	COMMAND,	0x29,
	DATA,		0x0029,
	/* set frame rate */
	COMMAND,	0x2B,
	DATA,		0x000B,

	DELAY_MS,	10,

	/* set GRAM horizontal address */
	COMMAND,	0x20,
	DATA,		0x0,
	/* set GRAM vertical  address */
	COMMAND,	0x21,
	DATA,		0x0,

	/* Gamma Settings */
	COMMAND,	0x30,
	DATA,		0x0004,
	COMMAND,	0x31,
	DATA,		0x0207,
	COMMAND,	0x32,
	DATA,		0x0400,
	COMMAND,	0x35,
	DATA,		0x0404,
	COMMAND,	0x36,
	DATA,		0x0404,
	COMMAND,	0x37,
	DATA,		0x0703,
	COMMAND,	0x38,
	DATA,		0x0005,
	COMMAND,	0x39,
	DATA,		0x0307,
	COMMAND,	0x3c,
	DATA,		0x0404,
	COMMAND,	0x3d,
	DATA,		0x0006,


	/* set RAM window */
	COMMAND,	0x50,
	DATA,		0x0,
	COMMAND,	0x51,
	DATA,		0x00EF,
	COMMAND,	0x52,
	DATA,		0x0,
	COMMAND,	0x53,
	DATA,		0x13F,

	DELAY_MS,	10,

	/* display on */
	COMMAND,	0x07,
	DATA,		0x0133
};

const unsigned int W3G_DISPLAY_ENABLE_NUM =
    sizeof(w3g_display_enable) / sizeof(w3g_display_enable[0]);

static int panel_probe(struct omap_dss_device *dssdev)
{
	DBG("%s\n", __func__);
	dssdev->ctrl.pixel_size = 16;
	dssdev->ctrl.rfbi_timings = panel_timings_in_ps;
	dssdev->panel.config = OMAP_DSS_LCD_TFT;
	dssdev->panel.timings = video_timings;
	return 0;
}

static void panel_remove(struct omap_dss_device *dssdev)
{
	return;
}

static void write(unsigned short *index, unsigned short *value)
{
	omap_rfbi_write_command((void *)index, sizeof(index));
	omap_rfbi_write_data((void *)value, sizeof(value));
}

#if 0 /* debug picture buffer */
u16 *temp_disp_buf[320 * 240];
#endif

static int panel_enable(struct omap_dss_device *dssdev)
{
	int ret;
	unsigned short rfbi_command_size;
	unsigned short rfbi_data_size;
	unsigned short rfbi_data_read;
	unsigned int   ii;

	DBG("%s\n", __func__);

	if (dssdev->platform_enable) {
		ret = dssdev->platform_enable(dssdev);
		if (ret)
			return ret;
	}

	if (OMAP_DISPLAY_TYPE_DBI == dssdev->type) {
		rfbi_command_size = 2; /* reg address size in bytes */
		rfbi_data_size	= 2; /* reg value size in bytes */

		for (ii = 0; ii < W3G_DISPLAY_ENABLE_NUM; ii++) {
			switch (w3g_display_enable[ii].type) {
			case COMMAND:
			{
				omap_rfbi_write_command(
					(void *)&(w3g_display_enable[ii].value),
					rfbi_command_size);
			} break;

			case DATA:
			{
				omap_rfbi_write_data(
					(void *)&(w3g_display_enable[ii].value),
					rfbi_data_size);
			} break;

			case DATA_READ:
			{
				omap_rfbi_read_data(
					(void *)&(rfbi_data_read),
					rfbi_data_size);
			} break;

			case DELAY_MS:
			{
				msleep(w3g_display_enable[ii].value);
			} break;

			default:
			{
				/* TODO: report unsupported type */
			}
			} /* switch (w3g_display_enable[ii].type) */
		}
	}


#if 0 /* debug picture preparation */
	for (ii = 0; ii < 80 * 240; ii++)
		temp_disp_buf[ii] = 0xFFFF; /* fill white */

	for (; ii < 160 * 240; ii++)
		temp_disp_buf[ii] = 0xF812; /* fill dark red */

	for (; ii < 240 * 240; ii++)
		temp_disp_buf[ii] = 0xFFFF; /* fill white */

	for (; ii < 300 * 240; ii++)
		temp_disp_buf[ii] = 0x101F; /* fill dark blue */

	for (; ii < 320 * 240; ii++)
		temp_disp_buf[ii] = 0xFFFF; /* fill white */


	panel_setup_update(NULL, 0, 0, 0, 0);
	omap_rfbi_write_pixels(temp_disp_buf, 240, 0, 0, 240, 320);
#endif

	return 0;
}

static void panel_disable(struct omap_dss_device *dssdev)
{
	DBG("%s\n", __func__);

	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);
}


static void panel_setup_update(struct omap_dss_device *dssdev,
				      u16 x, u16 y, u16 w, u16 h)
{
	unsigned short index, value;

	DBG("%s\n", __func__);

	value = 0x0000;

	index = 0x0020;
	omap_rfbi_write_command(&index, 2);
	omap_rfbi_write_data(&value, 2);

	index = 0x0021;
	omap_rfbi_write_command(&index, 2);
	omap_rfbi_write_data(&value, 2);

	index = 0x0022;
	omap_rfbi_write_command(&index, 2);
}

static int panel_enable_te(struct omap_dss_device *dssdev, bool enable)
{
	DBG("%s\n", __func__);
	return 0;
}

static int panel_rotate(struct omap_dss_device *display, u8 rotate)
{
	DBG("%s\n", __func__);
	return 0;
}

static int panel_mirror(struct omap_dss_device *display, bool enable)
{
	DBG("%s\n", __func__);
	return 0;
}

static int panel_suspend(struct omap_dss_device *dssdev)
{
	DBG("%s\n", __func__);
	panel_disable(dssdev);
	return 0;
}

static int panel_resume(struct omap_dss_device *dssdev)
{
	DBG("%s\n", __func__);
	return panel_enable(dssdev);
}

static bool panel_te_support(struct omap_dss_device *dssdev)
{
	DBG("%s\n", __func__);
	return false;
}

static int panel_run_test(struct omap_dss_device *display,
				int test_num)
{
	DBG("%s\n", __func__);
	return 0;
}

static struct omap_dss_driver panel_driver = {
	.probe = panel_probe,
	.remove = panel_remove,

	.enable = panel_enable,
	.disable = panel_disable,
	.suspend = panel_suspend,
	.resume = panel_resume,
	.setup_update = panel_setup_update,
	.enable_te = panel_enable_te,
	.te_support = panel_te_support,
	.set_rotate = panel_rotate,
	.set_mirror = panel_mirror,
	.run_test = panel_run_test,

	.driver = {
		.name = "hpm1200l-panel",
		.owner = THIS_MODULE,
	},
};


static int __init panel_init(void)
{
	DBG("%s\n", __func__);
	omap_dss_register_driver(&panel_driver);
	return 0;
}

static void __exit panel_exit(void)
{
	DBG("%s\n", __func__);
	omap_dss_unregister_driver(&panel_driver);
}

module_init(panel_init);
module_exit(panel_exit);

MODULE_AUTHOR("Afzal Mohammed <wfvj38@motorola.com>");
MODULE_DESCRIPTION("W3G Panel Driver");
MODULE_LICENSE("GPL");
