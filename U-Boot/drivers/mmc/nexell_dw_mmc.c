/*
 * (C) Copyright 2016 Nexell
 * Youngbok, Park <park@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <dwmmc.h>
#include <asm/arch/nexell.h>
#include <asm/arch/clk.h>
#include <asm/arch/reset.h>
#include <asm/arch/nx_gpio.h>
#include <asm/arch/tieoff.h>
#include <asm-generic/errno.h>
#include <fdtdec.h>
#include <libfdt.h>
static char *NX_NAME = "NEXELL DWMMC";

#define DWMCI_CLKSEL			0x09C
#define DWMCI_SHIFT_0			0x0
#define DWMCI_SHIFT_1			0x1
#define DWMCI_SHIFT_2			0x2
#define DWMCI_SHIFT_3			0x3
#define DWMCI_SET_SAMPLE_CLK(x)	(x)
#define DWMCI_SET_DRV_CLK(x)	((x) << 16)
#define DWMCI_SET_DIV_RATIO(x)	((x) << 24)

#define DWMCI_CLKCTRL			0x114

#define NX_MMC_CLK_DELAY(x, y, a, b)	(((x & 0xFF) << 0) |\
					((y & 0x03) << 16) |\
					((a & 0xFF) << 8)  |\
					((b & 0x03) << 24))

DECLARE_GLOBAL_DATA_PTR;

/* FIXME : This func will be remove after support pinctrl.
 * set mmc pad alternative func.
 */
static void set_mmc_pad_func(struct dwmci_host *host)
{
	switch (host->dev_index) {
	case 0:
		nx_gpio_set_pad_function(0, 29, 1);
		nx_gpio_set_pad_function(0, 31, 1);
		nx_gpio_set_pad_function(1, 1, 1);
		nx_gpio_set_pad_function(1, 3, 1);
		nx_gpio_set_pad_function(1, 5, 1);
		nx_gpio_set_pad_function(1, 7, 1);

		nx_gpio_set_drive_strength(0, 29, 2);
		nx_gpio_set_drive_strength(0, 31, 1);
		nx_gpio_set_drive_strength(1, 1, 1);
		nx_gpio_set_drive_strength(1, 3, 1);
		nx_gpio_set_drive_strength(1, 5, 1);
		nx_gpio_set_drive_strength(1, 7, 1);
		break;
	case 1:
		nx_gpio_set_pad_function(3, 22, 1);
		nx_gpio_set_pad_function(3, 23, 1);
		nx_gpio_set_pad_function(3, 24, 1);
		nx_gpio_set_pad_function(3, 25, 1);
		nx_gpio_set_pad_function(3, 26, 1);
		nx_gpio_set_pad_function(3, 27, 1);
		break;
	case 2:
		nx_gpio_set_pad_function(2, 18, 2);
		nx_gpio_set_pad_function(2, 19, 2);
		nx_gpio_set_pad_function(2, 20, 2);
		nx_gpio_set_pad_function(2, 21, 2);
		nx_gpio_set_pad_function(2, 22, 2);
		nx_gpio_set_pad_function(2, 23, 2);

		nx_gpio_set_drive_strength(2, 18, 2);
		nx_gpio_set_drive_strength(2, 19, 1);
		nx_gpio_set_drive_strength(2, 20, 1);
		nx_gpio_set_drive_strength(2, 21, 1);
		nx_gpio_set_drive_strength(2, 22, 1);
		nx_gpio_set_drive_strength(2, 23, 1);

		if (host->buswidth == 8) {
			nx_gpio_set_pad_function(4, 21, 2);
			nx_gpio_set_pad_function(4, 22, 2);
			nx_gpio_set_pad_function(4, 23, 2);
			nx_gpio_set_pad_function(4, 24, 2);

			nx_gpio_set_drive_strength(4, 21, 3);
			nx_gpio_set_drive_strength(4, 22, 3);
			nx_gpio_set_drive_strength(4, 23, 3);
			nx_gpio_set_drive_strength(4, 24, 3);
		}
		break;
	}
}

static unsigned int dw_mci_get_clk(struct dwmci_host *host, uint freq)
{
	struct clk *clk;
	int index = host->dev_index;
	char name[50] = {0, };

	sprintf(name, "%s.%d", DEV_NAME_SDHC, index);
	clk = clk_get((const char *)name);
	if (!clk)
		return 0;

	return clk_get_rate(clk)/2;
}

