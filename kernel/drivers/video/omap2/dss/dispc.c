/*
 * linux/drivers/video/omap2/dss/dispc.c
 *
 * Copyright (C) 2009 Nokia Corporation
 * Author: Tomi Valkeinen <tomi.valkeinen@nokia.com>
 *
 * Some code and ideas taken from drivers/video/omap/ driver
 * by Imre Deak.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define DSS_SUBSYS_NAME "DISPC"

#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#include <mach/sram.h>
#include <mach/board.h>
#include <mach/clock.h>

#include <mach/display.h>

#include "dss.h"

/* DISPC */
#define DISPC_BASE			0x47008400

#define DISPC_SZ_REGS			SZ_1K

struct dispc_reg { u16 idx; };

#define DISPC_REG(idx)			((const struct dispc_reg) { idx })

/* DISPC common */
#define DISPC_REVISION			DISPC_REG(0x0000)
#define DISPC_SYSCONFIG			DISPC_REG(0x0010)
#define DISPC_SYSSTATUS			DISPC_REG(0x0014)
#define DISPC_IRQSTATUS			DISPC_REG(0x0018)
#define DISPC_IRQENABLE			DISPC_REG(0x001C)
#define DISPC_CONTROL			DISPC_REG(0x0040)
#define DISPC_CONFIG			DISPC_REG(0x0044)
#define DISPC_DEFAULT_COLOR0		DISPC_REG(0x004C)
#define DISPC_LINE_STATUS		DISPC_REG(0x005C)
#define DISPC_LINE_NUMBER		DISPC_REG(0x0060)
#define DISPC_TIMING_H			DISPC_REG(0x0064)
#define DISPC_TIMING_V			DISPC_REG(0x0068)
#define DISPC_POL_FREQ			DISPC_REG(0x006C)
#define DISPC_DIVISOR			DISPC_REG(0x0070)
#define DISPC_SIZE_LCD			DISPC_REG(0x007C)
/* DISPC GFX plane */
#define DISPC_GFX_BA0			DISPC_REG(0x0080)
#define DISPC_GFX_POSITION		DISPC_REG(0x0088)
#define DISPC_GFX_SIZE			DISPC_REG(0x008C)
#define DISPC_GFX_ATTRIBUTES		DISPC_REG(0x00A0)
#define DISPC_GFX_FIFO_THRESHOLD	DISPC_REG(0x00A4)
#define DISPC_GFX_FIFO_SIZE_STATUS	DISPC_REG(0x00A8)
#define DISPC_GFX_ROW_INC		DISPC_REG(0x00AC)
#define DISPC_GFX_PIXEL_INC		DISPC_REG(0x00B0)
#define DISPC_GFX_TABLE_BA		DISPC_REG(0x00B8)

#define DISPC_DATA_CYCLE1		DISPC_REG(0x01D4)
#define DISPC_DATA_CYCLE2		DISPC_REG(0x01D8)
#define DISPC_DATA_CYCLE3		DISPC_REG(0x01DC)

#define DISPC_CPR_COEF_R		DISPC_REG(0x0220)
#define DISPC_CPR_COEF_G		DISPC_REG(0x0224)
#define DISPC_CPR_COEF_B		DISPC_REG(0x0228)

#define DISPC_GFX_PRELOAD		DISPC_REG(0x022C)
#define DISPC_IRQ_MASK_ERROR            (DISPC_IRQ_GFX_FIFO_UNDERFLOW | \
					 DISPC_IRQ_OCP_ERR | \
					 DISPC_IRQ_SYNC_LOST)

#define DISPC_MAX_NR_ISRS		8

struct omap_dispc_isr_data {
	omap_dispc_isr_t	isr;
	void			*arg;
	u32			mask;
};

#define REG_GET(idx, start, end) \
	FLD_GET(dispc_read_reg(idx), start, end)

#define REG_FLD_MOD(idx, val, start, end)				\
	dispc_write_reg(idx, FLD_MOD(dispc_read_reg(idx), val, start, end))

static const struct dispc_reg dispc_reg_att[] = { DISPC_GFX_ATTRIBUTES };

static struct {
	void __iomem    *base;

	struct clk	*dpll4_m4_ck;

	unsigned long	cache_req_pck;
	unsigned long	cache_prate;
	struct dispc_clock_info cache_cinfo;
	u32	fifo_size[1];
	spinlock_t irq_lock;
	u32 irq_error_mask;
	struct omap_dispc_isr_data registered_isr[DISPC_MAX_NR_ISRS];
	u32 error_irqs;
	struct work_struct error_work;

	u32		ctx[DISPC_SZ_REGS / sizeof(u32)];
} dispc;

static void _omap_dispc_set_irqs(void);

static inline void dispc_write_reg(const struct dispc_reg idx, u32 val)
{
	__raw_writel(val, dispc.base + idx.idx);
}

static inline u32 dispc_read_reg(const struct dispc_reg idx)
{
	return __raw_readl(dispc.base + idx.idx);
}

