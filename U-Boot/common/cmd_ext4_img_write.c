/*
 * (C) Copyright 2016 Nexell
 * Youngbok, Park <ybpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <mmc.h>
#include <asm/sections.h>
#include <part.h>

DECLARE_GLOBAL_DATA_PTR;

#define MMC_BLOCK_SIZE		(512)
#define SECTOR_BITS		9	/* 512B */

struct ext4_file_header {
	unsigned int magic;
	unsigned short major;
	unsigned short minor;
	unsigned short file_header_size;
	unsigned short chunk_header_size;
	unsigned int block_size;
	unsigned int total_blocks;
	unsigned int total_chunks;
	unsigned int crc32;
};

struct ext4_chunk_header {
	unsigned short type;
	unsigned short reserved;
	unsigned int chunk_size;
	unsigned int total_size;
};

#define EXT4_FILE_HEADER_MAGIC	0xED26FF3A
#define EXT4_FILE_HEADER_MAJOR	0x0001
#define EXT4_FILE_HEADER_MINOR	0x0000
#define EXT4_FILE_BLOCK_SIZE	0x1000

#define EXT4_FILE_HEADER_SIZE	(sizeof(struct ext4_file_header))
#define EXT4_CHUNK_HEADER_SIZE	(sizeof(struct ext4_chunk_header))


#define EXT4_CHUNK_TYPE_RAW		0xCAC1
#define EXT4_CHUNK_TYPE_FILL		0xCAC2
#define EXT4_CHUNK_TYPE_NONE		0xCAC3

#define ALIGN_BUFFER_SIZE		0x4000000
#define WRITE_SECTOR 65536					/* 32 MB */

typedef int (*WRITE_RAW_CHUNK_CB)(char *data, unsigned int sector,
				  unsigned int sector_size);

int set_write_raw_chunk_cb(WRITE_RAW_CHUNK_CB cb);
extern ulong mmc_bwrite(int dev_num, lbaint_t start, lbaint_t blkcnt,
			const void *src);

static int write_raw_chunk(char *data, unsigned int sector,
			   unsigned int sector_size);
static WRITE_RAW_CHUNK_CB write_raw_chunk_cb = write_raw_chunk;

int set_write_raw_chunk_cb(WRITE_RAW_CHUNK_CB cb)
{
	write_raw_chunk_cb = cb;

	return 0;
}

int check_compress_ext4(char *img_base, unsigned long long parti_size)
{
	struct ext4_file_header *file_header;

	file_header = (struct ext4_file_header *)img_base;

	if (file_header->magic != EXT4_FILE_HEADER_MAGIC)
		return -1;

	if (file_header->major != EXT4_FILE_HEADER_MAJOR) {
		printf("Invalid Version Info! 0x%2x\n", file_header->major);
		return -1;
	}

	if (file_header->file_header_size != EXT4_FILE_HEADER_SIZE) {
		printf("Invalid File Header Size! 0x%8x\n",
		       file_header->file_header_size);
		return -1;
	}

	if (file_header->chunk_header_size != EXT4_CHUNK_HEADER_SIZE) {
		printf("Invalid Chunk Header Size! 0x%8x\n",
		       file_header->chunk_header_size);
		return -1;
	}

	if (file_header->block_size != EXT4_FILE_BLOCK_SIZE) {
		printf("Invalid Block Size! 0x%8x\n", file_header->block_size);
		return -1;
	}

	if ((parti_size/file_header->block_size)  < file_header->total_blocks) {
		printf("Invalid Volume Size!");
		printf("Image is bigger than partition size!\n");
		printf("partion size %lld ", parti_size);
		printf(" image size %lld\n",
		       (uint64_t)file_header->total_blocks *
		       file_header->block_size);
		printf("Hang...\n");
		while (1)
			;
	}

	/* image is compressed ext4 */
	return 0;
}

int write_raw_chunk(char *data, unsigned int sector, unsigned int sector_size)
{
	char run_cmd[64];
	unsigned char *tmp_align;

	if (((unsigned long)data % 8) != 0) {
		int offset = 0;

		/* align buffer start = malloc_start_address - ALIGN_BUFFER_SIZE */
		tmp_align = (unsigned char *)(gd->relocaddr - TOTAL_MALLOC_LEN);
		tmp_align -= ALIGN_BUFFER_SIZE;

		do {
			if (sector_size > WRITE_SECTOR) {
				memcpy((unsigned char *)tmp_align,
				       (unsigned char *)data+offset,
				       WRITE_SECTOR*512);

				sprintf(run_cmd, "mmc write 0x%x 0x%x 0x%x",
					(int)((ulong)tmp_align),
					sector, WRITE_SECTOR);

				sector_size -= WRITE_SECTOR;
				sector += WRITE_SECTOR;
			} else {
				memcpy((unsigned char *)tmp_align,
				       (unsigned char *)data+offset,
				       sector_size * 512);
				sprintf(run_cmd, "mmc write 0x%x 0x%x 0x%x",
					(int)((ulong)tmp_align),
					sector, sector_size);

				sector_size -= sector_size;
			}
			run_command(run_cmd, 0);

		} while (sector_size > 0);

		return 0;
	}
	debug("write raw data in %d size %d\n", sector, sector_size);
	sprintf(run_cmd, "mmc write 0x%x 0x%x 0x%x", (int)((ulong)data),
		sector, sector_size);
	run_command(run_cmd, 0);

	return 0;
}

