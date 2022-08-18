/*
 * Faraday FTLCDC210 LCD Controller
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
/******************************************************************************
 * Include files
 *****************************************************************************/
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/of_address.h>
#include <linux/uaccess.h>

#include "ftlcdc210.h"
#include "ftlcdc210_dma.h"

/******************************************************************************
 * Define Constants
 *****************************************************************************/

#define FBTESTIMGBLT    0x4620
#define FBTESTFILLREC   0x4621
#define FBTESTCOPYAREA  0x4622


struct fb_image_user_memcpy {
   
   unsigned char  *src;      /* Pointer to image data */
   //unsigned char  *dst;    /* Pointer to dst buffer */
   unsigned int size;        /* memcpy size (byte)*/
   unsigned int height;      /* fillrec height        */
   
   unsigned int src_offset;
   unsigned int dst_offset;

   unsigned int src_inc;
   unsigned int dst_inc;
   
   unsigned int value;       /* fillrec fill color    */
};

unsigned int fb0_frame_buf_phy_base;
char *fb0_frame_buf_screen_base;

/*
 * LC_CLK on A369 depends on the SCLK_CFG0 register of FTSCU010
 * 00: AHB clock
 * 01: APB clock
 * 10: external clock
 */
#define LC_CLK AHB_CLK_IN

/*
 * Select a panel configuration
 */
//#define CONFIG_AUO_A036QN01_CPLD
#define CONFIG_TPO_TD070WGEC2
//#define CONFIG_VESA_800x600_60Hz
//#define CONFIG_VESA_1024x768_60Hz

#define CONFIG_FTLCDC210_NR_FB   1

#if defined(CACHE_FB)
   #define FLUSH_CNTDOWN         2

   volatile unsigned long cur_dma_addr = NULL;
   volatile unsigned int cur_frame_size = 0;
   volatile int flush_countdown=FLUSH_CNTDOWN;
#endif //CACHE_FB

#define FTLCDC_GET_SCREEN_MODE  0x4710

/* 
 * This structure defines the hardware state of the graphics card. Normally
 * you place this in a header file in linux/include/video. This file usually
 * also includes register information. That allows other driver subsystems
 * and userland applications the ability to use the same header file to 
 * avoid duplicate work and easy porting of software. 
 */
struct FTLCDC210 {
   struct resource *res;
   struct device *dev;
   void *base;
   int irq_be; /* bus error */
   int irq_ur; /* FIFO underrun */
   int irq_nb; /* base address update */
   int irq_vs; /* vertical status */
   int nb;     /* base updated */
   wait_queue_head_t wait_nb;
   struct clk *clk;
   unsigned long clk_value_khz;
   struct FTLCDC210fb *fb[CONFIG_FTLCDC210_NR_FB];
   
   unsigned int u_offset;     // not zero when yuv420
   unsigned int v_offset;
   int scalar_on;
   //int active_fb;        // 0 ==> fb0, 1 ==> fb1,...
};


struct fb_info *active_info=NULL;         // used by the snapshot driver

// for fps statistics
static unsigned int volatile display_cnt=0;
static unsigned long volatile start_jiffies=0;


struct FTLCDC210fb {
   struct FTLCDC210 *FTLCDC210;
   struct fb_info *info;
   int (*set_frame_base)(struct FTLCDC210 *pftlcdc210, unsigned int value);

   /* for pip */
   void (*set_dimension)(struct FTLCDC210 *pftlcdc210, int x, int y);
   void (*set_position)(struct FTLCDC210 *pftlcdc210, int x, int y);
   void (*get_position)(struct FTLCDC210 *pftlcdc210, int *x, int *y);

   /* for pop */
   void (*set_popscale)(struct FTLCDC210 *pftlcdc210, int scale);
   int (*get_popscale)(struct FTLCDC210 *pftlcdc210);

   /*
    * This pseudo_palette is used _only_ by fbcon, thus
    * it only contains 16 entries to match the number of colors supported
    * by the console. The pseudo_palette is used only if the visual is
    * in directcolor or truecolor mode.  With other visuals, the
    * pseudo_palette is not used. (This might change in the future.)
    */
   u32 pseudo_palette[16];
};

/**
 * ftlcdc210_default_fix - Default struct fb_fix_screeninfo
 * It is only used in ftlcdc210_probe, so mark it as __devinitdata
 */
static struct fb_fix_screeninfo ftlcdc210_default_fix  = {
   .id      = "FTLCDC210",
   .type    = FB_TYPE_PACKED_PIXELS,
   .ypanstep   = 1,
   .accel      = FB_ACCEL_NONE,
};

/**
 * ftlcdc210_default_var - Default struct fb_var_screeninfo
 * It is only used in ftlcdc210_probe, so mark it as __devinitdata
 */

#ifdef CONFIG_AUO_A036QN01_CPLD
static struct fb_var_screeninfo ftlcdc210_default_var  = {
   .xres    = 320,
   .yres    = 240,
   .xres_virtual  = 320,
   .yres_virtual  = 240,
   .bits_per_pixel   = 16,
   .pixclock   = 171521,
   .left_margin   = 44,
   .right_margin  = 6,
   .upper_margin  = 11,
   .lower_margin  = 8,
   .hsync_len  = 21,
   .vsync_len  = 3,
   .vmode      = FB_VMODE_NONINTERLACED,
   .sync    = 0,
};
#define CONFIG_FTLCDC210_PIXEL_BGR
#endif

#ifdef CONFIG_TPO_TD070WGEC2
static struct fb_var_screeninfo ftlcdc210_default_var  = {
   .xres    = 1024,
   .yres    = 600,
   .xres_virtual  = 1024,
   .yres_virtual  = 600,
   .bits_per_pixel   = 32,
   .pixclock   = 41521,       // 60hz, 2010/06/07: a/v sync, happen early situation
   //.pixclock = 122363,         // 20hz
   //.pixclock = 81575,       // 30hz
   .left_margin   = 46,
   .right_margin  = 210,
   .upper_margin  = 23,
   .lower_margin  = 23,
   .hsync_len  = 40,
   .vsync_len  = 23,
   .vmode      = FB_VMODE_NONINTERLACED,
   .sync    = 0,
};
#define CONFIG_PANEL_TYPE_24BIT

//#define CONFIG_FTLCDC210_SERIAL
//#define CONFIG_FTLCDC210_PIXEL_BGR         // lmc83
#endif

#ifdef CONFIG_VESA_800x600_60Hz
static struct fb_var_screeninfo ftlcdc210_default_var  = {
   .xres    = 800,
   .yres    = 600,
   .xres_virtual  = 800,
   .yres_virtual  = 600,
   .bits_per_pixel   = 16,
   .pixclock = 25131,       // 60hz  ==> by calculation
   //.pixclock   = 223,         // 60hz: better display
   .left_margin   = 88,       // horizontal back
   .right_margin  = 40,       // front
   .upper_margin  = 23,       // vertical back
   .lower_margin  = 1,        // vertical front
   .hsync_len  = 128,
   .vsync_len  = 4,
   .vmode      = FB_VMODE_NONINTERLACED,
   .sync    = 0,
};
#define CONFIG_PANEL_TYPE_24BIT

#endif

#ifdef CONFIG_VESA_1024x768_60Hz
static struct fb_var_screeninfo ftlcdc210_default_var  = {
   .xres    = 1024,
   .yres    = 768,
   .xres_virtual  = 1024,
   .yres_virtual  = 768,
   .bits_per_pixel   = 16,
   .pixclock   = 15385,       // 60hz  ==> by calculation
   .left_margin   = 160,      // horizontal back
   .right_margin  = 24,       // front
   .upper_margin  = 29,       // vertical back
   .lower_margin  = 3,        // vertical front
   .hsync_len  = 136,
   .vsync_len  = 6,
   .vmode      = FB_VMODE_NONINTERLACED,
   .sync    = 0,
};
#define CONFIG_PANEL_TYPE_24BIT
#endif


int enable_fs=0;

/******************************************************************************
 * internal functions
 *****************************************************************************/
static int FTLCDC210_fb0_set_frame_base(struct FTLCDC210 *pftlcdc210,
      unsigned int value)
{
   iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_FRAME_BASE0);
   dev_dbg(pftlcdc210->dev, "  [FRAME BASE] = %08x\n", value);
   //printk("[%s]  [FRAME BASE] = %08x\n",__func__, value);
   fb0_frame_buf_phy_base = value;
   //FTLCDC210->active_fb = 0;
   active_info = pftlcdc210->fb[0]->info;
   return 1;
}

#if CONFIG_FTLCDC210_NR_FB > 1
static int FTLCDC210_fb1_set_frame_base(struct FTLCDC210 *pftlcdc210,
      unsigned int value)
{
   iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_FRAME_BASE1);
   dev_dbg(pftlcdc210->dev, "  [FRAME BASE] = %08x\n", value);
   //FTLCDC210->active_fb = 1;
   active_info = pftlcdc210->fb[1]->info;
   return 1;
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 2
static int FTLCDC210_fb2_set_frame_base(struct FTLCDC210 *pftlcdc210,
      unsigned int value)
{
   iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_FRAME_BASE2);
   dev_dbg(pftlcdc210->dev, "  [FRAME BASE] = %08x\n", value);
   //FTLCDC210->active_fb = 2;
   active_info = pftlcdc210->fb[2]->info;
   return 1;
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 3
static int FTLCDC210_fb3_set_frame_base(struct FTLCDC210 *pftlcdc210,
      unsigned int value)
{
   iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_FRAME_BASE3);
   dev_dbg(pftlcdc210->dev, "  [FRAME BASE] = %08x\n", value);
   //FTLCDC210->active_fb = 3;
   active_info = pftlcdc210->fb[3]->info;
   return 1;
}
#endif


#if CONFIG_FTLCDC210_NR_FB > 4
static int FTLCDC210_dont_set_frame_base(struct FTLCDC210 *pftlcdc210,
      unsigned int value)
{
   return 0;
}

static int FTLCDC210_fb4_set_frame_base(struct FTLCDC210 *pftlcdc210,
      unsigned int value)
{
   if (pftlcdc210->u_offset!=0)      // yuv420 mode
   {
      iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_FRAME_BASE0);
      iowrite32(value+pftlcdc210->u_offset, pftlcdc210->base + FTLCDC210_OFFSET_FRAME_BASE1);
      iowrite32(value+pftlcdc210->v_offset, pftlcdc210->base + FTLCDC210_OFFSET_FRAME_BASE2);
   }
   else
   {
      iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_FRAME_BASE0);
   }
   dev_dbg(pftlcdc210->dev, "  [FRAME BASE] = %08x\n", value);
   //FTLCDC210->active_fb = 4;
   active_info = pftlcdc210->fb[4]->info;
   return 1;
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 1
static void FTLCDC210_fb1_set_position(struct FTLCDC210 *pftlcdc210,
      int x, int y)
{
   unsigned int value;

   value = FTLCDC210_PIP_POS_H(x) | FTLCDC210_PIP_POS_V(y);
   dev_dbg(pftlcdc210->dev, "  [PIP POS1]   = %08x (%d, %d)\n", value, x, y);
   iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_PIP_POS1);
}

static void FTLCDC210_fb1_get_position(struct FTLCDC210 *pftlcdc210,
      int *x, int *y)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_PIP_POS1);
   dev_dbg(pftlcdc210->dev, "  [PIP POS1]   = %08x\n", reg);

   *x = FTLCDC210_PIP_POS_EXTRACT_H(reg);
   *y = FTLCDC210_PIP_POS_EXTRACT_V(reg);
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 2
static void FTLCDC210_fb2_set_position(struct FTLCDC210 *pftlcdc210,
      int x, int y)
{
   unsigned int value;

   value = FTLCDC210_PIP_POS_H(x) | FTLCDC210_PIP_POS_V(y);
   dev_dbg(pftlcdc210->dev, "  [PIP POS2]   = %08x (%d, %d)\n", value, x, y);
   iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_PIP_POS2);
}

