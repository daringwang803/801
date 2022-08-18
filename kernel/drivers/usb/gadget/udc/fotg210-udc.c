/*
 * FOTG210 UDC Driver
 *
 * Copyright (C) 2013-2019 Faraday Technology Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "fotg210.h"

#define	DRIVER_DESC	"FOTG210 USB Device Controller Driver"

/* Note: CONFIG_USB_FOTG210_UDC_DOUBLE_FIFO usage
 * EP1 = FIFO 0~1, EP2 = FIFO 2~3
 * Please note that FIFO number is from 0~3, if user has configured EP numbers more than 2,
 * user can't define CONFIG_USB_FOTG210_UDC_DOUBLE_FIFO
 */

static const char udc_name[] = "fotg210_udc";
static const char * const fotg210_ep_name[] = {
	"ep0", "ep1", "ep2", "ep3", "ep4"};

#ifdef CONFIG_USB_FOTG210_UDC_VDMA
static u32 vdma_overlap = 0;	//To judge how many EPs use VDMA
#endif

static void fotg210_disable_fifo_int(struct fotg210_ep *ep)
{
	u32 value = ioread32(ep->fotg210->reg + FOTG210_DMISGR1);

	if (ep->dir_in)
		value |= DMISGR1_MF_IN_INT(ep->fifonum);
	else
		value |= DMISGR1_MF_OUTSPK_INT(ep->fifonum);
	iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR1);
}

static void fotg210_enable_fifo_int(struct fotg210_ep *ep)
{
	u32 value = ioread32(ep->fotg210->reg + FOTG210_DMISGR1);

	if (ep->dir_in)
		value &= ~DMISGR1_MF_IN_INT(ep->fifonum);
	else
		value &= ~DMISGR1_MF_OUTSPK_INT(ep->fifonum);
	iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR1);
}

static void fotg210_set_cxdone(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DCFESR);

	value |= DCFESR_CX_DONE;
	iowrite32(value, fotg210->reg + FOTG210_DCFESR);
}

static void fotg210_done(struct fotg210_ep *ep, struct fotg210_request *req,
			int status)
{
	list_del_init(&req->queue);

	/* don't modify queue heads during completion callback */
	if (ep->fotg210->gadget.speed == USB_SPEED_UNKNOWN)
		req->req.status = -ESHUTDOWN;
	else
		req->req.status = status;

	spin_unlock(&ep->fotg210->lock);
	if (req->req.complete)
		usb_gadget_giveback_request(&ep->ep, &req->req);
	spin_lock(&ep->fotg210->lock);

	if (ep->epnum) {
		fotg210_disable_fifo_int(ep);
		if (!list_empty(&ep->queue))
			fotg210_enable_fifo_int(ep);
	} else {
		fotg210_set_cxdone(ep->fotg210);
	}
}

static void fotg210_fifo_ep_mapping(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;

	/* Driver should map an ep to a fifo and then map the fifo
	 * to the ep.
	 */

	/* map a fifo to an ep */
	val = ioread32(fotg210->reg + FOTG210_EPMAP);
	val &= ~EPMAP_FIFONOMSK(ep->epnum, ep->dir_in);
	val |= EPMAP_FIFONO(ep->fifonum, ep->epnum, ep->dir_in);
	iowrite32(val, fotg210->reg + FOTG210_EPMAP);

	/* map the ep to the fifo */
	val = ioread32(fotg210->reg + FOTG210_FIFOMAP);
	val &= ~FIFOMAP_EPNOMSK(ep->fifonum);
	val |= FIFOMAP_EPNO(ep->fifonum, ep->epnum);
	iowrite32(val, fotg210->reg + FOTG210_FIFOMAP);

	/* enable fifo */
	val = ioread32(fotg210->reg + FOTG210_FIFOCF);
	val |= FIFOCF_FIFO_EN(ep->fifonum);
	iowrite32(val, fotg210->reg + FOTG210_FIFOCF);
}

static void fotg210_set_fifo_dir(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;

	val = ioread32(fotg210->reg + FOTG210_FIFOMAP);
	val &= ~FIFOMAP_NA(ep->fifonum);
	val |= (ep->dir_in ? FIFOMAP_DIRIN(ep->fifonum) : FIFOMAP_DIROUT(ep->fifonum));
	iowrite32(val, fotg210->reg + FOTG210_FIFOMAP);
}

static void fotg210_set_tfrtype(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;

	val = ioread32(fotg210->reg + FOTG210_FIFOCF);
	val |= FIFOCF_TYPE(ep->type, ep->fifonum);
#ifdef CONFIG_USB_FOTG210_UDC_DOUBLE_FIFO
	val |= FIFOCF_TYPE(ep->type, ep->fifonum + 1);
	val |= FIFOCF_BLK_DUB(ep->fifonum);
	val |= FIFOCF_BLK_DUB(ep->fifonum + 1);
#endif
	iowrite32(val, fotg210->reg + FOTG210_FIFOCF);
}

static void fotg210_set_tfrsize(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;

	val = ioread32(fotg210->reg + FOTG210_FIFOCF);

	if(ep->ep.maxpacket <= 512)
		val &= ~FIFOCF_BLKSZ_1024(ep->fifonum);
	else
		val |= FIFOCF_BLKSZ_1024(ep->fifonum);

	iowrite32(val, fotg210->reg + FOTG210_FIFOCF);
}

static void fotg210_set_mps(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;
	u32 offset = ep->dir_in ? FOTG210_INEPMPSR(ep->epnum) :
				FOTG210_OUTEPMPSR(ep->epnum);

	val = ioread32(fotg210->reg + offset);
	val &= ~INOUTEPMPSR_MPS(0x7FF);
	val |= INOUTEPMPSR_MPS(ep->ep.maxpacket);
	iowrite32(val, fotg210->reg + offset);
}

static int fotg210_config_ep(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;

	fotg210_set_fifo_dir(ep);
	fotg210_set_tfrtype(ep);
	fotg210_set_tfrsize(ep);
	fotg210_set_mps(ep);
	fotg210_fifo_ep_mapping(ep);

	fotg210->ep[ep->epnum] = ep;

	return 0;
}

static int fotg210_ep_enable(struct usb_ep *_ep,
			  const struct usb_endpoint_descriptor *desc)
{
	struct fotg210_ep *ep;

	ep = container_of(_ep, struct fotg210_ep, ep);

	ep->ep.desc = desc;
	ep->epnum = usb_endpoint_num(desc);
	ep->type = usb_endpoint_type(desc);
	ep->dir_in = usb_endpoint_dir_in(desc);
	ep->ep.maxpacket = usb_endpoint_maxp(desc);

#ifdef CONFIG_USB_FOTG210_UDC_DOUBLE_FIFO
	//EP1 = FIFO 0~1, EP2 = FIFO 2~3
	switch (ep->epnum) {
	case 1:
		ep->fifonum = FIFO0;
		break;
	case 2:
		ep->fifonum = FIFO2;
		break;
	default:
		printk("*****************Enpoint No. is over 2 or 0*************\n");
		break;
	}

#else //Single FIFO: EP1 = FIFO 0, EP2 = FIFO 1, ...
	switch (ep->epnum) {
	case 1:
		ep->fifonum = FIFO0;
		break;
	case 2:
		ep->fifonum = FIFO1;
		break;
	case 3:
		ep->fifonum = FIFO2;
		break;
	case 4:
		ep->fifonum = FIFO3;
		break;
	}
#endif

	return fotg210_config_ep(ep);
}

static void fotg210_reset_tseq(struct fotg210_udc *fotg210, u8 epnum)
{
	struct fotg210_ep *ep = fotg210->ep[epnum];
	u32 value;
	void __iomem *reg;

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(epnum);

	/* Note: Driver needs to set and clear INOUTEPMPSR_RESET_TSEQ
	 *	 bit. Controller wouldn't clear this bit. WTF!!!
	 */

	value = ioread32(reg);
	value |= INOUTEPMPSR_RESET_TSEQ;
	iowrite32(value, reg);

	value = ioread32(reg);
	value &= ~INOUTEPMPSR_RESET_TSEQ;
	iowrite32(value, reg);
}

