/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 */

#ifndef __IMX8MM_EBCRS20_H
#define __IMX8MM_EBCRS20_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include <env/nxp/imx_env.h>

#define UBOOT_ITB_OFFSET			0x57C00
#define FSPI_CONF_BLOCK_SIZE		0x1000
#define UBOOT_ITB_OFFSET_FSPI  \
	(UBOOT_ITB_OFFSET + FSPI_CONF_BLOCK_SIZE)
#ifdef CONFIG_FSPI_CONF_HEADER
#define CFG_SYS_UBOOT_BASE  \
	(QSPI0_AMBA_BASE + UBOOT_ITB_OFFSET_FSPI)
#else
#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + 0x300 * 512)
#endif

#ifdef CONFIG_XPL_BUILD
/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CFG_MALLOC_F_ADDR		0x930000
/* For RAW image gives a error info not panic */

#endif

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR        0x40000000
#define CFG_SYS_INIT_RAM_SIZE        0x200000

#define CFG_SYS_SDRAM_BASE           0x40000000
#define PHYS_SDRAM                      0x40000000

#if defined(CONFIG_TARGET_IMX8MM_EBCRS20A1_2G)
#define PHYS_SDRAM_SIZE			0x80000000 /* 2GB DDR */
#elif defined(CONFIG_TARGET_IMX8MM_EBCRS20A1_4G)
#define PHYS_SDRAM_SIZE			0xC0000000 /* 3GB DDR */
#define PHYS_SDRAM_2			0x100000000
#define PHYS_SDRAM_2_SIZE		0x40000000 /* 1GB */
#elif defined(CONFIG_TARGET_IMX8MM_EBCRS20A1_8G)
#define PHYS_SDRAM_SIZE			0xC0000000 /* 3GB DDR */
#define PHYS_SDRAM_2			0x100000000
#define PHYS_SDRAM_2_SIZE		0x140000000 /* 5GB */
#endif

#define CFG_FEC_MXC_PHYADDR          0

#define CFG_MXC_UART_BASE		UART_BASE_ADDR(2)

#ifdef CONFIG_TARGET_IMX8MM_DDR4_EVK
#define CFG_SYS_FSL_USDHC_NUM	1
#else
#define CFG_SYS_FSL_USDHC_NUM	2
#endif
#define CFG_SYS_FSL_ESDHC_ADDR	0

#define CFG_SYS_NAND_BASE           0x20000000

#ifdef CONFIG_IMX_MATTER_TRUSTY
#define NS_ARCH_ARM64 1
#endif

#ifdef CONFIG_ANDROID_SUPPORT
#include "imx8mm_evk_android.h"
#endif


#define WDOG_TRIG IMX_GPIO_NR(1, 15)

#define WDOG_ENABLE IMX_GPIO_NR(1, 9)

#ifdef CONFIG_TARGET_IMX8MM_EBCRS20A1_2G
#define LVDS_CORE_EN_PAD IMX_GPIO_NR(2, 11)
#define LVDS_1V8_EN_PAD IMX_GPIO_NR(4, 28)
#define LVDS_STBY_PAD IMX_GPIO_NR(1, 12)
#define LVDS_RESET_PAD IMX_GPIO_NR(1, 14)
#endif

#endif
