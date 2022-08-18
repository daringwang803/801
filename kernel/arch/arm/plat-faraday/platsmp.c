/*
 *  linux/arch/arm/mach-realview/platsmp.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/io.h>

#include <asm/cacheflush.h>
#include <asm/mach-types.h>
#include <asm/mcpm.h>
#include <asm/smp_scu.h>
#include <asm/smp_plat.h>

#include <mach/hardware.h>
#include <plat/core.h>

extern void platform_secondary_startup(void);

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	sync_cache_w(&pen_release);
}

static void __iomem *scu_base_addr(void)
{
	return ioremap(PLAT_CPU_PERIPH_BASE, SZ_4K);
}

static DEFINE_SPINLOCK(boot_lock);

void platform_secondary_init(unsigned int cpu)
{
	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

int platform_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	/*
	 * set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 *
	 * Note that "pen_release" is the hardware CPU ID, whereas
	 * "cpu" is Linux's internal ID.
	 */
	write_pen_release(cpu_logical_map(cpu));

	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there.
	 */
	arch_send_wakeup_ipi_mask(cpumask_of(cpu));

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init platform_smp_init_cpus(void)
{
	unsigned int i, ncores;

#ifdef CONFIG_ARMGENERICTIMER
	/* Read current CP15 Cache Size ID Register */
	asm volatile ("mrc p15, 1, %0, c9, c0, 2" : "=r" (ncores));
	ncores = ((ncores >> 24) & 0x3) + 1;
	printk("smp_init_cpus Cores number = %d", ncores);	
#else
	ncores = scu_get_core_count((void __iomem *)PLAT_CPU_PERIPH_VA_BASE);
	printk("smp_init_cpus Cores number = %d \n", ncores);	
#endif

	/* sanity check */
	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %u cores greater than maximum (%u), clipping\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

#ifdef CONFIG_ACP
	/* Always enable SCU if we are using ACP */
	scu_enable(PLAT_CPU_PERIPH_VA_BASE);
#endif
}

void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
#ifndef CONFIG_ARMGENERICTIMER
	scu_enable(scu_base_addr());
#endif
	/*
	 * Write the address of secondary startup into the
	 * system-wide flags register. The BootMonitor waits
	 * until it receives a soft interrupt, and then the
	 * secondary CPU branches to this address.
	 */
#ifdef CONFIG_MACH_A380
	 __raw_writel(666, PLAT_SMP_VA_BASE);
	 __raw_writel(virt_to_phys(platform_secondary_startup), (void __iomem *)PLAT_SMP_VA_BASE + 0x4);
	 __raw_writel(0x11111111, (void __iomem *)PLAT_SMP_VA_BASE + 0x8);
#endif
#ifdef CONFIG_MACH_HGU10G
	 __raw_writel(virt_to_phys(platform_secondary_startup), (void __iomem *)PLAT_SMP_VA_BASE + 0x4);
	 __raw_writel(0x5A313047, (void __iomem *)PLAT_SMP_VA_BASE);
#endif
#ifdef CONFIG_MACH_WIDEBAND001
	 __raw_writel(virt_to_phys(platform_secondary_startup), (void __iomem *)PLAT_SMP_VA_BASE + 0x4);
	 __raw_writel(0x5A313047, (void __iomem *)PLAT_SMP_VA_BASE);
#endif
#ifdef CONFIG_MACH_LEO 
	 __raw_writel(virt_to_phys(platform_secondary_startup), (void __iomem *)PLAT_SCU_VA_BASE + 0x8218);
	 __raw_writel(0x07010203, (void __iomem *)PLAT_SCU_VA_BASE + 0x8214);
#endif
}

struct smp_operations faraday_smp_ops __initdata = {
	.smp_init_cpus		= platform_smp_init_cpus,
	.smp_prepare_cpus	= platform_smp_prepare_cpus,
	.smp_secondary_init	= platform_secondary_init,
	.smp_boot_secondary	= platform_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die			= platform_cpu_die,
#endif
};

bool __init platform_smp_init_ops(void)
{
#ifdef CONFIG_MCPM
	/*
	 * The best way to detect a multi-cluster configuration at the moment
	 * is to look for the presence of a CCI in the system.
	 * Override the default vexpress_smp_ops if so.
	 */
	mcpm_smp_set_ops();
	return true;

#endif
	return false;
}

