#
# Copyright 2024 Advantech
#
# SPDX-License-Identifier:      GPL-2.0+
#
ifdef CONFIG_TARGET_IMX93_AFEE420A1
obj-y += imx93_afee420a1.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX93_AFEE420A1_LPDDR4X) += lpddr4x_timing_afee420a1.o lpddr4x_timing_1866mts.o
endif
endif
