/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <linux/sizes.h>
#include <asm/arch/nexell.h>
/*-----------------------------------------------------------------------
 *  u-boot-2016.01
 */
#define CONFIG_SYS_LDSCRIPT "arch/arm/cpu/armv7/s5p4418/u-boot.lds"

#define	CONFIG_MACH_S5P4418

/*-----------------------------------------------------------------------
 *  System memory Configuration
 */

#ifndef CONFIG_SYS_TEXT_BASE
#define	CONFIG_SYS_TEXT_BASE			0x94C00000
#endif
/* init and run stack pointer */
#define	CONFIG_SYS_INIT_SP_ADDR			CONFIG_SYS_TEXT_BASE

/* malloc() pool */
#define	CONFIG_MEM_MALLOC_START			(CONFIG_SYS_TEXT_BASE+0x400000)
#define CONFIG_MEM_MALLOC_LENGTH	(CONFIG_ENV_SIZE + (1 << 20) +	\
					CONFIG_SYS_DFU_DATA_BUF_SIZE * 2 + \
					(8 << 20))

/* when CONFIG_LCD */
#define CONFIG_FB_ADDR				(CONFIG_SYS_TEXT_BASE+0x2400000)

/* Download OFFSET */
#define CONFIG_MEM_LOAD_ADDR			(CONFIG_SYS_TEXT_BASE+0x4400000)

#define CONFIG_SYS_BOOTM_LEN    (64 << 20)      /* Increase max gunzip size */

/* AARCH64 */
#define COUNTER_FREQUENCY			200000000
#define CPU_RELEASE_ADDR			CONFIG_SYS_INIT_SP_ADDR

/*-----------------------------------------------------------------------
 *  High Level System Configuration
 */

/* Not used: not need IRQ/FIQ stuff	*/
#undef  CONFIG_USE_IRQ
/* decrementer freq: 1ms ticks */
#define CONFIG_SYS_HZ				1000

/* board_init_f */
#if defined(SDRAM_BASE) && (SDRAM_SIZE)
#define CONFIG_SYS_SDRAM_BASE			SDRAM_BASE
#define CONFIG_SYS_SDRAM_SIZE			SDRAM_SIZE
#else
#define	CONFIG_SYS_SDRAM_BASE			0x91000000
#define	CONFIG_SYS_SDRAM_SIZE			0x1f000000
#endif

/* dram 1 bank num */
#define CONFIG_NR_DRAM_BANKS			1

/* relocate_code and  board_init_r */
#define	CONFIG_SYS_MALLOC_END			(CONFIG_MEM_MALLOC_START + \
						 CONFIG_MEM_MALLOC_LENGTH)
/* board_init_f, more than 2M for ubifs */
#define CONFIG_SYS_MALLOC_LEN \
	(CONFIG_MEM_MALLOC_LENGTH - 0x8000)

/* kernel load address */
#define CONFIG_SYS_LOAD_ADDR			CONFIG_MEM_LOAD_ADDR

/* memtest works on */
#define CONFIG_SYS_MEMTEST_START		CONFIG_SYS_MALLOC_END
#define CONFIG_SYS_MEMTEST_END			((ulong)CONFIG_SYS_SDRAM_BASE \
						 + (ulong)CONFIG_SYS_SDRAM_SIZE)

/*-----------------------------------------------------------------------
 *  System initialize options (board_init_f)
 */

/* board_init_f->init_sequence, call arch_cpu_init */
#define CONFIG_ARCH_CPU_INIT
/* board_init_f->init_sequence, call board_early_init_f */
#define	CONFIG_BOARD_EARLY_INIT_F
/* board_init_r, call board_early_init_f */
#define	CONFIG_BOARD_LATE_INIT
/* board_init_f->init_sequence, call print_cpuinfo */
#define	CONFIG_DISPLAY_CPUINFO
/* board_init_f, CONFIG_SYS_ICACHE_OFF */
#define	CONFIG_SYS_DCACHE_OFF
/* board_init_r, call arch_misc_init */
#define	CONFIG_ARCH_MISC_INIT
/*#define	CONFIG_SYS_ICACHE_OFF*/

/*-----------------------------------------------------------------------
 *	U-Boot default cmd
 */
#define	CONFIG_CMD_MEMTEST

/*-----------------------------------------------------------------------
 *	U-Boot Environments
 */
