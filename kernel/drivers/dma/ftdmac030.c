/*
 * Faraday FTDMAC030 DMA engine driver
 *
 * (C) Copyright 2011 Faraday Technology
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
/******************************************************************************
 * Include files
 *****************************************************************************/
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_dma.h>

#include <asm/io.h>

#include "dmaengine.h"
#include "ftdmac030.h"
#include <plat/ftdmac030.h>

/******************************************************************************
 * Define Constants
 *****************************************************************************/
#define DRV_NAME	"ftdmac030"
#define CHANNEL_NR	8

/*
 * This value must be multiple of 32-bits to prevent from changing the
 * alignment of child descriptors.
 */
#define MAX_CYCLE_PER_BLOCK	0x200000
#if (MAX_CYCLE_PER_BLOCK > FTDMAC030_CYC_MASK) || \
	(MAX_CYCLE_PER_BLOCK & 0x3)
#error invalid MAX_CYCLE_PER_BLOCK
#endif

#define GRANT_WINDOW	64


static struct of_dma_filter_info ftdmac030_dma_info = {
	.filter_fn = ftdmac030_chan_filter,
};

/*
 * Initial number of descriptors to allocate for each channel. This could
 * be increased during dma usage.
 */
static unsigned int init_nr_desc_per_channel = 64;
module_param(init_nr_desc_per_channel, uint, 0644);
MODULE_PARM_DESC(init_nr_desc_per_channel,
		 "initial descriptors per channel (default: 64)");

/**
 * struct ftdmac030_desc - async transaction descriptor.
 * @lld: hardware descriptor, MUST be the first member
 * @ctrl: value for channel control register
 * @cfg: value for channel configuration register
 * @src: source physical address
 * @dst: destination physical addr
 * @next: phsical address to the first link list descriptor (2nd block)
 * @cycle: transfer size
 * @txd: support for the async_tx api
 * @child_list: list for transfer chain
 * @ftchan: the channel which this descriptor belongs to
 * @node: node on the descriptors list
 * @len: length in bytes
 */
struct ftdmac030_desc {
	struct ftdmac030_lld lld;

	/* used only by the first block */
	unsigned int cfg;
	unsigned int ccr;
	dma_addr_t src;
	dma_addr_t dst;
	dma_addr_t next;
	unsigned int cycle;

	struct dma_async_tx_descriptor txd;
	struct list_head child_list;

	/* used by all blocks */
	struct ftdmac030_chan *ftchan;
	struct list_head node;
	size_t len;
};

/**
 * struct ftdmac030_chan - internal representation of an ftdmac030 channel.
 * @common: common dmaengine channel object
 * @active_list: list of descriptors dmaengine is being running on
 * @free_list: list of descriptors usable by the channel
 * @tasklet: bottom half to finish transaction work
 * @lock: serializes enqueue/dequeue operations to descriptors lists
 * @ftdmac030: parent device
 * @completed_cookie: identifier for the most recently completed operation
 * @descs_allocated: number of allocated descriptors
 * @cyclic: cyclic transfer
 * @cyclic_happened: count for the number of completed cyclic transfers
 * @cyclic_callback: cyclic callback function (only used for cyclic transfer)
 * @cyclic_callback_param: parameters for cyclic callback function
 */
struct ftdmac030_chan {
	struct dma_chan common;
	struct list_head active_list;
	struct list_head free_list;
	struct tasklet_struct tasklet;
	spinlock_t lock;
	struct ftdmac030 *ftdmac030;
	int descs_allocated;
	
	bool cyclic;
	dma_async_tx_callback cyclic_callback;
	void *cyclic_callback_param;
	dma_cookie_t completed_cookie;
	int residue;
	int period_len;
	int buffer_len;
};

/**
 * struct ftdmac030 - internal representation of an ftdmac030 device
 * @dma: common dmaengine dma_device object
 * @res: io memory resource of hardware registers
 * @base: virtual address of hardware register base
 * @id: id for each device
 * @irq: irq number
 * @channel: channel table
 */
struct ftdmac030 {
	struct dma_device dma;
	struct resource *res;
	void __iomem *base;
	int id;
	unsigned int irq;
	
	int max_cycle_bits;
	unsigned int max_cycle_per_block;
	unsigned int cycle_mask;
	
	struct dma_pool *dma_desc_pool;
	struct ftdmac030_chan channel[CHANNEL_NR];
};

#define DMAC030_BUSWIDTHS	(BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) | \
				 BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) | \
				 BIT(DMA_SLAVE_BUSWIDTH_4_BYTES))

static dma_cookie_t ftdmac030_tx_submit(struct dma_async_tx_descriptor *);

/******************************************************************************
 * internal functions
 *****************************************************************************/

static inline struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}

static void ftdmac030_enable_handshake_sync(struct ftdmac030_chan *ftchan,int handshake_number)
{
	unsigned int handshake_id = (0x1 << handshake_number);
	void __iomem *base = ftchan->ftdmac030->base;
	unsigned int temp_reg;

	temp_reg = ioread32(base + FTDMAC030_OFFSET_SYNC);	
	temp_reg |= handshake_id; 
	iowrite32(temp_reg, base + FTDMAC030_OFFSET_SYNC);
}

static void ftdmac030_write_only_mode_constant_value(struct ftdmac030_chan *ftchan,
		unsigned int value)
{
	void __iomem *base = ftchan->ftdmac030->base;

	iowrite32(value, base + FTDMAC030_WRITE_ONLY_MODE_CONSTANT_VALUE);
}

/******************************************************************************
 * Descriptor classification functions
 *****************************************************************************/
/**
 * ftdmac030_alloc_desc - allocate and initialize descriptor
 * @ftchan: the channel to allocate descriptor for
 * @gfp_flags: GFP allocation flags
 */
static struct ftdmac030_desc *ftdmac030_alloc_desc(
		struct ftdmac030_chan *ftchan, gfp_t gfp_flags)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030 *ftdmac030 = ftchan->ftdmac030;
	struct ftdmac030_desc *desc;
	dma_addr_t phys;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	
	desc = dma_pool_alloc(ftdmac030->dma_desc_pool, gfp_flags, &phys);
	if (desc) {
		memset(desc, 0, sizeof(*desc));

		/* initialize dma_async_tx_descriptor fields */
		dma_async_tx_descriptor_init(&desc->txd, &ftchan->common);
		desc->txd.tx_submit = ftdmac030_tx_submit;
		desc->txd.phys = phys;
		INIT_LIST_HEAD(&desc->child_list);
		desc->ftchan = ftchan;
	}
	dev_dbg(chan2dev(chan), "%s desc = %p\n", __func__,desc);
	return desc;
}

/**
 * ftdmac030_free_desc - free descriptor
 * @ftchan: the channel to free descriptor for
 */
static void ftdmac030_free_desc(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &desc->ftchan->common;
	struct ftdmac030 *ftdmac030 = ftchan->ftdmac030;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	
	dma_pool_free(ftdmac030->dma_desc_pool, desc, desc->txd.phys);
}

/**
 * ftdmac030_desc_get - get an unused descriptor from free list
 * @ftchan: channel we want a new descriptor for
 */
static struct ftdmac030_desc *ftdmac030_desc_get(struct ftdmac030_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *desc = NULL;

	dev_dbg(chan2dev(chan), "%s\n", __func__);

	if (!list_empty(&ftchan->free_list)) {
		desc = list_first_entry(&ftchan->free_list,
			struct ftdmac030_desc, node);
		list_del(&desc->node);
	}

	/* no more descriptor available in initial pool: create one more */
	if (!desc) {
		desc = ftdmac030_alloc_desc(ftchan, GFP_ATOMIC);
		if (desc) {
			ftchan->descs_allocated++;
		} else {
			dev_err(chan2dev(chan),
				"not enough descriptors available\n");
		}
	} else {
		dev_dbg(chan2dev(chan), "%s got desc %p, ftchan->descs_allocated = %d\n", __func__, desc,ftchan->descs_allocated);
	}

	return desc;
}

/**
 * ftdmac030_desc_put - move a descriptor, including any children, to the free list
 * @ftchan: channel we work on
 * @desc: descriptor, at the head of a chain, to move to free list
 */
static void ftdmac030_desc_put(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *child;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);

	if (!list_empty(&desc->child_list)) {
		list_for_each_entry(child, &desc->child_list, node) {
			dev_dbg(chan2dev(chan),
				"moving child desc %p to freelist\n", child);
		}
		list_splice_init(&desc->child_list, &ftchan->free_list);
	}

	list_add(&desc->node, &ftchan->free_list);
}

/**
 * ftdmac030_unmap_desc - unmap a descriptor
 * @ftchan: channel we work on
 * @desc: descriptor, at the head of a chain, to move to free list
 */
static void ftdmac030_unmap_desc(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);

	if (ftchan->common.private)
		return;

	dma_descriptor_unmap(&desc->txd);
}

/******************************************************************************
 * Channel classification functions
 *****************************************************************************/
/**
 * ftdmac030_remove_chain - remove a descriptor
 * @desc: the descriptor to finish for
 */
