/*
 * Provide constants for A380 pinctrl bindings
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
#ifndef __DT_BINDINGS_PINCTRL_A380_H__
#define __DT_BINDINGS_PINCTRL_A380_H__

#define A380_MUX_FTAHBB020      0
#define A380_MUX_FTAHBWRAP      1
#define A380_MUX_FTDMAC020      2
#define A380_MUX_FTGPIO010      3
#define A380_MUX_FTI2C010_0     4
#define A380_MUX_FTI2C010_1     5
#define A380_MUX_FTINTC030      6
#define A380_MUX_FTKBC010       7
#define A380_MUX_FTLCD210       8
#define A380_MUX_FTLCD210TV     9
#define A380_MUX_FTNAND024      10
#define A380_MUX_FTSSP010_0     11
#define A380_MUX_FTSSP010_1     12
#define A380_MUX_FTSSP010_2     13
#define A380_MUX_FTUART010_0    14
#define A380_MUX_FTUART010_1    15
#define A380_MUX_FTUART010_2    16
#define A380_MUX_GBE            17
#define A380_MUX_NA             18  /* N/A */
#define A380_MUX_VCAP300        19

#endif	/* __DT_BINDINGS_PINCTRL_A380_H__ */
