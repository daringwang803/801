/*
 * FARADAY FTUART010 IrDA SIR Driver
 *
 * Copyright (C) 2019 Faraday Technology Corp.
 * Jack Chain <jack_ch@faraday-tech.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
 
/******************************************************************************
 * include file  
 *****************************************************************************/
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <net/irda/wrapper.h>
#include <net/irda/irda_device.h>
#include <linux/of.h>

//For A320 EVB
#ifdef CONFIG_FTPMU010
#include <mach/ftpmu010.h>
#endif

#include "ftuart010_irda-sir.h"

/******************************************************************************
 * Define  
 *****************************************************************************/
#define MAX_TIMEOUT 10000000
unsigned int psr_val = 2;

/******************************************************************************
 * Structure 
 *****************************************************************************/
struct ftuart010_sir_self {
   void __iomem         *membase;
   unsigned int          irq;
   struct clk           *clk;

   struct net_device    *ndev;

   struct irlap_cb      *irlap;
   struct qos_info       qos;

   iobuff_t              tx_buff;
   iobuff_t              rx_buff;
   
   unsigned int          baudrate;
};

/******************************************************************************
 * Basic Read/Write Register functions 
 *****************************************************************************/

static unsigned int irda_inw(unsigned int port)
{
   return inw(port);
   //return (*((unsigned int *)port));
}

static void irda_outw(unsigned int port,unsigned int data)
{
   outw(data,port);
   //*((unsigned int *)port)=data;
}

//=============================================================================
// Function Name: fLib_SIR_Read_RBR 
// Description: Register "RBR[0x00]" (READ)
//=============================================================================
static unsigned int fLib_SIR_Read_RBR(unsigned int port)
{
   return (irda_inw(port + SERIAL_RBR));
}

//=============================================================================
// Function Name: fLib_SIR_Enable_IER
// Description: Register "IER[0x04]" (READ/WRITE)
//=============================================================================
static void fLib_SIR_Enable_IER(unsigned int port, unsigned int mode)
{
   unsigned int data;
   
   data = irda_inw(port + SERIAL_IER);
   data |= mode;
   irda_outw(port + SERIAL_IER, data);
}

//=============================================================================
// Function Name: fLib_SIR_Disable_IER
// Description: Register "IER[0x04]" (READ/WRITE)
//=============================================================================
static void fLib_SIR_Disable_IER(unsigned int port, unsigned int mode)
{
   unsigned int data;
   
   data = irda_inw(port + SERIAL_IER);
   data &= ~mode;
   irda_outw(port + SERIAL_IER, data);
}

//=============================================================================
// Function Name: fLib_IRDA_Read_IIR 
// Description: Register "IIR[0x08]" (READ)
//=============================================================================
static unsigned int fLib_SIR_Read_IIR(unsigned int port)
{
   return (irda_inw(port + SERIAL_IIR));
}

//=============================================================================
// Function Name: fLib_SIR_Set_FCR
// Description: Register "FCR[0x08]" (WRITE)
//=============================================================================
static void fLib_SIR_Set_FCR(unsigned int port, unsigned int mode)
{
   irda_outw(port + SERIAL_FCR,mode);
}

//=============================================================================
// Function Name: fLib_SIR_Set_FCR_Trigl_Level
// Description: Register "FCR[0x08]" (WRITE)
//=============================================================================
static void fLib_SIR_Set_FCR_Trigl_Level(unsigned int port, 
       unsigned int Tx_Tri_Level , unsigned int Rx_Tri_Level)
{
   unsigned int data = 0;

   switch(Tx_Tri_Level)
   {
      case 0:
         data = data | 0x00;
         break;
      case 1:
         data = data | 0x10;
         break;
      case 2:
         data = data | 0x20;
         break;
      case 3:
         data = data | 0x30;
         break;
      default:
         break;
   }

   switch(Rx_Tri_Level)
   {
      case 0:
         data = data | 0x00;
         break;
      case 1:
         data = data | 0x40;
         break;
      case 2:
         data = data | 0x80;
         break;
      case 3:
         data = data | 0xC0;
         break;
      default:
         break;
   }
   
   irda_outw(port+SERIAL_FCR,data);
}

