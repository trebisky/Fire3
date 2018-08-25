Boot_NSIH

This is part of understanding how the s5p6818 SoC on the NanoPi Fire3 boots.

This is a research and reverse engineering project.
It began with Github project "s5p6818_spl" by Metro94.

He includes a "black box" binary called "tools/build"
along with a 512 byte binary file "tools/nsih.bin"

Running "build" does something to insert those 512 bytes
into the binary file (which leaves a gap for it) so that
it can be downloaded via USB (or placed on an SD card).

Digging into this answers one burning question, which is how
the transition from 32 bit code to 64 bit is managed.
There is a block of instructions in nsih.bin that handles
this.

For more information, see:

    http://cholla.mmto.org/fire3/