static int fotg210_ep_release(struct fotg210_ep *ep)
{
	if (!ep->epnum)
		return 0;
	ep->epnum = 0;
	ep->stall = 0;
	ep->wedged = 0;

	return 0;
}

static int fotg210_ep_disable(struct usb_ep *_ep)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	unsigned long flags;

	BUG_ON(!_ep);

	ep = container_of(_ep, struct fotg210_ep, ep);

	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next,
			struct fotg210_request, queue);
		spin_lock_irqsave(&ep->fotg210->lock, flags);
		fotg210_done(ep, req, -ECONNRESET);
		spin_unlock_irqrestore(&ep->fotg210->lock, flags);
	}

	ep->ep.desc = NULL;
	return fotg210_ep_release(ep);
}

static struct usb_request *fotg210_ep_alloc_request(struct usb_ep *_ep,
						gfp_t gfp_flags)
{
	struct fotg210_request *req;

	req = kzalloc(sizeof(struct fotg210_request), gfp_flags);
	if (!req)
		return NULL;

	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void fotg210_ep_free_request(struct usb_ep *_ep,
					struct usb_request *_req)
{
	struct fotg210_request *req;

	req = container_of(_req, struct fotg210_request, req);
	kfree(req);
}

#ifdef CONFIG_USB_FOTG210_UDC_VDMA
static void fotg210_enable_vdma(struct fotg210_ep *ep,
			      dma_addr_t d, u32 len)
{
	u32 value;
	struct fotg210_udc *fotg210 = ep->fotg210;

	if (ep->epnum) {
		/* set transfer length and direction */
		value = ioread32(fotg210->reg + FOTG210_VDMA_FNPS1(ep->fifonum));
		value &= ~(VDMAFPS1_VDMA_LEN(0x1FFFF) | VDMAFPS1_VDMA_TYPE(1));
		value |= VDMAFPS1_VDMA_LEN(len) | VDMAFPS1_VDMA_TYPE(ep->dir_in);
		iowrite32(value, fotg210->reg + FOTG210_VDMA_FNPS1(ep->fifonum));

		/* set VDMA memory address */
		iowrite32(d, fotg210->reg + FOTG210_VDMA_FNPS2(ep->fifonum));

		/* enable MVDMA_ERROR and MVDMA_CMPLT interrupt */
		value = ioread32(fotg210->reg + FOTG210_DMISGR3);
		value &= ~(DMISGR3_MVDMA_CMPLT_FN(ep->fifonum) |
				DMISGR3_MVDMA_ERROR_FN(ep->fifonum));
		iowrite32(value, fotg210->reg + FOTG210_DMISGR3);

		/* enable VDMA */
		iowrite32(VDMA_EN, fotg210->reg + FOTG210_VDMA_CTRL);

		/* start VDMA */
		value = ioread32(fotg210->reg + FOTG210_VDMA_FNPS1(ep->fifonum));
		value |= VDMAFPS1_VDMA_START;
		iowrite32(value, fotg210->reg + FOTG210_VDMA_FNPS1(ep->fifonum));
	}

	else {
		/* set transfer length and direction */
		value = ioread32(fotg210->reg + FOTG210_VDMA_CXFPS1);
		value &= ~(VDMAFPS1_VDMA_LEN(0x1FFFF) | VDMAFPS1_VDMA_TYPE(1));
		value |= VDMAFPS1_VDMA_LEN(len) | VDMAFPS1_VDMA_TYPE(ep->dir_in);
		iowrite32(value, fotg210->reg + FOTG210_VDMA_CXFPS1);

		/* set VDMA memory address */
		iowrite32(d, fotg210->reg + FOTG210_VDMA_CXFPS2);

		/* enable MVDMA_ERROR and MVDMA_CMPLT interrupt */
		value = ioread32(fotg210->reg + FOTG210_DMISGR3);
		value &= ~(DMISGR3_MVDMA_CMPLT_CXF | DMISGR3_MVDMA_ERROR_CXF);
		iowrite32(value, fotg210->reg + FOTG210_DMISGR3);

		/* enable VDMA */
		iowrite32(VDMA_EN, fotg210->reg + FOTG210_VDMA_CTRL);

		/* start VDMA */
		value = ioread32(fotg210->reg + FOTG210_VDMA_CXFPS1);
		value |= VDMAFPS1_VDMA_START;
		iowrite32(value, fotg210->reg + FOTG210_VDMA_CXFPS1);
	}
}

static void fotg210_disable_vdma(struct fotg210_ep *ep)
{
	iowrite32(0, ep->fotg210->reg + FOTG210_VDMA_CTRL);
}

static void fotg210_wait_vdma_done(struct fotg210_ep *ep)
{
	u32 value;

	if (ep->epnum) {
		do {
			value = ioread32(ep->fotg210->reg + FOTG210_DISGR3);
			if (value & DISGR3_VDMA_ERROR_FN(ep->fifonum))
				goto vdma_reset;
		} while (!(value & DISGR3_VDMA_CMPLT_FN(ep->fifonum)));

		//write '1' clear
		iowrite32(DISGR3_VDMA_CMPLT_FN(ep->fifonum),
			ep->fotg210->reg + FOTG210_DISGR3);
	}
	else {
		do {
			value = ioread32(ep->fotg210->reg + FOTG210_DISGR3);
			if (value & DISGR3_VDMA_ERROR_CXF)
				goto vdma_reset;
		} while (!(value & DISGR3_VDMA_CMPLT_CXF));

		//write '1' clear
		iowrite32(DISGR3_VDMA_CMPLT_CXF,
			ep->fotg210->reg + FOTG210_DISGR3);
	}
	return;

vdma_reset: /* reset fifo */

	if (ep->epnum) {
		//write '1' clear
		iowrite32(DISGR3_VDMA_ERROR_FN(ep->fifonum),
			ep->fotg210->reg + FOTG210_DISGR3);

		value = ioread32(ep->fotg210->reg +
				FOTG210_FIBCR(ep->fifonum));
		value |= FIBCR_FFRST;
		iowrite32(value, ep->fotg210->reg +
				FOTG210_FIBCR(ep->fifonum));
	} else {
		//write '1' clear
		iowrite32(DISGR3_VDMA_ERROR_CXF,
			ep->fotg210->reg + FOTG210_DISGR3);

		value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
		value |= DCFESR_CX_CLR;
		iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);
	}
}

static void fotg210_start_vdma_wfi(struct fotg210_ep *ep,
			struct fotg210_request *req, u32 length)
{
	dma_addr_t d;
	u8 *buffer;

	buffer = req->req.buf + req->req.actual;

