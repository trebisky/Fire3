/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <common.h>
#include <errno.h>

#include <asm/arch/nexell.h>
#include <asm/arch/tieoff.h>
#include <asm/arch/reset.h>
#include <asm/arch/display.h>

#include "soc/s5pxx18_soc_mipi.h"
#include "soc/s5pxx18_soc_disptop.h"
#include "soc/s5pxx18_soc_disptop_clk.h"

#define	PLLPMS_1000MHZ		0x33E8
#define	BANDCTL_1000MHZ		0xF
#define PLLPMS_960MHZ       0x2280
#define BANDCTL_960MHZ      0xF
#define	PLLPMS_900MHZ		0x2258
#define	BANDCTL_900MHZ		0xE
#define	PLLPMS_840MHZ		0x2230
#define	BANDCTL_840MHZ		0xD
#define	PLLPMS_750MHZ		0x43E8
#define	BANDCTL_750MHZ		0xC
#define	PLLPMS_660MHZ		0x21B8
#define	BANDCTL_660MHZ		0xB
#define	PLLPMS_600MHZ		0x2190
#define	BANDCTL_600MHZ		0xA
#define	PLLPMS_540MHZ		0x2168
#define	BANDCTL_540MHZ		0x9
#define	PLLPMS_512MHZ		0x03200
#define	BANDCTL_512MHZ		0x9
#define	PLLPMS_480MHZ		0x2281
#define	BANDCTL_480MHZ		0x8
#define	PLLPMS_420MHZ		0x2231
#define	BANDCTL_420MHZ		0x7
#define	PLLPMS_402MHZ		0x2219
#define	BANDCTL_402MHZ		0x7
#define	PLLPMS_330MHZ		0x21B9
#define	BANDCTL_330MHZ		0x6
#define	PLLPMS_300MHZ		0x2191
#define	BANDCTL_300MHZ		0x5
#define	PLLPMS_210MHZ		0x2232
#define	BANDCTL_210MHZ		0x4
#define	PLLPMS_180MHZ		0x21E2
#define	BANDCTL_180MHZ		0x3
#define	PLLPMS_150MHZ		0x2192
#define	BANDCTL_150MHZ		0x2
#define	PLLPMS_100MHZ		0x3323
#define	BANDCTL_100MHZ		0x1
#define	PLLPMS_80MHZ		0x3283
#define	BANDCTL_80MHZ		0x0

#define	__io_address(a)	(void *)(uintptr_t)(a)

static void mipi_reset(void)
{
	/* tieoff */
	nx_tieoff_set(NX_TIEOFF_MIPI0_NX_DPSRAM_1R1W_EMAA, 3);
	nx_tieoff_set(NX_TIEOFF_MIPI0_NX_DPSRAM_1R1W_EMAB, 3);

	/* reset */
	nx_rstcon_setrst(RESET_ID_MIPI, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_MIPI_DSI, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_MIPI_CSI, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_MIPI_PHY_S, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_MIPI_PHY_M, RSTCON_ASSERT);

	nx_rstcon_setrst(RESET_ID_MIPI, RSTCON_NEGATE);
	nx_rstcon_setrst(RESET_ID_MIPI_DSI, RSTCON_NEGATE);
	nx_rstcon_setrst(RESET_ID_MIPI_PHY_S, RSTCON_NEGATE);
	nx_rstcon_setrst(RESET_ID_MIPI_PHY_M, RSTCON_NEGATE);
}

static void mipi_init(void)
{
	int clkid = DP_CLOCK_MIPI;
	void *base;

	/*
	 * neet to reset befor open
	 */
	mipi_reset();

	base = __io_address(nx_disp_top_clkgen_get_physical_address(clkid));
	nx_disp_top_clkgen_set_base_address(clkid, base);
	nx_disp_top_clkgen_set_clock_pclk_mode(clkid, nx_pclkmode_always);

	base = __io_address(nx_mipi_get_physical_address(0));
	nx_mipi_set_base_address(0, base);

	nx_mipi_open_module(0);
}

