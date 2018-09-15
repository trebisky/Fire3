/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <common.h>
#include <errno.h>
#include <fastboot.h>
#include <malloc.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/compiler.h>
#include <version.h>
#include <g_dnl.h>
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
#include <fb_mmc.h>
#endif
#ifdef CONFIG_FASTBOOT_FLASH_NAND_DEV
#include <fb_nand.h>
#endif
#include <part.h>

#ifdef CONFIG_ENV_IS_NOWHERE
#define saveenv()			0
#endif

#define	FASTBOOT_MMC_MAX		3
#define	FASTBOOT_EEPROM_MAX		1
#define	FASTBOOT_NAND_MAX		1
#define	FASTBOOT_MEM_MAX		1

/* each device max partition max num */
#define	FASTBOOT_DEV_PART_MAX		(16)

/* device types */
#define	FASTBOOT_DEV_EEPROM		(1<<0)	/*  name "eeprom" */
#define	FASTBOOT_DEV_NAND		(1<<1)	/*  name "nand" */
#define	FASTBOOT_DEV_MMC		(1<<2)	/*  name "mmc" */
#define	FASTBOOT_DEV_MEM		(1<<3)	/*  name "mem" */

/* filesystem types */
#define	FASTBOOT_FS_2NDBOOT		(1<<0)	/*  name "boot" <- bootable */
#define	FASTBOOT_FS_BOOT		(1<<1)	/*  name "boot" <- bootable */
#define	FASTBOOT_FS_RAW			(1<<2)	/*  name "raw" */
#define	FASTBOOT_FS_ENV			(1<<3)	/*  name "env" */
#define	FASTBOOT_FS_FAT			(1<<4)	/*  name "fat" */
#define	FASTBOOT_FS_EXT4		(1<<5)	/*  name "ext4" */
#define	FASTBOOT_FS_UBI			(1<<6)	/*  name "ubi" */
#define	FASTBOOT_FS_UBIFS		(1<<7)	/*  name "ubifs" */
#define	FASTBOOT_FS_RAW_PART		(1<<8)	/*  name "emmc" */
#define FASTBOOT_FS_FACTORY		(1<<9)	/*  name "factory" */

#define	FASTBOOT_FS_MASK		(FASTBOOT_FS_EXT4 | FASTBOOT_FS_FAT | \
					 FASTBOOT_FS_UBI | FASTBOOT_FS_UBIFS | \
					 FASTBOOT_FS_RAW_PART)

/* Use 65 instead of 64
 * null gets dropped
 * strcpy's need the extra byte */
#define	RESP_SIZE			(65)

#ifndef CONFIG_FASTBOOT_DIV_SIZE
#define CONFIG_FASTBOOT_DIV_SIZE	0x10000000 /* 256MiB */
#endif

/*
 * f_devices[0,1,2..] : mmc
 *	|		|
 *	|		write_part
 *	|		|
 *	|		link -> fastboot_part -> fastboot_part  -> ...
 *	|						|
 *	|						.partition = bootloader
 *	|						, boot, system,...
 *	|						.start
 *	|						.length
 *	|						.....
 *	|
 */
struct fastboot_device;
struct fastboot_part {
	char partition[32];
	int dev_type;
	int dev_no;
	uint64_t start;
	uint64_t length;
	unsigned int fs_type;
	unsigned int flags;
	struct fastboot_device *fd;
	int part_num;
	struct list_head link;
};
static int part_num_cnt;

struct fastboot_device {
	char *device;
	int dev_max;
	unsigned int dev_type;
	unsigned int part_type;
	unsigned int fs_support;
	/* 0: start, 1: length */
	uint64_t parts[FASTBOOT_DEV_PART_MAX][2];
	struct list_head link;
	int (*write_part)(struct fastboot_part *fpart, void *buf,
			  uint64_t length);
	int (*capacity)(struct fastboot_device *fd, int devno,
			uint64_t *length);
	int (*create_part)(int dev, uint64_t (*parts)[2], int count);
};

struct fastboot_fs_type {
	char *name;
	unsigned int fs_type;
};

/* support fs type */
static struct fastboot_fs_type f_part_fs[] = {
	{ "2nd"			, FASTBOOT_FS_2NDBOOT	},
	{ "boot"		, FASTBOOT_FS_BOOT	},
	{ "env"			, FASTBOOT_FS_ENV	},
	{ "factory"		, FASTBOOT_FS_FACTORY	},
	{ "raw"			, FASTBOOT_FS_RAW	},
	{ "fat"			, FASTBOOT_FS_FAT	},
	{ "ext4"		, FASTBOOT_FS_EXT4	},
	{ "emmc"		, FASTBOOT_FS_RAW_PART	},
	{ "nand"		, FASTBOOT_FS_RAW_PART	},
	{ "ubi"			, FASTBOOT_FS_UBI	},
	{ "ubifs"		, FASTBOOT_FS_UBIFS	},
};

/* Reserved partition names
 *
 * NOTE :
 *	Each command must be ended with ";"
 *
 * partmap :
 *	flash= <device>,<devno> : <partition> : <fs type> : <start>,<length> ;
 *	EX> flash= nand,0:bootloader:boot:0x300000,0x400000;
 *
 * setenv :
 *	<command name> = " command arguments ";
 *	EX> bootcmd = "tftp 42000000 uImage";
 *
 * cmd :
 *	" command arguments ";
 *	EX> "tftp 42000000 uImage";
 *
 */

static const char * const f_reserve_part[] = {
	[0] = "partmap",		/* fastboot partition */
	[1] = "mem",			/* download only */
	[2] = "setenv",			/* u-boot environment setting */
	[3] = "cmd",			/* command run */
};

static unsigned int div_download_bytes;
static char *div_dl_part;
static bool is_div_dl;

/*
 * device partition functions
 */
static int get_parts_from_lists(struct fastboot_part *fpart,
				uint64_t (*parts)[2], int *count);
static void part_dev_print(struct fastboot_device *fd);

#ifdef CONFIG_CMD_MMC
extern ulong mmc_bwrite(int dev_num, lbaint_t start, lbaint_t blkcnt,
			const void *src);