	d = dma_map_single(ep->fotg210->gadget.dev.parent,
			buffer, length,
			ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	if (dma_mapping_error(ep->fotg210->gadget.dev.parent, d)) {
		pr_err("dma_mapping_error\n");
		return;
	}

	ep->irq_current_task->req = req;
	ep->irq_current_task->transfer_length = length;
	ep->irq_current_task->dma_addr = d;

	fotg210_enable_vdma(ep, d, length);
}

#else  //#ifdef CONFIG_USB_FOTG210_UDC_VDMA

static void fotg210_enable_dma(struct fotg210_ep *ep,
			      dma_addr_t d, u32 len)
{
	u32 value;
	struct fotg210_udc *fotg210 = ep->fotg210;

	/* set transfer length and direction */
	value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
	value &= ~(DMACPSR1_DMA_LEN(0x1FFFF) | DMACPSR1_DMA_TYPE(1));
	value |= DMACPSR1_DMA_LEN(len) | DMACPSR1_DMA_TYPE(ep->dir_in);
	iowrite32(value, fotg210->reg + FOTG210_DMACPSR1);

	/* set device DMA target FIFO number */
	value = ioread32(fotg210->reg + FOTG210_DMATFNR);
	if (ep->epnum)
		value |= DMATFNR_ACC_FN(ep->fifonum);
	else
		value |= DMATFNR_ACC_CXF;
	iowrite32(value, fotg210->reg + FOTG210_DMATFNR);

	/* set DMA memory address */
	iowrite32(d, fotg210->reg + FOTG210_DMACPSR2);

	/* enable MDMA_ERROR and MDMA_CMPLT interrupt */
	value = ioread32(fotg210->reg + FOTG210_DMISGR2);
	value &= ~(DMISGR2_MDMA_CMPLT | DMISGR2_MDMA_ERROR);
	iowrite32(value, fotg210->reg + FOTG210_DMISGR2);

	/* start DMA */
	value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
	value |= DMACPSR1_DMA_START;
	iowrite32(value, fotg210->reg + FOTG210_DMACPSR1);
}

static void fotg210_disable_dma(struct fotg210_ep *ep)
{
	iowrite32(DMATFNR_DISDMA, ep->fotg210->reg + FOTG210_DMATFNR);
}

static void fotg210_wait_dma_done(struct fotg210_ep *ep)
{
	u32 value;

	do {
		value = ioread32(ep->fotg210->reg + FOTG210_DISGR2);
		if ((value & DISGR2_USBRST_INT) ||
		    (value & DISGR2_DMA_ERROR))
			goto dma_reset;
	} while (!(value & DISGR2_DMA_CMPLT));

#ifndef CONFIG_USB_FOTG210_UDC_V1_11_0
	value &= ~DISGR2_DMA_CMPLT;
	iowrite32(value, ep->fotg210->reg + FOTG210_DISGR2);
#else	//write '1' clear
	iowrite32(DISGR2_DMA_CMPLT, ep->fotg210->reg + FOTG210_DISGR2);
#endif
	return;

dma_reset:
	value = ioread32(ep->fotg210->reg + FOTG210_DMACPSR1);
	value |= DMACPSR1_DMA_ABORT;
	iowrite32(value, ep->fotg210->reg + FOTG210_DMACPSR1);

	/* reset fifo */
	if (ep->epnum) {
		value = ioread32(ep->fotg210->reg +
				FOTG210_FIBCR(ep->fifonum));
		value |= FIBCR_FFRST;
		iowrite32(value, ep->fotg210->reg +
				FOTG210_FIBCR(ep->fifonum));
	} else {
		value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
		value |= DCFESR_CX_CLR;
		iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);
	}
}
#endif //not #ifdef CONFIG_USB_FOTG210_UDC_VDMA

static void fotg210_start_dma(struct fotg210_ep *ep,
			struct fotg210_request *req, u32 length)
{
	dma_addr_t d;
	u8 *buffer;

	buffer = req->req.buf + req->req.actual;

	d = dma_map_single(ep->fotg210->gadget.dev.parent,
			buffer, length,
			ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	if (dma_mapping_error(ep->fotg210->gadget.dev.parent, d)) {
		pr_err("dma_mapping_error\n");
		return;
	}

#ifdef CONFIG_USB_FOTG210_UDC_VDMA
	fotg210_enable_vdma(ep, d, length);

	/* check if vdma is complete */
	fotg210_wait_vdma_done(ep);

	if (vdma_overlap == 0)
		fotg210_disable_vdma(ep);
#else
	fotg210_enable_dma(ep, d, length);

	/* check if dma is done */
	fotg210_wait_dma_done(ep);

	fotg210_disable_dma(ep);
#endif
	/* update actual transfer length */
	req->req.actual += length;

	dma_unmap_single(ep->fotg210->gadget.dev.parent,
			 d, length, ep->dir_in ? DMA_TO_DEVICE :
					DMA_FROM_DEVICE);
}

static void fotg210_ep0_queue(struct fotg210_ep *ep,
				struct fotg210_request *req)
{
	if (ep->dir_in) { /* if IN */
		if (req->req.length) {
			if (req->req.length > ep->ep.maxpacket)
				fotg210_start_dma(ep, req, ep->ep.maxpacket);
			else
				fotg210_start_dma(ep, req, req->req.length);
		} else {
			pr_err("%s : req->req.length = 0x%x\n",
			       __func__, req->req.length);
		}
		if ((req->req.length == req->req.actual) ||
		    (req->req.actual < ep->ep.maxpacket))
			fotg210_done(ep, req, 0);
	} else { /* OUT */
		if (!req->req.length) {
			fotg210_done(ep, req, 0);
		} else {
			/* For set_feature(TEST_PACKET) */
			if (req->req.length == 53) {
				ep->dir_in = 1;
				fotg210_start_dma(ep, req, req->req.length);
			}
			else {
			u32 value = ioread32(ep->fotg210->reg +
						FOTG210_DMISGR0);

			value &= ~DMISGR0_MCX_OUT_INT;
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
			}
		}
	}
}

static int fotg210_ep_queue(struct usb_ep *_ep, struct usb_request *_req,
				gfp_t gfp_flags)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	unsigned long flags;
	int request = 0;

	ep = container_of(_ep, struct fotg210_ep, ep);
	req = container_of(_req, struct fotg210_request, req);

	if (ep->fotg210->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	spin_lock_irqsave(&ep->fotg210->lock, flags);

	if (list_empty(&ep->queue))
		request = 1;

	list_add_tail(&req->queue, &ep->queue);

	req->req.actual = 0;
	req->req.status = -EINPROGRESS;

	if (!ep->epnum) /* ep0 */
		fotg210_ep0_queue(ep, req);
	else if (request && !ep->stall)
		fotg210_enable_fifo_int(ep);

	spin_unlock_irqrestore(&ep->fotg210->lock, flags);

	return 0;
}

static int fotg210_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	unsigned long flags;

	ep = container_of(_ep, struct fotg210_ep, ep);
	req = container_of(_req, struct fotg210_request, req);

	spin_lock_irqsave(&ep->fotg210->lock, flags);
	if (!list_empty(&ep->queue))
		fotg210_done(ep, req, -ECONNRESET);
	spin_unlock_irqrestore(&ep->fotg210->lock, flags);

	return 0;
}