//=============================================================================
// Function Name: fLib_SIR_Enable_LCR
// Description: Register "LCR[0x08]" (READ/WRITE)
//=============================================================================
static void fLib_SIR_Enable_LCR(unsigned int port, unsigned int mode)
{
   unsigned int data;
   
   data = irda_inw(port + SERIAL_LCR);
   data |= mode;
   irda_outw(port + SERIAL_LCR, data);
}

//=============================================================================
// Function Name: fLib_SIR_Enable_MCR
// Description: Register "MCR[0x10]" (READ/WRITE)
//=============================================================================
static void fLib_SIR_Enable_MCR(unsigned int port,unsigned int mode)
{
   unsigned int data;

   data = irda_inw(port + SERIAL_MCR);
   data |= mode;
   irda_outw(port + SERIAL_MCR, data);
}

//=============================================================================
// Function Name: fLib_SIR_Disable_MCR
// Description: Register "MCR[0x10]" (READ/WRITE)
//=============================================================================
static void fLib_SIR_Disable_MCR(unsigned int port,unsigned int mode)
{
   unsigned int data;

   data = irda_inw(port + SERIAL_MCR);
   data &= ~mode;
   irda_outw(port + SERIAL_MCR, data);
}

//=============================================================================
// Function Name: fLib_SIR_Enable_MDR
// Description: Register "MDR[0x20]" (READ/WRITE)
//=============================================================================
static void fLib_SIR_Enable_MDR(unsigned int port, unsigned int mode)
{
   unsigned int data;
   
   data = irda_inw(port + SERIAL_MDR);
   data |= mode;
   irda_outw(port + SERIAL_MDR, data);
}

//=============================================================================
// Function Name: fLib_SIR_Enable_ACR
// Description: Register "ACR[0x24]" (READ/WRITE)
//=============================================================================
static void fLib_SIR_Enable_ACR(unsigned int port,unsigned int mode)
{
   unsigned int data;

   data = irda_inw(port + SERIAL_ACR);
   data |= mode;
   irda_outw(port + SERIAL_ACR, data);
}

//=============================================================================
// Function Name: fLib_SIR_Disable_ACR
// Description: Register "ACR[0x24]" (READ/WRITE)
//=============================================================================
static void fLib_SIR_Disable_ACR(unsigned int port,unsigned int mode)
{
   unsigned int data;

   data = irda_inw(port + SERIAL_ACR);
   data &= ~mode;
   irda_outw(port + SERIAL_ACR, data);
}

//=============================================================================
// Function Name: fLib_SIR_Set_Baud_Rate
// Description: 
//=============================================================================
static void fLib_SIR_Set_Baud_Rate(unsigned int port, unsigned int baudrate)
{
   unsigned int PSR_Value;
      
   // 1.Set DLAB=1
   irda_outw(port + SERIAL_LCR, 0x83); 

   // 2.Set PSR
#ifdef CONFIG_MACH_LEO
   PSR_Value = psr_val; 
#else
   PSR_Value = (unsigned int)(IRDA_CLK/PSR_CLK);
#endif
   if(PSR_Value>0x1f)
   {
      PSR_Value = 0x1f;
   }  
   irda_outw(port + SERIAL_PSR, PSR_Value);
      
   // 3.Set DLM/DLL  => Set baud rate 
   irda_outw(port + SERIAL_DLM, ((baudrate & 0xf00) >> 8));
   irda_outw(port + SERIAL_DLL, (baudrate & 0xff));
 
   printk("[%s]PSR_Value=0x%x baudrate=0x%x\n",__func__,PSR_Value,baudrate);
 
   // 4.Set DLAB=0
   irda_outw(port + SERIAL_LCR,0x3);   
}