/* refer to common/env_common.c	*/
#define CONFIG_BOOTDELAY			3

/*-----------------------------------------------------------------------
 * Miscellaneous configurable options
 */
#ifdef CONFIG_SYS_PROMPT
#undef CONFIG_SYS_PROMPT
/* Monitor Command Prompt   */
#define CONFIG_SYS_PROMPT			"artik530# "
#endif
/* undef to save memory	   */
#define CONFIG_SYS_LONGHELP
/* Console I/O Buffer Size  */
#define CONFIG_SYS_CBSIZE			1024
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE			(CONFIG_SYS_CBSIZE + \
						 sizeof(CONFIG_SYS_PROMPT)+16)
/* max number of command args   */
#define CONFIG_SYS_MAXARGS			16
/* Boot Argument Buffer Size    */
#define CONFIG_SYS_BARGSIZE			CONFIG_SYS_CBSIZE

/*-----------------------------------------------------------------------
 * allow to overwrite serial and ethaddr
 */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_SYS_HUSH_PARSER			/* use "hush" command parser */
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

/*-----------------------------------------------------------------------
 * Etc Command definition
 */
#define	CONFIG_CMD_IMI				/* image info	*/
#define CONFIG_CMDLINE_EDITING			/* add command line history */
#define	CONFIG_CMDLINE_TAG			/* use bootargs commandline */
#undef	CONFIG_BOOTM_NETBSD
#undef	CONFIG_BOOTM_RTEMS
#define CONFIG_INITRD_TAG
#define CONFIG_CMD_BOOTZ

/*-----------------------------------------------------------------------
 * serial console configuration
 */
#define CONFIG_PL011_SERIAL
#define CONFIG_CONS_INDEX			3
#define CONFIG_PL011_CLOCK			50000000
#define CONFIG_PL01x_PORTS			{(void *)PHY_BASEADDR_UART0, \
						(void *)PHY_BASEADDR_UART1, \
						(void *)PHY_BASEADDR_UART2, \
						(void *)PHY_BASEADDR_UART3}

#define CONFIG_BAUDRATE				115200
#define CONFIG_SYS_BAUDRATE_TABLE \
		{ 9600, 19200, 38400, 57600, 115200 }
#define CONFIG_PL011_SERIAL_FLUSH_ON_INIT

/*-----------------------------------------------------------------------
 * NOR FLASH
 */
#define	CONFIG_SYS_NO_FLASH

/*-----------------------------------------------------------------------
 * SD/MMC
 */

#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_DWMMC
#define CONFIG_NEXELL_DWMMC
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_CMD_MMC

#if defined(CONFIG_MMC)
#define CONFIG_2NDBOOT_OFFSET		512
#define CONFIG_2NDBOOT_SIZE		(64*1024)
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#define	CONFIG_ENV_OFFSET		0x2E0200
#define CONFIG_ENV_SIZE			(16*1024)	/* env size */
#endif

#if defined(CONFIG_MMC)
#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_FAT
#define CONFIG_FS_FAT
#define CONFIG_FAT_WRITE

#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_FS_EXT4
#define CONFIG_EXT4_WRITE
#endif

/*-----------------------------------------------------------------------
 * Default environment organization
 */
#if !defined(CONFIG_ENV_IS_IN_MMC) && !defined(CONFIG_ENV_IS_IN_NAND) && \
	!defined(CONFIG_ENV_IS_IN_FLASH) && !defined(CONFIG_ENV_IS_IN_EEPROM)
	/* default: CONFIG_ENV_IS_NOWHERE */
	#define CONFIG_ENV_IS_NOWHERE
	#define	CONFIG_ENV_OFFSET		1024
	#define CONFIG_ENV_SIZE			(4*1024)	/* env size */
	/* imls - list all images found in flash, default enable so disable */
	#undef	CONFIG_CMD_IMLS
#endif

/*-----------------------------------------------------------------------
 * PLL
 */

#define CONFIG_SYS_PLLFIN			24000000UL

/*-----------------------------------------------------------------------
 * Timer
 */

#define CONFIG_TIMER_SYS_TICK_CH		0

/*-----------------------------------------------------------------------
 * GPT
 */
#define CONFIG_CMD_GPT
#define CONFIG_EFI_PARTITION
#define CONFIG_PARTITION_UUIDS
#define CONFIG_RANDOM_UUID

