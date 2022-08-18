/*
 * ftgpio010.c -- access to GPIOs on Faraday FTGPIO010
 *
 * Copyright (C) 2011 Faraday-Tech, Inc.
 *
 *	Dante Su <dantesu@faraday-tech.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef FTGPIO010_H
#define FTGPIO010_H

struct ftgpio010_regs {
	uint32_t out;     /* 0x00: Data Output */
	uint32_t in;      /* 0x04: Data Input */
	uint32_t dir;     /* 0x08: Pin Direction */
	uint32_t bypass;  /* 0x0c: Bypass */
	uint32_t set;     /* 0x10: Data Set */
	uint32_t clr;     /* 0x14: Data Clear */
	uint32_t pull_up; /* 0x18: Pull-Up Enabled */
	uint32_t pull_st; /* 0x1c: Pull State (0=pull-down, 1=pull-up) */
};

#endif