//=============================================================================
// Function Name: fLib_SIR_Init
// Description: FTUART010 IrDA SIR mode Initial function
//=============================================================================
static void fLib_SIR_Init(unsigned int port, unsigned int baudrate,
      unsigned int SIP_PW_Value, unsigned int dwRX_Tri_Value,unsigned int bInv)
{   
   //1.Set Baud Rate
   fLib_SIR_Set_Baud_Rate(port, baudrate);
   //2.Set LCR Set Character=8 / Stop_Bit=1 / Parity=Disable
   fLib_SIR_Enable_LCR(port, 0x03);
   //3.Set MDR SIR Mode 
   fLib_SIR_Enable_MDR(port, SERIAL_MDR_SIR);
   //4.Set MCR Out3 bit 
   fLib_SIR_Enable_MCR(port, SERIAL_MCR_OUT3);
   //5.Disable MCR DMA2: PIO mode
   fLib_SIR_Disable_MCR(port, SERIAL_MCR_DMA2);
   //6.Set FCR RX/TX FIFO Trigger Level
   fLib_SIR_Set_FCR_Trigl_Level(port,dwRX_Tri_Value,dwRX_Tri_Value); 
   //8.Set ACR SIR_PW(1.6/316)
   fLib_SIR_Enable_ACR(port, (SIP_PW_Value<<7)); 
   //9.Disable the RX
   fLib_SIR_Disable_ACR(port,SERIAL_ACR_RX_ENABLE);

   //10.Disable the TX 
   fLib_SIR_Disable_ACR(port,SERIAL_ACR_TX_ENABLE); 

   //11.Reset FCR TX/RX FIFO Reset 
   fLib_SIR_Set_FCR(port,SERIAL_FCR_RX_FIFO_RESET); 
   fLib_SIR_Set_FCR(port,SERIAL_FCR_TX_FIFO_RESET);
 
   //12.Enable FIFO 
   fLib_SIR_Set_FCR(port,SERIAL_FCR_TXRX_FIFO_ENABLE);  
 
}

//=============================================================================
// Function Name: fLib_SIR_TX_Open
// Description: FTUART010 IrDA SIR mode TX initial 
//=============================================================================
static void fLib_SIR_TX_Open(unsigned int port)
{
   //1.Init TX interrupt  
   fLib_SIR_TX_Int_init(port);

   //2.Disable the RX
   fLib_SIR_Disable_ACR(port,SERIAL_ACR_RX_ENABLE);

   //3.Set TX Enable
   fLib_SIR_Enable_ACR(port, SERIAL_ACR_TX_ENABLE); 
}

//=============================================================================
// Function Name: fLib_SIR_TX_Close
// Description: FTUART010 IrDA SIR mode TX close 
//=============================================================================
static void fLib_SIR_TX_Close(unsigned int port)
{  
   //1.Disable the TX 
   fLib_SIR_Disable_ACR(port,SERIAL_ACR_TX_ENABLE); 

   //2.Set RX Enable
   fLib_SIR_Enable_ACR(port, SERIAL_ACR_RX_ENABLE);
           
   //3.Init RX Int
   fLib_SIR_RX_Int_init(port);     
}

//=============================================================================
// Function Name: fLib_SIR_TX_Int_init 
// Description: FTUART010 IrDA SIR mode TX Interrupt initial
//=============================================================================
static void fLib_SIR_TX_Int_init(unsigned int port)
{
   //Disable DR & RLS
   fLib_SIR_Disable_IER(port, (SERIAL_IER_DR | SERIAL_IER_RLS));
}

//=============================================================================
// Function Name: fLib_SIR_TX_Data
// Description: FTUART010 IrDA SIR mode Transmit data
//=============================================================================
static int fLib_SIR_TX_Write_Data(unsigned int port, char data)
{
   int timeout = MAX_TIMEOUT;
   int t1 = MAX_TIMEOUT;
   unsigned int status;
   int ret = -1;
     
   while (timeout--)
   {
      status=irda_inw(port+SERIAL_LSR);

      if ((status & SERIAL_LSR_THRE)==SERIAL_LSR_THRE)
      {
         irda_outw(port + SERIAL_THR,data);
         ret = 0;
         break;
      }
      else
      {
         timeout--;
         udelay(1);
      }
   }
   
   if (timeout > 0)
   {// Write Data Successful, wait [LSR] THRE bit
      while (t1>0)
      {
         status=irda_inw(port+SERIAL_LSR);
         if ((status & SERIAL_LSR_THRE)==SERIAL_LSR_THRE)
         {//THRE is 1
            ret = 0;
            break;
         }
         t1--;
         udelay(1);
      }
      
      if (t1 == 0)
      {//Wait [LSR] THRE bit Timeout
         //printk("[%s] Wait [LSR]THRE Bit Timeout!!\n",__func__);
         ret = -1;
      }
   }
   else
   {// Write Data timeout, return error!!
      //printk("[%s] Write Data Timeout!!\n",__func__);
      ret = -1;
   }
 
   return ret;
}

