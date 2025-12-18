Timer demo

So far this is the only code I have that is network booted by U-boot
(my usual way of playing with boards like this).

Works! as of 12-17-2025

This includes a gic v2 driver (gic400) and the idea is that this gets
booted by U-boot and then handles timer interrupts.

Like other aarch64 boards I have, the code launches at EL2,
 However for this board, the IMO bit in the HCR register is
 set, allowing interrupts at EL2.

For more information, see:

    http://cholla.mmto.org/fire3/
