/*
 * Program for nexell bootloader binary
 * To be run on Host PC
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author:
 * - Sangjong, Han <hans@nexell.co.kr>
 * - DeokJin Lee <truevirtue@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>

#include "BOOT_BINGEN.h"

#define DEBUG			0
#define DUMP_DEBUG		0

struct NX_SecondBootInfo* pBootInfo;

static uint8_t *cpu_name;
static uint8_t *option_name;
static uint8_t *nsih_name;
static uint8_t *input_name;
static uint8_t *output_name;
static uint32_t output_size;
static uint32_t input_size;

static uint32_t g_device_addr = -1;
static uint32_t g_device_portnum = -1;

static uint32_t loaded_addr = -1;
static uint32_t launch_addr = -1;


/* Data Dump */
void print_hexdump( uint32_t* pdwBuffer, uint32_t Size )
{
	int32_t i = 0;

	for(i = 0; i < Size / 4; i++) {
		printf("%8X  ", pdwBuffer[i] );
		if( ((i+1) % 8) == 0 )
			printf("\r\n");
	}
	printf("\r\n");
}

/* It Converted to Uppercase.  */
static void to_upper( char* string )
{
	char *str = (char*)string;
	int32_t ndx = 0;

	for (ndx = 0; ndx < strlen(str); ndx++) {
		str[ndx] = (char)toupper(str[ndx]);
	}
}