//=============================================================================
// Function Name: fLib_SIR_RX_Open
// Description: FTUART010 IrDA SIR mode RX initial
//=============================================================================
static void fLib_SIR_RX_Open(unsigned int port)
{
   //1.Set RX Enable
   fLib_SIR_Enable_ACR(port, SERIAL_ACR_RX_ENABLE);

   //2.Init RX interrupt  
   fLib_SIR_RX_Int_init(port);
  
}

//=============================================================================
// Function Name: fLib_SIR_TX_Close
// Description: FTUART010 IrDA SIR mode RX close 
//=============================================================================
static void fLib_SIR_RX_Close(unsigned int port)
{
   //1.Init TX interrupt (Close RX Interrupt)  
   fLib_SIR_TX_Int_init(port);

   //2.Disable the RX
   fLib_SIR_Disable_ACR(port,SERIAL_ACR_RX_ENABLE);

}

//=============================================================================
// Function Name: fLib_SIR_RX_Read_Data
// Description: FTUART010 IrDA SIR mode RX Read Data
//=============================================================================
static int fLib_SIR_RX_Read_Data(unsigned int port)
{
   unsigned int status;
   int data = -1;
   
   status=irda_inw(port+SERIAL_LSR);
   if ((status & SERIAL_LSR_DR)==SERIAL_LSR_DR)
   {//Read Data
      data = irda_inw(port + SERIAL_RBR);
   } 
   
   return data;
}

//=============================================================================
// Function Name: fLib_SIR_RX_Int_init 
// Description: FTUART010 IrDA SIR mode RX Interrupt initial 
//=============================================================================
static void fLib_SIR_RX_Int_init(unsigned int port)
{
   //Enable DR & RLS, Disable TE 
   fLib_SIR_Enable_IER(port, (SERIAL_IER_DR | SERIAL_IER_RLS));
   fLib_SIR_Disable_IER(port, SERIAL_IER_TE);
}

/******************************************************************************
 * SIR mode Tx/Rx function 
 *****************************************************************************/
static int __ftuart010_sir_init_iobuf(iobuff_t *io, int size)
{
   io->head = kmalloc(size, GFP_KERNEL);
   if (!io->head)
      return -ENOMEM;

   io->truesize   = size;
   io->in_frame   = FALSE;
   io->state      = OUTSIDE_FRAME;
   io->data       = io->head;

   return 0;
}

static void ftuart010_sir_remove_iobuf(struct ftuart010_sir_self *self)
{
   kfree(self->rx_buff.head);
   kfree(self->tx_buff.head);

   self->rx_buff.head = NULL;
   self->tx_buff.head = NULL;
}

static int ftuart010_sir_init_iobuf(struct ftuart010_sir_self *self, 
      int rxsize, int txsize)
{
   int err = -ENOMEM;

   if (self->rx_buff.head || self->tx_buff.head) 
   {
      dev_err(&self->ndev->dev, "iobuff has already existed.");
      return err;
   }

   err = __ftuart010_sir_init_iobuf(&self->rx_buff, rxsize);
   if (err)
      goto iobuf_err;

   err = __ftuart010_sir_init_iobuf(&self->tx_buff, txsize);

iobuf_err:
   if (err)
      ftuart010_sir_remove_iobuf(self);

   return err;
}

/******************************************************************************
 * SIR mode Tx/Rx function 
 *****************************************************************************/
static void ftuart010_sir_tx(struct ftuart010_sir_self *self)
{
   int ret = 0;
   int count = 1;

   while (self->tx_buff.len > 0)
   {
      ret = fLib_SIR_TX_Write_Data((unsigned int)self->membase, self->tx_buff.data[0]);
      printk("*************uart sir tx count%d[%x] ",count,self->tx_buff.data[0]);
#ifdef CONFIG_MACH_LEO
      udelay(400);
#endif
      if (ret == 0)
      {
         self->tx_buff.len--;
         self->tx_buff.data++;
         count++;
      }
      else
      {//Write Data Error!!
         dev_err(&self->ndev->dev, "Write Data Timeout!!");
         return;
      }
   } 
   
   mdelay(1);
}