static void mipi_enable(int enable)
{
	int clkid = DP_CLOCK_MIPI;
	int on = (enable ? 1 : 0);

	/* SPDIF and MIPI */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 1);

	/* START: CLKGEN, MIPI is started in setup function */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, on);
	nx_mipi_dsi_set_enable(0, on);
}

static int mipi_phy_pll(int bitrate, unsigned int *pllpms,
			unsigned int *bandctl)
{
	unsigned int pms, ctl;

	switch (bitrate) {
	case 1000:
		pms = PLLPMS_1000MHZ;
		ctl = BANDCTL_1000MHZ;
		break;
	case 960:
		pms = PLLPMS_960MHZ;
		ctl = BANDCTL_960MHZ;
		break;
	case 900:
		pms = PLLPMS_900MHZ;
		ctl = BANDCTL_900MHZ;
		break;
	case 840:
		pms = PLLPMS_840MHZ;
		ctl = BANDCTL_840MHZ;
		break;
	case 750:
		pms = PLLPMS_750MHZ;
		ctl = BANDCTL_750MHZ;
		break;
	case 660:
		pms = PLLPMS_660MHZ;
		ctl = BANDCTL_660MHZ;
		break;
	case 600:
		pms = PLLPMS_600MHZ;
		ctl = BANDCTL_600MHZ;
		break;
	case 540:
		pms = PLLPMS_540MHZ;
		ctl = BANDCTL_540MHZ;
		break;
	case 512:
		pms = PLLPMS_512MHZ;
		ctl = BANDCTL_512MHZ;
		break;
	case 480:
		pms = PLLPMS_480MHZ;
		ctl = BANDCTL_480MHZ;
		break;
	case 420:
		pms = PLLPMS_420MHZ;
		ctl = BANDCTL_420MHZ;
		break;
	case 402:
		pms = PLLPMS_402MHZ;
		ctl = BANDCTL_402MHZ;
		break;
	case 330:
		pms = PLLPMS_330MHZ;
		ctl = BANDCTL_330MHZ;
		break;
	case 300:
		pms = PLLPMS_300MHZ;
		ctl = BANDCTL_300MHZ;
		break;
	case 210:
		pms = PLLPMS_210MHZ;
		ctl = BANDCTL_210MHZ;
		break;
	case 180:
		pms = PLLPMS_180MHZ;
		ctl = BANDCTL_180MHZ;
		break;
	case 150:
		pms = PLLPMS_150MHZ;
		ctl = BANDCTL_150MHZ;
		break;
	case 100:
		pms = PLLPMS_100MHZ;
		ctl = BANDCTL_100MHZ;
		break;
	case 80:
		pms = PLLPMS_80MHZ;
		ctl = BANDCTL_80MHZ;
		break;
	default:
		return -EINVAL;
	}

	*pllpms = pms;
	*bandctl = ctl;

	return 0;
}

static int mipi_setup(int module, int input,
		      struct dp_sync_info *sync, struct dp_ctrl_info *ctrl,
		      struct dp_mipi_dev *dev)
{
	int clkid = DP_CLOCK_MIPI;
	int width = sync->h_active_len;
	int height = sync->v_active_len;
	int index = 0;
	int ret = 0;

	int HFP = sync->h_front_porch;
	int HBP = sync->h_back_porch;
	int HS = sync->h_sync_width;
	int VFP = sync->v_front_porch;
	int VBP = sync->v_back_porch;
	int VS = sync->v_sync_width;

	unsigned int hs_pllpms, hs_bandctl;
	unsigned int lp_pllpms, lp_bandctl;

	unsigned int pllctl = 0;
	unsigned int phyctl = 0;

	u32 esc_pre_value = 1;

	ret = mipi_phy_pll(dev->hs_bitrate, &hs_pllpms, &hs_bandctl);
	if (0 > ret)
		return ret;

	ret = mipi_phy_pll(dev->lp_bitrate, &lp_pllpms, &lp_bandctl);
	if (0 > ret)
		return ret;

	switch (input) {
	case DP_DEVICE_DP0:
		input = 0;
		break;
	case DP_DEVICE_DP1:
		input = 1;
		break;
	case DP_DEVICE_RESCONV:
		input = 2;
		break;
	default:
		return -EINVAL;
	}

