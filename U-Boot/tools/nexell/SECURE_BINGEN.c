/*
 * Program for nexell bootloader binary
 * To be run on Host PC
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author: Sangjong, Han <hans@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <ctype.h>

#include "nx_bootheader.h"
#include "SECURE_BINGEN.h"

#define DEBUG 0
#define DUMP_DEBUG 0

/* bootinfo -> number of boot table entries */
#define NR_BOOTINFO 4

struct nx_bootheader *pBootInfo;

static uint8_t *cpu_name;
static uint8_t *option_name;
static uint8_t *nsih_name;
static uint8_t *output_name;
static uint8_t *input_name;
static uint32_t output_size;
static uint32_t input_size;

static uint64_t loaded_addr = -1;
static uint64_t launch_addr = -1;

/* Address of the device to find the image want to load.*/
static uint64_t load_addr[NR_BOOTINFO];
static uint8_t load_dev[NR_BOOTINFO];
static uint8_t portnum[NR_BOOTINFO];
static uint8_t load_count;
static uint8_t boot_count;
static uint8_t port_count;

static uint8_t bootdev = BOOT_FROM_SDMMC;
/* When loading unified image might using usb */
static uint8_t unified_image;
static uint32_t simg_ofs[NR_BOOTINFO];
static uint32_t simg_siz[NR_BOOTINFO];
static uint8_t simg_ofscnt;
static uint8_t simg_sizcnt;


/* Data Dump */
void print_hexdump(uint32_t *pdwBuffer, uint32_t Size)
{
	int32_t i = 0;

	for (i = 0; i < Size / 4; i++) {
		printf("%8X  ", pdwBuffer[i]);
		if (((i + 1) % 8) == 0)
			printf("\r\n");
	}
	printf("\r\n");
}

/* It Converted to Uppercase. */
void to_upper(char *string)
{
	char *str = (char *)string;
	int32_t ndx = 0;

	for (ndx = 0; ndx < strlen(str); ndx++) {
		str[ndx] = (char)toupper(str[ndx]);
	}
}

/* It Converted to Lowercase. */
void to_lower(char *string)
{
	char *str = (char *)string;
	int32_t ndx = 0;

	for (ndx = 0; ndx < strlen(str); ndx++) {
		str[ndx] = (char)tolower(str[ndx]);
	}
}

/* Convert hexadecimal strings to integers. */
uint32_t hexa_toint(const char *string)
{
	char ch;
	uint32_t result = 0;

	while ((ch = *string++) != 0) {
		if (ch >= '0' && ch <= '9') {
			result = result * 16 + (ch - '0');
		} else if (ch >= 'a' && ch <= 'f') {
			result = result * 16 + (ch - 'a') + 10;
		} else if (ch >= 'A' && ch <= 'F') {
			result = result * 16 + (ch - 'A') + 10;
		}
	}

	return result;
}

size_t get_file_size(FILE *fd)
{
	size_t fileSize;
	long curPos;

	curPos = ftell(fd);

	fseek(fd, 0, SEEK_END);
	fileSize = ftell(fd);
	fseek(fd, curPos, SEEK_SET);

	return fileSize;
}


/* 4 bytes of CRC value calculated on the generation. */
static inline uint32_t iget_fcs(uint32_t fcs, uint32_t data)
{
	int32_t i;
	fcs ^= data;
	for (i = 0; i < 32; i++) {
		if (fcs & 0x01)
			fcs =  (fcs >> 1) ^ POLY;
		else
			fcs >>= 1;
	}
	return fcs;
}

static inline uint32_t __icalc_crc(void *addr, int32_t len)
{
	uint32_t *c = (uint32_t *)addr;
	uint32_t crc = 0, chkcnt = ((len + 3) / 4);
	uint32_t i;

	for (i = 0; chkcnt > i; i += CHKSTRIDE, c += CHKSTRIDE) {
		crc = iget_fcs(crc, *c);
	}

	return crc;
}