static void FTLCDC210_fb2_get_position(struct FTLCDC210 *pftlcdc210,
      int *x, int *y)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_PIP_POS2);
   dev_dbg(pftlcdc210->dev, "  [PIP POS2]   = %08x\n", reg);

   *x = FTLCDC210_PIP_POS_EXTRACT_H(reg);
   *y = FTLCDC210_PIP_POS_EXTRACT_V(reg);
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 1
static void FTLCDC210_fb1_set_dimension(struct FTLCDC210 *pftlcdc210,
      int x, int y)
{
   unsigned int value;

   value = FTLCDC210_PIP_DIM_H(x) | FTLCDC210_PIP_DIM_V(y);
   dev_dbg(pftlcdc210->dev, "  [PIP DIM1]   = %08x\n", value);
   iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_PIP_DIM1);
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 2
static void FTLCDC210_fb2_set_dimension(struct FTLCDC210 *pftlcdc210,
      int x, int y)
{
   unsigned int value;

   value = FTLCDC210_PIP_DIM_H(x) | FTLCDC210_PIP_DIM_V(y);
   dev_dbg(pftlcdc210->dev, "  [PIP DIM2]   = %08x\n", value);
   iowrite32(value, pftlcdc210->base + FTLCDC210_OFFSET_PIP_DIM2);
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 3
static void FTLCDC210_fb0_set_popscale(struct FTLCDC210 *pftlcdc210, int scale)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
   reg &= ~FTLCDC210_POPSCALE_0_MASK;

   switch (scale) {
   case 0:
      break;
   case 1:
      reg |= FTLCDC210_POPSCALE_0_QUARTER;
      break;
   case 2:
      reg |= FTLCDC210_POPSCALE_0_HALF;
      break;
   default:
      BUG();
   }

   dev_dbg(pftlcdc210->dev, "  [POPSCALE]   = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
}

static int FTLCDC210_fb0_get_popscale(struct FTLCDC210 *pftlcdc210)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
   dev_dbg(pftlcdc210->dev, "  [POPSCALE]   = %08x\n", reg);

   reg &= FTLCDC210_POPSCALE_0_MASK;

   switch (reg) {
   case 0:
      return 0;

   case FTLCDC210_POPSCALE_0_QUARTER:
      return 1;

   case FTLCDC210_POPSCALE_0_HALF:
      return 2;

   default: /* impossible */
      BUG();
   }
}

static void FTLCDC210_fb1_set_popscale(struct FTLCDC210 *pftlcdc210, int scale)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
   reg &= ~FTLCDC210_POPSCALE_1_MASK;

   switch (scale) {
   case 0:
      break;
   case 1:
      reg |= FTLCDC210_POPSCALE_1_QUARTER;
      break;
   case 2:
      reg |= FTLCDC210_POPSCALE_1_HALF;
      break;
   default:
      BUG();
   }

   dev_dbg(pftlcdc210->dev, "  [POPSCALE]   = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
}

static int FTLCDC210_fb1_get_popscale(struct FTLCDC210 *pftlcdc210)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
   dev_dbg(pftlcdc210->dev, "  [POPSCALE]   = %08x\n", reg);

   reg &= FTLCDC210_POPSCALE_1_MASK;

   switch (reg) {
   case 0:
      return 0;

   case FTLCDC210_POPSCALE_1_QUARTER:
      return 1;

   case FTLCDC210_POPSCALE_1_HALF:
      return 2;

   default: /* impossible */
      BUG();
   }
}

static void FTLCDC210_fb2_set_popscale(struct FTLCDC210 *pftlcdc210, int scale)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
   reg &= ~FTLCDC210_POPSCALE_2_MASK;

   switch (scale) {
   case 0:
      break;
   case 1:
      reg |= FTLCDC210_POPSCALE_2_QUARTER;
      break;
   case 2:
      reg |= FTLCDC210_POPSCALE_2_HALF;
      break;
   default:
      BUG();
   }

   dev_dbg(pftlcdc210->dev, "  [POPSCALE]   = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
}

static int FTLCDC210_fb2_get_popscale(struct FTLCDC210 *pftlcdc210)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
   dev_dbg(pftlcdc210->dev, "  [POPSCALE]   = %08x\n", reg);

   reg &= FTLCDC210_POPSCALE_2_MASK;

   switch (reg) {
   case 0:
      return 0;

   case FTLCDC210_POPSCALE_2_QUARTER:
      return 1;

   case FTLCDC210_POPSCALE_2_HALF:
      return 2;

   default: /* impossible */
      BUG();
   }
}

static void FTLCDC210_fb3_set_popscale(struct FTLCDC210 *pftlcdc210, int scale)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
   reg &= ~FTLCDC210_POPSCALE_3_MASK;

   switch (scale) {
   case 0:
      break;
   case 1:
      reg |= FTLCDC210_POPSCALE_3_QUARTER;
      break;
   case 2:
      reg |= FTLCDC210_POPSCALE_3_HALF;
      break;
   default:
      BUG();
   }

   dev_dbg(pftlcdc210->dev, "  [POPSCALE]   = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
}

static int FTLCDC210_fb3_get_popscale(struct FTLCDC210 *pftlcdc210)
{
   unsigned int reg;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_POPSCALE);
   dev_dbg(pftlcdc210->dev, "  [POPSCALE]   = %08x\n", reg);

   reg &= FTLCDC210_POPSCALE_3_MASK;

   switch (reg) {
   case 0:
      return 0;

   case FTLCDC210_POPSCALE_3_QUARTER:
      return 1;

   case FTLCDC210_POPSCALE_3_HALF:
      return 2;

   default: /* impossible */
      BUG();
   }
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 4
static void FTLCDC210_fb4_set_popscale(struct FTLCDC210 *pftlcdc210, int scale)
{
}

static int FTLCDC210_fb4_get_popscale(struct FTLCDC210 *pftlcdc210)
{
   return 0;
}
#endif

static int FTLCDC210_grow_framebuffer(struct fb_info *info,
      struct fb_var_screeninfo *var)
{
   struct device *dev = info->device;
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   unsigned int reg;

   #if 0    // lmc83
      unsigned long smem_len = (var->xres_virtual * var->yres_virtual
             * DIV_ROUND_UP(var->bits_per_pixel, 8));
   #else
      unsigned long smem_len = PAGE_ALIGN(var->xres_virtual * var->yres_virtual
             * var->bits_per_pixel / 8);
   #endif
   unsigned long smem_start;
   void *screen_base;

   if (smem_len <= info->fix.smem_len) {
      /* current framebuffer is big enough */
      return 0;
   }
   #if defined(CACHE_FB)
      //printk("[%s] Enter!! PATH #if defined(CACHE_FB) !!\n",__func__);
      screen_base = __get_free_pages(GFP_DMA|GFP_KERNEL, get_order(smem_len));
      cur_dma_addr = smem_start = virt_to_phys(screen_base);
      cur_frame_size = smem_len;
   #else
      screen_base = dma_alloc_writecombine(dev, smem_len,   (dma_addr_t *)&smem_start, GFP_KERNEL | GFP_DMA); 
      //printk("[%s] Enter!! PATH dma_alloc_writecombine !! screen_base = 0x%x smem_start=0x%x\n",__func__,screen_base,smem_start);
      fb0_frame_buf_screen_base = screen_base;
   #endif


   if (!screen_base) {
      panic("failed to allocate frame buffer\n");
      return -ENOMEM;
   }

   memset(screen_base, 0, smem_len);

   reg = FTLCDC210_FRAME_BASE(smem_start);
   pftlcdc210fb->set_frame_base(pftlcdc210, reg);

   /*
    * Free current framebuffer (if any)
    */
   if (info->screen_base) {
      #if defined(CACHE_FB)
         free_pages(info->screen_base, get_order(info->fix.smem_len));
      #else
         dma_free_writecombine(dev, info->fix.smem_len, info->screen_base, (dma_addr_t )info->fix.smem_start);
      #endif
   }

   info->screen_base = screen_base; 
   info->fix.smem_start = smem_start;
   info->fix.smem_len = smem_len;

   return 0;
}

/******************************************************************************
 * internal functions - gamma
 *
 * Note the brain-dead hardware behavoirs:
 * After reset, hardware does not initialize gamma tables to linear -
 * they are just garbages.
 * Gamma tables must be programmed while LCD disabled.
 *****************************************************************************/
static void FTLCDC210_set_gamma(void *gtbase, unsigned int i, unsigned int val)
{
   unsigned int reg;
   unsigned int offset = i & ~0x3;
   unsigned int shift = (i % 4) * 8;
   unsigned int mask = 0xff << shift;

   val = val > 0xff ? 0xff : val;
   val <<= shift;

   reg = ioread32(gtbase + offset);
   reg &= ~mask;
   reg |= val;
   iowrite32(reg, gtbase + offset);
}

static void FTLCDC210_set_gamma_red(void *base, unsigned int i, unsigned int val)
{
   FTLCDC210_set_gamma(base + FTLCDC210_OFFSET_GAMMA_R, i, val);
}

static void FTLCDC210_set_gamma_green(void *base, unsigned int i, unsigned int val)
{
   FTLCDC210_set_gamma(base + FTLCDC210_OFFSET_GAMMA_G, i, val);
}

static void FTLCDC210_set_gamma_blue(void *base, unsigned int i, unsigned int val)
{
   FTLCDC210_set_gamma(base + FTLCDC210_OFFSET_GAMMA_B, i, val);
}

/**
 * ftlcdc210_set_linear_gamma - Setup linear gamma tables
 */
