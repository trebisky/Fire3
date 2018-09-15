/*
 * nxe2000.c  --  Regulator driver for the NEXELL NXE2000
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

DECLARE_GLOBAL_DATA_PTR;

static const struct sec_voltage_desc buck_v1 = {
	.max = 3500000,
	.min = 600000,
	.step = 12500,
};

static const struct sec_voltage_desc ldo_v1 = {
	.max = 3500000,
	.min = 900000,
	.step = 25000,
};

static const struct sec_voltage_desc ldo_v2 = {
	.max = 3500000,
	.min = 600000,
	.step = 25000,
};

static const struct sec_voltage_desc ldo_v3 = {
	.max = 3500000,
	.min = 1700000,
	.step = 25000,
};

static const struct sec_voltage_desc ldo_v4 = {
	.max = 3500000,
	.min = 1200000,
	.step = 25000,
};

static const struct nxe2000_para nxe2000_buck_param[] = {
	{NXE2000_ID_DCDC1, 0x36, 0x0, 0xff, 0x2C, 0, &buck_v1},
	{NXE2000_ID_DCDC2, 0x37, 0x0, 0xff, 0x2E, 0, &buck_v1},
	{NXE2000_ID_DCDC3, 0x38, 0x0, 0xff, 0x30, 0, &buck_v1},
	{NXE2000_ID_DCDC4, 0x39, 0x0, 0xff, 0x32, 0, &buck_v1},
	{NXE2000_ID_DCDC5, 0x3A, 0x0, 0xff, 0x34, 0, &buck_v1},
};

static const struct nxe2000_para nxe1500_ldo_param[] = {
	{NXE2000_ID_LDO1, 0x4C, 0x0, 0x7f, 0x44, 0, &ldo_v1},
	{NXE2000_ID_LDO2, 0x4D, 0x0, 0x7f, 0x44, 1, &ldo_v1},
	{NXE2000_ID_LDO3, 0x4E, 0x0, 0x7f, 0x44, 2, &ldo_v2},
	{NXE2000_ID_LDO4, 0x4F, 0x0, 0x7f, 0x44, 3, &ldo_v1},
	{NXE2000_ID_LDO5, 0x50, 0x0, 0x7f, 0x44, 4, &ldo_v1},
	{NXE2000_ID_LDO6, 0x51, 0x0, 0x7f, 0x44, 5, &ldo_v2},
	{NXE2000_ID_LDO7, 0x52, 0x0, 0x7f, 0x44, 6, &ldo_v1},
	{NXE2000_ID_LDO8, 0x53, 0x0, 0x7f, 0x44, 7, &ldo_v2},
	{NXE2000_ID_LDO9, 0x54, 0x0, 0x7f, 0x45, 0, &ldo_v1},
	{NXE2000_ID_LDO10, 0x55, 0x0, 0x7f, 0x45, 1, &ldo_v1},
	{NXE2000_ID_LDORTC1, 0x56, 0x0, 0x7f, 0x45, 4, &ldo_v4},
	{NXE2000_ID_LDORTC2, 0x57, 0x0, 0x7f, 0x45, 5, &ldo_v1},
};

static const struct nxe2000_para nxe2000_ldo_param[] = {
	{NXE2000_ID_LDO1, 0x4C, 0x0, 0x7f, 0x44, 0, &ldo_v1},
	{NXE2000_ID_LDO2, 0x4D, 0x0, 0x7f, 0x44, 1, &ldo_v1},
	{NXE2000_ID_LDO3, 0x4E, 0x0, 0x7f, 0x44, 2, &ldo_v1},
	{NXE2000_ID_LDO4, 0x4F, 0x0, 0x7f, 0x44, 3, &ldo_v1},
	{NXE2000_ID_LDO5, 0x50, 0x0, 0x7f, 0x44, 4, &ldo_v2},
	{NXE2000_ID_LDO6, 0x51, 0x0, 0x7f, 0x44, 5, &ldo_v2},
	{NXE2000_ID_LDO7, 0x52, 0x0, 0x7f, 0x44, 6, &ldo_v1},
	{NXE2000_ID_LDO8, 0x53, 0x0, 0x7f, 0x44, 7, &ldo_v2},
	{NXE2000_ID_LDO9, 0x54, 0x0, 0x7f, 0x45, 0, &ldo_v1},
	{NXE2000_ID_LDO10, 0x55, 0x0, 0x7f, 0x45, 1, &ldo_v1},
	{NXE2000_ID_LDORTC1, 0x56, 0x0, 0x7f, 0x45, 4, &ldo_v3},
	{NXE2000_ID_LDORTC2, 0x57, 0x0, 0x7f, 0x45, 5, &ldo_v1},
};

static int nxe2000_reg_get_value(struct udevice *dev, const struct nxe2000_para *param)
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

static int nxe2000_reg_set_value(struct udevice *dev, const struct nxe2000_para *param,
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

static int nxe2000_ldo_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_platdata *uc_pdata;

	uc_pdata = dev_get_uclass_platdata(dev);

	uc_pdata->type = REGULATOR_TYPE_LDO;
	uc_pdata->mode_count = 0;

	return 0;
}
static int nxe2000_ldo_get_value(struct udevice *dev)
{
	const struct nxe2000_para *ldo_param = NULL;
	int ldo = dev->driver_data;

	switch (dev_get_driver_data(dev_get_parent(dev))) {
	case TYPE_NXE1500:
		ldo_param = &nxe1500_ldo_param[ldo];
		break;
	case TYPE_NXE2000:
		ldo_param = &nxe2000_ldo_param[ldo];
		break;
	default:
		debug("Unsupported NXE2000\n");
		return -EINVAL;
	}

	return nxe2000_reg_get_value(dev, ldo_param);
}

static int nxe2000_ldo_set_value(struct udevice *dev, int uv)
{
	const struct nxe2000_para *ldo_param = NULL;
	int ldo = dev->driver_data;

	switch (dev_get_driver_data(dev_get_parent(dev))) {
	case TYPE_NXE1500:
		ldo_param = &nxe1500_ldo_param[ldo];
		break;
	case TYPE_NXE2000:
		ldo_param = &nxe2000_ldo_param[ldo];
		break;
	default:
		debug("Unsupported NXE2000\n");
		return -EINVAL;
	}

	return nxe2000_reg_set_value(dev, ldo_param, uv);
}

static int nxe2000_reg_get_enable(struct udevice *dev, const struct nxe2000_para *param)
{
	bool enable;
	int ret;

	ret = pmic_reg_read(dev->parent, param->reg_enaddr);
	if (ret < 0)
		return ret;

	enable = (ret >> param->reg_enbitpos) & 0x1;

	return enable;
}

static int nxe2000_reg_set_enable(struct udevice *dev, const struct nxe2000_para *param,
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

static bool nxe2000_ldo_get_enable(struct udevice *dev)
{
	const struct nxe2000_para *ldo_param = NULL;
	int ldo = dev->driver_data;

	switch (dev_get_driver_data(dev_get_parent(dev))) {
	case TYPE_NXE1500:
		ldo_param = &nxe1500_ldo_param[ldo];
		break;
	case TYPE_NXE2000:
		ldo_param = &nxe2000_ldo_param[ldo];
		break;
	default:
		debug("Unsupported NXE2000\n");
		return -EINVAL;
	}

	return nxe2000_reg_get_enable(dev, ldo_param);
}

static int nxe2000_ldo_set_enable(struct udevice *dev, bool enable)
{
	const struct nxe2000_para *ldo_param = NULL;
	int ldo = dev->driver_data;

	switch (dev_get_driver_data(dev_get_parent(dev))) {
	case TYPE_NXE1500:
		ldo_param = &nxe1500_ldo_param[ldo];
		break;
	case TYPE_NXE2000:
		ldo_param = &nxe2000_ldo_param[ldo];
		break;
	default:
		debug("Unsupported NXE2000\n");
		return -EINVAL;
	}

	return nxe2000_reg_set_enable(dev, ldo_param, enable);
}

static int nxe2000_buck_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_platdata *uc_pdata;

	uc_pdata = dev_get_uclass_platdata(dev);

	uc_pdata->type = REGULATOR_TYPE_BUCK;
	uc_pdata->mode_count = 0;

	return 0;
}

static int nxe2000_buck_get_value(struct udevice *dev)
{
	int buck = dev->driver_data;

	return nxe2000_reg_get_value(dev, &nxe2000_buck_param[buck]);
}

static int nxe2000_buck_set_value(struct udevice *dev, int uv)
{
	int buck = dev->driver_data;

	return nxe2000_reg_set_value(dev, &nxe2000_buck_param[buck], uv);
}

static bool nxe2000_buck_get_enable(struct udevice *dev)
{
	int buck = dev->driver_data;

	return nxe2000_reg_get_enable(dev, &nxe2000_buck_param[buck]);
}

static int nxe2000_buck_set_enable(struct udevice *dev, bool enable)
{
	int buck = dev->driver_data;

	return nxe2000_reg_set_enable(dev, &nxe2000_buck_param[buck], enable);
}

static const struct dm_regulator_ops nxe2000_ldo_ops = {
	.get_value  = nxe2000_ldo_get_value,
	.set_value  = nxe2000_ldo_set_value,
	.get_enable = nxe2000_ldo_get_enable,
	.set_enable = nxe2000_ldo_set_enable,
};

U_BOOT_DRIVER(nxe2000_ldo) = {
	.name = NXE2000_LDO_DRIVER,
	.id = UCLASS_REGULATOR,
	.ops = &nxe2000_ldo_ops,
	.probe = nxe2000_ldo_probe,
};

static const struct dm_regulator_ops nxe2000_buck_ops = {
	.get_value  = nxe2000_buck_get_value,
	.set_value  = nxe2000_buck_set_value,
	.get_enable = nxe2000_buck_get_enable,
	.set_enable = nxe2000_buck_set_enable,
};

U_BOOT_DRIVER(nxe2000_buck) = {
	.name = NXE2000_BUCK_DRIVER,
	.id = UCLASS_REGULATOR,
	.ops = &nxe2000_buck_ops,
	.probe = nxe2000_buck_probe,
};
