# SPDX-License-Identifier: GPL-2.0
# Core
obj-$(CONFIG_ARCH_PENTAGRAM) += clk-sunplus.o

# SoC Drivers
ifdef CONFIG_SYS_ENV_ZEBU
obj-y += clk-q628.o
else
obj-$(CONFIG_SYS_ENV_SP7021_EVB) += clk-q628.o
endif
