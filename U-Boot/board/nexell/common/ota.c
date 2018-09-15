#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <mapmem.h>
#include <errno.h>
#include <artik_ota.h>

#ifndef CONFIG_FLAG_INFO_ADDR
#define CONFIG_FLAG_INFO_ADDR	0x49000000
#endif

static void write_flag_partition(void)
{
	char cmd[CMD_LEN] = {0, };

	sprintf(cmd, "mmc write 0x%X 0x%X %X",
			CONFIG_FLAG_INFO_ADDR, FLAG_PART_BLOCK_START, 1);
	run_command(cmd, 0);
}

static struct boot_info *read_flag_partition(void)
{
	char cmd[CMD_LEN] = {0, };

	sprintf(cmd, "mmc read 0x%X 0x%X %X",
			CONFIG_FLAG_INFO_ADDR, FLAG_PART_BLOCK_START, 1);
	run_command(cmd, 0);

	return (struct boot_info *)map_sysmem(
			CONFIG_FLAG_INFO_ADDR, sizeof(struct boot_info));
}

static inline void update_partition_env(enum boot_part part_num)
{
	if (part_num == PART0) {
		setenv("bootpart", __stringify(CONFIG_BOOT_PART));
		setenv("modulespart", __stringify(CONFIG_MODULES_PART));
	} else {
		setenv("bootpart", __stringify(CONFIG_BOOT1_PART));
		setenv("modulespart", __stringify(CONFIG_MODULES1_PART));
	}
}

int check_ota_update(void)
{
	struct boot_info *boot;
	struct part_info *cur_part, *bak_part;
	char cmd[CMD_LEN] = {0, };
	char *bootcmd, *rootdev;

	setenv("ota", "ota");
	/* Do not check flag partition on recoveryboot mode */
	bootcmd = getenv("bootcmd");
	if (strstr(bootcmd, "recoveryboot") != NULL) {
		return 0;
	}

	/* Check booted device */
	rootdev = getenv("rootdev");
	if (strncmp(rootdev, "0", 1) == 0) {
		sprintf(cmd, "mmc dev %d", MMC_DEV_NUM);
	} else if (strncmp(rootdev, "1", 1) == 0) {
		sprintf(cmd, "mmc dev %d", SD_DEV_NUM);
	} else {
		printf("Cannot find rootdev\n");
		return -1;
	}
	run_command(cmd, 0);

	boot = read_flag_partition();
	if (strncmp(boot->header_magic, HEADER_MAGIC, 32) != 0) {
		printf("Cannot read FLAG information\n");
		return -1;
	}

	if (boot->part_num == PART0) {
		cur_part = &boot->part0;
		bak_part = &boot->part1;
	} else {
		cur_part = &boot->part1;
		bak_part = &boot->part0;
	}

	switch (boot->state) {
	case BOOT_SUCCESS:
		printf("Booting State = Normal(%d)\n", boot->part_num);
		update_partition_env(boot->part_num);
		setenv("rescue", "0");
		break;
	case BOOT_UPDATED:
		if (cur_part->retry > 0) {
			printf("%s Partition Updated\n",
				boot->part_num == PART0 ? "PART0" : "PART1");
			cur_part->retry--;
			setenv("bootdelay", "0");
			setenv("rescue", "0");
		/* Handling Booting Fail */
		} else if (cur_part->retry == 0) {
			printf("Boot failed from %s\n",
				boot->part_num == PART0 ? "PART0" : "PART1");
			cur_part->state = BOOT_FAILED;
			if (bak_part->state == BOOT_SUCCESS) {
				if (boot->part_num == PART0)
					boot->part_num = PART1;
				else
					boot->part_num = PART0;
				boot->state = BOOT_SUCCESS;
				setenv("rescue", "1");
			} else {
				boot->state = BOOT_FAILED;
			}
		}

		update_partition_env(boot->part_num);
		write_flag_partition();
		break;
	case BOOT_FAILED:
	default:
		printf("Booting State = Abnormal\n");
		update_partition_env(PART0);
		return -1;
	}

	return 0;
}
