#
# Copyright 2019 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

ifdef CONFIG_TARGET_IMX8MP_RM02A1_6G
obj-y += imx8mp_rm02a1.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_rm02a1_6G.o
obj-$(CONFIG_IMX8M_DDR4) += ddr4_timing.o
endif
endif

ifdef CONFIG_TARGET_IMX8MP_RM02A1_4G
obj-y += imx8mp_rm02a1.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_rm02a1_4G.o
obj-$(CONFIG_IMX8M_DDR4) += ddr4_timing.o
endif
endif

ifdef CONFIG_TARGET_IMX8MP_RM02A1_2G
obj-y += imx8mp_rm02a1.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_rm02a1_2G.o
obj-$(CONFIG_IMX8M_DDR4) += ddr4_timing.o
endif
endif
