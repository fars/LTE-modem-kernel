/*
 * arch/arm/mach-omap2/board-hpm1200l-keypad.c
 *
 * Copyright (C) 2009 Google, Inc.
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

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_event.h>
#include <linux/keyreset.h>

#include <mach/mux.h>
#include <mach/keypad.h>
#include <mach/board-hpm1200l.h>

#ifdef CONFIG_ARM_OF
#include <mach/dt_path.h>
#include <asm/prom.h>
#endif

#define W3G_KEYBOARD_DEV_NAME "w3g-keypad"

static const unsigned short w3g_keymap[64] = {
	/* Row 0, columns 0 - 7 */
	 [0] = KEY_9,
	 [1] = KEY_7,
	 [2] = KEY_1,
	 [3] = KEY_RESERVED,	/* SPEC SHOWS BLANK */
	 [4] = KEY_5,
	 [5] = KEY_8,
	 [6] = KEY_2,
	 [7] = KEY_4,

	/* Row 1, columns 0 - 7 */
	 [8] = KEY_R,
	 [9] = KEY_M,
	[10] = KEY_Y,
	[11] = KEY_6,
	[12] = KEY_J,
	[13] = KEY_SPACE,
	[14] = KEY_0,		/*zero*/
	[15] = KEY_V,

	/* Row 2, columns 0 - 7 */
	[16] = KEY_RESERVED,	/* SPEC SHOWS BLANK: india: KEY_SEND n/c dummy for CALLSEND testing*/
	[17] = KEY_L,
	[18] = KEY_I,
	[19] = KEY_3,
	[20] = KEY_B,
	[21] = KEY_RIGHTALT,	/* SPEC SHOWS SW_ALT */
	[22] = KEY_F,
	[23] = KEY_S,

	/* Row 3, columns 0 - 7 */
	[24] = KEY_VOLUMEDOWN,
	[25] = KEY_K,
	[26] = KEY_RESERVED,	/* SPEC SHOWS BLANK: india: KEY_COMMA */
	[27] = KEY_RIGHT,
	[28] = KEY_RESERVED,	/* SPEC SHOWS BLANK: india: KEY_CAMERA-1 camera 1 key, steal KEY_HP*/
	[29] = KEY_RESERVED,	/* SPEC SHOWS BLANK */
	[30] = KEY_BACK,	/* SPEC SHOWS SW_CAPS  india: KEY_LEFTSHIFT*/
	[31] = KEY_P,

	/* Row 4, columns 0 - 7 */
	[32] = KEY_RESERVED,	/* SPEC SHOWS BLANK: india: KEY_F4 n/c dummy for CALLEND testing */
	[33] = KEY_N,
	[34] = KEY_LEFTALT,	/* SPEC SHOWS SW_SYMBOL: india: KEY_LEFTALT*/
	[35] = KEY_LEFT,
	[36] = KEY_T,
	[37] = KEY_RESERVED,	/* india: KEY_SLASH */
	[38] = KEY_ENTER,
	[39] = KEY_RESERVED,	/* india: KEY_QUESTION */

	/* Row 5, columns 0 - 7 */
	[40] = KEY_VOLUMEUP,
	[41] = KEY_C,
	[42] = KEY_DOT,		/* SPEC SHOWS SW_PERIOD */
	[43] = KEY_DOWN,
	[44] = KEY_RESERVED,	/* SPEC SHOWS BLANK: india: KEY_CAMERA"camera 2" key */
	[45] = KEY_RESERVED,	/* SPEC SHOWS BLANK: india: KEY_EMAIL */ /* @ */
	[46] = KEY_O,
	[47] = KEY_RESERVED,	/* SPEC SHOWS BLANK: india: KEY_MUTE */

	/* Row 6, columns 0 - 7 */
	[48] = KEY_MENU, /*KEY_SEARCH, - redefined for testing */
	[49] = KEY_Z,
	[50] = KEY_G,
	[51] = KEY_UP,
	[52] = KEY_RESERVED,	/* SPEC SHOWS BLANK: india: KEY_MENU */
	[53] = KEY_BACKSPACE,
	[54] = KEY_H,
	[55] = KEY_U,

	/* Row 7, columns 0 - 7 */
	[56] = KEY_D,
	[57] = KEY_RESERVED,	/* SPEC SHOWS BLANK: india: KEY_RIGHTSHIFT */
	[58] = KEY_E,
	[59] = KEY_SELECT,	/* SPEC SHOWS SW_NAV_SELECT: india: KEY_REPLY d-pad center key */
	[60] = KEY_X,
	[61] = KEY_A,
	[62] = KEY_HOME,	/* FIXME: Changed _Q to _HOME */
	[63] = KEY_W,
};

#define W3G_KBD_MEM_START   0x5C1DC000
#define IRQ_NUM 87

static struct resource w3g_keypad_resources[] = {
	[0] = {
		.start  = W3G_KBD_MEM_START,
		.end    = W3G_KBD_MEM_START + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_NUM,
		.end    = IRQ_NUM,
		.flags  = IORESOURCE_IRQ,
	}
};

static struct omap_kp_platform_data w3g_keypad_data = {
	.keymap = w3g_keymap,
	.input_name = "w3g-keypad",
};

static struct platform_device w3g_keypad_device = {
	.name = W3G_KEYBOARD_DEV_NAME,
	.id = 0,
	.dev = {
		.platform_data = &w3g_keypad_data,
	},
	.num_resources = ARRAY_SIZE(w3g_keypad_resources),
	.resource = w3g_keypad_resources,
};


static int __init w3g_init_keypad(void)
{
	/* Keypad Rows */
	omap_cfg_reg(W3G_KBD_R_0);
	omap_cfg_reg(W3G_KBD_R_1);
	omap_cfg_reg(W3G_KBD_R_2);
	omap_cfg_reg(W3G_KBD_R_3);
	omap_cfg_reg(W3G_KBD_R_4);
	omap_cfg_reg(W3G_KBD_R_5);
	omap_cfg_reg(W3G_KBD_R_6);
	omap_cfg_reg(W3G_KBD_R_7);

	/* keypad Columns */
	omap_cfg_reg(W3G_KBD_C_0);
	omap_cfg_reg(W3G_KBD_C_1);
	omap_cfg_reg(W3G_KBD_C_2);
	omap_cfg_reg(W3G_KBD_C_3);
	omap_cfg_reg(W3G_KBD_C_4);
	omap_cfg_reg(W3G_KBD_C_5);
	omap_cfg_reg(W3G_KBD_C_6);
	omap_cfg_reg(W3G_KBD_C_7);

	printk(KERN_ALERT "Keypad registering the device\n");
	return platform_device_register(&w3g_keypad_device);
}

device_initcall(w3g_init_keypad);