#define SR(reg) \
	dispc.ctx[(DISPC_##reg).idx / sizeof(u32)] = dispc_read_reg(DISPC_##reg)
#define RR(reg) \
	dispc_write_reg(DISPC_##reg, dispc.ctx[(DISPC_##reg).idx / sizeof(u32)])

void dispc_save_context(void)
{
	if (cpu_is_omap24xx())
		return;

	SR(SYSCONFIG);
	SR(IRQENABLE);
	SR(CONTROL);
	SR(CONFIG);
	SR(DEFAULT_COLOR0);
	SR(LINE_NUMBER);
	SR(TIMING_H);
	SR(TIMING_V);
	SR(POL_FREQ);
	SR(DIVISOR);
	SR(SIZE_LCD);
	SR(GFX_BA0);
	SR(GFX_POSITION);
	SR(GFX_SIZE);
	SR(GFX_ATTRIBUTES);
	SR(GFX_FIFO_THRESHOLD);
	SR(GFX_ROW_INC);
	SR(GFX_PIXEL_INC);
	SR(GFX_TABLE_BA);

	SR(DATA_CYCLE1);
	SR(DATA_CYCLE2);
	SR(DATA_CYCLE3);

	SR(CPR_COEF_R);
	SR(CPR_COEF_G);
	SR(CPR_COEF_B);

	SR(GFX_PRELOAD);
}

void dispc_restore_context(void)
{
	RR(SYSCONFIG);
	RR(IRQENABLE);
	/*RR(CONTROL);*/
	RR(CONFIG);
	RR(DEFAULT_COLOR0);
	RR(LINE_NUMBER);
	RR(TIMING_H);
	RR(TIMING_V);
	RR(POL_FREQ);
	RR(DIVISOR);
	RR(SIZE_LCD);

	RR(GFX_BA0);
	RR(GFX_POSITION);
	RR(GFX_SIZE);
	RR(GFX_ATTRIBUTES);
	RR(GFX_FIFO_THRESHOLD);
	RR(GFX_ROW_INC);
	RR(GFX_PIXEL_INC);
	RR(GFX_TABLE_BA);

	RR(DATA_CYCLE1);
	RR(DATA_CYCLE2);
	RR(DATA_CYCLE3);

	RR(CPR_COEF_R);
	RR(CPR_COEF_G);
	RR(CPR_COEF_B);

	RR(GFX_PRELOAD);

	/* enable last, because LCD & DIGIT enable are here */
	RR(CONTROL);
}

#undef SR
#undef RR

static inline void enable_clocks(bool enable)
{
	if (enable)
		dss_clk_enable(DSS_CLK_ICK | DSS_CLK_FCK1);
	else
		dss_clk_disable(DSS_CLK_ICK | DSS_CLK_FCK1);
}

bool dispc_go_busy(enum omap_channel channel)
{
	int bit;
    bool ret;

	if (channel == OMAP_DSS_CHANNEL_LCD)
		bit = 5; /* GOLCD */
	else
		bit = 6; /* GODIGIT */

	enable_clocks(1);
	ret = REG_GET(DISPC_CONTROL, bit, bit) == 1;
	enable_clocks(0);
    return ret;
}

void dispc_go(enum omap_channel channel)
{
	int bit;

	enable_clocks(1);

	if (channel == OMAP_DSS_CHANNEL_LCD)
		bit = 0; /* LCDENABLE */
	else
		bit = 1; /* DIGITALENABLE */

	/* if the channel is not enabled, we don't need GO */
	if (REG_GET(DISPC_CONTROL, bit, bit) == 0)
		goto end;

	if (channel == OMAP_DSS_CHANNEL_LCD)
		bit = 5; /* GOLCD */
	else
		bit = 6; /* GODIGIT */

	if (REG_GET(DISPC_CONTROL, bit, bit) == 1) {
		DSSERR("GO bit not down for channel %d\n", channel);
		goto end;
	}

	DSSDBG("GO %s\n", channel == OMAP_DSS_CHANNEL_LCD ? "LCD" : "DIGIT");

	REG_FLD_MOD(DISPC_CONTROL, 1, bit, bit);
end:
	enable_clocks(0);
}

static void _dispc_set_plane_ba0(enum omap_plane plane, u32 paddr)
{
	const struct dispc_reg ba0_reg[] = { DISPC_GFX_BA0 };

	dispc_write_reg(ba0_reg[plane], paddr);
}

static void _dispc_set_plane_pos(enum omap_plane plane, int x, int y)
{
	const struct dispc_reg pos_reg[] = { DISPC_GFX_POSITION	};

	u32 val = FLD_VAL(y, 26, 16) | FLD_VAL(x, 10, 0);

	dispc_write_reg(pos_reg[plane], val);
}

static void _dispc_set_pic_size(enum omap_plane plane, int width, int height)
{
	const struct dispc_reg siz_reg[] = { DISPC_GFX_SIZE };
	u32 val = FLD_VAL(height - 1, 26, 16) | FLD_VAL(width - 1, 10, 0);

	dispc_write_reg(siz_reg[plane], val);
}

static void _dispc_set_pix_inc(enum omap_plane plane, s32 inc)
{
	const struct dispc_reg ri_reg[] = { DISPC_GFX_PIXEL_INC };

	dispc_write_reg(ri_reg[plane], inc);
}

static void _dispc_set_row_inc(enum omap_plane plane, s32 inc)
{
	const struct dispc_reg ri_reg[] = { DISPC_GFX_ROW_INC };

	dispc_write_reg(ri_reg[plane], inc);
}

static void _dispc_set_color_mode(enum omap_plane plane,
		enum omap_color_mode color_mode)
{
	u32 m = 0;

	switch (color_mode) {
	case OMAP_DSS_COLOR_CLUT1:
		m = 0x0; break;
	case OMAP_DSS_COLOR_CLUT2:
		m = 0x1; break;
	case OMAP_DSS_COLOR_CLUT4:
		m = 0x2; break;
	case OMAP_DSS_COLOR_CLUT8:
		m = 0x3; break;
	case OMAP_DSS_COLOR_RGB12U:
		m = 0x4; break;
	case OMAP_DSS_COLOR_ARGB16:
		m = 0x5; break;
	case OMAP_DSS_COLOR_RGB16:
		m = 0x6; break;
	case OMAP_DSS_COLOR_RGB24U:
		m = 0x8; break;
	case OMAP_DSS_COLOR_RGB24P:
		m = 0x9; break;
	case OMAP_DSS_COLOR_YUV2:
		m = 0xa; break;
	case OMAP_DSS_COLOR_UYVY:
		m = 0xb; break;
	case OMAP_DSS_COLOR_ARGB32:
		m = 0xc; break;
	case OMAP_DSS_COLOR_RGBA32:
		m = 0xd; break;
	case OMAP_DSS_COLOR_RGBX32:
		m = 0xe; break;
	default:
		BUG(); break;
	}

	REG_FLD_MOD(dispc_reg_att[plane], m, 4, 1);
}

void dispc_set_burst_size(enum omap_plane plane,
		enum omap_burst_size burst_size)
{
	int shift;
	u32 val;

	enable_clocks(1);

	switch (plane) {
	case OMAP_DSS_GFX:
		shift = 6;
		break;
	default:
		BUG();
		return;
	}

	val = dispc_read_reg(dispc_reg_att[plane]);
	val = FLD_MOD(val, burst_size, shift+1, shift);
	dispc_write_reg(dispc_reg_att[plane], val);

	enable_clocks(0);
}

void dispc_enable_replication(enum omap_plane plane, bool enable)
{
	int bit;

	if (plane == OMAP_DSS_GFX)
		bit = 5;
	else
		bit = 10;

	enable_clocks(1);
	REG_FLD_MOD(dispc_reg_att[plane], enable, bit, bit);
	enable_clocks(0);
}

void dispc_set_lcd_size(u16 width, u16 height)
{
	u32 val;
	BUG_ON((width > (1 << 11)) || (height > (1 << 11)));
	val = FLD_VAL(height - 1, 26, 16) | FLD_VAL(width - 1, 10, 0);
	enable_clocks(1);
	dispc_write_reg(DISPC_SIZE_LCD, val);
	enable_clocks(0);
}

static void dispc_read_plane_fifo_sizes(void)
{
	const struct dispc_reg fsz_reg[] = { DISPC_GFX_FIFO_SIZE_STATUS };
	u32 size;
	int plane;

	enable_clocks(1);

	for (plane = 0; plane < ARRAY_SIZE(dispc.fifo_size); ++plane) {
		if (cpu_is_omap24xx()) {
			if (!cpu_is_omapw3g())
				size = FLD_GET(dispc_read_reg(fsz_reg[plane]),
					8, 0);
			else
				size = FLD_GET(dispc_read_reg(fsz_reg[plane]),
					9, 0);
		}
		else if (cpu_is_omap34xx())
			size = FLD_GET(dispc_read_reg(fsz_reg[plane]), 10, 0);
		else
			BUG();

		dispc.fifo_size[plane] = size;
	}

	enable_clocks(0);
}

u32 dispc_get_plane_fifo_size(enum omap_plane plane)
{
	return dispc.fifo_size[plane];
}

void dispc_setup_plane_fifo(enum omap_plane plane, u32 low, u32 high)
{
	const struct dispc_reg ftrs_reg[] = { DISPC_GFX_FIFO_THRESHOLD };

	enable_clocks(1);

	DSSDBG("fifo(%d) low/high old %u/%u, new %u/%u\n",
			plane,
			REG_GET(ftrs_reg[plane], 9, 0),
			REG_GET(ftrs_reg[plane], 25, 16),
			low, high);

	if (cpu_is_omap24xx()) {
		if (!cpu_is_omapw3g())
			dispc_write_reg(ftrs_reg[plane],
				FLD_VAL(high, 24, 16) | FLD_VAL(low, 8, 0));
		else
			dispc_write_reg(ftrs_reg[plane],
				FLD_VAL(high, 25, 16) | FLD_VAL(low, 9, 0));
	}
	else
		dispc_write_reg(ftrs_reg[plane],
				FLD_VAL(high, 27, 16) | FLD_VAL(low, 11, 0));

	enable_clocks(0);
}

static void _dispc_set_rotation_attrs(enum omap_plane plane, u8 rotation,
		bool mirroring, enum omap_color_mode color_mode)
{
	if (color_mode == OMAP_DSS_COLOR_YUV2 ||
			color_mode == OMAP_DSS_COLOR_UYVY) {
		int vidrot = 0;

		if (mirroring) {
			switch (rotation) {
			case 0:
				vidrot = 2;
				break;
			case 1:
				vidrot = 1;
				break;
			case 2:
				vidrot = 0;
				break;
			case 3:
				vidrot = 3;
				break;
			}
		} else {
			switch (rotation) {
			case 0:
				vidrot = 0;
				break;
			case 1:
				vidrot = 1;
				break;
			case 2:
				vidrot = 2;
				break;
			case 3:
				vidrot = 3;
				break;
			}
		}

		REG_FLD_MOD(dispc_reg_att[plane], vidrot, 13, 12);

		if (rotation == 1 || rotation == 3)
			REG_FLD_MOD(dispc_reg_att[plane], 0x1, 18, 18);
		else
			REG_FLD_MOD(dispc_reg_att[plane], 0x0, 18, 18);
	} else {
		REG_FLD_MOD(dispc_reg_att[plane], 0, 13, 12);
		REG_FLD_MOD(dispc_reg_att[plane], 0, 18, 18);
	}
}

static s32 pixinc(int pixels, u8 ps)
{
	if (pixels == 1)
		return 1;
	else if (pixels > 1)
		return 1 + (pixels - 1) * ps;
	else if (pixels < 0)
		return 1 - (-pixels + 1) * ps;
	else
		BUG();
}

static void calc_vrfb_rotation_offset(u8 rotation, bool mirror,
		u16 screen_width,
		u16 width, u16 height,
		enum omap_color_mode color_mode, bool fieldmode,
		unsigned int field_offset,
		unsigned *offset0, unsigned *offset1,
		s32 *row_inc, s32 *pix_inc)
{
	u8 ps;

	switch (color_mode) {
	case OMAP_DSS_COLOR_RGB16:
	case OMAP_DSS_COLOR_ARGB16:
		ps = 2;
		break;

	case OMAP_DSS_COLOR_RGB24P:
		ps = 3;
		break;

	case OMAP_DSS_COLOR_RGB24U:
	case OMAP_DSS_COLOR_ARGB32:
	case OMAP_DSS_COLOR_RGBA32:
	case OMAP_DSS_COLOR_RGBX32:
	case OMAP_DSS_COLOR_YUV2:
	case OMAP_DSS_COLOR_UYVY:
		ps = 4;
		break;

	default:
		BUG();
		return;
	}

	DSSDBG("calc_rot(%d): scrw %d, %dx%d\n", rotation, screen_width,
			width, height);

	/*
	 * field 0 = even field = bottom field
	 * field 1 = odd field = top field
	 */
	switch (rotation + mirror * 4) {
	case 0:
	case 2:
		/*
		 * If the pixel format is YUV or UYVY divide the width
		 * of the image by 2 for 0 and 180 degree rotation.
		 */
		if (color_mode == OMAP_DSS_COLOR_YUV2 ||
			color_mode == OMAP_DSS_COLOR_UYVY)
			width = width >> 1;
	case 1:
	case 3:
		*offset1 = 0;
		if (field_offset)
			*offset0 = field_offset * screen_width * ps;
		else
			*offset0 = 0;

		*row_inc = pixinc(1 + (screen_width - width) +
				(fieldmode ? screen_width : 0),
				ps);
		*pix_inc = pixinc(1, ps);
		break;

	case 4:
	case 6:
		/* If the pixel format is YUV or UYVY divide the width
		 * of the image by 2  for 0 degree and 180 degree
		 */
		if (color_mode == OMAP_DSS_COLOR_YUV2 ||
			color_mode == OMAP_DSS_COLOR_UYVY)
			width = width >> 1;
	case 5:
	case 7:
		*offset1 = 0;
		if (field_offset)
			*offset0 = field_offset * screen_width * ps;
		else
			*offset0 = 0;
		*row_inc = pixinc(1 - (screen_width + width) -
				(fieldmode ? screen_width : 0),
				ps);
		*pix_inc = pixinc(1, ps);
		break;

	default:
		BUG();
	}
}

static void calc_dma_rotation_offset(u8 rotation, bool mirror,
		u16 screen_width,
		u16 width, u16 height,
		enum omap_color_mode color_mode, bool fieldmode,
		unsigned int field_offset,
		unsigned *offset0, unsigned *offset1,
		s32 *row_inc, s32 *pix_inc)
{
	u8 ps;
	u16 fbw, fbh;

	switch (color_mode) {
	case OMAP_DSS_COLOR_RGB16:
	case OMAP_DSS_COLOR_ARGB16:
		ps = 2;
		break;

	case OMAP_DSS_COLOR_RGB24P:
		ps = 3;
		break;

	case OMAP_DSS_COLOR_RGB24U:
	case OMAP_DSS_COLOR_ARGB32:
	case OMAP_DSS_COLOR_RGBA32:
	case OMAP_DSS_COLOR_RGBX32:
		ps = 4;
		break;

	case OMAP_DSS_COLOR_YUV2:
	case OMAP_DSS_COLOR_UYVY:
		ps = 2;
		break;
	default:
		BUG();
		return;
	}

	DSSDBG("calc_rot(%d): scrw %d, %dx%d\n", rotation, screen_width,
			width, height);

	/* width & height are overlay sizes, convert to fb sizes */

	if (rotation == 0 || rotation == 2) {
		fbw = width;
		fbh = height;
	} else {
		fbw = height;
		fbh = width;
	}

	/*
	 * field 0 = even field = bottom field
	 * field 1 = odd field = top field
	 */
	switch (rotation + mirror * 4) {
	case 0:
		*offset1 = 0;
		if (field_offset)
			*offset0 = *offset1 + field_offset * screen_width * ps;
		else
			*offset0 = *offset1;
		*row_inc = pixinc(1 + (screen_width - fbw) +
				(fieldmode ? screen_width : 0),
				ps);
		*pix_inc = pixinc(1, ps);
		break;
	case 1:
		*offset1 = screen_width * (fbh - 1) * ps;
		if (field_offset)
			*offset0 = *offset1 + field_offset * ps;
		else
			*offset0 = *offset1;
		*row_inc = pixinc(screen_width * (fbh - 1) + 1 +
				(fieldmode ? 1 : 0), ps);
		*pix_inc = pixinc(-screen_width, ps);
		break;
	case 2:
		*offset1 = (screen_width * (fbh - 1) + fbw - 1) * ps;
		if (field_offset)
			*offset0 = *offset1 - field_offset * screen_width * ps;
		else
			*offset0 = *offset1;
		*row_inc = pixinc(-1 -
				(screen_width - fbw) -
				(fieldmode ? screen_width : 0),
				ps);
		*pix_inc = pixinc(-1, ps);
		break;
	case 3:
		*offset1 = (fbw - 1) * ps;
		if (field_offset)
			*offset0 = *offset1 - field_offset * ps;
		else
			*offset0 = *offset1;
		*row_inc = pixinc(-screen_width * (fbh - 1) - 1 -
				(fieldmode ? 1 : 0), ps);
		*pix_inc = pixinc(screen_width, ps);
		break;

	/* mirroring */
	case 0 + 4:
		*offset1 = (fbw - 1) * ps;
		if (field_offset)
			*offset0 = *offset1 + field_offset * screen_width * ps;
		else
			*offset0 = *offset1;
		*row_inc = pixinc(screen_width * 2 - 1 +
				(fieldmode ? screen_width : 0),
				ps);
		*pix_inc = pixinc(-1, ps);
		break;

	case 1 + 4:
		*offset1 = 0;
		if (field_offset)
			*offset0 = *offset1 + field_offset * ps;
		else
			*offset0 = *offset1;
		*row_inc = pixinc(-screen_width * (fbh - 1) + 1 +
				(fieldmode ? 1 : 0),
				ps);
		*pix_inc = pixinc(screen_width, ps);
		break;

	case 2 + 4:
		*offset1 = screen_width * (fbh - 1) * ps;
		if (field_offset)
			*offset0 = *offset1 - field_offset * screen_width * ps;
		else
			*offset0 = *offset1;
		*row_inc = pixinc(1 - screen_width * 2 -
				(fieldmode ? screen_width : 0),
				ps);
		*pix_inc = pixinc(1, ps);
		break;

	case 3 + 4:
		*offset1 = (screen_width * (fbh - 1) + fbw - 1) * ps;
		if (field_offset)
			*offset0 = *offset1 - field_offset * ps;
		else
			*offset0 = *offset1;
		*row_inc = pixinc(screen_width * (fbh - 1) - 1 -
				(fieldmode ? 1 : 0),
				ps);
		*pix_inc = pixinc(-screen_width, ps);
		break;

	default:
		BUG();
	}
}

static unsigned long calc_fclk_five_taps(u16 width, u16 height,
		u16 out_width, u16 out_height, enum omap_color_mode color_mode)
{
	u32 fclk = 0;
	/* FIXME venc pclk? */
	u64 tmp, pclk = dispc_pclk_rate();

	if (height > out_height) {
		/* FIXME get real display PPL */
		unsigned int ppl = 800;

		tmp = pclk * height * out_width;
		do_div(tmp, 2 * out_height * ppl);
		fclk = tmp;

		if (height > 2 * out_height && ppl != out_width) {
			tmp = pclk * (height - 2 * out_height) * out_width;
			do_div(tmp, 2 * out_height * (ppl - out_width));
			fclk = max(fclk, (u32) tmp);
		}
	}

	if (width > out_width) {
		tmp = pclk * width;
		do_div(tmp, out_width);
		fclk = max(fclk, (u32) tmp);

		if (color_mode == OMAP_DSS_COLOR_RGB24U)
			fclk <<= 1;
	}

	return fclk;
}

static unsigned long calc_fclk(u16 width, u16 height,
		u16 out_width, u16 out_height)
{
	unsigned int hf, vf;

	/*
	 * FIXME how to determine the 'A' factor
	 * for the no downscaling case ?
	 */

	if (width > 3 * out_width)
		hf = 4;
	else if (width > 2 * out_width)
		hf = 3;
	else if (width > out_width)
		hf = 2;
	else
		hf = 1;

	if (height > out_height)
		vf = 2;
	else
		vf = 1;

	/* FIXME venc pclk? */
	return dispc_pclk_rate() * vf * hf;
}

static int _dispc_setup_plane(enum omap_plane plane,
		u32 paddr, u16 screen_width,
		u16 pos_x, u16 pos_y,
		u16 width, u16 height,
		u16 out_width, u16 out_height,
		enum omap_color_mode color_mode,
		bool ilace,
		enum omap_dss_rotation_type rotation_type,
		u8 rotation, int mirror,
		u8 global_alpha)
{
	const int maxdownscale = cpu_is_omap34xx() ? 4 : 2;
	bool five_taps = 0;
	bool fieldmode = 0;
	int cconv = 0;
	unsigned offset0, offset1;
	s32 row_inc;
	s32 pix_inc;
	u16 frame_height = height;
	unsigned int field_offset = 0;

	if (paddr == 0)
		return -EINVAL;

	if (ilace && height == out_height)
		fieldmode = 1;

	if (ilace) {
		if (fieldmode)
			height /= 2;
		pos_y /= 2;
		out_height /= 2;

		DSSDBG("adjusting for ilace: height %d, pos_y %d, "
				"out_height %d\n",
				height, pos_y, out_height);
	}

	if (plane == OMAP_DSS_GFX) {
		if (width != out_width || height != out_height)
			return -EINVAL;

		switch (color_mode) {
		case OMAP_DSS_COLOR_ARGB16:
		case OMAP_DSS_COLOR_RGB16:
		case OMAP_DSS_COLOR_RGB24P:
		case OMAP_DSS_COLOR_RGB24U:
		case OMAP_DSS_COLOR_ARGB32:
		case OMAP_DSS_COLOR_RGBA32:
		case OMAP_DSS_COLOR_RGBX32:
			break;

		default:
			return -EINVAL;
		}
	} else {
		/* video plane */

		unsigned long fclk = 0;

		if (out_width < width / maxdownscale ||
		   out_width > width * 8)
			return -EINVAL;

		if (out_height < height / maxdownscale ||
		   out_height > height * 8)
			return -EINVAL;

		switch (color_mode) {
		case OMAP_DSS_COLOR_RGB16:
		case OMAP_DSS_COLOR_RGB24P:
		case OMAP_DSS_COLOR_RGB24U:
		case OMAP_DSS_COLOR_RGBX32:
			break;

		case OMAP_DSS_COLOR_ARGB16:
		case OMAP_DSS_COLOR_ARGB32:
		case OMAP_DSS_COLOR_RGBA32:
			if (plane == OMAP_DSS_VIDEO1)
				return -EINVAL;
			break;

		case OMAP_DSS_COLOR_YUV2:
		case OMAP_DSS_COLOR_UYVY:
			cconv = 1;
			break;

		default:
			return -EINVAL;
		}

		/* Must use 5-tap filter? */
		five_taps = height > out_height * 2;

		if (!five_taps) {
			fclk = calc_fclk(width, height,
					out_width, out_height);

			/* Try 5-tap filter if 3-tap fclk is too high */
			if (cpu_is_omap34xx() && height > out_height &&
					fclk > dispc_fclk_rate())
				five_taps = true;
		}

		if (width > (2048 >> five_taps))
			return -EINVAL;

		if (five_taps)
			fclk = calc_fclk_five_taps(width, height,
					out_width, out_height, color_mode);

		DSSDBG("required fclk rate = %lu Hz\n", fclk);
		DSSDBG("current fclk rate = %lu Hz\n", dispc_fclk_rate());

		if (fclk > dispc_fclk_rate())
			return -EINVAL;
	}

	if (ilace && !fieldmode) {
		/*
		 * when downscaling the bottom field may have to start several
		 * source lines below the top field. Unfortunately ACCUI
		 * registers will only hold the fractional part of the offset
		 * so the integer part must be added to the base address of the
		 * bottom field.
		 */
		if (!height || height == out_height)
			field_offset = 0;
		else
			field_offset = height / out_height / 2;
	}

	/* Fields are independent but interleaved in memory. */
	if (fieldmode)
		field_offset = 1;

	if (rotation_type == OMAP_DSS_ROT_DMA)
		calc_dma_rotation_offset(rotation, mirror,
				screen_width, width, frame_height, color_mode,
				fieldmode, field_offset,
				&offset0, &offset1, &row_inc, &pix_inc);
	else
		calc_vrfb_rotation_offset(rotation, mirror,
				screen_width, width, frame_height, color_mode,
				fieldmode, field_offset,
				&offset0, &offset1, &row_inc, &pix_inc);

	DSSDBG("offset0 %u, offset1 %u, row_inc %d, pix_inc %d\n",
			offset0, offset1, row_inc, pix_inc);

	_dispc_set_color_mode(plane, color_mode);

	_dispc_set_plane_ba0(plane, paddr + offset0);

	_dispc_set_row_inc(plane, row_inc);
	_dispc_set_pix_inc(plane, pix_inc);

	DSSDBG("%d,%d %dx%d -> %dx%d\n", pos_x, pos_y, width, height,
			out_width, out_height);

	_dispc_set_plane_pos(plane, pos_x, pos_y);

	_dispc_set_pic_size(plane, width, height);

	_dispc_set_rotation_attrs(plane, rotation, mirror, color_mode);
	return 0;
}

static void _dispc_enable_plane(enum omap_plane plane, bool enable)
{
	REG_FLD_MOD(dispc_reg_att[plane], enable ? 1 : 0, 0, 0);
}

static void dispc_disable_isr(void *data, u32 mask)
{
	struct completion *compl = data;
	complete(compl);
}

static void _enable_lcd_out(bool enable)
{
	REG_FLD_MOD(DISPC_CONTROL, enable ? 1 : 0, 0, 0);
}

void dispc_enable_lcd_out(bool enable)
{
	struct completion frame_done_completion;
	bool is_on;
	int r;

	enable_clocks(1);

	/* When we disable LCD output, we need to wait until frame is done.
	 * Otherwise the DSS is still working, and turning off the clocks
	 * prevents DSS from going to OFF mode */
	is_on = REG_GET(DISPC_CONTROL, 0, 0);

	if (!enable && is_on) {
		init_completion(&frame_done_completion);

		r = omap_dispc_register_isr(dispc_disable_isr,
				&frame_done_completion,
				DISPC_IRQ_FRAMEDONE);

		if (r)
			DSSERR("failed to register FRAMEDONE isr\n");
	}

	_enable_lcd_out(enable);

	if (!enable && is_on) {
		if (!wait_for_completion_timeout(&frame_done_completion,
					msecs_to_jiffies(100)))
			DSSERR("timeout waiting for FRAME DONE\n");

		r = omap_dispc_unregister_isr(dispc_disable_isr,
				&frame_done_completion,
				DISPC_IRQ_FRAMEDONE);

		if (r)
			DSSERR("failed to unregister FRAMEDONE isr\n");
	}

	enable_clocks(0);
}

void dispc_enable_fifohandcheck(bool enable)
{
	enable_clocks(1);
	REG_FLD_MOD(DISPC_CONFIG, enable ? 1 : 0, 16, 16);
	enable_clocks(0);
}


void dispc_set_lcd_display_type(enum omap_lcd_display_type type)
{
	int mode;

	switch (type) {
	case OMAP_DSS_LCD_DISPLAY_STN:
		mode = 0;
		break;

	case OMAP_DSS_LCD_DISPLAY_TFT:
		mode = 1;
		break;

	default:
		BUG();
		return;
	}

	enable_clocks(1);
	REG_FLD_MOD(DISPC_CONTROL, mode, 3, 3);
	enable_clocks(0);
}

void dispc_set_loadmode(enum omap_dss_load_mode mode)
{
	enable_clocks(1);
	REG_FLD_MOD(DISPC_CONFIG, mode, 2, 1);
	enable_clocks(0);
}


void dispc_set_default_color(enum omap_channel channel, u32 color)
{
	const struct dispc_reg def_reg[] = { DISPC_DEFAULT_COLOR0 };


	enable_clocks(1);
	dispc_write_reg(def_reg[channel], color);
	enable_clocks(0);
}

u32 dispc_get_default_color(enum omap_channel channel)
{
	const struct dispc_reg def_reg[] = { DISPC_DEFAULT_COLOR0 };
	u32 l;

	BUG_ON(channel != OMAP_DSS_CHANNEL_DIGIT &&
	       channel != OMAP_DSS_CHANNEL_LCD);

	enable_clocks(1);
	l = dispc_read_reg(def_reg[channel]);
	enable_clocks(0);

	return l;
}

void dispc_set_tft_data_lines(u8 data_lines)
{
	int code;

	switch (data_lines) {
	case 12:
		code = 0;
		break;
	case 16:
		code = 1;
		break;
	case 18:
		code = 2;
		break;
	case 24:
		code = 3;
		break;
	default:
		BUG();
		return;
	}

	enable_clocks(1);
	REG_FLD_MOD(DISPC_CONTROL, code, 9, 8);
	enable_clocks(0);
}

void dispc_set_parallel_interface_mode(enum omap_parallel_interface_mode mode)
{
	u32 l;
	int stallmode;
	int gpout0 = 1;
	int gpout1;

	switch (mode) {
	case OMAP_DSS_PARALLELMODE_BYPASS:
		stallmode = 0;
		gpout1 = 1;
		break;

	case OMAP_DSS_PARALLELMODE_RFBI:
		stallmode = 1;
		gpout1 = 0;
		break;

	case OMAP_DSS_PARALLELMODE_DSI:
		stallmode = 1;
		gpout1 = 1;
		break;

	default:
		BUG();
		return;
	}

	enable_clocks(1);

	l = dispc_read_reg(DISPC_CONTROL);

	l = FLD_MOD(l, stallmode, 11, 11);
	l = FLD_MOD(l, gpout0, 15, 15);
	l = FLD_MOD(l, gpout1, 16, 16);

	dispc_write_reg(DISPC_CONTROL, l);

	enable_clocks(0);
}

void dispc_set_lcd_divisor(u16 lck_div, u16 pck_div)
{
	BUG_ON(lck_div < 1);
	BUG_ON(pck_div < 2);

	enable_clocks(1);
	dispc_write_reg(DISPC_DIVISOR,
			FLD_VAL(lck_div, 23, 16) | FLD_VAL(pck_div, 7, 0));
	enable_clocks(0);
}

static void dispc_get_lcd_divisor(int *lck_div, int *pck_div)
{
	u32 l;
	l = dispc_read_reg(DISPC_DIVISOR);
	*lck_div = FLD_GET(l, 23, 16);
	*pck_div = FLD_GET(l, 7, 0);
}

unsigned long dispc_fclk_rate(void)
{
	unsigned long r = 0;

	if (dss_get_dispc_clk_source() == 0)
		r = dss_clk_get_rate(DSS_CLK_FCK1);
	else {
#ifdef CONFIG_OMAP2_DSS_DBI
		r = dss_clk_get_rate(DSS_CLK_FCK2);
#endif
#ifdef CONFIG_OMAP2_DSS_DSI
		r = dsi_get_dsi1_pll_rate();
#endif
	}
	return r;
}

unsigned long dispc_lclk_rate(void)
{
	int lcd;
	unsigned long r;
	u32 l;

	l = dispc_read_reg(DISPC_DIVISOR);

	lcd = FLD_GET(l, 23, 16);

	r = dispc_fclk_rate();

	return r / lcd;
}

unsigned long dispc_pclk_rate(void)
{
	int lcd, pcd;
	unsigned long r;
	u32 l;

	l = dispc_read_reg(DISPC_DIVISOR);

	lcd = FLD_GET(l, 23, 16);
	pcd = FLD_GET(l, 7, 0);

	r = dispc_fclk_rate();

	return r / lcd / pcd;
}

void dispc_dump_clocks(struct seq_file *s)
{
	int lcd, pcd;

	enable_clocks(1);

	dispc_get_lcd_divisor(&lcd, &pcd);

	seq_printf(s, "- dispc -\n");

	seq_printf(s, "dispc fclk source = %s\n",
			dss_get_dispc_clk_source() == 0 ?
			"dss1_alwon_fclk" : "dsi1_pll_fclk");

	seq_printf(s, "pixel clk = %lu / %d / %d = %lu\n",
			dispc_fclk_rate(),
			lcd, pcd,
			dispc_pclk_rate());

	enable_clocks(0);
}

void dispc_dump_regs(struct seq_file *s)
{
#define DUMPREG(r) seq_printf(s, "%-35s %08x\n", #r, dispc_read_reg(r))

	dss_clk_enable(DSS_CLK_ICK | DSS_CLK_FCK1);

	DUMPREG(DISPC_REVISION);
	DUMPREG(DISPC_SYSCONFIG);
	DUMPREG(DISPC_SYSSTATUS);
	DUMPREG(DISPC_IRQSTATUS);
	DUMPREG(DISPC_IRQENABLE);
	DUMPREG(DISPC_CONTROL);
	DUMPREG(DISPC_CONFIG);
	DUMPREG(DISPC_DEFAULT_COLOR0);
	DUMPREG(DISPC_LINE_STATUS);
	DUMPREG(DISPC_LINE_NUMBER);
	DUMPREG(DISPC_TIMING_H);
	DUMPREG(DISPC_TIMING_V);
	DUMPREG(DISPC_POL_FREQ);
	DUMPREG(DISPC_DIVISOR);
	DUMPREG(DISPC_SIZE_LCD);

	DUMPREG(DISPC_GFX_BA0);
	DUMPREG(DISPC_GFX_POSITION);
	DUMPREG(DISPC_GFX_SIZE);
	DUMPREG(DISPC_GFX_ATTRIBUTES);
	DUMPREG(DISPC_GFX_FIFO_THRESHOLD);
	DUMPREG(DISPC_GFX_FIFO_SIZE_STATUS);
	DUMPREG(DISPC_GFX_ROW_INC);
	DUMPREG(DISPC_GFX_PIXEL_INC);
	DUMPREG(DISPC_GFX_TABLE_BA);

	DUMPREG(DISPC_DATA_CYCLE1);
	DUMPREG(DISPC_DATA_CYCLE2);
	DUMPREG(DISPC_DATA_CYCLE3);

	DUMPREG(DISPC_CPR_COEF_R);
	DUMPREG(DISPC_CPR_COEF_G);
	DUMPREG(DISPC_CPR_COEF_B);

	DUMPREG(DISPC_GFX_PRELOAD);

	dss_clk_disable(DSS_CLK_ICK | DSS_CLK_FCK1);
#undef DUMPREG
}

void find_lck_pck_divs(bool is_tft, unsigned long req_pck, unsigned long fck,
		u16 *lck_div, u16 *pck_div)
{
	u16 pcd_min = is_tft ? 2 : 3;
	unsigned long best_pck;
	u16 best_ld, cur_ld;
	u16 best_pd, cur_pd;

	best_pck = 0;
	best_ld = 0;
	best_pd = 0;

    DSSDBG("[%s]->\n", __func__);
	for (cur_ld = 1; cur_ld <= 255; ++cur_ld) {
		unsigned long lck = fck / cur_ld;

		for (cur_pd = pcd_min; cur_pd <= 255; ++cur_pd) {
			unsigned long pck = lck / cur_pd;
			long old_delta = abs(best_pck - req_pck);
			long new_delta = abs(pck - req_pck);

			if (best_pck == 0 || new_delta < old_delta) {
				best_pck = pck;
				best_ld = cur_ld;
				best_pd = cur_pd;

				if (pck == req_pck)
					goto found;
			}

			if (pck < req_pck)
				break;
		}

		if (lck / pcd_min < req_pck)
			break;
	}

found:
	*lck_div = best_ld;
	*pck_div = best_pd;

    DSSDBG("[%s]-<; lcd: %d, pcd: %d\n", __func__, *lck_div, *pck_div);
}

int dispc_calc_clock_div(bool is_tft, unsigned long req_pck,
		struct dispc_clock_info *cinfo)
{
	unsigned long prate;
	struct dispc_clock_info cur, best;
	int match = 0;
	int min_fck_per_pck;
	unsigned long fck_rate = dss_clk_get_rate(DSS_CLK_FCK1);

    DSSDBG("[%s]->\n", __func__);
	if (cpu_is_omap34xx())
		prate = clk_get_rate(clk_get_parent(dispc.dpll4_m4_ck));
	else
		prate = 0;

	if (req_pck == dispc.cache_req_pck &&
			((cpu_is_omap34xx() && prate == dispc.cache_prate) ||
			 dispc.cache_cinfo.fck == fck_rate)) {
		DSSDBG("dispc clock info found from cache.\n");
		*cinfo = dispc.cache_cinfo;
		return 0;
	}

	min_fck_per_pck = CONFIG_OMAP2_DSS_MIN_FCK_PER_PCK;

	if (min_fck_per_pck &&
		req_pck * min_fck_per_pck > DISPC_MAX_FCK) {
		DSSERR("Requested pixel clock not possible with the current "
				"OMAP2_DSS_MIN_FCK_PER_PCK setting. Turning "
				"the constraint off.\n");
		min_fck_per_pck = 0;
	}

retry:
	memset(&cur, 0, sizeof(cur));
	memset(&best, 0, sizeof(best));

	if (cpu_is_omap24xx()) {
		/* XXX can we change the clock on omap2? */
		cur.fck = dss_clk_get_rate(DSS_CLK_FCK1);
		cur.fck_div = 1;

		match = 1;

		find_lck_pck_divs(is_tft, req_pck, cur.fck,
				&cur.lck_div, &cur.pck_div);

		cur.lck = cur.fck / cur.lck_div;
		cur.pck = cur.lck / cur.pck_div;

		best = cur;

		goto found;
	} else if (cpu_is_omap34xx()) {
		for (cur.fck_div = 16; cur.fck_div > 0; --cur.fck_div) {
			cur.fck = prate / cur.fck_div * 2;

			if (cur.fck > DISPC_MAX_FCK)
				continue;

			if (min_fck_per_pck &&
					cur.fck < req_pck * min_fck_per_pck)
				continue;

			match = 1;

			find_lck_pck_divs(is_tft, req_pck, cur.fck,
					&cur.lck_div, &cur.pck_div);

			cur.lck = cur.fck / cur.lck_div;
			cur.pck = cur.lck / cur.pck_div;

			if (abs(cur.pck - req_pck) < abs(best.pck - req_pck)) {
				best = cur;

				if (cur.pck == req_pck)
					goto found;
			}
		}
	} else {
		BUG();
	}

found:
	if (!match) {
		if (min_fck_per_pck) {
			DSSERR("Could not find suitable clock settings.\n"
					"Turning FCK/PCK constraint off and"
					"trying again.\n");
			min_fck_per_pck = 0;
			goto retry;
		}

		DSSERR("Could not find suitable clock settings.\n");

		return -EINVAL;
	}

	if (cinfo)
		*cinfo = best;

	dispc.cache_req_pck = req_pck;
	dispc.cache_prate = prate;
	dispc.cache_cinfo = best;

    DSSDBG("[%s]-<; req_pck: %ld, {cinfo} fcd: %d, lcd: %d, pcd: %d, \
    fck: %ld, lck: %ld, pck: %ld\n", __func__, req_pck, cinfo->fck_div,
    cinfo->lck_div, cinfo->pck_div, cinfo->fck, cinfo->lck, cinfo->pck);

	return 0;
}

int dispc_set_clock_div(struct dispc_clock_info *cinfo)
{
	unsigned long prate;
	int r;

	if (cpu_is_omap34xx()) {
		prate = clk_get_rate(clk_get_parent(dispc.dpll4_m4_ck));
		DSSDBG("dpll4_m4 = %ld\n", prate);
	}

	DSSDBG("fck = %ld (%d)\n", cinfo->fck, cinfo->fck_div);
	DSSDBG("lck = %ld (%d)\n", cinfo->lck, cinfo->lck_div);
	DSSDBG("pck = %ld (%d)\n", cinfo->pck, cinfo->pck_div);

	if (cpu_is_omap34xx()) {
		r = clk_set_rate(dispc.dpll4_m4_ck, prate / cinfo->fck_div);
		if (r)
			return r;
	}

	dispc_set_lcd_divisor(cinfo->lck_div, cinfo->pck_div);

	return 0;
}

int dispc_get_clock_div(struct dispc_clock_info *cinfo)
{
	cinfo->fck = dss_clk_get_rate(DSS_CLK_FCK1);

	if (cpu_is_omap34xx()) {
		unsigned long prate;
		prate = clk_get_rate(clk_get_parent(dispc.dpll4_m4_ck));
		cinfo->fck_div = prate / (cinfo->fck / 2);
	} else {
		cinfo->fck_div = 0;
	}

	cinfo->lck_div = REG_GET(DISPC_DIVISOR, 23, 16);
	cinfo->pck_div = REG_GET(DISPC_DIVISOR, 7, 0);

	cinfo->lck = cinfo->fck / cinfo->lck_div;
	cinfo->pck = cinfo->lck / cinfo->pck_div;

	return 0;
}

/* dispc.irq_lock has to be locked by the caller */
static void _omap_dispc_set_irqs(void)
{
	u32 mask;
	u32 old_mask;
	int i;
	struct omap_dispc_isr_data *isr_data;

	mask = dispc.irq_error_mask;

	for (i = 0; i < DISPC_MAX_NR_ISRS; i++) {
		isr_data = &dispc.registered_isr[i];

		if (isr_data->isr == NULL)
			continue;

		mask |= isr_data->mask;
	}

	enable_clocks(1);

	old_mask = dispc_read_reg(DISPC_IRQENABLE);
	/* clear the irqstatus for newly enabled irqs */
	dispc_write_reg(DISPC_IRQSTATUS, (mask ^ old_mask) & mask);

	dispc_write_reg(DISPC_IRQENABLE, mask);

	enable_clocks(0);
}

int omap_dispc_register_isr(omap_dispc_isr_t isr, void *arg, u32 mask)
{
	int i;
	int ret;
	unsigned long flags;
	struct omap_dispc_isr_data *isr_data;

	if (isr == NULL)
		return -EINVAL;

	spin_lock_irqsave(&dispc.irq_lock, flags);

	/* check for duplicate entry */
	for (i = 0; i < DISPC_MAX_NR_ISRS; i++) {
		isr_data = &dispc.registered_isr[i];
		if (isr_data->isr == isr && isr_data->arg == arg &&
				isr_data->mask == mask) {
			ret = -EINVAL;
			goto err;
		}
	}

	isr_data = NULL;
	ret = -EBUSY;

	for (i = 0; i < DISPC_MAX_NR_ISRS; i++) {
		isr_data = &dispc.registered_isr[i];

		if (isr_data->isr != NULL)
			continue;

		isr_data->isr = isr;
		isr_data->arg = arg;
		isr_data->mask = mask;
		ret = 0;

		break;
	}

	_omap_dispc_set_irqs();

	spin_unlock_irqrestore(&dispc.irq_lock, flags);

	return 0;
err:
	spin_unlock_irqrestore(&dispc.irq_lock, flags);

	return ret;
}
EXPORT_SYMBOL(omap_dispc_register_isr);

int omap_dispc_unregister_isr(omap_dispc_isr_t isr, void *arg, u32 mask)
{
	int i;
	unsigned long flags;
	int ret = -EINVAL;
	struct omap_dispc_isr_data *isr_data;

	spin_lock_irqsave(&dispc.irq_lock, flags);

	for (i = 0; i < DISPC_MAX_NR_ISRS; i++) {
		isr_data = &dispc.registered_isr[i];
		if (isr_data->isr != isr || isr_data->arg != arg ||
				isr_data->mask != mask)
			continue;

		/* found the correct isr */

		isr_data->isr = NULL;
		isr_data->arg = NULL;
		isr_data->mask = 0;

		ret = 0;
		break;
	}

	if (ret == 0)
		_omap_dispc_set_irqs();

	spin_unlock_irqrestore(&dispc.irq_lock, flags);

	return ret;
}
EXPORT_SYMBOL(omap_dispc_unregister_isr);

#ifdef DEBUG
static void print_irq_status(u32 status)
{
	if ((status & dispc.irq_error_mask) == 0)
		return;

	printk(KERN_DEBUG "DISPC IRQ: 0x%x: ", status);

#define PIS(x) \
	if (status & DISPC_IRQ_##x) \
		printk(#x " ");
	PIS(GFX_FIFO_UNDERFLOW);
	PIS(OCP_ERR);
	PIS(SYNC_LOST);
#undef PIS

	printk("\n");
}
#endif

/* Called from dss.c. Note that we don't touch clocks here,
 * but we presume they are on because we got an IRQ. However,
 * an irq handler may turn the clocks off, so we may not have
 * clock later in the function. */
void dispc_irq_handler(void)
{
	int i;
	u32 irqstatus;
	u32 handledirqs = 0;
	u32 unhandled_errors;
	struct omap_dispc_isr_data *isr_data;
	struct omap_dispc_isr_data registered_isr[DISPC_MAX_NR_ISRS];

	spin_lock(&dispc.irq_lock);

	irqstatus = dispc_read_reg(DISPC_IRQSTATUS);

#ifdef DEBUG
	if (dss_debug)
		print_irq_status(irqstatus);
#endif
	/* Ack the interrupt. Do it here before clocks are possibly turned
	 * off */
	dispc_write_reg(DISPC_IRQSTATUS, irqstatus);

	/* make a copy and unlock, so that isrs can unregister
	 * themselves */
	memcpy(registered_isr, dispc.registered_isr,
			sizeof(registered_isr));

	spin_unlock(&dispc.irq_lock);

	for (i = 0; i < DISPC_MAX_NR_ISRS; i++) {
		isr_data = &registered_isr[i];

		if (!isr_data->isr)
			continue;

		if (isr_data->mask & irqstatus) {
			isr_data->isr(isr_data->arg, irqstatus);
			handledirqs |= isr_data->mask;
		}
	}

	spin_lock(&dispc.irq_lock);

	unhandled_errors = irqstatus & ~handledirqs & dispc.irq_error_mask;

	if (unhandled_errors) {
		dispc.error_irqs |= unhandled_errors;

		dispc.irq_error_mask &= ~unhandled_errors;
		_omap_dispc_set_irqs();

		schedule_work(&dispc.error_work);
	}

	spin_unlock(&dispc.irq_lock);
}

static void dispc_error_worker(struct work_struct *work)
{
	int i;
	u32 errors;
	unsigned long flags;

	spin_lock_irqsave(&dispc.irq_lock, flags);
	errors = dispc.error_irqs;
	dispc.error_irqs = 0;
	spin_unlock_irqrestore(&dispc.irq_lock, flags);

	if (errors & DISPC_IRQ_GFX_FIFO_UNDERFLOW) {
		DSSERR("GFX_FIFO_UNDERFLOW, disabling GFX\n");
		for (i = 0; i < omap_dss_get_num_overlays(); ++i) {
			struct omap_overlay *ovl;
			ovl = omap_dss_get_overlay(i);

			if (!(ovl->caps & OMAP_DSS_OVL_CAP_DISPC))
				continue;

			if (ovl->id == 0) {
				dispc_enable_plane(ovl->id, 0);
				dispc_go(ovl->manager->id);
				mdelay(50);
				break;
			}
		}
	}

	if (errors & DISPC_IRQ_SYNC_LOST) {
		struct omap_overlay_manager *manager = NULL;
		bool enable = false;

		DSSERR("SYNC_LOST, disabling LCD\n");

		for (i = 0; i < omap_dss_get_num_overlay_managers(); ++i) {
			struct omap_overlay_manager *mgr;
			mgr = omap_dss_get_overlay_manager(i);

			if (mgr->id == OMAP_DSS_CHANNEL_LCD) {
				manager = mgr;
				enable = mgr->device->state ==
						OMAP_DSS_DISPLAY_ACTIVE;
				mgr->device->disable(mgr->device);
				break;
			}
		}

		if (manager) {
			for (i = 0; i < omap_dss_get_num_overlays(); ++i) {
				struct omap_overlay *ovl;
				ovl = omap_dss_get_overlay(i);

				if (!(ovl->caps & OMAP_DSS_OVL_CAP_DISPC))
					continue;

				if (ovl->id != 0 && ovl->manager == manager)
					dispc_enable_plane(ovl->id, 0);
			}

			dispc_go(manager->id);
			mdelay(50);
			if (enable)
				manager->device->enable(manager->device);
		}
	}

	if (errors & DISPC_IRQ_OCP_ERR) {
		DSSERR("OCP_ERR\n");
		for (i = 0; i < omap_dss_get_num_overlay_managers(); ++i) {
			struct omap_overlay_manager *mgr;
			mgr = omap_dss_get_overlay_manager(i);

			if (mgr->caps & OMAP_DSS_OVL_CAP_DISPC)
				mgr->device->disable(mgr->device);
		}
	}

	spin_lock_irqsave(&dispc.irq_lock, flags);
	dispc.irq_error_mask |= errors;
	_omap_dispc_set_irqs();
	spin_unlock_irqrestore(&dispc.irq_lock, flags);
}

int omap_dispc_wait_for_irq_timeout(u32 irqmask, unsigned long timeout)
{
	void dispc_irq_wait_handler(void *data, u32 mask)
	{
		complete((struct completion *)data);
	}

	int r;
	DECLARE_COMPLETION_ONSTACK(completion);

	r = omap_dispc_register_isr(dispc_irq_wait_handler, &completion,
			irqmask);

	if (r)
		return r;

	timeout = wait_for_completion_timeout(&completion, timeout);

	omap_dispc_unregister_isr(dispc_irq_wait_handler, &completion, irqmask);

	if (timeout == 0)
		return -ETIMEDOUT;

	if (timeout == -ERESTARTSYS)
		return -ERESTARTSYS;

	return 0;
}

int omap_dispc_wait_for_irq_interruptible_timeout(u32 irqmask,
		unsigned long timeout)
{
	void dispc_irq_wait_handler(void *data, u32 mask)
	{
		complete((struct completion *)data);
	}

	int r;
	DECLARE_COMPLETION_ONSTACK(completion);

	r = omap_dispc_register_isr(dispc_irq_wait_handler, &completion,
			irqmask);

	if (r)
		return r;

	timeout = wait_for_completion_interruptible_timeout(&completion,
			timeout);

	omap_dispc_unregister_isr(dispc_irq_wait_handler, &completion, irqmask);

	if (timeout == 0)
		return -ETIMEDOUT;

	if (timeout == -ERESTARTSYS)
		return -ERESTARTSYS;

	return 0;
}

#ifdef CONFIG_OMAP2_DSS_FAKE_VSYNC
void dispc_fake_vsync_irq(void)
{
	u32 irqstatus = DISPC_IRQ_VSYNC;
	int i;

	for (i = 0; i < DISPC_MAX_NR_ISRS; i++) {
		struct omap_dispc_isr_data *isr_data;
		isr_data = &dispc.registered_isr[i];

		if (!isr_data->isr)
			continue;

		if (isr_data->mask & irqstatus)
			isr_data->isr(isr_data->arg, irqstatus);
	}
}
#endif

static void _omap_dispc_initialize_irq(void)
{
	unsigned long flags;

	spin_lock_irqsave(&dispc.irq_lock, flags);

	memset(dispc.registered_isr, 0, sizeof(dispc.registered_isr));

	dispc.irq_error_mask = DISPC_IRQ_MASK_ERROR;

	/* there's SYNC_LOST_DIGIT waiting after enabling the DSS,
	 * so clear it */
	dispc_write_reg(DISPC_IRQSTATUS, dispc_read_reg(DISPC_IRQSTATUS));

	_omap_dispc_set_irqs();

	spin_unlock_irqrestore(&dispc.irq_lock, flags);
}

void dispc_enable_sidle(void)
{
	REG_FLD_MOD(DISPC_SYSCONFIG, 2, 4, 3);	/* SIDLEMODE: smart idle */
}

void dispc_disable_sidle(void)
{
	REG_FLD_MOD(DISPC_SYSCONFIG, 1, 4, 3);	/* SIDLEMODE: no idle */
}

static void _omap_dispc_initial_config(void)
{
	u32 l;

	l = dispc_read_reg(DISPC_SYSCONFIG);
	l = FLD_MOD(l, 2, 13, 12);	/* MIDLEMODE: smart standby */
	l = FLD_MOD(l, 2, 4, 3);	/* SIDLEMODE: smart idle */
	l = FLD_MOD(l, 1, 2, 2);	/* ENWAKEUP */
	l = FLD_MOD(l, 1, 0, 0);	/* AUTOIDLE */
	dispc_write_reg(DISPC_SYSCONFIG, l);

	/* FUNCGATED */
	REG_FLD_MOD(DISPC_CONFIG, 1, 9, 9);

	dispc_set_loadmode(OMAP_DSS_LOAD_FRAME_ONLY);

	dispc_read_plane_fifo_sizes();
}

int dispc_init(void)
{
	u32 rev;

	spin_lock_init(&dispc.irq_lock);

	INIT_WORK(&dispc.error_work, dispc_error_worker);

	dispc.base = ioremap(DISPC_BASE, DISPC_SZ_REGS);
	if (!dispc.base) {
		DSSERR("can't ioremap DISPC\n");
		return -ENOMEM;
	}

	if (cpu_is_omap34xx()) {
		dispc.dpll4_m4_ck = clk_get(NULL, "dpll4_m4_ck");
		if (IS_ERR(dispc.dpll4_m4_ck)) {
			DSSERR("Failed to get dpll4_m4_ck\n");
			return -ENODEV;
		}
	}

	enable_clocks(1);

	_omap_dispc_initial_config();

	_omap_dispc_initialize_irq();

	dispc_save_context();

	rev = dispc_read_reg(DISPC_REVISION);
	printk(KERN_INFO "OMAP DISPC rev %d.%d\n",
	       FLD_GET(rev, 7, 4), FLD_GET(rev, 3, 0));

	enable_clocks(0);

	return 0;
}

void dispc_exit(void)
{
	if (cpu_is_omap34xx())
		clk_put(dispc.dpll4_m4_ck);
	iounmap(dispc.base);
}

int dispc_enable_plane(enum omap_plane plane, bool enable)
{
	DSSDBG("dispc_enable_plane %d, %d\n", plane, enable);

	enable_clocks(1);
	_dispc_enable_plane(plane, enable);
	enable_clocks(0);

	return 0;
}

int dispc_setup_plane(enum omap_plane plane,
		       u32 paddr, u16 screen_width,
		       u16 pos_x, u16 pos_y,
		       u16 width, u16 height,
		       u16 out_width, u16 out_height,
		       enum omap_color_mode color_mode,
		       bool ilace,
		       enum omap_dss_rotation_type rotation_type,
		       u8 rotation, bool mirror, u8 global_alpha)
{
	int r = 0;

	DSSDBG("dispc_setup_plane %d, pa %x, sw %d, %d,%d, %dx%d -> "
	       "%dx%d, ilace %d, cmode %x, rot %d, mir %d\n",
	       plane, paddr, screen_width, pos_x, pos_y,
	       width, height,
	       out_width, out_height,
	       ilace, color_mode,
	       rotation, mirror);

	enable_clocks(1);

	r = _dispc_setup_plane(plane,
			   paddr, screen_width,
			   pos_x, pos_y,
			   width, height,
			   out_width, out_height,
			   color_mode, ilace,
			   rotation_type,
			   rotation, mirror,
			   global_alpha);

	enable_clocks(0);

	return r;
}