/* 1 bytes of CRC value calculated on the generation. */
static inline uint32_t get_fcs(uint32_t fcs, uint8_t data)
{
	int32_t i;
	fcs ^= (uint32_t)data;
	for (i = 0; i < 8; i++) {
		if (fcs & 0x01)
			fcs = (fcs >> 1) ^ POLY;
		else
			fcs >>= 1;
	}
	return fcs;
}

static inline uint32_t __calc_crc(void *addr, int32_t len)
{
	uint8_t *c = (uint8_t *)addr;
	uint32_t crc = 0;
	int32_t i;
	for (i = 0; len > i; i++) {
		crc = get_fcs(crc, c[i]);
	}
	return crc;
}

/* Nexell System Information Header */
int32_t process_nsih(const char *pfilename, uint8_t *pOutData)
{
	FILE *fp;
	char ch;
	int32_t writesize, skipline, line, bytesize, i;
	uint32_t writeval;

	fp = fopen(pfilename, "rb");
	if (!fp) {
		printf("process_nsih : ERROR - Failed to open %s file.\n",
		       pfilename);
		return 0;
	}

	bytesize = 0;
	writeval = 0;
	writesize = 0;
	skipline = 0;
	line = 0;

	while (0 == feof(fp)) {
		ch = fgetc(fp);

		if (skipline == 0) {
			if (ch >= '0' && ch <= '9') {
				writeval = writeval * 16 + ch - '0';
				writesize += 4;
			} else if (ch >= 'a' && ch <= 'f') {
				writeval = writeval * 16 + ch - 'a' + 10;
				writesize += 4;
			} else if (ch >= 'A' && ch <= 'F') {
				writeval = writeval * 16 + ch - 'A' + 10;
				writesize += 4;
			} else {
				if (writesize == 8 || writesize == 16 ||
				    writesize == 32) {
					for (i = 0; i < writesize / 8; i++) {
						pOutData[bytesize++] =
						    (uint8_t)(writeval & 0xFF);
						writeval >>= 8;
					}
				} else {
					if (writesize != 0)
						printf("process_nsih : Error "
						       "at %d line.\n",
						       line + 1);
				}
				writesize = 0;
				skipline = 1;
			}
		}

		if (ch == '\n') {
			line++;
			skipline = 0;
			writeval = 0;
		}
	}
	fclose(fp);

	return bytesize;
}

static int nsih_parsing(int8_t *nsih_name)
{
	int8_t *buf = (int8_t *)malloc(NSIH_SIZE);
	if (NSIH_SIZE != process_nsih(nsih_name, buf)) {
		NX_ERROR("Failed to process NSIH.\n");
		return -1;
	}

	pBootInfo = (struct nx_bootheader *)buf;

	return 0;
}

