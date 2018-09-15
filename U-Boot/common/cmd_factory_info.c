/*
 * Copyright (C) 2016 Chanho Park <chanho61.park@samsung.com>
 *
 * Factory Information support
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <linux/list.h>
#include <mmc.h>

#define FACTORY_INFO_HEADER_MAGIC	0x46414354
#define FACTORY_INFO_HEADER_MAJOR	0x0001
#define FACTORY_INFO_HEADER_MINOR	0x0000

struct fi_header {
	unsigned int magic;
	unsigned short major;
	unsigned short minor;
	unsigned int total_count;	/* A count of entities */
	unsigned int total_bytes;	/* Last position of buffer */
};

struct fi_entity_header {
	unsigned int name_len;
	unsigned int val_len;
};

struct fi_entity {
	struct list_head link;
	struct fi_entity_header e_hdr;
	char *name;
	char *val;
};

static struct list_head entity_list;
static struct fi_header f_hdr;
static int info_loaded;

enum fi_state {
	LOAD,
	SAVE
};

static int fi_mmc_ops(int state, int dev, u32 blk, u32 cnt, void *addr)
{
	u32 n;
	struct mmc *mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("no mmc device at slot %x\n", dev);
		return -1;
	}

	mmc_init(mmc);

	if (state == LOAD)
		n = mmc->block_dev.block_read(dev, blk, cnt, addr);
	else
		n = mmc->block_dev.block_write(dev, blk, cnt, addr);

	return (n == cnt) ? 0 : -1;
}

static void factory_header_init(struct fi_header *hdr)
{
	hdr->magic = FACTORY_INFO_HEADER_MAGIC;
	hdr->major = FACTORY_INFO_HEADER_MAJOR;
	hdr->minor = FACTORY_INFO_HEADER_MINOR;
	hdr->total_count = 0;
	hdr->total_bytes = sizeof(struct fi_header);
}

static int check_header(struct fi_header *hdr)
{
	if (hdr->magic != FACTORY_INFO_HEADER_MAGIC)
		return -1;

	if (hdr->major != FACTORY_INFO_HEADER_MAJOR ||
	    hdr->minor != FACTORY_INFO_HEADER_MINOR)
		return -1;

	return 0;
}

static inline char *alloc_string(const char *buf, int len)
{
	char *new_str = malloc(len + 1);
	memcpy(new_str, buf, len);
	new_str[len] = 0;

	return new_str;
}

static struct fi_entity *alloc_new_entity(const char *name, int name_len,
		const char *val, int val_len)
{
	struct fi_entity *entity = malloc(sizeof(struct fi_entity));
	entity->e_hdr.name_len = name_len;
	entity->e_hdr.val_len = val_len;
	entity->name = alloc_string(name, name_len);
	entity->val = alloc_string(val, val_len);
	return entity;
}

static struct fi_entity *find_entity(const char *entity_name)
{
	struct fi_entity *pos;

	list_for_each_entry(pos, &entity_list, link) {
		if (!strcmp(entity_name, pos->name))
			return pos;
	}

	return NULL;
}

/* Parse entities from memory buffer */
static int fi_deserialize(char *buf)
{
	struct fi_entity *entity;
	struct fi_entity_header hdr;
	int i, offset = 0;

	memcpy(&f_hdr, buf, sizeof(struct fi_header));

	if (check_header(&f_hdr))
		factory_header_init(&f_hdr);

	offset += sizeof(struct fi_header);

	for (i = 0; i < f_hdr.total_count; i++) {
		memcpy(&hdr, buf + offset, sizeof(struct fi_entity_header));
		offset += sizeof(struct fi_entity_header);

		entity = alloc_new_entity(buf + offset, hdr.name_len,
				buf + offset + hdr.name_len, hdr.val_len);
		list_add_tail(&entity->link, &entity_list);

		offset += hdr.name_len + hdr.val_len;

		setenv(entity->name, entity->val);
	}

	f_hdr.total_bytes = offset;

	info_loaded = 1;

	return 0;
}

/* Store entities into memory buffer */
static void fi_serialize(char *buf)
{
	int offset = 0;
	struct fi_entity *pos;

	offset = sizeof(struct fi_header);

	list_for_each_entry(pos, &entity_list, link) {
		memcpy(buf + offset, &pos->e_hdr,
		       sizeof(struct fi_entity_header));
		offset += sizeof(struct fi_entity_header);
		memcpy(buf + offset, pos->name, pos->e_hdr.name_len);
		offset += pos->e_hdr.name_len;
		memcpy(buf + offset, pos->val, pos->e_hdr.val_len);
		offset += pos->e_hdr.val_len;
	}

	f_hdr.total_bytes = offset;
	memcpy(buf, &f_hdr, sizeof(struct fi_header));
}

