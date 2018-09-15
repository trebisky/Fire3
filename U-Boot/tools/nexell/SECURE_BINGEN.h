#ifndef __SECURE_BINGEN_H__
#define __SECURE_BINGEN_H__

/*#define BOOT_DEBUG */

/* PRINT MACRO */
#ifdef BOOT_DEBUG
#define NX_DEBUG(msg...)                                                       \
	do {                                                                   \
		printf("[DEBUG] " msg);                                        \
	} while (0)
#else
#define NX_DEBUG(msg...)                                                       \
	do {                                                                   \
	} while (0)
#endif
#define NX_ERROR(msg...)                                                       \
	do {                                                                   \
		printf("[ERROR] " msg);                                        \
	} while (0)


/* crc poly */
#define POLY 0xEDB88320L

#define NSIH_SIZE 0x200
#define ENCKEY_SIZE 0x200
#define HEADER_SIZE (NSIH_SIZE + ENCKEY_SIZE)
#define CRCSIZE (4)

#define BLOCKSIZE (0x200)

#define ROMBOOT_STACK_SIZE (4 * 1024)

#define NXP4330_SRAM_SIZE (16 * 1024)
#define NXP5430_SRAM_SIZE (64 * 1024)
#define S5P4418_SRAM_SIZE (32 * 1024)

#define SECONDBOOT_FSIZENCRC (16 * 1024)
#define SECONDBOOT_SSIZENCRC (8 * 1024)
#define SECONDBOOT_FSIZE (SECONDBOOT_FSIZENCRC - (128 / 8))
#define SECONDBOOT_SSIZE (SECONDBOOT_SSIZENCRC - (128 / 8))
#define CHKSTRIDE 8

#define SECURE_BOOT (0)

#define SECURE_BINGEN_VER "20160909-1"


#define __ALIGN(x, a) __ALIGN_MASK(x, (a)-1)
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

#define ALIGN(x, a) __ALIGN((x), (a))
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define IS_POWER_OF_2(n) ((n) && ((n) & ((n)-1)) == 0)

enum { FALSE,
       TRUE,
};

enum { IS_3RDBOOT = 0, IS_2NDBOOT = 1 };

typedef union crc {
	int iCrc;
	char chCrc[4];
} CRC;

#define MEM_TYPE_DDR3 1
#define ARCH_NXP4330 1

#endif
