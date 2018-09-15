# Makefile to put U-Boot or and/or bl1 onto an SD card

# Mac address for my board:
# GENERIC ether read: -1
# MAC address:  [d2:a5:61:c9:2e:e4] (tjt)

# DEV = /dev/sdc
DEV = /dev/sdf

all: uboot

# Make a card to do the network boot via U-boot
uboot:
	dd if=bl1-sd.bin of=${DEV}  bs=512 seek=1 conv=fdatasync
	dd if=u-boot.bin of=$(DEV) bs=512 seek=129 conv=fdatasync
	sync

bl1:
	dd if=bl1-usb.bin of=${DEV} bs=512 seek=1 conv=fdatasync

# ------------------------------------

uboot-sd:
	dd if=u-boot.img of=${DEV} conv=fdatasync
	sync

img:
	dd if=${DEV} of=u-boot.img bs=512 count=8192
	sync

# THE END