static int mmc_make_parts(int dev, uint64_t (*parts)[2], int count)
{
	char cmd[1024];
	int i = 0, l = 0, p = 0;

	l = sprintf(cmd, "fdisk %d %d:", dev, count);
	p = l;
	for (i = 0; count > i; i++) {
		l = sprintf(&cmd[p], " 0x%llx:0x%llx", parts[i][0],
			    parts[i][1]);
		p += l;
	}

	if (p >= sizeof(cmd)) {
		printf("** %s: cmd stack overflow : stack %zu, cmd %d **\n",
		       __func__, sizeof(cmd), p);
		while (1)
			;
	}

	cmd[p] = 0;
	printf("%s\n", cmd);

/* "fdisk <dev no> [part table counts] <start:length> <start:length> ...\n" */
	return run_command(cmd, 0);
}

static int mmc_check_part_table(block_dev_desc_t *desc,
				struct fastboot_part *fpart)
{
	uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0, 0}, };
	int i = 0, num = 0;
	int ret = 1;

	if (0 > get_part_table(desc, parts, &num))
		return -1;

	for (i = 0; num > i; i++) {
		if (parts[i][0] == fpart->start && parts[i][1] == fpart->length)
			return 0;
		/* when last partition set value is zero,
		   set avaliable length */
		if (((num - 1) == i) && (parts[i][0] == fpart->start) &&
		    (0 == fpart->length)) {
			fpart->length = parts[i][1];
			ret = 0;
			break;
		}
	}
	return ret;
}

static int mmc_part_write(struct fastboot_part *fpart, void *buf,
			  uint64_t length)
{
	block_dev_desc_t *desc;
	struct fastboot_device *fd = fpart->fd;
	int dev = fpart->dev_no;
	lbaint_t blk, cnt;
	int blk_size = 512;
	char cmd[128];
	int ret = 0;
	int l = 0, p = 0;

	sprintf(cmd, "mmc dev %d", dev);

	debug("** mmc.%d partition %s (%s)**\n", dev, fpart->partition,
	      fpart->fs_type&FASTBOOT_FS_EXT4 ? "FS" : "Image");

	/* set mmc devicee */
	if (0 > get_device("mmc", simple_itoa(dev), &desc)) {
		if (0 > run_command(cmd, 0))
			return -1;
		if (0 > run_command("mmc rescan", 0))
			return -1;
	}

	if (0 > run_command(cmd, 0))	/* mmc device */
		return -1;

	if (0 > get_device("mmc", simple_itoa(dev), &desc))
		return -1;

	if (is_div_dl) {
		blk = fpart->start/blk_size;
		cnt = (length/blk_size) + ((length & (blk_size-1)) ? 1 : 0);

		printf("write image to 0x%llx(0x%x), 0x%llx(0x%x)\n",
		       fpart->start, (unsigned int)blk, length,
		       (unsigned int)cnt);

		ret = mmc_bwrite(dev, blk, cnt, buf);

		fpart->start += length;

		if (0 > ret)
			return ret;
		else
			return 0;
	}

	if (fpart->fs_type == FASTBOOT_FS_2NDBOOT ||
	    fpart->fs_type == FASTBOOT_FS_BOOT ||
	    fpart->fs_type == FASTBOOT_FS_ENV) {
		blk = fpart->start/blk_size;
		cnt = (length/blk_size) + ((length & (blk_size-1)) ? 1 : 0);
		p = sprintf(cmd, "mmc write ");
		l = sprintf(&cmd[p], "0x%p 0x%lx 0x%lx", buf,
			    blk, cnt);
		p += l;
		cmd[p] = 0;

		return run_command(cmd, 0);
	} else if (fpart->fs_type & FASTBOOT_FS_MASK) {
		ret = mmc_check_part_table(desc, fpart);
		if (0 > ret)
			return -1;

		if (ret) {	/* new partition */
			uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0, 0}, };
			int num;

			printf("Warn  : [%s] make new partitions ....\n",
			       fpart->partition);
			part_dev_print(fpart->fd);

			get_parts_from_lists(fpart, parts, &num);
			ret = mmc_make_parts(dev, parts, num);
			if (0 > ret) {
				printf("** Fail make partition : %s.%d %s**\n",
				       fd->device, dev, fpart->partition);
				return -1;
			}
		}

		if (mmc_check_part_table(desc, fpart))
			return -1;
	}

	if (fpart->fs_type & FASTBOOT_FS_EXT4) {
		p = sprintf(cmd, "ext4_img_write %d %p %d %x",
			    dev, buf, fpart->part_num, (unsigned int)length);
		ret = run_command(cmd, 0);
		printf("Flash : %s\n", (ret < 0) ? "FAIL" : "DONE");
		return ret;
	}

	blk = fpart->start/blk_size;
	cnt = (length/blk_size) + ((length & (blk_size-1)) ? 1 : 0);

	printf("write image to 0x%llx(0x%x), 0x%llx(0x%x)\n", fpart->start,
	       (unsigned int)blk, length, (unsigned int)cnt);

	ret = mmc_bwrite(dev, blk, cnt, buf);

	if (0 > ret)
		return ret;
	else
		return 0;
}

static int mmc_part_capacity(struct fastboot_device *fd, int devno,
			     uint64_t *length)
{
	block_dev_desc_t *desc;
	char cmd[32];

	debug("** mmc.%d capacity **\n", devno);

	/* set mmc devicee */
	if (0 > get_device("mmc", simple_itoa(devno), &desc)) {
		sprintf(cmd, "mmc dev %d", devno);
	if (0 > run_command(cmd, 0))
		return -1;
	if (0 > run_command("mmc rescan", 0))
		return -1;
	}

	if (0 > get_device("mmc", simple_itoa(devno), &desc))
		return -1;

	*length = (uint64_t)desc->lba * (uint64_t)desc->blksz;

	debug("%u*%u = %llu\n", (uint)desc->lba, (uint)desc->blksz, *length);
	return 0;
}
#endif

static struct fastboot_device f_devices[] = {
	{
		.device		= "mmc",
		.dev_max	= FASTBOOT_MMC_MAX,
		.dev_type	= FASTBOOT_DEV_MMC,
		.part_type	= PART_TYPE_DOS,
		.fs_support	= (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT |
				   FASTBOOT_FS_RAW | FASTBOOT_FS_ENV |
				   FASTBOOT_FS_FAT | FASTBOOT_FS_EXT4 |
				   FASTBOOT_FS_RAW_PART),
	#ifdef CONFIG_CMD_MMC
		.write_part	= mmc_part_write,
		.capacity = mmc_part_capacity,
		.create_part = mmc_make_parts,
	#endif
	},
};

#define	FASTBOOT_DEV_SIZE	ARRAY_SIZE(f_devices)

/*
 *
 * FASTBOOT COMMAND PARSE
 *
 */
