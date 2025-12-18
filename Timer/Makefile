# Makefile for the Fire3 timer demo
#
# Tom Trebisky  8-31-2018, 12-17-2025
# tom@mmto.org
#
# This is intended to be loaded by U-boot

# -------------------------------------

OBJS = start.o trap.o main.o serial.o timer.o intcon_gic400.o printf.o

CROSS_COMPILE = aarch64-linux-gnu-

CFLAGS		:=	-g -Wall -Wextra -ffreestanding -fno-builtin -mlittle-endian
CFLAGS		+= -march=armv8-a+crc
CFLAGS		+= -mtune=cortex-a53
CFLAGS		+= -I.

LDFLAGS		:=	-Bstatic \
			-Ttimer.lds \
			-Wl,--start-group \
			-Wl,--end-group \
			-Wl,--build-id=none \
			-nostdlib \
			-Wl,--no-warn-rwx-segments

CC			=	$(CROSS_COMPILE)gcc $(CFLAGS)
LD 			=	$(CROSS_COMPILE)gcc $(LDFLAGS)
OBJCOPY			=	$(CROSS_COMPILE)objcopy
DUMP			=	$(CROSS_COMPILE)objdump

.c.o:
	@echo " [CC]   $<"
	@$(CC) $< -c -o $@

.S.o:
	@echo " [CC]   $<"
	@$(CC) $< -c -o $@

# -------------------------------------

#all: timer.bin
all: install

install:	timer.bin
	cp timer.bin /var/lib/tftpboot/fire3.bin

timer.elf: $(OBJS)
	$(LD) $(OBJS) -o timer.elf

timer.bin: timer.elf
	$(OBJCOPY) -O binary timer.elf timer.bin

dis: timer.elf
	$(DUMP) -d timer.elf -z >timer.dis

.PHONY: clean
clean:
	rm -f *.o
	rm -f *.img
	rm -f *.elf
	rm -f *.bin
	rm -f *.dis

# THE END
