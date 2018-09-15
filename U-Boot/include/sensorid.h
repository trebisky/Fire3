/*
 * Copyright (c) 2016 Samsung Electronics
 * Author: Jaewon Kim <jaewon02.kim@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __SENSORID_H
#define __SENSORID_H
struct sensorid_ops {
	int (*get_type)(struct udevice *dev, uint16_t *type, int size);
	int (*get_addon)(struct udevice *dev, uint16_t *type);
};

int sensorid_get_type(struct udevice *dev, uint16_t *type, int size);
int sensorid_get_addon(struct udevice *dev, uint16_t *type);
#endif
