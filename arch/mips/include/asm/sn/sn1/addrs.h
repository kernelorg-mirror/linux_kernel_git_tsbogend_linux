/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 1992 - 1997, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 by Colin Ngam
 */
#ifndef _ASM_SN_SN1_ADDRS_H
#define _ASM_SN_SN1_ADDRS_H

/*
 * SN1 (on a TRex) Address map
 *
 * This file contains a set of definitions and macros which are used
 * to reference into the major address spaces (CAC, HSPEC, IO, MSPEC,
 * and UNCAC) used by the SN1 architecture.  It also contains addresses
 * for "major" statically locatable PROM/Kernel data structures, such as
 * the partition table, the configuration data structure, etc.
 * We make an implicit assumption that the processor using this file
 * follows the R12K's provisions for specifying uncached attributes;
 * should this change, the base registers may very well become processor-
 * dependent.
 *
 * For more information on the address spaces, see the "Local Resources"
 * chapter of the Hub specification.
 *
 * NOTE: This header file is included both by C and by assembler source
 *	 files.  Please bracket any language-dependent definitions
 *	 appropriately.
 */


/*
 * Some of the macros here need to be casted to appropriate types when used
 * from C.  They definitely must not be casted from assembly language so we
 * use some new ANSI preprocessor stuff to paste these on where needed.
 */

/*
 * The following couple of definitions will eventually need to be variables,
 * since the amount of address space assigned to each node depends on
 * whether the system is running in N-mode (more nodes with less memory)
 * or M-mode (fewer nodes with more memory).  We expect that it will
 * be a while before we need to make this decision dynamically, though,
 * so for now we just use defines bracketed by an ifdef.
 */

#ifdef CONFIG_SGI_SN_N_MODE

#define NODE_SIZE_BITS		32
#define BWIN_SIZE_BITS		28

#define NASID_BITS		8
#define NASID_BITMASK		(0xffLL)
#define NASID_SHFT		32
#define NASID_META_BITS		1
#define NASID_LOCAL_BITS	7

#define BDDIR_UPPER_MASK	(UINT64_CAST 0x1ffffff << 4)
#define BDECC_UPPER_MASK	(UINT64_CAST 0x1fffffff)

#else /* !defined(CONFIG_SGI_SN_N_MODE), assume that M-mode is desired */

#define NODE_SIZE_BITS		33
#define BWIN_SIZE_BITS		29

#define NASID_BITMASK		(0x7fLL)
#define NASID_BITS		7
#define NASID_SHFT		33
#define NASID_META_BITS		0
#define NASID_LOCAL_BITS	7

#define BDDIR_UPPER_MASK	(UINT64_CAST 0x3ffffff << 4)
#define BDECC_UPPER_MASK	(UINT64_CAST 0x3fffffff)

#endif /* defined(CONFIG_SGI_SN_N_MODE) */

#define NODE_ADDRSPACE_SIZE	(UINT64_CAST 1 << NODE_SIZE_BITS)

#define NASID_MASK		(UINT64_CAST NASID_BITMASK << NASID_SHFT)
#define NASID_GET(_pa)		(int)((UINT64_CAST(_pa) >>		\
					NASID_SHFT) & NASID_BITMASK)

#ifndef __ASSEMBLY__
#define NODE_SWIN_BASE(nasid, widget)					\
	((widget == 0) ? NODE_BWIN_BASE((nasid), SWIN0_BIGWIN)		\
	: RAW_NODE_SWIN_BASE(nasid, widget))
#else
#define NODE_SWIN_BASE(nasid, widget) \
	(NODE_IO_BASE(nasid) + (UINT64_CAST(widget) << SWIN_SIZE_BITS))
#endif /* __ASSEMBLY__ */

/*
 * The following definitions pertain to the IO special address
 * space.  They define the location of the big and little windows
 * of any given node.
 */

#define BWIN_INDEX_BITS		3
#define BWIN_SIZE		(UINT64_CAST 1 << BWIN_SIZE_BITS)
#define	BWIN_SIZEMASK		(BWIN_SIZE - 1)
#define	BWIN_WIDGET_MASK	0x7
#define NODE_BWIN_BASE0(nasid)	(NODE_IO_BASE(nasid) + BWIN_SIZE)
#define NODE_BWIN_BASE(nasid, bigwin)	(NODE_BWIN_BASE0(nasid) +	\
				(UINT64_CAST(bigwin) << BWIN_SIZE_BITS))

#define	BWIN_WIDGETADDR(addr)	((addr) & BWIN_SIZEMASK)
#define	BWIN_WINDOWNUM(addr)	(((addr) >> BWIN_SIZE_BITS) & BWIN_WIDGET_MASK)
/*
 * Verify if addr belongs to large window address of node with "nasid"
 *
 *
 * NOTE: "addr" is expected to be XKPHYS address, and NOT physical
 * address
 *
 *
 */

#define	NODE_BWIN_ADDR(nasid, addr)	\
		(((addr) >= NODE_BWIN_BASE0(nasid)) && \
		 ((addr) < (NODE_BWIN_BASE(nasid, HUB_NUM_BIG_WINDOW) + \
				BWIN_SIZE)))

/*
 * The following define the major position-independent aliases used
 * in SN1.
 *	CALIAS -- Varies in size, points to the first n bytes of memory
 *		  on the reader's node.
 */

#define CALIAS_BASE		CAC_BASE


/*
 * needed by symmon so it needs to be outside #if PROM
 * (see also POD_ELSCSIZE)
 */
#define IP27PROM_ELSC_BASE_A	PHYS_TO_K0(0x020e0000)
#define IP27PROM_ELSC_BASE_B	PHYS_TO_K0(0x020e0800)
#define IP27PROM_ELSC_BASE_C	PHYS_TO_K0(0x020e1000)
#define IP27PROM_ELSC_BASE_D	PHYS_TO_K0(0x020e1800)
#define IP27PROM_ELSC_SHFT	11
#define IP27PROM_ELSC_SIZE	(1 << IP27PROM_ELSC_SHFT)

#define FREEMEM_BASE		PHYS_TO_K0(0x4000000)

#define IO6PROM_STACK_SHFT	14	/* stack per cpu */
#define IO6PROM_STACK_SIZE	(1 << IO6PROM_STACK_SHFT)


#define KL_UART_BASE	LOCAL_HSPEC(HSPEC_UART_0)	/* base of UART regs */
#define KL_UART_CMD	LOCAL_HSPEC(HSPEC_UART_0)	/* UART command reg */
#define KL_UART_DATA	LOCAL_HSPEC(HSPEC_UART_1)	/* UART data reg */

#ifndef __ASSEMBLY__
/* Address 0x400 to 0x1000 ualias points to cache error eframe + misc
 * CACHE_ERR_SP_PTR could either contain an address to the stack, or
 * the stack could start at CACHE_ERR_SP_PTR
 */
#define CACHE_ERR_EFRAME	0x400

#define CACHE_ERR_ECCFRAME	(CACHE_ERR_EFRAME + EF_SIZE)
#define CACHE_ERR_SP_PTR	(0x1000 - 32)	/* why -32? TBD */
#define CACHE_ERR_IBASE_PTR	(0x1000 - 40)
#define CACHE_ERR_SP		(CACHE_ERR_SP_PTR - 16)
#define CACHE_ERR_AREA_SIZE	(ARCS_SPB_OFFSET - CACHE_ERR_EFRAME)

#endif	/* !__ASSEMBLY__ */

#define IP27PROM_LAUNCHSLAVE		PHYS_TO_COMPATK1(0x1fc00038)

#endif /* _ASM_SN_SN1_ADDRS_H */
