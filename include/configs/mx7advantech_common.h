/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX7.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
#ifndef __MX7_COMMON_H
#define __MX7_COMMON_H
*/

#ifndef __MX7D_SABRESD_COMMON_CONFIG_H
#define __MX7D_SABRESD_COMMON_CONFIG_H

#include "mx7_common.h"

/* #define CONFIG_ADVANTECH */
#define CONFIG_ADVANTECH_MX7

#define CONFIG_PHYLIB
#define CONFIG_PHY_ATHEROS


/* boot device dev number */
#define	CONFIG_EMMC_DEV_NUM	1
#define	CONFIG_SD_DEV_NUM	0

#endif