static void nsih_user_config(char *buffer)
{
	int32_t i = 0;
	struct nx_tbbinfo *info;

	/* 2ndboot & 3rdboot Header Modify Information. */
	pBootInfo = (struct nx_bootheader *)buffer;
	info = &pBootInfo->tbbi;

	/* Device Dependency */
	info->loadsize = input_size;

	/* NSIH Default (Load Address, Launch Address) */
	if (loaded_addr != -1)
		info->loadaddr = loaded_addr;
	if (launch_addr != -1)
		info->startaddr = launch_addr;


	if (bootdev == BOOT_FROM_USB) {
		/* Address of split image to load */
		for (i = 0; i < load_count; i++)
			info->dbi[i].usbbi.split_loadaddr = load_addr[i];

		for (i = 0; i < simg_ofscnt; i++)
			info->dbi[i].usbbi.split_offset = simg_ofs[i];

		for (i = 0; i < simg_sizcnt; i++)
			info->dbi[i].usbbi.split_size = simg_siz[i];
	} else {
		/* Address of the device to find the image want to load.*/
		for (i = 0; i < load_count; i++)
			info->dbi[i].sdmmcbi.deviceaddr = load_addr[i];

		for (i = 0; i < boot_count; i++)
			info->dbi[i].sdmmcbi.loaddevicenum = load_dev[i];

		for (i = 0; i < port_count; i++)
			info->dbi[i].sdmmcbi.portnumber = portnum[i];
	}

	info->unified = unified_image;
	info->bootdev = bootdev;

	/* Signature (NSIH) */
	info->signature = HEADER_ID;
	/* CRC code generation must be last */
	info->crc32 =
		__calc_crc((void *)(buffer + HEADER_SIZE), input_size);

#ifdef BOOT_DEBUG
	NX_DEBUG("LOADSIZE   : %u\n", info->loadsize);
	NX_DEBUG("LOADADDR   : %llx\n", info->loadaddr);
	NX_DEBUG("LAUNCHADDR : %llx\n", info->startaddr);
	NX_DEBUG("SIGNATURE  : %x\n", info->signature);
	NX_DEBUG("crc32      : %x\n", info->crc32);
	for (i = 0; i < load_count; i++)
		NX_DEBUG("to load device addr: 0x%llx\n",
			 info->dbi[i].sdmmcbi.deviceaddr);
	for (i = 0; i < boot_count; i++)
		NX_DEBUG("boot device number: %d\n",
			 info->dbi[i].sdmmcbi.loaddevicenum);
	for (i = 0; i < port_count; i++)
		NX_DEBUG("port number: %d\n",
			 info->dbi[i].sdmmcbi.portnumber);
#endif
}

static void nsih_get_bootmode(void)
{
}

static void print_bingen_info(void)
{
}


/* Chip by, calculating an output size based on the size limit on the SRAM.  */
static int32_t output_size_calcurate(uint32_t input_size, uint32_t *output_size)
{
	uint32_t out_size = 0;

	/* Ourput Size Calcurate */
	if (0 == strcmp(option_name, "2ndboot")) {
		if ((0 == strcmp(cpu_name, "NXP4330"))) {
			if (input_size > (NXP4330_SRAM_SIZE - HEADER_SIZE)) {
				printf("exceed SRAM size of nxp4330 (16KB)\n");
				return -1;
			}
			out_size = NXP4330_SRAM_SIZE;
		} else if ((0 == strcmp(cpu_name, "NXP5430")) ||
			   (0 == strcmp(cpu_name, "S5P6818"))) {
			out_size = input_size + HEADER_SIZE;
			if (out_size >
			    (NXP5430_SRAM_SIZE - ROMBOOT_STACK_SIZE)) {
				printf("The image is Generated exceeds 64KB. "
				       "The Creation failed!\n");
				printf("Calcurate image Size : %d\n",
				       out_size);
				printf("Return Error End!!\n");
				return -1;
			}
		} else if (0 == strcmp(cpu_name, "S5P4418")) {
			out_size = input_size + HEADER_SIZE;
			if (out_size >
			    (S5P4418_SRAM_SIZE - ROMBOOT_STACK_SIZE)) {
				printf("The image is Generated exceeds 28KB. "
				       "The Creation failed!\n");
				printf("Calcurate image Size : %d\n",
				       out_size);
				printf("Return Error End!!\n");
				/* out_size = 28*1024; */
				return -1;
			} else if (out_size < NXP4330_SRAM_SIZE) {
				/* S5P4418 BinGen Size < 16*1024 */
				out_size = NXP4330_SRAM_SIZE;
			}
		} else
			printf("CPU(Chip) name is unknown.\n");

	} else if (0 == strcmp(option_name, "3rdboot")) {
		out_size = input_size + HEADER_SIZE;
	}

	/* File Descript Check & Maximum Size */
	if (out_size == 0) {
		printf("Did not enter the Filesize files.\n");
		return -1;
	} else {
		if (out_size <= (NXP4330_SRAM_SIZE - 16))
			out_size = NXP4330_SRAM_SIZE;
	}

	*output_size = out_size;

	return 0;
}

