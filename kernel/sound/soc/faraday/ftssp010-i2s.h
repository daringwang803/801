/*
 * Faraday FTSSP010 I2S ALSA SoC Digital Audio Interface
 *
 * (C) Copyright 2010 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
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

#ifndef __FTSSP010_I2S_H
#define __FTSSP010_I2S_H

#include <linux/dmaengine.h>
#include <linux/dma/ftdmac030.h>

struct ftssp010_i2s {
	void __iomem *base;
	struct device *dev;
	unsigned int irq;
	unsigned int sysclk;
	struct snd_dmaengine_dai_dma_data capture_dma_data;
	struct snd_dmaengine_dai_dma_data playback_dma_data;
	unsigned int cr0;
	unsigned int cr1;
	unsigned int icr;
	struct ftdmac030_dma_slave playback_dma_slave;
	struct ftdmac030_dma_slave capture_dma_slave;
};

int ftssp010_asoc_dma_platform_register(struct device *dev);

#endif	/* __FTSSP010_I2S_H */
