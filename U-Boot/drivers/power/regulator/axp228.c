/*
 * axp228.c  --  Regulator driver for the X-Powers AXP228
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

static const struct sec_voltage_desc buck_v1 = {
	.max = 3400000,
	.min = 1600000,
	.step = 100000,
};

static const struct sec_voltage_desc buck_v2 = {
	.max = 1540000,
	.min = 600000,
	.step = 20000,
};

static const struct sec_voltage_desc buck_v3 = {
	.max = 1860000,
	.min = 600000,
	.step = 20000,
};

static const struct sec_voltage_desc buck_v4 = {
	.max = 2550000,
	.min = 1000000,
	.step = 50000,
};




static const struct sec_voltage_desc ldo_v1 = {
	.max = 3300000,
	.min = 700000,
	.step = 100000,
};


static const struct sec_voltage_desc ldo_v2 = {
	.max = 1400000,
	.min = 700000,
	.step = 100000,
};

static const struct axp228_para axp228_buck_param[] = {
	{AXP228_ID_DCDC1, 0x21, 0x0, 0x1f, 0x10, 1, &buck_v1},
	{AXP228_ID_DCDC2, 0x22, 0x0, 0x3f, 0x10, 2, &buck_v2},
	{AXP228_ID_DCDC3, 0x23, 0x0, 0x3f, 0x10, 3, &buck_v3},
	{AXP228_ID_DCDC4, 0x24, 0x0, 0x3f, 0x10, 4, &buck_v2},
	{AXP228_ID_DCDC5, 0x25, 0x0, 0x1f, 0x10, 5, &buck_v4},
};

static const struct axp228_para axp228_ldo_param[] = {
	{AXP228_ID_ALDO1, 0x28, 0x0, 0x3f, 0x10, 6, &ldo_v1},
	{AXP228_ID_ALDO2, 0x29, 0x0, 0x3f, 0x10, 7, &ldo_v1},
	{AXP228_ID_ALDO3, 0x2A, 0x0, 0x3f, 0x13, 7, &ldo_v1},
	{AXP228_ID_DLDO1, 0x15, 0x0, 0x1f, 0x12, 3, &ldo_v1},
	{AXP228_ID_DLDO2, 0x16, 0x0, 0x1f, 0x12, 4, &ldo_v1},
	{AXP228_ID_DLDO3, 0x17, 0x0, 0x1f, 0x12, 5, &ldo_v1},
	{AXP228_ID_DLDO4, 0x18, 0x0, 0x1f, 0x12, 6, &ldo_v1},
	{AXP228_ID_ELDO1, 0x19, 0x0, 0x1f, 0x12, 0, &ldo_v1},
	{AXP228_ID_ELDO2, 0x1A, 0x0, 0x1f, 0x12, 1, &ldo_v1},
	{AXP228_ID_ELDO3, 0x1B, 0x0, 0x1f, 0x12, 2, &ldo_v1},
	{AXP228_ID_DC5LDO, 0x1C, 0x0, 0x07, 0x10, 0, &ldo_v2},
};

static int axp228_reg_get_value(struct udevice *dev, const struct axp228_para *param)
{
	const struct sec_voltage_desc *desc;
	int ret, uv, val;

	ret = pmic_reg_read(dev->parent, param->vol_addr);
	if (ret < 0)
		return ret;

	desc = param->vol;
	val = (ret >> param->vol_bitpos) & param->vol_bitmask;
	uv = desc->min + val * desc->step;

	return uv;
}

static int axp228_reg_set_value(struct udevice *dev, const struct axp228_para *param,
			 int uv)
{
	const struct sec_voltage_desc *desc;
	int ret, val;

	desc = param->vol;
	if (uv < desc->min || uv > desc->max)
		return -EINVAL;
	val = (uv - desc->min) / desc->step;
	val = (val & param->vol_bitmask) << param->vol_bitpos;
	ret = pmic_clrsetbits(dev->parent, param->vol_addr,
			      param->vol_bitmask << param->vol_bitpos,
			      val);

	return ret;
}

static int axp228_ldo_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_platdata *uc_pdata;

	uc_pdata = dev_get_uclass_platdata(dev);

	uc_pdata->type = REGULATOR_TYPE_LDO;
	uc_pdata->mode_count = 0;

	return 0;
}
static int axp228_ldo_get_value(struct udevice *dev)
{
	int ldo = dev->driver_data;

	return axp228_reg_get_value(dev, &axp228_ldo_param[ldo]);
}

static int axp228_ldo_set_value(struct udevice *dev, int uv)
{
	int ldo = dev->driver_data;

	return axp228_reg_set_value(dev, &axp228_ldo_param[ldo], uv);
}

static int axp228_reg_get_enable(struct udevice *dev, const struct axp228_para *param)
{
	bool enable;
	int ret;

	ret = pmic_reg_read(dev->parent, param->reg_enaddr);
	if (ret < 0)
		return ret;

	enable = (ret >> param->reg_enbitpos) & 0x1;

	return enable;
}

static int axp228_reg_set_enable(struct udevice *dev, const struct axp228_para *param,
			  bool enable)
{
	int ret;

	ret = pmic_reg_read(dev->parent, param->reg_enaddr);
	if (ret < 0)
		return ret;

	ret = pmic_clrsetbits(dev->parent, param->reg_enaddr,
		0x1 << param->reg_enbitpos,
		enable ? 0x1 << param->reg_enbitpos : 0);

	return ret;
}

static bool axp228_ldo_get_enable(struct udevice *dev)
{
	int ldo = dev->driver_data;

	return axp228_reg_get_enable(dev, &axp228_ldo_param[ldo]);
}

static int axp228_ldo_set_enable(struct udevice *dev, bool enable)
{
	int ldo = dev->driver_data;

	return axp228_reg_set_enable(dev, &axp228_ldo_param[ldo], enable);
}

static int axp228_buck_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_platdata *uc_pdata;

	uc_pdata = dev_get_uclass_platdata(dev);

	uc_pdata->type = REGULATOR_TYPE_BUCK;
	uc_pdata->mode_count = 0;

	return 0;
}

static int axp228_buck_get_value(struct udevice *dev)
{
	int buck = dev->driver_data;

	return axp228_reg_get_value(dev, &axp228_buck_param[buck]);
}

static int axp228_buck_set_value(struct udevice *dev, int uv)
{
	int buck = dev->driver_data;

	return axp228_reg_set_value(dev, &axp228_buck_param[buck], uv);
}

static bool axp228_buck_get_enable(struct udevice *dev)
{
	int buck = dev->driver_data;

	return axp228_reg_get_enable(dev, &axp228_buck_param[buck]);
}

static int axp228_buck_set_enable(struct udevice *dev, bool enable)
{
	int buck = dev->driver_data;

	return axp228_reg_set_enable(dev, &axp228_buck_param[buck], enable);
}

static const struct dm_regulator_ops axp228_ldo_ops = {
	.get_value  = axp228_ldo_get_value,
	.set_value  = axp228_ldo_set_value,
	.get_enable = axp228_ldo_get_enable,
	.set_enable = axp228_ldo_set_enable,
};

U_BOOT_DRIVER(axp228_ldo) = {
	.name = AXP228_LDO_DRIVER,
	.id = UCLASS_REGULATOR,
	.ops = &axp228_ldo_ops,
	.probe = axp228_ldo_probe,
};

static const struct dm_regulator_ops axp228_buck_ops = {
	.get_value  = axp228_buck_get_value,
	.set_value  = axp228_buck_set_value,
	.get_enable = axp228_buck_get_enable,
	.set_enable = axp228_buck_set_enable,
};

U_BOOT_DRIVER(axp228_buck) = {
	.name = AXP228_BUCK_DRIVER,
	.id = UCLASS_REGULATOR,
	.ops = &axp228_buck_ops,
	.probe = axp228_buck_probe,
};
