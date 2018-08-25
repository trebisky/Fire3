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

See my project Fire3/Spl_blink, which is derived from the s5p6818 by Metro94.
A gap at the start of the file is left with 2048 bytes (0x800) zeros.

Dumping files as hex and then doing various "diff" runs show that what "build"
does is two things:

1. It replaces the first 512 bytes of zeros with the contents of nsih.bin
2. It changes one word as follows:

diff nsih.odx final_nsih.odx
00000040 0000 0000 0000 0000 0000 ffff 0000 ffff
---
00000040 0000 0000 8012 0000 0000 ffff 0000 ffff


For more information, see:

    http://cholla.mmto.org/fire3/
