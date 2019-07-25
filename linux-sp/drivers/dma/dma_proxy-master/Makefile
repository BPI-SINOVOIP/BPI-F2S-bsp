obj-m += dma_proxy.o

#set KDIR to kernel source root
#set BUILD_DIR to desired build directory

all:
	$(MAKE) O=$(BUILD_DIR) -C $(KDIR) M=$(shell pwd) modules

clean:
	$(MAKE) O=$(BUILD_DIR) -C $(KDIR) M=$(shell pwd) clean
