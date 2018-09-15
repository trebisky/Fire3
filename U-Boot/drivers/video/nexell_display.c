/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <common.h>
#include <dm.h>
#include <dm.h>
#include <malloc.h>
#include <linux/compat.h>
#include <linux/err.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <video_fb.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/display.h>
#include "videomodes.h"

DECLARE_GLOBAL_DATA_PTR;

struct nx_display_dev {
	GraphicDevice graphic_device;
	unsigned long base;
	int module;
	struct dp_sync_info sync;
	struct dp_ctrl_info ctrl;
	struct dp_plane_top top;
	struct dp_plane_info planes[DP_PLANS_NUM];
	int dev_type;
	void *device;
	struct dp_plane_info *fb_plane;
	unsigned int depth;	/* byte per pixel */
	unsigned int fb_addr;
	unsigned int fb_size;
};

static char *const dp_dev_str[] = {
	[DP_DEVICE_RESCONV] = "RESCONV",
	[DP_DEVICE_RGBLCD] = "LCD",
	[DP_DEVICE_HDMI] = "HDMI",
	[DP_DEVICE_MIPI] = "MiPi",
	[DP_DEVICE_LVDS] = "LVDS",
	[DP_DEVICE_CVBS] = "TVOUT",
	[DP_DEVICE_DP0] = "DP0",
	[DP_DEVICE_DP1] = "DP1",
};

__weak int __dp_mipi_init(int width, int height, void *private_data)
{
	printf("[todo for mipi %s:%s]\n", __FILE__, __func__);
	return 0;
}

__weak void __dp_mipi_exit(int width, int height, void *private_data)
{
	printf("[todo for mipi %s:%s]\n", __FILE__, __func__);
}

#if CONFIG_IS_ENABLED(OF_CONTROL)
static void nx_display_parse_dp_sync(const void *blob, int node,
				     struct dp_sync_info *sync)
{
	sync->h_active_len = fdtdec_get_int(blob, node, "h_active_len", 0);
	sync->h_sync_width = fdtdec_get_int(blob, node, "h_sync_width", 0);
	sync->h_back_porch = fdtdec_get_int(blob, node, "h_back_porch", 0);
	sync->h_front_porch = fdtdec_get_int(blob, node, "h_front_porch", 0);
	sync->h_sync_invert = fdtdec_get_int(blob, node, "h_sync_invert", 0);
	sync->v_active_len = fdtdec_get_int(blob, node, "v_active_len", 0);
	sync->v_sync_width = fdtdec_get_int(blob, node, "v_sync_width", 0);
	sync->v_back_porch = fdtdec_get_int(blob, node, "v_back_porch", 0);
	sync->v_front_porch = fdtdec_get_int(blob, node, "v_front_porch", 0);
	sync->v_sync_invert = fdtdec_get_int(blob, node, "v_sync_invert", 0);
	sync->pixel_clock_hz = fdtdec_get_int(blob, node, "pixel_clock_hz", 0);

	debug("DP: sync ->\n");
	debug("ha:%d, hs:%d, hb:%d, hf:%d, hi:%d\n",
	      sync->h_active_len, sync->h_sync_width,
	      sync->h_back_porch, sync->h_front_porch, sync->h_sync_invert);
	debug("va:%d, vs:%d, vb:%d, vf:%d, vi:%d\n",
	      sync->v_active_len, sync->v_sync_width,
	      sync->v_back_porch, sync->v_front_porch, sync->v_sync_invert);
}

static void nx_display_parse_dp_ctrl(const void *blob, int node,
				     struct dp_ctrl_info *ctrl)
{
	/* clock gen */
	ctrl->clk_src_lv0 = fdtdec_get_int(blob, node, "clk_src_lv0", 0);
	ctrl->clk_div_lv0 = fdtdec_get_int(blob, node, "clk_div_lv0", 0);
	ctrl->clk_src_lv1 = fdtdec_get_int(blob, node, "clk_src_lv1", 0);
	ctrl->clk_div_lv1 = fdtdec_get_int(blob, node, "clk_div_lv1", 0);

