/*
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2000 - 2001 by Kanoj Sarcar (kanoj@sgi.com)
 * Copyright (C) 2000 - 2001 by Silicon Graphics, Inc.
 */
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/bootinfo.h>
#include <asm/cacheflush.h>
#include <asm/sgialib.h>
#include <asm/sn/arch.h>
#include <asm/sn/agent.h>

#include "ip27-common.h"

static DECLARE_BITMAP(hub_init_mask, MAX_NUMNODES);
nasid_t master_nasid = INVALID_NASID;

struct cpuinfo_ip27 sn_cpu_info[NR_CPUS];
EXPORT_SYMBOL_GPL(sn_cpu_info);

static void per_hub_init(nasid_t nasid)
{
	struct hub_data *hub = hub_data(nasid);

	cpumask_set_cpu(smp_processor_id(), &hub->h_cpus);

	if (test_and_set_bit(nasid, hub_init_mask))
		return;
	/*
	 * Set CRB timeout at 5ms, (< PI timeout of 10ms)
	 */
	REMOTE_HUB_S(nasid, IIO_ICTP, 0x800);
	REMOTE_HUB_S(nasid, IIO_ICTO, 0xff);

	REMOTE_HUB_S(nasid, PI_RT_EN_A, 1);
	REMOTE_HUB_S(nasid, PI_RT_EN_B, 1);
	REMOTE_HUB_S(nasid, PI_PROF_EN_A, 0);
	REMOTE_HUB_S(nasid, PI_PROF_EN_B, 0);
	REMOTE_HUB_S(nasid, PI_RT_COUNT, 0);
	REMOTE_HUB_S(nasid, PI_RT_PEND_A, 0);
	REMOTE_HUB_S(nasid, PI_RT_PEND_B, 0);

#ifdef CONFIG_SGI_IP35
	REMOTE_HUB_S(nasid, PI_RT_EN_A, 1);
	REMOTE_HUB_S(nasid, PI_RT_EN_B, 1);
	REMOTE_HUB_S(nasid, PI_PROF_EN_A, 0);
	REMOTE_HUB_S(nasid, PI_PROF_EN_B, 0);
	REMOTE_HUB_S(nasid, PI_RT_COUNT, 0);
	REMOTE_HUB_S(nasid, PI_RT_PEND_A, 0);
	REMOTE_HUB_S(nasid, PI_RT_PEND_B, 0);
#endif

	if (nasid) {
		/* copy exception handlers from first node to current node */
		memcpy((void *)NODE_OFFSET_TO_K0(nasid, 0),
		       (void *)CKSEG0, 0x200);
		__flush_cache_all();
		/* switch to node local exception handlers */
		REMOTE_HUB_S(nasid, PI_CALIAS_SIZE, PI_CALIAS_SIZE_8K);
	}
}

void per_cpu_init(void)
{
	int cpu = smp_processor_id();
	nasid_t nasid = get_nasid();

	clear_c0_status(ST0_IM);

	per_hub_init(nasid);

	pr_info("CPU %d clock is %dMHz.\n", cpu, sn_cpu_info[cpu].p_speed);

	install_ipi();

	/* Install our NMI handler if symmon hasn't installed one. */
	install_cpu_nmi_handler(cputoslice(cpu));

	enable_percpu_irq(IP27_HUB_PEND0_IRQ, IRQ_TYPE_NONE);
	enable_percpu_irq(IP27_HUB_PEND1_IRQ, IRQ_TYPE_NONE);
}

#include <asm/sn/klconfig.h>

void __init plat_mem_setup(void)
{
	u64 p, e, n_mode;
	nasid_t nid;

	register_smp_ops(&ip27_smp_ops);

	ip27_reboot_setup();

	/*
	 * hub_rtc init and cpu clock intr enabled for later calibrate_delay.
	 */
	nid = get_nasid();
#ifdef CONFIG_SGI_IP35
	pr_info("IP35: Running on node %d.\n", nid);
	n_mode = 0; /* no N mode for IP35 */
#else
	pr_info("IP27: Running on node %d.\n", nid);
	n_mode = LOCAL_HUB_L(NI_STATUS_REV_ID) & NSRI_MORENODES_MASK;
#endif

	p = LOCAL_HUB_L(PI_CPU_PRESENT_A) & 1;
	e = LOCAL_HUB_L(PI_CPU_ENABLE_A) & 1;
	pr_info("Node %d has %s primary CPU%s.\n", nid,
		p ? "a" : "no",
		e ? ", CPU is running" : "");

	p = LOCAL_HUB_L(PI_CPU_PRESENT_B) & 1;
	e = LOCAL_HUB_L(PI_CPU_ENABLE_B) & 1;
	pr_info("Node %d has %s secondary CPU%s.\n", nid,
		p ? "a" : "no",
		e ? ", CPU is running" : "");

	/*
	 * Try to catch kernel missconfigurations and give user an
	 * indication what option to select.
	 */
	pr_info("Machine is in %c mode.\n", n_mode ? 'N' : 'M');
#ifdef CONFIG_SGI_SN_N_MODE
	if (!n_mode)
		panic("Kernel compiled for N mode.");
#else
	if (n_mode)
		panic("Kernel compiled for M mode.");
#endif

	ioport_resource.start = 0;
	ioport_resource.end = ~0UL;
	set_io_port_base(IO_BASE);
}

const char *ip27_system_type = "SGI Origin";

const char *get_system_type(void)
{
	return ip27_system_type;
}

void __init prom_init(void)
{
	prom_init_cmdline(fw_arg0, (LONG *)fw_arg1);
	prom_meminit();
}

