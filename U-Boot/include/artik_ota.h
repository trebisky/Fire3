/*
 * Copyright (c) 2016 Samsung Electronics
 * Author: Jaewon Kim <jaewon02.kim@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ARTIK_OTA_H
#define __ARTIK_OTA_H

#define FLAG_PART_BLOCK_START	0x2000
#define HEADER_MAGIC		"ARTIK_OTA"
#define CMD_LEN			128
#define MMC_DEV_NUM		0
#define SD_DEV_NUM		1

enum boot_part {
	PART0		= 0,
	PART1,
};

enum boot_state {
	BOOT_FAILED	= 0,
	BOOT_SUCCESS,
	BOOT_UPDATED,
};

struct part_info {
	enum boot_state state;
	char retry;
	char tag[32];
};

struct boot_info {
	char header_magic[32];
	enum boot_part part_num;
	enum boot_state state;
	struct part_info part0;
	struct part_info part1;
};

int check_ota_update(void);
#endif