static void fotg210_set_epnstall(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 value;
	void __iomem *reg;

	/* check if IN FIFO is empty before stall */
	if (ep->dir_in) {
		do {
			value = ioread32(fotg210->reg + FOTG210_DCFESR);
		} while (!(value & DCFESR_FIFO_EMPTY(ep->fifonum)));
	}

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(ep->epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(ep->epnum);
	value = ioread32(reg);
	value |= INOUTEPMPSR_STL_EP;
	iowrite32(value, reg);
}

static void fotg210_clear_epnstall(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 value;
	void __iomem *reg;

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(ep->epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(ep->epnum);
	value = ioread32(reg);
	value &= ~INOUTEPMPSR_STL_EP;
	iowrite32(value, reg);
}

static int fotg210_set_halt_and_wedge(struct usb_ep *_ep, int value, int wedge)
{
	struct fotg210_ep *ep;
	struct fotg210_udc *fotg210;
	unsigned long flags;
	int ret = 0;

	ep = container_of(_ep, struct fotg210_ep, ep);

	fotg210 = ep->fotg210;

	spin_lock_irqsave(&ep->fotg210->lock, flags);

	if (!list_empty(&ep->queue)) {
		ret = -EAGAIN;
		goto out;
	}

	if (value) {
		fotg210_set_epnstall(ep);
		ep->stall = 1;
		if (wedge)
			ep->wedged = 1;
	} else {
		fotg210_clear_epnstall(ep);
		ep->stall = 0;
		ep->wedged = 0;
	}

out:
	spin_unlock_irqrestore(&ep->fotg210->lock, flags);
	return ret;
}

static int fotg210_ep_set_halt(struct usb_ep *_ep, int value)
{
	return fotg210_set_halt_and_wedge(_ep, value, 0);
}

static int fotg210_ep_set_wedge(struct usb_ep *_ep)
{
	return fotg210_set_halt_and_wedge(_ep, 1, 1);
}

static void fotg210_ep_fifo_flush(struct usb_ep *_ep)
{
}

static const struct usb_ep_ops fotg210_ep_ops = {
	.enable		= fotg210_ep_enable,
	.disable	= fotg210_ep_disable,

	.alloc_request	= fotg210_ep_alloc_request,
	.free_request	= fotg210_ep_free_request,

	.queue		= fotg210_ep_queue,
	.dequeue	= fotg210_ep_dequeue,

	.set_halt	= fotg210_ep_set_halt,
	.fifo_flush	= fotg210_ep_fifo_flush,
	.set_wedge	= fotg210_ep_set_wedge,
};

static void fotg210_clear_tx0byte(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_TX0BYTE);

	//Note: From HW version 1.26, this register change to W1C
#ifndef CONFIG_USB_FOTG210_UDC_V1_26_0
	value &= ~(TX0BYTE_EP1 | TX0BYTE_EP2 | TX0BYTE_EP3
		   | TX0BYTE_EP4);
#endif
	iowrite32(value, fotg210->reg + FOTG210_TX0BYTE);
}

static void fotg210_clear_rx0byte(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_RX0BYTE);

	//Note: From HW version 1.26, this register change to W1C
#ifndef CONFIG_USB_FOTG210_UDC_V1_26_0
	value &= ~(RX0BYTE_EP1 | RX0BYTE_EP2 | RX0BYTE_EP3
		   | RX0BYTE_EP4);
#endif
	iowrite32(value, fotg210->reg + FOTG210_RX0BYTE);
}

/* read 8-byte setup packet only */
static void fotg210_rdsetupp(struct fotg210_udc *fotg210,
		   u8 *buffer)
{
	int i = 0;
	u8 *tmp = buffer;
	u32 data;
	u32 length = 8;

	iowrite32(DMATFNR_ACC_CXF, fotg210->reg + FOTG210_DMATFNR);

	for (i = (length >> 2); i > 0; i--) {
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		pr_info("    0x%x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		*(tmp + 3) = (data >> 24) & 0xFF;
		tmp = tmp + 4;
	}

	switch (length % 4) {
	case 1:
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		pr_info("    0x%x\n", data);
		*tmp = data & 0xFF;
		break;
	case 2:
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		pr_info("    0x%x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		break;
	case 3:
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		pr_info("    0x%x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		break;
	default:
		break;
	}

	iowrite32(DMATFNR_DISDMA, fotg210->reg + FOTG210_DMATFNR);
}

static int fotg210_is_rmwkup(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DMCR);

	return value & DMCR_CAP_RMWAKUP ? 1 : 0;
}

static void fotg210_set_rmwkup(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DMCR);

	value |= DMCR_CAP_RMWAKUP;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);
}

static void fotg210_clear_rmwkup(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DMCR);

	value &= ~DMCR_CAP_RMWAKUP;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);
}

static void fotg210_after_configuration(struct fotg210_udc *fotg210, u8 setted)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DAR);

	if (setted)
		value |= DAR_AFT_CONF;
	else
		value &= ~DAR_AFT_CONF;
	iowrite32(value, fotg210->reg + FOTG210_DAR);
}

static void fotg210_set_dev_addr(struct fotg210_udc *fotg210, u32 addr)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DAR);

	value &= ~0x7F;
	value |= (addr & 0x7F);
	iowrite32(value, fotg210->reg + FOTG210_DAR);
}

static void fotg210_set_cxstall(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DCFESR);

	value |= DCFESR_CX_STL;
	iowrite32(value, fotg210->reg + FOTG210_DCFESR);
}

static void fotg210_request_error(struct fotg210_udc *fotg210)
{
	fotg210_set_cxstall(fotg210);
	pr_err("request error!!\n");
}

static void fotg210_set_configuration(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u8 i;

	if (!w_value)
		fotg210_after_configuration(fotg210, 0);
	else {
		if (w_value == 1) {
			fotg210_after_configuration(fotg210, 1);
			for (i = 1; i <= FOTG210_MAX_NUM_EP; i++)
				fotg210_reset_tseq(fotg210, i);
		}
	}
}

static void fotg210_set_address(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);

	if (w_value >= 0x0100) {
		fotg210_request_error(fotg210);
	} else {
		fotg210_set_dev_addr(fotg210, w_value);
		fotg210_set_cxdone(fotg210);
	}
}

void gen_test_packet(u8 *tst_packet)
{
	u8 *tp = tst_packet;

	int i;
	for (i = 0; i < 9; i++)/*JKJKJKJK x 9*/
		*tp++ = 0x00;

	for (i = 0; i < 8; i++) /* 8*AA */
		*tp++ = 0xAA;

	for (i = 0; i < 8; i++) /* 8*EE */
		*tp++ = 0xEE;

	*tp++ = 0xFE;

	for (i = 0; i < 11; i++) /* 11*FF */
		*tp++ = 0xFF;

	*tp++ = 0x7F;
	*tp++ = 0xBF;
	*tp++ = 0xDF;
	*tp++ = 0xEF;
	*tp++ = 0xF7;
	*tp++ = 0xFB;
	*tp++ = 0xFD;
	*tp++ = 0xFC;
	*tp++ = 0x7E;
	*tp++ = 0xBF;
	*tp++ = 0xDF;
	*tp++ = 0xEF;
	*tp++ = 0xF7;
	*tp++ = 0xFB;
	*tp++ = 0xFD;
	*tp++ = 0x7E;
}

static void fotg210_set_feature(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u32 val;
	u8 tst_packet[53];

	struct fotg210_ep *ep =
		fotg210->ep[w_index & USB_ENDPOINT_NUMBER_MASK];

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		switch(w_value) {
		case USB_DEVICE_REMOTE_WAKEUP:
			fotg210_set_rmwkup(fotg210);
			fotg210_set_cxdone(fotg210);
			break;

		case USB_DEVICE_TEST_MODE:
			switch (w_index >> 8) {
			case TEST_J:
				iowrite32(PHYTMSR_TST_JSTA,
					fotg210->reg + FOTG210_PHYTMSR);
				fotg210_set_cxdone(fotg210);
				break;
			case TEST_K:
				iowrite32(PHYTMSR_TST_KSTA,
					fotg210->reg + FOTG210_PHYTMSR);
				fotg210_set_cxdone(fotg210);
				break;
			case TEST_SE0_NAK:
				iowrite32(PHYTMSR_TST_SE0NAK,
					fotg210->reg + FOTG210_PHYTMSR);
				fotg210_set_cxdone(fotg210);
				break;
			case TEST_PACKET:
				iowrite32(PHYTMSR_TST_PKT,
					fotg210->reg + FOTG210_PHYTMSR);
				fotg210_set_cxdone(fotg210);

				gen_test_packet(tst_packet);

				fotg210->ep0_req->buf = tst_packet;
				fotg210->ep0_req->length = 53;

				spin_unlock(&fotg210->lock);
				fotg210_ep_queue(fotg210->gadget.ep0,
						fotg210->ep0_req, GFP_KERNEL);
				spin_lock(&fotg210->lock);

				//Test Packet Done set
				val = ioread32(fotg210->reg + FOTG210_DCFESR);
				val |= DCFESR_TST_PKDONE;
				iowrite32(val, fotg210->reg + FOTG210_DCFESR);
				break;
			case TEST_FORCE_EN:
				fotg210_set_cxdone(fotg210);
				break;
			default:
				fotg210_request_error(fotg210);
				break;
			}
			break;

		default:
			fotg210_set_cxdone(fotg210);
			break;
		}
		break;

	case USB_RECIP_INTERFACE:
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_ENDPOINT:
		if (w_value == USB_ENDPOINT_HALT) {
			if (w_index & USB_ENDPOINT_NUMBER_MASK) {
				fotg210_set_epnstall(ep);
				ep->stall = 1;
			}
			else
				fotg210_set_cxstall(fotg210);
			fotg210_set_cxdone(fotg210);
		}
		else
			fotg210_request_error(fotg210);
		break;
	default:
		fotg210_request_error(fotg210);
		break;
	}
}

