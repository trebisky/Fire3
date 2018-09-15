/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>
#include <memalign.h>

#ifdef CONFIG_DM_PMIC_NXE2000
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#include <power/nxe2000.h>
#endif

#ifdef CONFIG_USB_GADGET
#include <usb.h>
#endif

#ifdef CONFIG_SENSORID_ARTIK
#include <sensorid.h>
#include <sensorid_artik.h>
#endif

#ifdef CONFIG_ARTIK_OTA
#include <artik_ota.h>
#endif

#ifdef CONFIG_ARTIK_MAC
#include <artik_mac.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_CHECK_BONDING_ID
int check_bonding_id(void)
{
	int bonding_id = 0;

	bonding_id = readl(PHY_BASEADDR_ECID + ID_REG_EC0);

	return bonding_id & WIRE0_MASK;
}
#endif

#ifdef CONFIG_CHECK_BOARD_TYPE
int check_board_type(void)
{	int val;

	val = nx_gpio_get_input_value(1, 25);
	val <<= 1;

	val |= nx_gpio_get_input_value(1, 24);

	return val;
}
#endif

#ifdef CONFIG_REVISION_TAG
u32 board_rev;

u32 get_board_rev(void)
{
	return board_rev;
}

static void check_hw_revision(void)
{
	u32 val = 0;

	val |= nx_gpio_get_input_value(4, 6);
	val <<= 1;

	val |= nx_gpio_get_input_value(4, 5);
	val <<= 1;

	val |= nx_gpio_get_input_value(4, 4);

	board_rev = val;
}

static void set_board_rev(u32 revision)
{
	char info[64] = {0, };

	snprintf(info, ARRAY_SIZE(info), "%d", revision);
	setenv("board_rev", info);
}
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	char board_type[20] = "ARTIK530 Raptor";
#ifdef CONFIG_CHECK_BONDING_ID
	int is_dual = check_bonding_id();

	if (is_dual) {
		memset(board_type, 0, 20);
		strncpy(board_type, "ARTIK532 Raptor", 20);
	}
#endif
#ifdef CONFIG_CHECK_BOARD_TYPE
	if (check_board_type() == 1) {
		memset(board_type, 0, 20);
		strncpy(board_type, "ARTIK533 Raptor", 20);
	}
#endif
	printf("\nBoard: %s\n", board_type);

	return 0;
}
#endif

#ifdef CONFIG_SENSORID_ARTIK
static void get_sensorid(u32 revision)
{
	static struct udevice *dev;
	uint16_t buf[5] = {0, };
	char panel_env[64], *panel_str;
	bool found_panel = false;
	int i, ret;

	ret = uclass_get_device_by_name(UCLASS_SENSOR_ID, "sensor_id@36", &dev);
	if (ret < 0) {
		printf("Cannot find sensor_id device\n");
		return;
	}

	ret = sensorid_get_type(dev, &buf[0], 4);
	if (ret < 0) {
		printf("Cannot read sensor type - %d\n", ret);
		return;
	}

	ret = sensorid_get_addon(dev, &buf[4]);
	if (ret < 0) {
		printf("Cannot read add-on board type - %d\n", ret);
		return;
	}

	printf("LCD#1:0x%X, LCD#2:0x%X, CAM#1:0x%X, CAM#2:0x%X\n",
			buf[0], buf[1], buf[2], buf[3]);
	printf("ADD-ON-BOARD : 0x%X\n", buf[4]);

	for (i = 0; i < SENSORID_LCD_MAX; i++) {
		if (buf[i] != SENSORID_LCD_NONE) {
			snprintf(panel_env, sizeof(panel_env), "lcd%d_%d",
				 i + 1, buf[i]);
			panel_str = getenv(panel_env);
			if (panel_str) {
				setenv("lcd_panel", panel_str);
				found_panel = true;
			}
			break;
		}
	}

	if (!found_panel)
		setenv("lcd_panel", "NONE");
}
#endif

#ifdef CONFIG_SET_DFU_ALT_INFO
void set_dfu_alt_info(char *interface, char *devstr)
{
	size_t buf_size = CONFIG_SET_DFU_ALT_BUF_LEN;
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, buf_size);

	snprintf(buf, buf_size, "setenv dfu_alt_info \"%s\"", CONFIG_DFU_ALT);
	run_command(buf, 0);
}
#endif

/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */

static void nx_phy_init(void)
{
#ifdef CONFIG_SENSORID_ARTIK
	/* I2C-GPIO for AVR */
	nx_gpio_set_pad_function(1, 11, 2);
	nx_gpio_set_pad_function(1, 18, 2);
#endif
}