	/* scan format */
	ctrl->interlace = fdtdec_get_int(blob, node, "interlace", 0);

	/* syncgen format */
	ctrl->out_format = fdtdec_get_int(blob, node, "out_format", 0);
	ctrl->invert_field = fdtdec_get_int(blob, node, "invert_field", 0);
	ctrl->swap_RB = fdtdec_get_int(blob, node, "swap_RB", 0);
	ctrl->yc_order = fdtdec_get_int(blob, node, "yc_order", 0);

	/* extern sync delay */
	ctrl->delay_mask = fdtdec_get_int(blob, node, "delay_mask", 0);
	ctrl->d_rgb_pvd = fdtdec_get_int(blob, node, "d_rgb_pvd", 0);
	ctrl->d_hsync_cp1 = fdtdec_get_int(blob, node, "d_hsync_cp1", 0);
	ctrl->d_vsync_fram = fdtdec_get_int(blob, node, "d_vsync_fram", 0);
	ctrl->d_de_cp2 = fdtdec_get_int(blob, node, "d_de_cp2", 0);

	/* extern sync delay */
	ctrl->vs_start_offset =
	    fdtdec_get_int(blob, node, "vs_start_offset", 0);
	ctrl->vs_end_offset = fdtdec_get_int(blob, node, "vs_end_offset", 0);
	ctrl->ev_start_offset =
	    fdtdec_get_int(blob, node, "ev_start_offset", 0);
	ctrl->ev_end_offset = fdtdec_get_int(blob, node, "ev_end_offset", 0);

	/* pad clock seletor */
	ctrl->vck_select = fdtdec_get_int(blob, node, "vck_select", 0);
	ctrl->clk_inv_lv0 = fdtdec_get_int(blob, node, "clk_inv_lv0", 0);
	ctrl->clk_delay_lv0 = fdtdec_get_int(blob, node, "clk_delay_lv0", 0);
	ctrl->clk_inv_lv1 = fdtdec_get_int(blob, node, "clk_inv_lv1", 0);
	ctrl->clk_delay_lv1 = fdtdec_get_int(blob, node, "clk_delay_lv1", 0);
	ctrl->clk_sel_div1 = fdtdec_get_int(blob, node, "clk_sel_div1", 0);

	debug("DP: ctrl [%s] ->\n",
	      ctrl->interlace ? "Interlace" : " Progressive");
	debug("cs0:%d, cd0:%d, cs1:%d, cd1:%d\n",
	      ctrl->clk_src_lv0, ctrl->clk_div_lv0,
	      ctrl->clk_src_lv1, ctrl->clk_div_lv1);
	debug("fmt:0x%x, inv:%d, swap:%d, yb:0x%x\n",
	      ctrl->out_format, ctrl->invert_field,
	      ctrl->swap_RB, ctrl->yc_order);
	debug("dm:0x%x, drp:%d, dhs:%d, dvs:%d, dde:0x%x\n",
	      ctrl->delay_mask, ctrl->d_rgb_pvd,
	      ctrl->d_hsync_cp1, ctrl->d_vsync_fram, ctrl->d_de_cp2);
	debug("vss:%d, vse:%d, evs:%d, eve:%d\n",
	      ctrl->vs_start_offset, ctrl->vs_end_offset,
	      ctrl->ev_start_offset, ctrl->ev_end_offset);
	debug("sel:%d, i0:%d, d0:%d, i1:%d, d1:%d, s1:%d\n",
	      ctrl->vck_select, ctrl->clk_inv_lv0, ctrl->clk_delay_lv0,
	      ctrl->clk_inv_lv1, ctrl->clk_delay_lv1, ctrl->clk_sel_div1);
}

