# SPDX-License-Identifier: GPL-2.0+
#
# Advantech i.MX vendor-common objects. Sourced from NXP's board/nxp/common
# (6.18) since the AOM-5521 board reuses NXP's tcpc / mmc-env helpers.

obj-y			+= mmc.o

# tcpc.o only in the full U-Boot build; SPL (CONFIG_XPL_BUILD) uses the inline
# stubs in tcpc.h, so compiling tcpc.c there would redefine those functions.
ifndef CONFIG_XPL_BUILD
obj-$(CONFIG_USB_TCPC)	+= tcpc.o
endif
