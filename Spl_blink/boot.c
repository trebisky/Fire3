/*
 *	Copyright (c) 2017 Metro94
 *
 *	File:        boot.c
 *	Description: boot code for S5P6818
 *	Author:      Metro94 <flattiles@gmail.com>
 *
 * This blinks the on board LED on GPIO B12
 *  It works for the NanoPi M3 and Fire3 boards
 */

#include <common.h>
#include <io.h>

void boot_master(void)
{
	int i, d = 0;

	clrsetbits32(0xc001b020, 3 << 24, 2 << 24);
	setbits32(0xc001b004, 1 << 12);

	while (1) {
		for (i = 0; i < 200000; ++i)
			d ^= i;
		tglbits32(0xc001b000, 1 << 12);
	}
}

void boot_slave(void)
{
}