void ftdmac030_remove_chain(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *child;
	struct ftdmac030_desc *tmp;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	
	//spin_lock_bh(&ftchan->lock);
	
	list_for_each_entry_safe(child, tmp, &desc->child_list, node) {
		dev_dbg(chan2dev(chan), "removing child desc %p\n", child);
		list_del(&child->node);
		ftdmac030_unmap_desc(child);
		ftdmac030_desc_put(child);
	}

	ftdmac030_unmap_desc(desc);
	ftdmac030_desc_put(desc);
	
	//spin_unlock_bh(&ftchan->lock);
}

/**
 * ftdmac030_start_chain - start channel
 * @ftchan: the channel to start for
 */
static void ftdmac030_start_chain(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	void __iomem *base = ftchan->ftdmac030->base;
	int chan_id = ftchan->common.chan_id;
	unsigned int tmp;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);

	//Special for 2D DMA, need to enable 2D DMA before fill size
	if (desc->lld.ctrl & FTDMAC030_CTRL_2D)
	{
		//unmake enable channel
		tmp = desc->lld.ctrl & ~FTDMAC030_CTRL_ENABLE;
		iowrite32(tmp, base + FTDMAC030_OFFSET_CTRL_CH(chan_id));
	}

	dev_dbg(chan2dev(chan), "\t[CFG %d] = %x\n", chan_id, desc->cfg);
	iowrite32(desc->cfg, base + FTDMAC030_OFFSET_CFG_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[SRC %d] = %x\n", chan_id, desc->lld.src);
	iowrite32(desc->lld.src, base + FTDMAC030_OFFSET_SRC_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[DST %d] = %x\n", chan_id, desc->lld.dst);
	iowrite32(desc->lld.dst, base + FTDMAC030_OFFSET_DST_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[LLP %d] = %x\n", chan_id, desc->lld.next);
	iowrite32(desc->lld.next, base + FTDMAC030_OFFSET_LLP_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[LEN %d] = %x\n", chan_id, desc->lld.cycle);
	iowrite32(desc->lld.cycle, base + FTDMAC030_OFFSET_CYC_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[STRIDE %d] = %x\n", chan_id, desc->lld.stride);
	iowrite32(desc->lld.stride, base + FTDMAC030_OFFSET_STRIDE_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[CTRL %d] = %x\n", chan_id, desc->lld.ctrl);
	iowrite32(desc->lld.ctrl, base + FTDMAC030_OFFSET_CTRL_CH(chan_id));
}

/**
 * ftdmac030_start_chain - start a new channel
 * @ftchan: the channel to start for
 */
static void ftdmac030_start_new_chain(struct ftdmac030_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *desc;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	
	if (list_empty(&ftchan->active_list))
		return;

	desc = list_first_entry(&ftchan->active_list, struct ftdmac030_desc, node);
	ftdmac030_start_chain(desc);
}

/**
 * ftdmac030_chan_is_enabled - get channel is enable status
 * @ftchan: the channel get enable status for
 */
static int ftdmac030_chan_is_enabled(struct ftdmac030_chan *ftchan)
{
	int chan_id = ftchan->common.chan_id;
	struct dma_chan *chan = &ftchan->common;
	void __iomem *base = ftchan->ftdmac030->base;
	unsigned int enabled;

	dev_dbg(chan2dev(chan), "%s: \n", __func__);

	enabled = ioread32(base + FTDMAC030_OFFSET_CH_ENABLED);
	return enabled & (1 << chan_id);
}

/**
 * ftdmac030_stop_channel - stop channel
 * @ftchan: the channel to abort
 */
static void ftdmac030_stop_channel(struct ftdmac030_chan *ftchan)
{
	int chan_id = ftchan->common.chan_id;
	struct dma_chan *chan = &ftchan->common;
	void __iomem *base = ftchan->ftdmac030->base;
	unsigned int reg;
	int chan_en;
	unsigned int timeout = 0xFFFFFFF;
	int i = 0;

	dev_dbg(chan2dev(chan), "%s: \n", __func__);

	spin_lock(&ftchan->lock);

	chan_en = ftdmac030_chan_is_enabled(ftchan);
	dev_dbg(chan2dev(chan), "\t[channel enable] = %x\n", chan_en);
	
	i=0;
	if (chan_en)
	{
		reg = 0xFFFFFFFF;
		reg &= ~(0x1 << chan_id);
		dev_dbg(chan2dev(chan), "\t[Write channel enable 0x1c] = %x\n", reg);
		
		iowrite32(reg, base + FTDMAC030_OFFSET_CH_ENABLED);
		
		//check DMA is really stop
		do
		{
			chan_en = ftdmac030_chan_is_enabled(ftchan);
			i++;
			if(i > timeout)
			{
				dev_err(chan2dev(chan), "%s: Stop Channel %d TIMEOUT!!\n",__func__,chan_id); 
				break; 
			}
		} while (chan_en);
	}
	
	spin_unlock(&ftchan->lock);
}

/**
 * ftdmac030_create_chain - create a DMA transfer chain
 * @ftchan: the channel to allocate descriptor for
 * @first: first descriptor of a transfer chain if any
 * @src: physical source address
 * @dest: phyical destination address
 * @len: length in bytes
 * @shift: shift value for width (0: byte, 1: halfword, 2: word)
 * @fixed_src: source address is fixed (device register)
 * @fixed_dest: destination address is fixed (device register)
 */
static struct ftdmac030_desc *ftdmac030_create_chain(
		struct ftdmac030_chan *ftchan,
		struct ftdmac030_desc *first,
		dma_addr_t src, dma_addr_t dest, size_t len,
		unsigned int shift, int maxburst, int fixed_src, int fixed_dest)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *prev = NULL;
	unsigned int ctrl;
	size_t offset;
	unsigned int cycle;

	dev_dbg(chan2dev(chan), "%s(src %x, dest %x, len %x, shift %d)\n",
		__func__, src, dest, len, shift);
	if ((shift == 2 && ((src | dest | len) & 3)) ||
	    (shift == 1 && ((src | dest | len) & 1))) {
		dev_err(chan2dev(chan), "%s: register or data misaligned\n",
			__func__);
		return NULL;
	}

	if (first) {
		if (list_empty(&first->child_list))
			prev = first;
		else
			prev = list_entry(first->child_list.prev,
				struct ftdmac030_desc, node);
	}

	ctrl = FTDMAC030_CTRL_ENABLE ;
	
	switch (maxburst) {
	case 1:
		ctrl |= FTDMAC030_CTRL_1BEAT;
		break;
	case 2:
		ctrl |= FTDMAC030_CTRL_2BEATS;
		break;
	case 4:
		ctrl |= FTDMAC030_CTRL_4BEATS;
		break;
	case 8:
		ctrl |= FTDMAC030_CTRL_8BEATS;
		break;
	case 16:
		ctrl |= FTDMAC030_CTRL_16BEATS;
		break;
	case 32:
		ctrl |= FTDMAC030_CTRL_32BEATS;
		break;
	case 64:
		ctrl |= FTDMAC030_CTRL_64BEATS;
		break;
	case 128:
		ctrl |= FTDMAC030_CTRL_128BEATS;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect burst size\n",
			__func__);
		BUG();
		break;
	}
	
	switch (shift) {
	case 2:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_32
		     |  FTDMAC030_CTRL_SRC_WIDTH_32;
		break;
	case 1:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_16
		     |  FTDMAC030_CTRL_SRC_WIDTH_16;
		break;
	case 0:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_8
		     |  FTDMAC030_CTRL_SRC_WIDTH_8;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
		break;
	}

	if (fixed_src)
		ctrl |= FTDMAC030_CTRL_SRC_FIXED;
	else
		ctrl |= FTDMAC030_CTRL_SRC_INC;

	if (fixed_dest)
		ctrl |= FTDMAC030_CTRL_DST_FIXED;
	else
		ctrl |= FTDMAC030_CTRL_DST_INC;

	for (offset = 0; offset < len; offset += cycle << shift) {
		struct ftdmac030_desc *desc;

		cycle = min_t(size_t, (len - offset) >> shift,
				MAX_CYCLE_PER_BLOCK);

		desc = ftdmac030_desc_get(ftchan);
		if (!desc){
			goto err;
		}

		if (fixed_src)
			desc->lld.src = src;
		else
			desc->lld.src = src + offset;

		if (fixed_dest)
			desc->lld.dst = dest;
		else
			desc->lld.dst = dest + offset;

		desc->cfg = FTDMAC030_CFG_GW(GRANT_WINDOW)
			  | FTDMAC030_CFG_HIGH_PRIO;
		desc->len = cycle << shift;

		desc->lld.next	= 0;
		desc->lld.cycle	= FTDMAC030_CYC_TOTAL(cycle);
		desc->lld.ctrl	= ctrl;

		if (!first) {
			first = desc;
		} else {
			/*
			 * Mask terminal count interrupt for this descriptor.
			 * What an inconvenient stupid design.
			 */
			prev->lld.ctrl |= FTDMAC030_CTRL_MASK_TC;

			/* hardware link list pointer */
			prev->lld.next = desc->txd.phys;

			/* insert the link descriptor to the transfer chain */
			list_add_tail(&desc->node, &first->child_list);
		}

		prev = desc;
	}

	return first;
err:

	if (first)
		ftdmac030_remove_chain(first);
	return NULL;
}

/**
 * ftdmac030_create_chain_cyclic - create a DMA transfer chain for cycylic mode
 * @ftchan: the channel to allocate descriptor for
 * @first: first descriptor of a transfer chain if any
 * @src: physical source address
 * @dest: phyical destination address
 * @len: length in bytes
 * @shift: shift value for width (0: byte, 1: halfword, 2: word)
 * @fixed_src: source address is fixed (device register)
 * @fixed_dest: destination address is fixed (device register)
 */
static struct ftdmac030_desc *ftdmac030_create_chain_cyclic(
		struct ftdmac030_chan *ftchan,
		struct ftdmac030_desc *first,
		dma_addr_t src, dma_addr_t dest, size_t len,
		unsigned int shift, int maxburst, int fixed_src, int fixed_dest)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *prev = NULL;
	unsigned int ctrl;
	size_t offset;
	unsigned int cycle;

	dev_dbg(chan2dev(chan), "%s: src = 0x%x, dest = 0x%x, len = 0x%x, shift = %d\n",
		__func__, src, dest, len, shift);

	if ((shift == 2 && ((src | dest | len) & 3)) ||
	    (shift == 1 && ((src | dest | len) & 1))) {
		dev_err(chan2dev(chan), "%s: register or data misaligned\n",
			__func__);
		return NULL;
	}

	if (first) {
		if (list_empty(&first->child_list))
			prev = first;
		else
			prev = list_entry(first->child_list.prev,
				struct ftdmac030_desc, node);
	}

	ctrl = FTDMAC030_CTRL_ENABLE ;

	switch (maxburst) {
	case 1:
		ctrl |= FTDMAC030_CTRL_1BEAT;
		break;
	case 2:
		ctrl |= FTDMAC030_CTRL_2BEATS;
		break;
	case 4:
		ctrl |= FTDMAC030_CTRL_4BEATS;
		break;
	case 8:
		ctrl |= FTDMAC030_CTRL_8BEATS;
		break;
	case 16:
		ctrl |= FTDMAC030_CTRL_16BEATS;
		break;
	case 32:
		ctrl |= FTDMAC030_CTRL_32BEATS;
		break;
	case 64:
		ctrl |= FTDMAC030_CTRL_64BEATS;
		break;
	case 128:
		ctrl |= FTDMAC030_CTRL_128BEATS;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect burst size\n",
			__func__);
		BUG();
		break;
	}

	switch (shift) {
	case 2:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_32
		     |  FTDMAC030_CTRL_SRC_WIDTH_32;
		break;
	case 1:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_16
		     |  FTDMAC030_CTRL_SRC_WIDTH_16;
		break;
	case 0:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_8
		     |  FTDMAC030_CTRL_SRC_WIDTH_8;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
		break;
	}

	if (fixed_src)
		ctrl |= FTDMAC030_CTRL_SRC_FIXED;
	else
		ctrl |= FTDMAC030_CTRL_SRC_INC;

	if (fixed_dest)
		ctrl |= FTDMAC030_CTRL_DST_FIXED;
	else
		ctrl |= FTDMAC030_CTRL_DST_INC;

	for (offset = 0; offset < len; offset += cycle << shift) {
		struct ftdmac030_desc *desc;

		cycle = min_t(size_t, (len - offset) >> shift,
				MAX_CYCLE_PER_BLOCK);

		desc = ftdmac030_desc_get(ftchan);
		if (!desc){
			goto err;
		}

		if (fixed_src)
			desc->lld.src = src;
		else
			desc->lld.src = src + offset;

		if (fixed_dest)
			desc->lld.dst = dest;
		else
			desc->lld.dst = dest + offset;

		desc->cfg = FTDMAC030_CFG_GW(GRANT_WINDOW)
			  | FTDMAC030_CFG_HIGH_PRIO;
		desc->len = cycle << shift;

		desc->lld.next	= 0;
		desc->lld.cycle	= FTDMAC030_CYC_TOTAL(cycle);
		desc->lld.ctrl	= ctrl;

		if (!first) {
			first = desc;
		} else {
			// hardware link list pointer 
			prev->lld.next = desc->txd.phys;

			// insert the link descriptor to the transfer chain 
			list_add_tail(&desc->node, &first->child_list);
		}

		desc->lld.next = first->txd.phys;
		prev = desc;
	}
	
	return first;
err:

	if (first)
		ftdmac030_remove_chain(first);
	return NULL;
}

/**
 * ftdmac030_create_chain_constant_value - create a DMA transfer chain with 
 *                                            constant value
 * @ftchan: the channel to allocate descriptor for
 * @first: first descriptor of a transfer chain if any
 * @dest: phyical destination address
 * @len: length in bytes
 * @vlaue: wirte constant value
 * @shift: shift value for width (2: word)
 */
static struct ftdmac030_desc *ftdmac030_create_chain_constant_value(
		struct ftdmac030_chan *ftchan,
		struct ftdmac030_desc *first,
		dma_addr_t dest, size_t len, unsigned int value,
		unsigned int shift, int maxburst)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *prev = NULL;
	unsigned int ctrl;
	size_t offset;
	unsigned int cycle;

	dev_dbg(chan2dev(chan), "%s(dest %x, len %x, value %x, shift %d)\n",
		__func__, dest, len, value, shift);

	if ((shift == 2 && ((dest | len) & 3)) ||
	    (shift == 1 && ((dest | len) & 1))) {
		dev_err(chan2dev(chan), "%s: register or data misaligned\n",
			__func__);
		return NULL;
	}

	if (first) {
		if (list_empty(&first->child_list))
			prev = first;
		else
			prev = list_entry(first->child_list.prev,
				struct ftdmac030_desc, node);
	}

	ctrl = FTDMAC030_CTRL_ENABLE ;

	switch (maxburst) {
	case 1:
		ctrl |= FTDMAC030_CTRL_1BEAT;
		break;
	case 2:
		ctrl |= FTDMAC030_CTRL_2BEATS;
		break;
	case 4:
		ctrl |= FTDMAC030_CTRL_4BEATS;
		break;
	case 8:
		ctrl |= FTDMAC030_CTRL_8BEATS;
		break;
	case 16:
		ctrl |= FTDMAC030_CTRL_16BEATS;
		break;
	case 32:
		ctrl |= FTDMAC030_CTRL_32BEATS;
		break;
	case 64:
		ctrl |= FTDMAC030_CTRL_64BEATS;
		break;
	case 128:
		ctrl |= FTDMAC030_CTRL_128BEATS;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect burst size\n",
			__func__);
		BUG();
		break;
	}  

	switch (shift) {
	case 2:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_32
		     |  FTDMAC030_CTRL_SRC_WIDTH_32;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
		break;
	}

	ctrl |= FTDMAC030_CTRL_SRC_FIXED;
	ctrl |= FTDMAC030_CTRL_DST_INC;

	for (offset = 0; offset < len; offset += cycle << shift) {
		struct ftdmac030_desc *desc;

		cycle = min_t(size_t, (len - offset) >> shift,
			MAX_CYCLE_PER_BLOCK);

		desc = ftdmac030_desc_get(ftchan);
		if (!desc) {
			goto err;
		}

		desc->lld.src = dest;
		desc->lld.dst = dest + offset;

		desc->cfg = FTDMAC030_CFG_GW(GRANT_WINDOW)
			  | FTDMAC030_CFG_HIGH_PRIO | FTDMAC030_CFG_WRITE_ONLY_MODE;
		desc->len = cycle << shift;

		desc->lld.next = 0;
		desc->lld.cycle = FTDMAC030_CYC_TOTAL(cycle);
		desc->lld.ctrl = ctrl;

		//Write value to Write-only Mode Constant Value Register
		ftdmac030_write_only_mode_constant_value(ftchan,value);

		if (!first) {
			first = desc;
		} else {
			// Mask terminal count interrupt for this descriptor.
			// What an inconvenient stupid design.
			prev->lld.ctrl |= FTDMAC030_CTRL_MASK_TC;

			// hardware link list pointer 
			prev->lld.next = desc->txd.phys;

			// insert the link descriptor to the transfer chain 
			list_add_tail(&desc->node, &first->child_list);
		}

		prev = desc;
	}

	return first;

err:
	if (first)
		ftdmac030_remove_chain(first);

	return NULL;
}

/**
 * ftdmac030_create_chain_unalign_mode - create a DMA transfer chain
 * @ftchan: the channel to allocate descriptor for
 * @first: first descriptor of a transfer chain if any
 * @src: physical source address
 * @dest: phyical destination address
 * @len: length in bytes
 * @shift: shift value for width (0: byte, 1: halfword, 2: word)
 * @fixed_src: source address is fixed (device register)
 * @fixed_dest: destination address is fixed (device register)
 */
static struct ftdmac030_desc *ftdmac030_create_chain_unalign_mode(
		struct ftdmac030_chan *ftchan,
		struct ftdmac030_desc *first,
		dma_addr_t src, dma_addr_t dest, size_t len,
		unsigned int shift, int maxburst, int fixed_src, int fixed_dest)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *prev = NULL;
	unsigned int ctrl;
	size_t offset;
	unsigned int cycle;

	dev_dbg(chan2dev(chan), "%s(src %x, dest %x, len %x, shift %d)\n",
		__func__, src, dest, len, shift);

	if (first) 
	{
		if (list_empty(&first->child_list))
			prev = first;
		else
			prev = list_entry(first->child_list.prev,
				struct ftdmac030_desc, node);
	}

	ctrl = FTDMAC030_CTRL_ENABLE ;

	switch (maxburst) 
	{
	case 1:
		ctrl |= FTDMAC030_CTRL_1BEAT;
		break;
	case 2:
		ctrl |= FTDMAC030_CTRL_2BEATS;
		break;
	case 4:
		ctrl |= FTDMAC030_CTRL_4BEATS;
		break;
	case 8:
		ctrl |= FTDMAC030_CTRL_8BEATS;
		break;
	case 16:
		ctrl |= FTDMAC030_CTRL_16BEATS;
		break;
	case 32:
		ctrl |= FTDMAC030_CTRL_32BEATS;
		break;
	case 64:
		ctrl |= FTDMAC030_CTRL_64BEATS;
		break;
	case 128:
		ctrl |= FTDMAC030_CTRL_128BEATS;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect burst size\n",
			__func__);
		BUG();
		break;
	}  

	ctrl |= FTDMAC030_CTRL_DST_WIDTH_8
	     |  FTDMAC030_CTRL_SRC_WIDTH_8;

	if (fixed_src)
		ctrl |= FTDMAC030_CTRL_SRC_FIXED;
	else
		ctrl |= FTDMAC030_CTRL_SRC_INC;

	if (fixed_dest)
		ctrl |= FTDMAC030_CTRL_DST_FIXED;
	else
		ctrl |= FTDMAC030_CTRL_DST_INC;

	for (offset = 0; offset < len; offset += cycle) 
	{
		struct ftdmac030_desc *desc;

		cycle = min_t(size_t, (len - offset), MAX_CYCLE_PER_BLOCK);

		dev_dbg(chan2dev(chan), "[%s] offset=0x%x len=0x%x cycle=0x%x shift=0x%x\n",
			__func__,offset,len,cycle,shift);

		desc = ftdmac030_desc_get(ftchan);
		if (!desc) {
			goto err;
		}

		if (fixed_src)
			desc->lld.src = src;
		else
			desc->lld.src = src + offset;

		if (fixed_dest)
			desc->lld.dst = dest;
		else
			desc->lld.dst = dest + offset;

		desc->cfg = FTDMAC030_CFG_GW(GRANT_WINDOW)
			  | FTDMAC030_CFG_HIGH_PRIO | FTDMAC030_CFG_UNALIGNMODE;
		desc->len = cycle << shift;

		desc->lld.next = 0;
		desc->lld.cycle   = FTDMAC030_CYC_TOTAL(cycle);
		desc->lld.ctrl = ctrl;

		if (!first) 
		{
			first = desc;
		} 
		else 
		{
			/*
			 * Mask terminal count interrupt for this descriptor.
			 * What an inconvenient stupid design.
			 */
			prev->lld.ctrl |= FTDMAC030_CTRL_MASK_TC;

			/* hardware link list pointer */
			prev->lld.next = desc->txd.phys;

			/* insert the link descriptor to the transfer chain */
			list_add_tail(&desc->node, &first->child_list);
		}

		prev = desc;
	}

	return first;

