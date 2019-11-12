#
## Makefile for the sp7021-pwm
#
SUPPORT_CHECK_CODING_STYLE=0

ccflags-y += -Werror

#check linux kernel coding style start
ifeq ($(SUPPORT_CHECK_CODING_STYLE), 1)
CHECK_IGNORE_FILES = $(src)/pwm_sp7021.h
CHECK_SCRIPT_CMD := $(srctree)/scripts/checkpatch.pl -q -f
CHECK_STYLE = $(shell $(CHECK_SCRIPT_CMD) $(1) 1>/dev/null 2>&1; echo $$?)
#$(foreach i, $(filter-out $(wildcard $(src)/*.mod.c) $(CHECK_IGNORE_FILES), $(wildcard $(src)/*.c $(src)/*.h)), $(if $(filter-out $(call CHECK_STYLE, $i), 0),$(error $(CHECK_SCRIPT_CMD) $(srctree)/$i), ))
$(foreach i, $(filter-out $(wildcard $(src)/*.mod.c) $(CHECK_IGNORE_FILES), $(wildcard $(src)/*.c $(src)/*.h)), $(if $(filter-out $(call CHECK_STYLE, $i), 0),$(error coding style error: $i), ))
endif
#check linux kernel coding style end

## the same with ecos
#KBUILD_CFLAGS := $(filter-out -Wno-pointer-sign, $(KBUILD_CFLAGS))
KBUILD_CFLAGS := $(filter-out -fno-strict-aliasing, $(KBUILD_CFLAGS))
## C99 have 9 trigraphs, but we don't use.
KBUILD_CFLAGS := $(filter-out -Wno-trigraphs, $(KBUILD_CFLAGS))
## coverity issue
KBUILD_CFLAGS := $(filter-out -Wno-unused-but-set-variable, $(KBUILD_CFLAGS))
## ex: char buf[10]={0}; printk(buf);
KBUILD_CFLAGS := $(filter-out -Wno-format-security, $(KBUILD_CFLAGS))


obj-$(CONFIG_PWM_SP7021)	+= pwm_sp7021.o