/* It Converted to Lowercase.  */
static void to_lower( char* string )
{
	char *str = (char*)string;
	int32_t ndx = 0;

	for(ndx = 0; ndx < strlen(str); ndx++) {
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

static int nsih_parsing( int8_t* nsih_name )
{
	int8_t *buf = (int8_t *)malloc(NSIH_SIZE);
	if (NSIH_SIZE != process_nsih(nsih_name, buf)) {
		NX_ERROR("Failed to process NSIH.\n");
		return -1;
	}

	pBootInfo = (struct NX_SecondBootInfo*)buf;

	return 0;
}

static void nsih_user_config( char* buffer )
{
	CRC uCrc;
	int32_t i 		= 0;

	/* 2ndboot & 3rdboot Header Modify Information. */
	pBootInfo = (struct NX_SecondBootInfo*)buffer;

	/* Device Dependency */
	if (g_device_portnum != -1)
		pBootInfo->DBI.SDMMCBI.PortNumber = (uint8_t)g_device_portnum;
	if (g_device_addr != -1)
		pBootInfo->DEVICEADDR = g_device_addr;

	pBootInfo->LOADSIZE = input_size;

	/* NSIH Default ( Load Address, Launch Address ) */
	if (loaded_addr != -1)
		pBootInfo->LOADADDR = loaded_addr;
	if (launch_addr != -1)
		pBootInfo->LAUNCHADDR = launch_addr;

	pBootInfo->SIGNATURE = HEADER_ID; /* Signature (NSIH) */
	pBootInfo->DBI.SDMMCBI.CRC32 = __calc_crc((void*)(buffer + NSIH_SIZE),
					input_size);
	/* CRC - 2ndboot (16KB-16  ) */
	if ((input_size + NSIH_SIZE) <= (NXP4330_SRAM_SIZE - 16)) {
		uCrc.iCrc = __calc_crc((void*)(buffer), NXP4330_SRAM_SIZE - 16);
		for (i = 0; i < CRCSIZE; i++)
			buffer[NXP4330_SRAM_SIZE-16+i] = uCrc.chCrc[i];
	}

#ifdef BOOT_DEBUG
	NX_DEBUG("LOADSIZE	: %8X\n", pBootInfo->LOADSIZE);
	NX_DEBUG("LOADADDR	: %8X\n", pBootInfo->LOADADDR);
	NX_DEBUG("LAUNCHADDR	: %8X\n", pBootInfo->LAUNCHADDR);
	NX_DEBUG("SIGNATURE 	: %8X\n", pBootInfo->SIGNATURE);
	NX_DEBUG("CRC32 	: %8X\n", pBootInfo->DBI.SDMMCBI.CRC32);
#endif
}

static void print_bingen_info( void )
{
}

static void nsih_get_bootmode( void )
{
}


/* Chip by, calculating an output size based on the size limit on the SRAM.  */
static int32_t output_size_calcurate(uint32_t input_size, uint32_t *output_size)
{
	uint32_t out_size = 0;

	/* Ourput Size Calcurate */
	if (0 == strcmp(option_name, "2ndboot")) {
		if ((0 == strcmp(cpu_name, "NXP4330"))) {
			if (input_size > (NXP4330_SRAM_SIZE - NSIH_SIZE)) {
				printf("exceed SRAM size of nxp4330 (16KB)\n");
				return -1;
			}
			out_size = NXP4330_SRAM_SIZE;
		} else if ((0 == strcmp(cpu_name, "NXP5430")) ||
			   (0 == strcmp(cpu_name, "S5P6818"))) {
			out_size = input_size + NSIH_SIZE;
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
			out_size = input_size + NSIH_SIZE;
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
		out_size = input_size + NSIH_SIZE;
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

	/* Output Size Calcurate. */
	if (0 > output_size_calcurate(input_size, &output_size))
		goto out_close;

	/* allocate output buffer */
	out_buffer = (uint8_t *)malloc(output_size);
	memset(out_buffer, 0x00, output_size); /* 0xff */

	/* Read to Process NSIH */
	if (NSIH_SIZE != process_nsih(nsih_name, out_buffer)) {
		NX_ERROR("Failed to process NSIH.\n" );
		ret = -1;
		goto out_free;
	}

	read_cnt = fread(out_buffer + NSIH_SIZE, 1, input_size, in_file);
	if (read_cnt < 0) {
		NX_ERROR("File Read Failed. (read cnt: %u)\n", read_cnt);
		ret = -1;
		goto out_free;
	}

#if SECURE_BOOT
	// Secure Boot <-- Decript Issue ( 16Byte Convert )
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

/* @ Function : useage.
 * @ Param    : None.
 * @ Remak   : Help on useage.
 */
void usage( void )
{
	printf("----------------------------------------------------------\n");
	printf(" Release  Version         : Ver.%04d\n", BOOT_BINGEN_VER );
	printf("----------------------------------------------------------\n");
	printf(" Usage : This will tell you How to Use Help.\n");
	printf("----------------------------------------------------------\n");
	printf("   -h [HELP]              : show usage\n");
	printf("   -c [S5P6818/S5P4418]   : chip name(mandatory)\n");
	printf("   -t [2nboot/3rdboot]    : boot type(mandatory)\n");
	printf("   	->[2ndboot]\n");
	printf("   	->[3rdboot]\n");
	printf("   -n [file name]         : [NSIH] file name(mandatory)\n");
	printf("   -i [file name]         : [INPUT]file name  (mandatory)\n");
	printf("   -o [file name]         : [OUTPUT]file name (mandatory)\n");
	printf("   -l [load address]      : Binary Load  Address\n");
	printf("   	-> Default Load	  Address : Default NSIH.txt\n");
	printf("   -e [launch address]           : Binary Launch Addres\n");
	printf("   	-> Default Launch Address : Default NSIH.txt\n");
	printf("\n");
}

int32_t main(int32_t argc, char **argv )
{
	uint32_t param_opt = 0;

	/* Pre-Fix  Default (Name & Value) */
	cpu_name = "S5P4418";
	option_name = "2ndboot";

	nsih_name = "NSIH.txt";
	output_name = "2ndboot_spi.bin";
	input_name = "pyrope_2ndboot_spi.bin";

	output_size = (16*1024);

	if (argc == true) {
		usage();
		return true;
	}

	while(-1 != (param_opt =
		     getopt( argc, argv, "hc:t:n:i:o:l:e:a:u:v:p:f:r:"))) {
		switch(param_opt) {
		case 'h':
			usage();
			return true;
		case 'c':
			cpu_name	  = strdup(optarg);
			to_upper(cpu_name);
			break;
		case 't':
			option_name = strdup(optarg);
			to_lower(option_name);
			break;
		case 'n':
			nsih_name  = strdup(optarg);
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
		case 'u':
			g_device_portnum = strtoull(optarg, NULL, 0);
			break;
		case 'a':
			g_device_addr	 = strtoull(optarg, NULL, 0);
			break;
		/* Unknown Option */
		default:
			NX_ERROR("unknown option_num parameter\r\n");
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
	nsih_parsing( (int8_t*)nsih_name );

	NX_DEBUG("BOOT BINGEN!! \n");
	if (0 > boot_bingen()) {
		NX_ERROR("BOOT BINGEN Failed \n");
		return -1;
	}

	return 0;
}
