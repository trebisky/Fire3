/*
 * nxe2000.h  --  PMIC driver for the NEXELL NXE2000
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongshin, Park <pjsin865@nexell.co.kr>
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */

#ifndef __NXE2000_H_
#define __NXE2000_H_

/* NXE2000 registers */
#define	NXE2000_NUM_OF_REGS 0xFF

#define	NXE2000_REG_PWRONTIMSET				0x10
#define	NXE2000_REG_DC1CTL2					0x2D
#define	NXE2000_REG_DC2CTL2					0x2F
#define	NXE2000_REG_DC3CTL2					0x31
#define	NXE2000_REG_DC4CTL2					0x33
#define	NXE2000_REG_DC5CTL2					0x35
#define	NXE2000_REG_LDOEN1					0x44
#define	NXE2000_REG_CHGCTL1					0xB3
#define	NXE2000_REG_BANKSEL					0xFF

/* PWRONTIMSET */
#define	NXE2000_POS_PWRONTIMSET_DIS_OFF_PWRON_TIM	7
#define	NXE2000_POS_PWRONTIMSET_OFF_PRESS_PWRON		4
#define	NXE2000_POS_PWRONTIMSET_OFF_JUDGE_PWRON		3
#define	NXE2000_POS_PWRONTIMSET_ON_PRESS_PWRON		0

#define	NXE2000_POS_DCxCTL2_DCxOSC			6
#define	NXE2000_POS_DCxCTL2_DCxSR			4
#define	NXE2000_POS_DCxCTL2_DCxLIM			1
#define	NXE2000_POS_DCxCTL2_DCxLIMSDEN		0
#define	NXE2000_POS_CHGCTL1_SUSPEND			3

#define	NXE2000_POS_LDOEN1_LDO5EN			4
#define	NXE2000_POS_LDOEN1_LDO4EN			3
#define	NXE2000_POS_LDOEN1_LDO3EN			2
#define	NXE2000_POS_LDOEN1_LDO2EN			1
#define	NXE2000_POS_LDOEN1_LDO1EN			0

/* Unified sub device IDs for AXP */
/* LDO0 For RTCLDO ,LDO1-3 for ALDO,LDO*/
enum nxe2000_regnum {
	NXE2000_ID_DCDC1 = 0,
	NXE2000_ID_DCDC2,
	NXE2000_ID_DCDC3,
	NXE2000_ID_DCDC4,
	NXE2000_ID_DCDC5,
	NXE2000_ID_LDO1,
	NXE2000_ID_LDO2,
	NXE2000_ID_LDO3,
	NXE2000_ID_LDO4,
	NXE2000_ID_LDO5,
	NXE2000_ID_LDO6,
	NXE2000_ID_LDO7,
	NXE2000_ID_LDO8,
	NXE2000_ID_LDO9,
	NXE2000_ID_LDO10,
	NXE2000_ID_LDORTC1,
	NXE2000_ID_LDORTC2,
};

enum {
	TYPE_UNKNOWN = 0,
	TYPE_NXE1500,
	TYPE_NXE2000,
	TYPE_MAX,
};

struct sec_voltage_desc {
	int max;
	int min;
	int step;
};

/**
 * struct nxe2000_para - nxe2000 register parameters
 * @param vol_addr	i2c address of the given buck/ldo register
 * @param vol_bitpos	bit position to be set or clear within register
 * @param vol_bitmask	bit mask value
 * @param reg_enaddr	control register address, which enable the given
 *			given buck/ldo.
 * @param reg_enbiton	value to be written to buck/ldo to make it ON
 * @param vol		Voltage information
 */
struct nxe2000_para {
	enum nxe2000_regnum regnum;
	u8	vol_addr;
	u8	vol_bitpos;
	u8	vol_bitmask;
	u8	reg_enaddr;
	u8	reg_enbitpos;
	const struct sec_voltage_desc *vol;
};

/* Drivers name */
#define NXE2000_LDO_DRIVER	"nxe2000_ldo"
#define NXE2000_BUCK_DRIVER	"nxe2000_buck"

#endif /* __NXE2000_H_ */
