/*
 * Faraday FTKBC010 Keyboard Controller
 *
 * (C) Copyright 2009 Faraday Technology
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

#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/input/matrix_keypad.h>
#include <linux/of_gpio.h>

#include <asm/io.h>

#include <plat/ftkbc010.h>

#define MAX_KEY_ROWS 8
#define MAX_KEY_COLS 8
#define MAX_KEYPAD_KEYS (MAX_KEY_ROWS * MAX_KEY_COLS)

struct ftkbc010 {
   unsigned short keycode[MAX_KEYPAD_KEYS];
   struct input_dev *input;
   struct resource *res;
   void *base;
   int irq;
   unsigned int rows;
   unsigned int cols;
   unsigned int row_shift;
};

/******************************************************************************
 * internal functions
 *****************************************************************************/
static void ftkbc010_enable(struct ftkbc010 *ftkbc010)
{
   struct device *dev = &ftkbc010->input->dev;
   int reg;

   reg = FTKBC010_ASP_PERIOD(0x98fff);

   dev_info(dev, "[ASP] = %08x\n", reg);
   iowrite32(reg, ftkbc010->base + FTKBC010_OFFSET_ASP);

   reg = FTKBC010_CR_ENABLE
       | FTKBC010_CR_EN_TXINT
       | FTKBC010_CR_EN_RXINT
       | FTKBC010_CR_CL_TXINT
       | FTKBC010_CR_CL_RXINT
       | FTKBC010_CR_EN_PAD
       | FTKBC010_CR_AUTOSCAN
       | FTKBC010_CR_CL_PADINT;

   dev_info(dev, "[CR]  = %08x\n", reg);
   iowrite32(reg, ftkbc010->base + FTKBC010_OFFSET_CR);
}

