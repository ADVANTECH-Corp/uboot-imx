/*
 * Copyright (C) 2012-2016 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6Q SabreSD board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6QSABRESD_CONFIG_H
#define __MX6QSABRESD_CONFIG_H

#ifdef CONFIG_SPL
#define CONFIG_SATA_BOOT
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_LIBDISK_SUPPORT
#define CONFIG_SPL_MMC_SUPPORT
#include "imx6_spl_advantech.h"
#endif

#define CONFIG_MACH_TYPE	3980
#define CONFIG_MXC_UART_BASE	UART1_BASE
#define CONFIG_CONSOLE_DEV		"ttymxc0"
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"  /* SDHC3 */
/* support SATA boot */
#define CONFIG_SATA_BOOT
#define CONFIG_SATA_GEN2	0x05918732

#if defined(CONFIG_TARGET_MX6ROM3420A1_512M)
#define PHYS_SDRAM_SIZE         (512u * 1024 * 1024)
#elif defined(CONFIG_TARGET_MX6ROM3420A1_1G)
#define PHYS_SDRAM_SIZE         (1u * 1024 * 1024 * 1024)
#elif defined(CONFIG_TARGET_MX6ROM3420A1_2G)
#define PHYS_SDRAM_SIZE         (2u * 1024 * 1024 * 1024)
#endif
	
#if defined(CONFIG_MX6QP)
#define CONFIG_DEFAULT_FDT_FILE	"imx6qp-rom3420-a1.dtb"
#elif defined(CONFIG_MX6Q)
#define CONFIG_DEFAULT_FDT_FILE	"imx6q-rom3420-a1.dtb"
#elif defined(CONFIG_MX6DL)
#define CONFIG_DEFAULT_FDT_FILE	"imx6dl-rom3420-a1.dtb"
#elif defined(CONFIG_MX6SOLO)
#define CONFIG_DEFAULT_FDT_FILE	"imx6dl-rom3420-a1.dtb"
#endif

#include "mx6advantech_common.h"
/* don't use pmic */
#undef CONFIG_LDO_BYPASS_CHECK

/* support SPI boot */
#define CONFIG_SPI_BOOT
#define CONFIG_SPL_SPI_BUS	0
#define CONFIG_SPL_SPI_CS	0
#define IOMUX_SPI5_SCLK         MX6_PAD_SD1_CLK__ECSPI5_SCLK
#define IOMUX_SPI5_MISO         MX6_PAD_SD1_DAT0__ECSPI5_MISO
#define IOMUX_SPI5_MOSI         MX6_PAD_SD1_CMD__ECSPI5_MOSI
#define IOMUX_SPI5_CS0          MX6_PAD_SD1_DAT1__ECSPI5_SS0
#define IOMUX_SPI5_CS1          MX6_PAD_SD1_DAT2__ECSPI5_SS1
#define IOMUX_SPI5_CS2          MX6_PAD_SD1_DAT3__ECSPI5_SS2

#ifdef CONFIG_SPL_BUILD
#define CONFIG_BOOT_SELECT
#define CONFIG_SPL_BOOT_DELAY	500
#endif

#define CONFIG_SYS_FSL_USDHC_NUM	3
#define CONFIG_SYS_MMC_ENV_DEV		1	/* SDHC3 */
#define CONFIG_SYS_MMC_ENV_PART                0       /* user partition */

#ifdef CONFIG_SYS_USE_SPINOR
#define CONFIG_SF_DEFAULT_CS   0
#endif
#undef CONFIG_SF_DEFAULT_BUS
#define CONFIG_SF_DEFAULT_BUS  4

#undef CONFIG_FEC_MXC_PHYADDR
#define CONFIG_FEC_MXC_PHYADDR	0

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

/* USB Configs */
#define CONFIG_CMD_USB
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_STORAGE
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
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
	#define CONFIG_CMD_BMP
	#define CONFIG_LCD
	#define CONFIG_SYS_CONSOLE_IS_IN_ENV
	#undef LCD_TEST_PATTERN
	/* #define CONFIG_SPLASH_IS_IN_MMC			1 */
	#define LCD_BPP					LCD_MONOCHROME
	/* #define CONFIG_SPLASH_SCREEN_ALIGN		1 */

	#define CONFIG_WAVEFORM_BUF_SIZE		0x200000
#endif /* CONFIG_SPLASH_SCREEN && CONFIG_MXC_EPDC */
#endif

/* uncomment for SECURE mode support */
/* #define CONFIG_SECURE_BOOT */

#ifdef CONFIG_SECURE_BOOT
#ifndef CONFIG_CSF_SIZE
#define CONFIG_CSF_SIZE 0x4000
#endif
#endif

/* #define CONFIG_MFG_IGNORE_CHECK_SECURE_BOOT */

#define CONFIG_SUPPORT_LVDS
#ifdef CONFIG_SUPPORT_LVDS
#define IOMUX_LCD_BKLT_PWM 	MX6_PAD_GPIO_9__GPIO1_IO09
#define IOMUX_LCD_BKLT_EN	MX6_PAD_CSI0_VSYNC__GPIO5_IO21
#define IOMUX_LCD_VDD_EN	MX6_PAD_CSI0_DATA_EN__GPIO5_IO20
#define LCD_BKLT_PWM 		IMX_GPIO_NR(1, 9)
#define LCD_BKLT_EN 		IMX_GPIO_NR(5, 21)
#define LCD_VDD_EN 		IMX_GPIO_NR(5, 20)	
#endif

#define SPI1_CS0                IMX_GPIO_NR(4,9)
#define SPI1_CS1                IMX_GPIO_NR(4,10)
#define IOMUX_SPI_SCLK          MX6_PAD_KEY_COL0__ECSPI1_SCLK
#define IOMUX_SPI_MISO          MX6_PAD_KEY_COL1__ECSPI1_MISO
#define IOMUX_SPI_MOSI          MX6_PAD_KEY_ROW0__ECSPI1_MOSI
#define IOMUX_SPI_CS0           MX6_PAD_KEY_ROW1__ECSPI1_SS0
#define IOMUX_SPI_CS1           MX6_PAD_KEY_COL2__ENET_RX_DATA2

#define USDHC2_CD_GPIO          IMX_GPIO_NR(2, 2)

#define CONFIG_PCIE_RESET
#define IOMUX_PCIE_RESET        MX6_PAD_CSI0_PIXCLK__GPIO5_IO18    //PCIE_A_RST
#define PCIE_RESET              IMX_GPIO_NR(5,18)

#endif                         /* __MX6QSABRESD_CONFIG_H */