static void ftlcdc210_set_linear_gamma(struct FTLCDC210 *pftlcdc210)
{
   int i;

   for (i = 0; i < 256; i++) {
      FTLCDC210_set_gamma_red(pftlcdc210->base, i, i);
      FTLCDC210_set_gamma_green(pftlcdc210->base, i, i);
      FTLCDC210_set_gamma_blue(pftlcdc210->base, i, i);
   }
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftlcdc210_interrupt(int irq, void *dev_id)
{
   struct FTLCDC210 *pftlcdc210 = dev_id;
   unsigned int status;

   status = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_INT_STATUS);

   if (status & FTLCDC210_INT_UNDERRUN) {
      #ifdef not_complete_yet
      if (printk_ratelimit())
         dev_notice(pftlcdc210->dev, "underrun\n");
      #endif /* end_of_not */
   }

   if (status & FTLCDC210_INT_NEXT_BASE) {
      #ifdef not_complete_yet
      if (printk_ratelimit())
         dev_dbg(pftlcdc210->dev, "frame base updated\n");
      #endif /* end_of_not */

      pftlcdc210->nb = 1;
      wake_up(&pftlcdc210->wait_nb);
   }

   if (status & FTLCDC210_INT_VSTATUS) {
      #ifdef not_complete_yet
      if (printk_ratelimit())
         dev_dbg(pftlcdc210->dev, "vertical duration reached \n");
      #endif /* end_of_not */
   }

   if (status & FTLCDC210_INT_BUS_ERROR) {
      #ifdef not_complete_yet
      if (printk_ratelimit())
         dev_err(pftlcdc210->dev, "bus error!\n");
      #endif /* end_of_not */
   }

   iowrite32(status, pftlcdc210->base + FTLCDC210_OFFSET_INT_CLEAR);

   return IRQ_HANDLED;
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
/**
 * fb_check_var - Validates a var passed in.
 * @var: frame buffer variable screen structure
 * @info: frame buffer structure that represents a single frame buffer 
 *
 * Checks to see if the hardware supports the state requested by
 * var passed in. This function does not alter the hardware state!!! 
 * This means the data stored in struct fb_info and struct FTLCDC210 do 
 * not change. This includes the var inside of struct fb_info. 
 * Do NOT change these. This function can be called on its own if we
 * intent to only test a mode and not actually set it. The stuff in 
 * modedb.c is a example of this. If the var passed in is slightly 
 * off by what the hardware can support then we alter the var PASSED in
 * to what we can do.
 *
 * For values that are off, this function must round them _up_ to the
 * next value that is supported by the hardware.  If the value is
 * greater than the highest value supported by the hardware, then this
 * function must return -EINVAL.
 *
 * Exception to the above rule:  Some drivers have a fixed mode, ie,
 * the hardware is already set at boot up, and cannot be changed.  In
 * this case, it is more acceptable that this function just return
 * a copy of the currently working var (info->var). Better is to not
 * implement this function, as the upper layer will do the copying
 * of the current var for you.
 *
 * Note:  This is the only function where the contents of var can be
 * freely adjusted after the driver has been registered. If you find
 * that you have code outside of this function that alters the content
 * of var, then you are doing something wrong.  Note also that the
 * contents of info->var must be left untouched at all times after
 * driver registration.
 *
 * Returns negative errno on error, or zero on success.
 */
static int FTLCDC210_fb0_check_var(struct fb_var_screeninfo *var,
      struct fb_info *info)
{
   struct device *dev = info->device;
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   unsigned long clk_value_khz = pftlcdc210->clk_value_khz;
   int ret;

   dev_dbg(dev, "fb%d: %s:\n", info->node, __func__);

   if (var->pixclock == 0) {
      dev_err(dev, "pixclock not specified\n");
      return -EINVAL;
   }

   dev_dbg(dev, "  resolution: %ux%u (%ux%u virtual)\n",
      var->xres, var->yres,
      var->xres_virtual, var->yres_virtual);
   dev_dbg(dev, "  pixclk:       %lu KHz\n", PICOS2KHZ(var->pixclock));
   dev_dbg(dev, "  bpp:          %u\n", var->bits_per_pixel);
   dev_dbg(dev, "  clk:          %lu KHz\n", clk_value_khz);
   dev_dbg(dev, "  left  margin: %u\n", var->left_margin);
   dev_dbg(dev, "  right margin: %u\n", var->right_margin);
   dev_dbg(dev, "  upper margin: %u\n", var->upper_margin);
   dev_dbg(dev, "  lower margin: %u\n", var->lower_margin);
   dev_dbg(dev, "  hsync:        %u\n", var->hsync_len);
   dev_dbg(dev, "  vsync:        %u\n", var->vsync_len);

   printk("  pixclk:       %lu KHz\n", PICOS2KHZ(var->pixclock));
   printk("  bpp:          %u\n", var->bits_per_pixel);
   printk("  clk:          %lu KHz\n", clk_value_khz);
   

   if (PICOS2KHZ(var->pixclock) > clk_value_khz) {
      dev_err(dev, "%lu KHz pixel clock is too fast\n",
         PICOS2KHZ(var->pixclock));
      return -EINVAL;
   }

   if (var->xres != info->var.xres)
      return -EINVAL;

   if (var->yres != info->var.yres)
      return -EINVAL;

   if (var->xres_virtual != info->var.xres_virtual)
      return -EINVAL;

   if (var->xres_virtual != info->var.xres)
      return -EINVAL;

   if (var->yres_virtual < info->var.yres)
      return -EINVAL;

   ret = FTLCDC210_grow_framebuffer(info, var);
   if (ret)
      return ret;

   switch (var->bits_per_pixel) {
   case 1: case 2: case 4: case 8:
      var->red.offset = var->green.offset = var->blue.offset = 0;
      var->red.length = var->green.length = var->blue.length
            = var->bits_per_pixel;
      break;

   case 16: /* RGB:565 mode */
      #if defined(CONFIG_FTLCDC210_PIXEL_BGR)
         var->red.offset      = 0;
         var->green.offset = 5;
         var->blue.offset  = 11;

         var->red.length      = 5;
         var->green.length = 6;
         var->blue.length  = 5;
         var->transp.length   = 0;
      #else    // RGB mode
         var->red.offset      = 11;
         var->green.offset = 5;
         var->blue.offset  = 0;

         var->red.length      = 5;
         var->green.length = 6;
         var->blue.length  = 5;
         var->transp.length   = 0;
      #endif
      break;

   case 32: /* RGB:888 mode */
      var->red.offset      = 16;
      var->green.offset = 8;
      var->blue.offset  = 0;
      var->transp.offset   = 24;

      var->red.length = var->green.length = var->blue.length = 8;
      var->transp.length   = 8;
      break;

   default:
      dev_err(dev, "color depth %d not supported\n",
         var->bits_per_pixel);
      return -EINVAL;
   }

   return 0;
}

#if CONFIG_FTLCDC210_NR_FB > 1
static int FTLCDC210_fb1_check_var(struct fb_var_screeninfo *var,
      struct fb_info *info)
{
   struct device *dev = info->device;
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   int ret;

   dev_dbg(dev, "fb%d: %s:\n", info->node, __func__);
   dev_dbg(dev, "  resolution: %ux%u (%ux%u virtual)\n",
      var->xres, var->yres,
      var->xres_virtual, var->yres_virtual);
   dev_dbg(dev, "  bpp:          %u\n", var->bits_per_pixel);

   /*
    * The resolution of sub image should not be larger than the physical
    * resolution (the resolution of fb[0]).
    */
   if (var->xres > pftlcdc210->fb[0]->info->var.xres)
      return -EINVAL;

   if (var->yres > pftlcdc210->fb[0]->info->var.yres)
      return -EINVAL;

   if (var->xres_virtual != var->xres)
      return -EINVAL;

   if (var->yres_virtual < var->yres)
      return -EINVAL;

   ret = FTLCDC210_grow_framebuffer(info, var);
   if (ret)
      return ret;

   switch (var->bits_per_pixel) {
   case 1: case 2: case 4: case 8:
      var->red.offset = var->green.offset = var->blue.offset = 0;
      var->red.length = var->green.length = var->blue.length
            = var->bits_per_pixel;
      break;

   case 16: /* RGB:565 mode */
      var->red.offset      = 11;
      var->green.offset = 5;
      var->blue.offset  = 0;

      var->red.length      = 5;
      var->green.length = 6;
      var->blue.length  = 5;
      var->transp.length   = 0;
      break;

   case 32: /* RGB:888 mode */
      var->red.offset      = 16;
      var->green.offset = 8;
      var->blue.offset  = 0;
      var->transp.offset   = 24;

      var->red.length = var->green.length = var->blue.length = 8;
      var->transp.length   = 8;
      break;

   default:
      dev_err(dev, "color depth %d not supported\n",
         var->bits_per_pixel);
      return -EINVAL;
   }

   return 0;
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 4
static int FTLCDC210_fb4_check_var(struct fb_var_screeninfo *var,
      struct fb_info *info)
{
   struct device *dev = info->device;
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   int ret;

   dev_dbg(dev, "fb%d: %s:\n", info->node, __func__);
   dev_dbg(dev, "  resolution: %ux%u (%ux%u virtual)\n",
      var->xres, var->yres,
      var->xres_virtual, var->yres_virtual);
   dev_dbg(dev, "  bpp:          %u\n", var->bits_per_pixel);

   /*
    * We use second stage scaler only.
    * According to the limitation of second stage scaler,
    * the resolution for zooming should be between half or twice
    * the physical resolution (the resolution of fb[0]).
    * Sucks!
    */
   if (var->xres * 2 < pftlcdc210->fb[0]->info->var.xres)
      return -EINVAL;

   if (var->xres > pftlcdc210->fb[0]->info->var.xres * 2)
      return -EINVAL;

   if (var->yres * 2 < pftlcdc210->fb[0]->info->var.yres)
      return -EINVAL;

   if (var->yres > pftlcdc210->fb[0]->info->var.yres * 2)
      return -EINVAL;

   if (var->xres_virtual != var->xres)
      return -EINVAL;

   if (var->yres_virtual < var->yres)
      return -EINVAL;

   

   ret = FTLCDC210_grow_framebuffer(info, var);
   if (ret)
      return ret;

   return 0;
   
   switch (var->bits_per_pixel) {
   case 1: case 2: case 4: case 8:
      var->red.offset = var->green.offset = var->blue.offset = 0;
      var->red.length = var->green.length = var->blue.length
            = var->bits_per_pixel;
      break;

   case 16: /* RGB:565 mode */
      var->red.offset      = 11;
      var->green.offset = 5;
      var->blue.offset  = 0;

      var->red.length      = 5;
      var->green.length = 6;
      var->blue.length  = 5;
      var->transp.length   = 0;
      break;

   case 32: /* RGB:888 mode */
      var->red.offset      = 16;
      var->green.offset = 8;
      var->blue.offset  = 0;
      var->transp.offset   = 24;

      var->red.length = var->green.length = var->blue.length = 8;
      var->transp.length   = 8;
      break;

   case 12: /* lmc83: YCbCr420 mode */
      break;

   default:
      dev_err(dev, "color depth %d not supported\n",
         var->bits_per_pixel);
      return -EINVAL;
   }

   return 0;
}
#endif

/**
 * fb_set_par - Alters the hardware state.
 * @info: frame buffer structure that represents a single frame buffer
 *
 * Using the fb_var_screeninfo in fb_info we set the resolution of the
 * this particular framebuffer. This function alters the par AND the
 * fb_fix_screeninfo stored in fb_info. It does not alter var in 
 * fb_info since we are using that data. This means we depend on the
 * data in var inside fb_info to be supported by the hardware. 
 *
 * This function is also used to recover/restore the hardware to a
 * known working state.
 *
 * fb_check_var is always called before fb_set_par to ensure that
 * the contents of var is always valid.
 *
 * Again if you can't change the resolution you don't need this function.
 *
 * However, even if your hardware does not support mode changing,
 * a set_par might be needed to at least initialize the hardware to
 * a known working state, especially if it came back from another
 * process that also modifies the same hardware, such as X.
 *
 * Returns negative errno on error, or zero on success.
 */
static int FTLCDC210_fb0_set_par(struct fb_info *info)
{
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   unsigned long clk_value_khz = pftlcdc210->clk_value_khz;
   unsigned int divno;
   unsigned int reg;

   dev_dbg(info->device, "fb%d: %s:\n", info->node, __func__);
   dev_dbg(info->device, "  resolution:     %ux%u (%ux%u virtual)\n",
      info->var.xres, info->var.yres,
      info->var.xres_virtual, info->var.yres_virtual);

   /*
    * Fill uninitialized fields of struct fb_fix_screeninfo
    */
   if (info->var.bits_per_pixel == 1)
      info->fix.visual = FB_VISUAL_MONO01;
   else if (info->var.bits_per_pixel <= 8)
      info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
   else
      info->fix.visual = FB_VISUAL_TRUECOLOR;

   #if 0 // lmc83
      info->fix.line_length = info->var.xres_virtual *
            DIV_ROUND_UP(info->var.bits_per_pixel, 8);
   #else
      info->fix.line_length = info->var.xres_virtual * info->var.bits_per_pixel / 8;
   #endif


   /*
    * polarity control
    */
   //divno = DIV_ROUND_UP(clk_value_khz, PICOS2KHZ(info->var.pixclock));
   divno = clk_value_khz/PICOS2KHZ(info->var.pixclock);     // round down
   printk("div no %d\n",divno);
   if (divno == 0) {
      dev_err(info->device,
         "pixel clock(%lu kHz) > bus clock(%lu kHz)\n",
         PICOS2KHZ(info->var.pixclock), clk_value_khz);
      return -EINVAL;
   }

   clk_value_khz = DIV_ROUND_UP(clk_value_khz, divno);
   info->var.pixclock = KHZ2PICOS(clk_value_khz)+1;
   dev_dbg(info->device, "  updated pixclk: %lu KHz (divno = %d)\n",
      clk_value_khz, divno - 1);

   printk("  updated pixclk: %lu KHz (divno = %d)\n",
      clk_value_khz, divno - 1);
   dev_dbg(info->device, "  frame rate:     %lu Hz\n",
      clk_value_khz * 1000
      / (info->var.xres + info->var.left_margin
         + info->var.right_margin + info->var.hsync_len)
      / (info->var.yres + info->var.upper_margin
         + info->var.lower_margin + info->var.vsync_len));

   reg = FTLCDC210_POLARITY_ICK
       | FTLCDC210_POLARITY_DIVNO(divno - 1);

   if ((info->var.sync & FB_SYNC_HOR_HIGH_ACT) == 0)
      reg |= FTLCDC210_POLARITY_IHS;

   if ((info->var.sync & FB_SYNC_VERT_HIGH_ACT) == 0)
      reg |= FTLCDC210_POLARITY_IVS;

   dev_dbg(info->device, "  [POLARITY]   = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_POLARITY);

   /*
    * horizontal timing control
    */
   reg = FTLCDC210_HTIMING_PL(info->var.xres / 16 - 1);
   reg |= FTLCDC210_HTIMING_HW(info->var.hsync_len - 1);
   reg |= FTLCDC210_HTIMING_HFP(info->var.right_margin - 1);
   reg |= FTLCDC210_HTIMING_HBP(info->var.left_margin - 1);

   dev_dbg(info->device, "  [HTIMING]    = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_HTIMING);

   /*
    * vertical timing control
    */
   reg = FTLCDC210_VTIMING0_LF(info->var.yres - 1);
   reg |= FTLCDC210_VTIMING0_VW(info->var.vsync_len - 1);
   reg |= FTLCDC210_VTIMING0_VFP(info->var.lower_margin);

   dev_dbg(info->device, "  [VTIMING0]   = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_VTIMING0);

   reg = FTLCDC210_VTIMING1_VBP(info->var.upper_margin);

   dev_dbg(info->device, "  [VTIMING1]   = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_VTIMING1);

   /*
    * Panel Pixel
    */
   reg = FTLCDC210_PIXEL_LEB_LEP;
#ifdef CONFIG_FTLCDC210_PIXEL_BGR
   reg |= FTLCDC210_PIXEL_BGR;
#endif

#ifdef CONFIG_PANEL_TYPE_24BIT
   reg |= FTLCDC210_PIXEL_24BIT_PANEL;
#endif

   switch (info->var.bits_per_pixel) {
      case 1:
         reg |= FTLCDC210_PIXEL_BPP1;
         break;

      case 2:
         reg |= FTLCDC210_PIXEL_BPP2;
         break;

      case 4:
         reg |= FTLCDC210_PIXEL_BPP4;
         break;

      case 8:
         reg |= FTLCDC210_PIXEL_BPP8;
         break;

      case 16:
         reg |= FTLCDC210_PIXEL_BPP16;
         break;

      case 32:
         reg |= FTLCDC210_PIXEL_BPP24;
         break;

      default:
         BUG();
         break;
   }

   dev_info(info->device, "  [PIXEL]      = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_PIXEL);

   /*
    * Serial Panel Pixel
    */
   reg = 0;
#ifdef CONFIG_FTLCDC210_SERIAL
   reg |= FTLCDC210_SERIAL_SERIAL;
#endif

   dev_dbg(info->device, "  [SERIAL]     = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_SERIAL);

   /*
    * Function Enable
    */
   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_CTRL);
   reg |= FTLCDC210_CTRL_ENABLE | FTLCDC210_CTRL_LCD;

   if (pftlcdc210->scalar_on==0)
   {
      reg &= ~FTLCDC210_CTRL_SCALAR;
   }

   dev_dbg(info->device, "  [CTRL]       = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

   return 0;
}

#if CONFIG_FTLCDC210_NR_FB > 1
static int FTLCDC210_fb1_set_par(struct fb_info *info)
{
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;

   dev_dbg(info->device, "fb%d: %s:\n", info->node, __func__);
   dev_dbg(info->device, "  resolution:     %ux%u (%ux%u virtual)\n",
      info->var.xres, info->var.yres,
      info->var.xres_virtual, info->var.yres_virtual);

   /*
    * Fill uninitialized fields of struct fb_fix_screeninfo
    */
   if (info->var.bits_per_pixel == 1)
      info->fix.visual = FB_VISUAL_MONO01;
   else if (info->var.bits_per_pixel <= 8)
      info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
   else
      info->fix.visual = FB_VISUAL_TRUECOLOR;

   #if 0 // lmc83
      info->fix.line_length = info->var.xres_virtual *
            DIV_ROUND_UP(info->var.bits_per_pixel, 8);
   #else
      info->fix.line_length = info->var.xres_virtual * info->var.bits_per_pixel / 8;
   #endif

   /*
    * Dimension
    */
   pftlcdc210fb->set_dimension(pftlcdc210, info->var.xres, info->var.yres);

   return 0;
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 3
static int FTLCDC210_fb3_set_par(struct fb_info *info)
{
   dev_dbg(info->device, "fb%d: %s:\n", info->node, __func__);
   dev_dbg(info->device, "  resolution:     %ux%u (%ux%u virtual)\n",
      info->var.xres, info->var.yres,
      info->var.xres_virtual, info->var.yres_virtual);

   /*
    * Fill uninitialized fields of struct fb_fix_screeninfo
    */
   if (info->var.bits_per_pixel == 1)
      info->fix.visual = FB_VISUAL_MONO01;
   else if (info->var.bits_per_pixel <= 8)
      info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
   else
      info->fix.visual = FB_VISUAL_TRUECOLOR;

   #if 0 // lmc83
      info->fix.line_length = info->var.xres_virtual *
            DIV_ROUND_UP(info->var.bits_per_pixel, 8);
   #else
      info->fix.line_length = info->var.xres_virtual * info->var.bits_per_pixel / 8;
   #endif

   return 0;
}
#endif


void FTLCDC210_scalar_setup(struct fb_info *info)
{
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   unsigned int xsrc, ysrc;
   unsigned int xdst, ydst;
   unsigned int reg;
   unsigned int hcoef, vcoef;

   xsrc = info->var.xres;
   ysrc = info->var.yres;
   xdst = pftlcdc210->fb[0]->info->var.xres;
   ydst = pftlcdc210->fb[0]->info->var.yres;

   reg = (xsrc - 1) & FTLCDC210_SCALE_IN_HRES_MASK;
   dev_dbg(pftlcdc210->dev, "  [SCALE INH]  = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_IN_HRES);

   reg = (ysrc - 1) & FTLCDC210_SCALE_IN_VRES_MASK;
   dev_dbg(pftlcdc210->dev, "  [SCALE INV]  = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_IN_VRES);

   reg = xdst & FTLCDC210_SCALE_OUT_HRES_MASK;
   dev_dbg(pftlcdc210->dev, "  [SCALE OUTH] = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_OUT_HRES);

   reg = ydst & FTLCDC210_SCALE_OUT_VRES_MASK;
   dev_dbg(pftlcdc210->dev, "  [SCALE OUTV] = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_OUT_VRES);

   /*
    * Stupid hardware design.
    * Since all the information is available to hardware,
    * why bother to set up this register by software?
    */
   if (xdst >= xsrc) {
      hcoef = xsrc * 256 / xdst;
   } else {
      hcoef = xsrc % xdst * 256 / xdst;
   }

   if (ydst >= ysrc) {
      vcoef = ysrc * 256 / ydst;
   } else {
      vcoef = ysrc % ydst * 256 / ydst;
   }

   reg = FTLCDC210_SCALE_PAR(hcoef, vcoef);
   dev_dbg(pftlcdc210->dev, "  [SCALE PAR]  = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_PAR);

   /*
    * We use second stage scaler only.
    */
   reg = 0;
   dev_dbg(pftlcdc210->dev, "  [SCALE CTRL] = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_CTRL);
}


#if CONFIG_FTLCDC210_NR_FB > 4
static int FTLCDC210_fb4_set_par(struct fb_info *info)
{
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   unsigned int reg;

   dev_dbg(info->device, "fb%d: %s:\n", info->node, __func__);
   dev_dbg(info->device, "  resolution:     %ux%u (%ux%u virtual)\n",
      info->var.xres, info->var.yres,
      info->var.xres_virtual, info->var.yres_virtual);

   /*
    * Fill uninitialized fields of struct fb_fix_screeninfo
    */
   if (info->var.bits_per_pixel == 1)
   {
      info->fix.visual = FB_VISUAL_MONO01;
   }
   else if (info->var.bits_per_pixel <= 8)
   {
      info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
   }
   else if (info->var.bits_per_pixel == 12)
   {
      // info->fix.visual = FB_VISUAL_PSEUDOCOLOR; ???
      FTLCDC210->u_offset = info->var.xres * info->var.yres;
      FTLCDC210->v_offset = info->var.xres * info->var.yres * 5 / 4;
   }  
   else
   {
      info->fix.visual = FB_VISUAL_TRUECOLOR;
   }

   #if 0    // lmc83
      info->fix.line_length = info->var.xres_virtual *
            DIV_ROUND_UP(info->var.bits_per_pixel, 8);
   #else
      info->fix.line_length = info->var.xres_virtual * info->var.bits_per_pixel / 8;
   #endif

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_CTRL);
   if (pftlcdc210->scalar_on==1)
   {
      if ( (reg & FTLCDC210_CTRL_SCALAR)==0 )      // not scalar_on yet
      {
         reg &= ~(FTLCDC210_CTRL_ENABLE | FTLCDC210_CTRL_LCD);    // disable first
         iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_CTRL);
         
         FTLCDC210_scalar_setup(info);
         reg |= (FTLCDC210_CTRL_SCALAR | FTLCDC210_CTRL_ENABLE | FTLCDC210_CTRL_LCD);
         iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_CTRL); // enable
      }
   }
   else
   {
      reg &= ~FTLCDC210_CTRL_SCALAR;
      iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_CTRL);
   }

   return 0;
}
#endif

/**
 * fb_setcolreg - Optional function. Sets a color register.
 * @regno: Which register in the CLUT we are programming 
 * @red: The red value which can be up to 16 bits wide 
 * @green: The green value which can be up to 16 bits wide 
 * @blue:  The blue value which can be up to 16 bits wide.
 * @transp: If supported, the alpha value which can be up to 16 bits wide.
 * @info: frame buffer info structure
 * 
 * Set a single color register. The values supplied have a 16 bit
 * magnitude which needs to be scaled in this function for the hardware. 
 * Things to take into consideration are how many color registers, if
 * any, are supported with the current color visual. With truecolor mode
 * no color palettes are supported. Here a pseudo palette is created
 * which we store the value in pseudo_palette in struct fb_info. For
 * pseudocolor mode we have a limited color palette. To deal with this
 * we can program what color is displayed for a particular pixel value.
 * DirectColor is similar in that we can program each color field. If
 * we have a static colormap we don't need to implement this function. 
 * 
 * Returns negative errno on error, or zero on success.
 */
static int FTLCDC210_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
      unsigned blue, unsigned transp, struct fb_info *info)
{
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   u32 val;

   dev_dbg(info->device, "fb%d: %s(%d, %d, %d, %d, %d)\n",
      info->node, __func__, regno, red, green, blue, transp);

   if (regno >= 256)  /* no. of hw registers */
      return -EINVAL;

   /*
    * If grayscale is true, then we convert the RGB value
    * to grayscale no mater what visual we are using.
    */
   if (info->var.grayscale) {
      /* grayscale = 0.30*R + 0.59*G + 0.11*B */
      red = green = blue = (red * 77 + green * 151 + blue * 28) >> 8;
   }

#define CNVT_TOHW(val,width) ((((val) << (width)) + 0x7FFF - (val)) >> 16)
   red = CNVT_TOHW(red, info->var.red.length);
   green = CNVT_TOHW(green, info->var.green.length);
   blue = CNVT_TOHW(blue, info->var.blue.length);
   transp = CNVT_TOHW(transp, info->var.transp.length);
#undef CNVT_TOHW

   switch (info->fix.visual) {
   case FB_VISUAL_TRUECOLOR:
      if (regno >= 16)
         return -EINVAL;
      /*
       * The contents of the pseudo_palette is in raw pixel format.
       * Ie, each entry can be written directly to the framebuffer
       * without any conversion.
       */
   
      val = (red << info->var.red.offset);
      val |= (green << info->var.green.offset);
      val |= (blue << info->var.blue.offset);

      pftlcdc210fb->pseudo_palette[regno] = val;

      break;

   case FB_VISUAL_STATIC_PSEUDOCOLOR:
   case FB_VISUAL_PSEUDOCOLOR:
      /* TODO set palette registers */
      break;

   default:
      return -EINVAL;
   }

   return 0;
}

extern void do_notify_snapshot(struct fb_info *info);
/**
 * fb_pan_display - NOT a required function. Pans the display.
 * @var: frame buffer variable screen structure
 * @info: frame buffer structure that represents a single frame buffer
 *
 * Pan (or wrap, depending on the `vmode' field) the display using the
 * `xoffset' and `yoffset' fields of the `var' structure.
 * If the values don't fit, return -EINVAL.
 *
 * Returns negative errno on error, or zero on success.
 */
static int FTLCDC210_fb_pan_display(struct fb_var_screeninfo *var,
      struct fb_info *info)
{
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   unsigned long dma_addr;
   unsigned int value;

   dev_dbg(info->device, "fb%d: %s\n", info->node, __func__);
   //printk("[%s] fb%d: Enter!!\n",__func__,info->node);

   dma_addr = info->fix.smem_start + var->yoffset * info->fix.line_length;
   value = FTLCDC210_FRAME_BASE(dma_addr);
   ++display_cnt;

   pftlcdc210->nb = 0;
   #if defined(CACHE_FB)
   {
      unsigned int frame_size = info->fix.line_length * info->var.yres;
      
      cur_dma_addr = dma_addr;
      cur_frame_size = frame_size;
      flush_countdown = FLUSH_CNTDOWN;
      
      //printk("[%s] fb%d: Start DMA!!\n",__func__,info->node);
      
      dma_sync_single_for_device(NULL, dma_addr, frame_size, DMA_TO_DEVICE);
   }
   #endif
   if (pftlcdc210fb->set_frame_base(pftlcdc210, value)==1)
   {  
      //do_notify_snapshot(info); only for the driver: fa_drv/snapshot
      wait_event_timeout(pftlcdc210->wait_nb, pftlcdc210->nb, HZ / 10);
   }
   
   return 0;
}


int faradayfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    unsigned int size;

    //printk("[%s] Enter!!\n",__func__);

    size = vma->vm_end - vma->vm_start;
    if (size>info->fix.smem_len)
    {
        return -EFAULT;
    }
    vma->vm_pgoff = info->fix.smem_start >> PAGE_SHIFT;
    //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot))
       return -EAGAIN;
   return 0;
}


ssize_t faradayfb_read(struct fb_info *info, char __user *buf, size_t count, loff_t *ppos)
{
   unsigned long p = *ppos;
   void *src;
   int err = 0;
   unsigned long total_size;

   //printk("[%s] Enter!!\n",__func__);

   // got real fb we want (depend on full screen mode/non-full screen mode)
   info = active_info;
   
   if (info->state != FBINFO_STATE_RUNNING)
      return -EPERM;

   total_size = info->screen_size;
    if (total_size == 0)
      total_size = info->fix.smem_len;

    if (p >= total_size)
      return 0;

    if (count >= total_size)
      count = total_size;

    if (count + p > total_size)
            count = total_size - p;

    src = (void __force *)(info->screen_base + p);

    if (copy_to_user(buf, src, count))
      err = -EFAULT;

    if  (!err)
        *ppos += count;

    return (err) ? err : count;
}

static ssize_t faradayfb_write(struct fb_info *info, const char __user *buf,
             size_t count, loff_t *ppos)
{
      //printk("[%s] Enter!! count = 0x%x\n",__func__,count);  
      return  count;    
}

static int faradayfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
   int   ret = 0;
   struct fb_image_user_memcpy *p; 
   unsigned char* src_buf;
   dma_addr_t  dma_src_buf = 0;
   dma_addr_t  dma_dst_buf = 0;

   switch (cmd) 
   {
      case FTLCDC_GET_SCREEN_MODE:
         if (copy_to_user((unsigned int *)arg, &enable_fs, sizeof(unsigned int))) 
         {
            ret = -EFAULT;
         }
         break;
 
      case FBTESTIMGBLT:
      
         p = kmalloc(sizeof(struct fb_image_user_memcpy),GFP_KERNEL);
         if (!p)
         {
            ret = -EFAULT;
            goto error_imgblt;
         }
         if (copy_from_user(p,(struct fb_image_user_memcpy *) arg, sizeof(struct fb_image_user_memcpy))) 
         {
            ret = -EFAULT;
            goto error_imgblt;
         }     
        
         //CPU PATH
         /*src_buf = kmalloc((p->size*p->height), GFP_KERNEL); 
         if (!src_buf)
         {
            printk("[%s] kmalloc src_buf FAIL!!\n",__func__);
            ret = -EFAULT;
            break;
         }*/
         
         //DMA PATH
         src_buf = dma_alloc_writecombine(info->device, (p->size*p->height),   (dma_addr_t *)&dma_src_buf, GFP_KERNEL | GFP_DMA); 
         if (!src_buf)
         {
            ret = -EFAULT;
            goto error_imgblt;
         }
         
         if (copy_from_user(src_buf,p->src, (p->size*p->height))) 
         {
            ret = -EFAULT;
            goto error_imgblt;
         }
                 
         //CPU PATH: memcpy
         /*dst_buf = fb0_frame_buf_screen_base + p->dst_offset; 
         printk("[%s] src_buf=0x%x dst_buf=0x%x!!\n",__func__,src_buf,dst_buf); 
         
         for (i=0;i<p->height;i++)
         {
            src_buf = src_buf + p->src_inc;
            dst_buf = dst_buf + p->dst_inc;
            memcpy(dst_buf, src_buf, p->size);
         }*/
         
         //DMA PATH: 
         dma_dst_buf = fb0_frame_buf_phy_base + p->dst_offset;
         ftlcdc210_DMA_imgblt(info->device,dma_src_buf,dma_dst_buf,p->size,p->height,p->src_inc,p->dst_inc);

error_imgblt:                  
  
         //CPU PATH
         //kfree(src_buf);
         
         //DMA PATH
         dma_free_writecombine(info->device, (p->size*p->height), src_buf, dma_src_buf);    

         kfree(p);
      
         break; 
         
      case FBTESTFILLREC:
      
         p = kmalloc(sizeof(struct fb_image_user_memcpy),GFP_KERNEL);
         if (!p)
         {
            ret = -EFAULT;
            goto error_fillrec;
         }
         if (copy_from_user(p,(struct fb_image_user_memcpy *) arg, sizeof(struct fb_image_user_memcpy))) 
         {
            ret = -EFAULT;
            goto error_fillrec;
         }     
                         
         //CPU PATH: memcpy
         //dst_buf = fb0_frame_buf_screen_base + p->dst_inc; 
         //memcpy(dst_buf, src_buf, p->size);

         //DMA PATH: 
         dma_dst_buf = (fb0_frame_buf_phy_base + p->dst_offset);
         ftlcdc210_DMA_fillrec(info->device,dma_dst_buf,p->size,p->height,p->dst_inc,p->value);

error_fillrec:         
         kfree(p);
         break;
         
      case FBTESTCOPYAREA:

         p = kmalloc(sizeof(struct fb_image_user_memcpy),GFP_KERNEL);
         if (!p)
         {
            ret = -EFAULT;
            goto error_copyarea;
         }
         if (copy_from_user(p,(struct fb_image_user_memcpy *) arg, sizeof(struct fb_image_user_memcpy))) 
         {
            ret = -EFAULT;
            goto error_copyarea;
         }     
               
         //CPU PATH: memcpy
         /*dst_buf = fb0_frame_buf_screen_base + p->dst_offset;          
         for (i=0;i<p->height;i++)
         {
            src_buf = src_buf + p->src_inc;
            dst_buf = dst_buf + p->dst_inc;
            memcpy(dst_buf, src_buf, p->size);
         }*/
         
         //DMA PATH: first test fillrec
         dma_src_buf = fb0_frame_buf_phy_base + p->src_offset;
         dma_dst_buf = fb0_frame_buf_phy_base + p->dst_offset;
         ftlcdc210_DMA_copyarea(info->device,dma_src_buf,dma_dst_buf,p->size,p->height,p->src_inc,p->dst_inc);

error_copyarea:                
         kfree(p);
         break;         
         
      default:
         ret = -EFAULT;
         break;
    }
    return ret;
}

static struct fb_ops FTLCDC210_fb0_ops = {
   .owner      = THIS_MODULE,
   .fb_check_var  = FTLCDC210_fb0_check_var,
   .fb_set_par = FTLCDC210_fb0_set_par,
   .fb_setcolreg  = FTLCDC210_fb_setcolreg,
   .fb_pan_display   = FTLCDC210_fb_pan_display,
  
   .fb_read        = faradayfb_read,
   .fb_write       = faradayfb_write,
   /* These are generic software based fb functions */
   .fb_fillrect   = cfb_fillrect,
   .fb_copyarea   = cfb_copyarea,
   .fb_imageblit  = cfb_imageblit,
   
   .fb_ioctl     = faradayfb_ioctl,
  #if defined(CACHE_FB)
   .fb_mmap   = faradayfb_mmap,
  #endif
};

#if CONFIG_FTLCDC210_NR_FB > 1
static struct fb_ops FTLCDC210_fb1_ops = {
   .owner      = THIS_MODULE,
   .fb_check_var  = FTLCDC210_fb1_check_var,
   .fb_set_par = FTLCDC210_fb1_set_par,
   .fb_setcolreg  = FTLCDC210_fb_setcolreg,
   .fb_pan_display   = FTLCDC210_fb_pan_display,

   /* These are generic software based fb functions */
   .fb_fillrect   = cfb_fillrect,
   .fb_copyarea   = cfb_copyarea,
   .fb_imageblit  = cfb_imageblit,
};
#endif

#if CONFIG_FTLCDC210_NR_FB > 3
static struct fb_ops FTLCDC210_fb3_ops = {
   .owner      = THIS_MODULE,
   .fb_check_var  = FTLCDC210_fb1_check_var,
   .fb_set_par = FTLCDC210_fb3_set_par,
   .fb_setcolreg  = FTLCDC210_fb_setcolreg,
   .fb_pan_display   = FTLCDC210_fb_pan_display,

   /* These are generic software based fb functions */
   .fb_fillrect   = cfb_fillrect,
   .fb_copyarea   = cfb_copyarea,
   .fb_imageblit  = cfb_imageblit,
};
#endif

#if CONFIG_FTLCDC210_NR_FB > 4
static struct fb_ops FTLCDC210_fb4_ops = {
   .owner      = THIS_MODULE,
   .fb_check_var  = FTLCDC210_fb4_check_var,
   .fb_set_par = FTLCDC210_fb4_set_par,
   .fb_setcolreg  = FTLCDC210_fb_setcolreg,
   .fb_pan_display   = FTLCDC210_fb_pan_display,
   .fb_ioctl     = faradayfb_ioctl,
  #if defined(CACHE_FB)
   .fb_mmap   = faradayfb_mmap,
  #endif
   /* These are generic software based fb functions */
   .fb_fillrect   = cfb_fillrect,
   .fb_copyarea   = cfb_copyarea,
   .fb_imageblit  = cfb_imageblit,
};
#endif

/******************************************************************************
 * struct device_attribute functions
 *
 * These functions handle files in
 * /sys/devices/platform/FTLCDC210.x/
 *****************************************************************************/
#if CONFIG_FTLCDC210_NR_FB > 1
static ssize_t FTLCDC210_show_pip(struct device *device,
      struct device_attribute *attr, char *buf)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   unsigned int reg;
   int pip = 0;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_CTRL);
   reg &= FTLCDC210_CTRL_PIP_MASK;

   if (reg == FTLCDC210_CTRL_PIP_SINGLE) {
      pip = 1;
   } else if (reg == FTLCDC210_CTRL_PIP_DOUBLE) {
      pip = 2;
   }

   return snprintf(buf, PAGE_SIZE, "%d\n", pip);
}

static ssize_t FTLCDC210_store_pip(struct device *device,
      struct device_attribute *attr, const char *buf, size_t count)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   char **last = NULL;
   unsigned int reg;
   int pip;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   pip = simple_strtoul(buf, last, 0);
   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_CTRL);
   reg &= ~FTLCDC210_CTRL_PIP_MASK;
   reg |= FTLCDC210_CTRL_BLEND;

   if (pip == 0) {
   } else if (pip == 1) {
      reg |= FTLCDC210_CTRL_PIP_SINGLE;
   } else if (pip == 2) {
      reg |= FTLCDC210_CTRL_PIP_DOUBLE;
   } else {
      dev_info(pftlcdc210->dev, "invalid pip window number %d\n", pip);
      return -EINVAL;
   }

   dev_dbg(pftlcdc210->dev, "  [CTRL]       = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

   return count;
}


static ssize_t FTLCDC210_show_fps(struct device *device,
      struct device_attribute *attr, char *buf)
{
   //struct FTLCDC210 *FTLCDC210 = dev_get_drvdata(device);
   int fps;
   
   // fps * 10
   fps = display_cnt*HZ*10/(jiffies-start_jiffies);         // HZ jiffies = 1 sec

   return snprintf(buf, PAGE_SIZE, "%d.%d\n", fps/10, (fps)%10 );
}


static ssize_t FTLCDC210_store_fps(struct device *device,
      struct device_attribute *attr, const char *buf, size_t count)
{
   //struct FTLCDC210 *FTLCDC210 = dev_get_drvdata(device);

   // don't care contain
   start_jiffies = jiffies;
   display_cnt = 0;

   return count;
}




static ssize_t FTLCDC210_show_blend1(struct device *device,
      struct device_attribute *attr, char *buf)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   unsigned int reg;
   int blend1;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_PIP);
   blend1 = FTLCDC210_PIP_BLEND_EXTRACT_1(reg);

   return snprintf(buf, PAGE_SIZE, "%d\n", blend1);
}

static ssize_t FTLCDC210_store_blend1(struct device *device,
      struct device_attribute *attr, const char *buf, size_t count)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   char **last = NULL;
   unsigned int reg;
   int blend1;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   blend1 = simple_strtoul(buf, last, 0);

   if (blend1 > 16)
      return -EINVAL;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_PIP);
   reg &= ~FTLCDC210_PIP_BLEND_MASK_1;
   reg |= FTLCDC210_PIP_BLEND_1(blend1);

   dev_dbg(pftlcdc210->dev, "  [PIP]        = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_PIP);

   return count;
}

static ssize_t FTLCDC210_show_blend2(struct device *device,
      struct device_attribute *attr, char *buf)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   unsigned int reg;
   int blend2;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_PIP);
   blend2 = FTLCDC210_PIP_BLEND_EXTRACT_2(reg);

   return snprintf(buf, PAGE_SIZE, "%d\n", blend2);
}

