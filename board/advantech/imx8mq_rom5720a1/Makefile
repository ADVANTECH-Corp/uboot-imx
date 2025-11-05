#
# Copyright 2017 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

ifdef CONFIG_TARGET_IMX8MQ_ROM5720A1_2G
obj-y += imx8mq_rom5720a1.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_rom5720a1_2G.o lpddr4_timing_b0.o
endif
endif
