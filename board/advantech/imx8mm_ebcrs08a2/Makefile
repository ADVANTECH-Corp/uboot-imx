#
# Copyright 2022 Advantech
#
# SPDX-License-Identifier:      GPL-2.0+
#

ifdef CONFIG_TARGET_IMX8MM_EBCRS08A2_2G
obj-y += imx8mm_ebcrs08a2.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_ebcrs08a2_2G.o
endif
endif

ifdef CONFIG_TARGET_IMX8MM_EBCRS08A2_1G
obj-y += imx8mm_ebcrs08a2.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_ebcrs08a2_1G.o
endif
endif
