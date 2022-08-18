/*
 * ftlcdc210_dma.c - DMA Engine API support for ftlcdc210
 *
 * Copyright (C)  2018 Jack Chain (jack_ch@faraday-tech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ftlcdc210_dma.h"


static void __dma_tx_complete(void *param)
{
   struct ftlcdc210_dma *dma = param;

   //printk("[%s] Enter!! param=0x%x\n",__func__,param);

   dma->is_running = 0;

   //printk("[%s] Leave!! \n",__func__);
}

int ftlcdc210_DMA_imgblt(struct device *dev,dma_addr_t src,dma_addr_t dst,size_t xcnt,
           size_t ycnt,size_t sstride,size_t dstride)
{
   struct ftlcdc210_dma *dma;
   dma_cap_mask_t          mask;
   int    ret = 0;
   struct dma_async_tx_descriptor   *desc = NULL;
   //int i;
   struct dma_interleaved_template *xt = NULL;
   enum dma_ctrl_flags  flags;

   //printk("[%s] Enter!! src=0x%x dst=0x%x xcnt=0x%x ycnt=0x%x\n",__func__,src,dst,xcnt,ycnt);
   //printk("[%s] Enter!! sstride=0x%x dstride=0x%x \n",__func__,sstride,dstride);
    
   dma = kmalloc(sizeof(struct ftlcdc210_dma),GFP_KERNEL);
   if (!dma)
   {
      printk("[%s] dma kmalloc() FAIL!!\n",__func__);
      ret = -EFAULT;
      goto error;
   }

   xt = kmalloc(sizeof(struct dma_interleaved_template), GFP_KERNEL);
   if (!xt)
   {
      printk("[%s] xt kmalloc() FAIL!!\n",__func__);
      ret = -EFAULT;
      goto error;
   }

   // Default slave configuration parameters 
   dma->slave_conf.direction        = DMA_MEM_TO_MEM;
   dma->slave_conf.dst_addr_width   = DMA_SLAVE_BUSWIDTH_1_BYTE;
   dma->slave_conf.src_addr_width   = DMA_SLAVE_BUSWIDTH_1_BYTE;
   dma->slave_conf.src_maxburst     = 1;
   dma->slave_conf.dst_maxburst     = 1;
   dma->dev        = dev;

   dma_cap_zero(mask);
   dma_cap_set(DMA_SLAVE, mask);
   dma_cap_set(DMA_MEMCPY, mask);
   dma_cap_set(DMA_INTERLEAVE, mask);

   dma->is_running = 0;

   // Get a channel 
   dma->chan = dma_request_chan(dev,"imgblt");
   if (!dma->chan) {
      printk("[%s] dma_request_chan FAIL!!\n",__func__);
      ret = -ENODEV;
      goto error;
   }
   
   //interleave version
   xt->src_start = src;
   xt->dst_start = dst;
   xt->dir     = true;
   xt->src_inc = true;
   xt->dst_inc = true;
   xt->src_sgl = true;
   xt->dst_sgl = true;
   xt->numf = 1;
   xt->frame_size = ycnt;
   xt->sgl[0].size = xcnt;
   xt->sgl[0].icg = 0;
   xt->sgl[0].src_icg = sstride;
   xt->sgl[0].dst_icg = dstride;
   
   flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
   
   desc = dmaengine_prep_interleaved_dma(dma->chan,xt,flags);
   
   desc->callback = __dma_tx_complete;
   desc->callback_param = dma;

   dma->is_running = 1;

   dma->cookie = dmaengine_submit(desc);
   dma_async_issue_pending(dma->chan);
   
   while(dma->is_running == 1) msleep(1);
   
   dmaengine_terminate_sync(dma->chan);
   
   //memcpy version
   /*for (i=0;i<ycnt;i++)
   {
      dma->src_dma_addr = src + sstride*i;
      dma->dst_dma_addr = dst + dstride*i;
      dma->trans_size   = xcnt;
          
      desc = dmaengine_prep_dma_memcpy(
         dma->chan, dma->dst_dma_addr, dma->src_dma_addr, dma->trans_size,
         (DMA_CTRL_ACK | DMA_PREP_INTERRUPT));
      
      if (!desc) {
         printk("[%s] dmaengine_prep_dma_memset FAIL!!\n",__func__);
         ret = -EBUSY;
         goto error;
      }
      
      desc->callback = __dma_tx_complete;
      desc->callback_param = dma;

      dma->cookie = dmaengine_submit(desc);
      dma_async_issue_pending(dma->chan);    
      dmaengine_terminate_sync(dma->chan);
      
   }*/
   
   // Release DMA
   dmaengine_terminate_sync(dma->chan);
   dma_release_channel(dma->chan);
   dma->chan = NULL;
   dma->is_running = 0;
   
