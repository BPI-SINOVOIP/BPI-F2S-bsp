#!/bin/bash

opt=$1

export PATH="../../crossgcc/arm-linux-gnueabihf/bin/:$PATH"
export CROSS_COMPILE=arm-linux-gnueabihf-

# Load CFG=xxx if exists
BUILD_CFG=build.cfg
if [ -f $BUILD_CFG ];then
	. $BUILD_CFG
fi

cfg_num=$1
if [ "$CFG" = "" ];then
	cfg_num=0
fi

if [ "$cfg_num" != "" ];then
	# Select CFG
	if [ "$cfg_num" = "0" ];then
		cfg_num=
		echo "* Select config:"
		echo "--------------------"
		echo " [1] sc7021 achip emu (rootfs)"
		echo " [2] sc7021 bchip emu (rootfs)"
		echo " [3] 8388 achip emu"
		echo " [4] 8388 bchip "
		echo " [5] sc7021 achip emu (initramfs)"
		echo " [6] sc7021 bchip emu (initramfs)"
		echo -n " -> "
		read cfg_num
	else
		cfg_num=$1
	fi

	case "$cfg_num" in
		1)
			CFG=pentagram_sc7021_achip_emu
			;;
		2)
			CFG=pentagram_sc7021_bchip_emu
			;;
		3)
			CFG=pentagram_8388_achip_emu
			;;
		4)
			CFG=pentagram_8388_bchip
			;;
		5)
			CFG=pentagram_sc7021_achip_emu_initramfs
			;;
		6)
			CFG=pentagram_sc7021_bchip_emu_initramfs
			;;
		*)
			echo "Error: bad number!!"
			exit 1
	esac

	echo "configure to ${CFG}"
	make ${CFG}_defconfig && echo "CFG=${CFG}" > $BUILD_CFG
fi

# CFG is selected now !

DTB=`echo $CFG | sed 's/_/-/g'`.dtb

make -j4 uImage V=0 || exit 1

if [ $? -eq 0 ];then
	echo "Build dtb..."
	make $DTB || exit 1
	if [ ! -f arch/arm/boot/dts/$DTB ];then
		echo "DTB output not found: $DTB"
		exit 1
	fi

	# Make a link for upper build layer
	rm -f dtb
	ln -s arch/arm/boot/dts/$DTB dtb

	# append to zImage
	#cat arch/arm/boot/zImage arch/arm/boot/dts/$DTB >> arch/arm/boot/zImage

	[ -d ../../ipack/ ] && make -C ../../ipack/
fi