static ssize_t FTLCDC210_store_blend2(struct device *device,
      struct device_attribute *attr, const char *buf, size_t count)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   char **last = NULL;
   unsigned int reg;
   int blend2;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   blend2 = simple_strtoul(buf, last, 0);

   if (blend2 > 16)
      return -EINVAL;

   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_PIP);
   reg &= ~FTLCDC210_PIP_BLEND_MASK_2;
   reg |= FTLCDC210_PIP_BLEND_2(blend2);

   dev_dbg(pftlcdc210->dev, "  [PIP]        = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_PIP);

   return count;
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 3
static ssize_t FTLCDC210_show_pop(struct device *device,
      struct device_attribute *attr, char *buf)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   unsigned int reg;
   int pop;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

   if (reg & FTLCDC210_CTRL_POP) {
      pop = 1;
   } else {
      pop = 0;
   }

   return snprintf(buf, PAGE_SIZE, "%d\n", pop);
}

static ssize_t FTLCDC210_store_pop(struct device *device,
      struct device_attribute *attr, const char *buf, size_t count)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   char **last = NULL;
   unsigned int reg;
   int pop;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   pop = simple_strtoul(buf, last, 0);
   reg = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

   if (pop == 0) {
      reg &= ~FTLCDC210_CTRL_POP;
   } else {
      reg |= FTLCDC210_CTRL_POP;
   }

   dev_dbg(pftlcdc210->dev, "  [CTRL]       = %08x\n", reg);
   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

   return count;
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 4
static ssize_t FTLCDC210_show_zoom(struct device *device,
      struct device_attribute *attr, char *buf)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   unsigned int ctrl;
   int zoom;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   ctrl = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

   if (ctrl & FTLCDC210_CTRL_SCALAR) {
      zoom = 1;
   } else {
      zoom = 0;
   }

   return snprintf(buf, PAGE_SIZE, "%d\n", zoom);
}

