#
# (C) Copyright 2016 Nexell
# Hyunseok, Jung <hsjung@nexell.co.kr>
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y				+= clock.o
obj-y				+= timer.o
obj-y				+= reset.o
obj-y				+= nx_gpio.o
obj-y				+= tieoff.o
obj-y				+= nx_sec_reg.o
obj-$(CONFIG_ARCH_S5P4418)	+= reg-call.o
obj-$(CONFIG_CMD_BOOTL)	+= cmd_boot_linux.o
obj-$(CONFIG_S5P_SERIAL) 	+= serial.o
obj-$(CONFIG_SPL_BUILD) += spl/
