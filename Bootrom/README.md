Read out and disassemble the on-chip bootrom in the Samsung S5P6818

This is part of my attempt to understand the details of how my NanoPi Fire3
board boots.

The bootrom is 20K in size at address 0x34000000.
However, I always find it mapped at address 0.
It uses static ram (also on chip) at 0xffff0000.

It runs in 32 bit mode (A32 code) !!

For notes and supporting information, see my website:

http://cholla.mmto.org/fire3/
