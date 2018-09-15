/*
 * Copyright (C) 2016 Samsung Electronics
 * Author: Jaewon Kim <jaewon02.kim@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <sensorid.h>

int sensorid_get_type(struct udevice *dev, u16 *type, int size)
{
	const struct sensorid_ops *ops = device_get_ops(dev);

	if (!ops->get_type)
		return -ENOSYS;

	return ops->get_type(dev, type, size);
}

int sensorid_get_addon(struct udevice *dev, u16 *type)
{
	const struct sensorid_ops *ops = device_get_ops(dev);

	if (!ops->get_addon)
		return -ENOSYS;

	return ops->get_addon(dev, type);
}

UCLASS_DRIVER(sensorid) = {
	.id		= UCLASS_SENSOR_ID,
	.name		= "sensorid",
};
