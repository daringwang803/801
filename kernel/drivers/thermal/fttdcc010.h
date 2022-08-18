/*
 * Driver for FTDCC010 Temperature-To-Digital Converter Controller
 *
 * Copyright (c) 2016 Faraday Technology Corporation
 *
 * B.C. Chen <bcchen@faraday-tech.com>
 *
 * Most of code borrowed from the Linux-3.7 EHCI driver
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __FTTDCC010_H
#define __FTTDCC010_H

struct fttdcc010_regs {
	/* DATA: offset 0x000~0x01C */
	u32		data[8];
	/* reserve */
	u32		reserve[24];
	/* THRHOLD: offset 0x080~0x09C */
	u32		thrhold[8];
#define HTHR_EN         (1<<31)
#define HTHR(x)         (((x)&0xFFF)<<16)
#define LTHR_EN         (1<<15)
#define LTHR(x)         (((x)&0xFFF)<<0)
	/* reserve */
	u32		reserve1[24];
	/* CTRL: offset 0x100 */
	u32		ctrl;
#define BUSY            (1<<8)
#define SCANMODE_SGL    (0<<1)
#define SCANMODE_CONT   (1<<1)
#define TDC_EN          (1<<0)
	/* TRIM: offset 0x104 */
	u32		trim;
	/* INTEN: offset 0x108 */
	u32		inten;
#define OVR_INTEN(x)    (1<<((x)+24))
#define UNDR_INTEN(x)   (1<<((x)+16))
#define CHDONE_INTEN(x) (1<<((x)+8))
#define TH_INTEN        (1<<2)
#define DONE_INTEN      (1<<1)
#define INTEN           (1<<0)
	/* INTST: offset 0x10C */
	u32		intst;
#define OVR_INTST(x)    (1<<((x)+24))
#define UNDR_INTST(x)   (1<<((x)+16))
#define CHDONE_INTST(x) (1<<((x)+8))
#define TH_INTST        (1<<2)
#define DONE_INTST      (1<<1)
#define INTST           (1<<0)
	/* TPARAM0: offset 0x110 */
	u32		tparam0;
	/* TPARAM1: offset 0x114 */
	u32		tparam1;
	/* reserve */
	u32		reserve2;
	/* PRESCAL: offset 0x11C */
	u32		prescal;
	/* reserve */
	u32		reserve3[7];
	/* REVISION: offset 0x13C */
	u32		revision;
};

#define CRITICAL_THERMAL                    140
#define MCELSIUS(temp)                      ((temp) * 4 + 374)
#define fttdcc010_thermal_priv_to_dev(priv) ((priv)->dev)

struct fttdcc010_thermal_priv {
	struct device *dev;
	struct fttdcc010_regs __iomem *regs;
	struct fttdcc010_thermal_zone *thermal;
	struct work_struct work;
	struct list_head head;
	unsigned long pending;
};

struct fttdcc010_thermal_zone {
	int id;
	struct thermal_zone_device *thermal;
	int ntrips;
	struct fttdcc010_thermal_trip *trips;
	struct completion done;
	struct list_head list;
};

struct fttdcc010_thermal_trip {
	unsigned long temperature;
	enum thermal_trip_type type;
};

#endif /* __FTTDCC010_H */
