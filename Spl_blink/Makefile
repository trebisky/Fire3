# Makefile for Spl_blink project
# Adaped from Metro94 Github s5p6818_spl project.
# Tom Trebisky  8-24-2018
# tom@mmto.org
# Metro94 <flattiles@gmail.com>

BOARD = Fire3
CROSS_COMPILE = aarch64-linux-gnu-

OBJS = start.o exceptions.o boot.o

CFLAGS		:=	-g -Wall -Wextra -ffreestanding -fno-builtin -mlittle-endian
CFLAGS		+= -march=armv8-a+crc
CFLAGS		+= -mtune=cortex-a53
CFLAGS		+= -I.

LDFLAGS		:=	-Bstatic \
			-Wl,-Map=$(BOARD).map,--cref \
			-Tspl.lds \
			-Wl,--start-group \
			-Wl,--end-group \
			-Wl,--build-id=none \
			-nostdlib

CC			=	$(CROSS_COMPILE)gcc $(CFLAGS)
LD 			=	$(CROSS_COMPILE)gcc $(LDFLAGS)
OBJCOPY			=	$(CROSS_COMPILE)objcopy

BUILD			=	tools/build
LOAD			=	tools/load

# This gives us dependencies in .d files.
# CFLAGS		+= -MMD

.c.o:
	@echo " [CC]   $<"
	@$(CC) $< -c -o $@

.S.o:
	@echo " [CC]   $<"
	@$(CC) $< -c -o $@

all: build

$(BOARD).elf: $(OBJS)
	@echo " [LD]   $(BOARD).elf"
	$(LD) $(OBJS) -o $(BOARD).elf

build: $(BOARD).elf
	@echo " [IMG]  $(BOARD)_nonsih.img"
	@$(OBJCOPY) -O binary $(BOARD).elf $(BOARD)_nonsih.img
	@echo " [IMG]  $(BOARD).img"
	@$(BUILD) tools/nsih.bin $(BOARD)_nonsih.img $(BOARD).img

install: 
	$(LOAD) $(BOARD).img

.PHONY: clean
clean:
	rm -f *.o
	rm -f *.img
	rm -f *.elf
	rm -f *.map
#	rm -f *.d

# THE END