err:
	if (first)
		ftdmac030_remove_chain(first);

	return NULL;
}

/**
 * ftdmac030_create_chain_2D_mode - create a DMA transfer chain
 * @ftchan: the channel to allocate descriptor for
 * @first: first descriptor of a transfer chain if any
 * @src: physical source address
 * @dest: phyical destination address
 * @len: length in bytes
 * @shift: shift value for width (0: byte, 1: halfword, 2: word)
 * @fixed_src: source address is fixed (device register)
 * @fixed_dest: destination address is fixed (device register)
 */
static struct ftdmac030_desc *ftdmac030_create_chain_2D_mode(
		struct ftdmac030_chan *ftchan,
		struct ftdmac030_desc *first,
		dma_addr_t src, dma_addr_t dest,
		unsigned int shift, int maxburst, int x_tcnt, int y_tcnt,
		int src_stride,int dst_stride)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *prev = NULL;
	struct ftdmac030_desc *desc;
	unsigned int ctrl;

	dev_dbg(chan2dev(chan), "%s(src 0x%x, dest 0x%x, shift %d maxburst %d)\n",
		__func__, src, dest, shift,maxburst);
	dev_dbg(chan2dev(chan), "%s(x_tcnt 0x%x, y_tcnt 0x%x, src_stride 0x%x, dst_stride 0x%x)\n",
		__func__, x_tcnt, y_tcnt, src_stride, dst_stride);

	if (first) 
	{
		if (list_empty(&first->child_list))
			prev = first;
		else
			prev = list_entry(first->child_list.prev,
				struct ftdmac030_desc, node);
	}

	ctrl = FTDMAC030_CTRL_ENABLE ;

	switch (maxburst) 
	{
	case 1:
		ctrl |= FTDMAC030_CTRL_1BEAT;
		break;
	case 2:
		ctrl |= FTDMAC030_CTRL_2BEATS;
		break;
	case 4:
		ctrl |= FTDMAC030_CTRL_4BEATS;
		break;
	case 8:
		ctrl |= FTDMAC030_CTRL_8BEATS;
		break;
	case 16:
		ctrl |= FTDMAC030_CTRL_16BEATS;
		break;
	case 32:
		ctrl |= FTDMAC030_CTRL_32BEATS;
		break;
	case 64:
		ctrl |= FTDMAC030_CTRL_64BEATS;
		break;
	case 128:
		ctrl |= FTDMAC030_CTRL_128BEATS;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect burst size\n",
			__func__);
		BUG();
		break;
	}  

	switch (shift) 
	{
	case 2:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_32
		     |  FTDMAC030_CTRL_SRC_WIDTH_32;
		break;
	case 1:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_16
		     |  FTDMAC030_CTRL_SRC_WIDTH_16;
		break;
	case 0:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_8
		     |  FTDMAC030_CTRL_SRC_WIDTH_8;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
		break;
	}

	ctrl |= (FTDMAC030_CTRL_SRC_INC | FTDMAC030_CTRL_DST_INC | FTDMAC030_CTRL_2D
		| FTDMAC030_CTRL_EXP);

	desc = ftdmac030_desc_get(ftchan);
	if (!desc) {
		goto err;
	}

	desc->lld.src = src;
	desc->lld.dst = dest;

	desc->cfg = FTDMAC030_CFG_GW(GRANT_WINDOW) | FTDMAC030_CFG_HIGH_PRIO ;
	desc->len = 0;

	desc->lld.next = 0;
	desc->lld.cycle   = ((y_tcnt&0xFFFF)<<16)|(x_tcnt&0xFFFF);
	desc->lld.ctrl = ctrl;
	desc->lld.stride  = ((dst_stride&0xFFFF)<<16)|(src_stride&0xFFFF);

	if (!first) 
	{
		first = desc;
	} 
	else 
	{
		prev->lld.ctrl |= FTDMAC030_CTRL_MASK_TC;
		prev->lld.next = desc->txd.phys;
		list_add_tail(&desc->node, &first->child_list);
	}

	prev = desc;

	return first;

err:
	if (first)
		ftdmac030_remove_chain(first);

	return NULL;
}

/**
 * ftdmac030_finish_chain - finish a descriptor
 * @desc: the descriptor to finish for
 */
static void ftdmac030_finish_chain(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	
	ftchan->completed_cookie = desc->txd.cookie;

	/*
	 * The API requires that no submissions are done from a
	 * callback, so we don't need to drop the lock here
	 */
	if (desc->txd.callback)
		desc->txd.callback(desc->txd.callback_param);
	
	ftdmac030_remove_chain(desc);
}

/**
 * ftdmac030_finish_chain - finish channel all desciptor chain
 * @ftchan: the ftchan to finish for
 */
static void ftdmac030_finish_all_chains(struct ftdmac030_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *desc;
	struct ftdmac030_desc *tmp;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	
	spin_lock(&ftchan->lock);
	
	list_for_each_entry_safe(desc, tmp, &ftchan->active_list, node) {
		list_del(&desc->node);
		//ftdmac030_finish_chain(desc);
		//should add to free_list
		ftdmac030_desc_put(desc);
	}
	
	spin_unlock(&ftchan->lock);
	
	ftchan->cyclic_callback = NULL;
	ftchan->cyclic_callback_param = NULL;
	ftchan->residue = 0;
	ftchan->period_len = 0;
	ftchan->buffer_len = 0;

	dev_dbg(chan2dev(chan), "%s Leave!!\n", __func__);
}

/******************************************************************************
 * tasklet function
 *****************************************************************************/
