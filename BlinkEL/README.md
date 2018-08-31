BlinkEL

Some bare metal programming for the NanoPi Fire3

Adapted from by Blink64 project.

This builds a little standalone executable that is run directly
from the on-chip boot loader.
It blinks a count that indicates the EL.
I get 3 blinks.

Use "make" to build the image, then either:

1. make usb -- to send it to a Fire3 without an SD card in place

This uses my USB_loader to install a header and either download
it via USB or place it on a flash card.

For more information, see:

    http://cholla.mmto.org/fire3/