static void fotg210_clear_feature(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);

	struct fotg210_ep *ep =
		fotg210->ep[w_index & USB_ENDPOINT_NUMBER_MASK];

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		if (w_value == USB_DEVICE_REMOTE_WAKEUP)
			fotg210_clear_rmwkup(fotg210);
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_INTERFACE:
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_ENDPOINT:
		if (w_value == USB_ENDPOINT_HALT) {
			if (w_index & USB_ENDPOINT_NUMBER_MASK) {
				fotg210_reset_tseq(fotg210, ep->epnum);
				if (ep->wedged) {
					fotg210_set_cxdone(fotg210);
					return;
				}
				if (ep->stall) {
					ep->stall = 0;
					fotg210_clear_epnstall(ep);
				}
				if (!list_empty(&ep->queue))
					fotg210_enable_fifo_int(ep);
			}
			fotg210_set_cxdone(fotg210);
		}
		else
			fotg210_request_error(fotg210);
		break;
	default:
		fotg210_request_error(fotg210);
		break;
	}
}

static int fotg210_is_epnstall(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 value;
	void __iomem *reg;

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(ep->epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(ep->epnum);
	value = ioread32(reg);
	return value & INOUTEPMPSR_STL_EP ? 1 : 0;
}

static void fotg210_get_status(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u8 epnum;
	u16 w_index = le16_to_cpu(ctrl->wIndex);

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		fotg210->ep0_data = 1 << USB_DEVICE_SELF_POWERED;
		if (fotg210_is_rmwkup(fotg210))
			fotg210->ep0_data |= 1 << USB_DEVICE_REMOTE_WAKEUP;
		break;
	case USB_RECIP_INTERFACE:
		fotg210->ep0_data = 0;
		break;
	case USB_RECIP_ENDPOINT:
		epnum = w_index & USB_ENDPOINT_NUMBER_MASK;
		if (epnum)
			fotg210->ep0_data =
				fotg210_is_epnstall(fotg210->ep[epnum])
				<< USB_ENDPOINT_HALT;
		else
			fotg210_request_error(fotg210);
		break;

	default:
		fotg210_request_error(fotg210);
		return;		/* exit */
	}

	fotg210->ep0_req->buf = &fotg210->ep0_data;
	fotg210->ep0_req->length = 2;

	spin_unlock(&fotg210->lock);
	fotg210_ep_queue(fotg210->gadget.ep0, fotg210->ep0_req, GFP_KERNEL);
	spin_lock(&fotg210->lock);
}

static int fotg210_setup_packet(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u8 *p = (u8 *)ctrl;
	u8 ret = 0;

	fotg210_rdsetupp(fotg210, p);

	fotg210->ep[0]->dir_in = ctrl->bRequestType & USB_DIR_IN;

	if (fotg210->gadget.speed == USB_SPEED_UNKNOWN) {
		u32 value = ioread32(fotg210->reg + FOTG210_DMCR);
		fotg210->gadget.speed = value & DMCR_HS_EN ?
				USB_SPEED_HIGH : USB_SPEED_FULL;
	}

	/* check request */
	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		switch (ctrl->bRequest) {
		case USB_REQ_GET_STATUS:
			fotg210_get_status(fotg210, ctrl);
			break;
		case USB_REQ_CLEAR_FEATURE:
			fotg210_clear_feature(fotg210, ctrl);
			break;
		case USB_REQ_SET_FEATURE:
			fotg210_set_feature(fotg210, ctrl);
			break;
		case USB_REQ_SET_ADDRESS:
			fotg210_set_address(fotg210, ctrl);
			break;
		case USB_REQ_SET_CONFIGURATION:
			fotg210_set_configuration(fotg210, ctrl);
			ret = 1;
			break;
		default:
			ret = 1;
			break;
		}
	} else {
		ret = 1;
	}

	return ret;
}

static void fotg210_ep0out(struct fotg210_udc *fotg210)
{
	struct fotg210_ep *ep = fotg210->ep[0];
	u32 value;

	if (!list_empty(&ep->queue) && !ep->dir_in) {
		struct fotg210_request *req;

		req = list_first_entry(&ep->queue,
			struct fotg210_request, queue);

		if (req->req.length)
			fotg210_start_dma(ep, req, req->req.length);

		if (req->req.length == req->req.actual) {
			fotg210_done(ep, req, 0);
			//mask CX_OUT_INT until ep0_queue()
			value = ioread32(ep->fotg210->reg + FOTG210_DMISGR0);
			value |= DMISGR0_MCX_OUT_INT;
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
		}
	} else {
		pr_err("%s : empty queue\n", __func__);
	}
}

static void fotg210_ep0in(struct fotg210_udc *fotg210)
{
	struct fotg210_ep *ep = fotg210->ep[0];
	u32 length = 0;

	if ((!list_empty(&ep->queue)) && (ep->dir_in)) {
		struct fotg210_request *req;

		req = list_entry(ep->queue.next,
				struct fotg210_request, queue);

		length = req->req.length - req->req.actual;
		if (length) {
			if (length > ep->ep.maxpacket)
				fotg210_start_dma(ep, req, ep->ep.maxpacket);
			else
				fotg210_start_dma(ep, req, length);
		}

		if (req->req.length == req->req.actual)
			fotg210_done(ep, req, 0);
	} else {
		fotg210_set_cxdone(fotg210);
	}
}

static void fotg210_clear_comabt_int(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DISGR0);

	value |= DISGR0_CX_COMABT_INT;
	iowrite32(value, fotg210->reg + FOTG210_DISGR0);
}

#ifdef CONFIG_USB_FOTG210_UDC_VDMA
/* wait for interrupt (wfi) method, not polling method
 * fotg210_irq_dma_done() will be called when dma complete
 */
static void fotg210_in_fifo_handler_wfi(struct fotg210_udc *fotg210, int fifo_int)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	int i;
	u32 value;

	for (i = 1; i < FOTG210_MAX_NUM_EP; i++) {
		ep = fotg210->ep[i];
		if (ep->fifonum == fifo_int)
			break;
	}
	req = list_entry(ep->queue.next, struct fotg210_request, queue);

	if (req->req.length) {
		fotg210_start_vdma_wfi(ep, req, req->req.length);

		fotg210_disable_fifo_int(ep);
		vdma_overlap ++;

		//Temporarily block EP0 setup interrupt,
		//CPU is not allowed to obtain 8-byte setup command from data port when VDMA_EN is set
		value = ioread32(ep->fotg210->reg + FOTG210_DMISGR0);
		value |= DMISGR0_MCX_SETUP_INT;
		iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
	}
	else
		fotg210_done(ep, req, 0);
}

static void fotg210_out_fifo_handler_wfi(struct fotg210_udc *fotg210, int fifo_int)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	int i;
	u32 value;

	for (i = 1; i < FOTG210_MAX_NUM_EP; i++) {
		ep = fotg210->ep[i];
		if (ep->fifonum == fifo_int)
			break;
	}
	req = list_entry(ep->queue.next, struct fotg210_request, queue);

	fotg210_start_vdma_wfi(ep, req, req->req.length);

	fotg210_disable_fifo_int(ep);
	vdma_overlap ++;

	//Temporarily block EP0 setup interrupt,
	//CPU is not allowed to obtain 8-byte setup command from data port when VDMA_EN is set
	value = ioread32(ep->fotg210->reg + FOTG210_DMISGR0);
	value |= DMISGR0_MCX_SETUP_INT;
	iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
}