/* @ Function : Other Boot Bingen (SPI, SD, UART, ETC )
 * @ Param    : None.
 * @ Remak   : (SD, SPI, UART, etc.) Except the boot mode, making BINGEN NAND.
 */
static int32_t boot_bingen(void)
{
	FILE *in_file = NULL;
	FILE *out_file = NULL;

	uint8_t *out_buffer = NULL;

	uint32_t ret = -1;
	uint32_t read_cnt;

	in_file = fopen(input_name, "r");
	out_file = fopen(output_name, "wb");


	if (!in_file) {
		NX_ERROR("Input File open failed!! Check filename!!\n");
		goto out_close;
	}

	if ((!out_file)) {
		NX_ERROR("Output File open failed!! Check filename!!\n");
		goto out_close;
	}

	/* File Size Confirm. */
	fseek(in_file, 0, SEEK_END);
	input_size = ftell(in_file);
	fseek(in_file, 0, SEEK_SET);

	/* Output Size Calcurate. (+header size) */
	if (0 > output_size_calcurate(input_size, &output_size))
		goto out_close;


	/* allocate output buffer */
	out_buffer = (uint8_t *)malloc(output_size);
	memset(out_buffer, 0x00, output_size); /* 0xff */

	/* Read to Process NSIH */
	if (NSIH_SIZE != process_nsih(nsih_name, out_buffer)) {
		NX_ERROR("Failed to process NSIH.\n");
		ret = -1;
		goto out_free;
	}

	read_cnt = fread(out_buffer + HEADER_SIZE, 1, input_size, in_file);
	if (read_cnt < 0) {
		NX_ERROR("File Read Failed. (read cnt: %u)\n", read_cnt);
		ret = -1;
		goto out_free;
	}

#if SECURE_BOOT
	/* Secure Boot <-- Decript Issue ( 16Byte Convert ) */
	if (((input_size % 16) != 0))
		input_size = ((input_size / 16) * 16);
#endif
	/* NSIH, Set the user you want. */
	nsih_user_config((char *)out_buffer);
	/* Written information on the boot device. */
	nsih_get_bootmode();

	/* file image write. */
	fwrite(out_buffer, 1, output_size, out_file);
	/* bingen debug mesage */
	print_bingen_info();

	ret = 0;

out_free:
	free(out_buffer);
out_close:
	if (in_file != NULL)
		fclose(in_file);
	if (out_file != NULL)
		fclose(out_file);

	return ret;
}

/* @ Function : usage.
 * @ Param    : None.
 * @ Remak   : Help on usage.
 */
static void usage(void)
{
	printf("-----------------------------------------------------------\n");
	printf(" Release  Version         : Ver.%s\n", SECURE_BINGEN_VER);
	printf("-----------------------------------------------------------\n");
	printf(" Usage :\n");
	printf("   -h [HELP]                  : show usage\n");
	printf("   -k [dev id]                : DEVID_USB=0, DEVID_SDMMC=3\n");
	printf("\n");
	printf("  if usb\n");
	printf("   -u                         : unified image\n");
	printf("   -m [load address]          : split load address\n");
	printf("   -z [load size]             : split load size\n");
	printf("-----------------------------------------------------------\n");
	printf("\n");
}

