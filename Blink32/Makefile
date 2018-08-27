# Makefile for Spl_blink32 project
#
# The goal here is to blink the on board LED
# running in 32 bit mode, with the least code possible.
#
# Tom Trebisky  8-26-2018
# tom@mmto.org

BOARD = fire3
# CROSS_COMPILE = aarch64-linux-gnu-
CROSS_COMPILE = arm-linux-gnu-

# -------------------------------------

OBJS = start.o blink.o

TARGET = $(BOARD).bin

#CFLAGS = -nostdinc -marm -mcpu=cortex-a53 -Os
CFLAGS = -nostdinc -marm -mcpu=cortex-a53

# CFLAGS		:=	-g -Wall -Wextra -ffreestanding -fno-builtin -mlittle-endian

LDFLAGS		:=	-Bstatic \
			-Tspl.lds \
			-Wl,--start-group \
			-Wl,--end-group \
			-Wl,--build-id=none \
			-nostdlib

CC			=	$(CROSS_COMPILE)gcc $(CFLAGS)
LD 			=	$(CROSS_COMPILE)gcc $(LDFLAGS)
OBJCOPY			=	$(CROSS_COMPILE)objcopy
DUMP			=	$(CROSS_COMPILE)objdump

LOAD			=	tools/loader -inject -h32

.c.o:
	$(CC) $< -c -o $@

.S.o:
	$(CC) $< -c -o $@

# -------------------------------------

all: $(TARGET)

$(BOARD).elf: $(OBJS)
	@echo " [LD]   $(BOARD).elf"
	@$(LD) $(OBJS) -o $(BOARD).elf

$(TARGET): $(BOARD).elf
	@echo " [IMG]  $(TARGET)"
	@$(OBJCOPY) -O binary $(BOARD).elf $(TARGET)

dis: $(BOARD).elf
	$(DUMP) -d $(BOARD).elf -z >$(BOARD).dis

fetch:
	cp ../USB_loader/loader tools

# for debug
img:
	$(LOAD) -o $(TARGET) >$(BOARD).img

# -------------------
# You will need to be root for either of these to work.

usb:  $(TARGET)
	$(LOAD) $(TARGET)

sdcard:  $(TARGET)
	$(LOAD) -o $(TARGET) | dd of=/dev/sdf seek=1 conv=fdatasync

.PHONY: clean
clean:
	rm -f *.o
	rm -f *.img
	rm -f *.elf
	rm -f *.bin
	rm -f *.map
	rm -f *.dis
#	rm -f *.d

# THE END
