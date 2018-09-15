/*
 * nxe2000.c  --  PMIC driver for the NEXELL NXE2000
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongshin, Park <pjsin865@nexell.co.kr>
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */

#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <power/nxe2000.h>
#ifdef CONFIG_REVISION_TAG
#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static const struct pmic_child_info pmic_children_info[] = {
	{ .prefix = "LDO", .driver = NXE2000_LDO_DRIVER },
	{ .prefix = "BUCK", .driver = NXE2000_BUCK_DRIVER },
	{ },
};

#ifdef CONFIG_REVISION_TAG
static void gpio_init(void)
{
	nx_gpio_set_pad_function(4, 4, 0);
	nx_gpio_set_pad_function(4, 5, 0);
	nx_gpio_set_pad_function(4, 6, 0);
	nx_gpio_set_output_value(4, 4, 0);
	nx_gpio_set_output_value(4, 5, 0);
	nx_gpio_set_output_value(4, 6, 0);
}

static u32 hw_revision(void)
{
	u32 val = 0;

	val |= nx_gpio_get_input_value(4, 6);
	val <<= 1;

	val |= nx_gpio_get_input_value(4, 5);
	val <<= 1;

	val |= nx_gpio_get_input_value(4, 4);

	return val;
}
#endif

static int nxe2000_reg_count(struct udevice *dev)
{
	return NXE2000_NUM_OF_REGS;
}

static int nxe2000_write(struct udevice *dev, uint reg, const uint8_t *buff,
			  int len)
{
	if (dm_i2c_write(dev, reg, buff, len)) {
		error("write error to device: %p register: %#x!", dev, reg);
		return -EIO;
	}

	return 0;
}

static int nxe2000_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	if (dm_i2c_read(dev, reg, buff, len)) {
		error("read error from device: %p register: %#x!", dev, reg);
		return -EIO;
	}

	return 0;
}

static int nxe2000_bind(struct udevice *dev)
{
	int regulators_node;
	const void *blob = gd->fdt_blob;
	int children;

#ifdef CONFIG_REVISION_TAG
	gpio_init();

	if ((hw_revision() < 3) && !strcmp(dev->name, "nxe2000_gpio@32"))
		return 0;

	if ((hw_revision() >= 3) && !strcmp(dev->name, "nxe2000@32"))
		return 0;
#endif

	debug("%s: dev->name:%s\n", __func__, dev->name);

	if (!strncmp(dev->name, "nxe2000", 7))
		regulators_node = fdt_subnode_offset(blob,
				fdt_path_offset(blob, "/"),
				"voltage-regulators");
	else
		regulators_node = fdt_subnode_offset(blob, dev->of_offset,
				"voltage-regulators");

	if (regulators_node <= 0) {
		debug("%s: regulators subnode not found!", __func__);
		return 0;
	}

	debug("%s: found regulators subnode\n", __func__);

	children = pmic_bind_children(dev, regulators_node, pmic_children_info);
	if (!children)
		debug("%s: %s - no child found\n", __func__, dev->name);

	/* Always return success for this device */
	return 0;
}

static struct dm_pmic_ops nxe2000_ops = {
	.reg_count = nxe2000_reg_count,
	.read = nxe2000_read,
	.write = nxe2000_write,
};

static const struct udevice_id nxe2000_ids[] = {
	{ .compatible = "nexell,nxe1500", .data = (ulong)TYPE_NXE1500 },
	{ .compatible = "nexell,nxe2000", .data = (ulong)TYPE_NXE2000 },
	{ }
};

U_BOOT_DRIVER(pmic_nxe2000) = {
	.name = "nxe2000_pmic",
	.id = UCLASS_PMIC,
	.of_match = nxe2000_ids,
	.bind = nxe2000_bind,
	.ops = &nxe2000_ops,
};