static inline void parse_comment(const char *str, const char **ret, int len)
{
	char *p = (char *)str, *r;

	do {
		r = strchr(p, '#');
		if (!r)
			break;
		r++;

		p = strchr(r, '\n');
		if (!p) {
			printf("---- not end comments '#' ----\n");
			break;
		}
		p++;
		len -= (int)(p - str);
		if (len <= 0)
			p = (char *)str;
	} while (len > 1);

	/* for next */
	*ret = p;
}

static inline int parse_string(const char *s, const char *e, char *b, int len)
{
	int l, a = 0;

	do {
		while (0x20 == *s || 0x09 == *s || 0x0a == *s)
			s++;
	} while (0);

	if (0x20 == *(e-1) || 0x09 == *(e-1))
		do {
			e--; while (0x20 == *e || 0x09 == *e) { e--; }; a = 1;
		} while (0);

	l = (e - s + a);
	if (l > len) {
		printf("-- Not enough buffer %d for string len %d [%s] --\n",
		       len, l, s);
		return -1;
	}

	strncpy(b, s, l);
	b[l] = 0;

	return l;
}

static inline void sort_string(char *p, int len)
{
	int i, j;
	for (i = 0, j = 0; len > i; i++) {
		if (0x20 != p[i] && 0x09 != p[i] && 0x0A != p[i])
			p[j++] = p[i];
	}
	p[j] = 0;
}

static int parse_part_device(const char *parts, const char **ret,
			struct fastboot_device **fdev,
			struct fastboot_part *fpart)
{
	struct fastboot_device *fd = *fdev;
	const char *p, *id, *c;
	char str[32];
	int i = 0, id_len;

	p = parts;
	id = p;

	p = strchr(id, ':');
	if (!p) {
		printf("no <dev-id> identifier\n");
		return 1;
	}
	id_len = p - id;

	/* for next */
	p++, *ret = p;

	c = strchr(id, ',');
	parse_string(id, c, str, sizeof(str));

	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		if (strcmp(fd->device, str) == 0) {
			/* add to device */
			list_add_tail(&fpart->link, &fd->link);
			*fdev = fd;

			/* dev no */
			debug("device: %s", fd->device);
			p = strchr(id, ',');
			if (!p) {
				printf("no <dev-no> identifier\n");
				return -1;
			}
			p++;
			parse_string(p, p+id_len, str, sizeof(str));
			/* dev no */
			fpart->dev_no = simple_strtoul(str, NULL, 10);
			if (fpart->dev_no >= fd->dev_max) {
				printf("** Over dev-no max %s.%d : %d **\n",
				       fd->device, fd->dev_max-1,
				       fpart->dev_no);
				return -1;
			}

			debug(".%d\n", fpart->dev_no);
			fpart->fd = fd;
			return 0;
		}
	}

	/* to delete */
	fd = *fdev;
	strcpy(fpart->partition, "unknown");
	list_add_tail(&fpart->link, &fd->link);

	printf("** Can't device parse : %s **\n", parts);
	return -1;
}

static int parse_part_partition(const char *parts, const char **ret,
			struct fastboot_device **fdev,
			struct fastboot_part *fpart)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	const char *p, *id;
	char str[32] = { 0, };
	int i = 0;

	p = parts;
	id = p;

	p = strchr(id, ':');
	if (!p) {
		printf("no <name> identifier\n");
		return -1;
	}

	/* for next */
	p++, *ret = p;
	p--; parse_string(id, p, str, sizeof(str));

	for (i = 0; ARRAY_SIZE(f_reserve_part) > i; i++) {
		const char *r = f_reserve_part[i];
		if (!strcmp(r, str)) {
			printf("** Reserved partition name : %s  **\n", str);
			return -1;
		}
	}

	/* check partition */
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (!strcmp(fp->partition, str)) {
				printf("** Exist partition : %s -> %s **\n",
				       fd->device, fp->partition);
				strcpy(fpart->partition, str);
				fpart->partition[strlen(str)] = 0;
				return -1;
			}
		}
	}

	strcpy(fpart->partition, str);
	fpart->partition[strlen(str)] = 0;
	debug("part  : %s\n", fpart->partition);

	return 0;
}

static int parse_part_fs(const char *parts, const char **ret,
		struct fastboot_device **fdev, struct fastboot_part *fpart)
{
	struct fastboot_device *fd = *fdev;
	struct fastboot_fs_type *fs = f_part_fs;
	const char *p, *id;
	char str[16] = { 0, };
	int i = 0;

	p = parts;
	id = p;

	p = strchr(id, ':');
	if (!p) {
		printf("no <dev-id> identifier\n");
		return -1;
	}

	/* for next */
	p++, *ret = p;
	p--; parse_string(id, p, str, sizeof(str));

	for (; ARRAY_SIZE(f_part_fs) > i; i++, fs++) {
		if (strcmp(fs->name, str) == 0) {
			if (!(fd->fs_support & fs->fs_type)) {
				printf("** '%s' not support '%s' fs **\n",
				       fd->device, fs->name);
				return -1;
			}
			if (fs->fs_type & FASTBOOT_FS_MASK) {
				part_num_cnt++;
				fpart->part_num = part_num_cnt;
			}

			fpart->fs_type = fs->fs_type;
			debug("fs    : %s\n", fs->name);
			return 0;
		}
	}

	printf("** Can't fs parse : %s **\n", str);
	return -1;
}

static int parse_part_address(const char *parts, const char **ret,
			struct fastboot_device **fdev,
			struct fastboot_part *fpart)
{
	const char *p, *id;
	char str[64] = { 0, };
	int id_len;

	p = parts;
	id = p;
	p = strchr(id, ';');
	if (!p) {
		p = strchr(id, '\n');
		if (!p) {
			printf("no <; or '\n'> identifier\n");
			return -1;
		}
	}
	id_len = p - id;

	/* for next */
	p++, *ret = p;

	p = strchr(id, ',');
	if (!p) {
		printf("no <start> identifier\n");
		return -1;
	}

	parse_string(id, p, str, sizeof(str));
	fpart->start = simple_strtoull(str, NULL, 16);
	debug("start : 0x%llx\n", fpart->start);

	p++;
	parse_string(p, p+id_len, str, sizeof(str));	/* dev no*/
	fpart->length = simple_strtoull(str, NULL, 16);
	debug("length: 0x%llx\n", fpart->length);

	return 0;
}

static int parse_part_head(const char *parts, const char **ret)
{
	const char *p = parts;
	int len = strlen("flash=");

	debug("\n");
	p = strstr(p, "flash=");
	if (!p)
		return -1;

	*ret = p + len;
	return 0;
}

