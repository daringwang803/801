/*
 * Faraday FTLCDC210 DMA Functions
 *
 * (C) Copyright 2018 Faraday Technology
 * Jack Chain <jack_ch@faraday-tech.com>
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

#ifndef __FTLCDC210_DMA_H
#define __FTLCDC210_DMA_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

struct ftlcdc210_dma {
	struct dma_slave_config	   slave_conf;
	struct dma_chan	*chan;
	struct device     *dev;
	dma_addr_t		   src_dma_addr;
	dma_addr_t		   dst_dma_addr;
	dma_cookie_t		cookie;
	size_t			   trans_size;
	unsigned int      value;
	unsigned char		is_running;
};

int ftlcdc210_DMA_imgblt(struct device *dev,dma_addr_t src,dma_addr_t dst,size_t xcnt,
           size_t ycnt,size_t sstride,size_t dstride);
           
int ftlcdc210_DMA_fillrec(struct device *dev,dma_addr_t dst,size_t xcnt,
           size_t ycnt,size_t dstride,unsigned int color);
           
int ftlcdc210_DMA_copyarea(struct device *dev,dma_addr_t src,dma_addr_t dst,size_t xcnt,
           size_t ycnt,size_t sstride,size_t dstride);           

#endif   /* __FTLCDC210_DMA_H */
