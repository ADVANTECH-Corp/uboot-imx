/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP
 *
 * Configuration settings for the Freescale i.MX6Q SabreSD board.
 */

#ifndef __MX6ADVANTECH_CONFIG_H
#define __MX6ADVANTECH_CONFIG_H

#ifdef CONFIG_SPL
#include "imx6_spl_advantech.h"
#endif

#define CONFIG_MACH_TYPE	3980
#define CONFIG_MXC_UART_BASE	UART1_BASE
#define CONSOLE_DEV		"ttymxc0"
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"  /* SDHC2 */

#if defined(CONFIG_TARGET_MX6QRSB4411A1_512M) || defined(CONFIG_TARGET_MX6QRSB4411A2_512M)
#define PHYS_SDRAM_SIZE         (512u * 1024 * 1024)
#elif defined(CONFIG_TARGET_MX6QRSB4411A1_1G) || defined(CONFIG_TARGET_MX6QRSB4411A2_1G)
#define PHYS_SDRAM_SIZE         (1u * 1024 * 1024 * 1024)
#elif defined(CONFIG_TARGET_MX6QRSB4411A1_2G) || defined(CONFIG_TARGET_MX6QRSB4411A2_2G) || defined(CONFIG_TARGET_MX6DLRSB4411A1_2G)
#define PHYS_SDRAM_SIZE         (2u * 1024 * 1024 * 1024)
#endif

#include "mx6advantech_common.h"

/* Falcon Mode */
#define CONFIG_SPL_FS_LOAD_ARGS_NAME	"args"
#define CONFIG_SPL_FS_LOAD_KERNEL_NAME	"uImage"
#define CONFIG_SYS_SPL_ARGS_ADDR       0x18000000

/* Falcon Mode - MMC support: args@1MB kernel@2MB */
#define CONFIG_SYS_MMCSD_RAW_MODE_ARGS_SECTOR  0x800   /* 1MB */
#define CONFIG_SYS_MMCSD_RAW_MODE_ARGS_SECTORS (CONFIG_CMD_SPL_WRITE_SIZE / 512)
#define CONFIG_SYS_MMCSD_RAW_MODE_KERNEL_SECTOR        0x1000  /* 2MB */

#define CONFIG_SYS_FSL_USDHC_NUM	3

#undef CONFIG_FEC_MXC_PHYADDR
#define CONFIG_FEC_MXC_PHYADDR  0

/*
 * imx6 q/dl/solo pcie would be failed to work properly in kernel, if
 * the pcie module is iniialized/enumerated both in uboot and linux
 * kernel.
 * rootcause:imx6 q/dl/solo pcie don't have the reset mechanism.
 * it is only be RESET by the POR. So, the pcie module only be
 * initialized/enumerated once in one POR.
 * Set to use pcie in kernel defaultly, mask the pcie config here.
 * Remove the mask freely, if the uboot pcie functions, rather than
 * the kernel's, are required.
 */
#ifdef CONFIG_CMD_PCI
#define CONFIG_PCI_SCAN_SHOW
#define CONFIG_PCIE_IMX
#ifndef CONFIG_DM_PCI
#define CONFIG_PCIE_IMX_PERST_GPIO	IMX_GPIO_NR(7, 12)
#define CONFIG_PCIE_IMX_POWER_GPIO	IMX_GPIO_NR(3, 19)
#endif
#endif

/* USB Configs */
#ifdef CONFIG_CMD_USB
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS		0
#define CONFIG_USB_MAX_CONTROLLER_COUNT	1 /* Enabled USB controller number */
#endif

/*#define CONFIG_SPLASH_SCREEN*/
/*#define CONFIG_MXC_EPDC*/

/*
 * SPLASH SCREEN Configs
 */
#ifndef CONFIG_ADVANTECH
#if defined(CONFIG_SPLASH_SCREEN) && defined(CONFIG_MXC_EPDC)
	/*
	 * Framebuffer and LCD
	 */
	#undef LCD_TEST_PATTERN
	/* #define CONFIG_SPLASH_IS_IN_MMC			1 */
	#define LCD_BPP					LCD_MONOCHROME
	/* #define CONFIG_SPLASH_SCREEN_ALIGN		1 */

	#define CONFIG_WAVEFORM_BUF_SIZE		0x400000
#endif /* CONFIG_SPLASH_SCREEN && CONFIG_MXC_EPDC */
#endif

#define CONFIG_SUPPORT_LVDS
#ifdef CONFIG_SUPPORT_LVDS
#define IOMUX_LCD_BKLT_PWM 	MX6_PAD_GPIO_1__GPIO1_IO01
#define IOMUX_LCD_BKLT_EN	MX6_PAD_KEY_COL0__GPIO4_IO06
#define IOMUX_LCD_VDD_EN	MX6_PAD_KEY_ROW0__GPIO4_IO07
#define LCD_BKLT_PWM 		IMX_GPIO_NR(1, 1)
#define LCD_BKLT_EN 		IMX_GPIO_NR(4, 6)
#define LCD_VDD_EN 		IMX_GPIO_NR(4, 7)	
#endif
#define USDHC3_CD_GPIO          IMX_GPIO_NR(7, 1)

#define CONFIG_PCIE_RESET
#define IOMUX_PCIE_RESET	MX6_PAD_CSI0_DAT4__GPIO5_IO22	//CPU_WIFI_RESET
#define PCIE_RESET              IMX_GPIO_NR(5,22)

#define CONFIG_M2_SLOT
#define IOMUX_M2_WLAN_OFF	MX6_PAD_NANDF_D7__GPIO2_IO07	//M.2_WLAN_OFF
#define IOMUX_M2_BT_OFF		MX6_PAD_NANDF_D1__GPIO2_IO01	//M.2_BT_OFF
#define	M2_WLAN_OFF		IMX_GPIO_NR(2, 7)
#define	M2_BT_OFF		IMX_GPIO_NR(2, 1)
#endif                         /* __MX6ADVANTECH_CONFIG_H */
