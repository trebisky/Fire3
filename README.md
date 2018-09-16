This is a collection of programs I have been writing to run on My NanoPi Fire3 board.
These are bare metal programming or research projects
September 2018

This board is based on the Samsung s5p6818 chip.

First some tools to do bare metal programming

1. SDcard - binary images of bl1 and u-boot for bare metal work.
1. USB_loader - host side (linux) tool to download code via USB.
1. U-Boot - U-Boot hacked to do network booting.

And some reverse engineering

1. Bootrom - disassemble the s5p6818 on-chip boot loader
1. Boot_NSIH - disassemble a header that jumps from A32 to A64
1. Boot1 - (bogus) disassemble the "bl1" boot loader distributed with Linux
1. Arm_wrap - tool for "wrapping" a binary file into an ELF file

And finally some bare metal programming

1. Blink32 - tiny standalone program to blink onboard LED in A32 mode
1. Blink64 - small standalone program to blink onboard LED in A64 mode
1. BlinkEL - blinks to indicate the EL (Exception Level)
1. Hello1 - simple serial IO, loaded by bl1.
1. Mini - tiny stub code to debug USB mode of the bl1 bootloader.

For notes and supporting information, see my website:

http://cholla.mmto.org/fire3/
