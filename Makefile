.PHONY: all clean help
.PHONY: u-boot kernel kernel-config
.PHONY: linux pack

include chosen_board.mk

SUDO=sudo
#CROSS_COMPILE?=arm-linux-gnueabihf-
CROSS_COMPILE=$(COMPILE_TOOL)/arm-linux-gnueabihf-
U_CROSS_COMPILE=$(CROSS_COMPILE)
K_CROSS_COMPILE=$(CROSS_COMPILE)

OUTPUT_DIR=$(CURDIR)/linux-sp/output
TARGET_KDIR=$(CURDIR)
RTKDIR=$(TOPDIR)/phoenix/system/src/drivers

U_O_PATH=u-boot-sp
K_O_PATH=linux-sp
U_CONFIG_H=$(U_O_PATH)/include/config.h
K_DOT_CONFIG=$(K_O_PATH)/.config

ROOTFS=$(CURDIR)/rootfs/linux/default_linux_rootfs.tar.gz

Q=
J=$(shell expr `grep ^processor /proc/cpuinfo  | wc -l` \* 2)

all: bsp

clean: u-boot-clean kernel-clean
	rm -f chosen_board.mk env.sh

pack: 
	$(Q)scripts/mk_pack.sh

# u-boot
$(U_CONFIG_H): u-boot-sp
	$(Q)$(MAKE) -C u-boot-sp $(UBOOT_CONFIG) CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J

u-boot: $(U_CONFIG_H)
	$(Q)$(MAKE) -C u-boot-sp all CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J

u-boot-clean:
	$(Q)$(MAKE) -C u-boot-sp CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J distclean

$(K_DOT_CONFIG): linux-sp
	$(Q)$(MAKE) -C linux-sp ARCH=arm $(KERNEL_CONFIG)

kernel: $(K_DOT_CONFIG)
#	$(Q)$(MAKE) -C linux-sp ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output UIMAGE_LOADADDR=0x40008000 uImage dtbs
	$(Q)$(MAKE) -C linux-sp ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output uImage dtbs
	$(Q)$(MAKE) -C linux-sp ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules
	$(Q)$(MAKE) -C linux-sp ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules_install
	mkdir $(OUTPUT_DIR)/lib/modules/4.19.37-BPI-F2S-Kernel/kernel/extra
#	$(Q)$(MAKE) -C phoenix/system/src/drivers ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} TARGET_KDIR=$(TARGET_KDIR) -j$J INSTALL_MOD_PATH=output
#	$(Q)$(MAKE) -C phoenix/system/src/drivers ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} TARGET_KDIR=$(TARGET_KDIR) -j$J INSTALL_MOD_PATH=output install
#	$(Q)$(MAKE) -C linux-sp ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output _depmod
#	$(Q)$(MAKE) -C linux-sp ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J headers_install

kernel-clean:
	$(Q)$(MAKE) -C phoenix/system/src/drivers ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} TARGET_KDIR=$(TARGET_KDIR) -j$J INSTALL_MOD_PATH=output clean
	$(Q)$(MAKE) -C linux-sp ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J distclean
	rm -rf linux-sp/output/

kernel-config: $(K_DOT_CONFIG)
	$(Q)$(MAKE) -C linux-sp ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J menuconfig
	cp linux-sp/.config linux-sp/arch/arm/configs/$(KERNEL_CONFIG)

bsp: u-boot kernel

help:
	@echo ""
	@echo "Usage:"
	@echo "  make bsp             - Default 'make'"
	@echo "  make pack            - pack the images and rootfs to a PhenixCard download image."
	@echo "  make clean"
	@echo ""
	@echo "Optional targets:"
	@echo "  make kernel           - Builds linux kernel"
	@echo "  make kernel-config    - Menuconfig"
	@echo "  make u-boot          - Builds u-boot"
	@echo ""

