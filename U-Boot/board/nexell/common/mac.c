/*
 * Copyright (c) 2017 Samsung Electronics
 * Author: Jaewon Kim <jaewon02.kim@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <linux/ctype.h>
#include <artik_mac.h>

#define ARTIK_A710		"710"
#define ARTIK_A530		"530"
#define ARTIK_A533		"533"
#define ARTIK_A305		"305"
#define CMD_MAX			256
#define GET_YEAR_FROM_SN(x)	(((x) - 'B') + 2016)

static const struct artik_info_t artik_info[] = {
	{
		/* Artik710 */
		.board_name	= ARTIK_A710,
		.year_offset	= 2016,
		.security	= '0',
		.model_code	= 0,
		.oui		= 0x08152F,
	}, {
		/* Artik710s */
		.board_name	= ARTIK_A710,
		.year_offset	= 2016,
		.security	= 'L',
		.model_code	= 1,
		.oui		= 0x08152F,
	}, {
		/* Artik530 */
		.board_name	= ARTIK_A530,
		.year_offset	= 2016,
		.security	= '0',
		.model_code	= 2,
		.oui		= 0x08152F,
	}, {
		/* Artik530s */
		.board_name	= ARTIK_A530,
		.year_offset	= 2017,
		.security	= 'L',
		.model_code	= 3,
		.oui		= 0x08152F,
	}, {
		/* Artik305 */
		.board_name	= ARTIK_A305,
		.year_offset	= 2017,
		.security	= '0',
		.model_code	= 1,
		.oui		= 0x448F17,
	}, {
		/* Artik533 */
		.board_name	= ARTIK_A533,
		.year_offset	= 2017,
		.security	= '0',
		.model_code	= 0,
		.oui		= 0x448F17,
	},
	{ },
};

static const struct artik_info_t *get_board_info(
		const int year, const char *name, const char *security)
{
	const struct artik_info_t *info = &artik_info[0];
	int year_field;

	while (info->board_name != NULL) {
		year_field = year - info->year_offset;
		if (year_field < 0 || year_field > 4) {
			info++;
			continue;
		}

		if (strncmp(info->board_name, name, 3) == 0 &&
				strncmp(&info->security, security, 1) == 0)
			return info;
		info++;
	}

	return NULL;
}

static int convert_number(const char *cp, size_t len)
{
	int value, result = 0;

	while (len-- > 0) {
		if (isdigit(*cp)) {
			value = *cp - '0';
		} else if (isupper(*cp)) {
			value = *cp - 'A' + 10;
		} else {
			printf("Artik MAC : Unsupported Type(%c)\n", *cp);
			return -EINVAL;
		}

		result = (result * 10) + value;
		cp++;
	}

	return result;
}

void generate_mac(void)
{
	const struct artik_info_t *info;
	struct serial_t *serial;
	char *ethaddr, *artik_serial;
	char env_var[CMD_MAX] = {0, };
	int val;
	u32 mac = 0;

	/* Check ethernet mac address */
	ethaddr = getenv("ethaddr");
	if (ethaddr != NULL)
		return;

	/* Check artik serial number.
	 * If there is no serial number, generate random mac address.
	 */
	artik_serial = getenv("serial#");
	if (artik_serial == NULL)
		return;
	serial = (struct serial_t *)artik_serial;

	/* Get board type */
	info = get_board_info(GET_YEAR_FROM_SN(serial->year[0]),
			serial->model, serial->security);
	if (info == NULL)
		return;
	mac = SET_MODEL_FIELD(info->model_code);

	/* Convert Year */
	mac |= SET_YEAR_FIELD(
			GET_YEAR_FROM_SN(serial->year[0]) - info->year_offset);

	/* Convert Month */
	val = convert_number(serial->month, 1);
	if (val < 0)
		return;
	mac |= SET_MONTH_FIELD(val);

	/* Convert Day */
	val = convert_number(serial->day, 1);
	if (val < 0)
		return;
	mac |= SET_DAY_FIELD(val);

	/* Convert Serial */
	val = convert_number(serial->serial, 4);
	if (val < 0)
		return;
	mac |= SET_SERIAL_FILED(val);

	/* Set MAC Address */
	snprintf(env_var, CMD_MAX, "%x:%x:%x:%x:%x:%x",
			MAC_ADDR_2(info->oui), MAC_ADDR_1(info->oui),
			MAC_ADDR_0(info->oui), MAC_ADDR_2(mac),
			MAC_ADDR_1(mac), MAC_ADDR_0(mac));
	setenv("ethaddr", env_var);
}
