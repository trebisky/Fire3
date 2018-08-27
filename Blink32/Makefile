# Makefile for Spl_blink32 project
#
# The goal here is to blink the on board LED
# running in 32 bit mode, with the least code possible.
#
# Tom Trebisky  8-26-2018
# tom@mmto.org

CROSS_COMPILE = arm-linux-gnu-

# -------------------------------------

OBJS = start.o blink.o
TOBJS = tiny.o blink.o

# optimizing makes the blink "go away".
#CFLAGS = -nostdinc -marm -mcpu=cortex-a53 -Os
CFLAGS = -nostdinc -marm -mcpu=cortex-a53

LDFLAGS		:=	-Bstatic \
			-Wl,--start-group \
			-Wl,--end-group \
			-Wl,--build-id=none \
			-nostdlib

CC			=	$(CROSS_COMPILE)gcc $(CFLAGS)
LD 			=	$(CROSS_COMPILE)gcc $(LDFLAGS)
OBJCOPY			=	$(CROSS_COMPILE)objcopy
DUMP			=	$(CROSS_COMPILE)objdump

LOAD			=	tools/loader -h32

.c.o:
	$(CC) $< -c -o $@

.S.o:
	$(CC) $< -c -o $@

# -------------------------------------

all: tiny.bin blink.bin

blink.elf: $(OBJS)
	@echo " [LD]   blink.elf"
	@$(LD) $(OBJS) -Tblink.lds -o blink.elf

blink.bin: blink.elf
	@echo " [IMG]  blink.bin"
	@$(OBJCOPY) -O binary blink.elf blink.bin

# ----
# "tiny" is kind of a silly experiment to see if we could place all
# of the blink code into the 512 byte header.  We can, but do not
# want to do this in general (how do we detect when the code becomes
# too big -- we just don't go there).  It does work.

tiny.elf: $(TOBJS)
	@echo " [LD]   tiny.elf"
	@$(LD) $(TOBJS) -Ttiny.lds -o tiny.elf

tiny.bin: tiny.elf
	@echo " [IMG]  tiny.bin"
	@$(OBJCOPY) -O binary tiny.elf tiny.bin

# ----

dis: blink.elf
	$(DUMP) -d blink.elf -z >blink.dis

fetch:
	cp ../USB_loader/loader tools

# -------------------------------------

# for debug
img:
	$(LOAD) -o blink.bin >blink.img

# -------------------
# You will need to be root for any of these to work.

tinyusb:  tiny.bin
	$(LOAD) -inject tiny.bin

usb:  blink.bin
	$(LOAD) blink.bin

sdcard:  blink.bin
	$(LOAD) -o blink.bin | dd of=/dev/sdf seek=1 conv=fdatasync

.PHONY: clean
clean:
	rm -f *.o
	rm -f *.img
	rm -f *.elf
	rm -f *.bin
	rm -f *.dis
	rm -f *.map
	rm -f *.d

# THE END
