#
# Makefile for the mipi i2c subsystem
#
# i2c main file: <sp_mipi_i2c.c>
# - register platform device, platform driver (suspend/resume)
#	- register misc device (ioctl)
# - Should be build as module driver
#
# i2c sub modules:
#  <HAL: hal/hal_mipi_i2c.c>
#

obj-y += sp_mipi_i2c.o
obj-y += hal_mipi_i2c.o

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