static void ftuart010_sir_rx(struct ftuart010_sir_self *self)
{
   int timeout = MAX_TIMEOUT;
   int data;
   int count = 1;

   while (timeout--) 
   {
      data = fLib_SIR_RX_Read_Data((unsigned int)self->membase);
      //printk("%d(%x) ",count,data);
#ifdef CONFIG_MACH_LEO
      udelay(100);
#endif
      if (data < 0){
         mdelay(1);
         break;
      }

      async_unwrap_char(self->ndev, &self->ndev->stats,
              &self->rx_buff, (char)data);
      count++;
   }  
   
   //Check if Read data Timeout
   if (timeout <= 0)
   {
      dev_err(&self->ndev->dev, "Read Data Timeout!!");
   }   
}

/******************************************************************************
 * IRQ handler function 
 *****************************************************************************/
static irqreturn_t ftuart010_sir_irq(int irq, void *dev_id)
{
   struct ftuart010_sir_self *self = dev_id;
   unsigned int reg;
   unsigned int status;
      
   reg = fLib_SIR_Read_IIR((unsigned int)self->membase);
   reg &= 0xf;
   //printk("[%s] irq=%d reg=0x%x\n",__func__,irq,reg);
   if (reg == SERIAL_IIR_RLS)
   {//Receiver Line Status
      status=irda_inw((unsigned int)(self->membase+SERIAL_LSR));

      if ((status & SERIAL_LSR_OE) == SERIAL_LSR_OE)
      {
         dev_err(&self->ndev->dev, "Overrun ERROR!!");
      }
      if ((status & SERIAL_LSR_PE) == SERIAL_LSR_PE)
      {
         dev_err(&self->ndev->dev, "Parity ERROR!!");
      }
      if ((status & SERIAL_LSR_FE) == SERIAL_LSR_FE)
      {
         dev_err(&self->ndev->dev, "Framing ERROR!!");
      }
      if ((status & SERIAL_LSR_BI) == SERIAL_LSR_BI)
      {
         dev_err(&self->ndev->dev, "Break Interrupt!!");
      }
   }
   if (reg == SERIAL_IIR_TIMEOUT)
   {//read RBR to clear timeout interrupt
      reg = fLib_SIR_Read_RBR((unsigned int)self->membase);
   }
   else if (reg == SERIAL_IIR_DR)
   {//ready to read data
      ftuart010_sir_rx(self);
   } 
         
   return IRQ_HANDLED;
}

/******************************************************************************
 * net_device_ops functions
 *****************************************************************************/
static int ftuart010_sir_hard_xmit(struct sk_buff *skb, struct net_device *ndev)
{
   struct ftuart010_sir_self *self = netdev_priv(ndev);

   netif_stop_queue(ndev);

   self->tx_buff.data = self->tx_buff.head;
   self->tx_buff.len = 0;
   
   if (skb->len)
      self->tx_buff.len = async_wrap_skb(skb, self->tx_buff.data,
                     self->tx_buff.truesize);

   dev_kfree_skb(skb);

   //call tx
   fLib_SIR_TX_Open((unsigned int)self->membase);
   printk("SIR TX open\n");
   ftuart010_sir_tx(self);
   printk("uart sir tx\n");
   fLib_SIR_TX_Close((unsigned int)self->membase);
   printk("SIR TX close\n");
   
   netif_wake_queue(ndev);

   return 0;
}

static int ftuart010_sir_ioctl(struct net_device *ndev, struct ifreq *ifreq, int cmd)
{
   return 0; 
}

static struct net_device_stats *ftuart010_sir_stats(struct net_device *ndev)
{
   struct ftuart010_sir_self *self = netdev_priv(ndev);
   return &self->ndev->stats;
}

static int ftuart010_sir_open(struct net_device *ndev)
{
   struct ftuart010_sir_self *self = netdev_priv(ndev);
   int err;

   self->irlap = irlap_open(ndev, &self->qos, "ftuart010_irda_sir");
   if (!self->irlap) 
   {
      err = -ENODEV;
      goto open_err;
   }

   netif_start_queue(ndev);

   fLib_SIR_RX_Open((unsigned int)self->membase);

   dev_info(&self->ndev->dev, "opened\n");
   return 0;

open_err:
   return err; 
   
}

