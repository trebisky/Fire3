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
#define SECOND_BOOT_OFFSET		(2048)

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

typedef unsigned char uint8;
typedef unsigned int  uint32;

/* --------------------------- */

enum { NOHEADER, H32, H64 } header_type = NOHEADER;

#define H32_SIZE	512
#define H64_SIZE	2048

/* tjt: 100M !! heck of a big buffer. */
// #define BUF_SIZE	100 * 1024 * 1024
#define BUF_SIZE	16 * 1024 * 1024

uint8 buf[BUF_SIZE];

int load_size;
unsigned int load_addr =   0xffff0000;
unsigned int launch_addr = 0xffff0000;

/* --------------------------- */

/* This makes the code portable to big endian machines */
void
write_long (uint8 *addr, uint32 data)
{
	*(addr)     = (uint8)(data & 0xff);
	*(addr + 1) = (uint8)((data >> 8) & 0xff);
	*(addr + 2) = (uint8)((data >> 16) & 0xff);
	*(addr + 3) = (uint8)((data >> 24) & 0xff);
}

uint32	code64_at_0[] = {
	0xea000015,
	0xea00003c,
	0xea00003b,
	0xea00003a,
	0xea000039,
	0xea000038,
	0xea000037,
	0xea000036
};

uint32	code64_at_5c[] = {
	0xe3010000,
	0xe34c0001,
	0xe30c1200,
	0xe3431fff,
	0xe3002000,
	0xe3402000,
	0xe5801140,
	0xe5802144,
	0xe5801148,
	0xe580214c,
	0xe5801150,
	0xe5802154,
	0xe5801158,
	0xe580215c,
	0xe5801184,
	0xe5802188,
	0xe580118c,
	0xe5802190,
	0xe5801194,
	0xe5802198,
	0xe580119c,
	0xe58021a0,
	0xe590113c,
	0xe3811a0f,
	0xe580113c,
	0xe5901180,
	0xe38110f0,
	0xe5801180,
	0xe5901138,
	0xe381160f,
	0xe5801138,
	0xe590117c,
	0xe3811a0f,
	0xe580117c,
	0xe3000000,
	0xe34c0001,
	0xe59012ac,
	0xe3811001,
	0xe58012ac,
	0xe320f003,
	0xe30b0000,
	0xe34c0001,
	0xe5901020,
	0xe3c11403,
	0xe3811402,
	0xe5801020,
	0xe5901000,
	0xe3811a01,
	0xe5801000,
	0xe5901004,
	0xe3811a01,
	0xe5801004,
	0xeafffffe
};

void
mk_header64 ( uint8 *buf )
{
	int i, num;
	uint8 *p;

	num = sizeof(code64_at_0)/sizeof(uint32);
	p = &buf[0];
	for ( i=0; i<num; i++ ) {
	    write_long ( p, code64_at_0[i] );
	    p += sizeof(uint32);
	}

	num = sizeof(code64_at_5c)/sizeof(uint32);
	p = &buf[0x5c];
	for ( i=0; i<num; i++ ) {
	    write_long ( p, code64_at_5c[i] );
	    p += sizeof(uint32);
	}

	write_long (buf + 0x44, load_size - 512);
	write_long (buf + 0x48, load_addr);
	write_long (buf + 0x4c, launch_addr);

	write_long (buf + 0x1fc, 0x4849534e);
}

void
mk_header32 ( uint8 *buf )
{
	write_long (buf + 0, 0xea00007e );	/* b 0xffff0200 */

	write_long (buf + 0x44, load_size - 512);
	write_long (buf + 0x48, load_addr);
	write_long (buf + 0x4c, launch_addr);

	write_long (buf + 0x1fc, 0x4849534e);
}

void
fix_size ( void )
{
	write_long (buf + 0x44, load_size - 512);
}

void
set_next_addr ( uint32 addr )
{
	write_long (buf + 0x40, addr );
}