error:
   
   kfree(xt);
   kfree(dma);
   
   //printk("[%s] Leave!! ret=0x%x\n",__func__,ret);
   
   return ret;    
}
           
int ftlcdc210_DMA_fillrec(struct device *dev,dma_addr_t dst,size_t xcnt,
           size_t ycnt,size_t dstride,unsigned int color)
{
   struct ftlcdc210_dma *dma;
   dma_cap_mask_t          mask;
   int    ret = 0;
   struct dma_async_tx_descriptor   *desc = NULL;
   int i;

   //printk("[%s] Enter!! dst=0x%x xcnt=0x%x ycnt=0x%x dstride=0x%x color=0x%x\n",
   //           __func__,dst,xcnt,ycnt,dstride,color); 

   dma = kmalloc(sizeof(struct ftlcdc210_dma),GFP_KERNEL);
   if (!dma)
   {
      printk("[%s] dma kmalloc() FAIL!!\n",__func__);
      ret = -EFAULT;
      goto error;
   }

   // Default slave configuration parameters 
   dma->slave_conf.direction        = DMA_MEM_TO_MEM;
   dma->slave_conf.dst_addr_width   = DMA_SLAVE_BUSWIDTH_1_BYTE;
   dma->slave_conf.src_addr_width   = DMA_SLAVE_BUSWIDTH_1_BYTE;
   dma->slave_conf.src_maxburst     = 1;
   dma->slave_conf.dst_maxburst     = 1;
   dma->dev        = dev;

   dma_cap_zero(mask);
   dma_cap_set(DMA_SLAVE, mask);
   dma_cap_set(DMA_MEMSET, mask);

   dma->is_running = 0;

   // Get a channel 
   dma->chan = dma_request_chan(dev,"imgblt");
   if (!dma->chan) {
      printk("[%s] dma_request_chan FAIL!!\n",__func__);
      ret = -ENODEV;
      goto error;
   }
   
   for (i=0;i<ycnt;i++)
   {
      
      dma->dst_dma_addr = dst + dstride*i;
      dma->value        = color;
      dma->trans_size   = xcnt;
     
      desc = dmaengine_prep_dma_memset(
         dma->chan, dma->dst_dma_addr, dma->value, dma->trans_size,
         (DMA_CTRL_ACK | DMA_PREP_INTERRUPT));
               
      if (!desc) {
         printk("[%s] dmaengine_prep_dma_memset FAIL!!\n",__func__);
         ret = -EBUSY;
         goto error;
      }
      
      desc->callback = __dma_tx_complete;
      desc->callback_param = dma;

      dma->is_running = 1;

      dma->cookie = dmaengine_submit(desc);
      dma_async_issue_pending(dma->chan); 
      
      while(dma->is_running == 1) msleep(1);       
   }
   
   // Release DMA
   dmaengine_terminate_sync(dma->chan);
   dma_release_channel(dma->chan);
   dma->chan = NULL;
   dma->is_running = 0;
   
error:
   
   kfree(dma);
   
   //printk("[%s] Leave!! ret=0x%x\n",__func__,ret);
   
   return ret;   
         
}
           
