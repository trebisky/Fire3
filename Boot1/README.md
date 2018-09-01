Boot1

Read out and disassemble the Stage 1 bootloader for the Samsung S5P6818

This is a defunct and abandoned project.
I have lost interest in this since I found sources for a version
of the bl1 bootloader, and am now using the version I build from source.

I have been unable to get that source to build this black box,
but I have no further interest in reverse engineering the
FriendlyArm linux boot sequence.

One thing is worthy of note.  This thing runs entirely in A32 mode,
whereas the version I am now using has some header code that
switches to A64 which his how that bl1 runs.

This begins with the file bl1-mmcboot.bin, which I found in
the sd-fuse_s5p6818/prebuilt for the NanoPi Fire3

The file bootrom.txt is the final product.

For notes and supporting information, see my website:

http://cholla.mmto.org/fire3/