static ssize_t FTLCDC210_store_zoom(struct device *device,
      struct device_attribute *attr, const char *buf, size_t count)
{
   struct FTLCDC210 *pftlcdc210 = dev_get_drvdata(device);
   char **last = NULL;
   unsigned int ctrl;
   unsigned int base;
   int zoom;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   zoom = simple_strtoul(buf, last, 0);
   ctrl = ioread32(pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

   if (zoom == 0) {
      if (FTLCDC210->scalar_on==0)
         goto out;

      pftlcdc210->scalar_on = 0;
      pftlcdc210->fb[0]->set_frame_base = FTLCDC210_fb0_set_frame_base;
      pftlcdc210->fb[4]->set_frame_base = FTLCDC210_dont_set_frame_base;

      ctrl &= ~(FTLCDC210_CTRL_YUV | FTLCDC210_CTRL_YUV420);
      ctrl &= ~FTLCDC210_CTRL_SCALAR;

      base = FTLCDC210_FRAME_BASE(pftlcdc210->fb[0]->info->fix.smem_start);
      FTLCDC210_fb4_set_frame_base(pftlcdc210, base);
   } else {

       /* set mode (rgb or yuv) */
      if (pftlcdc210->fb[4]->info->var.bits_per_pixel==12)      // yuv420 mode
      {
         ctrl |= (FTLCDC210_CTRL_YUV | FTLCDC210_CTRL_YUV420);
      }
      else
      {
         ctrl &= ~(FTLCDC210_CTRL_YUV | FTLCDC210_CTRL_YUV420);      // RGB565
         if (pftlcdc210->fb[4]->info->var.nonstd==1)
         {
            ctrl |= (FTLCDC210_CTRL_YUV); // uyvy
         }
      }

      if (pftlcdc210->scalar_on==1)
         goto out;

      pftlcdc210->scalar_on = 1;
      pftlcdc210->fb[0]->set_frame_base = FTLCDC210_dont_set_frame_base;
      pftlcdc210->fb[4]->set_frame_base = FTLCDC210_fb4_set_frame_base;

      base = FTLCDC210_FRAME_BASE(pftlcdc210->fb[4]->info->fix.smem_start);
      FTLCDC210_fb4_set_frame_base(pftlcdc210, base);
   }

   dev_dbg(pftlcdc210->dev, "  [CTRL]       = %08x\n", ctrl);
   iowrite32(ctrl, pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

out:
   return count;
}
#endif

#if CONFIG_FTLCDC210_NR_FB > 1
static struct device_attribute ftlcdc210_device_attrs[] = {
   __ATTR(pip, S_IRUGO|S_IWUSR, FTLCDC210_show_pip, FTLCDC210_store_pip),
   __ATTR(fps, S_IRUGO|S_IWUSR, FTLCDC210_show_fps, FTLCDC210_store_fps),
   __ATTR(blend1, S_IRUGO|S_IWUSR, FTLCDC210_show_blend1, FTLCDC210_store_blend1),
   __ATTR(blend2, S_IRUGO|S_IWUSR, FTLCDC210_show_blend2, FTLCDC210_store_blend2),
#if CONFIG_FTLCDC210_NR_FB > 3
   __ATTR(pop, S_IRUGO|S_IWUSR, FTLCDC210_show_pop, FTLCDC210_store_pop),
#endif
#if CONFIG_FTLCDC210_NR_FB > 4
   __ATTR(zoom, S_IRUGO|S_IWUSR, FTLCDC210_show_zoom, FTLCDC210_store_zoom),
#endif
};
#endif

/******************************************************************************
 * struct device_attribute functions
 *
 * These functions handle files in
 * /sys/class/graphics/fb0/
 * /sys/class/graphics/fb1/
 * /sys/class/graphics/fb2/
 * /sys/class/graphics/fb3/
 *****************************************************************************/
static ssize_t ftlcdc210_show_smem_start(struct device *device,
      struct device_attribute *attr, char *buf)
{
   struct fb_info *info = dev_get_drvdata(device);

   return snprintf(buf, PAGE_SIZE, "%08lx\n", info->fix.smem_start);
}

#if CONFIG_FTLCDC210_NR_FB > 3
static ssize_t ftlcdc210_show_popscale(struct device *device,
      struct device_attribute *attr, char *buf)
{
   struct fb_info *info = dev_get_drvdata(device);
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   int scale;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);

   scale = pftlcdc210fb->get_popscale(pftlcdc210);

   return snprintf(buf, PAGE_SIZE, "%d\n", scale);
}

static ssize_t ftlcdc210_store_popscale(struct device *device,
      struct device_attribute *attr, const char *buf, size_t count)
{
   struct fb_info *info = dev_get_drvdata(device);
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   char **last = NULL;
   int scale;

