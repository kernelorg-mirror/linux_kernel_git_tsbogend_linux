/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 1992 - 1997, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 by Colin Ngam
 */

#ifndef _ASM_SN_SN1_HUBNI_NEXT_H
#define _ASM_SN_SN1_HUBNI_NEXT_H

#define NI_LOCAL_ENTRIES        128
#define NI_META_ENTRIES        1

#define NI_LOCAL_TABLE(_x)      (NI_LOCAL_TABLE_0 + (8 * (_x)))
#define NI_META_TABLE(_x)       (NI_GLOBAL_TABLE + (8 * (_x)))

/*
 * Masks and shifts for NI registers are defined below.
 */

#define NPS_LINKUP_SHFT		1
#define NPS_LINKUP_MASK		BIT_ULL(1)


#define NPR_LOCALRESET		BIT_ULL(2)	/* Reset loc. bdrck */
#define NPR_PORTRESET		BIT_ULL(1)	/* Send warm reset  */
#define NPR_LINKRESET		BIT_ULL(0)	/* Send link reset  */

/* NI_DIAG_PARMS bit definitions */
#define NDP_SENDERROR		BIT_ULL(0)   /* Send data error  */
#define NDP_PORTDISABLE		BIT_ULL(1)   /* Port disable     */
#define NDP_SENDERROFF		BIT_ULL(2)   /* Disable send error recovery */

/* NI_PORT_ERROR mask and shift definitions (some are not present in SN0) */

#define NPE_LINKRESET		(UINT64_CAST 1 << 52)
#define NPE_INTLONG_SHFT	48
#define NPE_INTLONG_MASK	(UINT64_CAST 0xf << NPE_INTLONG_SHFT)
#define NPE_INTSHORT_SHFT	44
#define NPE_INTSHORT_MASK	(UINT64_CAST 0xf << NPE_INTSHORT_SHFT)
#define NPE_EXTBADHEADER_SHFT	40
#define NPE_EXTBADHEADER_MASK	(UINT64_CAST 0xf << NPE_EXTBADHEADER_SHFT)
#define NPE_EXTLONG_SHFT	36
#define NPE_EXTLONG_MASK	(UINT64_CAST 0xf << NPE_EXTLONG_SHFT)
#define NPE_EXTSHORT_SHFT	32
#define NPE_EXTSHORT_MASK	(UINT64_CAST 0xf << NPE_EXTSHORT_SHFT)
#define NPE_FIFOOVFLOW_SHFT	28
#define NPE_FIFOOVFLOW_MASK	(UINT64_CAST 0xf << NPE_FIFOOVFLOW_SHFT)
#define NPE_TAILTO_SHFT		24
#define NPE_TAILTO_MASK		(UINT64_CAST 0xf << NPE_TAILTO_SHFT)
#define NPE_RETRYCOUNT_SHFT	16
#define NPE_RETRYCOUNT_MASK	(UINT64_CAST 0xff << NPE_RETRYCOUNT_SHFT)
#define NPE_CBERRCOUNT_SHFT	8
#define NPE_CBERRCOUNT_MASK	(UINT64_CAST 0xff << NPE_CBERRCOUNT_SHFT)
#define NPE_SNERRCOUNT_SHFT	0
#define NPE_SNERRCOUNT_MASK	(UINT64_CAST 0xff << NPE_SNERRCOUNT_SHFT)

#define NPE_COUNT_MAX		0xff

#define NPE_FATAL_ERRORS	(NPE_LINKRESET | NPE_INTLONG_MASK |\
				 NPE_INTSHORT_MASK | NPE_EXTBADHEADER_MASK |\
				 NPE_EXTLONG_MASK | NPE_EXTSHORT_MASK |\
				 NPE_FIFOOVFLOW_MASK | NPE_TAILTO_MASK)

/* NI_PORT_HEADER[AB] registers (not automatically generated) */

#ifndef __ASSEMBLY__

union ni_port_header_a_u {
	u64	ni_port_header_a_regval;
	struct	{
		u64	pha_reserved		  :	 2;
		u64	pha_address_b		  :	27;
		u64	pha_prexsel		  :	 3;
		u64	pha_command		  :	 8;
		u64	pha_reserved_1		  :	 3;
		u64	pha_destination		  :	 8;
		u64	pha_direction		  :	 4;
		u64	pha_age			  :	 8;
		u64	pha_v			  :	 1;
	} ni_port_header_a_fld_s;
};

union ni_port_header_b_u {
	u64	ni_port_header_b_regval;
	struct	{
		u64	phb_reserved		  :	18;
		u64	phb_address_c		  :	 8;
		u64	phb_address_a		  :	 3;
		u64	phb_reserved_1		  :	 8;
		u64	phb_source		  :	11;
		u64	phb_reserved_2		  :	 5;
		u64	phb_supplemental	   :	11;
	} ni_port_header_b_fld_s;
};

#endif /* __ASSEMBLY */

/* NI_RESET_ENABLE mask definitions */

#define NRE_RESETOK		(UINT64_CAST 1)	/* Let LLP reset bedrock */

/* NI PORT_ERRORS, Max number of RETRY_COUNT, Check Bit, and Sequence   */
/* Number errors (8 bit counters that do not wrap).                     */
#define NI_LLP_RETRY_MAX        0xff
#define NI_LLP_CB_MAX           0xff
#define NI_LLP_SN_MAX           0xff

/* NI_PORT_PARMS shift and mask definitions */

#define NPP_VCH_ERR_EN_SHFT	31
#define NPP_VCH_ERR_EN_MASK	(0xf << NPP_VCH_ERR_EN_SHFT)
#define NPP_SQUASH_ERR_EN_SHFT	30
#define NPP_SQUASH_ERR_EN_MASK	(0x1 << NPP_SQUASH_ERR_EN_SHFT)
#define NPP_FIRST_ERR_EN_SHFT	29
#define NPP_FIRST_ERR_EN_MASK	(0x1 << NPP_FIRST_ERR_EN_SHFT)
#define NPP_D_AVAIL_SEL_SHFT	26
#define NPP_D_AVAIL_SEL_MASK	(0x3 << NPP_D_AVAIL_SEL_SHFT)
#define NPP_MAX_RETRY_SHFT	16
#define NPP_MAX_RETRY_MASK	(0x3ff << NPP_MAX_RETRY_SHFT)
#define NPP_NULL_TIMEOUT_SHFT	10
#define NPP_NULL_TIMEOUT_MASK	(0x3f << NPP_NULL_TIMEOUT_SHFT)
#define NPP_MAX_BURST_SHFT	0
#define NPP_MAX_BURST_MASK	(0x3ff << NPP_MAX_BURST_SHFT)

#define NPP_RESET_DEFAULTS	(0xf << NPP_VCH_ERR_EN_SHFT |   \
				 0x1 << NPP_FIRST_ERR_EN_SHFT | \
				 0x3ff << NPP_MAX_RETRY_SHFT |  \
				 0x6 << NPP_NULL_TIMEOUT_SHFT | \
				 0x3f0 << NPP_MAX_BURST_SHFT)

#endif  /* _ASM_SN_SN1_HUBNI_NEXT_H */
