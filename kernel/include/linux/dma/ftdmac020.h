/*
 *  arch/arm/mach-faraday/include/mach/ftdmac020.h
 *
 *  Faraday FTDMAC020 DMA controller
 *
 *  Copyright (C) 2010 Faraday Technology
 *  Copyright (C) 2010 Po-Yu Chuang <ratbert@faraday-tech.com>
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
#ifndef __MACH_FTDMAC020_H
#define __MACH_FTDMAC020_H

enum ftdmac020_channels {
	FTDMAC020_CHANNEL_0 = (1 << 0),
	FTDMAC020_CHANNEL_1 = (1 << 1),
	FTDMAC020_CHANNEL_2 = (1 << 2),
	FTDMAC020_CHANNEL_3 = (1 << 3),
	FTDMAC020_CHANNEL_4 = (1 << 4),
	FTDMAC020_CHANNEL_5 = (1 << 5),
	FTDMAC020_CHANNEL_6 = (1 << 6),
	FTDMAC020_CHANNEL_7 = (1 << 7),
	FTDMAC020_CHANNEL_ALL = 0xff,
};

/**
 * struct ftdmac020_dma_slave - DMA slave data
 * @common: physical address and register width...
 * @id: specify which ftdmac020 device to use, -1 for wildcard
 * @channels: bitmap of usable DMA channels
 * @handshake: hardware handshake number, -1 to disable handshake mode
 */
struct ftdmac020_dma_slave {
	struct dma_slave_config common;
	int id;
	enum ftdmac020_channels channels;
	int handshake;
};

/**
 * ftdmac020_chan_filter() - filter function for dma_request_channel().
 * @chan: DMA channel
 * @data: pointer to ftdmac020_dma_slave
 */
bool ftdmac020_chan_filter(struct dma_chan *chan, void *data);
#endif /* __MACH_FTDMAC020_H */