   dev_dbg(pftlcdc210->dev, "%s\n", __func__);
   scale = simple_strtoul(buf, last, 0);

   if (scale > 2)
      return -EINVAL;

   pftlcdc210fb->set_popscale(pftlcdc210, scale);

   return count;
}
#endif

static struct device_attribute ftlcdc210_fb_device_attrs[] = {
   __ATTR(smem_start, S_IRUGO, ftlcdc210_show_smem_start, NULL),
#if CONFIG_FTLCDC210_NR_FB > 3
   __ATTR(scaledown, S_IRUGO|S_IWUSR, ftlcdc210_show_popscale, ftlcdc210_store_popscale),
#endif
};

/******************************************************************************
 * struct device_attribute functions
 *
 * These functions handle files in
 * /sys/class/graphics/fb1/
 * /sys/class/graphics/fb2/
 *****************************************************************************/
#if CONFIG_FTLCDC210_NR_FB > 1
static ssize_t ftlcdc210_show_pos(struct device *device,
      struct device_attribute *attr, char *buf)
{
   struct fb_info *info = dev_get_drvdata(device);
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   unsigned int x, y;

   dev_dbg(pftlcdc210->dev, "fb%d: %s\n", info->node, __func__);
   pftlcdc210fb->get_position(pftlcdc210, &x, &y);

