/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 1992 - 1997, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 by Colin Ngam
 */

#ifndef _ASM_SN_SN1_HUBLB_H
#define _ASM_SN_SN1_HUBLB_H


#define    LB_REV_ID                 0x00600000    /*
						    * Bedrock Revision
						    * and ID
						    */
#define    LB_CPU_PERMISSION         0x00604000    /*
						    * CPU PIO access
						    * permission bits
						    */
#define    LB_CPU_PERM_OVRRD         0x00604008    /*
						    * CPU PIO access
						    * permission bit
						    * override
						    */
#define    LB_IO_PERMISSION          0x00604010    /*
						    * IO PIO access
						    * permission bits
						    */
#define    LB_SOFT_RESET             0x00604018    /*
						    * Soft reset the
						    * Bedrock chip
						    */
#define    LB_REGION_PRESENT         0x00604020    /*
						    * Regions Present for
						    * Invalidates
						    */
#define    LB_NODES_ABSENT           0x00604028    /*
						    * Nodes Absent for
						    * Invalidates
						    */
#define    LB_MICROLAN_CTL           0x00604030    /*
						    * Microlan Control
						    * (NIC)
						    */
#define    LB_ERROR_BITS             0x00604040    /*
						    * Local Block error
						    * bits
						    */
#define    LB_ERROR_MASK_CLR         0x00604048    /*
						    * Bit mask write to
						    * clear error bits
						    */
#define    LB_ERROR_HDR1             0x00604050    /*
						    * Source, Suppl and
						    * Cmd fields
						    */
#define    LB_ERROR_HDR2             0x00604058    /*
						    * Address field from
						    * first error
						    */
#define    LB_ERROR_DATA             0x00604060    /*
						    * Data flit (if any)
						    * from first error
						    */
#define    LB_DEBUG_SELECT           0x00604100    /*
						    * Choice of debug
						    * signals from chip
						    */
#define    LB_DEBUG_PINS             0x00604108    /*
						    * Value on the chip's
						    * debug pins
						    */
#define    LB_RT_LOCAL_CTRL          0x00604200    /*
						    * Local generation of
						    * real-time clock
						    */
#define    LB_RT_FILTER_CTRL         0x00604208    /*
						    * Control of
						    * filtering of global
						    * clock
						    */
#define    LB_SCRATCH_REG0           0x00608000    /* Scratch Register 0     */
#define    LB_SCRATCH_REG1           0x00608008    /* Scratch Register 1     */
#define    LB_SCRATCH_REG2           0x00608010    /* Scratch Register 2     */
#define    LB_SCRATCH_REG3           0x00608018    /* Scratch Register 3     */
#define    LB_SCRATCH_REG4           0x00608020    /* Scratch Register 4     */
#define    LB_SCRATCH_REG0_WZ        0x00608040    /*
						    * Scratch Register 0
						    * (WZ alias)
						    */
#define    LB_SCRATCH_REG1_WZ        0x00608048    /*
						    * Scratch Register 1
						    * (WZ alias)
						    */
#define    LB_SCRATCH_REG2_WZ        0x00608050    /*
						    * Scratch Register 2
						    * (WZ alias)
						    */
#define    LB_SCRATCH_REG3_RZ        0x00608058    /*
						    * Scratch Register 3
						    * (RZ alias)
						    */
#define    LB_SCRATCH_REG4_RZ        0x00608060    /*
						    * Scratch Register 4
						    * (RZ alias)
						    */
#define    LB_VECTOR_PARMS           0x0060C000    /*
						    * Vector PIO
						    * parameters
						    */
#define    LB_VECTOR_ROUTE           0x0060C008    /*
						    * Vector PIO Vector
						    * Route
						    */
#define    LB_VECTOR_DATA            0x0060C010    /*
						    * Vector PIO Write
						    * Data
						    */
#define    LB_VECTOR_STATUS          0x0060C020    /*
						    * Vector PIO Return
						    * Status
						    */
#define    LB_VECTOR_RETURN          0x0060C028    /*
						    * Vector PIO Return
						    * Route
						    */
#define    LB_VECTOR_READ_DATA       0x0060C030    /*
						    * Vector PIO Read
						    * Data
						    */
#define    LB_VECTOR_STATUS_CLEAR    0x0060C038    /*
						    * Clear Vector PIO
						    * Return Status
						    */

