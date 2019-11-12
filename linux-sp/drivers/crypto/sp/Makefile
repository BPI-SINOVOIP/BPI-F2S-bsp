
ifneq ($(KERNELRELEASE),)
	obj-$(CONFIG_CRYPTO_DEV_SP) = sp_crypto.o
	sp_crypto-objs := sp-crypto.o \
		sp-aes.o \
		sp-hash.o \
		sp-rsa.o 
else
    KERNELDIR ?= ../../../
    PWD       := $(shell pwd)
 
modules:
	$(MAKE) -C $(KERNELDIR) ARCH=arm make CROSS_COMPILE=arm-xilinx-linux-gnueabi- M=$(PWD)  modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
endif

