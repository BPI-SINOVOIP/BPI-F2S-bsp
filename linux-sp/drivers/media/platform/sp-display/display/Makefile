#
## Makefile for the display subsystem
#
SUPPORT_CHECK_CODING_STYLE=0

#check linux kernel coding style start
ifeq ($(SUPPORT_CHECK_CODING_STYLE), 1)
CHECK_SCRIPT_CMD := $(srctree)/scripts/checkpatch.pl -q --ignore NEW_TYPEDEFS -f
CHECK_STYLE = $(shell $(CHECK_SCRIPT_CMD) $(1) 1>/dev/null 2>&1; echo $$?)
$(foreach i, $(filter-out $(wildcard $(src)/*.mod.c), $(wildcard $(src)/*.c $(src)/*.h)), $(if $(filter-out $(call CHECK_STYLE, $i), 0),$(error $(CHECK_SCRIPT_CMD) $(srctree)/$i), ))
#$(foreach i, $(filter-out $(wildcard $(src)/*.mod.c), $(wildcard $(src)/*.c $(src)/*.h)), $(if $(filter-out $(call CHECK_STYLE, $i), 0),$(error coding style error: $i), ))
endif
#check linux kernel coding style end

## the same with ecos
#KBUILD_CFLAGS := $(filter-out -Wno-pointer-sign, $(KBUILD_CFLAGS))
#KBUILD_CFLAGS := $(filter-out -fno-strict-aliasing, $(KBUILD_CFLAGS))
## C99 have 9 trigraphs, but we don't use.
#KBUILD_CFLAGS := $(filter-out -Wno-trigraphs, $(KBUILD_CFLAGS))
## coverity issue
#KBUILD_CFLAGS := $(filter-out -Wno-unused-but-set-variable, $(KBUILD_CFLAGS))
## ex: char buf[10]={0}; printk(buf);
#KBUILD_CFLAGS := $(filter-out -Wno-format-security, $(KBUILD_CFLAGS))

ccflags-y += -Werror
ccflags-y += -Wformat-security

obj-$(CONFIG_VIDEO_SP_DISP) += display_sp7021b.o
display_sp7021b-objs := display.o disp_dve.o disp_dmix.o disp_tgen.o disp_osd.o disp_vpp.o

