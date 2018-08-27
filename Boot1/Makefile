# Makefile to perform disassembly of the H3 chip bootrom
# from my Orange Pi PC board
# Patterned after things I did with the ESP8266 bootrom
#
# Tom Trebisky  12-14-2016  8-21-2018

#DUMP = xtensa-lx106-elf-objdump
#DUMP = arm-linux-gnu-objdump
DUMP = aarch64-linux-gnu-objdump

#MACH = -mxtensa
MACH = -marm

all:	bl1.dis

naive.dis: bl1.bin
	$(DUMP) -D -b binary $(MACH) bl1.bin >naive.dis

# objdump will suppress zones with zero bytes, displaying
# simply "..." (3 dots) for any number of zeros.
# The -z option tells it to show all those zeros.
bl1.dis: bl1.elf
	$(DUMP) $(MACH) -d bl1.elf -z >bl1.dis

bl1.elf:	arm_wrap bl1.bin
	./arm_wrap -bffff0000 bl1.bin bl1.elf

arm_wrap:
	cp ../Arm_wrap/arm_wrap .

clean:
	rm -f *.dis
	rm -f *.elf