static void factory_info_destroy(void)
{
	struct fi_entity *pos, *tmp;

	list_for_each_entry_safe_reverse(pos, tmp, &entity_list, link) {
		list_del(&pos->link);
		free(pos->name);
		free(pos->val);
		free(pos);
	}

	factory_header_init(&f_hdr);
}

static int factory_info_load(const char *interface, const char *device,
		const char *offset, const char *count)
{
	int err;

	if (info_loaded)
		factory_info_destroy();

	INIT_LIST_HEAD(&entity_list);

	if (!strncmp(interface, "mmc", 3)) {
		int dev = simple_strtoul(device, NULL, 10);
		u32 blk = simple_strtoul(offset, NULL, 16);
		u32 cnt = simple_strtoul(count, NULL, 16);
		err = fi_mmc_ops(LOAD, dev, blk, cnt,
				(void *)CONFIG_FACTORY_INFO_BUF_ADDR);
		if (err)
			return err;
	}

	return fi_deserialize((char *)CONFIG_FACTORY_INFO_BUF_ADDR);
}

static int factory_info_save(const char *interface, const char *device,
		const char *offset, const char *count)
{
	int err;

	if (!info_loaded) {
		printf("Please load the information at first\n");
		return -1;
	}

	fi_serialize((char *)CONFIG_FACTORY_INFO_BUF_ADDR);

	if (!strncmp(interface, "mmc", 3)) {
		int dev = simple_strtoul(device, NULL, 10);
		u32 blk = simple_strtoul(offset, NULL, 16);
		u32 cnt = simple_strtoul(count, NULL, 16);
		err = fi_mmc_ops(SAVE, dev, blk, cnt,
				(void *)CONFIG_FACTORY_INFO_BUF_ADDR);
		if (err)
			return err;
	}

	return 0;
}

static int factory_info_read_entity(const char *entity_name)
{
	struct fi_entity *entity;

	if (!info_loaded) {
		printf("Please load the information at first\n");
		return -1;
	}

	entity = find_entity(entity_name);
	if (entity) {
		printf("%s\n", entity->val);
		return 0;
	}

	return -1;
}

static void update_entity_value(struct fi_entity *entity, const char *value)
{
	free(entity->val);
	entity->e_hdr.val_len = strlen(value);
	entity->val = alloc_string(value, entity->e_hdr.val_len);
}

static int factory_info_write_entity(const char *entity_name,
		const char *value)
{
	struct fi_entity *entity;

	if (!info_loaded) {
		printf("Please load the information at first\n");
		return -1;
	}

	entity = find_entity(entity_name);
	if (entity) {
		update_entity_value(entity, value);
	} else {
		entity = alloc_new_entity(entity_name, strlen(entity_name),
				value, strlen(value));
		list_add_tail(&entity->link, &entity_list);
		f_hdr.total_count++;
		f_hdr.total_bytes += sizeof(struct fi_entity_header) +
			entity->e_hdr.name_len + entity->e_hdr.val_len;
	}

	return 0;
}

static int factory_info_list(void)
{
	struct fi_entity *pos;

	if (!info_loaded) {
		printf("Please load the information at first\n");
		return -1;
	}

	list_for_each_entry(pos, &entity_list, link)
		printf("%s %s\n", pos->name, pos->val);

	return 0;
}

static int factory_info_clean(void)
{
	if (!info_loaded) {
		printf("Please load the information at first\n");
		return -1;
	}

	factory_info_destroy();

	return 0;
}

int do_factory_info(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	switch (argc) {
	case 6:
		if (!strncmp(argv[1], "load", 4))
			factory_info_load(argv[2], argv[3], argv[4],
					  argv[5]);
		else if (!strncmp(argv[1], "save", 4))
			factory_info_save(argv[2], argv[3], argv[4],
					  argv[5]);
		break;
	case 4:
		if (!strncmp(argv[1], "write", 5))
			factory_info_write_entity(argv[2], argv[3]);
		break;
	case 3:
		if (!strncmp(argv[1], "read", 4))
			factory_info_read_entity(argv[2]);
		break;
	case 2:
		if (!strncmp(argv[1], "list", 4))
			factory_info_list();
		else if (!strncmp(argv[1], "clean", 5))
			factory_info_clean();
		break;
	case 1:
	default:
		return CMD_RET_USAGE;
	}

	return 0;
}

U_BOOT_CMD(factory_info, 6, 0, do_factory_info,
	   "Factory Information commands",
	   "list - List factory information\n"
	   "factory_info load <interface> <device> <offset> <cnt>\n"
	   "        - Load factory information from the interface\n"
	   "factory_info save <interface> <device> <offset> <cnt>\n"
	   "        - Save factory information to the interface\n"
	   "factory_info read <entity name> - Read a value of entity name\n"
	   "factory_info write <entity name> <val> - Write a value of entity name\n"
	   "factory_info clean - Clean factioy information\n"
);