typedef int (parse_fnc_t) (const char *parts, const char **ret,
			   struct fastboot_device **fdev,
			   struct fastboot_part *fpart);

parse_fnc_t *parse_part_seqs[] = {
	parse_part_device,
	parse_part_partition,
	parse_part_fs,
	parse_part_address,
	0,	/* end */
};

static inline void part_lists_init(int init)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i = 0;

	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (head->next == NULL)
			INIT_LIST_HEAD(head);

		if (init) {
			if (!list_empty(head)) {
				debug("delete [%s]:", fd->device);
				list_for_each_safe(entry, n, head) {
					fp = list_entry(entry, struct fastboot_part, link);
					debug("%s ", fp->partition);
					list_del(entry);
					free(fp);
				}
			}
			debug("\n");

			INIT_LIST_HEAD(head);
			memset(fd->parts, 0x0, sizeof(FASTBOOT_DEV_PART_MAX*2));
			continue;
		}

		if (list_empty(head))
			continue;

		debug("delete [%s]:", fd->device);
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			debug("%s ", fp->partition);
			list_del(entry);
			free(fp);
		}
		debug("\n");
		INIT_LIST_HEAD(head);
	}
}

static int part_lists_make(const char *ptable_str, int ptable_str_len)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	parse_fnc_t **p_fnc_ptr;
	const char *p = ptable_str;
	int len = ptable_str_len;
	int err = -1;
	part_num_cnt = 0;

	debug("\n---part_lists_make ---\n");
	part_lists_init(0);

	parse_comment(p, &p, len);
	len -= (int)(p - ptable_str);
	sort_string((char *)p, len);

	/* new parts table */
	while (1) {
		if (parse_part_head(p, &p)) {
			if (err)
				printf("-- unknown parts head: [%s]\n", p);
			break;
		}

		fp = malloc(sizeof(*fp));
		if (!fp) {
			printf("* Can't malloc fastboot part table entry *\n");
			err = -1;
			break;
		}

		for (p_fnc_ptr = parse_part_seqs; *p_fnc_ptr; ++p_fnc_ptr) {
			if ((*p_fnc_ptr)(p, &p, &fd, fp) != 0) {
				err = -1;
				free(fp);
				goto fail_parse;
			}
		}
		err = 0;
	}

fail_parse:
	if (err)
		part_lists_init(0);

	return err;
}

static void part_lists_print(void)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i;

	printf("\nFastboot Partitions:\n");
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			printf(" %s.%d: %s, %s : 0x%llx, 0x%llx\n",
			       fd->device, fp->dev_no, fp->partition,
			       FASTBOOT_FS_MASK&fp->fs_type ? "fs" : "img",
			       fp->start, fp->length);
		}
	}

	printf("Support fstype :");
	for (i = 0; ARRAY_SIZE(f_part_fs) > i; i++)
		printf(" %s ", f_part_fs[i].name);
	printf("\n");

	printf("Reserved part  :");
	for (i = 0; ARRAY_SIZE(f_reserve_part) > i; i++)
		printf(" %s ", f_reserve_part[i]);
	printf("\n");
}

static int part_lists_check(const char *part)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i = 0;

	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (!strcmp(fp->partition, part))
				return 0;
		}
	}
	return -1;
}

static void part_dev_print(struct fastboot_device *fd)
{
	struct fastboot_part *fp;
	struct list_head *entry, *n;

	printf("Device: %s\n", fd->device);
	struct list_head *head = &fd->link;
	if (!list_empty(head)) {
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			printf(" %s.%d: %s, %s : 0x%llx, 0x%llx\n",
			       fd->device, fp->dev_no, fp->partition,
			       FASTBOOT_FS_MASK&fp->fs_type ? "fs" : "img",
			       fp->start, fp->length);
		}
	}
}

/* fastboot getvar capacity.<device>.<dev no> */
static int part_dev_capacity(const char *device, uint64_t *length)
{
	struct fastboot_device *fd = f_devices;
	const char *s = device, *c = device;
	char str[32] = {0,};
	uint64_t len = 0;
	int no = 0, i = 0;

	c = strchr(s, '.');
	if (c) {
		strncpy(str, s, (c-s));
		str[c-s] = 0;
		c += 1;
		no = simple_strtoul(c, NULL, 10);
		for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
			if (strcmp(fd->device, str))
				continue;
			if (fd->capacity)
				fd->capacity(fd, no, &len);
			break;
		}
	}

	*length = len;

	return !len ? -1 : 0;
}

static void part_mbr_update(void)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i = 0, j = 0;

	debug("%s:\n", __func__);
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		uint64_t part_dev[FASTBOOT_DEV_PART_MAX][3] = { {0, 0, 0}, };
		int count = 0, dev = 0;
		int total = 0;

		if (list_empty(head))
			continue;

		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (FASTBOOT_FS_MASK & fp->fs_type) {
				part_dev[total][0] = fp->start;
				part_dev[total][1] = fp->length;
				part_dev[total][2] = fp->dev_no;
				debug("%s.%d 0x%llx, 0x%llx\n",
				      fd->device, fp->dev_no,
				      part_dev[total][0], part_dev[total][1]);
				total++;
			}
		}
		debug("total parts : %d\n", total);

		count = total;
		while (count > 0) {
			uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0, 0 }, };
			int mbrs = 0;

			if (dev > fd->dev_max) {
				printf("** Fail make to %s dev %d",
				       fd->device, dev);
				printf("is over max %d **\n", fd->dev_max);
				break;
			}

			for (j = 0; total > j; j++) {
				if (dev == (int)part_dev[j][2]) {
					parts[mbrs][0] = part_dev[j][0];
					parts[mbrs][1] = part_dev[j][1];
					debug("MBR %s.%d 0x%llx,",
					      fd->device, dev, parts[mbrs][0]);
					debug("0x%llx (%d:%d)\n",
					      parts[mbrs][1], total, count);
					mbrs++;
				}
			}

			/* new MBR */
			if (mbrs && fd->create_part)
				fd->create_part(dev, parts, mbrs);

			count -= mbrs;
			debug("count %d, mbrs %d, dev %d\n", count, mbrs, dev);
			if (count)
				dev++;
		}
	}
}

