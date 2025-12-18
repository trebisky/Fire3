Timer demo

So far this is the only code I have that is network booted by U-boot
(my usual way of playing with boards like this).

NOT yet working as of 12-2025

This includes a gic v2 driver (gic400) and the idea is that this gets
booted by U-boot and then handles timer interrupts.

For more information, see:

    http://cholla.mmto.org/fire3/
