# Makefile for Hello1
# Tom Trebisky  8-31-2018
# tom@mmto.org

# -------------------------------------

OBJS = start.o hello.o serial.o printf.o

CROSS_COMPILE = aarch64-linux-gnu-

CFLAGS		:=	-g -Wall -Wextra -ffreestanding -fno-builtin -mlittle-endian
CFLAGS		+= -march=armv8-a+crc
CFLAGS		+= -mtune=cortex-a53
CFLAGS		+= -I.

LDFLAGS		:=	-Bstatic \
			-Thello.lds \
			-Wl,--start-group \
			-Wl,--end-group \
			-Wl,--build-id=none \
			-nostdlib

CC			=	$(CROSS_COMPILE)gcc $(CFLAGS)
LD 			=	$(CROSS_COMPILE)gcc $(LDFLAGS)
OBJCOPY			=	$(CROSS_COMPILE)objcopy
DUMP			=	$(CROSS_COMPILE)objdump

LOAD			=	usb_loader

.c.o:
	@echo " [CC]   $<"
	@$(CC) $< -c -o $@

.S.o:
	@echo " [CC]   $<"
	@$(CC) $< -c -o $@

# -------------------------------------

all: hello.bin

hello.elf: $(OBJS)
	$(LD) $(OBJS) -o hello.elf

hello.bin: hello.elf
	$(OBJCOPY) -O binary hello.elf hello.bin

dis: hello.elf
	$(DUMP) -d hello.elf -z >hello.dis

#fetch:
#	cp ../USB_loader/loader tools

# -------------------
# You will need to be root for either of these to work.
# Only "sdcard" will work, unless you have a special bl1.

usb:  hello.bin
	$(LOAD) hello.bin

sdcard:  hello.bin
	dd if=hello.bin of=/dev/sdf seek=129 conv=fdatasync

.PHONY: clean
clean:
	rm -f *.o
	rm -f *.img
	rm -f *.elf
	rm -f *.bin
	rm -f *.dis

# THE END
