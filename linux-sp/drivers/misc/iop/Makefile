#
# Makefile for the IOP module drivers.
#

	# call from kernel build system

	obj-$(CONFIG_SUNPLUS_IOP) += sp_iop.o
	obj-$(CONFIG_SUNPLUS_IOP) += hal_iop.o
	obj-$(CONFIG_SUNPLUS_IOP) += iopnormal.o
	obj-$(CONFIG_SUNPLUS_IOP) += iopstandby.o
	#obj-y += sp_iop.o
	#Please remember  ENABLE_QTEST_CBDMA in the file(linux\kernel\arch\arm\boot\dts\pentagram-sc7021-bchip.dtsi)when you enable qtest
		#obj-y				+= qtest/

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions