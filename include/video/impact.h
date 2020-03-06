/*
 * linux/drivers/video/impact.h
 *   SGI Octane MardiGras (IMPACTSR) graphics
 *   SGI Indigo2 MardiGras (IMPACT) graphics
 *
 * Copyright (c)
 *   2004-2006 by Stanislaw Skowronek <skylark@unaligned.org> (author)
 *   2005 by Peter Fuerst <post@pfrst.de> (Indigo2 adaptation)
 *   2007-2009 by Johannes Dickgreber <tanzy@gmx.de> (platform_driver)
 *   2014 by Thomas Bogendoerfer <tsbogend@alpha.franken.de> (gio_driver)
 *   2011-2015 by Joshua Kinard <kumba@gentoo.org> (upkeeping/maintenance)
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#ifndef _IMPACT_H
#define _IMPACT_H

/* IP22/IP28 have HQ3, IP30 has HQ4. */
#if defined(CONFIG_SGI_IP22) || defined(CONFIG_SGI_IP28)
#define SGI_INDIGO2 1
#else
#undef SGI_INDIGO2
#endif

/* Some fixed register values. */
#ifndef SGI_INDIGO2
 /* ImpactSR (HQ4) registers */
 #define VAL_CFIFO_HW			0x47
 #define VAL_CFIFO_LW			0x14
 #define VAL_CFIFO_DELAY		0x64
 #define VAL_DFIFO_HW			0x40
 #define VAL_DFIFO_LW			0x10
 #define VAL_DFIFO_DELAY		0
 #define MSK_CFIFO_CNT			0xff
 #define USEPOOLS			5
#else
 /* Impact (HQ3) registers */
 #define VAL_CFIFO_HW			0x20 /* 0x18 ? */
 #define VAL_CFIFO_LW			0x14
 #define VAL_CFIFO_DELAY		0x64
 #define VAL_DFIFO_HW			0x28
 #define VAL_DFIFO_LW			0x14
 #define VAL_DFIFO_DELAY		0xfff
 #define MSK_CFIFO_CNT			0x7f
 #define USEPOOLS			4
#endif

/* Convenient access macros */
#define IMP_REG64(vma, off)		(*(volatile u64 *)((vma) + (off)))
#define IMP_REG32(vma, off)		(*(volatile u32 *)((vma) + (off)))
#define IMP_REG16(vma, off)		(*(volatile u16 *)((vma) + (off)))
#define IMP_REG8(vma, off)		(*(volatile u8 *)((vma) + (off)))

#ifndef SGI_INDIGO2
 /* ImpactSR (HQ4) register offsets */
 #define IMP_CFIFO(vma)			IMP_REG64(vma, 0x020400)
 #define IMP_CFIFOW(vma)		IMP_REG32(vma, 0x020400)
 #define IMP_CFIFOW1(vma)		IMP_REG32(vma, 0x020404)
 #define IMP_CFIFOP(vma)		IMP_REG64(vma, 0x130400)
 #define IMP_CFIFOPW(vma)		IMP_REG32(vma, 0x130400)
 #define IMP_CFIFOPW1(vma)		IMP_REG32(vma, 0x130404)

 #define IMP_STATUS(vma)		IMP_REG32(vma, 0x020000)
 #define IMP_FIFOSTATUS(vma)		IMP_REG32(vma, 0x020008)
 #define IMP_GIOSTATUS(vma)		IMP_REG32(vma, 0x020100)
 #define IMP_DMABUSY(vma)		IMP_REG32(vma, 0x020200)

 #define IMP_CFIFO_HW(vma)		IMP_REG32(vma, 0x040000)
 #define IMP_CFIFO_LW(vma)		IMP_REG32(vma, 0x040008)
 #define IMP_CFIFO_DELAY(vma)		IMP_REG32(vma, 0x040010)
 #define IMP_DFIFO_HW(vma)		IMP_REG32(vma, 0x040020)
 #define IMP_DFIFO_LW(vma)		IMP_REG32(vma, 0x040028)
 #define IMP_DFIFO_DELAY(vma)		IMP_REG32(vma, 0x040030)

 #define IMP_XMAP_OFF(off)		(0x71c00 + (off))
 #define IMP_VC3_OFF(off)		(0x72000 + (off))
 #define IMP_RSS_OFF(off)		(0x2c000 + (off))
