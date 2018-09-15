#ifndef __BOOT_BINGEN_H__
#define __BOOT_BINGEN_H__
#include <stdint.h>

#define HEADER_ID		\
	((((uint32_t)'N') << 0) |	\
	 (((uint32_t)'S') << 8) |	\
	 (((uint32_t)'I') << 16) |	\
	 (((uint32_t)'H') << 24))

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

#define POLY 0x04C11DB7L

#define NSIH_SIZE	(0x200)
#define CRCSIZE		(4)

#define BLOCKSIZE	(0x200)

#define ROMBOOT_STACK_SIZE (4*1024)

#define NXP4330_SRAM_SIZE (16*1024)
#define NXP5430_SRAM_SIZE (64*1024)
#define S5P4418_SRAM_SIZE (32*1024)

#define SECONDBOOT_FSIZENCRC (16*1024)
#define SECONDBOOT_SSIZENCRC (8*1024)
#define SECONDBOOT_FSIZE (SECONDBOOT_FSIZENCRC - (128 / 8))
#define SECONDBOOT_SSIZE (SECONDBOOT_SSIZENCRC - (128 / 8))
#define CHKSTRIDE 8

#define SECURE_BOOT (0)

#define BOOT_BINGEN_VER (1011)


#define __ALIGN(x, a)		__ALIGN_MASK(x, (a) - 1)
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))

#define ALIGN(x, a)		__ALIGN((x), (a))
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define IS_POWER_OF_2(n)	((n) && ((n) & ((n)-1)) == 0)

enum {
	FALSE,
	TRUE,
};

enum {
	IS_3RDBOOT = 0,
	IS_2NDBOOT = 1
};

typedef union crc {
	int  iCrc;
	char chCrc[4];
} CRC;

#define MEM_TYPE_DDR3		1
#define ARCH_NXP4330		1

struct NX_NANDBootInfo {
	uint8_t  AddrStep;
	uint8_t  tCOS;
	uint8_t  tACC;
	uint8_t  tOCH;
	uint8_t  PageSize;           // 512bytes unit
	uint8_t  TIOffset;           // 3rd boot Image copy Offset. 1MB unit.
	uint8_t  CopyCount;          // 3rd boot image copy count
	uint8_t  LoadDevice;         // device chip select number
	uint32_t CRC32;
};

struct NX_SPIBootInfo {
	uint8_t  AddrStep;
	uint8_t  _Reserved0[2];
	uint8_t  PortNumber;

	uint32_t _Reserved1 : 24;
	uint32_t LoadDevice : 8;

	uint32_t CRC32;
};

struct NX_UARTBootInfo {
	uint32_t _Reserved0;

	uint32_t _Reserved1 : 24;
	uint32_t LoadDevice : 8;

	uint32_t CRC32;
};

struct NX_SDMMCBootInfo {
	uint8_t  PortNumber;
	uint8_t  _Reserved0[3];

	uint32_t _Reserved1	: 24;
	uint32_t LoadDevice	: 8 ;

	uint32_t CRC32;
};

struct NX_DDR3DEV_DRVDSInfo {
	uint8_t  MR2_RTT_WR;
	uint8_t  MR1_ODS;
	uint8_t  MR1_RTT_Nom;
	uint8_t  _Reserved0;
};

struct NX_LPDDR3DEV_DRVDSInfo {
	uint8_t  MR3_DS      : 4;
	uint8_t  MR11_DQ_ODT : 2;
	uint8_t  MR11_PD_CON : 1;
	uint8_t  _Reserved0  : 1;

	uint8_t  _Reserved1;
	uint8_t  _Reserved2;
	uint8_t  _Reserved3;
};

struct NX_DDRPHY_DRVDSInfo {
	uint8_t  DRVDS_Byte0;
	uint8_t  DRVDS_Byte1;
	uint8_t  DRVDS_Byte2;
	uint8_t  DRVDS_Byte3;

	uint8_t  DRVDS_CK;
	uint8_t  DRVDS_CKE;
	uint8_t  DRVDS_CS;
	uint8_t  DRVDS_CA;

	uint8_t  ZQ_DDS;
	uint8_t  ZQ_ODT;
	uint8_t  _Reserved0[2];
};

