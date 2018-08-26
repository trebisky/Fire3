# Makefile for fire3_loader

# This is a host side utility to send files to
# a s5p6818 board like the NanoPi Fire3 or M3

# tjt 8-25-2018 -- on Fedora 27, I install
# libusb-devel (via "dnf install libusb-devel"
# and this puts libusb.h at /usr/include/libusb.h
# but this clever business with pkg-config will sort
# that out magically.

#USB_JIVE = -I/usr/local/include/libusb-1.0 main.c -L/usr/local/lib -lusb-1.0
USB_JIVE = $$(pkg-config --cflags --libs libusb-1.0)

all:	loader

loader: loader.c
	gcc -Wall -Wextra -Os loader.c $(USB_JIVE) -o loader

test1:
	./loader fire3.img

test2:
	dd if=fire3.img of=fire3_nohd.img skip=4
	./loader -h64 fire3_nohd.img

clean:
	rm -f loader
