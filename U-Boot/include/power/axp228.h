/*
 * axp228.h  --  PMIC driver for the X-Powers AXP228
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongshin, Park <pjsin865@nexell.co.kr>
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */

#ifndef __AXP228_H_
#define __AXP228_H_

/* AXP228 registers */
#define	AXP228_NUM_OF_REGS 0xEF

/* Unified sub device IDs for AXP */
/* LDO0 For RTCLDO ,LDO1-3 for ALDO,LDO*/
enum axp228_regnum {
	AXP228_ID_DCDC1 = 0,
	AXP228_ID_DCDC2,
	AXP228_ID_DCDC3,
	AXP228_ID_DCDC4,
	AXP228_ID_DCDC5,
	AXP228_ID_ALDO1,
	AXP228_ID_ALDO2,
	AXP228_ID_ALDO3,
	AXP228_ID_DLDO1,
	AXP228_ID_DLDO2,
	AXP228_ID_DLDO3,
	AXP228_ID_DLDO4,
	AXP228_ID_ELDO1,
	AXP228_ID_ELDO2,
	AXP228_ID_ELDO3,
	AXP228_ID_DC5LDO,
};

struct sec_voltage_desc {
	int max;
	int min;
	int step;
};

/**
 * struct axp228_para - axp228 register parameters
 * @param vol_addr	i2c address of the given buck/ldo register
 * @param vol_bitpos	bit position to be set or clear within register
 * @param vol_bitmask	bit mask value
 * @param reg_enaddr	control register address, which enable the given
 *			given buck/ldo.
 * @param reg_enbiton	value to be written to buck/ldo to make it ON
 * @param vol		Voltage information
 */
struct axp228_para {
	enum axp228_regnum regnum;
	u8	vol_addr;
	u8	vol_bitpos;
	u8	vol_bitmask;
	u8	reg_enaddr;
	u8	reg_enbitpos;
	const struct sec_voltage_desc *vol;
};

/* Drivers name */
#define AXP228_LDO_DRIVER	"axp228_ldo"
#define AXP228_BUCK_DRIVER	"axp228_buck"

#endif /* __AXP228_H_ */