int write_compressed_ext4(char *img_base, unsigned int sector_base)
{
	unsigned int sector_size;
	int total_chunks;
	struct ext4_chunk_header *chunk_header;
	struct ext4_file_header *file_header;

	file_header = (struct ext4_file_header *)img_base;
	total_chunks = file_header->total_chunks;

	debug("total chunk = %d\n", total_chunks);

	img_base += EXT4_FILE_HEADER_SIZE;

	while (total_chunks) {
		chunk_header = (struct ext4_chunk_header *)img_base;
		sector_size =
			(chunk_header->chunk_size * file_header->block_size)
			>> SECTOR_BITS;

		switch (chunk_header->type) {
		case EXT4_CHUNK_TYPE_RAW:
			debug("raw_chunk\n");
			write_raw_chunk_cb(img_base + EXT4_CHUNK_HEADER_SIZE,
					   sector_base, sector_size);
			sector_base += sector_size;
			break;

		case EXT4_CHUNK_TYPE_FILL:
			printf("*** fill_chunk ***\n");
			sector_base += sector_size;
			break;

		case EXT4_CHUNK_TYPE_NONE:
			debug("none chunk\n");
			sector_base += sector_size;
			break;

		default:
			printf("*** unknown chunk type ***\n");
			sector_base += sector_size;
			break;
		}
		total_chunks--;
		debug("remain chunks = %d\n", total_chunks);

		img_base += chunk_header->total_size;
	};

	debug("write done\n");
	return 0;
}

int do_compressed_ext4_write(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	block_dev_desc_t *desc;
	uint64_t dst_addr = 0, mem_len = 0;
	unsigned int mem_addr = 0;
	unsigned char *p;
	char cmd[32];
	lbaint_t blk, cnt;
	int ret, dev;

	if (5 > argc)
		goto usage;

	ret = get_device("mmc", argv[1], &desc);
	if (0 > ret) {
		printf("** Not find device mmc.%s **\n", argv[1]);
		return 1;
	}
	dev = simple_strtoul(argv[1], NULL, 10);
	sprintf(cmd, "mmc dev %d", dev);
	if (0 > run_command(cmd, 0))	/* mmc device */
		return -1;

	mem_addr = simple_strtoul(argv[2], NULL, 16);
	dst_addr = simple_strtoull(argv[3], NULL, 16);
	mem_len  = simple_strtoull(argv[4], NULL, 16);

	p   = (unsigned char *)((ulong)mem_addr);
	blk = (dst_addr/MMC_BLOCK_SIZE);
	cnt = (mem_len/MMC_BLOCK_SIZE) +
		((mem_len & (MMC_BLOCK_SIZE-1)) ? 1 : 0);

	flush_dcache_all();

	uint64_t parts[8][2] = { {0, 0}, };
	uint64_t part_len = 0;
	int partno = (int)dst_addr;
	int num = 0;

	if (0 > get_part_table(desc, parts, &num))
			return 1;

	if (partno > num || 1 > partno)  {
		printf("Invalid mmc.%d partition number %d (1 ~ %d)\n",
		       dev, partno, num);
		return 1;
	}

	dst_addr = parts[partno-1][0];
	part_len = parts[partno-1][1];
	blk = (dst_addr/MMC_BLOCK_SIZE);
	if (0 == check_compress_ext4((char *)p, part_len)) {
		printf("0x%llx(%d) ~ 0x%llx(%d):\n", dst_addr,
		       (unsigned int)blk, mem_len,
		       (unsigned int)cnt);

		ret = write_compressed_ext4((char *)p, blk);
		printf("%s\n", ret ? "Fail" : "Done");
		return 1;
	}
	goto do_write;

do_write:
	if (!blk) {
		printf("Fail: start %d block(0x%llx) is in MBR zone (0x200)\n",
		       (int)blk, dst_addr);
		return -1;
	}

	printf("write mmc.%d = 0x%llx(0x%x) ~ 0x%llx(0x%x): ",
	       dev, dst_addr, (unsigned int)blk, mem_len, (unsigned int)cnt);

	ret = mmc_bwrite(dev, blk, cnt, (void const *)p);

	printf("%s\n", ret ? "Done" : "Fail");
	return ret;

usage:
	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(
	ext4_img_write, CONFIG_SYS_MAXARGS, 1,	do_compressed_ext4_write,
	"Compressed ext4 image write.\n",
	"img_write <dev no> 'mem' 'part no' 'length'\n"
	"    - update partition image 'length' on 'mem' to mmc 'part no'.\n\n"
	"Note.\n"
	"    - All numeric parameters are assumed to be hex.\n"
);