static int get_parts_from_lists(struct fastboot_part *fpart,
				uint64_t (*parts)[2], int *count)
{
	struct fastboot_part *fp = fpart;
	struct fastboot_device *fd = fpart->fd;
	struct list_head *head = &fd->link;
	struct list_head *entry, *n;
	int dev = fpart->dev_no;
	int i = 0;

	if (!parts || !count) {
		printf("-- No partition input params --\n");
		return -1;
	}
	*count = 0;

	if (list_empty(head))
		return 0;

	list_for_each_safe(entry, n, head) {
		fp = list_entry(entry, struct fastboot_part, link);
		if ((FASTBOOT_FS_MASK & fp->fs_type) &&
		    (dev == fp->dev_no)) {
			parts[i][0] = fp->start;
			parts[i][1] = fp->length;
			i++;
			debug("%s.%d = %s\n", fd->device, dev, fp->partition);
		}
	}

	*count = i;	/* set part count */
	return 0;
}

static int parse_env_head(const char *env, const char **ret, char *str, int len)
{
	const char *p = env, *r = p;

	parse_comment(p, &p, len);
	r = strchr(p, '=');
	if (!r)
		return -1;

	if (0 > parse_string(p, r, str, len))
		return -1;

	r = strchr(r, '"');
	if (!r) {
		printf("no <\"> identifier\n");
		return -1;
	}

	r++; *ret = r;
	return 0;
}

static int parse_env_end(const char *env, const char **ret, char *str, int len)
{
	const char *p = env;
	const char *r = p;

	r = strchr(p, '"');
	if (!r) {
		printf("no <\"> end identifier\n");
		return -1;
	}

	if (0 > parse_string(p, r, str, len))
		return -1;

	r++;
	r = strchr(p, ';');
	if (!r) {
		r = strchr(p, '\n');
		if (!r) {
			printf("no <;> exit identifier\n");
			return -1;
		}
	}

	/* for next */
	r++, *ret = r;
	return 0;
}

static int parse_cmd(const char *cmd, const char **ret, char *str, int len)
{
	const char *p = cmd, *r = p;

	parse_comment(p, &p, len);
	p = strchr(p, '"');
	if (!p)
		return -1;
	p++;

	r = strchr(p, '"');
	if (!r)
		return -1;

	if (0 > parse_string(p, r, str, len))
		return -1;
	r++;

	r = strchr(p, ';');
	if (!r) {
		r = strchr(p, '\n');
		if (!r) {
			printf("no <;> exit identifier\n");
			return -1;
		}
	}

	/* for next */
	r++, *ret = r;
	return 0;
}

static int fb_setenv(const char *str, int str_len)
{
	const char *p = str;
	int len = str_len;
	char cmd[128];
	char arg[1024];
	int err = -1;

	debug("---fb_setenv---\n");
	do {
		if (parse_env_head(p, &p, cmd, sizeof(cmd)))
			break;

		if (parse_env_end(p, &p, arg, sizeof(arg)))
			break;

		printf("%s=%s\n", cmd, arg);
		setenv(cmd, (char *)arg);
		saveenv();
		err = 0;
		len -= (int)(p - str);
	} while (len > 1);

	return err;
}

static int fb_command(const char *str, int str_len)
{
	const char *p = str;
	int len = str_len;
	char cmd[128];
	int err = -1;

	debug("---fb_command---\n");
	do {
		if (parse_cmd(p, &p, cmd, sizeof(cmd)))
			break;

		printf("Run [%s]\n", cmd);
		err = run_command(cmd, 0);
		if (0 > err)
			break;
		len -= (int)(p - str);
	} while (len > 1);

	return err;
}

static void fb_flash_write_based_partmap(const char *cmd,
					 void *download_buffer,
					 unsigned int download_bytes,
					 char *response)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i = 0;

	/* flash to partition */
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;

		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (!strcmp(fp->partition, cmd)) {
				if ((download_bytes > fp->length) &&
				    (fp->length != 0)) {
					debug("[down: %u, length: %lld]\n",
					      download_bytes, fp->length);
					sprintf(response,
						"FAIL image too large");
					fastboot_fail(response,
						      "cannot find partition");
				}

				if ((fd->dev_type != FASTBOOT_DEV_MEM) &&
				    fd->write_part) {
					if (0 > fd->write_part(fp,
							       download_buffer,
							       download_bytes))
						sprintf(response,
							"FAIL to flash");
				}

				fastboot_okay(response, "");
			}
		}
	}
}

#define FASTBOOT_VERSION		"0.4"

#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)

#define EP_BUFFER_SIZE			4096

struct f_fastboot {
	struct usb_function usb_function;

	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
};

static inline struct f_fastboot *func_to_fastboot(struct usb_function *f)
{
	return container_of(f, struct f_fastboot, usb_function);
}

static struct f_fastboot *fastboot_func;
static unsigned int fastboot_flash_session_id;
static unsigned int download_size;
static unsigned int download_bytes;
static bool is_high_speed;

static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= TX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1,
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1,
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor hs_ep_in = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= TX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0,
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0,
	.bInterval		= 0x00,
};

static struct usb_interface_descriptor interface_desc = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints		= 0x02,
	.bInterfaceClass	= FASTBOOT_INTERFACE_CLASS,
	.bInterfaceSubClass	= FASTBOOT_INTERFACE_SUB_CLASS,
	.bInterfaceProtocol	= FASTBOOT_INTERFACE_PROTOCOL,
};

static struct usb_descriptor_header *fb_runtime_descs[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&hs_ep_in,
	(struct usb_descriptor_header *)&hs_ep_out,
	NULL,
};

/*
 * static strings, in UTF-8
 */
static const char fastboot_name[] = "Android Fastboot";

static struct usb_string fastboot_string_defs[] = {
	[0].s = fastboot_name,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_fastboot = {
	.language	= 0x0409,	/* en-us */
	.strings	= fastboot_string_defs,
};

static struct usb_gadget_strings *fastboot_strings[] = {
	&stringtab_fastboot,
	NULL,
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req);
static int strcmp_l1(const char *s1, const char *s2);


void fastboot_fail(char *response, const char *reason)
{
	strncpy(response, "FAIL\0", 5);
	strncat(response, reason, FASTBOOT_RESPONSE_LEN - 4 - 1);
}

void fastboot_okay(char *response, const char *reason)
{
	strncpy(response, "OKAY\0", 5);
	strncat(response, reason, FASTBOOT_RESPONSE_LEN - 4 - 1);
}

static void fastboot_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;
	if (!status)
		return;
	printf("status: %d ep '%s' trans: %d\n", status, ep->name, req->actual);
}

