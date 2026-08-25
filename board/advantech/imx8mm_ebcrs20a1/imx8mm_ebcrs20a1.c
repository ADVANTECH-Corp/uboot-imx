// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 NXP
 */
// #include <common.h>
#include <efi_loader.h>
#include <env.h>
#include <init.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/global_data.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <i2c.h>
#include <asm/io.h>
#include "../common/tcpc.h"
#include <usb.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MM_PAD_SAI3_TXFS_UART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_SAI3_TXC_UART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
struct efi_fw_image fw_images[] = {
	{
		.image_type_id = IMX_BOOT_IMAGE_GUID,
		.fw_name = u"IMX8MM-EVK-RAW",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "mmc 2=flash-bin raw 0x42 0x2000 mmcpart 1",
	.images = fw_images,
};

u8 num_image_type_guids = ARRAY_SIZE(fw_images);
#endif /* EFI_HAVE_CAPSULE_SUPPORT */

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	init_uart_clk(1);

	return 0;
}

static iomux_v3_cfg_t const lvds_pads[] = {
	IMX8MM_PAD_SAI3_RXFS_GPIO4_IO28 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_SD1_STROBE_GPIO2_IO11 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_GPIO1_IO12_GPIO1_IO12 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_GPIO1_IO14_GPIO1_IO14 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_lvds(void)
{
//#ifdef CONFIG_TARGET_IMX8MM_RSB3730A2_2G
//	imx_iomux_v3_setup_multiple_pads(lvds_pads, ARRAY_SIZE(lvds_pads));
//#endif
#ifdef LVDS_1V8_EN_PAD
	gpio_request(LVDS_1V8_EN_PAD, "LVDS_1V8_EN_PAD");
	gpio_direction_output(LVDS_1V8_EN_PAD, 1);
#endif
#ifdef LVDS_CORE_EN_PAD
	gpio_request(LVDS_CORE_EN_PAD, "LVDS_CORE_EN_PAD");
	gpio_direction_output(LVDS_CORE_EN_PAD, 1);
#endif
#ifdef LVDS_STBY_PAD
	gpio_request(LVDS_STBY_PAD, "LVDS_STBY");
	gpio_direction_output(LVDS_STBY_PAD, 1);
	udelay(100);	//for lvds bridge init sequence
#endif
#ifdef LVDS_RESET_PAD
	gpio_request(LVDS_RESET_PAD, "LVDS_RESET");
	gpio_direction_output(LVDS_RESET_PAD, 1);
#endif
}

static void setup_iomux_wdt(void)
{
#ifdef WDOG_ENABLE
	imx_iomux_v3_setup_pad(IMX8MM_PAD_GPIO1_IO15_GPIO1_IO15| MUX_PAD_CTRL(NO_PAD_CTRL));
	imx_iomux_v3_setup_pad(IMX8MM_PAD_GPIO1_IO09_GPIO1_IO9| MUX_PAD_CTRL(NO_PAD_CTRL));
	gpio_request(WDOG_ENABLE, "wdt_en");
	gpio_direction_output(WDOG_ENABLE,0);
	gpio_request(WDOG_TRIG, "wdt_trig");
	gpio_direction_output(WDOG_TRIG,1);
#endif
}

#if IS_ENABLED(CONFIG_FEC_MXC)
static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1], 0x2000, 0);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

#ifndef CONFIG_DM_ETH
	/* enable rgmii rxc skew and phy mode select to RGMII copper */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);
#endif

	return 0;
}
#endif

#ifdef CONFIG_USB_TCPC

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	debug("board_usb_init %d, type %d\n", index, init);

	imx8m_usb_power(index, true);

	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	debug("board_usb_cleanup %d, type %d\n", index, init);

	imx8m_usb_power(index, false);
	return ret;
}

int board_ehci_usb_phy_mode(struct udevice *dev)
{
	return USB_INIT_DEVICE;
}

#endif

int board_init(void)
{
#ifdef CONFIG_USB_TCPC
//	setup_typec();
#endif

	if (IS_ENABLED(CONFIG_FEC_MXC))
		setup_fec();

	setup_iomux_lvds();
	setup_iomux_wdt();	
	return 0;
}

int board_late_init(void)
{
	int id;
	char *display;
	char fdt[100]={0};

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	if (IS_ENABLED(CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG)) {
		env_set("board_name", "EVK");
		env_set("board_rev", "iMX8MM");
	}

#ifdef BOARD_ID1
	gpio_request(BOARD_ID3, "BOARD_ID3");
	gpio_request(BOARD_ID2, "BOARD_ID2");
	gpio_request(BOARD_ID1, "BOARD_ID1");
	gpio_direction_input(BOARD_ID3);
	gpio_direction_input(BOARD_ID2);
	gpio_direction_input(BOARD_ID1);
	id = gpio_get_value(BOARD_ID3);
	id = (id<<1)|gpio_get_value(BOARD_ID2);
	id = (id<<1)|gpio_get_value(BOARD_ID1);
	printf("HW BOARD ID:%d\n",id);
	display = env_get("fdtfile");
	if(id == 0) {
		if (!strstr(display, "-dsi2lvds-")) {
			sprintf(fdt, "%s%s", CONFIG_OF_LIST, "-dsi2lvds-1280x800.dtb");
			env_set("fdtfile", fdt);
		}
	}
#endif

	return 0;
}


#ifdef CONFIG_ANDROID_SUPPORT
bool is_power_key_pressed(void) {
	return (bool)(!!(readl(SNVS_HPSR) & (0x1 << 6)));
}
#endif

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0; /* TODO */
}
#endif /* CONFIG_ANDROID_RECOVERY */
#endif /* CONFIG_FSL_FASTBOOT */
