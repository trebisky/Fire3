Boot_NSIH

This is part of understanding how the s5p6818 SoC on the NanoPi Fire3 boots.

The most interesting thing here is probably nsih.txt,
which is my annotated disassembly of nsih.bin.
All the rest are tools that led to this.

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
2. It changes one word as follows.

diff nsih.odx final_nsih.odx
1. 00000040 0000 0000 0000 0000 0000 ffff 0000 ffff
2. 00000040 0000 0000 8012 0000 0000 ffff 0000 ffff

A lucky guess would be that this is the number of bytes following the
512 byte header.  The file I am experimenting with has 5248 bytes.
5248 - 512 = 4736 and 4736 is 0x1280.  The ARM is little endian,
so this is either stored as a 16 byte thing at 0x44 or a 32 bit thing
at 0x44.  This is indeed true.

As a bonus, you get my ruby script "odx" that does hex dumping in a way I like.

For more information, see:

    http://cholla.mmto.org/fire3/