static int fastboot_bind(struct usb_configuration *c, struct usb_function *f)
{
	int id;
	struct usb_gadget *gadget = c->cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);
	const char *s;

	/* DYNAMIC interface numbers assignments */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	interface_desc.bInterfaceNumber = id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	fastboot_string_defs[0].id = id;
	interface_desc.iInterface = id;

	f_fb->in_ep = usb_ep_autoconfig(gadget, &fs_ep_in);
	if (!f_fb->in_ep)
		return -ENODEV;
	f_fb->in_ep->driver_data = c->cdev;

	hs_ep_in.bEndpointAddress = fs_ep_in.bEndpointAddress;

	f_fb->out_ep = usb_ep_autoconfig(gadget, &fs_ep_out);
	if (!f_fb->out_ep)
		return -ENODEV;
	f_fb->out_ep->driver_data = c->cdev;

	hs_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;

	s = getenv("serial#");
	if (s)
		g_dnl_set_serialnumber((char *)s);

	return 0;
}

static void fastboot_unbind(struct usb_configuration *c, struct usb_function *f)
{
	memset(fastboot_func, 0, sizeof(*fastboot_func));
}

static void fastboot_disable(struct usb_function *f)
{
	struct f_fastboot *f_fb = func_to_fastboot(f);

	usb_ep_disable(f_fb->out_ep);
	usb_ep_disable(f_fb->in_ep);

	if (f_fb->out_req) {
		free(f_fb->out_req->buf);
		usb_ep_free_request(f_fb->out_ep, f_fb->out_req);
		f_fb->out_req = NULL;
	}
	if (f_fb->in_req) {
		free(f_fb->in_req->buf);
		usb_ep_free_request(f_fb->in_ep, f_fb->in_req);
		f_fb->in_req = NULL;
	}
}

static struct usb_request *fastboot_start_ep(struct usb_ep *ep)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, 0);
	if (!req)
		return NULL;

	req->length = EP_BUFFER_SIZE;
	req->buf = memalign(CONFIG_SYS_CACHELINE_SIZE, EP_BUFFER_SIZE);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	memset(req->buf, 0, req->length);
	return req;
}