#define CONFIG_CMD_DFU
#define CONFIG_USB_GADGET_DOWNLOAD

#define CONFIG_DISPLAY_BOARDINFO

/* TIZEN THOR downloader support */
#define CONFIG_CMD_THOR_DOWNLOAD
#define CONFIG_USB_FUNCTION_THOR

#define CONFIG_USB_FUNCTION_DFU
#define CONFIG_DFU_MMC
#define CONFIG_SYS_DFU_DATA_BUF_SIZE SZ_32M
#define DFU_DEFAULT_POLL_TIMEOUT 300

#define CONFIG_CMD_USB_MASS_STORAGE
#define CONFIG_USB_FUNCTION_MASS_STORAGE

/*-----------------------------------------------------------------------
 * Fastboot and USB OTG
 */

#define CONFIG_USB_FUNCTION_FASTBOOT
#define CONFIG_CMD_FASTBOOT
#define CONFIG_FASTBOOT_FLASH
#define CONFIG_FASTBOOT_FLASH_MMC_DEV   0
#define CONFIG_FASTBOOT_BUF_SIZE        (CONFIG_SYS_SDRAM_SIZE - SZ_1M)
#define CONFIG_FASTBOOT_BUF_ADDR        CONFIG_SYS_SDRAM_BASE
#define CONFIG_USB_GADGET
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_USB_GADGET_VBUS_DRAW     0
#define CONFIG_USB_GADGET_DWC2_OTG
#define CONFIG_USB_GADGET_NX_UDC_OTG_PHY
#define CONFIG_USB_GADGET_DOWNLOAD
#define CONFIG_SYS_CACHELINE_SIZE       64
#define CONFIG_G_DNL_VENDOR_NUM         0x18d1  /* google */
#define CONFIG_G_DNL_PRODUCT_NUM        0x0002  /* nexus one */
#define CONFIG_G_DNL_THOR_VENDOR_NUM	0x04e8
#define CONFIG_G_DNL_THOR_PRODUCT_NUM	0x685D
#define CONFIG_G_DNL_UMS_VENDOR_NUM	0x04e8
#define CONFIG_G_DNL_UMS_PRODUCT_NUM	0x685C
#define CONFIG_G_DNL_MANUFACTURER       "Samsung Electronics"

/*-----------------------------------------------------------------------
 * Nexell USB Downloader
 */
#define CONFIG_NX_USBDOWN

/*-----------------------------------------------------------------------
 * OF_CONTROL
 */

#define CONFIG_FIT_BEST_MATCH
#define CONFIG_OF_LIBFDT

/*-----------------------------------------------------------------------
 * GMAC
 */
#define CONFIG_PHY_REALTEK

#define CONFIG_ETHPRIME			"RTL8211"
#define CONFIG_PHY_ADDR			3

#define CONFIG_DW_ALTDESCRIPTOR

#define CONFIG_PHY_GIGE
#define CONFIG_MII
#define CONFIG_CMD_MII

/* NET */
#define CONFIG_CMD_GEN_ETHADDR

/* FACTORY_INFO */
#define CONFIG_CMD_FACTORY_INFO
#define CONFIG_FACTORY_INFO_BUF_ADDR		(CONFIG_SYS_SDRAM_BASE + \
						 CONFIG_SYS_SDRAM_SIZE - \
						 0x4000000)
#define CONFIG_FACTORY_INFO_START		0x1c00
#define CONFIG_FACTORY_INFO_SIZE		0x100

/* Board specific feature */
#define CONFIG_CHECK_BONDING_ID
#define CONFIG_CHECK_BOARD_TYPE

/* OTA */
#if defined(CONFIG_ARTIK_OTA)
#define CONFIG_FLAG_INFO_ADDR	0x9A000000
#endif

/* MAC Generator */
#define CONFIG_ARTIK_MAC

/*-----------------------------------------------------------------------
 * BOOTCOMMAND
 */
#define CONFIG_REVISION_TAG

#define CONFIG_DEFAULT_CONSOLE		"console=ttyAMA3,115200n8\0"

#define CONFIG_ROOT_DEV		0
#define CONFIG_BOOT_PART_SD	1
#define CONFIG_MODULES_PART_SD	2
#define CONFIG_ROOT_PART_SD	3

