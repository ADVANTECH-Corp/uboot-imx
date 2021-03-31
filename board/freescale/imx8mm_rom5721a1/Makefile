#
# Copyright 2018 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

ifdef CONFIG_TARGET_IMX8MM_ROM5721A1_2G
obj-y += imx8mm_rom5721a1.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_rom5721a1_2G.o
endif
endif

ifdef CONFIG_TARGET_IMX8MM_ROM5721A1_1G
obj-y += imx8mm_rom5721a1.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_rom5721a1_1G.o
endif
endif