static void nx_display_parse_dp_top_layer(const void *blob, int node,
					  struct dp_plane_top *top)
{
	top->screen_width = fdtdec_get_int(blob, node, "screen_width", 0);
	top->screen_height = fdtdec_get_int(blob, node, "screen_height", 0);
	top->video_prior = fdtdec_get_int(blob, node, "video_prior", 0);
	top->interlace = fdtdec_get_int(blob, node, "interlace", 0);
	top->back_color = fdtdec_get_int(blob, node, "back_color", 0);
	top->plane_num = DP_PLANS_NUM;

	debug("DP: top [%s] ->\n",
	      top->interlace ? "Interlace" : " Progressive");
	debug("w:%d, h:%d, prior:%d, bg:0x%x\n",
	      top->screen_width, top->screen_height,
	      top->video_prior, top->back_color);
}

static void nx_display_parse_dp_layer(const void *blob, int node,
				      struct dp_plane_info *plane)
{
	plane->fb_base = fdtdec_get_int(blob, node, "fb_base", 0);
	plane->format = fdtdec_get_int(blob, node, "format", 0);
	plane->left = fdtdec_get_int(blob, node, "left", 0);
	plane->width = fdtdec_get_int(blob, node, "width", 0);
	plane->top = fdtdec_get_int(blob, node, "top", 0);
	plane->height = fdtdec_get_int(blob, node, "height", 0);
	plane->pixel_byte = fdtdec_get_int(blob, node, "pixel_byte", 0);
	plane->format = fdtdec_get_int(blob, node, "format", 0);
	plane->alpha_on = fdtdec_get_int(blob, node, "alpha_on", 0);
	plane->alpha_depth = fdtdec_get_int(blob, node, "alpha", 0);
	plane->tp_on = fdtdec_get_int(blob, node, "tp_on", 0);
	plane->tp_color = fdtdec_get_int(blob, node, "tp_color", 0);

	/* enable layer */
	if (plane->fb_base)
		plane->enable = 1;
	else
		plane->enable = 0;

	if (0 == plane->fb_base) {
		printf("fail : dp plane.%d invalid fb base [0x%x] ->\n",
		       plane->layer, plane->fb_base);
		return;
	}

	debug("DP: plane.%d [0x%x] ->\n", plane->layer, plane->fb_base);
	debug("f:0x%x, l:%d, t:%d, %d * %d, bpp:%d, a:%d(%d), t:%d(0x%x)\n",
	      plane->format, plane->left, plane->top, plane->width,
	      plane->height, plane->pixel_byte, plane->alpha_on,
	      plane->alpha_depth, plane->tp_on, plane->tp_color);
}

static void nx_display_parse_dp_planes(const void *blob, int node,
				       struct nx_display_dev *dp)
{
	const char *name;
	for (node = fdt_first_subnode(blob, node);
	     node > 0; node = fdt_next_subnode(blob, node)) {
		name = fdt_get_name(blob, node, NULL);

		if (0 == strcmp(name, "layer_top"))
			nx_display_parse_dp_top_layer(blob, node, &dp->top);

		if (0 == strcmp(name, "layer_0"))
			nx_display_parse_dp_layer(blob, node, &dp->planes[0]);

		if (0 == strcmp(name, "layer_1"))
			nx_display_parse_dp_layer(blob, node, &dp->planes[1]);

		if (0 == strcmp(name, "layer_2"))
			nx_display_parse_dp_layer(blob, node, &dp->planes[2]);
	}
}

static int nx_display_parse_dp_lvds(const void *blob, int node,
				    struct nx_display_dev *dp)
{
	struct dp_lvds_dev *dev = kzalloc(sizeof(*dev), GFP_KERNEL);

	if (!dev) {
		printf("failed to allocate display LVDS object.\n");
		return -ENOMEM;
	}

	dp->device = dev;