#if !defined(CONFIG_ARTIK_OTA)
#define CONFIG_BOOT_PART	1
#define CONFIG_MODULES_PART	2
#define CONFIG_ROOT_PART	3
#else
#define CONFIG_BOOT_PART	2
#define CONFIG_BOOT1_PART	3
#define CONFIG_MODULES_PART	5
#define CONFIG_MODULES1_PART	6
#define CONFIG_ROOT_PART	7
#endif

#define CONFIG_SET_DFU_ALT_INFO
#define CONFIG_SET_DFU_ALT_BUF_LEN	(SZ_1K)

#define CONFIG_DFU_ALT \
	"bl1-emmcboot.img raw 0x1 0x80;" \
	"bl1-sdboot.img raw 0x1 0x80;" \
	"loader-emmc.img raw 0x81 0x180;" \
	"loader-sd.img raw 0x81 0x180;" \
	"bl_mon.img raw 0x201 0x100;" \
	"secureos.img raw 0x301 0xc00;" \
	"bootloader.img raw 0xf01 0x800;" \
	"/uImage ext4 $rootdev $bootpart;" \
	"/zImage ext4 $rootdev $bootpart;" \
	"/uInitrd ext4 $rootdev $bootpart;" \
	"/ramdisk.gz ext4 $rootdev $bootpart;" \
	"/s5p4418-artik532-raptor-rev03.dtb ext4 $rootdev $bootpart;" \
	"/s5p4418-artik530-raptor-rev03.dtb ext4 $rootdev $bootpart;" \
	"/s5p4418-artik530-raptor-rev00.dtb ext4 $rootdev $bootpart;" \
	"/s5p4418-artik532-explorer.dtb ext4 $rootdev $bootpart;" \
	"/s5p4418-artik530-explorer.dtb ext4 $rootdev $bootpart;" \
	"boot part $rootdev $bootpart;" \
	"modules part $rootdev $modulespart;" \
	"rootfs part $rootdev $rootpart;" \
	"params.bin raw 0x1701 0x20;" \
	"/Image.itb ext4 $rootdev $bootpart\0"

