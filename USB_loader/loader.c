/* fire3_loader.
 * This is derived from Github:metro94 s5pxx18_load and s5p6818_spl projects.
 * s5p6818 had some "black box" executables (build and load) that were linux elf files
 * with no source made available.  I reverse engineered those and incorporated
 * the knowledge into this.
 *
 * Tom Trebisky  tom@mmto.org  8-26-2018
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libusb.h>

#ifdef notdef
/* from s5p_boot.h */
#define SECOND_BOOT_ALL_MAX_SIZE	(56 * 1024)
#define SECOND_BOOT_HEADER_SIZE		(512)
#define SECOND_BOOT_OFFSET			(2048)

#define SECOND_BOOT_DEVICEADDR		(0x00000000)
#define SECOND_BOOT_LOADSIZE		(0x00000000)	// Calculated at runtime
#define SECOND_BOOT_LOADADDR		(0xffff0000)
#define SECOND_BOOT_LAUNCHADDR		(0xffff0000)
#endif

/* from s5p_usb.h */
#define S5P6818_VID		(0x04e8)
#define S5P6818_PID		(0x1234)

#define S5P6818_INTERFACE	(0)
#define S5P6818_EP_OUT		((unsigned char)0x02)

/* --------------------------- */

/* tjt: 100M !! heck of a big buffer. */
#define BOOTLOADER_MAX_SIZE	100 * 1024 * 1024

typedef unsigned char uint8;
typedef unsigned int  uint32;

unsigned char buf[BOOTLOADER_MAX_SIZE];

int load_size;
unsigned int load_addr = 0xffff0000;
unsigned int launch_addr = 0xffff0000;

enum { NOHEADER, H32, H64 } header_type = NOHEADER;
int inject_header = 0;

/* --------------------------- */

void write32LE(uint8 *addr, uint32 data)
{
	*(addr)     = (uint8)(data & 0xff);
	*(addr + 1) = (uint8)((data >> 8) & 0xff);
	*(addr + 2) = (uint8)((data >> 16) & 0xff);
	*(addr + 3) = (uint8)((data >> 24) & 0xff);
}

int
mk_header32 ( uint8 *buf )
{
	write32LE(buf + 0x1fc, 0x4849534e);
	return 0;
}

int
mk_header64 ( uint8 *buf )
{
	write32LE(buf + 0x1fc, 0x4849534e);
	return 0;
}

void initBootloaderHeader (uint8 *buf )
{
	// Clear buffer
	memset(buf, 0x00, 512);

	write32LE(buf + 0x40, 0x00000000);
	write32LE(buf + 0x44, load_size);
	write32LE(buf + 0x48, load_addr);
	write32LE(buf + 0x4c, launch_addr);

	write32LE(buf + 0x1fc, 0x4849534e);
}

int
readBin ( uint8 *buf, const char *filepath )
{
	FILE *fin;

	int size;

	fin = fopen(filepath, "rb");
	if (fin == NULL) {
	    fprintf ( stderr, "Unable to open: %s\n", filepath );
	    return -1;
	}

	size = (int)fread(buf, 1, BOOTLOADER_MAX_SIZE, fin);
	fclose(fin);
	if (size <= 0)
		return size;

	// Size must be aligned by 16bytes
	if (size % 16 != 0)
		size = ((size / 16) + 1) * 16;

	return size;
}

int
usbBoot ( uint8 *buf, int size )
{
	libusb_context        *ctx;
	libusb_device        **list;
	libusb_device_handle  *dev_handle;
	int transfered;

	if ( libusb_init(&ctx) < 0 ) {
	    fprintf ( stderr, "Cannot initialize libusb\n" );
	    return -1;
	}

	libusb_set_debug(ctx, LIBUSB_LOG_LEVEL_INFO);

	if ( libusb_get_device_list(ctx, &list) < 1 ) {
	    fprintf ( stderr, "No USB devices found\n" );
	    return -1;
	}

	dev_handle = libusb_open_device_with_vid_pid(ctx, S5P6818_VID, S5P6818_PID);
	if ( dev_handle == NULL ) {
	    fprintf ( stderr, "USB device for S5P6818 download not found\n" );
	    return -1;
	}

	libusb_free_device_list(list, 1);

	if ( libusb_claim_interface(dev_handle, S5P6818_INTERFACE) < 0 ) {
	    fprintf ( stderr, "cannot claim USB device for S5P6818 download\n" );
	    return -1;
	}

	fprintf ( stderr, "Start transfer, size = %d\n", size );

	if ( libusb_bulk_transfer(dev_handle, S5P6818_EP_OUT, buf, size, &transfered, 0) < 0 ) {
	    fprintf ( stderr, "USB bulk transfer failed\n" );
	    return -1;
	}

	if (size != transfered) {
	    fprintf ( stderr, "Transferred only %d of %d bytes\n", transfered, size );
	    fprintf ( stderr, "This may fail to run\n" );
	} else
	    fprintf ( stderr, "Transfer successful\n" );

	libusb_release_interface(dev_handle, 0);
	libusb_close(dev_handle);
	libusb_exit(ctx);
	
	return 0;
}

