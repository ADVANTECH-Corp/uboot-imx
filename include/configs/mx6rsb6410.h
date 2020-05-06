/*
 * Copyright (C) 2012-2014 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6Q SabreSD board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6QSABRESD_CONFIG_H
#define __MX6QSABRESD_CONFIG_H

#include <asm/arch/imx-regs.h>
#include <asm/imx-common/gpio.h>

#define CONFIG_MACH_TYPE	3980
#define CONFIG_MXC_UART_BASE	UART1_BASE
#define CONFIG_CONSOLE_DEV		"ttymxc0"
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"  /* SDHC3 */

/* support SATA boot */
#define CONFIG_SATA_BOOT

#include "mx6advantech_common.h"

#ifdef CONFIG_ADVANTECH
#define CONFIG_ROM7420

/* don't use pmic */
#undef CONFIG_LDO_BYPASS_CHECK
#endif

/* USB Configs */
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_STORAGE
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS   0
#define CONFIG_USB_MAX_CONTROLLER_COUNT 1 /* Enabled USB controller number */

#define CONFIG_SYS_FSL_USDHC_NUM	3
#define CONFIG_SYS_MMC_ENV_DEV		1	/* SDHC3 */
#define CONFIG_SYS_MMC_ENV_PART                0       /* user partition */

#ifdef CONFIG_SYS_USE_SPINOR
#ifdef CONFIG_ADVANTECH
#define CONFIG_SF_DEFAULT_CS    1
#else
#define CONFIG_SF_DEFAULT_CS   (0|(IMX_GPIO_NR(4, 9)<<8))
#endif
#endif

#ifdef CONFIG_CMD_SF
	#ifdef CONFIG_ROM7420
		#ifdef CONFIG_SPI_FLASH_CS
			#undef CONFIG_SPI_FLASH_CS
			#define CONFIG_SPI_FLASH_CS	1
		#endif
	#endif
#endif
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
/* #define CONFIG_CMD_PCI */
#ifdef CONFIG_CMD_PCI
#define CONFIG_PCI
#define CONFIG_PCI_PNP
#define CONFIG_PCI_SCAN_SHOW
#define CONFIG_PCIE_IMX
#define CONFIG_PCIE_IMX_PERST_GPIO	IMX_GPIO_NR(7, 12)
#define CONFIG_PCIE_IMX_POWER_GPIO	IMX_GPIO_NR(3, 19)
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
	#define CONFIG_CMD_BMP
	#define CONFIG_LCD
	#define CONFIG_FB_BASE				(CONFIG_SYS_TEXT_BASE + 0x300000)
	#define CONFIG_SYS_CONSOLE_IS_IN_ENV
	#undef LCD_TEST_PATTERN
	/* #define CONFIG_SPLASH_IS_IN_MMC			1 */
	#define LCD_BPP					LCD_MONOCHROME
	/* #define CONFIG_SPLASH_SCREEN_ALIGN		1 */

	#define CONFIG_WORKING_BUF_ADDR			(CONFIG_SYS_TEXT_BASE + 0x100000)
	#define CONFIG_WAVEFORM_BUF_ADDR		(CONFIG_SYS_TEXT_BASE + 0x200000)
	#define CONFIG_WAVEFORM_FILE_OFFSET		0x600000
	#define CONFIG_WAVEFORM_FILE_SIZE		0xF0A00
	#define CONFIG_WAVEFORM_FILE_IN_MMC

#ifdef CONFIG_SPLASH_IS_IN_MMC
	#define CONFIG_SPLASH_IMG_OFFSET		0x4c000
	#define CONFIG_SPLASH_IMG_SIZE			0x19000
#endif
#endif /* CONFIG_SPLASH_SCREEN && CONFIG_MXC_EPDC */
#endif

#define CONFIG_SUPPORT_LVDS
#ifdef CONFIG_SUPPORT_LVDS
#define IOMUX_LCD_BKLT_PWM 	MX6_PAD_SD1_DAT3__GPIO1_IO21
#define IOMUX_LCD_BKLT_EN	MX6_PAD_NANDF_WP_B__GPIO6_IO09
#define IOMUX_LCD_VDD_EN	MX6_PAD_NANDF_CLE__GPIO6_IO07
#define LCD_BKLT_PWM 		IMX_GPIO_NR(1, 21)
#define LCD_BKLT_EN 		IMX_GPIO_NR(6, 9)
#define LCD_VDD_EN 		IMX_GPIO_NR(6, 7)	
#endif

#ifdef CONFIG_ADVANTECH
	#define WD_ENABLE_ADV IMX_GPIO_NR(1,26)
	#define POWER_ENABLE_ADV	IMX_GPIO_NR(1,30)
	#define WIFI_ENABLE_ADV	IMX_GPIO_NR(7,2)

#define IOMUX_PCIE_DIS_B 	MX6Q_PAD_KEY_COL4__GPIO_4_14
#define IOMUX_M2_W_DISABLE	MX6Q_PAD_SD3_CMD__GPIO_7_2
#define PCIE_DIS_B 		IMX_GPIO_NR(4, 14)
#define M2_W_DISABLE 		IMX_GPIO_NR(7, 2)
#endif
#endif                         /* __MX6QSABRESD_CONFIG_H */