	debug("%s: mipi lp:%dmhz:0x%x:0x%x, hs:%dmhz:0x%x:0x%x\n",
	      __func__, dev->lp_bitrate, lp_pllpms, lp_bandctl,
	      dev->hs_bitrate, hs_pllpms, hs_bandctl);

	if (dev->dev_init) {
		nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
				    lp_pllpms, lp_bandctl, 0, 0);
		mdelay(20);

		nx_mipi_dsi_software_reset(index);
		nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, 1,
				      esc_pre_value);
		nx_mipi_dsi_set_phy(index, 0, 1, 1, 0, 0, 0, 0, 0);

		nx_mipi_dsi_set_escape_lp(index, nx_mipi_dsi_lpmode_lp,
					  nx_mipi_dsi_lpmode_lp);
		mdelay(10);

		/* run callback */
		ret = dev->dev_init(width, height, dev->private_data);

		nx_mipi_dsi_set_escape_lp(index, nx_mipi_dsi_lpmode_hs,
					  nx_mipi_dsi_lpmode_hs);

		if (0 > ret)
			return ret;
	}

	nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
			    hs_pllpms, hs_bandctl, pllctl, phyctl);
	mdelay(1);

	nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, 1, 10);
	mdelay(1);

	nx_mipi_dsi_software_reset(index);
	nx_mipi_dsi_set_clock(index, 1, 0, 1, 1, 1, 1, 1, 1, 1, esc_pre_value);
	nx_mipi_dsi_set_phy(index, 3, 1, 1, 1, 1, 1, 0, 0);
	nx_mipi_dsi_set_config_video_mode(index, 1, 0, 1,
					  nx_mipi_dsi_syncmode_event, 1, 1, 1,
					  1, 1, 0, nx_mipi_dsi_format_rgb888,
					  HFP, HBP, HS, VFP, VBP, VS, 0);

	nx_mipi_dsi_set_size(index, width, height);
	nx_disp_top_set_mipimux(1, input);

	/*  0 is spdif, 1 is mipi vclk */
	nx_disp_top_clkgen_set_clock_source(clkid, 1, ctrl->clk_src_lv0);
	nx_disp_top_clkgen_set_clock_divisor(clkid, 1,
					     ctrl->clk_div_lv1 *
					     ctrl->clk_div_lv0);
	return 0;
}

void nx_mipi_write_header(u32 data)
{
	nx_mipi_dsi_write_pkheader(0, data);
}

void nx_mipi_write_payload(u32 data)
{
	nx_mipi_dsi_write_payload(0, data);
}

/*
 * disply
 * MIPI DSI Setting
 *		(1) Initiallize MIPI(DSIM,DPHY,PLL)
 *		(2) Initiallize LCD
 *		(3) ReInitiallize MIPI(DSIM only)
 *		(4) Turn on display(MLC,DPC,...)
 */
void nx_mipi_display(int module,
		     struct dp_sync_info *sync, struct dp_ctrl_info *ctrl,
		     struct dp_plane_top *top, struct dp_plane_info *planes,
		     struct dp_mipi_dev *dev)
{
	struct dp_plane_info *plane = planes;
	int input = module == 0 ? DP_DEVICE_DP0 : DP_DEVICE_DP1;
	int count = top->plane_num;
	int i = 0;

	printf("MIPI: dp.%d\n", module);

	dp_control_init(module);
	dp_plane_init(module);

	mipi_init();

	/* set plane */
	dp_plane_screen_setup(module, top);

	for (i = 0; count > i; i++, plane++) {
		if (!plane->enable)
			continue;
		dp_plane_layer_setup(module, plane);
		dp_plane_layer_enable(module, plane, 1);
	}
	dp_plane_screen_enable(module, 1);

	/* set mipi */
	mipi_setup(module, input, sync, ctrl, dev);

	mipi_enable(1);

	/* set dp control */
	dp_control_setup(module, sync, ctrl);
	dp_control_enable(module, 1);
}
