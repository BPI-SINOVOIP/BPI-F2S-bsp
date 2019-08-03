#!/bin/sh

# make partition table by fdisk command
# reserve part for fex binaries download 0~204799
# partition1 /dev/sdc1 vfat 204800~327679
# partition2 /dev/sdc2 ext4 327680~end

die() {
        echo "$*" >&2
        exit 1
}

[ $# -eq 1 ] || die "Usage: $0 /dev/sdc"

[ -s "./env.sh" ] || die "please run ./configure first."

. ./env.sh

O=$1

UBOOT=$TOPDIR/u-boot-sp/u-boot.img

echo "Banana Pi BPI-F2S support FAT32 bootfile /ISPBOOOT.BIN (xboot) & u-boot.img for SD/USB boot"
echo "Banana Pi BPI-F2S emmc boot with mmcblk1boot0 (xboot) & GPT or emmc load uboot@blk=0x00000022"

sudo dd if=$UBOOT 	of=$O bs=512 seek=34