static void ftdmac030_tasklet(unsigned long data)
{
	struct ftdmac030_chan *ftchan = (struct ftdmac030_chan *)data;
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *desc;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	//BUG_ON(list_empty(&ftchan->active_list));

	if (! list_empty(&ftchan->active_list))
	{
		if (ftchan->cyclic)
		{
			ftchan->residue -= ftchan->period_len;
			if (ftchan->residue == 0)
				ftchan->residue = ftchan->buffer_len;
			
			if (ftchan->cyclic_callback)
				ftchan->cyclic_callback(ftchan->cyclic_callback_param);
		}
		else
		{
			spin_lock_bh(&ftchan->lock);
			/* remove already finished descriptor */
			desc = list_first_entry(&ftchan->active_list, struct ftdmac030_desc,node);
			list_del(&desc->node);

			/* check if there were another transfer to do */
			ftdmac030_start_new_chain(ftchan);

			ftdmac030_finish_chain(desc);
			spin_unlock_bh(&ftchan->lock);
		}
	}
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftdmac030_interrupt(int irq, void *dev_id)
{
	struct ftdmac030 *ftdmac030 = dev_id;
	unsigned int tcs;
	unsigned int eas;
	int i;

	tcs = ioread32(ftdmac030->base + FTDMAC030_OFFSET_TCISR);
	eas = ioread32(ftdmac030->base + FTDMAC030_OFFSET_EAISR);
	if (!tcs && !eas)
		return IRQ_NONE;

	/* clear status */
	if (tcs)
		iowrite32(tcs, ftdmac030->base + FTDMAC030_OFFSET_TCICR);
	if (eas)
		iowrite32(eas, ftdmac030->base + FTDMAC030_OFFSET_EAICR);

	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac030_chan *ftchan = &ftdmac030->channel[i];
		struct dma_chan *chan = &ftchan->common;

		if (eas & FTDMAC030_EA_ERR_CH(i))
			dev_dbg(chan2dev(chan), "error happened\n");

		if (eas & FTDMAC030_EA_ABT_CH(i))
			dev_dbg(chan2dev(chan), "transfer aborted\n");

		if (tcs & (1 << i)) {
			dev_dbg(chan2dev(chan), "terminal count\n");
			tasklet_schedule(&ftchan->tasklet);
		}
	}

	return IRQ_HANDLED;
}

/******************************************************************************
 * struct dma_async_tx_descriptor function
 *****************************************************************************/
static dma_cookie_t ftdmac030_new_cookie(struct ftdmac030_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	dma_cookie_t cookie = ftchan->common.cookie;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	
	if (++cookie < 0)
		cookie = DMA_MIN_COOKIE;

	return cookie;
}

/**
 * ftdmac030_tx_submit - set the prepared descriptor(s) to be executed by the engine
 * @txd: async transaction descriptor
 */
static dma_cookie_t ftdmac030_tx_submit(struct dma_async_tx_descriptor *txd)
{
	struct ftdmac030_desc *desc;
	struct ftdmac030_chan *ftchan;
	struct dma_chan *chan;
	dma_cookie_t cookie;

	desc = container_of(txd, struct ftdmac030_desc, txd);
	ftchan = desc->ftchan;
	chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s: submit desc %p\n", __func__, desc);
	BUG_ON(!async_tx_test_ack(txd)  && !ftchan->cyclic);

	if (ftchan->cyclic) {
		ftchan->cyclic_callback = desc->txd.callback;
		ftchan->cyclic_callback_param = desc->txd.callback_param;
	}

	cookie = ftdmac030_new_cookie(ftchan);
	ftchan->common.cookie = cookie;
	txd->cookie = cookie;
	list_add_tail(&desc->node, &ftchan->active_list);

	return cookie;
}

/**
 * ftdmac030_of_dma_simple_xlate - to do simple translate for slave dma
 * @dma_spec: target DMA channel
 * @ofdma: slave capability
 */
struct dma_chan *ftdmac030_of_dma_simple_xlate(struct of_phandle_args *dma_spec,struct of_dma *ofdma)
{
	struct ftdmac030_dma_slave *slave;
	int count = dma_spec->args_count;
	struct of_dma_filter_info *info = ofdma->of_dma_data;

	if (!info || !info->filter_fn)
		return NULL;

	if (count != 3)
		return NULL;

	slave = kmalloc(sizeof(*slave), GFP_KERNEL);
	if (!slave)
		return NULL;

	slave->id = dma_spec->args[0];
	slave->channels = dma_spec->args[1];
	slave->handshake = dma_spec->args[2];

	return dma_request_channel(info->dma_cap, info->filter_fn, slave);
}

/******************************************************************************
 * filter function for dma_request_channel()
 *****************************************************************************/
bool ftdmac030_chan_filter(struct dma_chan *chan, void *data)
{
	const char *drv_name = dev_driver_string(chan->device->dev);
	struct ftdmac030_dma_slave *slave = data;
	struct ftdmac030_chan *ftchan;
	struct ftdmac030 *ftdmac030;

	dev_dbg(chan2dev(chan), "%s\n", __func__);

	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftdmac030 = ftchan->ftdmac030;

	if (strncmp(DRV_NAME, drv_name, sizeof(DRV_NAME)))
		return false;

	//Choose FTDMAC030 ID
	if (slave->id >= 0 && slave->id != ftdmac030->id)
		return false;

	//Choose Channel ID, slave-> channels = 0xFF mean slave not choose channel id
	if (slave->channels != 0xFF)
	{
		if (slave->channels != chan->chan_id)
			return false;
	}

	chan->private = slave;
	return true;
}
EXPORT_SYMBOL_GPL(ftdmac030_chan_filter);

/******************************************************************************
 * struct dma_device functions
 *****************************************************************************/
/**
 * ftdmac030_alloc_chan_resources - allocate resources for DMA channel
 * @chan: DMA channel
 */
static int ftdmac030_alloc_chan_resources(struct dma_chan *chan)
{
	struct ftdmac030_chan *ftchan;
	LIST_HEAD(tmp_list);
	int i;

	dev_dbg(chan2dev(chan), "%s\n", __func__);

	ftchan = container_of(chan, struct ftdmac030_chan, common);

	/* have we already been set up?
	 * reconfigure channel but no need to reallocate descriptors */
	if (!list_empty(&ftchan->free_list))
		return ftchan->descs_allocated;

	/* Allocate initial pool of descriptors */
	for (i = 0; i < init_nr_desc_per_channel; i++) {
		struct ftdmac030_desc *desc;

		desc = ftdmac030_alloc_desc(ftchan, GFP_KERNEL);
		if (!desc) {
			dev_err(chan2dev(chan),
				"Only %d initial descriptors\n", i);
			break;
		}
		list_add_tail(&desc->node, &tmp_list);
	}

	spin_lock_bh(&ftchan->lock);
	ftchan->descs_allocated = i;
	list_splice(&tmp_list, &ftchan->free_list);
	ftchan->completed_cookie = chan->cookie = 1;
	spin_unlock_bh(&ftchan->lock);

	dev_dbg(chan2dev(chan), "%s: allocated %d descriptors\n",
		__func__, ftchan->descs_allocated);

	return ftchan->descs_allocated;
}

/**
 * ftdmac030_free_chan_resources - free all channel resources
 * @chan: DMA channel
 */
static void ftdmac030_free_chan_resources(struct dma_chan *chan)
{
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc;
	struct ftdmac030_desc *tmp;
	struct ftdmac030 *ftdmac030;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftdmac030 = ftchan->ftdmac030;

	/* channel must be idle */
	BUG_ON(!list_empty(&ftchan->active_list));
	BUG_ON(ftdmac030_chan_is_enabled(ftchan));

	ftdmac030_stop_channel(ftchan);
	ftdmac030_finish_all_chains(ftchan);

	spin_lock_bh(&ftchan->lock);
	list_for_each_entry_safe(desc, tmp, &ftchan->free_list, node) {
		list_del(&desc->node);
		ftdmac030_free_desc(desc);
	}
	spin_unlock_bh(&ftchan->lock);

	ftchan->descs_allocated = 0;
	ftchan->cyclic_callback = NULL;
	ftchan->cyclic_callback_param = NULL;
	ftchan->residue = 0;
	ftchan->period_len = 0;
	ftchan->buffer_len = 0;
}

/**
 * ftdmac030_prep_dma_memcpy - prepare a memcpy operation
 * @chan: the channel to prepare operation on
 * @dest: operation virtual destination address
 * @src: operation virtual source address
 * @len: operation length
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
ftdmac030_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest,
		dma_addr_t src, size_t len, unsigned long flags)
{
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc;
	unsigned int shift;

	dev_dbg(chan2dev(chan), "%s(src %x, dest %x, len %x)\n",
		__func__, src, dest, len);
	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftchan->cyclic = false;

	if (unlikely(!len)) {
		dev_info(chan2dev(chan), "%s: length is zero!\n", __func__);
		return NULL;
	}

	/*
	 * We can be a lot more clever here, but this should take care
	 * of the most common optimization.
	 */
	if (!((src | dest | len) & 3)) {
		shift = 2;
	} else if (!((src | dest | len) & 1)) {
		shift = 1;
	} else {
		shift = 0;
	}

	desc = ftdmac030_create_chain(ftchan, NULL, src, dest, len,
		shift, 1, 0, 0);
	if (!desc)
		goto err;

	desc->txd.flags = flags;

	return &desc->txd;

