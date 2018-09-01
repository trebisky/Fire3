This is a collection of programs I have been writing to run on My NanoPi Fire3 board.
These are bare metal programming or research projects

This board is based on the Samsung s5p6818 chip.

1. Align - experiment with ways to allow unaligned memory access.
1. Arm_wrap - tool for "wrapping" a binary file into an ELF file
1. Blink32 - tiny standalone program to blink onboard LED in A32 mode
1. Blink64 - small standalone program to blink onboard LED in A64 mode
1. BlinkEL - blinks to indicate the EL (Exception Level)
1. Boot1 - (bogus) disassemble the "bl1" boot loader distributed with Linux
1. Boot_NSIH - disassemble a header that jumps from A32 to A64
1. Bootrom - disassemble the s5p6818 on-chip boot loader
1. Hello1 - simple serial IO, loaded by bl1.
1. USB_loader - host side (linux) tool to download code via USB.

For notes and supporting information, see my website:

http://cholla.mmto.org/fire3/