	dev->lvds_format = fdtdec_get_int(blob, node, "format", 0);
	dev->pol_inv_hs = fdtdec_get_int(blob, node, "pol_inv_hs", 0);
	dev->pol_inv_vs = fdtdec_get_int(blob, node, "pol_inv_vs", 0);
	dev->pol_inv_de = fdtdec_get_int(blob, node, "pol_inv_de", 0);
	dev->pol_inv_ck = fdtdec_get_int(blob, node, "pol_inv_ck", 0);
	dev->voltage_level = fdtdec_get_int(blob, node, "voltage_level", 0);

	if (!dev->voltage_level)
		dev->voltage_level = DEF_VOLTAGE_LEVEL;

	debug("DP: LVDS -> %s, voltage LV:0x%x\n",
	      dev->lvds_format == DP_LVDS_FORMAT_VESA ? "VESA" :
	      dev->lvds_format == DP_LVDS_FORMAT_JEIDA ? "JEIDA" : "LOC",
	      dev->voltage_level);
	debug("pol inv hs:%d, vs:%d, de:%d, ck:%d\n",
	      dev->pol_inv_hs, dev->pol_inv_vs,
	      dev->pol_inv_de, dev->pol_inv_ck);

	return 0;
}

static int nx_display_parse_dp_rgb(const void *blob, int node,
				   struct nx_display_dev *dp)
{
	struct dp_rgb_dev *dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		printf("failed to allocate display RGB LCD object.\n");
		return -ENOMEM;
	}
	dp->device = dev;

	dev->lcd_mpu_type = fdtdec_get_int(blob, node, "lcd_mpu_type", 0);

	debug("DP: RGB -> MPU[%s]\n", dev->lcd_mpu_type ? "O" : "X");
	return 0;
}

static int nx_display_parse_dp_mipi(const void *blob, int node,
				    struct nx_display_dev *dp)
{
	struct dp_mipi_dev *dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		printf("failed to allocate display MiPi object.\n");
		return -ENOMEM;
	}
	dp->device = dev;

	dev->lp_bitrate = fdtdec_get_int(blob, node, "lp_bitrate", 0);
	dev->hs_bitrate = fdtdec_get_int(blob, node, "hs_bitrate", 0);
	dev->dev_init = __dp_mipi_init;
	dev->dev_exit = __dp_mipi_exit;

	debug("DP: MIPI ->\n");
	debug("lp:%dmhz, hs:%dmhz\n", dev->lp_bitrate, dev->hs_bitrate);

	return 0;
}

static int nx_display_parse_dp_hdmi(const void *blob, int node,
				    struct nx_display_dev *dp)
{
	struct dp_hdmi_dev *dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		printf("failed to allocate display HDMI object.\n");
		return -ENOMEM;
	}
	dp->device = dev;

	dev->preset = fdtdec_get_int(blob, node, "preset", 0);

	debug("DP: HDMI -> %d\n", dev->preset);

	return 0;
}

static int nx_display_parse_dp_lcds(const void *blob, int node,
				    const char *type, struct nx_display_dev *dp)
{
	if (0 == strcmp(type, "lvds")) {
		dp->dev_type = DP_DEVICE_LVDS;
		return nx_display_parse_dp_lvds(blob, node, dp);
	} else if (0 == strcmp(type, "rgb")) {
		dp->dev_type = DP_DEVICE_RGBLCD;
		return nx_display_parse_dp_rgb(blob, node, dp);
	} else if (0 == strcmp(type, "mipi")) {
		dp->dev_type = DP_DEVICE_MIPI;
		return nx_display_parse_dp_mipi(blob, node, dp);
	} else if (0 == strcmp(type, "hdmi")) {
		dp->dev_type = DP_DEVICE_HDMI;
		return nx_display_parse_dp_hdmi(blob, node, dp);
	} else {
		printf("%s: node %s unknown display type\n", __func__,
		       fdt_get_name(blob, node, NULL));
		return -EINVAL;
	}

	return 0;
}

#define	DT_SYNC		(1<<0)
#define	DT_CTRL		(1<<1)
#define	DT_PLANES	(1<<2)
#define	DT_DEVICE	(1<<3)

