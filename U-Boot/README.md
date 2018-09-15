This originated from the Samsung Artik 710 U-boot on Github.
I didn't fork that because I didn't want all of the branches.
It has been modified to run on the NanoPi Fire3, which is a
Samsung S5P6818 board just like the Fire3.

In particular, as configured, this offers network support,
which the Friendly Arm U-boot does not.  I currently use this
code on my Fire3 to do DHCP, then TFTP a binary, which allows
me to do bare metal development.

What I did to generate this:

1. Clone the Samsung Artik710 U-boot from Github
1. Checkout the A710/v2016.01 branch
1. Add my CROSS_COMPILE prefix to the Makefile
1. Edit include/configs/artik710_raptor.h
1. -- change SERIAL_INDEX from 3 to 0
1. -- change PHY_ADDR from 3 to 0
1. -- change the CONFIG_SYS_DRAM_SIZE to 4000_0000
1. Edit arch/arm/cpu/armv8/start.S
1. -- add NSIH header
1. Edit configs/artik710_raptor_defconfig
1. -- add CONFIG_DHCP=y

I may have done other things that I don't remember.
This works along with my modified bl1 boot loader without
any of the secure FIP nonsense that Friendly Arm uses to
boot Linux.