#define CONFIG_EXTRA_ENV_SETTINGS					\
	"fdt_high=0xffffffff\0"						\
	"kernel_file=zImage\0"						\
	"ramdisk_file=uInitrd\0"					\
	"sdram_base=" __stringify(CONFIG_SYS_SDRAM_BASE) "\0"		\
	"kernel_offs=0x00080000\0"					\
	"ramdisk_offs=0x09000000\0"					\
	"fdt_offs=0x0a000000\0"						\
	"sd_offset=0x08000000\0"				\
	"gen_addr="							\
		"setexpr kerneladdr $sdram_base + $kernel_offs; "	\
		"setexpr ramdiskaddr $sdram_base + $ramdisk_offs; "	\
		"setexpr fdtaddr $sdram_base + $fdt_offs\0"		\
	"gen_sdrecaddr="										\
		"setexpr sdrecaddr $sdram_base + $sd_offset\0"	\
	"load_fdt="							\
		"if test -z \"$fdtfile\"; then "                        \
		"loop=$board_rev; "					\
		"number=$board_rev: "					\
		"success=0; "						\
		"until test $loop -eq 0 || test $success -ne 0; do "	\
			"if test $loop -lt 10; then "			\
				"number=0$loop; "			\
			"else number=$loop; "				\
			"fi; "						\
			"ext4size mmc $rootdev:$bootpart s5p4418-artik${model_id}-raptor-rev${number}.dtb && setexpr success 1; " \
			"setexpr loop $loop - 1; "			\
		"done; "					\
		"if test $success -eq 0; then "				\
			"ext4load mmc $rootdev:$bootpart $fdtaddr s5p4418-artik530-raptor-rev00.dtb;"	\
		"else "							\
			"ext4load mmc $rootdev:$bootpart $fdtaddr s5p4418-artik${model_id}-raptor-rev${number}.dtb; "	\
		"fi; "							\
		"else ext4load mmc $rootdev:$bootpart $fdtaddr $fdtfile; " \
		"fi; setenv success; setenv number; setenv loop;\0"	\
	"bootdelay=" __stringify(CONFIG_BOOTDELAY) "\0"			\
	"console=" CONFIG_DEFAULT_CONSOLE				\
	"consoleon=setenv console=" CONFIG_DEFAULT_CONSOLE		\
		"; saveenv; reset\0"					\
	"consoleoff=setenv console=ram; saveenv; reset\0"		\
	"rootdev=" __stringify(CONFIG_ROOT_DEV) "\0"			\
	"rootpart=" __stringify(CONFIG_ROOT_PART) "\0"			\
	"bootpart=" __stringify(CONFIG_BOOT_PART) "\0"			\
	"modulespart=" __stringify(CONFIG_MODULES_PART) "\0"		\
	"rescue=0\0"							\
	"root_rw=rw\0"							\
	"model_id=530\0"							\
	"nr_cpus=4\0"							\
	"opts=loglevel=4\0"						\
	"rootfs_type=ext4\0"						\
	"lcd1_0=s6e8fa0\0"						\
	"lcd2_0=gst7d0038\0"						\
	"lcd_panel=s6e8fa0\0"						\
	"sdrecovery=run boot_cmd_sdboot;"				\
		"sd_recovery mmc 1:3 $sdrecaddr partmap_emmc.txt\0"	\
	"factory_load=factory_info load mmc 0 "				\
		__stringify(CONFIG_FACTORY_INFO_START) " "		\
		__stringify(CONFIG_FACTORY_INFO_SIZE) "\0"		\
	"factory_save=factory_info save mmc 0 "				\
		__stringify(CONFIG_FACTORY_INFO_START) " "		\
		__stringify(CONFIG_FACTORY_INFO_SIZE) "\0"		\
	"factory_set_ethaddr=run factory_load; gen_eth_addr ;"		\
		"factory_info write ethaddr $ethaddr;"			\
		"run factory_save\0"					\
	"load_args=run factory_load; setenv bootargs ${console} "	\
		"root=/dev/mmcblk${rootdev}p${rootpart} ${root_rw} "	\
		"rootfstype=${rootfs_type} ${recoverymode} ${ota} "	\
		"drm_panel=$lcd_panel nr_cpus=${nr_cpus} ${opts} "	\
		"bootfrom=${bootpart} rescue=${rescue};\0"		\
	"load_kernel="							\
		"ret=0; "						\
		"ext4load mmc ${rootdev}:${bootpart} $kerneladdr $kernel_file && setexpr ret 1; " \
		"if test $ret -eq 0; then "				\
			"if test $bootpart -eq 2; then "		\
				"setenv bootpart 3; "			\
			"else setenv bootpart 2; "			\
			"fi; "						\
			"setenv rescue 1; "				\
			"ext4load mmc ${rootdev}:${bootpart} $kerneladdr $kernel_file; " \
			"run load_args; "				\
		"fi;\0"							\
	"load_initrd=ext4load mmc ${rootdev}:${bootpart} $ramdiskaddr $ramdisk_file\0" \
	"boot_cmd_initrd="						\
		"run load_kernel; run load_fdt; run load_initrd;"	\
		"bootz $kerneladdr $ramdiskaddr $fdtaddr\0"		\
	"boot_cmd_mmcboot="						\
		"run load_kernel; run load_fdt;"			\
		"bootz $kerneladdr - $fdtaddr\0"			\
	"boot_cmd_sdboot="						\
		"setenv bootpart " __stringify(CONFIG_BOOT_PART_SD)"; "	\
		"setenv modulespart " __stringify(CONFIG_MODULES_PART_SD)"; "	\
		"setenv rootpart " __stringify(CONFIG_ROOT_PART_SD)";\0"	\
	"ramfsboot=run gen_addr; run load_args; run boot_cmd_initrd\0"	\
	"mmcboot=run gen_addr; run load_args; run boot_cmd_mmcboot\0"	\
	"recovery_cmd=run sdrecovery; setenv recoverymode recovery\0"	\
	"recoveryboot=run gen_sdrecaddr; run recovery_cmd; run ramfsboot\0"		\
	"hwtestboot=setenv rootdev 1;"					\
		"setenv opts rootfstype=ext4 rootwait loglevel=4;"	\
		"setenv fdtfile s5p4418-artik${model_id}-explorer.dtb;"		\
		"run mmcboot\0"						\
	"hwtest_recoveryboot=run recovery_cmd; run hwtestboot\0"        \
	"bootcmd=run ramfsboot\0"

#endif /* __CONFIG_H__ */