#else
 /* Impact (HQ3) register offsets */
 #define IMP_CFIFO(vma)			IMP_REG64(vma, 0x070080)
 #define IMP_CFIFOW(vma)		IMP_REG32(vma, 0x070080)
 #define IMP_CFIFOW1(vma)		IMP_REG32(vma, 0x070084)
 #define IMP_CFIFOP(vma)		IMP_REG64(vma, 0x050080)
 #define IMP_CFIFOPW(vma)		IMP_REG32(vma, 0x050080)
 #define IMP_CFIFOPW1(vma)		IMP_REG32(vma, 0x050084)

 #define IMP_STATUS(vma)		IMP_REG32(vma, 0x070000)
 #define IMP_FIFOSTATUS(vma)		IMP_REG32(vma, 0x070004)
 #define IMP_GIOSTATUS(vma)		IMP_REG32(vma, 0x070100)
 #define IMP_DMABUSY(vma)		IMP_REG32(vma, 0x070104)

 #define IMP_CFIFO_HW(vma)		IMP_REG32(vma, 0x050020)
 #define IMP_CFIFO_LW(vma)		IMP_REG32(vma, 0x050024)
 #define IMP_CFIFO_DELAY(vma)		IMP_REG32(vma, 0x050028)
 #define IMP_DFIFO_HW(vma)		IMP_REG32(vma, 0x05002c)
 #define IMP_DFIFO_LW(vma)		IMP_REG32(vma, 0x050030)
 #define IMP_DFIFO_DELAY(vma)		IMP_REG32(vma, 0x050034)

 #define IMP_XMAP_OFF(off)		(0x61c00 + (off))
 #define IMP_VC3_OFF(off)		(0x62000 + (off))
 #define IMP_RSS_OFF(off)		(0x7c000 + (off))
#endif

#define IMP_RESTATUS(vma)		IMP_REG32(vma, IMP_RSS_OFF(0x578))

#define IMP_XMAP_PP1SELECT(vma)		IMP_REG8(vma, IMP_XMAP_OFF(0x008))
#define IMP_XMAP_INDEX(vma)		IMP_REG8(vma, IMP_XMAP_OFF(0x088))
#define IMP_XMAP_CONFIG(vma)		IMP_REG32(vma, IMP_XMAP_OFF(0x100))
#define IMP_XMAP_CONFIGB(vma)		IMP_REG8(vma, IMP_XMAP_OFF(0x108))
#define IMP_XMAP_BUF_SELECT(vma)	IMP_REG32(vma, IMP_XMAP_OFF(0x180))
#define IMP_XMAP_MAIN_MODE(vma)		IMP_REG32(vma, IMP_XMAP_OFF(0x200))
#define IMP_XMAP_OVERLAY_MODE(vma)	IMP_REG32(vma, IMP_XMAP_OFF(0x280))
#define IMP_XMAP_DIB(vma)		IMP_REG32(vma, IMP_XMAP_OFF(0x300))
#define IMP_XMAP_DIB_DW(vma)		IMP_REG32(vma, IMP_XMAP_OFF(0x340))
#define IMP_XMAP_RE_RAC(vma)		IMP_REG32(vma, IMP_XMAP_OFF(0x380))

#define IMP_VC3_INDEX(vma)		IMP_REG8(vma, IMP_VC3_OFF(0x008))
#define IMP_VC3_INDEXDATA(vma)		IMP_REG32(vma, IMP_VC3_OFF(0x038))
#define IMP_VC3_DATA(vma)		IMP_REG16(vma, IMP_VC3_OFF(0x0b0))
#define IMP_VC3_RAM(vma)		IMP_REG16(vma, IMP_VC3_OFF(0x190))

#define IMP_BDVERS0(vma)		IMP_REG8(vma, IMP_VC3_OFF(0x408))
#define IMP_BDVERS1(vma)		IMP_REG8(vma, IMP_VC3_OFF(0x488))


/* FIFO status */
#ifdef SGI_INDIGO2
  #define IMP_CFIFO_MAX		64
#else
  #define IMP_CFIFO_MAX		128
#endif
#define IMP_BFIFO_MAX		16


/* Commands for CFIFO */
#define IMP_CMD_WRITERSS(reg, val)				\
	(((0x00180004L | ((reg) << 8)) << 32) | ((u32)(val) & 0xffffffff))

#define IMP_CMD_EXECRSS(reg, val)				\
	(((0x001c0004L | ((reg) << 8)) << 32) | ((u32)(val) & 0xffffffff))

#define IMP_CMD_GLINE_XSTARTF(v)	IMP_CMD_WRITERSS(0x00c, v)
#define IMP_CMD_IR_ALIAS(v)		IMP_CMD_EXECRSS(0x045, v)
#define IMP_CMD_BLOCKXYSTARTI(x, y)	IMP_CMD_WRITERSS(0x046, ((x) << 16) | (y))
#define IMP_CMD_BLOCKXYENDI(x, y)	IMP_CMD_WRITERSS(0x047, ((x) << 16) | (y))
#define IMP_CMD_PACKEDCOLOR(v)		IMP_CMD_WRITERSS(0x05b, v)
#define IMP_CMD_RED(v)			IMP_CMD_WRITERSS(0x05c, v)
#define IMP_CMD_ALPHA(v)		IMP_CMD_WRITERSS(0x05f, v)
#define IMP_CMD_CHAR(v)			IMP_CMD_EXECRSS(0x070, v)
#define IMP_CMD_CHAR_H(v)		IMP_CMD_WRITERSS(0x070, v)
#define IMP_CMD_CHAR_L(v)		IMP_CMD_EXECRSS(0x071, v)
#define IMP_CMD_XFRCONTROL(v)		IMP_CMD_WRITERSS(0x102, v)
#define IMP_CMD_FILLMODE(v)		IMP_CMD_WRITERSS(0x110, v)
#define IMP_CMD_CONFIG(v)		IMP_CMD_WRITERSS(0x112, v)
#define IMP_CMD_XYWIN(x, y)		IMP_CMD_WRITERSS(0x115, ((y) << 16) | (x))
#define IMP_CMD_BKGRD_RG(v)		IMP_CMD_WRITERSS(0x140, ((v) << 8))
#define IMP_CMD_BKGRD_BA(v)		IMP_CMD_WRITERSS(0x141, ((v) << 8))
#define IMP_CMD_WINMODE(v)		IMP_CMD_WRITERSS(0x14f, v)
#define IMP_CMD_XFRSIZE(x, y)		IMP_CMD_WRITERSS(0x153, ((y) << 16) | (x))
#define IMP_CMD_XFRMASKLO(v)		IMP_CMD_WRITERSS(0x156, v)
#define IMP_CMD_XFRMASKHI(v)		IMP_CMD_WRITERSS(0x157, v)
#define IMP_CMD_XFRCOUNTERS(x, y)	IMP_CMD_WRITERSS(0x158, ((y) << 16) | (x))
#define IMP_CMD_XFRMODE(v)		IMP_CMD_WRITERSS(0x159, v)
#define IMP_CMD_RE_TOGGLECNTX(v)	IMP_CMD_WRITERSS(0x15f, v)
#define IMP_CMD_PIXCMD(v)		IMP_CMD_WRITERSS(0x160, v)
#define IMP_CMD_PP1FILLMODE(m, o)	IMP_CMD_WRITERSS(0x161, (m) | (o << 26))
#define IMP_CMD_COLORMASKMSBS(v)	IMP_CMD_WRITERSS(0x162, v)
#define IMP_CMD_COLORMASKLSBSA(v)	IMP_CMD_WRITERSS(0x163, v)
#define IMP_CMD_COLORMASKLSBSB(v)	IMP_CMD_WRITERSS(0x164, v)
#define IMP_CMD_BLENDFACTOR(v)		IMP_CMD_WRITERSS(0x165, v)
#define IMP_CMD_DRBPOINTERS(v)		IMP_CMD_WRITERSS(0x16d, v)

