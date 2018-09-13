# arm_wrap - convert a binary file into
# an elf file to allow improved disassembly
#
# Tom Trebisky  12-14-2016  8-21-2018

all:	arm_wrap

arm_wrap:	arm_wrap.c
	cc -o arm_wrap arm_wrap.c

install: arm_wrap
	cp arm_wrap /home/tom/bin

clean:
	rm -f arm_wrap