static int fastboot_set_alt(struct usb_function *f,
			    unsigned interface, unsigned alt)
{
	int ret;
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	debug("%s: func: %s intf: %d alt: %d\n",
	      __func__, f->name, interface, alt);

	/* make sure we don't enable the ep twice */
	if (gadget->speed == USB_SPEED_HIGH) {
		ret = usb_ep_enable(f_fb->out_ep, &hs_ep_out);
		is_high_speed = true;
	} else {
		ret = usb_ep_enable(f_fb->out_ep, &fs_ep_out);
		is_high_speed = false;
	}
	if (ret) {
		puts("failed to enable out ep\n");
		return ret;
	}

	f_fb->out_req = fastboot_start_ep(f_fb->out_ep);
	if (!f_fb->out_req) {
		puts("failed to alloc out req\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->out_req->complete = rx_handler_command;

	/* make sure we don't enable the ep twice */
	if (gadget->speed == USB_SPEED_HIGH) {
		ret = usb_ep_enable(f_fb->in_ep, &hs_ep_in);
		is_high_speed = true;
	} else {
		ret = usb_ep_enable(f_fb->in_ep, &fs_ep_in);
		is_high_speed = false;
	}
	if (ret) {
		puts("failed to enable in ep\n");
		goto err;
	}

	f_fb->in_req = fastboot_start_ep(f_fb->in_ep);
	if (!f_fb->in_req) {
		puts("failed alloc req in\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->in_req->complete = fastboot_complete;

	ret = usb_ep_queue(f_fb->out_ep, f_fb->out_req, 0);
	if (ret)
		goto err;

	return 0;
err:
	fastboot_disable(f);
	return ret;
}

static int fastboot_add(struct usb_configuration *c)
{
	struct f_fastboot *f_fb = fastboot_func;
	int status;

	debug("%s: cdev: 0x%p\n", __func__, c->cdev);

	if (!f_fb) {
		f_fb = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*f_fb));
		if (!f_fb)
			return -ENOMEM;

		fastboot_func = f_fb;
		memset(f_fb, 0, sizeof(*f_fb));
	}

	f_fb->usb_function.name = "f_fastboot";
	f_fb->usb_function.hs_descriptors = fb_runtime_descs;
	f_fb->usb_function.bind = fastboot_bind;
	f_fb->usb_function.unbind = fastboot_unbind;
	f_fb->usb_function.set_alt = fastboot_set_alt;
	f_fb->usb_function.disable = fastboot_disable;
	f_fb->usb_function.strings = fastboot_strings;

	status = usb_add_function(c, &f_fb->usb_function);
	if (status) {
		free(f_fb);
		fastboot_func = f_fb;
	}

	return status;
}
DECLARE_GADGET_BIND_CALLBACK(usb_dnl_fastboot, fastboot_add);

static int fastboot_tx_write(const char *buffer, unsigned int buffer_size)
{
	struct usb_request *in_req = fastboot_func->in_req;
	int ret;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;

	usb_ep_dequeue(fastboot_func->in_ep, in_req);

	ret = usb_ep_queue(fastboot_func->in_ep, in_req, 0);
	if (ret)
		printf("Error %d on queue\n", ret);
	return 0;
}

static int fastboot_tx_write_str(const char *buffer)
{
	return fastboot_tx_write(buffer, strlen(buffer));
}

static void compl_do_reset(struct usb_ep *ep, struct usb_request *req)
{
	do_reset(NULL, 0, 0, NULL);
}

int __weak fb_set_reboot_flag(void)
{
	return -ENOSYS;
}

static void cb_reboot(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	if (!strcmp_l1("reboot-bootloader", cmd)) {
		if (fb_set_reboot_flag()) {
			fastboot_tx_write_str("FAILCannot set reboot flag");
			return;
		}
	}
	fastboot_func->in_req->complete = compl_do_reset;
	fastboot_tx_write_str("OKAY");
}

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

static void cb_getvar(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[FASTBOOT_RESPONSE_LEN];
	const char *s;
	size_t chars_left;

	strcpy(response, "OKAY");
	chars_left = sizeof(response) - strlen(response) - 1;

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing variable\n");
		fastboot_tx_write_str("FAILmissing var");
		return;
	}

	if (!strcmp_l1("version", cmd)) {
		strncat(response, FASTBOOT_VERSION, chars_left);
	} else if (!strcmp_l1("bootloader-version", cmd)) {
		strncat(response, U_BOOT_VERSION, chars_left);
	} else if (!strcmp_l1("downloadsize", cmd) ||
		!strcmp_l1("max-download-size", cmd)) {
		char str_num[12];

		sprintf(str_num, "0x%08x", CONFIG_FASTBOOT_BUF_SIZE);
		strncat(response, str_num, chars_left);

		/*
		 * This also indicates the start of a new flashing
		 * "session", in which we could have 1-N buffers to
		 * write to a partition.
		 *
		 * Reset our session counter.
		 */
		fastboot_flash_session_id = 0;
	} else if (!strcmp_l1("serialno", cmd)) {
		s = getenv("serial#");
		if (s)
			strncat(response, s, chars_left);
		else
			strcpy(response, "FAILValue not set");
	} else if (!strcmp_l1("partition-type:", cmd)) {
		char *s = cmd, *m;
		int err;

		s += strlen("partition-type:");
		m = getenv("partmap");
		if (m) {
			part_lists_init(1);
			err = part_lists_make(m, strlen(m));
			if (0 > err)
				printf("ERR part_lists_make\n");
			if (part_lists_check(s))
				strcpy(response, "FAILBad partition");
			printf("\nReady : ");
		}
		strncat(response, s, chars_left);
	} else if (!strcmp_l1("product", cmd)) {
		strncat(response, CONFIG_SYS_BOARD, chars_left);
	} else if (!strcmp_l1("capacity", cmd)) {
		char *s = cmd;
		uint64_t length = 0;

		s += strlen("capacity");
		s += 1;

		if (0 > part_dev_capacity(s, &length)) {
			strcpy(response, "FAILBad device");
		} else {
			sprintf(s, "%lld", length);
			strncat(response, s, chars_left);
		}
	} else if (!strcmp_l1("partmap", cmd)) {
		int err;

		s = getenv("partmap");
		if (s) {
			strncat(response, s, chars_left);
			part_lists_init(1);
			err = part_lists_make(s, strlen(s));
			if (0 > err)
				printf("ERR part_lists_make\n");
			part_lists_print();
		} else {
			printf("FAIL Please set partmap setting\n");
			strcpy(response, "FAILPlease set partmap setting");
		}
	} else {
		error("unknown variable: %s\n", cmd);
		strcpy(response, "FAILVariable not implemented");
	}
	fastboot_tx_write_str(response);
}

static unsigned int rx_bytes_expected(unsigned int maxpacket)
{
	int rx_remain = download_size - download_bytes;
	int rem = 0;
	if (rx_remain < 0)
		return 0;
	if (rx_remain > EP_BUFFER_SIZE)
		return EP_BUFFER_SIZE;
	if (rx_remain < maxpacket) {
		rx_remain = maxpacket;
	} else if (rx_remain % maxpacket != 0) {
		rem = rx_remain % maxpacket;
		rx_remain = rx_remain - rem;
	}
	return rx_remain;
}

#define BYTES_PER_DOT	0x20000
static void rx_handler_dl_image(struct usb_ep *ep, struct usb_request *req)
{
	char response[FASTBOOT_RESPONSE_LEN];
	unsigned int transfer_size = download_size - download_bytes;
	const unsigned char *buffer = req->buf;
	unsigned int buffer_size = req->actual;
	unsigned int pre_dot_num, now_dot_num;
	unsigned int max;

	if (req->status != 0) {
		printf("Bad status: %d\n", req->status);
		return;
	}

	if (buffer_size < transfer_size)
		transfer_size = buffer_size;

	if (!is_div_dl) {
		memcpy((void *)CONFIG_FASTBOOT_BUF_ADDR + download_bytes,
		       buffer, transfer_size);
		pre_dot_num = download_bytes / BYTES_PER_DOT;
		download_bytes += transfer_size;
		now_dot_num = download_bytes / BYTES_PER_DOT;
	} else {
		memcpy((void *)CONFIG_FASTBOOT_BUF_ADDR + div_download_bytes,
		       buffer, transfer_size);
		pre_dot_num = div_download_bytes / BYTES_PER_DOT;
		download_bytes += transfer_size;
		div_download_bytes += transfer_size;
		now_dot_num = div_download_bytes / BYTES_PER_DOT;
	}

	if (pre_dot_num != now_dot_num) {
		putc('.');
		if (!(now_dot_num % 74))
			putc('\n');
	}

	/* Check if transfer is done */
	if (download_bytes >= download_size) {
		/*
		 * Reset global transfer variable, keep download_bytes because
		 * it will be used in the next possible flashing command
		 */
		download_size = 0;
		req->complete = rx_handler_command;
		req->length = EP_BUFFER_SIZE;

		sprintf(response, "OKAY");
		fastboot_tx_write_str(response);

		printf("\ndownloading of %d bytes finished\n", download_bytes);
	} else {
		max = is_high_speed ? hs_ep_out.wMaxPacketSize :
				fs_ep_out.wMaxPacketSize;
		req->length = rx_bytes_expected(max);
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
		if (is_div_dl &&
		    (div_download_bytes >= CONFIG_FASTBOOT_DIV_SIZE)) {
			fb_flash_write_based_partmap(div_dl_part,
						     (void *)
						     CONFIG_FASTBOOT_BUF_ADDR,
						     div_download_bytes,
						     response);
			div_download_bytes = 0;
		}
	}

	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}

static void cb_download(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[FASTBOOT_RESPONSE_LEN];
	unsigned int max;

	strsep(&cmd, ":");
	download_size = simple_strtoul(cmd, NULL, 16);
	download_bytes = 0;
	div_download_bytes = 0;
	is_div_dl = 0;

	printf("Starting download of %d bytes\n", download_size);

	if (0 == download_size) {
		sprintf(response, "FAILdata invalid size");
	} else if (download_size > CONFIG_FASTBOOT_BUF_SIZE) {
		div_dl_part = getenv("partition");
		if (div_dl_part) {
			is_div_dl = 1;
			sprintf(response, "DATA%08x", download_size);
			req->complete = rx_handler_dl_image;
			max = is_high_speed ? hs_ep_out.wMaxPacketSize :
				fs_ep_out.wMaxPacketSize;
			req->length = rx_bytes_expected(max);
			if (req->length < ep->maxpacket)
				req->length = ep->maxpacket;

		} else {
			download_size = 0;
			sprintf(response, "FAILdata too large");
		}
	} else {
		sprintf(response, "DATA%08x", download_size);
		req->complete = rx_handler_dl_image;
		max = is_high_speed ? hs_ep_out.wMaxPacketSize :
			fs_ep_out.wMaxPacketSize;
		req->length = rx_bytes_expected(max);
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
	}
	fastboot_tx_write_str(response);
}

static void do_bootm_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	char boot_addr_start[12];
	char *bootm_args[] = { "bootm", boot_addr_start, NULL };

	puts("Booting kernel..\n");

	sprintf(boot_addr_start, "0x%lx", load_addr);
	do_bootm(NULL, 0, 2, bootm_args);

	/* This only happens if image is somehow faulty so we start over */
	do_reset(NULL, 0, 0, NULL);
}