#define IMP_UNSIGNED(v)			((unsigned)(v) & 0xffffffff)

#define	IMP_CMD_HQ_PIXELFORMAT(v)	(0x000c000400000000L | IMP_UNSIGNED(v))
#define	IMP_CMD_HQ_SCANWIDTH(v)		(0x000a020400000000L | IMP_UNSIGNED(v))
#define	IMP_CMD_HQ_DMATYPE(v)		(0x000a060400000000L | IMP_UNSIGNED(v))
#define	IMP_CMD_HQ_PG_LIST_0(v)		(0x0008000400000000L | IMP_UNSIGNED(v))
#define	IMP_CMD_HQ_PG_WIDTH(v)		(0x0008040400000000L | IMP_UNSIGNED(v))
#define	IMP_CMD_HQ_PG_OFFSET(v)		(0x0008050400000000L | IMP_UNSIGNED(v))
#define	IMP_CMD_HQ_PG_STARTADDR(v)	(0x0008060400000000L | IMP_UNSIGNED(v))
#define	IMP_CMD_HQ_PG_LINECNT(v)	(0x0008070400000000L | IMP_UNSIGNED(v))
#define	IMP_CMD_HQ_PG_WIDTHA(v)		(0x0008080400000000L | IMP_UNSIGNED(v))
#define IMP_CMD_HQ_PGSIZE(v)		(0x00482a0400000000L | IMP_UNSIGNED(v))
#define IMP_CMD_HQ_STACKPTR(v)		(0x00483a0400000000L | IMP_UNSIGNED(v))
#define IMP_CMD_HQ_TXBASE(p)		(0x00482008 | ((p) << 9))

#define IMP_CMD_HQ_DMACTRL_0(v)		(0x00080b04000000b1L | ((v) & 8))

#define IMP_CMD_HQ_TXMAX(p, v)					\
	(0x0048300400000000L | IMP_UNSIGNED(v) |		\
	 ((unsigned long)(p) << 40))

#define IMP_CMD_HQ_PGBITS(p, v)					\
	(0x00482b0400000000L | IMP_UNSIGNED(v) |		\
	 ((unsigned long)(p) << 40))

#define IMP_CMD_HQ_484A(p, v)					\
	((0x00484a0400000000L + ((unsigned long)(p) << 40)) |	\
	 IMP_UNSIGNED(v))

#define IMP_CMD_HQ_484B(p, v)					\
	((0x00484b0400000000L + ((unsigned long)(p) << 40)) |	\
	 IMP_UNSIGNED(v))

/*
 * Logic operations for the PP1:
 *    - SI = source invert
 *    - DI = dest invert
 *    - RI = result invert
 */
#define IMP_LO_AND			0x01
#define IMP_LO_SI_AND			0x04
#define IMP_LO_DI_AND			0x02
#define IMP_LO_RI_AND			0x0e

#define IMP_LO_OR			0x07
#define IMP_LO_SI_OR			0x0d
#define IMP_LO_DI_OR			0x0b
#define IMP_LO_RI_OR			0x08

#define IMP_LO_XOR			0x06
#define IMP_LO_RI_XOR			0x09

#define IMP_LO_NOP			0x05
#define IMP_LO_RI_NOP			0x0a

#define IMP_LO_COPY			0x03
#define IMP_LO_RI_COPY			0x0c

#define IMP_LO_CLEAR			0x00
#define IMP_LO_SET			0x0f

/* Blending factors */
#define IMP_BLEND_ALPHA			0x0704c900

#endif /* _IMPACT_H */
