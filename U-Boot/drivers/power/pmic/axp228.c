/*
 * axp228.c  --  PMIC driver for the X-Powers AXP228
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
#include <power/axp228.h>

DECLARE_GLOBAL_DATA_PTR;

static const struct pmic_child_info pmic_children_info[] = {
	{ .prefix = "LDO", .driver = AXP228_LDO_DRIVER },
	{ .prefix = "BUCK", .driver = AXP228_BUCK_DRIVER },
	{ },
};

static int axp228_reg_count(struct udevice *dev)
{
	return AXP228_NUM_OF_REGS;
}

static int axp228_write(struct udevice *dev, uint reg, const uint8_t *buff,
			  int len)
{
	if (dm_i2c_write(dev, reg, buff, len)) {
		error("write error to device: %p register: %#x!", dev, reg);
		return -EIO;
	}

	return 0;
}

static int axp228_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	if (dm_i2c_read(dev, reg, buff, len)) {
		error("read error from device: %p register: %#x!", dev, reg);
		return -EIO;
	}

	return 0;
}

static int axp228_bind(struct udevice *dev)
{
	int regulators_node;
	const void *blob = gd->fdt_blob;
	int children;

	regulators_node = fdt_subnode_offset(blob, dev->of_offset,
					     "voltage-regulators");
	if (regulators_node <= 0) {
		debug("%s: %s regulators subnode not found!", __func__,
							     dev->name);
		return -ENXIO;
	}

	debug("%s: '%s' - found regulators subnode\n", __func__, dev->name);

	children = pmic_bind_children(dev, regulators_node, pmic_children_info);
	if (!children)
		debug("%s: %s - no child found\n", __func__, dev->name);

	/* Always return success for this device */
	return 0;
}

static struct dm_pmic_ops axp228_ops = {
	.reg_count = axp228_reg_count,
	.read = axp228_read,
	.write = axp228_write,
};

static const struct udevice_id axp228_ids[] = {
	{ .compatible = "x-powers,axp228" },
	{ }
};

U_BOOT_DRIVER(pmic_axp228) = {
	.name = "axp228_pmic",
	.id = UCLASS_PMIC,
	.of_match = axp228_ids,
	.bind = axp228_bind,
	.ops = &axp228_ops,
};
