/**
 *  include/linux/platform_data/clk-faraday.h
 *
 *  Faraday Evaluation Boards Clock Tree Initialization
 *
 *  Copyright (C) 2015 Faraday Technology
 *  Copyright (C) 2015 Bing-Yao Luo <bjluo@faraday-tech.com>
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
int __init a380_clocks_init(void);
int __init tc12ngrc_clocks_init(void);
int __init leo_clocks_init(void);

#ifdef CONFIG_OF
void __init of_a380_clocks_init(struct device_node *np);
void __init of_leo_clocks_init(struct device_node *np);
#endif