#ifndef __ASSEMBLY__

/************************************************************************
 *                                                                      *
 * Description:  This register contains information that allows         *
 * exploratory software to probe for chip type. This is also the        *
 * register that sets this node's ID and the size of each region        *
 * (which affects the maximum possible system size). IBM assigns the    *
 * values for the REVISION, PART_NUMBER and MANUFACTURER fields, in     *
 * accordance with the IEEE 1149.1 standard; SGI is not at liberty to   *
 * unilaterally change the values of these fields.                      *
 *  .                                                                   *
 *                                                                      *
 ************************************************************************/

union lb_rev_id_u {
	u64       lb_rev_id_regval;
	struct	{
		u64	ri_reserved		  :	16;
		u64	ri_region_size		  :	 2;
		u64	ri_reserved_1		  :	 6;
		u64	ri_node_id		  :	 8;
		u64	ri_revision		  :	 4;
		u64	ri_part_number		  :	16;
		u64	ri_manufacturer		  :	11;
		u64	ri_reserved_2		  :	 1;
	} lb_rev_id_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register contains the PI-access-rights bit-vector for the      *
 * LB, NI, XB and MD portions of the Bedrock local register space. If   *
 * a bit in the bit-vector is set, the region corresponding to that     *
 * bit has read/write permission on the LB, NI, XB and MD local         *
 * registers. If the bit is clear, that region has no write access to   *
 * the local registers and no read access if the read will cause any    *
 * state change. If a write or a read with side effects is attempted    *
 * by a PI in a region for which access is restricted, the LB will      *
 * not perform the operation and will send back a reply which           *
 * indicates an error.                                                  *
 *                                                                      *
 ************************************************************************/

union lb_cpu_permission_u {
	u64	lb_cpu_permission_regval;
	struct  {
		u64	cp_cpu_access             :	64;
	} lb_cpu_permission_fld_s;
};

/************************************************************************
 *                                                                      *
 *  A write to this register of the 64-bit value "SGIrules" will        *
 * cause the bit in the LB_CPU_PROTECT register corresponding to the    *
 * region of the requester to be set.                                   *
 *                                                                      *
 ************************************************************************/

union lb_cpu_perm_ovrrd_u {
	u64	lb_cpu_perm_ovrrd_regval;
	struct  {
		u64	cpo_cpu_perm_ovr          :	64;
	} lb_cpu_perm_ovrrd_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register contains the II-access-rights bit-vector for the      *
 * LB, NI, XB and MD portions of the Bedrock local register space. If   *
 * a bit in the bit-vector is set, the region corresponding to that     *
 * bit has read/write permission on the LB, NI, XB and MD local         *
 * registers. If the bit is clear, then that region has no write        *
 * access to the local registers and no read access if the read         *
 * results in any state change. If a write or a read with side          *
 * effects is attempted by an II in a region for which access is        *
 * restricted, the LB will not perform the operation and will send      *
 * back a reply which indicates an error.                               *
 *                                                                      *
 ************************************************************************/

union lb_io_permission_u {
	u64	lb_io_permission_regval;
	struct  {
		u64	ip_io_permission          :	64;
	} lb_io_permission_fld_s;
};

/************************************************************************
 *                                                                      *
 *  A write to this bit resets the Bedrock chip with a soft reset.      *
 *                                                                      *
 ************************************************************************/

union lb_soft_reset_u {
	u64	lb_soft_reset_regval;
	struct	{
		u64	sr_reserved		  :	63;
		u64	sr_soft_reset		  :	 1;
	} lb_soft_reset_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register indicates which regions are present and capable of    *
 * receiving an invalidate (INVAL) request. The LB samples this         *
 * register at the start of processing each LINVAL. When an LINVAL      *
 * indicates that a particular PI unit might hold a shared copy of a    *
 * cache block but this PI is in a region which is not present (i.e.,   *
 * its bit in LB_REGION_PRESENT is clear), then the LB sends an IVACK   *
 * reply packet on behalf of this PI. The REGION_SIZE field in the      *
 * LB_REV_ID register determines the number of nodes per region (and    *
 * hence, the number of PI units which share a common bit in the        *
 * LB_REGION_PRESENT register).                                         *
 *                                                                      *
 ************************************************************************/

union lb_region_present_u {
	u64	lb_region_present_regval;
	struct  {
		u64	rp_present_bits           :	64;
	} lb_region_present_fld_s;
};

/************************************************************************
 *                                                                      *
 * Description:  This register indicates which nodes are absent and     *
 * not capable of receiving an invalidate (INVAL) request. The LB       *
 * samples this register at the start of processing each LINVAL. When   *
 * an LINVAL indicates that a particular PI unit might hold a shared    *
 * copy of a cache block but this PI unit's node is not present         *
 * (i.e., its node ID is listed in the LB_NODES_ABSENT register),       *
 * then the LB sends an IVACK reply packet on behalf of this PI.        *
 *                                                                      *
 *                                                                      *
 ************************************************************************/

union lb_nodes_absent_u {
	u64	lb_nodes_absent_regval;
	struct	{
		u64	na_node_3_valid		  :	 1;
		u64	na_reserved		  :	 7;
		u64	na_node_3		  :	 8;
		u64	na_node_2_valid		  :	 1;
		u64	na_reserved_1		  :	 7;
		u64	na_node_2		  :	 8;
		u64	na_node_1_valid		  :	 1;
		u64	na_reserved_2		  :	 7;
		u64	na_node_1		  :	 8;
		u64	na_node_0_valid		  :	 1;
		u64	na_reserved_3		  :	 7;
		u64	na_node_0		  :	 8;
	} lb_nodes_absent_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register provides access to the Number-In-a-Can add-only       *
 * serial PROM that is used to store node board serial number and       *
 * configuration information. (Refer to NIC datasheet Dallas 1990A      *
 * that is viewable at                                                  *
 * URL::http://www.dalsemi.com/DocControl/PDFs/pdfindex.html). Data     *
 * comes from this interface LSB first.                                 *
 *                                                                      *
 ************************************************************************/

union lb_microlan_ctl_u {
	u64       lb_microlan_ctl_regval;
	struct  {
		u64       mc_reserved               :     30;
		u64       mc_clkdiv_phi1            :      7;
		u64       mc_clkdiv_phi0            :      7;
		u64       mc_pulse                  :     10;
		u64       mc_sample                 :      8;
		u64       mc_done                   :      1;
		u64       mc_rd_data                :      1;
	} lb_microlan_ctl_fld_s;
};

/************************************************************************
 *                                                                      *
 * Description:  This register contains the LB error status bits.       *
 * Whenever a particular type of error occurs, the LB sets its bit in   *
 * this register so that software will be aware that such an event      *
 * has happened. Reads from this register are non-destructive and the   *
 * contents of this register remain intact across reset operations.     *
 * Whenever any of these bits is set, the LB will assert its            *
 * interrupt request output signals that go to the PI units.            *
 *  Software can simulate the occurrence of an error by first writing   *
 * appropriate values into the LB_ERROR_HDR1, LB_ERROR_HDR2 and         *
 * LB_ERROR_DATA registers, and then writing to the LB_ERROR_BITS       *
 * register to set the error bits in a particular way. Setting one or   *
 * more error bits will cause the LB to interrupt a processor and       *
 * invoke error-handling software.                                      *
 *                                                                      *
 ************************************************************************/

union lb_error_bits_u {
	u64	lb_error_bits_regval;
	struct	{
		u64	eb_reserved		  :	53;
		u64	eb_gclk_drop		  :	 1;
		u64	eb_rq_bad_linval	  :	 1;
		u64	eb_rq_bad_addr		  :	 1;
		u64	eb_rp_bad_data		  :	 1;
		u64	eb_rq_bad_data		  :	 1;
		u64	eb_rp_long		  :	 1;
		u64	eb_rq_long		  :	 1;
		u64	eb_rp_short		  :	 1;
		u64	eb_rq_short		  :	 1;
		u64	eb_rp_bad_cmd		  :	 1;
		u64	eb_rq_bad_cmd		  :	 1;
	} lb_error_bits_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register lets software clear some of the bits in the           *
 * LB_ERROR_BITS register without affecting other bits.  Essentially,   *
 * it provides bit mask functionality. When software writes to the      *
 * LB_ERROR_MASK_CLR register, the bits which are set in the data       *
 * value indicate which bits are to be cleared in LB_ERROR_BITS. If a   *
 * bit is clear in the data value written to the LB_ERROR_MASK_CLR      *
 * register, then its corresponding bit in the LB_ERROR_BITS register   *
 * is not affected. Hence, software can atomically clear any subset     *
 * of the error bits in the LB_ERROR_BITS register.                     *
 *                                                                      *
 ************************************************************************/

union lb_error_mask_clr_u {
	u64	lb_error_mask_clr_regval;
	struct	{
		u64	emc_reserved		  :	53;
		u64	emc_clr_gclk_drop	  :	 1;
		u64	emc_clr_rq_bad_linval	  :	 1;
		u64	emc_clr_rq_bad_addr	  :	 1;
		u64	emc_clr_rp_bad_data	  :	 1;
		u64	emc_clr_rq_bad_data	  :	 1;
		u64	emc_clr_rp_long		  :	 1;
		u64	emc_clr_rq_long		  :	 1;
		u64	emc_clr_rp_short	  :	 1;
		u64	emc_clr_rq_short	  :	 1;
		u64	emc_clr_rp_bad_cmd	  :	 1;
		u64	emc_clr_rq_bad_cmd	  :	 1;
	} lb_error_mask_clr_fld_s;
};

/************************************************************************
 *                                                                      *
 *  If the LB detects an error when VALID==0 in the LB_ERROR_HDR1       *
 * register, then it saves the contents of the offending packet's       *
 * header flit in the LB_ERROR_HDR1 and LB_ERROR_HDR2 registers, sets   *
 * the VALID bit in LB_ERROR_HDR1 and clears the OVERRUN bit in         *
 * LB_ERROR_HDR1 (and it will also set the corresponding bit in the     *
 * LB_ERROR_BITS register). The ERR_TYPE field indicates specifically   *
 * what kind of error occurred.  Its encoding corresponds to the bit    *
 * positions in the LB_ERROR_BITS register (e.g., ERR_TYPE==5           *
 * indicates a RP_LONG error).  If an error (of any type except         *
 * GCLK_DROP) subsequently happens while VALID==1, then the LB sets     *
 * the OVERRUN bit in LB_ERROR_HDR1. This register is not relevant      *
 * when a GCLK_DROP error occurs; the LB does not even attempt to       *
 * change the ERR_TYPE, VALID or OVERRUN field when a GCLK_DROP error   *
 * happens.                                                             *
 *                                                                      *
 ************************************************************************/

union lb_error_hdr1_u {
	u64	lb_error_hdr1_regval;
	struct	{
		u64	eh_reserved		  :	19;
		u64	eh_valid		  :	 1;
		u64	eh_reserved_1		  :	 3;
		u64	eh_overrun		  :	 1;
		u64	eh_reserved_2		  :	 4;
		u64	eh_err_type		  :	 4;
		u64	eh_reserved_3		  :	 1;
		u64	eh_source		  :	11;
		u64	eh_reserved_4		  :	 1;
		u64	eh_suppl		  :	11;
		u64	eh_reserved_5		  :	 1;
		u64	eh_command		  :	 7;
	} lb_error_hdr1_fld_s;
};

/************************************************************************
 *                                                                      *
 *  Contents of the Address field from header flit of first packet      *
 * that causes an error. This register is not relevant when a           *
 * GCLK_DROP error occurs.                                              *
 *                                                                      *
 ************************************************************************/

union lb_error_hdr2_u {
	u64	lb_error_hdr2_regval;
	struct	{
		u64	eh_reserved		  :	26;
		u64	eh_address		  :	38;
	} lb_error_hdr2_fld_s;
};

/************************************************************************
 *                                                                      *
 * Description:  This register accompanies the LB_ERROR_HDR1 and        *
 * LB_ERROR_HDR2 registers.  The LB updates the value in this           *
 * register when an incoming packet with a data flit causes an error    *
 * while VALID==0 in the LB_ERROR_HDR1 register.  This register         *
 * retains the contents of the data flit from the incoming packet       *
 * that caused the error. This register is relevant for the following   *
 * types of errors:                                                     *
 * <UL >                                                                *
 * <UL >                                                                *
 * <UL >                                                                *
 * <UL >                                                                *
 * <UL >                                                                *
 * <LI >RQ_BAD_LINVAL for a LINVAL request.                             *
 * <LI >RQ_BAD_ADDR for a normal or vector PIO request.                 *
 * <LI >RP_BAD_DATA for a vector PIO reply.                             *
 * <LI >RQ_BAD DATA for an incoming request with data.                  *
 * <LI >RP_LONG for a vector PIO reply.                                 *
 * <LI >RQ_LONG for an incoming request with expected data.             *
 * <BLOCKQUOTE >                                                        *
 * In the case of RQ_BAD_LINVAL, the register retains the 64-bit data   *
 * value that followed the header flit.  In the case of RQ_BAD_ADDR     *
 * or RQ_BAD_DATA, the register retains the incoming packet's 64-bit    *
 * data value (i.e., 2nd flit in the packet for a normal PIO write or   *
 * an LINVAL, 3rd flit for a vector PIO read or write). In the case     *
 * of RP_BAD_DATA, the register retains the 64-bit data value in the    *
 * 3rd flit of the packet. When a RP_LONG or RQ_LONG error occurs,      *
 * the LB loads the LB_ERROR_DATA register with the contents of the     *
 * expected data flit (i.e., the 3rd flit in the packet for a vector    *
 * PIO request or reply, the 2nd flit for other packets), if any. The   *
 * contents of the LB_ERROR_DATA register are undefined after a         *
 * RP_SHORT, RQ_SHORT, RP_BAD_CMD or RQ_BAD_CMD error. The contents     *
 * of the LB_ERROR_DATA register are also undefined after an incoming   *
 * normal PIO read request which encounters a RQ_LONG error.            *
 *                                                                      *
 ************************************************************************/

union lb_error_data_u {
	u64	lb_error_data_regval;
	struct  {
		u64	ed_data                   :	64;
	} lb_error_data_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register enables software to control what internal Bedrock     *
 * signals are visible on the chip's debug pins. The LB provides the    *
 * 6-bit value in this register to Bedrock's DEBUG unit. The JTAG       *
 * unit provides a similar 6-bit selection input to the DEBUG unit,     *
 * along with another signal that tells the DEBUG unit whether to use   *
 * the selection signal from the LB or the JTAG unit. For a             *
 * description of the menu of choices for debug signals, refer to the   *
 * documentation for the DEBUG unit.                                    *
 *                                                                      *
 ************************************************************************/

union lb_debug_select_u {
	u64	lb_debug_select_regval;
	struct	{
		u64	ds_reserved		  :	58;
		u64	ds_debug_sel		  :	 6;
	} lb_debug_select_fld_s;
};

/************************************************************************
 *                                                                      *
 *  A PIO read from this register returns the 32-bit value that is      *
 * currently on the Bedrock chip's debug pins. This register allows     *
 * software to observe debug pin output values which do not change      *
 * frequently (i.e., they remain constant over a period of many         *
 * cycles).                                                             *
 *                                                                      *
 ************************************************************************/

union lb_debug_pins_u {
	u64	lb_debug_pins_regval;
	struct	{
		u64	dp_reserved		  :	32;
		u64	dp_debug_pins		  :	32;
	} lb_debug_pins_fld_s;
};

/************************************************************************
 *                                                                      *
 *  The LB unit provides the PI0 and PI1 units with a real-time clock   *
 * signal. The LB can generate this signal itself, based on the         *
 * Bedrock chip's system clock which the LB receives as an input.       *
 * Alternatively, the LB can filter a global clock signal which it      *
 * receives as an input and provide the filtered version to PI0 and     *
 * PI1. The user can program the LB_RT_LOCAL_CTRL register to choose    *
 * the source of the real-time clock. If the user chooses to generate   *
 * the real-time clock internally within the LB, then the user can      *
 * specify the period for the real-time clock signal.                   *
 *                                                                      *
 ************************************************************************/

union lb_rt_local_ctrl_u {
	u64       lb_rt_local_ctrl_regval;
	struct  {
		u64       rlc_reserved              :     31;
		u64       rlc_use_internal          :      1;
		u64       rlc_reserved_1            :      3;
		u64       rlc_gclk                  :      1;
		u64       rlc_reserved_2            :      2;
		u64       rlc_gclk_counter          :     10;
		u64       rlc_reserved_3            :      2;
		u64       rlc_max_count             :     10;
		u64       rlc_reserved_4            :      3;
		u64       rlc_gclk_enable           :      1;
	} lb_rt_local_ctrl_fld_s;
};

/************************************************************************
 *                                                                      *
 *  When the value of the USE_INTERNAL field in the LB_RT_LOCAL_CTRL    *
 * register is 0, the LB filters an incoming global clock signal and    *
 * provides the result to PI0 and PI1 for their real-time clock         *
 * inputs. The LB can perform either simple filtering or complex        *
 * filtering, depending on the value of the MASK_ENABLE bit. For the    *
 * simple filtering option, the LB merely removes glitches from the     *
 * incoming global clock; if the global clock goes high (or low) for    *
 * only a single cycle, the LB considers it to be a glitch and does     *
 * not pass it through to PI0 and PI1. For the complex filtering        *
 * option, the LB expects positive edges on the incoming global clock   *
 * to be spaced at fairly regular intervals and it looks for them at    *
 * these times; the LB keeps track of unexpected or missing positive    *
 * edges, and it generates an edge itself whenever the incoming         *
 * global clock apparently misses an edge. For each filtering option,   *
 * the real-time clock which the LB provides to PI0 and PI1 is not      *
 * necessarily a square wave; when a positive edge happens, the         *
 * real-time clock stays high for (2*MAX_COUNT+1-OFFSET)/2 cycles of    *
 * the LB's system clock, and then is low until the next positive       *
 * edge.                                                                *
 *                                                                      *
 ************************************************************************/

union lb_rt_filter_ctrl_u {
	u64       lb_rt_filter_ctrl_regval;
	struct  {
		u64       rfc_reserved              :      6;
		u64       rfc_error_counter         :     10;
		u64       rfc_reserved_1            :      2;
		u64       rfc_dropout_thresh        :     10;
		u64       rfc_reserved_2            :      2;
		u64       rfc_dropout_counter       :     10;
		u64       rfc_reserved_3            :      3;
		u64       rfc_mask_enable           :      1;
		u64       rfc_mask_counter          :     12;
		u64       rfc_reserved_4            :      3;
		u64       rfc_offset                :      5;
	} lb_rt_filter_ctrl_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register is a scratch register that is reset to 0x0. At the    *
 * normal address, the register is a simple storage location. At the    *
 * Write-If-Zero address, the register accepts a new value from a       *
 * write operation only if the current value is zero.                   *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg0_u {
	u64	lb_scratch_reg0_regval;
	struct  {
		u64	sr_scratch_bits           :	64;
	} lb_scratch_reg0_fld_s;
};

/************************************************************************
 *                                                                      *
 *  These registers are scratch registers that are not reset. At a      *
 * register's normal address, it is a simple storage location. At a     *
 * register's Write-If-Zero address, it accepts a new value from a      *
 * write operation only if the current value is zero.                   *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg1_u {
	u64	lb_scratch_reg1_regval;
	struct  {
		u64	sr_scratch_bits           :	64;
	} lb_scratch_reg1_fld_s;
};

/************************************************************************
 *                                                                      *
 *  These registers are scratch registers that are not reset. At a      *
 * register's normal address, it is a simple storage location. At a     *
 * register's Write-If-Zero address, it accepts a new value from a      *
 * write operation only if the current value is zero.                   *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg2_u {
	u64	lb_scratch_reg2_regval;
	struct  {
		u64	sr_scratch_bits           :	64;
	} lb_scratch_reg2_fld_s;
};

/************************************************************************
 *                                                                      *
 *  These one-bit registers are scratch registers. At a register's      *
 * normal address, it is a simple storage location. At a register's     *
 * Read-Set-If-Zero address, it returns the original contents and       *
 * sets the bit if the original value is zero.                          *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg3_u {
	u64	lb_scratch_reg3_regval;
	struct	{
		u64	sr_reserved		  :	63;
		u64	sr_scratch_bit		  :	 1;
	} lb_scratch_reg3_fld_s;
};

/************************************************************************
 *                                                                      *
 *  These one-bit registers are scratch registers. At a register's      *
 * normal address, it is a simple storage location. At a register's     *
 * Read-Set-If-Zero address, it returns the original contents and       *
 * sets the bit if the original value is zero.                          *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg4_u {
	u64	lb_scratch_reg4_regval;
	struct	{
		u64	sr_reserved		  :	63;
		u64	sr_scratch_bit		  :	 1;
	} lb_scratch_reg4_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register is a scratch register that is reset to 0x0. At the    *
 * normal address, the register is a simple storage location. At the    *
 * Write-If-Zero address, the register accepts a new value from a       *
 * write operation only if the current value is zero.                   *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg0_wz_u {
	u64	lb_scratch_reg0_wz_regval;
	struct  {
		u64	srw_scratch_bits          :	64;
	} lb_scratch_reg0_wz_fld_s;
};

/************************************************************************
 *                                                                      *
 *  These registers are scratch registers that are not reset. At a      *
 * register's normal address, it is a simple storage location. At a     *
 * register's Write-If-Zero address, it accepts a new value from a      *
 * write operation only if the current value is zero.                   *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg1_wz_u {
	u64	lb_scratch_reg1_wz_regval;
	struct  {
		u64	srw_scratch_bits          :	64;
	} lb_scratch_reg1_wz_fld_s;
};

/************************************************************************
 *                                                                      *
 *  These registers are scratch registers that are not reset. At a      *
 * register's normal address, it is a simple storage location. At a     *
 * register's Write-If-Zero address, it accepts a new value from a      *
 * write operation only if the current value is zero.                   *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg2_wz_u {
	u64	lb_scratch_reg2_wz_regval;
	struct  {
		u64	srw_scratch_bits          :	64;
	} lb_scratch_reg2_wz_fld_s;
};

/************************************************************************
 *                                                                      *
 *  These one-bit registers are scratch registers. At a register's      *
 * normal address, it is a simple storage location. At a register's     *
 * Read-Set-If-Zero address, it returns the original contents and       *
 * sets the bit if the original value is zero.                          *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg3_rz_u {
	u64	lb_scratch_reg3_rz_regval;
	struct	{
		u64	srr_reserved		  :	63;
		u64	srr_scratch_bit		  :	 1;
	} lb_scratch_reg3_rz_fld_s;
};

/************************************************************************
 *                                                                      *
 *  These one-bit registers are scratch registers. At a register's      *
 * normal address, it is a simple storage location. At a register's     *
 * Read-Set-If-Zero address, it returns the original contents and       *
 * sets the bit if the original value is zero.                          *
 *                                                                      *
 ************************************************************************/

union lb_scratch_reg4_rz_u {
	u64	lb_scratch_reg4_rz_regval;
	struct	{
		u64	srr_reserved		  :	63;
		u64	srr_scratch_bit		  :	 1;
	} lb_scratch_reg4_rz_fld_s;
};

/************************************************************************
 *                                                                      *
 * Description:  This register contains vector PIO parameters. A        *
 * write to this register triggers the LB to send out a vector PIO      *
 * request packet. Immediately after servicing a write request to the   *
 * LB_VECTOR_PARMS register, the LB sends back a reply (i.e., the LB    *
 * doesn't wait for the vector PIO operation to finish first). Three    *
 * LB registers provide the contents for an outgoing vector PIO         *
 * request packet. Software should wait until the BUSY bit in           *
 * LB_VECTOR_PARMS is clear and then initialize all three of these      *
 * registers before initiating a vector PIO operation. The three        *
 * vector PIO registers are:                                            *
 * LB_VECTOR_ROUTE                                                      *
 * LB_VECTOR_DATA                                                       *
 * LB_VECTOR_PARMS (should be written last)                             *
 *                                                                      *
 ************************************************************************/

union lb_vector_parms_u {
	u64	lb_vector_parms_regval;
	struct	{
		u64	vp_busy			  :	 1;
		u64	vp_reserved		  :	12;
		u64	vp_pio_id		  :	11;
		u64	vp_write_id		  :	 8;
		u64	vp_reserved_1		  :	 8;
		u64	vp_address		  :	21;
		u64	vp_reserved_2		  :	 2;
		u64	vp_type			  :	 1;
	} lb_vector_parms_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register contains the vector PIO route. This is one of the 3   *
 * vector PIO control registers.                                        *
 *                                                                      *
 ************************************************************************/

union lb_vector_route_u {
	u64	lb_vector_route_regval;
	struct  {
		u64	vr_vector                 :	64;
	} lb_vector_route_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register contains the vector PIO write data. This is one of    *
 * the 3 vector PIO control registers. The contents of this register    *
 * also provide the data value to be sent in outgoing vector PIO read   *
 * requests and vector PIO write replies.                               *
 *                                                                      *
 ************************************************************************/

union lb_vector_data_u {
	u64	lb_vector_data_regval;
	struct  {
		u64	vd_write_data             :	64;
	} lb_vector_data_fld_s;
};

/************************************************************************
 *                                                                      *
 * Description:  This register contains the vector PIO return status.   *
 * Software should clear this register before launching a vector PIO    *
 * request from the LB. The LB will not modify this register's value    *
 * if an incoming reply packet encounters any kind of error. If an      *
 * incoming reply packet does not encounter an error but the            *
 * STATUS_VALID bit is already set, then the LB sets the OVERRUN bit    *
 * and leaves the other fields unchanged. The LB updates the values     *
 * of the SOURCE, PIO_ID, WRITE_ID, ADDRESS and TYPE fields only if     *
 * an incoming vector PIO reply packet does not encounter an error      *
 * and the STATUS_VALID bit is clear; at the same time, the LB sets     *
 * the STATUS_VALID bit and will also update the LB_VECTOR_RETURN and   *
 * LB_VECTOR_READ_DATA registers.                                       *
 *                                                                      *
 *                                                                      *
 ************************************************************************/

union lb_vector_status_u {
	u64	lb_vector_status_regval;
	struct	{
		u64	vs_status_valid		  :	 1;
		u64	vs_overrun		  :	 1;
		u64	vs_source		  :	11;
		u64	vs_pio_id		  :	11;
		u64	vs_write_id		  :	 8;
		u64	vs_reserved		  :	 8;
		u64	vs_address		  :	21;
		u64	vs_type			  :	 3;
	} lb_vector_status_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register contains the return vector PIO route. The LB will     *
 * not modify this register's value if an incoming reply packet         *
 * encounters any kind of error. The LB also will not modify this       *
 * register's value if the STATUS_VALID bit in the LB_VECTOR_STATUS     *
 * register is set when it receives an incoming vector PIO reply. The   *
 * LB stores an incoming vector PIO reply packet's vector route flit    *
 * in this register only if the packet does not encounter an error      *
 * and the STATUS_VALID bit is clear.                                   *
 *                                                                      *
 ************************************************************************/

union lb_vector_return_u {
	u64	lb_vector_return_regval;
	struct  {
		u64	vr_return_vector          :	64;
	} lb_vector_return_fld_s;
};

/************************************************************************
 *                                                                      *
 *  This register contains the vector PIO read data, if any. The LB     *
 * will not modify this register's value if an incoming reply packet    *
 * encounters any kind of error. The LB also will not modify this       *
 * register's value if the STATUS_VALID bit in the LB_VECTOR_STATUS     *
 * register is set when it receives an incoming vector PIO reply. The   *
 * LB stores an incoming vector PIO reply packet's data flit in this    *
 * register only if the packet does not encounter an error and the      *
 * STATUS_VALID bit is clear.                                           *
 *                                                                      *
 ************************************************************************/

union lb_vector_read_data_u {
	u64	lb_vector_read_data_regval;
	struct  {
		u64	vrd_read_data             :	64;
	} lb_vector_read_data_fld_s;
};

/************************************************************************
 *                                                                      *
 * Description:  This register contains the vector PIO return status.   *
 * Software should clear this register before launching a vector PIO    *
 * request from the LB. The LB will not modify this register's value    *
 * if an incoming reply packet encounters any kind of error. If an      *
 * incoming reply packet does not encounter an error but the            *
 * STATUS_VALID bit is already set, then the LB sets the OVERRUN bit    *
 * and leaves the other fields unchanged. The LB updates the values     *
 * of the SOURCE, PIO_ID, WRITE_ID, ADDRESS and TYPE fields only if     *
 * an incoming vector PIO reply packet does not encounter an error      *
 * and the STATUS_VALID bit is clear; at the same time, the LB sets     *
 * the STATUS_VALID bit and will also update the LB_VECTOR_RETURN and   *
 * LB_VECTOR_READ_DATA registers.                                       *
 *                                                                      *
 *                                                                      *
 ************************************************************************/

union lb_vector_status_clear_u {
	u64	lb_vector_status_clear_regval;
	struct	{
		u64	vsc_status_valid	  :	 1;
		u64	vsc_overrun		  :	 1;
		u64	vsc_source		  :	11;
		u64	vsc_pio_id		  :	11;
		u64	vsc_write_id		  :	 8;
		u64	vsc_reserved		  :	 8;
		u64	vsc_address		  :	21;
		u64	vsc_type		  :	 3;
	} lb_vector_status_clear_fld_s;
};

#endif /* __ASSEMBLY__ */

#endif /* _ASM_SN_SN1_HUBLB_H */