int ftlcdc210_DMA_copyarea(struct device *dev,dma_addr_t src,dma_addr_t dst,size_t xcnt,
           size_t ycnt,size_t sstride,size_t dstride)
{
   struct ftlcdc210_dma *dma;
   dma_cap_mask_t          mask;
   int    ret = 0;
   struct dma_async_tx_descriptor   *desc = NULL;
   //int i;
   struct dma_interleaved_template *xt = NULL;
   enum dma_ctrl_flags  flags;

   //printk("[%s] Enter!! src=0x%x dst=0x%x xcnt=0x%x ycnt=0x%x\n",__func__,src,dst,xcnt,ycnt);
   //printk("[%s] Enter!! sstride=0x%x dstride=0x%x \n",__func__,sstride,dstride);
    
    dma = kmalloc(sizeof(struct ftlcdc210_dma),GFP_KERNEL);
   if (!dma)
   {
      printk("[%s] dma kmalloc() FAIL!!\n",__func__);
      ret = -EFAULT;
      goto error;
   }

   xt = kmalloc(sizeof(struct dma_interleaved_template), GFP_KERNEL);
   if (!xt)
   {
      printk("[%s] xt kmalloc() FAIL!!\n",__func__);
      ret = -EFAULT;
      goto error;
   }

   // Default slave configuration parameters 
   dma->slave_conf.direction        = DMA_MEM_TO_MEM;
   dma->slave_conf.dst_addr_width   = DMA_SLAVE_BUSWIDTH_1_BYTE;
   dma->slave_conf.src_addr_width   = DMA_SLAVE_BUSWIDTH_1_BYTE;
   dma->slave_conf.src_maxburst     = 1;
   dma->slave_conf.dst_maxburst     = 1;
   dma->dev        = dev;

   dma_cap_zero(mask);
   dma_cap_set(DMA_SLAVE, mask);
   dma_cap_set(DMA_MEMCPY, mask);
   dma_cap_set(DMA_INTERLEAVE, mask);

   dma->is_running = 0;

   // Get a channel 
   dma->chan = dma_request_chan(dev,"imgblt");
   if (!dma->chan) {
      printk("[%s] dma_request_chan FAIL!!\n",__func__);
      ret = -ENODEV;
      goto error;
   }
   
   //interleave version
   xt->src_start = src;
   xt->dst_start = dst;
   xt->dir     = true;
   xt->src_inc = true;
   xt->dst_inc = true;
   xt->src_sgl = true;
   xt->dst_sgl = true;
   xt->numf = 1;
   xt->frame_size = ycnt;
   xt->sgl[0].size = xcnt;
   xt->sgl[0].icg = 0;
   xt->sgl[0].src_icg = sstride;
   xt->sgl[0].dst_icg = dstride;
   
   flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
   
   desc = dmaengine_prep_interleaved_dma(dma->chan,xt,flags);
   
   desc->callback = __dma_tx_complete;
   desc->callback_param = dma;

   dma->is_running = 1;

   dma->cookie = dmaengine_submit(desc);
   dma_async_issue_pending(dma->chan);  
   
   while(dma->is_running == 1) msleep(1);
     
   dmaengine_terminate_sync(dma->chan);
   
   
   //memcpy version  
   /*for (i=0;i<ycnt;i++)
   {
      dma->src_dma_addr = src + sstride*i;
      dma->dst_dma_addr = dst + dstride*i;
      dma->trans_size   = xcnt;
          
      desc = dmaengine_prep_dma_memcpy(
         dma->chan, dma->dst_dma_addr, dma->src_dma_addr, dma->trans_size,
         (DMA_CTRL_ACK | DMA_PREP_INTERRUPT));
      
      if (!desc) {
         printk("[%s] dmaengine_prep_dma_memset FAIL!!\n",__func__);
         ret = -EBUSY;
         goto error;
      }
      
      desc->callback = __dma_tx_complete;
      desc->callback_param = dma;

      dma->is_running = 1;

      dma->cookie = dmaengine_submit(desc);
      dma_async_issue_pending(dma->chan);  
      
      while(dma->is_running == 1) msleep(1);
        
      dmaengine_terminate_sync(dma->chan);
      
   }*/
   
   // Release DMA
   dmaengine_terminate_sync(dma->chan);
   dma_release_channel(dma->chan);
   dma->chan = NULL;
   dma->is_running = 0;
   
error:

   kfree(xt);   
   kfree(dma);
   
   //printk("[%s] Leave!! ret=0x%x\n",__func__,ret);
   
   return ret;    
}