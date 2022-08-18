/*
 *
 *  Copyright (C) 2013 Faraday Technology
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
#include <mach/serdes.h>
#include <linux/delay.h>
#include <asm/io.h>

void serdes_multi_mode_init(void __iomem *serdes_base, struct init_seq *seq , int len)
{
	int i;
	for(i= 0; i < len;i++){				
		writel(seq[i].value,serdes_base+(seq[i].addr)*4);
		udelay(10);
	}	
}
