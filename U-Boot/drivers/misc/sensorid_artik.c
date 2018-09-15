/*
 * Copyright (C) 2016 Samsung Electronics
 * Author: Jaewon Kim <jaewon02.kim@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <linux/time.h>
#include <asm/io.h>
#include <sensorid.h>
#include <sensorid_artik.h>

static int artik_sensor_type(struct udevice *dev, uint16_t *type, int size)
{
	int i, ret;

	for (i = 0; i < size; i++) {
		ret = dm_i2c_read(dev, CMD_SENSOR1 + i, (uint8_t *)&type[i], 2);
		if (ret < 0) {
			printf("Cannot read from device: %p register: %#x %d\n",
					dev, CMD_SENSOR1 + i, ret);
			return ret;
		}
		udelay(1);
	}

	return 0;
}

static int artik_addon_type(struct udevice *dev, uint16_t *type)
{
	int ret;

	ret = dm_i2c_read(dev, CMD_ADDON, (uint8_t *)type, 2);
	if (ret < 0) {
		printf("Cannot read from device: %p register: %#x %d\n",
				dev, CMD_ADDON, ret);
		return ret;
	}

	return 0;
}

static const struct sensorid_ops artik_sensorid_ops = {
	.get_type = artik_sensor_type,
	.get_addon = artik_addon_type,
};

static const struct udevice_id artik_sensorid_ids[] = {
	{ .compatible = "samsung,artik_sensorid" },
	{}
};

U_BOOT_DRIVER(artik_sensorid) = {
	.name		= "artik_sensorid",
	.id		= UCLASS_SENSOR_ID,
	.of_match	= artik_sensorid_ids,
	.ops		= &artik_sensorid_ops,
};