err:
	return NULL;
}

/**
 * ftdmac030_prep_dma_memset - prepares a memset operation
 * @chan: the channel to prepare operation on
 * @dest: operation virtual destination address
 * @value: write value
 * @len: operation length
 * @flags: tx descriptor status flags
 */
struct dma_async_tx_descriptor *
ftdmac030_prep_dma_memset(struct dma_chan *chan, dma_addr_t dest, int value, 
		size_t len, unsigned long flags)
{
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc;
	dma_addr_t align_dest;
	size_t align_len;
	unsigned int word_value;
	size_t end_dest_len;

	dev_dbg(chan2dev(chan), "[%s] dest=0x%x, len=0x%x, value=0x%x\n",
		__func__, dest, len, value);

	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftchan->cyclic = false;

	if (unlikely(!len)) {
		dev_info(chan2dev(chan), "%s: length is zero!\n", __func__);
		return NULL;
	}

	if (len < 8) {
		dev_info(chan2dev(chan), "%s: length is less than 8! HW not support!!\n",
			__func__);
		return NULL;
	}

	//FTDMAC030 Write-Only mode Constant value only support 32-bit data 
	//       & alignment, so need to fill not alignment address
	if (dest & 0x3)
	{//unalignment dest
		align_dest = ((dest + 4) & 0xfffffffc);
		align_len = len & 0xfffffffc;
		if ((align_dest + align_len) > (dest + len))
			align_len -= 4;
	}
	else 
	{//alignment dest
		align_dest = dest;
		align_len = len & 0xfffffffc;
	}
	end_dest_len = dest + len - align_dest - align_len; 
	word_value = ((unsigned int)value << 24) | ((unsigned int)value << 16) | 
	             ((unsigned int)value << 8) | ((unsigned int)value); 

	dev_dbg(chan2dev(chan), "[%s] (align_dest=0x%x, align_len=0x%x, word_value=0x%x,end_dest_len=0x%x)\n",
		__func__, align_dest, align_len, word_value, end_dest_len);

	desc = ftdmac030_create_chain_constant_value(ftchan, NULL, align_dest, 
		align_len, word_value,2,1);
	if (!desc)
		goto err;

	//check if need move data to unalignment byte of dest buffer
	if (dest != align_dest)
	{
		desc = ftdmac030_create_chain_unalign_mode(ftchan,desc,align_dest,dest,
			(align_dest-dest),0,1,1,0);
		if (!desc)
			goto err;
	} 

	//check if need move data to unalignment byte for dest buffer end
	if (end_dest_len != 0) 
	{
		desc = ftdmac030_create_chain_unalign_mode(ftchan,desc,align_dest,
		(dest+len-end_dest_len),end_dest_len,0,1,1,0);
		if (!desc)
			goto err;
	} 

	desc->txd.cookie = -EBUSY;
	desc->txd.flags = flags;
	return &desc->txd;   

err:
	return 0;
}

/**
 * ftdmac030_prep_dma_memset_sg - prepares a memset operation over a 
 *                                scatter list
 * @chan: the channel to prepare operation on
 * @sg: 
 * @nents:
 * @value: 
 * @flags: tx descriptor status flags
 */
struct dma_async_tx_descriptor *
ftdmac030_prep_dma_memset_sg(struct dma_chan *chan, struct scatterlist *sg,
		unsigned int nents, int value, unsigned long flags)
{
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc = NULL;
	struct scatterlist *sgl;
	dma_addr_t dest;
	size_t len;
	dma_addr_t align_dest;
	size_t align_len;
	size_t end_dest_len;
	unsigned int word_value;
	unsigned int i;

	dev_dbg(chan2dev(chan), "%s\n", __func__);

	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftchan->cyclic = false;

	if (unlikely(!nents)) 
	{
		dev_err(chan2dev(chan), "%s: scatter list length is 0\n",
			__func__);
		return NULL;
	}

	for_each_sg(sg, sgl, nents, i) 
	{
		dest = sg_dma_address(sgl);
		len = sg_dma_len(sgl);

		//dev_dbg(chan2dev(chan), "[%s] dest = 0x%x len = 0x%x i=%d\n", __func__,dest,len,i);

		//FTDMAC030 Write-Only mode Constant value only support 32-bit data 
		//       & alignment, so need to fill not alignment address
		if (dest & 0x3)
		{//unalignment dest
			align_dest = ((dest + 4) & 0xfffffffc);
			align_len = len & 0xfffffffc;
			if ((align_dest + align_len) > (dest + len))
				align_len -= 4;
		}    
		else 
		{//alignment dest
			align_dest = dest;
			align_len = len & 0xfffffffc;
		}
		end_dest_len = dest + len - align_dest - align_len; 
		word_value = ((unsigned int)value << 24) | ((unsigned int)value << 16) | 
			((unsigned int)value << 8) | ((unsigned int)value); 

		dev_dbg(chan2dev(chan), "[%s] (align_dest=0x%x, align_len=0x%x, word_value=0x%x, end_dest_len=0x%x)\n",
			__func__, align_dest, align_len, word_value, end_dest_len);

		desc = ftdmac030_create_chain_constant_value(ftchan, desc, align_dest, 
			align_len, word_value,2,1);
		if (!desc)
			goto err;

		//check if need move data to unalignment byte of dest buffer
		if (dest != align_dest)
		{
			desc = ftdmac030_create_chain_unalign_mode(ftchan,desc,align_dest,dest,
				(align_dest-dest),0,1,1,0);
			if (!desc)
				goto err;   
		} 

		//check if need move data to unalignment byte for dest buffer end
		if (end_dest_len != 0) 
		{
			desc = ftdmac030_create_chain_unalign_mode(ftchan,desc,align_dest,
				(dest+len-end_dest_len),end_dest_len,0,1,1,0);
			if (!desc)
				goto err;
		}
	}

	desc->txd.cookie = -EBUSY;
	desc->txd.flags = flags;
	return &desc->txd;   

err:
	return 0;
}

/**
 * ftdmac030_prep_slave_sg - prepare descriptors for a DMA_SLAVE transaction
 * @chan: DMA channel
 * @sgl: scatterlist to transfer to/from
 * @sg_len: number of entries in @scatterlist
 * @direction: DMA direction
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
ftdmac030_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct ftdmac030_dma_slave *slave = chan->private;
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc = NULL;
	struct scatterlist *sg;
	unsigned int shift;
	unsigned int i;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftchan->cyclic = false;

	if (unlikely(!slave)) {
		dev_err(chan2dev(chan), "%s: no slave data\n", __func__);
		return NULL;
	}

	if (unlikely(!sg_len)) {
		dev_err(chan2dev(chan), "%s: scatter list length is 0\n",
			__func__);
		return NULL;
	}

	if (unlikely(slave->common.src_addr_width !=
			slave->common.dst_addr_width)) {
		dev_err(chan2dev(chan), "%s: data width mismatched\n",
			__func__);
		return NULL;
	}

	switch (slave->common.src_addr_width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		shift = 0;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		shift = 1;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		shift = 2;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
	}

	switch (direction) {
	case DMA_TO_DEVICE:
		for_each_sg(sgl, sg, sg_len, i) {
			struct ftdmac030_desc *tmp;
			unsigned int len;
			dma_addr_t mem;

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);

			tmp = ftdmac030_create_chain(ftchan, desc,
				mem, slave->common.dst_addr, len,
				shift, slave->common.src_maxburst, 0, 1);
			if (!tmp)
				goto err;

			if (!desc)
				desc = tmp;
		}

		if ((slave->handshake >= 0) && (slave->handshake <= 15))
		{
			ftdmac030_enable_handshake_sync(ftchan,slave->handshake);	
			
			desc->cfg = FTDMAC030_CFG_DST_HANDSHAKE_EN
				| FTDMAC030_CFG_DST_HANDSHAKE(slave->handshake);
		}
		break;
	case DMA_FROM_DEVICE:
		for_each_sg(sgl, sg, sg_len, i) {
			struct ftdmac030_desc *tmp;
			unsigned int len;
			dma_addr_t mem;

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);

			tmp = ftdmac030_create_chain(ftchan, desc,
				slave->common.src_addr, mem, len,
				shift, slave->common.src_maxburst, 1, 0);
			if (!tmp)
				goto err;

			if (!desc)
				desc = tmp;
		}

		if ((slave->handshake >= 0) && (slave->handshake <= 15))
		{
			ftdmac030_enable_handshake_sync(ftchan,slave->handshake);	
			
			desc->cfg = FTDMAC030_CFG_SRC_HANDSHAKE_EN
				| FTDMAC030_CFG_SRC_HANDSHAKE(slave->handshake);
		}
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect direction\n",
			__func__);
		goto err;
	}

	desc->txd.flags = flags;
	return &desc->txd;

err:
	return NULL;
}

/**
 * ftdmac030_prep_dma_cyclic - prepare a cyclic dma operation suitable 
 * for audio.
 * The function takes a buffer of size buf_len. The callback function will
 * be called after period_len bytes have been transferred.
 * @chan: DMA channel
 * @sgl: scatterlist to transfer to/from
 * @sg_len: number of entries in @scatterlist
 * @direction: DMA direction
 * @flags: tx descriptor status flags
 */