#define ALIGN	16

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

	size = (int)fread(buf, 1, BUF_SIZE, fin);
	fclose(fin);

	if (size <= 0) {
	    fprintf ( stderr, "Empty file: %s\n", filepath );
	    return -1;
	}

	// Size must be aligned by 16 bytes
	if (size % ALIGN != 0)
	    size = ((size / ALIGN) + 1) * ALIGN;

	return size;
}

int
usbLoad ( uint8 *buf, int size )
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
	    fprintf ( stderr, "perhaps you need to be root or to reset the board\n" );
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
 *   -s<addr) sets launch_addr to <addr> (i.e. -sffff0200)
 *
 * if both -l and -s are used, -s must follow -l
 */

void
usage ( void )
{
	fprintf ( stderr, "Usage: loader [options] image\n" );
	exit ( 1 );
}

/* I got weird behavior when downloading just a 512 byte executable.
 * The download would work, but not start running.
 * If I repeated the download without resetting the board,
 * then it would run.
 * Of course the size field in the header is zero, which must be
 * causing some odd behavior in the on-chip bootrom.
 * bumping the size up a notch fixes all this nicely.
 */

#define MIN_SIZE	512 + ALIGN

int
main(int argc, const char *argv[])
{
	int inject_header = 0;
	int emit_file = 0;
	int set_next = 0;

	int offset;
	int ret;
	int file_size;
	unsigned int val;
	unsigned int next_addr = 0;

	argc--;
	argv++;

	while ( argc && argv[0][0] == '-' ) {
	    if ( strcmp ( "h64", &argv[0][1] ) == 0 ) {
		header_type = H64;
	    }
	    if ( strcmp ( "h32", &argv[0][1] ) == 0 ) {
		header_type = H32;
	    }
	    if ( strcmp ( "inject", &argv[0][1] ) == 0 ) {
		inject_header = 1;
	    }
	    if ( argv[0][1] == 'o' ) {
		emit_file = 1;
	    }
	    if ( argv[0][1] == 'l' ) {
		/* load address -lffff0000 */
                val = strtol ( &argv[0][2], NULL, 16 );
                load_addr = val;
                launch_addr = val;
	    }
	    if ( argv[0][1] == 's' ) {
		/* start address -sffff0800 */
                val = strtol ( &argv[0][2], NULL, 16 );
                launch_addr = val;
	    }
	    if ( argv[0][1] == 'a' ) {
		/* sector offset -a129 not hex */
                val = strtol ( &argv[0][2], NULL, 10 );
		set_next = 1;
                next_addr = val * 512;
	    }

	    argc--;
	    argv++;
	}

	if ( argc != 1 )
	    usage ();

	/* In case we get asked to inject into a binary
	 * with size 512 or less.
	 */
	memset ( buf, 0x00, MIN_SIZE );

	offset = 0;

	if ( header_type == H32 )
	    offset = H32_SIZE;

	if ( header_type == H64 )
	    offset = H64_SIZE;

	if ( inject_header )
	    offset = 0;

	if ( offset )
	    memset ( buf, 0x00, offset );

	file_size = readBin ( &buf[offset], argv[0] );
	if ( file_size <= 0 )
	    exit ( 1 );

	load_size = file_size + offset;
	if ( load_size <= 512 )
	    load_size = MIN_SIZE;

	if ( header_type == H32 )
	    mk_header32 ( buf );

	if ( header_type == H64 )
	    mk_header64 ( buf );

	/* For verbatim files */
	fix_size ();

	if ( set_next )
	    set_next_addr ( next_addr );

	if ( emit_file ) {
	    fwrite ( buf, 1, load_size, stdout );
	    exit ( 0 );
	}

	ret = usbLoad ( buf, load_size );
	if (ret < 0) {
	    fprintf(stderr, "Download failed\n");
	    exit ( 1 );
	}

	exit ( 0 );
}

/* THE END */
