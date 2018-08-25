# Makefile for various research exercises on
# the nsih.bin binary blob used for NanoPi Fire3
# SPL booting

DUMP = aarch64-linux-gnu-objdump -marm

all: nsih.dis nsih.odx

nsih.odx:  nsih.bin
	odx nsih.bin >nsih.odx

nsih.elf:  nsih.bin
	arm_wrap -bffff0000 -effff0000 nsih.bin nsih.elf

# objdump will suppress zones with zero bytes, displaying
# simply "..." (3 dots) for any number of zeros.
# The -z option tells it to show all those zeros.
nsih.dis: nsih.elf
	$(DUMP) -d nsih.elf -z >nsih.dis

clean:
	rm -f *.elf
	rm -f *.dis
	rm -f *.odx

# THE END
