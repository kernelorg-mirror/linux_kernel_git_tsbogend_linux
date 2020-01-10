/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 1992 - 1997, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 by Colin Ngam
 */

#ifndef _ASM_SN_SN1_BEDROCK_H
#define _ASM_SN_SN1_BEDROCK_H

/* The secret password; used to release protection */
#define HUB_PASSWORD		0x53474972756c6573ull

#define CHIPID_HUB		0x3012
#define CHIPID_ROUTER		0x3017

#define BEDROCK_REV_1_0		1
#define BEDROCK_REV_1_1		2

#define MAX_HUB_PATH		80

#include <asm/sn/arch.h>
#include <asm/sn/sn1/addrs.h>
#include <asm/sn/sn1/hubpi.h>
#include <asm/sn/sn1/hubmd.h>
#include <asm/sn/sn1/hubio.h>
#include <asm/sn/sn1/hubni.h>
#include <asm/sn/sn1/hublb.h>
#include <asm/sn/sn1/hubxb.h>
#include <asm/sn/sn1/hubpi_next.h>
#include <asm/sn/sn1/hubmd_next.h>
#include <asm/sn/sn1/hubio_next.h>
#include <asm/sn/sn1/hubni_next.h>
#include <asm/sn/sn1/hublb_next.h>
#include <asm/sn/sn1/hubxb_next.h>

/* Translation of uncached attributes */
#define	UATTR_HSPEC	0
#define	UATTR_IO	1
#define	UATTR_MSPEC	2
#define	UATTR_UNCAC	3

#ifdef __ASSEMBLY__
/*
 * Returns the local nasid into res.
 */
	.macro GET_NASID_ASM res
	dli	\res, LOCAL_HUB_ADDR(LB_REV_ID)
	ld	\res, (\res)
	and	\res, LRI_NODEID_MASK
	dsrl	\res, LRI_NODEID_SHFT
	.endm
#else
/*
 * get_nasid() returns the physical node id number of the caller.
 */
static inline nasid_t get_nasid(void)
{
	return (nasid_t)((LOCAL_HUB_L(LB_REV_ID) & LRI_NODEID_MASK)
					>> LRI_NODEID_SHFT);
}
#endif


#endif /* _ASM_SN_SN1_BEDROCK_H */