int32_t main(int32_t argc, char **argv)
{
	uint32_t param_opt = 0;

	/* Pre-Fix  Default (Name & Value) */
	cpu_name = "S5P4418";
	option_name = "2ndboot";

	nsih_name = "NSIH.txt";
	output_name = "2ndboot_spi.bin";
	input_name = "pyrope_2ndboot_spi.bin";

	output_size = (16 * 1024);

	if (argc == true) {
		usage();
		return true;
	}

	while (-1 != (param_opt =
		      getopt(argc, argv, "hc:t:n:i:o:l:e:m:b:p:uf:z:k:"))) {
		switch (param_opt) {
		case 'h':
			usage();
			return true;
		case 'c':
			cpu_name = strdup(optarg);
			to_upper(cpu_name);
			break;
		case 't':
			option_name = strdup(optarg);
			to_lower(option_name);
			break;
		case 'n':
			nsih_name = strdup(optarg);
			break;
		case 'i':
			input_name = strdup(optarg);
			break;
		case 'o':
			output_name = strdup(optarg);
			break;
		case 'l':
			loaded_addr = strtoull(optarg, NULL, 0);
			break;
		case 'e':
			launch_addr = strtoull(optarg, NULL, 0);
			break;
		case 'm':
			if (load_count >= NR_BOOTINFO) {
				NX_ERROR("Too many load entry (max:%d)\n",
					 NR_BOOTINFO);
				break;
			}

			load_addr[load_count] = strtoull(optarg, NULL, 0);
			NX_DEBUG(" load[%d] %llu\n",
				 load_count + 1, load_addr[load_count]);
			load_count++;
			break;
		case 'b':
			if (boot_count >= NR_BOOTINFO) {
				NX_ERROR("Too many load_dev entry (max:%d)\n",
					 NR_BOOTINFO);
				break;
			}

			load_dev[boot_count] = strtoull(optarg, NULL, 0);
			NX_DEBUG(" load_dev[%d] %llu\n",
				 boot_count + 1, load_dev[boot_count]);
			boot_count++;
			break;
		case 'p':
			if (port_count >= NR_BOOTINFO) {
				NX_ERROR("Too many portnum entry (max:%d)\n",
					 NR_BOOTINFO);
				break;
			}

			portnum[port_count] = strtoull(optarg, NULL, 0);
			NX_DEBUG(" portnum[%d] %llu\n",
				 port_count + 1, portnum[port_count]);
			port_count++;
			break;
		case 'u':
			unified_image = 1;
			break;
		case 'f':
			if (simg_ofscnt >= NR_BOOTINFO) {
				NX_ERROR("Too many split ofs entry (max:%d)\n",
					 NR_BOOTINFO);
				break;
			}

			simg_ofs[simg_ofscnt] = strtoul(optarg, NULL, 0);
			NX_DEBUG(" split ofs[%d] %lu\n",
				 simg_ofscnt + 1, simg_ofs[simg_ofscnt]);
			simg_ofscnt++;
			break;
		case 'z':
			if (simg_sizcnt >= NR_BOOTINFO) {
				NX_ERROR("Too many split ofs entry (max:%d)\n",
					 NR_BOOTINFO);
				break;
			}

			simg_siz[simg_sizcnt] = strtoul(optarg, NULL, 0);
			NX_DEBUG(" split ofs[%d] %lu\n",
				 simg_sizcnt + 1, simg_siz[simg_sizcnt]);
			simg_sizcnt++;
			break;
		case 'k':
			bootdev = strtoull(optarg, NULL, 0);
			break;
		/* Unknown Option */
		default:
			NX_ERROR("unknown option_num parameter\n");
			break;
		}
	}

	if (nsih_name == NULL) {
		nsih_name = "NSIH.txt";
		NX_DEBUG("Did not enter the NSIH files.\n");
		NX_DEBUG("This has been used as the default NSIH file.\n");
	}

	if (input_name == NULL) {
		input_name = "pyrope_secondboot.bin";
		NX_DEBUG("Did not enter the Binary files.\n");
		NX_DEBUG("This has been used as the default "
			 "pyrope_secondboot.bin.\n");
	}

	/* NSIH Parsing */
	nsih_parsing((int8_t *)nsih_name);

	NX_DEBUG("BOOT BINGEN!!\n");
	if (0 > boot_bingen()) {
		NX_ERROR("SECURE BINGEN Failed\n");
		return -1;
	}

	return 0;
}