static void fotg210_irq_vdma_done(struct fotg210_udc *fotg210, int fifo_cmplt,
			int force_out_done)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	int i;
	u32 value, rem_len;

	if (fifo_cmplt == CXFIFO)
		ep = fotg210->ep[0];
	else {
		for (i = 1; i < FOTG210_MAX_NUM_EP; i++) {
			ep = fotg210->ep[i];
			if (ep->fifonum == fifo_cmplt)
				break;
		}
	}
	req = ep->irq_current_task->req;

	/* update actual transfer length */
	value = ioread32(fotg210->reg + FOTG210_VDMA_FNPS1(ep->fifonum));
	rem_len = GET_VDMAFPS1_VDMA_LEN(value);
	req->req.actual += (ep->irq_current_task->transfer_length - rem_len);

	vdma_overlap --;
	if (vdma_overlap == 0) {
		fotg210_disable_vdma(ep);

		//Restore EP0 setup interrupt,
		//CPU is not allowed to obtain 8-byte setup command from data port when VDMA_EN is set
		value = ioread32(fotg210->reg + FOTG210_DMISGR0);
		value &= ~DMISGR0_MCX_SETUP_INT;
		iowrite32(value, fotg210->reg + FOTG210_DMISGR0);
	}

	dma_unmap_single(ep->fotg210->gadget.dev.parent,
			 ep->irq_current_task->dma_addr,
			 ep->irq_current_task->transfer_length,
			 ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	if (ep->dir_in) {
		fotg210_done(ep, req, 0);
	}
	else {
		/* finish out transfer */
		if (force_out_done)
			fotg210_done(ep, req, 0);
		else {
			if ((req->req.length == req->req.actual) ||
			    ((req->req.actual % ep->ep.maxpacket) != 0))
				fotg210_done(ep, req, 0);
		}
	}
}

#ifdef RELEASE_PACKET_SIZE_OUT_TRANSFER
static int fotg210_out_thread(void *_fotg210)
{
	struct fotg210_udc *fotg210 = _fotg210;
	unsigned long flags;
	u32 reg;
	int rem_len, fifo;
	static int old_rem_len = -1;

	while(!kthread_should_stop()) 	{
		spin_lock_irqsave(&fotg210->lock, flags);

		//Only OUT transfer
		// TODO: modify the correct FIFO number for out ep
#ifdef CONFIG_USB_FOTG210_UDC_DOUBLE_FIFO
		fifo = FIFO2;
#else
		fifo = FIFO1;
#endif
		reg = ioread32(fotg210->reg + FOTG210_VDMA_FNPS1(fifo));
		rem_len = GET_VDMAFPS1_VDMA_LEN(reg);

		if ((vdma_overlap == 1) && (reg & VDMAFPS1_VDMA_START)) {
			if (old_rem_len == rem_len) {
				fotg210_irq_vdma_done(fotg210, fifo, 1);
				old_rem_len = -1;
			}
			else
				old_rem_len = rem_len;
		}
		else
			old_rem_len = -1;

		spin_unlock_irqrestore(&fotg210->lock, flags);

		msleep(1000); //Change it by case
	}

	spin_lock_irq(&fotg210->lock);
	fotg210->out_thread_task = NULL;
	spin_unlock_irq(&fotg210->lock);

	return 0;
}
#endif

#else	//not #ifdef CONFIG_USB_FOTG210_UDC_VDMA
static void fotg210_in_fifo_handler(struct fotg210_udc *fotg210, int fifo_int)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	int i;

	for (i = 1; i < FOTG210_MAX_NUM_EP; i++) {
		ep = fotg210->ep[i];
		if (ep->fifonum == fifo_int)
			break;
	}
	req = list_entry(ep->queue.next, struct fotg210_request, queue);

	if (req->req.length)
		fotg210_start_dma(ep, req, req->req.length);
	fotg210_done(ep, req, 0);
}

static void fotg210_out_fifo_handler(struct fotg210_udc *fotg210, int fifo_int)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	int i;
	u32 reg = ioread32(fotg210->reg + FOTG210_FIBCR(fifo_int));
	u32 length = reg & FIBCR_BCFX;

	for (i = 1; i < FOTG210_MAX_NUM_EP; i++) {
		ep = fotg210->ep[i];
		if (ep->fifonum == fifo_int)
			break;
	}
	req = list_entry(ep->queue.next, struct fotg210_request, queue);

	fotg210_start_dma(ep, req, length);
	/* finish out transfer */
	if (req->req.length == req->req.actual ||
	    length < ep->ep.maxpacket)
		fotg210_done(ep, req, 0);
}

#endif

static irqreturn_t fotg210_irq(int irq, void *_fotg210)
{
	struct fotg210_udc *fotg210 = _fotg210;
	u32 int_grp = ioread32(fotg210->reg + FOTG210_DIGR);
	u32 int_msk = ioread32(fotg210->reg + FOTG210_DMIGR);

	int_grp &= ~int_msk;

	spin_lock(&fotg210->lock);

	if (int_grp & DIGR_INT_G2) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR2;
		u32 int_grp2 = ioread32(reg);
		u32 int_msk2 = ioread32(fotg210->reg + FOTG210_DMISGR2);
#ifndef CONFIG_USB_FOTG210_UDC_V1_11_0
		u32 value;
#endif
		int_grp2 &= ~int_msk2;

#ifndef CONFIG_USB_FOTG210_UDC_V1_11_0
		if (int_grp2 & DISGR2_USBRST_INT) {
			value = ioread32(reg);
			value &= ~DISGR2_USBRST_INT;
			iowrite32(value, reg);
			fotg210->gadget.speed = USB_SPEED_UNKNOWN;
			pr_info("fotg210 udc reset\n");
		}
		if (int_grp2 & DISGR2_SUSP_INT) {
			value = ioread32(reg);
			value &= ~DISGR2_SUSP_INT;
			iowrite32(value, reg);
			pr_info("fotg210 udc suspend\n");
		}
		if (int_grp2 & DISGR2_RESM_INT) {
			value = ioread32(reg);
			value &= ~DISGR2_RESM_INT;
			iowrite32(value, reg);
			pr_info("fotg210 udc resume\n");
		}
		if (int_grp2 & DISGR2_ISO_SEQ_ERR_INT) {
			value = ioread32(reg);
			value &= ~DISGR2_ISO_SEQ_ERR_INT;
			iowrite32(value, reg);
			pr_info("fotg210 iso sequence error\n");
		}
		if (int_grp2 & DISGR2_ISO_SEQ_ABORT_INT) {
			value = ioread32(reg);
			value &= ~DISGR2_ISO_SEQ_ABORT_INT;
			iowrite32(value, reg);
			pr_info("fotg210 iso sequence abort\n");
		}
		if (int_grp2 & DISGR2_TX0BYTE_INT) {
			fotg210_clear_tx0byte(fotg210);
			value = ioread32(reg);
			value &= ~DISGR2_TX0BYTE_INT;
			iowrite32(value, reg);
			pr_info("fotg210 transferred 0 byte\n");
		}
		if (int_grp2 & DISGR2_RX0BYTE_INT) {
			fotg210_clear_rx0byte(fotg210);
			value = ioread32(reg);
			value &= ~DISGR2_RX0BYTE_INT;
			iowrite32(value, reg);
			pr_info("fotg210 received 0 byte\n");
		}
		if (int_grp2 & DISGR2_DMA_ERROR) {
			value = ioread32(reg);
			value &= ~DISGR2_DMA_ERROR;
			iowrite32(value, reg);
			pr_info("fotg210 DMA error\n");
		}
#else	//write '1' clear
		if (int_grp2 & DISGR2_USBRST_INT) {
			iowrite32(DISGR2_USBRST_INT, reg);
			fotg210->gadget.speed = USB_SPEED_UNKNOWN; //lichun@add
			pr_info("fotg210 udc reset\n");
		}
		if (int_grp2 & DISGR2_SUSP_INT) {
			iowrite32(DISGR2_SUSP_INT, reg);
			pr_info("fotg210 udc suspend\n");
		}
		if (int_grp2 & DISGR2_RESM_INT) {
			iowrite32(DISGR2_RESM_INT, reg);
			pr_info("fotg210 udc resume\n");
		}
		if (int_grp2 & DISGR2_ISO_SEQ_ERR_INT) {
			iowrite32(DISGR2_ISO_SEQ_ERR_INT, reg);
			pr_info("fotg210 iso sequence error\n");
		}
		if (int_grp2 & DISGR2_ISO_SEQ_ABORT_INT) {
			iowrite32(DISGR2_ISO_SEQ_ABORT_INT, reg);
			pr_info("fotg210 iso sequence abort\n");
		}
		if (int_grp2 & DISGR2_TX0BYTE_INT) {
			fotg210_clear_tx0byte(fotg210);
			iowrite32(DISGR2_TX0BYTE_INT, reg);
			pr_info("fotg210 transferred 0 byte\n");
		}
		if (int_grp2 & DISGR2_RX0BYTE_INT) {
			fotg210_clear_rx0byte(fotg210);
			iowrite32(DISGR2_RX0BYTE_INT, reg);
			pr_info("fotg210 received 0 byte\n");
		}
		if (int_grp2 & DISGR2_DMA_ERROR) {
			iowrite32(DISGR2_DMA_ERROR, reg);
			pr_info("fotg210 DMA error\n");
		}
#endif
	}

