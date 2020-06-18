#
# Makefile for the SP7021 framebuffer driver
#
SUPPORT_CHECK_CODING_STYLE=0

#check linux kernel coding style start
ifeq ($(SUPPORT_CHECK_CODING_STYLE), 1)
CHECK_SCRIPT_CMD := $(srctree)/scripts/checkpatch.pl -q -f
CHECK_STYLE = $(shell $(CHECK_SCRIPT_CMD) $(1) 1>/dev/null 2>&1; echo $$?)
#$(foreach i, $(filter-out $(wildcard $(src)/*.mod.c), $(wildcard $(src)/*.c $(src)/*.h)), $(if $(filter-out $(call CHECK_STYLE, $i), 0),$(error $(CHECK_SCRIPT_CMD) $(srctree)/$i), ))
$(foreach i, $(filter-out $(wildcard $(src)/*.mod.c), $(wildcard $(src)/*.c $(src)/*.h)), $(if $(filter-out $(call CHECK_STYLE, $i), 0),$(error coding style error: $i), ))
endif
#check linux kernel coding style end

ccflags-y += -Werror
ccflags-y += -Wformat-security

obj-$(CONFIG_FB_SP7021) += fb_sp7021.o
fb_sp7021-objs := fb_sp7021_main.o

ifdef CONFIG_FB_SP7021_DEBUG
	FONT_LIB = libFontAPI.lib
	fb_sp7021-objs += fb_sp7021_debug.o
	fb_sp7021-objs += $(FONT_LIB)
endif

