#
# Copyright 2022 Advantech
#
# SPDX-License-Identifier:      GPL-2.0+
#

ifdef CONFIG_TARGET_IMX8MM_EBCRS20A1_8G
obj-y += imx8mm_ebcrs20a1.o

ifdef CONFIG_XPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_ebcrs20a1_8G.o
endif
endif

ifdef CONFIG_TARGET_IMX8MM_EBCRS20A1_4G
obj-y += imx8mm_ebcrs20a1.o

ifdef CONFIG_XPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_ebcrs20a1_4G.o
endif
endif
