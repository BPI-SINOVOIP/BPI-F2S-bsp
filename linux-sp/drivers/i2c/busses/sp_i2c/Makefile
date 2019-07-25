#
# Makefile for the i2c subsystem
#
# i2c main file: <sp_i2cm0.c> <sp_i2cm1.c>
# - register platform device, platform driver (suspend/resume)
#	- register misc device (ioctl)
# - Should be build as module driver
#
# i2c sub modules:
#  <HAL: hal/hal_i2c.c>
#

obj-y += sp_i2cm.o
obj-y += hal_i2c.o

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