struct dma_async_tx_descriptor *
ftdmac030_prep_dma_cyclic(struct dma_chan *chan, dma_addr_t buf_addr, 
		size_t buf_len,size_t period_len, enum dma_transfer_direction direction,
		unsigned long flags)
{
	struct ftdmac030_dma_slave *slave = chan->private;
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc = NULL;
	struct ftdmac030_desc *first = NULL;
	unsigned int shift;
	size_t offset;

	dev_dbg(chan2dev(chan), "%s: buf_len = 0x%x period_len = 0x%x\n", __func__,
		buf_len,period_len);

	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftchan->cyclic = true;
	ftchan->residue = buf_len;
	ftchan->period_len = period_len;
	ftchan->buffer_len = buf_len;

	if (unlikely(!slave)) {
		dev_err(chan2dev(chan), "%s: no slave data\n", __func__);
		return NULL;
	}

	switch (direction) {
	case DMA_TO_DEVICE:  
		switch (slave->common.dst_addr_width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			shift = 0;
			break;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			shift = 1;
			break;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			shift = 2;
			break;
		default:
			dev_err(chan2dev(chan), "%s: incorrect dst data width\n",
				__func__);
			BUG();
		}

		for (offset = 0; offset < buf_len; offset += period_len) {
			desc = ftdmac030_create_chain_cyclic(ftchan, desc,
				buf_addr + offset, slave->common.dst_addr, period_len,
				shift, slave->common.dst_maxburst, 0, 1);
			if (!desc){
				goto err;
			}

			if ((slave->handshake >= 0) && (slave->handshake <= 15)) {
				ftdmac030_enable_handshake_sync(ftchan,slave->handshake);
				
				desc->cfg = FTDMAC030_CFG_DST_HANDSHAKE_EN
					| FTDMAC030_CFG_DST_HANDSHAKE(slave->handshake);
			}

			if (!first)
				first = desc;
		}
		break;
	case DMA_FROM_DEVICE:
		switch (slave->common.src_addr_width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			shift = 0;
			break;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			shift = 1;
			break;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			shift = 2;
			break;
		default:
			dev_err(chan2dev(chan), "%s: incorrect src data width\n",__func__);
			BUG();
		}
		for (offset = 0; offset < buf_len; offset += period_len) {
			desc = ftdmac030_create_chain_cyclic(ftchan, desc,
				slave->common.src_addr, buf_addr + offset, period_len,
				shift, slave->common.src_maxburst, 1, 0);
			if (!desc){
				goto err;
			}

			if ((slave->handshake >= 0) && (slave->handshake <= 15))
			{
				ftdmac030_enable_handshake_sync(ftchan,slave->handshake);	
				
				desc->cfg = FTDMAC030_CFG_SRC_HANDSHAKE_EN
					| FTDMAC030_CFG_SRC_HANDSHAKE(slave->handshake);
			}

			if (!first)
				first = desc;
		}
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect direction\n",
			__func__);
		goto err;
	}

	desc->txd.flags = flags;
	
	return &first->txd;

err:
	if (first)
		ftdmac030_remove_chain(first);

	return NULL;
}

/**
 * ftdmac030_prep_interleaved_dma - Transfer expression in a generic way.
 * @chan: the channel to prepare operation on
 * @xt:
 * @flags:
 */
struct dma_async_tx_descriptor *
ftdmac030_prep_interleaved_dma(struct dma_chan *chan, 
		struct dma_interleaved_template *xt, unsigned long flags)
{
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc = NULL;
	unsigned int shift;
	//int i = 0;
	//int j = 0;

	dma_addr_t src_start = 0;
	dma_addr_t dst_start = 0;
	dma_addr_t align_src_start = 0;
	dma_addr_t align_dst_start = 0;
	enum dma_transfer_direction dir = DMA_TO_DEVICE;
	bool src_inc = 0;
	bool dst_inc = 0;
	bool src_sgl = 0;
	bool dst_sgl = 0;
	size_t numf = 0;
	size_t frame_size = 0;

	dev_dbg(chan2dev(chan), "[%s] src_start=0x%x dst_start=0x%x dir=0x%x\n", __func__,xt->src_start,xt->dst_start,xt->dir);
	dev_dbg(chan2dev(chan), "[%s] src_inc=0x%x dst_inc=0x%x src_sgl=0x%x\n", __func__,xt->src_inc,xt->dst_inc,xt->src_sgl);
	dev_dbg(chan2dev(chan), "[%s] dst_sgl=0x%x numf=0x%x frame_size=0x%x\n", __func__,xt->dst_sgl,xt->numf,xt->frame_size);
	dev_dbg(chan2dev(chan), "[%s] sgl[0].size=0x%x sgl[0].icg=0x%x\n", __func__,xt->sgl[0].size,xt->sgl[0].icg);
	dev_dbg(chan2dev(chan), "[%s] sgl[0].dst_icg=0x%x sgl[0].src_icg=0x%x\n", __func__,xt->sgl[0].dst_icg,xt->sgl[0].src_icg); 

	if (!xt->numf || !xt->sgl[0].size)
		return NULL;

	if (xt != NULL)
	{
		src_start = xt->src_start;
		dst_start = xt->dst_start;
		align_src_start = ((src_start + 4) & 0xFFFFFFFC);
		align_dst_start = ((dst_start + 4) & 0xFFFFFFFC);
		dir = xt->dir;
		src_inc = xt->src_inc;
		dst_inc = xt->dst_inc;
		src_sgl = xt->src_sgl;
		dst_sgl = xt->dst_sgl;
		numf = xt->numf;
		frame_size = xt->frame_size;
	}

	//dev_dbg(chan2dev(chan), "[%s] src_start=0x%x dst_start=0x%x dir=0x%x\n", __func__,src_start,dst_start,dir);
	//dev_dbg(chan2dev(chan), "[%s] src_inc=0x%x dst_inc=0x%x src_sgl=0x%x\n", __func__,src_inc,dst_inc,src_sgl);
	//dev_dbg(chan2dev(chan), "[%s] dst_sgl=0x%x numf=0x%x frame_size=0x%x\n", __func__,dst_sgl,numf,frame_size); 

	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftchan->cyclic = false;

	shift = 0;

	//1D DMA Path
	/*for (i=0;i<numf;i++)
	{//number of frame
		for (j=0;j<frame_size;j++)
		{//number of chunks
			desc = ftdmac030_create_chain(ftchan,desc,src_start,dst_start,xt->sgl[0].size, shift, 1,0,0);
			src_start = src_start + xt->sgl[0].src_icg;
			dst_start = dst_start + xt->sgl[0].dst_icg;
		}
		src_start += xt->sgl[0].icg;
		dst_start += xt->sgl[0].icg;
	}*/

	//2D DMA Path
	desc = ftdmac030_create_chain_2D_mode(ftchan,NULL,src_start, dst_start,
	                                      shift, 1, xt->sgl[0].size, xt->frame_size,
	                                      xt->sgl[0].src_icg,xt->sgl[0].dst_icg);

	desc->txd.cookie = -EBUSY;
	desc->txd.flags = flags;

	return &desc->txd;
}

/**
 * ftdmac030_slave_config - prepares a slave dma operation
 * @chan: the channel to prepare operation on
 * @config: dma slave config need to setting
 */
static int ftdmac030_slave_config(struct dma_chan *chan,
				  struct dma_slave_config *config)
{
	struct ftdmac030_dma_slave *slave;

	dev_dbg(chan2dev(chan), "%s\n", __func__);

	slave = (struct ftdmac030_dma_slave *) chan->private;

	if (config->direction == DMA_MEM_TO_DEV)
		slave->common.dst_addr = config->dst_addr;
	else if (config->direction == DMA_DEV_TO_MEM)
		slave->common.src_addr = config->src_addr;
	else
		return -EINVAL;

	slave->common.dst_maxburst = config->dst_maxburst;
	slave->common.src_maxburst = config->src_maxburst;
	slave->common.dst_addr_width = config->dst_addr_width;
	slave->common.src_addr_width = config->src_addr_width;

	return 0;
}

/**
 * ftdmac030_terminate_all - Aborts all transfers on a channel. Returns 0
 *	                          or an error code
 * @chan: the channel to prepare operation on
 */
static int ftdmac030_terminate_all(struct dma_chan *chan)
{
	struct ftdmac030_chan *ftchan;

	dev_dbg(chan2dev(chan), "%s\n", __func__);

	ftchan = container_of(chan, struct ftdmac030_chan, common);

	ftdmac030_stop_channel(ftchan);
	ftdmac030_finish_all_chains(ftchan);

	return 0;
}