/* Here is the usage manual.
 * 
 *  "loader image" -- send verbatim, image already has header
 *  "loader -h32 image" -- prepend A32 mode header and send
 *  "loader -h64 image" -- prepend A64 mode header and send
 *
 * Note that sending always means updating the size field in the header and padding
 *  the image with zeros to a multiple of 16 bytes if necessary.
 *
 * A 32 bit header is 512 bytes, so code must have an offset of 0x200
 * A 64 bit header is 2048 bytes, so code must have an offset of 0x800
 *
 * If code is built without an offset, but has a space at the start to accomodate
 *  the header, the "-inject" option can be used.
 *
 * The "-o" option allows output to stdout in lieu of sending over USB
 *  and may be useful for debugging or who knows what else.
 *
 * load_addr and launch_addr default to 0xffff0000, but these can be overriden.
 *
 *   -l<addr> sets both to addr, which is a hex value (i.e. -lffff0800)
 *   -e<addr) sets launch_addr to <addr> (i.e. -effff0200)
 *
 * if both -e and -l are used, -e must follow -l
 */

void
usage ( void )
{
	fprintf ( stderr, "Usage: loader [options] image\n" );
	exit ( 1 );
}

int
main(int argc, const char *argv[])
{
	int offset;
	int ret;

	argc--;
	argv++;

	while ( argc && argv[0][0] == '-' ) {
	    argc--;
	    argv++;
	}

	if ( argc != 1 )
	    usage ();

	if ( header_type == H32 ) {
	    offset = mk_header32 ( buf );
	}

	if ( header_type == H64 ) {
	    offset = mk_header64 ( buf );
	}

	load_size = readBin ( &buf[offset], argv[0] );
	if ( load_size <= 0 ) {
	    exit ( 1 );
	}

	ret = usbBoot ( buf, load_size );
	if (ret < 0) {
	    fprintf(stderr, "Download failed\n");
	    exit ( 1 );
	}

#ifdef notdef

	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Wrong Parameters\n");
		printf("Usage: load bootloader load_addr [launch_addr]\n");
		printf("If launch_addr is not passed or launch_addr equals to 0, bootloader will be loaded without executing it.\n");
		return -1;
	}
	
	if (sscanf(argv[2], "%x", &load_addr) != 1) {
		fprintf(stderr, "Invalid Parameters \"load_addr\"\n");
		printf("Usage: load bootloader load_addr [launch_addr]\n");
		printf("If launch_addr is not passed or launch_addr equals to 0, bootloader will be loaded without executing it.\n");
	}
	
	if (argc == 3) {
		launch_addr = 0;
	} else if (sscanf(argv[3], "%x", &launch_addr) != 1) {
		fprintf(stderr, "Invalid Parameters \"launch_addr\"\n");
		printf("Usage: load bootloader load_addr [launch_addr]\n");
		printf("If launch_addr is not passed or launch_addr equals to 0, bootloader will be loaded without executing it.\n");
	}

	load_size = readBin(mem + 0x200, argv[1]);
	if (size <= 0) {
		fprintf(stderr, "Failed when loading bootloader\n");
		return -1;
	}

	initBootloaderHeader(mem, size, load_addr, launch_addr);
	size += 0x200;

	ret = usbBoot(mem, load_size);
	if (ret < 0) {
		fprintf(stderr, "Failed when starting bootloader\n");
		return -1;
	}
#endif

	exit ( 0 );
}

/* THE END */