struct NX_SDFSBootInfo {
	char BootFileName[12];
};

union NX_DeviceBootInfo {
	struct NX_NANDBootInfo  NANDBI;
	struct NX_SPIBootInfo   SPIBI;
	struct NX_SDMMCBootInfo SDMMCBI;
	struct NX_SDFSBootInfo  SDFSBI;
	struct NX_UARTBootInfo  UARTBI;
};

struct NX_DDRInitInfo {
	uint8_t  ChipNum;
	uint8_t  ChipRow;
	uint8_t  BusWidth;
	uint8_t  ChipCol;

	uint16_t ChipMask;
	uint16_t ChipBase;

	uint8_t  CWL;
	uint8_t  CL;
	uint8_t  MR1_AL;
	uint8_t  MR0_WR;

	uint32_t READDELAY;
	uint32_t WRITEDELAY;

	uint32_t TIMINGPZQ;
	uint32_t TIMINGAREF;
	uint32_t TIMINGROW;
	uint32_t TIMINGDATA;
	uint32_t TIMINGPOWER;
};

struct NX_SecondBootInfo {
	uint32_t VECTOR[8];			/* 0x000 ~ 0x01C */
	uint32_t VECTOR_Rel[8];			/* 0x020 ~ 0x03C */

	uint32_t DEVICEADDR;			/* 0x040 */

	uint32_t LOADSIZE;			/* 0x044 */
	uint32_t LOADADDR;			/* 0x048 */
	uint32_t LAUNCHADDR;			/* 0x04C */
	union NX_DeviceBootInfo DBI;		/* 0x050~0x058 */

	uint32_t PLL[4];			/* 0x05C ~ 0x068 */
	uint32_t PLLSPREAD[2];			/* 0x06C ~ 0x070 */

#if defined(ARCH_NXP4330) || defined(ARCH_S5P4418)
	uint32_t DVO[5];			/* 0x074 ~ 0x084 */

	struct NX_DDRInitInfo DII;		/* 0x088 ~ 0x0AC */

#if defined(MEM_TYPE_DDR3)
	struct NX_DDR3DEV_DRVDSInfo     DDR3_DSInfo;	/* 0x0B0 */
#endif
#if defined(MEM_TYPE_LPDDR23)
	struct NX_LPDDR3DEV_DRVDSInfo   LPDDR3_DSInfo;	/* 0x0B0 */
#endif
	struct NX_DDRPHY_DRVDSInfo      PHY_DSInfo;	/* 0x0B4 ~ 0x0BC */

	uint16_t LvlTr_Mode;					/* 0x0C0 ~ 0x0C1 */
	uint16_t FlyBy_Mode;					/* 0x0C2 ~ 0x0C3 */

	uint32_t Stub[(0x1EC-0x0C4)/4];			/* 0x0C4 ~ 0x1EC */
#endif
#if defined(ARCH_NXP5430)
	uint32_t DVO[9];				/* 0x074 ~ 0x094 */

	struct NX_DDRInitInfo DII;			/* 0x098 ~ 0x0BC */

#if defined(MEM_TYPE_DDR3)
	struct NX_DDR3DEV_DRVDSInfo     DDR3_DSInfo;	/* 0x0C0 */
#endif
#if defined(MEM_TYPE_LPDDR23)
	struct NX_LPDDR3DEV_DRVDSInfo   LPDDR3_DSInfo;	/* 0x0C0 */
#endif
	struct NX_DDRPHY_DRVDSInfo      PHY_DSInfo;	/* 0x0C4 ~ 0x0CC */

	uint16_t LvlTr_Mode;					/* 0x0D0 ~ 0x0D1 */
	uint16_t FlyBy_Mode;					/* 0x0D2 ~ 0x0D3 */

	uint32_t Stub[(0x1EC-0x0D4)/4];			/* 0x0D4 ~ 0x1EC */
#endif

	uint32_t MemTestAddr;				/* 0x1EC */
	uint32_t MemTestSize;				/* 0x1F0 */
	uint32_t MemTestTryCount;			/* 0x1F4 */

	uint32_t BuildInfo;				/* 0x1F8 */

	uint32_t SIGNATURE;				/* 0x1FC "NSIH" */
} __attribute__ ((packed,aligned(4)));

void usage(void);

#endif
