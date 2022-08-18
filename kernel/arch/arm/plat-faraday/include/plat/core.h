/*
 *  linux/arch/arm/mach-realview/core.h
 *
 *  Copyright (C) 2004 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ASM_ARCH_REALVIEW_H
#define __ASM_ARCH_REALVIEW_H

#include <linux/io.h>

bool platform_smp_init_ops(void);

extern struct smp_operations faraday_smp_ops;
extern void platform_cpu_die(unsigned int cpu);
extern void ca9mp_speculative_linefills_init(void __iomem *base);

#endif