#ifdef CONFIG_USB_FOTG210_UDC_VDMA
	if (int_grp & DIGR_INT_G3) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR3;
		u32 int_grp3 = ioread32(reg);
		u32 int_msk3 = ioread32(fotg210->reg + FOTG210_DMISGR3);
		int fifo;

		int_grp3 &= ~int_msk3;

		if (int_grp3 & DISGR3_VDMA_CMPLT_CXF) {
			iowrite32(DISGR3_VDMA_CMPLT_CXF, reg);
			//pr_info("fotg210 CXF VDMA complete\n");
		}
		if (int_grp3 & DISGR3_VDMA_ERROR_CXF) {
			iowrite32(DISGR3_VDMA_ERROR_CXF, reg);
			pr_info("fotg210 CXF VDMA error\n");
		}

		for (fifo = 0; fifo < FOTG210_MAX_FIFO_NUM; fifo++) {
			if (int_grp3 & DISGR3_VDMA_CMPLT_FN(fifo)) {
				iowrite32(DISGR3_VDMA_CMPLT_FN(fifo), reg);
				fotg210_irq_vdma_done(fotg210, fifo, 0);
			}

			if (int_grp3 & DISGR3_VDMA_ERROR_FN(fifo)) {
				iowrite32(DISGR3_VDMA_ERROR_FN(fifo), reg);
				printk("fotg210 F%d VDMA error\n", fifo);
				fotg210_irq_vdma_done(fotg210, fifo, 0);
			}
		}
	}
#endif

	if (int_grp & DIGR_INT_G0) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR0;
		u32 int_grp0 = ioread32(reg);
		u32 int_msk0 = ioread32(fotg210->reg + FOTG210_DMISGR0);
		struct usb_ctrlrequest ctrl;

		int_grp0 &= ~int_msk0;

		/* the highest priority in this source register */
		if (int_grp0 & DISGR0_CX_COMABT_INT) {
			fotg210_clear_comabt_int(fotg210);
			pr_info("fotg210 CX command abort\n");
		}

		if (int_grp0 & DISGR0_CX_SETUP_INT) {
			pr_info("fotg210 ep0 setup\n");
			if (fotg210_setup_packet(fotg210, &ctrl)) {
				spin_unlock(&fotg210->lock);
				if (fotg210->driver) {
					if (fotg210->driver->setup(
					    &fotg210->gadget,
					    &ctrl) < 0)
						fotg210_set_cxstall(fotg210);
				}
				spin_lock(&fotg210->lock);
			}
		}
		if (int_grp0 & DISGR0_CX_COMEND_INT)
			pr_info("fotg210 cmd end\n");

		if (int_grp0 & DISGR0_CX_IN_INT)
			fotg210_ep0in(fotg210);

		if (int_grp0 & DISGR0_CX_OUT_INT)
			fotg210_ep0out(fotg210);

		if (int_grp0 & DISGR0_CX_COMFAIL_INT) {
			fotg210_set_cxstall(fotg210);
			pr_info("fotg210 ep0 fail\n");
		}
	}

	if (int_grp & DIGR_INT_G1) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR1;
		u32 int_grp1 = ioread32(reg);
		u32 int_msk1 = ioread32(fotg210->reg + FOTG210_DMISGR1);
		int fifo;

		int_grp1 &= ~int_msk1;

		for (fifo = 0; fifo < FOTG210_MAX_FIFO_NUM; fifo++) {
			if (int_grp1 & DISGR1_IN_INT(fifo))
				#ifdef CONFIG_USB_FOTG210_UDC_VDMA
				fotg210_in_fifo_handler_wfi(fotg210, fifo);
				#else
				fotg210_in_fifo_handler(fotg210, fifo);
				#endif

			if ((int_grp1 & DISGR1_OUT_INT(fifo)) ||
			    (int_grp1 & DISGR1_SPK_INT(fifo)))
				#ifdef CONFIG_USB_FOTG210_UDC_VDMA
				fotg210_out_fifo_handler_wfi(fotg210, fifo);
				#else
				fotg210_out_fifo_handler(fotg210, fifo);
				#endif
		}
	}

	spin_unlock(&fotg210->lock);

	return IRQ_HANDLED;
}

static void fotg210_disable_unplug(struct fotg210_udc *fotg210)
{
	u32 reg = ioread32(fotg210->reg + FOTG210_PHYTMSR);

	reg &= ~PHYTMSR_UNPLUG;
	iowrite32(reg, fotg210->reg + FOTG210_PHYTMSR);
}

static int fotg210_udc_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct fotg210_udc *fotg210 = gadget_to_fotg210(g);
	u32 value;

	/* hook up the driver */
	driver->driver.bus = NULL;
	fotg210->driver = driver;

#ifdef CONFIG_USB_FOTG210_OTG
	value = ioread32(fotg210->reg + FOTG210_OTGCSR);

	/* if peripheral mode */
	if (value & OTGCSR_CROLE) {
		//disable Host
		value = ioread32(fotg210->reg + FOTG210_USBCMD);
		if (value & USBCMD_RUN) {
			value &= ~USBCMD_RUN;
			iowrite32(value, fotg210->reg + FOTG210_USBCMD);
		}

		//Mask Host interrupt
		iowrite32(GMIR_MHC_INT, fotg210->reg + FOTG210_GMIR);

		/* clear device un-plug */
		fotg210_disable_unplug(fotg210);

		/* enable device global interrupt */
		value = ioread32(fotg210->reg + FOTG210_DMCR);
		value |= DMCR_GLINT_EN;
		iowrite32(value, fotg210->reg + FOTG210_DMCR);
	}
	/* if host mode */
	else {
		//Mask device interrupt
		iowrite32(GMIR_MDEV_INT, fotg210->reg + FOTG210_GMIR);
	}
#else
	fotg210_disable_unplug(fotg210);

	/* enable device global interrupt */
	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value |= DMCR_GLINT_EN;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);
#endif
	return 0;
}