static void cb_boot(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_func->in_req->complete = do_bootm_on_complete;
	fastboot_tx_write_str("OKAY");
}

static void do_exit_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	g_dnl_trigger_detach();
}

static void cb_continue(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_func->in_req->complete = do_exit_on_complete;
	fastboot_tx_write_str("OKAY");
}

#ifdef CONFIG_FASTBOOT_FLASH
static void cb_flash(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[FASTBOOT_RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name\n");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	/* new partition map */
	if (!strcmp("partmap", cmd)) {
		const char *p_tmp = (void *)CONFIG_FASTBOOT_BUF_ADDR;
		part_lists_init(1);
		char p[512];
		strncpy(p, p_tmp, download_bytes);
		if (0 > part_lists_make(p, download_bytes)) {
			sprintf(response, "FAIL partition map parse");
			goto err_flash;
		}
		part_lists_print();
		part_mbr_update();
		if (0 == setenv("partmap", p) && 0 == saveenv()) {
			fastboot_okay(response, "");
			goto done_flash;
		}

	/* set environments */
	} else if (!strcmp("setenv", cmd)) {
		char *p = (void *)CONFIG_FASTBOOT_BUF_ADDR;
		if (0 > fb_setenv(p, download_bytes)) {
			sprintf(response, "FAIL environment parse");
			goto err_flash;
		}
		fastboot_okay(response, "");
		goto done_flash;

	/* run command */
	} else if (!strcmp("cmd", cmd)) {
		char *p = (void *)CONFIG_FASTBOOT_BUF_ADDR;
		if (0 > fb_command(p, download_bytes)) {
			sprintf(response, "FAIL cmd parse");
			goto err_flash;
		}
		fastboot_okay(response, "");
		goto done_flash;

	/* memory partition : do nothing */
	} else if (0 == strcmp("mem", cmd)) {
		fastboot_okay(response, "");
		goto done_flash;
	}

	if (is_div_dl) {
		fb_flash_write_based_partmap(div_dl_part,
					     (void *)CONFIG_FASTBOOT_BUF_ADDR,
					     div_download_bytes, response);
		fastboot_okay(response, "");
		goto done_flash;
	}

	const char *s;
	s = getenv("partmap");
	if (s) {
		part_lists_init(1);
		int err = part_lists_make(s, strlen(s));
		if (0 > err)
			printf("ERR part_lists_make\n");

		fb_flash_write_based_partmap(cmd,
					     (void *)CONFIG_FASTBOOT_BUF_ADDR,
					     download_bytes, response);
		goto done_flash;
	}
	strcpy(response, "FAILno flash device defined");
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	fb_mmc_flash_write(cmd, fastboot_flash_session_id,
			   (void *)CONFIG_FASTBOOT_BUF_ADDR,
			   download_bytes, response);
#endif
#ifdef CONFIG_FASTBOOT_FLASH_NAND_DEV
	fb_nand_flash_write(cmd, fastboot_flash_session_id,
			    (void *)CONFIG_FASTBOOT_BUF_ADDR,
			    download_bytes, response);
#endif

err_flash:
	sprintf(response, "FAIL partition does not exist");

done_flash:
	fastboot_flash_session_id++;
	fastboot_tx_write_str(response);

	part_lists_init(0);
}
#endif

static void cb_oem(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	if (strncmp("format", cmd + 4, 6) == 0) {
		char cmdbuf[32];
                sprintf(cmdbuf, "gpt write mmc %x $partitions",
			CONFIG_FASTBOOT_FLASH_MMC_DEV);
                if (run_command(cmdbuf, 0))
			fastboot_tx_write_str("FAIL");
                else
			fastboot_tx_write_str("OKAY");
	} else
#endif
	if (strncmp("unlock", cmd + 4, 8) == 0) {
		fastboot_tx_write_str("FAILnot implemented");
	}
	else {
		fastboot_tx_write_str("FAILunknown oem command");
	}
}

#ifdef CONFIG_FASTBOOT_FLASH
static void cb_erase(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[FASTBOOT_RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	strcpy(response, "FAILno flash device defined");

#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	fb_mmc_erase(cmd, response);
#endif
#ifdef CONFIG_FASTBOOT_FLASH_NAND_DEV
	fb_nand_erase(cmd, response);
#endif
	fastboot_tx_write_str(response);
}
#endif

struct cmd_dispatch_info {
	char *cmd;
	void (*cb)(struct usb_ep *ep, struct usb_request *req);
};

static const struct cmd_dispatch_info cmd_dispatch_info[] = {
	{
		.cmd = "reboot",
		.cb = cb_reboot,
	}, {
		.cmd = "getvar:",
		.cb = cb_getvar,
	}, {
		.cmd = "download:",
		.cb = cb_download,
	}, {
		.cmd = "boot",
		.cb = cb_boot,
	}, {
		.cmd = "continue",
		.cb = cb_continue,
	},
#ifdef CONFIG_FASTBOOT_FLASH
	{
		.cmd = "flash",
		.cb = cb_flash,
	}, {
		.cmd = "erase",
		.cb = cb_erase,
	},
#endif
	{
		.cmd = "oem",
		.cb = cb_oem,
	},
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	char *cmdbuf = req->buf;
	void (*func_cb)(struct usb_ep *ep, struct usb_request *req) = NULL;
	int i;

	if (req->status != 0 || req->length == 0)
		return;

	for (i = 0; i < ARRAY_SIZE(cmd_dispatch_info); i++) {
		if (!strcmp_l1(cmd_dispatch_info[i].cmd, cmdbuf)) {
			func_cb = cmd_dispatch_info[i].cb;
			break;
		}
	}

	if (!func_cb) {
		error("unknown command: %s\n", cmdbuf);
		fastboot_tx_write_str("FAILunknown command");
	} else {
		if (req->actual < req->length) {
			u8 *buf = (u8 *)req->buf;
			buf[req->actual] = 0;
			func_cb(ep, req);
		} else {
			error("buffer overflow\n");
			fastboot_tx_write_str("FAILbuffer overflow");
		}
	}

	*cmdbuf = '\0';
	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}
