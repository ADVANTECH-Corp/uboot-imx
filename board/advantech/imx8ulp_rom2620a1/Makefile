# SPDX-License-Identifier: GPL-2.0+

ifdef CONFIG_TARGET_IMX8ULP_ROM2620A1_2G
obj-y	+= imx8ulp_rom2620a1.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_TARGET_IMX8ULP_ROM2620A1_2G) += lpddr4_timing_rom2620a1_2G.o
endif
endif

ifdef CONFIG_TARGET_IMX8ULP_ROM2620A1_1G
obj-y   += imx8ulp_rom2620a1.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_TARGET_IMX8ULP_ROM2620A1_1G) += lpddr4_timing_rom2620a1_1G.o
endif
endif