   return snprintf(buf, PAGE_SIZE, "%d,%d\n", x, y);
}

static ssize_t ftlcdc210_store_pos(struct device *device,
      struct device_attribute *attr, const char *buf, size_t count)
{
   struct fb_info *info = dev_get_drvdata(device);
   struct FTLCDC210fb *pftlcdc210fb = info->par;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   char *last = NULL;
   unsigned int x, y;

   dev_dbg(pftlcdc210->dev, "fb%d: %s\n", info->node, __func__);

   x = simple_strtoul(buf, &last, 0);
   last++;
   if (last - buf >= count)
      return -EINVAL;
   y = simple_strtoul(last, &last, 0);

   pftlcdc210fb->set_position(pftlcdc210, x, y);

   return count;
}

static struct device_attribute ftlcdc210_fb1_device_attrs[] = {
   __ATTR(position, S_IRUGO|S_IWUSR, ftlcdc210_show_pos, ftlcdc210_store_pos),
};
#endif

/******************************************************************************
 * internal functions - struct FTLCDC210fb
 *****************************************************************************/
static int __init ftlcdc210_alloc_ftlcdc210fb(struct FTLCDC210 *pftlcdc210, int nr)
{
   struct device *dev = pftlcdc210->dev;
   struct FTLCDC210fb *pftlcdc210fb;
   struct fb_info *info;
   int ret;
   int i;

   dev_dbg(dev, "%s\n", __func__);
   /*
    * Allocate info and par
    */
   info = framebuffer_alloc(sizeof(struct FTLCDC210fb), dev);
   if (!info) {
      dev_err(dev, "Failed to allocate fb_info\n");
      ret = -ENOMEM;
      goto err_alloc_info;
   }

   pftlcdc210fb = info->par;
   pftlcdc210fb->info = info;

   pftlcdc210fb->FTLCDC210 = pftlcdc210;
   pftlcdc210->fb[nr] = pftlcdc210fb;

   /*
    * Set up flags to indicate what sort of acceleration your
    * driver can provide (pan/wrap/copyarea/etc.) and whether it
    * is a module -- see FBINFO_* in include/linux/fb.h
    */
   info->flags = FBINFO_DEFAULT | FBINFO_HWACCEL_COPYAREA | FBINFO_HWACCEL_FILLRECT | FBINFO_HWACCEL_IMAGEBLIT ;

   /*
    * Copy default parameters
    */
   info->fix = ftlcdc210_default_fix;
   info->var = ftlcdc210_default_var;

   switch (nr) {
   case 0:
      #ifdef FOR_ANDROID
         info->var.yres_virtual = info->var.yres * 2;
      #endif
      info->fbops = &FTLCDC210_fb0_ops;
      pftlcdc210fb->set_frame_base = FTLCDC210_fb0_set_frame_base;

#if CONFIG_FTLCDC210_NR_FB > 3
      pftlcdc210fb->set_popscale = FTLCDC210_fb0_set_popscale;
      pftlcdc210fb->get_popscale = FTLCDC210_fb0_get_popscale;
#endif
      break;

#if CONFIG_FTLCDC210_NR_FB > 1
   case 1:
      info->var.yres = info->var.yres_virtual = 0;    // 用到時再 allocate
      info->fbops = &FTLCDC210_fb1_ops;
      pftlcdc210fb->set_frame_base = FTLCDC210_fb1_set_frame_base;
      pftlcdc210fb->set_dimension = FTLCDC210_fb1_set_dimension;
      pftlcdc210fb->set_position = FTLCDC210_fb1_set_position;
      pftlcdc210fb->get_position = FTLCDC210_fb1_get_position;
#if CONFIG_FTLCDC210_NR_FB > 3
      pftlcdc210fb->set_popscale = FTLCDC210_fb1_set_popscale;
      pftlcdc210fb->get_popscale = FTLCDC210_fb1_get_popscale;
#endif

      pftlcdc210fb->set_position(pftlcdc210, 0, 0);
      break;
#endif

#if CONFIG_FTLCDC210_NR_FB > 2
   case 2:
      info->var.yres = info->var.yres_virtual = 0;    // 用到時再 allocate
      info->fbops = &FTLCDC210_fb1_ops;
      pftlcdc210fb->set_frame_base = FTLCDC210_fb2_set_frame_base;
      pftlcdc210fb->set_dimension = FTLCDC210_fb2_set_dimension;
      pftlcdc210fb->set_position = FTLCDC210_fb2_set_position;
      pftlcdc210fb->get_position = FTLCDC210_fb2_get_position;
#if CONFIG_FTLCDC210_NR_FB > 3
      pftlcdc210fb->set_popscale = FTLCDC210_fb2_set_popscale;
      pftlcdc210fb->get_popscale = FTLCDC210_fb2_get_popscale;
#endif

      pftlcdc210fb->set_position(pftlcdc210, 0, 0);
      break;
#endif

#if CONFIG_FTLCDC210_NR_FB > 3
   case 3:
      info->var.yres = info->var.yres_virtual = 0;    // 用到時再 allocate
      info->fbops = &FTLCDC210_fb3_ops;
      pftlcdc210fb->set_frame_base = FTLCDC210_fb3_set_frame_base;
      pftlcdc210fb->set_popscale = FTLCDC210_fb3_set_popscale;
      pftlcdc210fb->get_popscale = FTLCDC210_fb3_get_popscale;
      break;
#endif

#if CONFIG_FTLCDC210_NR_FB > 4
   case 4:
      #if 0
         info->var.yres_virtual = info->var.yres * 4;    // for mpeg4 & h264
      #else
         info->var.xres_virtual = info->var.xres = 1024;
         info->var.yres = 768;
            info->var.yres_virtual = info->var.yres * 4;
      #endif
      info->fbops = &FTLCDC210_fb4_ops;
      pftlcdc210fb->set_frame_base = FTLCDC210_dont_set_frame_base;
      pftlcdc210fb->set_popscale = FTLCDC210_fb4_set_popscale;
      pftlcdc210fb->get_popscale = FTLCDC210_fb4_get_popscale;
      break;
#endif

   default:
      BUG();
   }

   info->pseudo_palette = pftlcdc210fb->pseudo_palette;



   /*
    * Allocate colormap
    */
   ret = fb_alloc_cmap(&info->cmap, 256, 0);
   if (ret < 0) {
      dev_err(dev, "Failed to allocate colormap\n");
      goto err_alloc_cmap;
   }

   

#if 0 
   /*
    * Copy default parameters
    */
   info->fix = ftlcdc210_default_fix;
   info->var = ftlcdc210_default_var;
#endif

   ret = info->fbops->fb_check_var(&info->var, info);
   if (ret < 0) {
      dev_err(dev, "fb_check_var() failed\n");
      goto err_check_var;
   }  

   /*
    * Does a call to fb_set_par() before register_framebuffer needed?  This
    * will depend on you and the hardware.  If you are sure that your driver
    * is the only device in the system, a call to fb_set_par() is safe.
    *
    * Hardware in x86 systems has a VGA core.  Calling set_par() at this
    * point will corrupt the VGA console, so it might be safer to skip a
    * call to set_par here and just allow fbcon to do it for you.
    */
   info->fbops->fb_set_par(info);
   
   /*
    * Tell the world that we're ready to go
    */
   if (register_framebuffer(info) < 0) {
      dev_err(dev, "Failed to register frame buffer\n");
      ret = -EINVAL;
      goto err_register_info;
   }


   /*
    * create files in /sys/class/graphics/fbx/
    */
   for (i = 0; i < ARRAY_SIZE(ftlcdc210_fb_device_attrs); i++) {
      ret = device_create_file(info->dev,
         &ftlcdc210_fb_device_attrs[i]);
      if (ret) {
         break;
      }
   }

   if (ret) {
      dev_err(dev, "Failed to create device files\n");
      while (--i >= 0) {
         device_remove_file(info->dev,
            &ftlcdc210_fb_device_attrs[i]);
      }
      goto err_sysfs1;
   }

   

   switch (nr) {
   case 0:
      break;
#if CONFIG_FTLCDC210_NR_FB > 1
   case 1:
#if CONFIG_FTLCDC210_NR_FB > 2
   case 2:
#endif
      /*
       * create files in /sys/class/graphics/fbx/
       */
      for (i = 0; i < ARRAY_SIZE(ftlcdc210_fb1_device_attrs); i++) {
         ret = device_create_file(info->dev,
            &ftlcdc210_fb1_device_attrs[i]);
         if (ret)
            break;
      }

      if (ret) {
         dev_err(dev, "Failed to create device files\n");
         while (--i >= 0) {
            device_remove_file(info->dev,
               &ftlcdc210_fb1_device_attrs[i]);
         }
         goto err_sysfs2;
      }

      break;
#endif

#if CONFIG_FTLCDC210_NR_FB > 3
   case 3:
#if CONFIG_FTLCDC210_NR_FB > 4
   case 4:
#endif
      break;
#endif

   default:
      BUG();
   }

   dev_info(dev, "fb%d: %s frame buffer device\n", info->node,
      info->fix.id);

   return 0;

#if CONFIG_FTLCDC210_NR_FB > 1
err_sysfs2:
#endif
   for (i = 0; ARRAY_SIZE(ftlcdc210_fb_device_attrs); i++)
      device_remove_file(info->dev,
         &ftlcdc210_fb_device_attrs[i]);
err_sysfs1:
   unregister_framebuffer(info);
err_register_info:
err_check_var:
   if (info->screen_base) {
      #if defined(CACHE_FB)
         //kfree(info->screen_base);
         free_pages(info->screen_base, get_order(info->fix.smem_len));
      #else
         dma_free_writecombine(dev, info->fix.smem_len, info->screen_base, (dma_addr_t )info->fix.smem_start);
      #endif
   }

   fb_dealloc_cmap(&info->cmap);
err_alloc_cmap:
   framebuffer_release(info);
err_alloc_info:
   return ret;
}

static void ftlcdc210_free_ftlcdc210fb(struct FTLCDC210fb *pftlcdc210fb, int nr)
{
   struct fb_info *info = pftlcdc210fb->info;
   struct FTLCDC210 *pftlcdc210 = pftlcdc210fb->FTLCDC210;
   struct device *dev = pftlcdc210->dev;
   
#if CONFIG_FTLCDC210_NR_FB > 1
   int i;
#endif

   dev_dbg(dev, "%s\n", __func__);
   switch (nr) {
   case 0:
      break;
#if CONFIG_FTLCDC210_NR_FB > 1
   case 1:
#if CONFIG_FTLCDC210_NR_FB > 2
   case 2:
#endif
      for (i = 0; i < ARRAY_SIZE(ftlcdc210_fb1_device_attrs); i++)
         device_remove_file(info->dev,
            &ftlcdc210_fb1_device_attrs[i]);

      break;
#endif

#if CONFIG_FTLCDC210_NR_FB > 3
   case 3:
#if CONFIG_FTLCDC210_NR_FB > 4
   case 4:
#endif
      break;
#endif

   default:
      BUG();
   }

#if CONFIG_FTLCDC210_NR_FB > 3
   for (i = 0; i < ARRAY_SIZE(ftlcdc210_fb_device_attrs); i++)
      device_remove_file(info->dev, &ftlcdc210_fb_device_attrs[i]);
#endif

   unregister_framebuffer(info);
   dma_free_writecombine(dev, info->fix.smem_len, info->screen_base, (dma_addr_t )info->fix.smem_start);

   fb_dealloc_cmap(&info->cmap);
   framebuffer_release(info);
}


#if defined(CACHE_FB)
int lcd_thread(void * unused)
{
   int i;
        
    for (i=0; ; ++i)
   {
      if (cur_dma_addr != NULL)
      {
         if (--flush_countdown==0)
         {
            dma_sync_single_for_device(NULL, cur_dma_addr, cur_frame_size, DMA_TO_DEVICE);
            flush_countdown = FLUSH_CNTDOWN;
         }
      }
      msleep(500);
   }
   return 0;
}
#endif //CACHE_FB


static const __initconst struct of_device_id board_clk_match[] = {
#ifdef CONFIG_MACH_LEO
   { .compatible = "leo,hclk", .data = of_fixed_factor_clk_setup, },
   { .compatible = "leo,pclk", .data = of_fixed_factor_clk_setup, },
   { .compatible = "leo,mcpu", .data = of_fixed_factor_clk_setup, },
#else   
   { .compatible = "a380evb,hclk", .data = of_fixed_factor_clk_setup, },
   { .compatible = "a380evb,pclk", .data = of_fixed_factor_clk_setup, },
   { .compatible = "a380evb,mcpu", .data = of_fixed_factor_clk_setup, },
#endif   
};


/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftlcdc210_probe(struct platform_device *pdev)
{
   struct device *dev = &pdev->dev;
   struct FTLCDC210 *pftlcdc210;
   struct resource *res;
   struct clk *clk = 0;
   unsigned int reg;
   int irq_be;
   int irq_ur;
   int irq_nb;
   int irq_vs;
   int ret;
   int i;

   unsigned long cpuclk, aclk, hclk, pclk, mclk;
   dev_dbg(dev, "%s\n", __func__);
   res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
   if (!res) {
      return -ENXIO;
   }

   irq_be = platform_get_irq(pdev, 0);
   if (irq_be < 0) {
      ret = irq_be;
      goto err_get_irq;
   }

   irq_ur = platform_get_irq(pdev, 1);
   if (irq_ur < 0) {
      ret = irq_ur;
      goto err_get_irq;
   }

   irq_nb = platform_get_irq(pdev, 2);
   if (irq_nb < 0) {
      ret = irq_nb;
      goto err_get_irq;
   }

   irq_vs = platform_get_irq(pdev, 3);
   if (irq_vs < 0) {
      ret = irq_vs;
      goto err_get_irq;
   }

   pftlcdc210 = kzalloc(sizeof(struct FTLCDC210), GFP_KERNEL);
   if (!pftlcdc210) {
      dev_err(dev, "Failed to allocate private data\n");
      ret = -ENOMEM;
      goto err_alloc_ftlcdc210;
   }

   platform_set_drvdata(pdev, pftlcdc210);
   pftlcdc210->dev = dev;

   init_waitqueue_head(&pftlcdc210->wait_nb);

   cpuclk = 0; aclk = 0; hclk = 0; pclk = 0; mclk = 0;

   of_clk_init(board_clk_match);

   if (pdev->dev.of_node) {
      clk = devm_clk_get(&pdev->dev, NULL);
      hclk = clk_get_rate(clk);
      pftlcdc210->clk = ((struct clk *)clk);
      pftlcdc210->clk_value_khz = hclk/1000;//0x30570;
   }
   else
   {
      clk = clk_get(NULL, "hclk");
      if (IS_ERR(clk)) {
         dev_err(dev, "Failed to get clock\n");
         ret = PTR_ERR(clk);
         goto err_clk_get;
      }
      clk_enable(clk);
      pftlcdc210->clk = clk;
      pftlcdc210->clk_value_khz = clk_get_rate(clk) / 1000;
   }

#ifdef CONFIG_FTLCDC210_SERIAL
   // In serial mode, 3 clocks are used to send a pixel 
   pftlcdc210->clk_value_khz /= 3;
#endif

   //Map io memory
   pftlcdc210->res = request_mem_region(res->start, res->end - res->start,
         dev_name(dev));
   if (!pftlcdc210->res) {
      dev_err(dev, "Could not reserve memory region\n");
      ret = -ENOMEM;
      goto err_req_mem_region;
   }

   pftlcdc210->base = ioremap(res->start, res->end - res->start);
   if (!pftlcdc210->base) {
      dev_err(dev, "Failed to ioremap registers\n");
      ret = -EIO;
      goto err_ioremap;
   }
      
   //Make sure LCD controller is disabled
   iowrite32(0, pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

   //Register interrupt handler
   ret = request_irq(irq_be, ftlcdc210_interrupt, IRQF_SHARED, pdev->name, pftlcdc210);
   if (ret < 0) {
      dev_err(dev, "Failed to request irq %d\n", irq_be);
      goto err_req_irq_be;
   }

   ret = request_irq(irq_ur, ftlcdc210_interrupt, IRQF_SHARED, pdev->name, pftlcdc210);
   if (ret < 0) {
      dev_err(dev, "Failed to request irq %d\n", irq_ur);
      goto err_req_irq_ur;
   }

   ret = request_irq(irq_nb, ftlcdc210_interrupt, IRQF_SHARED, pdev->name, pftlcdc210);
   if (ret < 0) {
      dev_err(dev, "Failed to request irq %d\n", irq_nb);
      goto err_req_irq_nb;
   }

   ret = request_irq(irq_vs, ftlcdc210_interrupt, IRQF_SHARED, pdev->name, pftlcdc210);
   if (ret < 0) {
      dev_err(dev, "Failed to request irq %d\n", irq_vs);
      goto err_req_irq_vs;
   }

   pftlcdc210->irq_be = irq_be;
   pftlcdc210->irq_ur = irq_ur;
   pftlcdc210->irq_nb = irq_nb;
   pftlcdc210->irq_vs = irq_vs;

   //Enable interrupts
   reg = FTLCDC210_INT_UNDERRUN
       | FTLCDC210_INT_NEXT_BASE
       | FTLCDC210_INT_BUS_ERROR;

   iowrite32(reg, pftlcdc210->base + FTLCDC210_OFFSET_INT_ENABLE);
   
   // lmc83: 
   iowrite32(512, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_HHT);
   iowrite32(256, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_HLT);
   iowrite32(512, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_VHT);
   iowrite32(256, pftlcdc210->base + FTLCDC210_OFFSET_SCALE_VLT);


   //Initialize gamma table
   ftlcdc210_set_linear_gamma(pftlcdc210);
#if 0
   // bypass gamma - this register is not in the data sheet 
   iowrite32(0xf, pftlcdc210->base + 0x54);
#endif


#if CONFIG_FTLCDC210_NR_FB > 1

   //create files in /sys/devices/platform/FTLCDC210.x/
   for (i = 0; i < ARRAY_SIZE(ftlcdc210_device_attrs); i++) {
      ret = device_create_file(pftlcdc210->dev,
         &ftlcdc210_device_attrs[i]);
      if (ret)
         break;
   }

   if (ret) {
      dev_err(dev, "Failed to create device files\n");
      while (--i >= 0) {
         device_remove_file(pftlcdc210->dev,
            &ftlcdc210_device_attrs[i]);
      }
      goto err_sysfs;
   }
#endif
   
   
   for (i = 0; i < CONFIG_FTLCDC210_NR_FB; i++) {
      ret = ftlcdc210_alloc_ftlcdc210fb(pftlcdc210, i);
      if (ret) {
         goto err_alloc_ftlcdc210fb;
      }
   }
   
   #if defined(CACHE_FB)
      kernel_thread(lcd_thread, NULL, SIGCHLD);
   #endif //CACHE_FB
    
   return 0;

err_alloc_ftlcdc210fb:
   // disable LCD HW 
   iowrite32(0, pftlcdc210->base + FTLCDC210_OFFSET_CTRL);
   iowrite32(0, pftlcdc210->base + FTLCDC210_OFFSET_INT_ENABLE);

   while (--i >= 0) {
      ftlcdc210_free_ftlcdc210fb(pftlcdc210->fb[i], i);
   }
#if CONFIG_FTLCDC210_NR_FB > 1
err_sysfs:
#endif
   free_irq(irq_vs, pftlcdc210);
err_req_irq_vs:
   free_irq(irq_nb, pftlcdc210);
err_req_irq_nb:
   free_irq(irq_ur, pftlcdc210);
err_req_irq_ur:
   free_irq(irq_be, pftlcdc210);
err_req_irq_be:
   iounmap(pftlcdc210->base);
err_ioremap:
err_req_mem_region:
   clk_put(clk);
err_clk_get:
   platform_set_drvdata(pdev, NULL);
   kfree(pftlcdc210);
err_alloc_ftlcdc210:
err_get_irq:
   release_resource(res);
   
   return ret;
}

static int  ftlcdc210_remove(struct platform_device *pdev)
{
   struct FTLCDC210 *pftlcdc210;
   int i;

   pftlcdc210 = platform_get_drvdata(pdev);

   // disable LCD HW 
   iowrite32(0, pftlcdc210->base + FTLCDC210_OFFSET_INT_ENABLE);
   iowrite32(0, pftlcdc210->base + FTLCDC210_OFFSET_CTRL);

   for (i = 0; i < CONFIG_FTLCDC210_NR_FB; i++) {
      ftlcdc210_free_ftlcdc210fb(pftlcdc210->fb[i], i);
   }

#if CONFIG_FTLCDC210_NR_FB > 1
   for (i = 0; i < ARRAY_SIZE(ftlcdc210_device_attrs); i++)
      device_remove_file(pftlcdc210->dev, &ftlcdc210_device_attrs[i]);
#endif

   free_irq(pftlcdc210->irq_vs, pftlcdc210);
   free_irq(pftlcdc210->irq_nb, pftlcdc210);
   free_irq(pftlcdc210->irq_ur, pftlcdc210);
   free_irq(pftlcdc210->irq_be, pftlcdc210);

   iounmap(pftlcdc210->base);
   clk_put(pftlcdc210->clk);
   platform_set_drvdata(pdev, NULL);
   release_resource(pftlcdc210->res);
   kfree(pftlcdc210);

   return 0;
}

static const struct of_device_id ftlcdc210_dt_ids[] = {
   { .compatible = "faraday,ftlcdc210", },
   {},
};

static struct platform_driver ftlcdc210_driver = {
   .probe      = ftlcdc210_probe,
   .remove     = ftlcdc210_remove,

   .driver     = {
      .name = "ftlcdc210", 
      .owner   = THIS_MODULE,
      .of_match_table = ftlcdc210_dt_ids,
   },
};

module_platform_driver(ftlcdc210_driver);

MODULE_DESCRIPTION("FTLCDC210 LCD Controller framebuffer driver");
MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_LICENSE("GPL");
