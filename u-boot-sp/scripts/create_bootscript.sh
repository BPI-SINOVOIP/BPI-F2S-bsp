#<! please execute this script in u-boot root !>
#$1 : boot mode : emmc | nand | romter | zmem
#output : ./u-boot.scr.img
#output : ./u-boot.scr.txt

# NOTICE: sync these addresses with layout in ipack/update_all.sh
romter_kern_src=0x98600000 # 6M in the firmware file
romter_kern_dst=0x307FC0   # header in 0x307FC0 and stext in 0x308000
romter_dtb_src=0x98020000  # 2M in the firmware file
romter_dtb_dst=0x2FFFC0    # header in 0x2FFFC0 and dtb in 0x300000

kernel_img_path=../../ipack/bin/uImage
dtb_img_path=../../ipack/bin/dtb.img

scr_img=./u-boot.scr.img
scr_txt=./u-boot.scr.txt

#cp.l 0x98600000 0x307FC0 0x280000 ; \
#cp.l 0x98020000 0x2FFFC0 0x400 ; \
#sp_go 0x308000 0x300000"


usage()
{
	echo ""
	echo "Usage :"
	echo "\$1 : emmc | nand | romter | zmem"
	echo "output ./u-boot.scr.img"
	echo "output ./u-boot.scr.txt"
	echo ""
}

# parse size infomation in mkimage header
# $1 path to image with header
# output : update "image_size"
image_size="00000000"
parse_image_size()
{
	magic="00000000"
	magic=`hexdump -v -e '/1 "%02X"' $1 -s 0 -n 4`
	if ! [ $magic = "27051956" ]; then
		echo "magic is invalid $magic"
		exit 1
	else
		echo "magic is OK $magic for $1"
	fi

	# man hexdump for detail
	image_size=`hexdump -v -e '/1 "%02X"' $1 -s 12 -n 4`
	#echo "hex "$image_size

	# convert to dec and then divide 4 for cp.l
	dec_image_size=$((16#$image_size))
	#echo "dec "$dec_image_size
	dec_image_size=$((dec_image_size / 4))
	#echo "/4 "$dec_image_size
	image_size=`printf '%x\n' $dec_image_size`
	#echo "hex/4 "$image_size
}

# use mkimage to create u-boot boot script
# $1 boot mode : emmc | nand | romter | zmem
mkimage_uboot_script()
{
	rm $scr_img $scr_txt

	if [ ! -z $1 ] && [ $1 = "romter" ]; then
		parse_image_size $kernel_img_path
		echo "cp.l $romter_kern_src $romter_kern_dst 0x$image_size ;" >> $scr_txt
		parse_image_size $dtb_img_path
		echo "cp.l $romter_dtb_src $romter_dtb_dst 0x$image_size ;" >> $scr_txt
	fi
	echo "sp_go 0x308000 0x300000" >> $scr_txt
	
	mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n "u-boot booting script" -d $scr_txt $scr_img
}

if [ -z $1 ] ; then
	usage
	exit 1
fi

mkimage_uboot_script $1