/**
 * ftdmac030_tx_status - poll for transaction completion, the optional
 * txstate parameter can be supplied with a pointer to get a
 * struct with auxiliary transfer status information, otherwise the call
 * will just return a simple status code
 * @chan: DMA channel
 * @cookie: transaction identifier to check status of
 * @txstate: if not %NULL updated with transaction state
 *
 * If @txstate is passed in, upon return it reflect the driver
 * internal state and can be used with dma_async_is_complete() to check
 * the status of multiple cookies without re-checking hardware state.
 */
static enum dma_status
ftdmac030_tx_status(struct dma_chan *chan, dma_cookie_t cookie,
		struct dma_tx_state *txstate)
{
	struct ftdmac030_chan *ftchan;
	dma_cookie_t last_used;
	dma_cookie_t last_complete;
	enum dma_status ret;
	int residue = 0;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac030_chan, common);

	last_complete = ftchan->completed_cookie;
	last_used = chan->cookie;

	if (ftchan->cyclic)
		residue = ftchan->residue;
	else
		residue = ioread32(ftchan->ftdmac030->base + FTDMAC030_OFFSET_CYC_CH(ftchan->common.chan_id));

	ret = dma_async_is_complete(cookie, last_complete, last_used);
	dma_set_tx_state(txstate, last_complete, last_used, residue);
	
	return ret;
}

/**
 * ftdmac030_issue_pending - push pending transactions to hardware
 * @chan: target DMA channel
 */
static void ftdmac030_issue_pending(struct dma_chan *chan)
{
	struct ftdmac030_chan *ftchan;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac030_chan, common);

	ftdmac030_start_new_chain(ftchan);
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftdmac030_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct ftdmac030 *ftdmac030;
	struct dma_device *dma;
	int irq;
	int i;
	int err;
	int tmp_id;

	if (pdev == NULL)
		return -ENODEV;

	if ((res = platform_get_resource(pdev, IORESOURCE_MEM, 0)) == 0) {
		return -ENXIO;
	}

	if ((irq = platform_get_irq(pdev, 0)) < 0) {
		return irq;
	}

	ftdmac030 = kzalloc(sizeof(*ftdmac030), GFP_KERNEL);
	if (!ftdmac030) {
		return -ENOMEM;
	}

	dma = &ftdmac030->dma;

	INIT_LIST_HEAD(&dma->channels);
	dma->dev = &pdev->dev;

	platform_set_drvdata(pdev, ftdmac030);
	
	if (pdev->dev.of_node) 
	{
		of_property_read_u32(pdev->dev.of_node, "dev_id", &tmp_id);
		ftdmac030->id = tmp_id;
	}
	else 
	{
		ftdmac030->id = pdev->id;
	}
	

	ftdmac030->res = request_mem_region(res->start, res->end - res->start + 1,
			dev_name(&pdev->dev));
	if (ftdmac030->res == NULL) {
		dev_err(&pdev->dev, "Could not reserve memory region\n");
		err = -ENOMEM;
		goto err_req_mem;
	}

	ftdmac030->base = ioremap(res->start, res->end - res->start + 1);
	if (ftdmac030->base == NULL) {
		dev_err(&pdev->dev, "Failed to ioremap ftdmac030 registers\n");
		err = -EIO;
		goto err_ioremap;
	}

	/* create a pool of consistent memory blocks for hardware descriptors */
	ftdmac030->dma_desc_pool = dma_pool_create("ftdmac030_desc_pool",
			&pdev->dev, sizeof(struct ftdmac030_desc),
			4 /* word alignment */, 0);
	if (!ftdmac030->dma_desc_pool) {
		dev_err(&pdev->dev, "No memory for descriptor pool\n");
		err = -ENOMEM;
		goto err_pool_create;
	}

	/* initialize channels */
	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac030_chan *ftchan = &ftdmac030->channel[i];

		INIT_LIST_HEAD(&ftchan->active_list);
		INIT_LIST_HEAD(&ftchan->free_list);
		ftchan->common.device	= dma;
		ftchan->common.cookie	= DMA_MIN_COOKIE;

		spin_lock_init(&ftchan->lock);
		ftchan->ftdmac030		= ftdmac030;
		ftchan->completed_cookie	= DMA_MIN_COOKIE;

		tasklet_init(&ftchan->tasklet, ftdmac030_tasklet,
				(unsigned long)ftchan);
		list_add_tail(&ftchan->common.device_node, &dma->channels);
	}

	/* initialize dma_device */
	dma->device_alloc_chan_resources = ftdmac030_alloc_chan_resources;
	dma->device_free_chan_resources  = ftdmac030_free_chan_resources;
	dma->device_prep_dma_memcpy      = ftdmac030_prep_dma_memcpy;
	dma->device_prep_dma_memset      = ftdmac030_prep_dma_memset;
	dma->device_prep_dma_memset_sg   = ftdmac030_prep_dma_memset_sg;
	dma->device_prep_slave_sg        = ftdmac030_prep_slave_sg;
	dma->device_prep_dma_cyclic      = ftdmac030_prep_dma_cyclic;
	dma->device_prep_interleaved_dma = ftdmac030_prep_interleaved_dma;
	dma->device_config               = ftdmac030_slave_config;
	dma->device_terminate_all        = ftdmac030_terminate_all;
	dma->device_tx_status            = ftdmac030_tx_status;
	dma->device_issue_pending        = ftdmac030_issue_pending;

	/* slave_caps */
	dma->src_addr_widths = DMAC030_BUSWIDTHS;
	dma->dst_addr_widths = DMAC030_BUSWIDTHS;
	dma->directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV) | BIT(DMA_MEM_TO_MEM);
	dma->residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;

	/* set DMA capability */
	dma_cap_set(DMA_MEMCPY, dma->cap_mask);
	dma_cap_set(DMA_MEMSET, dma->cap_mask);
	dma_cap_set(DMA_MEMSET_SG, dma->cap_mask);
	dma_cap_set(DMA_SLAVE, dma->cap_mask);
	dma_cap_set(DMA_CYCLIC, dma->cap_mask);
	dma_cap_set(DMA_INTERLEAVE, dma->cap_mask);
   
	/* set alignment */
	dma->copy_align = true;
	dma->xor_align  = true;
	dma->pq_align   = true;
	dma->fill_align = true;

	err = request_irq(irq, ftdmac030_interrupt, IRQF_SHARED, pdev->name,
		ftdmac030);
	if (err) {
		dev_err(&pdev->dev, "failed to request irq %d\n", irq);
		goto err_req_irq;
	}

	ftdmac030->irq = irq;

	err = dma_async_device_register(dma);
	if (err) {
		dev_err(&pdev->dev, "failed to register dma device\n");
		goto err_register;
	}

	ftdmac030_dma_info.dma_cap = dma->cap_mask;

	/* Device-tree DMA controller registration */
	if (pdev->dev.of_node) {
		err = of_dma_controller_register(pdev->dev.of_node,
			ftdmac030_of_dma_simple_xlate, &ftdmac030_dma_info);
		if (err) {
			dev_err(&pdev->dev, "Failed to register DMA controller (Device Tree)!!\n");
			goto err_register;
		}
	}

	dev_info(&pdev->dev,
		"DMA engine driver: irq %d, mapped at %p\n",
		irq, ftdmac030->base);

	return 0;

err_register:
	free_irq(irq, ftdmac030);
err_req_irq:
	dma_pool_destroy(ftdmac030->dma_desc_pool);
err_pool_create:
	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac030_chan *ftchan = &ftdmac030->channel[i];

		list_del(&ftchan->common.device_node);
		tasklet_kill(&ftchan->tasklet);
	}

	iounmap(ftdmac030->base);
err_ioremap:
	release_resource(ftdmac030->res);
err_req_mem:
	platform_set_drvdata(pdev, NULL);
	kfree(ftdmac030);
	return err;
}

static int __exit ftdmac030_remove(struct platform_device *pdev)
{
	struct ftdmac030 *ftdmac030;
	int i;

	ftdmac030 = platform_get_drvdata(pdev);

	dma_async_device_unregister(&ftdmac030->dma);

	free_irq(ftdmac030->irq, ftdmac030);
	dma_pool_destroy(ftdmac030->dma_desc_pool);

	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac030_chan *ftchan = &ftdmac030->channel[i];

		tasklet_disable(&ftchan->tasklet);
		tasklet_kill(&ftchan->tasklet);
		list_del(&ftchan->common.device_node);
	}

	iounmap(ftdmac030->base);
	release_resource(ftdmac030->res);

	platform_set_drvdata(pdev, NULL);
	kfree(ftdmac030);
	return 0;
}

static const struct of_device_id ftdmac030_dt_ids[] = {
	{ .compatible = "faraday,ftdmac030", },
	{},
};

static struct platform_driver ftdmac030_driver = {
	.probe		= ftdmac030_probe,
	.remove		= __exit_p(ftdmac030_remove),
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = ftdmac030_dt_ids,
	},
};

/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftdmac030_init(void)
{
	return platform_driver_register(&ftdmac030_driver);
}

static void __exit ftdmac030_exit(void)
{
	platform_driver_unregister(&ftdmac030_driver);
}

module_init(ftdmac030_init);
module_exit(ftdmac030_exit);

MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_DESCRIPTION("FTDMAC030 DMA engine driver");
MODULE_LICENSE("GPL");