static int ftuart010_sir_stop(struct net_device *ndev)
{
   struct ftuart010_sir_self *self = netdev_priv(ndev);

   if (self->irlap) 
   {
      irlap_close(self->irlap);
      self->irlap = NULL;
   }

   netif_stop_queue(ndev);

   fLib_SIR_RX_Close((unsigned int)self->membase);

   dev_info(&ndev->dev, "stopped\n");

   return 0;
}

static const struct net_device_ops ftuart010_sir_ndo = {
   .ndo_open         = ftuart010_sir_open,
   .ndo_stop         = ftuart010_sir_stop,
   .ndo_start_xmit   = ftuart010_sir_hard_xmit,
   .ndo_do_ioctl     = ftuart010_sir_ioctl,
   .ndo_get_stats    = ftuart010_sir_stats,
};



#if 0  //zg-2020117
static ssize_t ctrl_send_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
        printk(KERN_INFO"ctrl_send_store");
        struct platform_device *pdev = to_platform_device(dev);
        struct net_device *ndev = platform_get_drvdata(pdev);
        struct ftuart010_sir_self *self = netdev_priv(ndev);
	self->tx_buff.data = buf;
	self->tx_buff.len = count;
        fLib_SIR_TX_Open((unsigned int)self->membase);
        ftuart010_sir_tx(self);
        fLib_SIR_TX_Close((unsigned int)self->membase);


        return count;
 }


static DEVICE_ATTR(ctrl_send,0644,NULL,ctrl_send_store);
#endif



/******************************************************************************
 * platform driver functions
 *****************************************************************************/
static int ftuart010_sir_probe(struct platform_device *pdev)
{
   struct net_device *ndev;
   struct ftuart010_sir_self *self;
   struct resource *res;
   int irq;
   int err = -ENOMEM;
   unsigned int input_baud;
   unsigned int baud;

   res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
   
   irq = platform_get_irq(pdev, 0);
   if (!res || irq < 0) 
   {
      dev_err(&pdev->dev, "Not enough platform resources.\n");
      goto exit;
   }

   ndev = alloc_irdadev(sizeof(*self));
   if (!ndev)
      goto exit;

   self = netdev_priv(ndev);
   self->membase = ioremap_nocache(res->start, resource_size(res));
   if (!self->membase) 
   {
      err = -ENXIO;
      dev_err(&pdev->dev, "Unable to ioremap.\n");
      goto err_mem_1;
   }

   err = ftuart010_sir_init_iobuf(self, IRDA_SKB_MAX_MTU, IRDA_SIR_MAX_FRAME);
   if (err)
      goto err_mem_2;

   self->clk = clk_get(&pdev->dev, "irda");
   if (IS_ERR(self->clk)) 
   {
      dev_err(&pdev->dev, "cannot get clock: irda!!\n");
      err = -ENODEV;
      goto err_mem_3;
   }

   err = clk_prepare_enable(self->clk);
   if (err) 
   {
      dev_err(&pdev->dev, "failed to enable clock!!\n");
      goto err_mem_4;
   }

   irda_init_max_qos_capabilies(&self->qos);

   ndev->netdev_ops  = &ftuart010_sir_ndo;
   ndev->irq         = irq;
   self->ndev        = ndev;
	 if (pdev->dev.of_node) {
		 of_property_read_u32(pdev->dev.of_node, "baud", &baud);
     printk("baud %d\n",baud);
     switch(baud){
       case 9600:
          self->baudrate = IRDA_BAUD_9600;
          self->qos.baud_rate.bits      = IR_9600; 
          psr_val = 24;
          break;
       case 115200:
          self->baudrate = IRDA_BAUD_115200;
          self->qos.baud_rate.bits      = IR_115200; 
          psr_val = 2;
          break;
     }
   }else{
     self->qos.baud_rate.bits      = IR_9600; 
   }
      
   self->qos.magic               = LAP_MAGIC;
   self->qos.min_turn_time.bits  = 1; /* 10 ms or more */
   self->qos.data_size.value     = 32;
   irda_qos_bits_to_value(&self->qos);

   err = register_netdev(ndev);
   if (err)
      goto err_mem_4;

   platform_set_drvdata(pdev, ndev);
   err = devm_request_irq(&pdev->dev, irq, ftuart010_sir_irq, 0, "ftuart010_sir", self);
   if (err) 
   {
      dev_warn(&pdev->dev, "Unable to attach ftuart010_sir interrupt\n");
      goto err_mem_4;
   }

//For A320 EVB, start Irda need to write PMU Register
#ifdef CONFIG_FTPMU010
   reg = ftpmu010_readl(0x28);
   reg |= 0x1004;
   reg &= 0xfffffdff;
   ftpmu010_writel(reg,0x28);
#endif

#ifndef CONFIG_MACH_LEO
   self->baudrate = IRDA_BAUD_115200;
   switch(self->baudrate){
     case IRDA_BAUD_115200:
         input_baud = clk_get_rate(self->clk)/1843200;
         break;
     case IRDA_BAUD_57600:
         input_baud = clk_get_rate(self->clk)/921600;
         break;
     case IRDA_BAUD_38400:
         input_baud = clk_get_rate(self->clk)/614400;
         break;
     case IRDA_BAUD_19200:
         input_baud = clk_get_rate(self->clk)/307200;
         break;
     case IRDA_BAUD_14400:
         input_baud = clk_get_rate(self->clk)/230400;
         break;
     case IRDA_BAUD_9600:
         input_baud = clk_get_rate(self->clk)/153600;
         break;
     default://115200
         input_baud = clk_get_rate(self->clk)/1843200;
   }
#else
   input_baud = 33;//well,  please check leo design spec (irda clock) for detailed informtaion. It was fixed and only valid in 9600,115200
#endif
#if 0 //zg-20201117
	sysfs_create_file(&pdev->dev.kobj,&dev_attr_ctrl_send.attr);
#endif

   fLib_SIR_Init((unsigned int)self->membase, input_baud ,1,1,0);
   dev_info(&pdev->dev, "FTUART010 IrDA SIR probed\n");

   goto exit;

err_mem_4:
   clk_put(self->clk);
err_mem_3:
   ftuart010_sir_remove_iobuf(self);
err_mem_2:
   iounmap(self->membase);
err_mem_1:
   free_netdev(ndev);
exit:
   return err;
}

