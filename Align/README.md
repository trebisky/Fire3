BlinkEL

Some bare metal programming for the NanoPi Fire3

Adapted from the BlinkEL project.

This code is booted by bl1 from the SD card.
It runs at 0x40000000 (start of RAM).

It has the start of my own Uart driver, but in truth
it inherits the uart initialization done by bl1.

For more information, see:

    http://cholla.mmto.org/fire3/
