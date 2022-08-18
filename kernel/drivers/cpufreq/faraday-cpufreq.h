/*
 * Faraday CPU CPUFreq Driver Framework
 *
 * (C) Copyright 2020 Faraday Technology
 * Kay Hsu <kayhsu@faraday-tech.com>
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

struct faraday_cpu_dvfs_info {
	void __iomem    *sysc_base;
	void __iomem    *ddrc_base;
	void __iomem    *uart_base;
	struct clk      *cpu_clk;
	struct device   *dev;
	struct cpufreq_frequency_table  *freq_table;
	struct list_head list_head;
	void (*set_freq)(struct faraday_cpu_dvfs_info *info, unsigned int);
	unsigned int    max_support_idx;
	unsigned int    min_support_idx;
};