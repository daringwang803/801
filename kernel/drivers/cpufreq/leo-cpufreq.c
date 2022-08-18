/*
 * Faraday LEO - CPU frequency scaling support
 *
 * (C) Copyright 2020 Faraday Technology
 * Bo-Cun Chen <bcchen@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>

#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#ifdef CONFIG_SMP
#include <asm/smp.h>
#endif

#include "faraday-cpufreq.h"

static DEFINE_SPINLOCK(wfi_lock);

enum cpufreq_level_index {
	L0, L1, L2, L3
};

static void set_wfi(void) 
{
	/*
	 * Stop the local timer for this CPU.
	 */
	/*
	 * Flush user cache and TLB mappings, and then remove this CPU
	 * from the vm mask set of all processes.
	*/
	if (!spin_trylock(&wfi_lock)) {
		printk("cpu_wfi lock error\n"); 
		return;
	}

	flush_cache_all();
	local_flush_tlb_all();

	asm("wfi"); 
	asm("nop"); 
	asm("nop"); 
	asm("nop"); 
	asm("nop"); 
	asm("nop"); 
	asm("nop"); 

	spin_unlock(&wfi_lock);
}

#define UART_LSR        0x14        /* In:  Line Status Register */ 
#define UART_LSR_TEMT   0x40        /* Transmitter empty */ 

static void leo_set_frequency(struct faraday_cpu_dvfs_info *info, unsigned int index)
{
	unsigned int val, timeout = 10000;

	/* wait uart tx over */
	while (timeout-- > 0) {
		if (readl(info->uart_base + UART_LSR) & UART_LSR_TEMT ) {
			break;
		}
		udelay(10);
	}

	udelay(100);

	// Check whether DDR is in self-refresh state
	// DDR can't be in self-refresh state before speed change
	val = readl(info->ddrc_base + 0x04);
	if (val & (0x1<<10)) {
		val = readl(info->ddrc_base + 0x04) | (0x1<<3);
		writel(val, info->ddrc_base + 0x04);
	}

	writel(0x00000000, info->sysc_base + 0xB4);

	val = readl(info->sysc_base + 0x28);
        printk("$$$$ inttrupt  val:%x \n",val);

	val = readl(info->sysc_base + 0x30) & ~(0x0F << 4);
	printk("$$$$ change before: val:%x,index: %d\n",val,index);

 	val = readl(info->sysc_base + 0x20); 
	printk("$$$$ fcs before: val:%x \n",val);
        val |= (0x01 << 6); 
        writel(val, info->sysc_base + 0x20);
	val = readl(info->sysc_base + 0x20);               
        printk("$$$$ fcs after: val:%x \n",val);

	switch (index) {
		case L0: //cpu 100000 KHz
			val = readl(info->sysc_base + 0x30) & ~(0x0F << 4);
			val |= (0x03 << 4);
			writel(val, info->sysc_base + 0x30);
			break;
		case L1: //cpu 200000 KHz
			val = readl(info->sysc_base + 0x30) & ~(0x0F << 4);
			val |= (0x02 << 4);
			writel(val, info->sysc_base + 0x30);
			break;
		case L2: //cpu 400000 KHz
			val = readl(info->sysc_base + 0x30) & ~(0x0F << 4);
			val |= (0x01 << 4);
			writel(val, info->sysc_base + 0x30);
			break;
		case L3: //cpu 800000 KHz
			val = readl(info->sysc_base + 0x30) & ~(0x0F << 4);
			val |= (0x00 << 4);
			writel(val, info->sysc_base + 0x30);
			break;
	}
	val = readl(info->sysc_base + 0x30) & ~(0x0F << 4);
	printk("$$$$change after: val:%x\n",val);	

	writel(0x60000040, info->sysc_base + 0x20);

	set_wfi();
}

int plat_cpufreq_init(struct faraday_cpu_dvfs_info *info)
{
	unsigned int status; 

	info->set_freq = leo_set_frequency;

	status = readl(info->sysc_base + 0x24);
	writel(status, info->sysc_base + 0x24);    /* clear interrupt */
	writel(0x0040, info->sysc_base + 0x28);    /* enable FCS interrupt */

	return 0;
}
EXPORT_SYMBOL(plat_cpufreq_init);