static void ftkbc010_disable(struct ftkbc010 *ftkbc010)
{
   iowrite32(0, ftkbc010->base + FTKBC010_OFFSET_CR);
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftkbc010_interrupt(int irq, void *dev_id)
{
   struct ftkbc010 *ftkbc010 = dev_id;
   struct input_dev *input = ftkbc010->input;
   unsigned int status;
   unsigned int cr;

   status = ioread32(ftkbc010->base + FTKBC010_OFFSET_ISR);

   /*
    * The clear interrupt bits are inside control register,
    * so we need to read it out and then modify it -
    * completely brain-dead design.
    */
   cr = ioread32(ftkbc010->base + FTKBC010_OFFSET_CR);

   if (status & FTKBC010_ISR_RXINT) {
      dev_dbg(&input->dev, "RXINT\n");
      cr |= FTKBC010_CR_CL_RXINT;
   }

   if (status & FTKBC010_ISR_TXINT) {
      dev_dbg(&input->dev, "TXINT\n");
      cr |= FTKBC010_CR_CL_TXINT;
   }

   if (status & FTKBC010_ISR_PADINT) {
      unsigned int data;
      unsigned int row, col;
      unsigned int scancode;
      unsigned int keycode;

      data = ioread32(ftkbc010->base + FTKBC010_OFFSET_XC);
      col = ffs(0xff - FTKBC010_XC_L(data)) - 1;

      data = ioread32(ftkbc010->base + FTKBC010_OFFSET_YC);
      row = ffs(0xff - FTKBC010_YC_L(data)) - 1;

      scancode = MATRIX_SCAN_CODE(row, col, ftkbc010->row_shift);

      keycode = ftkbc010->keycode[scancode];

      input_report_key(input, keycode, 1);
      input_sync(input);

      /* We cannot tell key release - simulate one */
      input_report_key(input, keycode, 0);
      input_sync(input);

      dev_info(&input->dev, "(%x, %x) scancode %d, keycode %d\n",
               row, col, scancode, keycode);

      cr |= FTKBC010_CR_CL_PADINT;
   }

   iowrite32(cr, ftkbc010->base + FTKBC010_OFFSET_CR);

   return IRQ_HANDLED;
}

static int ftkbc010_keypad_open(struct input_dev *input)
{
   struct ftkbc010 *ftkbc010 = input_get_drvdata(input);
   
   ftkbc010_enable(ftkbc010);

   return 0;
}

static void ftkbc010_keypad_close(struct input_dev *input)
{
   struct ftkbc010 *ftkbc010 = input_get_drvdata(input);
   
   ftkbc010_disable(ftkbc010);
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
#ifdef CONFIG_USE_OF
static struct matrix_keypad_platform_data *
ftkbc010_parse_dt(struct device *dev)
{
   struct matrix_keypad_platform_data *pdata;
   struct matrix_keymap_data *keymap_data;
   struct device_node *np = dev->of_node;
   unsigned int key_count;
   uint32_t *keymap;
   int nrow, ncol;
   int ret;

   if (!np) {
      dev_err(dev, "device lacks DT data\n");
      return ERR_PTR(-ENODEV);
   }

   pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
   if (!pdata) {
      dev_err(dev, "could not allocate memory for platform data\n");
      return ERR_PTR(-ENOMEM);
   }

   of_property_read_u32(np, "keypad,num-rows", &nrow);
   of_property_read_u32(np, "keypad,num-columns", &ncol);
   pdata->num_row_gpios = nrow;
   pdata->num_col_gpios = ncol;
   if (nrow <= 0 || ncol <= 0) {
      dev_err(dev, "number of keypad rows/columns not specified\n");
      return ERR_PTR(-EINVAL);
   }

   keymap_data = devm_kzalloc(dev, sizeof(*keymap_data), GFP_KERNEL);
   if (!keymap_data) {
      dev_err(dev, "could not allocate memory for keymap data\n");
      return ERR_PTR(-ENOMEM);
   }
   pdata->keymap_data = keymap_data;

   key_count = nrow * ncol;
   keymap_data->keymap_size = key_count;
   keymap = devm_kzalloc(dev, sizeof(uint32_t) * key_count, GFP_KERNEL);
   if (!keymap) {
      dev_err(dev, "could not allocate memory for keymap\n");
      return ERR_PTR(-ENOMEM);
   }
   keymap_data->keymap = keymap;

   ret = of_property_read_u32_array(np, "linux,keymap", keymap, key_count);
   if (ret) {
      dev_err(dev, "of_property_read_u32_array ERROR!! ret = %d\n",ret);
      return ERR_PTR(ret);
   }

   return pdata;
}
#else
static inline struct matrix_keypad_platform_data *
ftkbc010_parse_dt(struct device *dev)
{
   dev_err(dev, "no platform data defined\n");

   return ERR_PTR(-EINVAL);
}
#endif
 
 
static int ftkbc010_probe(struct platform_device *pdev)
{
   //struct matrix_keypad_platform_data *pdata = pdev->dev.platform_data;
   struct matrix_keypad_platform_data *pdata;
   struct device *dev = &pdev->dev;
   struct ftkbc010 *ftkbc010;
   struct input_dev *input;
   struct resource *res;
   unsigned int row_shift, max_keys;
   int irq;
   int ret;

   //printk("[%s] Enter!!\n",__func__);

#ifdef CONFIG_USE_OF
    pdata = dev_get_platdata(&pdev->dev);
   if (!pdata) {
      pdata = ftkbc010_parse_dt(&pdev->dev);
      if (IS_ERR(pdata)) {
         dev_err(&pdev->dev, "no platform data defined\n");
         return PTR_ERR(pdata);
      }
   } else if (!pdata->keymap_data) {
      dev_err(&pdev->dev, "no keymap data defined\n");
      return -EINVAL;
   }
#else
    pdata = pdev->dev.platform_data;
#endif

   res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
   if (!res) {
      dev_err(dev, "No mmio resource defined.\n");
      return -ENXIO;
   }

   if ((irq = platform_get_irq(pdev, 0)) < 0) {
      dev_err(dev, "No irq resource defined.\n");
      ret = irq;
      goto err_get_irq;
   }

   /*
    * Allocate driver private data
    */
   ftkbc010 = kzalloc(sizeof(struct ftkbc010), GFP_KERNEL);
   if (!ftkbc010) {
      dev_err(dev, "Failed to allocate memory.\n");
      ret = -ENOMEM;
      goto err_alloc_priv;
   }
   platform_set_drvdata(pdev, ftkbc010);

   /*
    * Allocate input_dev
    */
   input = input_allocate_device();
   if (!input) {
      dev_err(dev, "Failed to allocate input device.\n");
      ret = -EBUSY;
      goto err_alloc_input_dev;
   }
   ftkbc010->input = input;

   /*
    * Map io memory
    */
   ftkbc010->res = request_mem_region(res->start, resource_size(res), dev_name(dev));
   if (!ftkbc010->res) {
      dev_err(dev, "Resources is unavailable.\n");
      ret = -EBUSY;
      goto err_req_mem_region;
   }

   ftkbc010->base = ioremap(res->start, resource_size(res));
   if (!ftkbc010->base) {
      dev_err(dev, "Failed to map registers.\n");
      ret = -ENOMEM;
      goto err_ioremap;
   }

   ret = request_irq(irq, ftkbc010_interrupt, 0, pdev->name, ftkbc010);
   if (ret < 0) {
      dev_err(dev, "Failed to request irq %d\n", irq);
      goto err_req_irq;
   }
   ftkbc010->irq = irq;

   row_shift = get_count_order(pdata->num_col_gpios);
   max_keys = pdata->num_row_gpios << row_shift;
   ftkbc010->rows = pdata->num_row_gpios;
   ftkbc010->cols = pdata->num_col_gpios;
   ftkbc010->row_shift = row_shift;

   input->name = "Faraday keyboard controller";
   input->keycode    = ftkbc010->keycode;
   input->keycodesize   = sizeof(ftkbc010->keycode[0]);
   input->keycodemax = max_keys;
   input->id.bustype = BUS_HOST;
   input->dev.parent = &pdev->dev;
   
   input->open = ftkbc010_keypad_open;
   input->close = ftkbc010_keypad_close;

   /*
    * Declare what events and event codes can be generated
    */
   input->evbit[0] = BIT_MASK(EV_KEY);

   matrix_keypad_build_keymap(pdata->keymap_data, NULL,
                                   pdata->num_row_gpios,
                                   pdata->num_col_gpios,
                                   input->keycode, input);

   input_set_drvdata(input, ftkbc010);

   /*
    * All went ok, so register to the input system
    */
   ret = input_register_device(input);
   if (ret)
      goto err_register_input;

   dev_info(dev, "irq %d, mapped at %p\n", irq, ftkbc010->base);

   return 0;

err_register_input:
   free_irq(irq, ftkbc010);
err_req_irq:
   iounmap(ftkbc010->base);
err_ioremap:
   release_mem_region(res->start, resource_size(res));
err_req_mem_region:
   input_free_device(ftkbc010->input);
err_alloc_input_dev:
   platform_set_drvdata(pdev, NULL);
   kfree(ftkbc010);
err_alloc_priv:
err_get_irq:
   release_resource(res);
   return ret;
}

static int __exit ftkbc010_remove(struct platform_device *pdev)
{
   struct ftkbc010 *ftkbc010 = platform_get_drvdata(pdev);
   struct resource *res = ftkbc010->res;

   ftkbc010_disable(ftkbc010);

   input_unregister_device(ftkbc010->input);
   free_irq(ftkbc010->irq, ftkbc010);
   iounmap(ftkbc010->base);
   release_mem_region(res->start, resource_size(res));
   input_free_device(ftkbc010->input);
   platform_set_drvdata(pdev, NULL);
   kfree(ftkbc010);
   release_resource(res);

   return 0;
}

static const struct of_device_id ftkbc010_dt_ids[] = {
   { .compatible = "faraday,ftkbc010", },
   {},
};

static struct platform_driver ftkbc010_driver = {
   .probe      = ftkbc010_probe,
   .remove     = __exit_p(ftkbc010_remove),
   .driver     = {
      .name = "ftkbc010",
      .owner   = THIS_MODULE,
      .of_match_table = ftkbc010_dt_ids,
   },
};

module_platform_driver(ftkbc010_driver);

MODULE_DESCRIPTION("FTKBC010 keyboard driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