static int nx_display_parse_dt(const void *blob,
			struct nx_display_dev *dp, int node)
{
	const char *name, *dtype;
	int ret = 0;
	unsigned int dt_status = 0;

	if (!node) {
		node = fdtdec_next_compatible(blob, 0, COMPAT_NEXELL_DISPLAY);
		if (0 >= node) {
			printf("%s: can't get device node for dp\n", __func__);
			return -ENODEV;
		}
	}

	dp->module = fdtdec_get_int(blob, node, "module", -1);
	if (-1 == dp->module)
		dp->module = fdtdec_get_int(blob, node, "index", 0);

	dtype = fdt_getprop(blob, node, "lcd-type", NULL);

	for (node = fdt_first_subnode(blob, node);
	     node > 0; node = fdt_next_subnode(blob, node)) {
		name = fdt_get_name(blob, node, NULL);

		if (0 == strcmp("dp-sync", name)) {
			dt_status |= DT_SYNC;
			nx_display_parse_dp_sync(blob, node, &dp->sync);
		}

		if (0 == strcmp("dp-ctrl", name)) {
			dt_status |= DT_CTRL;
			nx_display_parse_dp_ctrl(blob, node, &dp->ctrl);
		}

		if (0 == strcmp("dp-planes", name)) {
			dt_status |= DT_PLANES;
			nx_display_parse_dp_planes(blob, node, dp);
		}

		if (0 == strcmp("dp-device", name)) {
			dt_status |= DT_DEVICE;
			ret = nx_display_parse_dp_lcds(blob, node, dtype, dp);
		}
	}

	if (dt_status != (DT_SYNC | DT_CTRL | DT_PLANES | DT_DEVICE)) {
		printf("Not enough DT config for display [0x%x]\n", dt_status);
		return -ENODEV;
	}

	return ret;
}
#endif

static struct nx_display_dev *nx_display_setup(void)
{
	struct nx_display_dev *dp;
	int i, ret, node = 0;

#ifdef CONFIG_DM
	struct udevice *dev;

	/* call driver probe */
	debug("DT: uclass device call...\n");
	ret = uclass_get_device(UCLASS_DISPLAY_PORT, 0, &dev);
	if (ret)
		return NULL;

	dp = dev_get_priv(dev);
	node = dev->of_offset;

#elif defined CONFIG_OF_CONTROL
	dp = kzalloc(sizeof(*dp), GFP_KERNEL);
	if (!dp) {
		printf("failed to allocate display object.\n");
		return NULL;
	}
#endif

#if CONFIG_IS_ENABLED(OF_CONTROL)
	ret = nx_display_parse_dt(gd->fdt_blob, dp, node);
	if (ret)
		goto err_setup;
#endif

	for (i = 0; dp->top.plane_num > i; i++) {
		dp->planes[i].layer = i;
		if (dp->planes[i].enable && !dp->fb_plane) {
			dp->fb_plane = &dp->planes[i];
			dp->fb_addr = dp->fb_plane->fb_base;
			dp->depth = dp->fb_plane->pixel_byte;
		}
	}

	switch (dp->dev_type) {
#ifdef CONFIG_VIDEO_NX_RGB
	case DP_DEVICE_RGBLCD:
		nx_rgb_display(dp->module,
			       &dp->sync, &dp->ctrl, &dp->top,
			       dp->planes, (struct dp_rgb_dev *)dp->device);
		break;
#endif
#ifdef CONFIG_VIDEO_NX_LVDS
	case DP_DEVICE_LVDS:
		nx_lvds_display(dp->module,
				&dp->sync, &dp->ctrl, &dp->top,
				dp->planes, (struct dp_lvds_dev *)dp->device);
		break;
#endif
#ifdef CONFIG_VIDEO_NX_MIPI
	case DP_DEVICE_MIPI:
		nx_mipi_display(dp->module,
				&dp->sync, &dp->ctrl, &dp->top,
				dp->planes, (struct dp_mipi_dev *)dp->device);
		break;
#endif
#ifdef CONFIG_VIDEO_NX_HDMI
	case DP_DEVICE_HDMI:
		nx_hdmi_display(dp->module,
				&dp->sync, &dp->ctrl, &dp->top,
				dp->planes, (struct dp_hdmi_dev *)dp->device);
		break;
#endif
	default:
		printf("fail : not support lcd type %d !!!\n", dp->dev_type);
		goto err_setup;
	};

