/*
 * Copyright (c) 2017 Samsung Electronics
 * Author: Jaewon Kim <jaewon02.kim@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ARTIK_MAC_H
#define __ARTIK_MAC_H

#define SET_YEAR_FIELD(x)	(((x) & 0x3) << 22)
#define SET_MONTH_FIELD(x)	(((x) & 0xF) << 18)
#define SET_DAY_FIELD(x)	(((x) & 0x1F) << 13)
#define SET_MODEL_FIELD(x)	(((x) & 0x3) << 11)
#define SET_SERIAL_FILED(x)	(((x) & 0x7FF))

#define MAC_ADDR_0(x)		(((x) >> 0) & 0xFF)
#define MAC_ADDR_1(x)		(((x) >> 8) & 0xFF)
#define MAC_ADDR_2(x)		(((x) >> 16) & 0xFF)

struct serial_t {
	char model[3];
	char sub[1];
	char assy[1];
	char year[1];
	char month[1];
	char day[1];
	char hw_version[3];
	char security[1];
	char serial[4];
};

struct artik_info_t {
	const char *board_name;
	const char security;
	const u32 year_offset;
	const u32 model_code;
	const u32 oui;
};

void generate_mac(void);
#endif
