Blink32

A first bit of bare metal programming for the NanoPi Fire3
This is a trimmed down version of Blink64 that runs in A32
mode, so is smaller and does not need the A32 to A64
transition code.  It is adapted from my Blink64 project.

It runs at 0xffff0000 in the on chip static ram.

Use "make" to build the image, then either:

1. make usb -- to send it to a Fire3 without an SD card in place
2. make sdcard - to put it onto an SD card that can then be "booted"

This uses my USB_loader to put a proper header on the image and
either download it by USB or put it on a flash card.

For more information, see:

    http://cholla.mmto.org/fire3/