static int ftuart010_sir_remove(struct platform_device *pdev)
{
   struct net_device *ndev = platform_get_drvdata(pdev);
   struct ftuart010_sir_self *self = netdev_priv(ndev);
   
   if (!self)
      return 0;

   unregister_netdev(ndev);
   clk_put(self->clk);
   ftuart010_sir_remove_iobuf(self);
   iounmap(self->membase);
   free_netdev(ndev);
#if 0 //zg-2020117
sysfs_remove_file(&pdev->dev.kobj,&dev_attr_ctrl_send.attr);
#endif
//For A320 EVB, start Irda need to write PMU Register
#ifdef CONFIG_FTPMU010
   reg = ftpmu010_readl(0x28);
   reg &= (~0x1004);
   ftpmu010_writel(reg,0x28);
#endif

   return 0;
}

static const struct of_device_id ftuart010_sir_dt_ids[] = {
   { .compatible = "faraday,ftuart010_sir", },
   {},
};

static struct platform_driver ftuart010_sir_driver = {
   .probe   = ftuart010_sir_probe,
   .remove  = ftuart010_sir_remove,
   .driver  = {
      .name = "ftuart010_irda_sir",
      .owner   = THIS_MODULE,
      .of_match_table = ftuart010_sir_dt_ids,
   },
};

/******************************************************************************
 * platform module initialization / finalization
 *****************************************************************************/
static int __init ftuart010_sir_init(void)
{
   return platform_driver_register(&ftuart010_sir_driver);
}

static void __exit ftuart010_sir_exit(void)
{
   platform_driver_unregister(&ftuart010_sir_driver);
}






module_init(ftuart010_sir_init);
module_exit(ftuart010_sir_exit);

MODULE_AUTHOR("Jack Chain <jack_ch@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday FTUART010 IrDA SIR driver");
MODULE_LICENSE("GPL");
