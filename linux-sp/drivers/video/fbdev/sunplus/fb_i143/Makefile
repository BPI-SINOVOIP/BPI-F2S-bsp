#
# Makefile for the I143 framebuffer driver
#
ccflags-y += -Werror
ccflags-y += -Wformat-security

obj-$(CONFIG_FB_I143) += fb_i143.o
fb_i143-objs := fb_i143_main.o

ifdef CONFIG_FB_I143_DEBUG
fb_i143-objs += fb_i143_debug.o
endif

