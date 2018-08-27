Spl_blink

A first bit of bare metal programming for the NanoPi Fire3

Adapted from the Github project "s5p6818_spl" by Metro94

This builds a little standalone executable that can be run at the
lowest level possible via the in chip bootloader.
It runs at 0xffff0000 in the on chip static ram that is
used for the second stage boot loader by the usual boot scheme.

Use "make" to build the image, then either:

1. make usb -- to send it to a Fire3 without an SD card in place
2. make sdcard - to put it onto an SD card that can then be "booted"

For more information, see:

    http://cholla.mmto.org/fire3/