static unsigned long dw_mci_set_clk(int dev_index, unsigned  rate)
{
	struct clk *clk;
	char name[50];

	sprintf(name, "%s.%d", DEV_NAME_SDHC, dev_index);
	clk = clk_get((const char *)name);
	if (!clk)
		return 0;

	clk_disable(clk);
	rate = clk_set_rate(clk, rate);
	clk_enable(clk);

	return rate;
}

static void dw_mci_clksel(struct dwmci_host *host)
{
	u32 val;

	val = DWMCI_SET_SAMPLE_CLK(DWMCI_SHIFT_0) |
		DWMCI_SET_DRV_CLK(DWMCI_SHIFT_0) | DWMCI_SET_DIV_RATIO(3);

	dwmci_writel(host, DWMCI_CLKSEL, val);
}
static void dw_mci_clk_delay(const void *blob, int node,
			     struct dwmci_host *host)
{
	int drive_delay, drive_shift, sample_delay, sample_shift;
	int val;

	drive_delay = fdtdec_get_int(blob, node, "nexell,drive_dly", 0);
	drive_shift = fdtdec_get_int(blob, node, "nexell,drive_shift", 0);
	sample_delay = fdtdec_get_int(blob, node, "nexell,sample_dly", 0);
	sample_shift = fdtdec_get_int(blob, node, "nexell,sample_shift", 0);

	val = NX_MMC_CLK_DELAY(drive_delay, drive_shift,
				sample_delay, sample_shift);
	writel(val, (host->ioaddr + DWMCI_CLKCTRL));
}
static void dw_mci_reset(int ch)
{
	int rst_id = RESET_ID_SDMMC0 + ch;

	nx_rstcon_setrst(rst_id, 0);
	nx_rstcon_setrst(rst_id, 1);
}

struct dwmci_host *host = NULL;
static int nexell_get_config(const void *blob, int node,
			     struct dwmci_host *host)
{
	unsigned long base;
	int fifo_size = 0x20;

	host->dev_index = fdtdec_get_int(blob, node, "index", 0);
	host->buswidth = fdtdec_get_int(blob, node,
					 "nexell,bus-width", 0);
	if (host->buswidth <= 0) {
		printf("DW_MMC%d Can't get bus width\n", host->dev_index);
		return -EINVAL;
	}

	base = fdtdec_get_uint(blob, node, "reg", 0);
	if (!base) {
		printf("DWMMC%d: Can't get base address\n", host->dev_index);
		return -EINVAL;
	}
	host->ioaddr = (void *)base;

	host->name = NX_NAME;
	host->clksel = dw_mci_clksel;
	host->dev_id = host->dev_index;
	host->get_mmc_clk = dw_mci_get_clk;
	host->fifoth_val = MSIZE(0x2) | RX_WMARK(fifo_size/2 - 1)
	| TX_WMARK(fifo_size/2);

	if (fdtdec_get_int(blob, node, "nexell,ddr", 0))
		host->caps |= MMC_MODE_DDR_52MHz;
	return 0;
}

static int nexell_mmc_process_node(const void *blob, int node_list[], int count)
{
	int i, node, err;
	int freq;

	for (i = 0; i < count ; i++) {
		node = node_list[i];
		host = malloc(sizeof(struct dwmci_host));

		if (!host) {
			printf("dwmci_host malloc fail!\n");
			return 1;
		}
		memset(host, 0x00, sizeof(*host));
		err = nexell_get_config(blob, node, host);
		if (err) {
			printf("%s: failed to decode dev %d\n", __func__, i);
			return err;
		}

		freq = fdtdec_get_int(blob, node, "frequency", 0);
		dw_mci_set_clk(host->dev_index, freq * 4);

		add_dwmci(host, freq, 400000);
		set_mmc_pad_func(host);
#if defined (CONFIG_ARCH_S5P6818)
		if (host->buswidth == 8)
			nx_tieoff_set(NX_TIEOFF_MMC_8BIT, 1);
#endif
		dw_mci_reset(host->dev_index);
		dw_mci_clk_delay(blob, node, host);
	}
	return 0;
}

int nexell_mmc_init(const void  *blob)
{
	int compat_id;
	int err = 0;
	int count;
	int node_list[3] = { 0, };


	compat_id = COMPAT_NEXELL_DWMMC;
	count = fdtdec_find_aliases_for_id(blob, "mmc",
				compat_id, node_list, 3);

	err = nexell_mmc_process_node(blob, node_list, count);
	if (err) {
		printf("fail to add nexell DW_MMC\n");
		return err;
	}

	return err;
}
int board_mmc_init(bd_t *bis)
{
	int err;

	err = nexell_mmc_init(gd->fdt_blob);
	return err;
}