static void fotg210_init(struct fotg210_udc *fotg210)
{
	u32 value;

	/* disable global interrupt and set int polarity to active high */
#ifdef OLD_DESIGN
	iowrite32(GMIR_MHC_INT | GMIR_MOTG_INT | GMIR_INT_POLARITY,
		  fotg210->reg + FOTG210_GMIR);
#else
	iowrite32(GMIR_MHC_INT | GMIR_MOTG_INT,
		  fotg210->reg + FOTG210_GMIR);
#endif
	/* disable device global interrupt */
	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value &= ~DMCR_GLINT_EN;
#ifdef CONFIG_USB_FOTG210_UDC_HALF_SPEED
	value |= DMCR_HALF_SPEED;
#endif
	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	/* disable all fifo interrupt */
	iowrite32(~(u32)0, fotg210->reg + FOTG210_DMISGR1);

	/* disable cmd end */
	/* mask CX_OUT_INT until ep0_queue() */
	value = ioread32(fotg210->reg + FOTG210_DMISGR0);
	value |= (DMISGR0_MCX_COMEND | DMISGR0_MCX_OUT_INT);
	iowrite32(value, fotg210->reg + FOTG210_DMISGR0);

	// on device mode, Dev_Idle on A369 always set, so we disable interrupt.
	// on FPGA we have no such issue
	// we also disable Dev_Wakeup_byVBUS
	iowrite32((DMISGR2_MDEV_IDLE | DMISGR2_MDEV_WAKEUP_VBUS),
		fotg210->reg + FOTG210_DMISGR2);

	/* set default value */
	iowrite32(0x33333333, fotg210->reg + FOTG210_EPMAP);
	iowrite32(0x33333333, fotg210->reg + FOTG210_EPMAP2);
}

static int fotg210_udc_stop(struct usb_gadget *g)
{
	struct fotg210_udc *fotg210 = gadget_to_fotg210(g);
	unsigned long	flags;

	spin_lock_irqsave(&fotg210->lock, flags);

	fotg210_init(fotg210);
	fotg210->driver = NULL;

	spin_unlock_irqrestore(&fotg210->lock, flags);

	return 0;
}

static int fotg210_udc_pullup(struct usb_gadget *g, int is_on)
{
	return 0;
}

static const struct usb_gadget_ops fotg210_gadget_ops = {
	.pullup			= fotg210_udc_pullup,
	.udc_start		= fotg210_udc_start,
	.udc_stop		= fotg210_udc_stop,
};

static int fotg210_udc_remove(struct platform_device *pdev)
{
	struct fotg210_udc *fotg210 = platform_get_drvdata(pdev);

	usb_del_gadget_udc(&fotg210->gadget);
	iounmap(fotg210->reg);
	free_irq(platform_get_irq(pdev, 0), fotg210);

	fotg210_ep_free_request(&fotg210->ep[0]->ep, fotg210->ep0_req);
	kfree(fotg210);

	return 0;
}

static int fotg210_udc_probe(struct platform_device *pdev)
{
	struct resource *res, *ires;
	struct fotg210_udc *fotg210 = NULL;
	struct fotg210_ep *_ep[FOTG210_MAX_NUM_EP];
	int ret = 0;
	int i;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("platform_get_resource error.\n");
		return -ENODEV;
	}

	ires = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!ires) {
		pr_err("platform_get_resource IORESOURCE_IRQ error.\n");
		return -ENODEV;
	}

	ret = -ENOMEM;

	/* initialize udc */
	fotg210 = kzalloc(sizeof(struct fotg210_udc), GFP_KERNEL);
	if (fotg210 == NULL)
		goto err_alloc;

	for (i = 0; i < FOTG210_MAX_NUM_EP; i++) {
		_ep[i] = kzalloc(sizeof(struct fotg210_ep), GFP_KERNEL);
		if (_ep[i] == NULL)
			goto err_alloc;
		fotg210->ep[i] = _ep[i];
	}

	fotg210->reg = ioremap(res->start, resource_size(res));
	if (fotg210->reg == NULL) {
		pr_err("ioremap error.\n");
		goto err_map;
	}

	spin_lock_init(&fotg210->lock);

	platform_set_drvdata(pdev, fotg210);

	fotg210->gadget.ops = &fotg210_gadget_ops;

	fotg210->gadget.max_speed = USB_SPEED_HIGH;
	fotg210->gadget.name = udc_name;

	INIT_LIST_HEAD(&fotg210->gadget.ep_list);

	for (i = 0; i < FOTG210_MAX_NUM_EP; i++) {
		struct fotg210_ep *ep = fotg210->ep[i];

		if (i) {
			INIT_LIST_HEAD(&fotg210->ep[i]->ep.ep_list);
			list_add_tail(&fotg210->ep[i]->ep.ep_list,
				      &fotg210->gadget.ep_list);
		}
		ep->fotg210 = fotg210;
		INIT_LIST_HEAD(&ep->queue);
		ep->ep.name = fotg210_ep_name[i];
		ep->ep.ops = &fotg210_ep_ops;
		usb_ep_set_maxpacket_limit(&ep->ep, (unsigned short) ~0);

		if (i == 0) {
			ep->ep.caps.type_control = true;
		} else {
			ep->ep.caps.type_iso = true;
			ep->ep.caps.type_bulk = true;
			ep->ep.caps.type_int = true;
		}

		ep->ep.caps.dir_in = true;
		ep->ep.caps.dir_out = true;
	}
	usb_ep_set_maxpacket_limit(&fotg210->ep[0]->ep, 0x40);
	fotg210->gadget.ep0 = &fotg210->ep[0]->ep;
	INIT_LIST_HEAD(&fotg210->gadget.ep0->ep_list);

	fotg210->ep0_req = fotg210_ep_alloc_request(&fotg210->ep[0]->ep,
				GFP_KERNEL);
	if (fotg210->ep0_req == NULL)
		goto err_req;

	fotg210_init(fotg210);

	ret = request_irq(ires->start, fotg210_irq, IRQF_SHARED,
			  udc_name, fotg210);
	if (ret < 0) {
		pr_err("request_irq error (%d)\n", ret);
		goto err_irq;
	}

	ret = usb_add_gadget_udc(&pdev->dev, &fotg210->gadget);
	if (ret)
		goto err_add_udc;

	for (i = 0; i < FOTG210_MAX_NUM_EP; i++) {
		struct fotg210_ep *ep = fotg210->ep[i];

		ep->irq_current_task = kzalloc(sizeof(struct fotg210_irq_task), GFP_KERNEL);
		if (ep->irq_current_task == NULL) {
			pr_err("irq_current_task kzalloc error.\n");
			goto err_add_udc;
		}

		ep->irq_current_task->transfer_length = 0;
		ep->irq_current_task->req = NULL;
	}

#if defined (CONFIG_USB_FOTG210_UDC_VDMA) && defined (RELEASE_PACKET_SIZE_OUT_TRANSFER)
	/* thread for monitoring packet-size transfer */
	fotg210->out_thread_task =
			kthread_create(fotg210_out_thread, fotg210, "udc-out-process");
	if (IS_ERR(fotg210->out_thread_task)) {
		pr_err("out_thread_task create error.\n");
		ret = PTR_ERR(fotg210->out_thread_task);
		goto err_add_udc;
	}

	wake_up_process(fotg210->out_thread_task);
#endif
	dev_info(&pdev->dev, "%s probe\n", udc_name);

	return 0;

err_add_udc:
err_irq:
	free_irq(ires->start, fotg210);

err_req:
	fotg210_ep_free_request(&fotg210->ep[0]->ep, fotg210->ep0_req);

err_map:
	if (fotg210->reg)
		iounmap(fotg210->reg);

err_alloc:
	kfree(fotg210);

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id fotg210_udc_dt_ids[] = {
	{ .compatible = "faraday,fotg210_udc" },
	{ }
};

MODULE_DEVICE_TABLE(of, fotg210_udc_dt_ids);
#endif

static struct platform_driver fotg210_driver = {
	.driver		= {
		.name =	(char *)udc_name,
		.of_match_table = of_match_ptr(fotg210_udc_dt_ids),
	},
	.probe		= fotg210_udc_probe,
	.remove		= fotg210_udc_remove,
};

module_platform_driver(fotg210_driver);

MODULE_AUTHOR("Faraday Technology Corporation");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
