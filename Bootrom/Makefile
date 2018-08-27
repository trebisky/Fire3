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

all:	bootrom.dis

naive.dis: bootrom.bin
	$(DUMP) -D -b binary $(MACH) bootrom.bin >naive.dis

# objdump will suppress zones with zero bytes, displaying
# simply "..." (3 dots) for any number of zeros.
# The -z option tells it to show all those zeros.
bootrom.dis: bootrom.elf
	$(DUMP) $(MACH) -d bootrom.elf -z >bootrom.dis

bootrom.elf:	arm_wrap bootrom.bin
	./arm_wrap -b0 bootrom.bin bootrom.elf

bootrom.bin:	uboot.out
	./tobin

arm_wrap:
	cp ../Arm_wrap/arm_wrap .

clean:
	rm -f naive.dis
	rm -f bootrom.elf
	rm -f bootrom.dis
	rm -f bootrom.bin
