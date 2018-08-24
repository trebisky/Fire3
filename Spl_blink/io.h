/*
 *	Copyright (c) 2017 Metro94
 *
 *	File:        io.h
 *	Description: I/O header for S5P6818
 *	Author:      Metro94 <flattiles@gmail.com>
 *  Date:        
 */

#include <common.h>

#define ref64(addr)				(*(uint64_t *)(addr))
#define read64(addr, val)			(val = ref64(addr))
#define write64(addr, val)			(ref64(addr) = val)
#define setbits64(addr, set)			(ref64(addr) |= (set))
#define clrbits64(addr, clr)			(ref64(addr) &= ~(clr))
#define tglbits64(addr, tgl)			(ref64(addr) ^= (tgl))
#define clrsetbits64(addr, clr, set)		(ref64(addr) = (ref64(addr) & ~(clr)) | (set))

#define ref32(addr)				(*(uint32_t *)(addr))
#define read32(addr, val)			(val = ref32(addr))
#define write32(addr, val)			(ref32(addr) = val)
#define setbits32(addr, set)			(ref32(addr) |= (set))
#define clrbits32(addr, clr)			(ref32(addr) &= ~(clr))
#define tglbits32(addr, tgl)			(ref32(addr) ^= (tgl))
#define clrsetbits32(addr, clr, set)		(ref32(addr) = (ref32(addr) & ~(clr)) | (set))

#define ref8(addr)				(*(uint8_t *)(addr))
#define read8(addr, val)			(val = ref8(addr))
#define write8(addr, val)			(ref8(addr) = val)