/* call from u-boot */
int board_early_init_f(void)
{
	return 0;
}

int mmc_get_env_dev(void)
{
	int port_num;
	int boot_mode = readl(PHY_BASEADDR_CLKPWR + SYSRSTCONFIG);

	if ((boot_mode & BOOTMODE_MASK) == BOOTMODE_SDMMC) {
		//port_num = readl(PHY_BASEADDR_SRAM + DEVICEBOOTINFO);
		port_num = readl(SCR_ARM_SECOND_BOOT_REG1);
		if (port_num == EMMC_PORT_NUM)
			return 0;
		else if (port_num == SD_PORT_NUM)
			return 1;
	} else if ((boot_mode & BOOTMODE_MASK) == BOOTMODE_USB) {
		return 0;
	}

	return -1;
}

int board_init(void)
{
#ifdef CONFIG_REVISION_TAG
	check_hw_revision();
	printf("HW Revision:\t%d\n", board_rev);
#endif

	nx_phy_init();

	return 0;
}

/* u-boot dram initialize  */
int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	return 0;
}

/* u-boot dram board specific */
void dram_init_banksize(void)
{
	/* set global data memory */
	gd->bd->bi_arch_number = machine_arch_type;
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x00000100;

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size  = CONFIG_SYS_SDRAM_SIZE;
}

#ifdef CONFIG_DM_PMIC_NXE2000
void pmic_init(void)
{
	static struct udevice *dev;
	int ret = -ENODEV;
	uint8_t bit_mask = 0;

	ret = pmic_get("nxe1500@33", &dev);
	if (ret)
		printf("Can't get PMIC: %s!\n", "nxe1500@33");

	bit_mask = 0x00;
	bit_mask = pmic_reg_read(dev, NXE2000_REG_PWRONTIMSET);
	bit_mask &= ~(0x1 << NXE2000_POS_PWRONTIMSET_OFF_JUDGE_PWRON);
	bit_mask |= (0x0 << NXE2000_POS_PWRONTIMSET_OFF_JUDGE_PWRON);
	ret = pmic_write(dev, NXE2000_REG_PWRONTIMSET, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC REG: %d!\n", NXE2000_REG_PWRONTIMSET);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(3 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(0 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC1CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC1CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(3 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(0 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC2CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC2CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC3CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC3CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC4CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC4CTL2);

	bit_mask = ((1 << NXE2000_POS_LDOEN1_LDO1EN) |
			(1 << NXE2000_POS_LDOEN1_LDO2EN) |
			(1 << NXE2000_POS_LDOEN1_LDO3EN) |
			(0 << NXE2000_POS_LDOEN1_LDO4EN) |
			(0 << NXE2000_POS_LDOEN1_LDO5EN));
	ret = pmic_write(dev, NXE2000_REG_LDOEN1, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_LDOEN1);

}
#endif

int board_late_init(void)
{
#ifdef CONFIG_CHECK_BONDING_ID
	int is_dual = check_bonding_id();
	int nr_cpus = 4;

	if (is_dual) {
		nr_cpus = 2;
		setenv_ulong("model_id", 532);
	}

	setenv_ulong("nr_cpus", nr_cpus);
#endif

#ifdef CONFIG_CHECK_BOARD_TYPE
	if (check_board_type() == 1)
		setenv_ulong("model_id", 533);
#endif

#ifdef CONFIG_DM_PMIC_NXE2000
	pmic_init();
#endif

#ifdef CONFIG_REVISION_TAG
	set_board_rev(board_rev);
#endif
#ifdef CONFIG_CMD_FACTORY_INFO
	run_command("run factory_load", 0);
#endif
#ifdef CONFIG_ARTIK_MAC
	generate_mac();
#endif
#ifdef CONFIG_SENSORID_ARTIK
	get_sensorid(board_rev);
#endif
#ifdef CONFIG_ARTIK_OTA
	check_ota_update();
#endif
	return 0;
}

#ifdef CONFIG_USB_GADGET
int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	if (!strcmp(name, "usb_dnl_thor")) {
		put_unaligned(CONFIG_G_DNL_THOR_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_THOR_PRODUCT_NUM, &dev->idProduct);
	} else if (!strcmp(name, "usb_dnl_ums")) {
		put_unaligned(CONFIG_G_DNL_UMS_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_UMS_PRODUCT_NUM, &dev->idProduct);
	} else {
		put_unaligned(CONFIG_G_DNL_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_PRODUCT_NUM, &dev->idProduct);
	}
	return 0;
}
#endif