	printf("LCD: [%s] dp.%d.%d %dx%d %dbpp FB:0x%08x\n",
	       dp_dev_str[dp->dev_type], dp->module, dp->fb_plane->layer,
	       dp->fb_plane->width, dp->fb_plane->height, dp->depth * 8,
	       dp->fb_addr);

	return dp;

err_setup:
	kfree(dp);

	return NULL;
}

void *video_hw_init(void)
{
	static GraphicDevice *graphic_device;
	struct nx_display_dev *dp;
	unsigned int pp_index = 0;

	dp = nx_display_setup();
	if (!dp)
		return NULL;

	switch (dp->depth) {
	case 2:
		pp_index = GDF_16BIT_565RGB;
		break;
	case 3:
		pp_index = GDF_24BIT_888RGB;
		break;
	case 4:
		pp_index = GDF_32BIT_X888RGB;
		break;
	default:
		printf("fail : not support LCD bit per pixel %d\n",
		       dp->depth * 8);
		return NULL;
	}

	graphic_device = &dp->graphic_device;
	graphic_device->frameAdrs = dp->fb_addr;
	graphic_device->gdfIndex = pp_index;
	graphic_device->gdfBytesPP = dp->depth;
	graphic_device->winSizeX = dp->fb_plane->width;
	graphic_device->winSizeY = dp->fb_plane->height;
	graphic_device->plnSizeX =
	    graphic_device->winSizeX * graphic_device->gdfBytesPP;

	return graphic_device;
}

#ifdef CONFIG_DM
static int nx_display_probe(struct udevice *dev)
{
	struct nx_display_platdata *plat = dev_get_platdata(dev);
	struct nx_display_dev *dp = dev_get_priv(dev);
	int i = 0;
	if (!plat)
		return -EINVAL;

	dp->module = plat->module;
	dp->dev_type = plat->dev_type;
	dp->device = plat->device;

	memcpy(&dp->sync, &plat->sync, sizeof(struct dp_sync_info));
	memcpy(&dp->ctrl, &plat->ctrl, sizeof(struct dp_ctrl_info));
	memcpy(&dp->top, &plat->top, sizeof(struct dp_plane_top));
	memcpy(&dp->planes, plat->plane,
	       sizeof(struct dp_plane_info) * DP_PLANS_NUM);

	dp->top.plane_num = DP_PLANS_NUM;

	for (i = 0; dp->top.plane_num > i; i++) {
		dp->planes[i].layer = i;
		if (dp->planes[i].fb_base && dp->planes[i].width &&
		    dp->planes[i].height && dp->planes[i].pixel_byte &&
		    dp->planes[i].format)
			dp->planes[i].enable = 1;
		else
			dp->planes[i].enable = 0;
	}

	return 0;
}

#if CONFIG_IS_ENABLED(OF_CONTROL)
static const struct udevice_id nx_display_id[] = {
	{.compatible = "nexell,nexell-display", },
	{}
};
#endif

U_BOOT_DRIVER(nexell_display) = {
	.name = "nexell-display",
	.id = UCLASS_DISPLAY_PORT,
	.of_match = of_match_ptr(nx_display_id),
	.platdata_auto_alloc_size =
	    sizeof(struct nx_display_platdata),
	.probe = nx_display_probe,
	.priv_auto_alloc_size = sizeof(struct nx_display_dev),
};
#endif
