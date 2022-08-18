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

#ifndef __FTLCDC210_H
#define __FTLCDC210_H

#define FTLCDC210_OFFSET_CTRL             0x00
#define FTLCDC210_OFFSET_PIXEL            0x04
#define FTLCDC210_OFFSET_INT_ENABLE       0x08
#define FTLCDC210_OFFSET_INT_CLEAR        0x0c
#define FTLCDC210_OFFSET_INT_STATUS       0x10
#define FTLCDC210_OFFSET_POPSCALE         0x14
#define FTLCDC210_OFFSET_FRAME_BASE0      0x18
#define FTLCDC210_OFFSET_FRAME_BASE1      0x24
#define FTLCDC210_OFFSET_FRAME_BASE2      0x30
#define FTLCDC210_OFFSET_FRAME_BASE3      0x3c
#define FTLCDC210_OFFSET_PATGEN           0x48
#define FTLCDC210_OFFSET_FIFO_THRESHOLD   0x4c
#define FTLCDC210_OFFSET_GPIO_CONTROL     0x50
#define FTLCDC210_OFFSET_HTIMING          0x100
#define FTLCDC210_OFFSET_VTIMING0         0x104
#define FTLCDC210_OFFSET_VTIMING1         0x108
#define FTLCDC210_OFFSET_POLARITY         0x10c
#define FTLCDC210_OFFSET_SERIAL           0x200
#define FTLCDC210_OFFSET_CCIR656          0x204
#define FTLCDC210_OFFSET_PIP              0x300
#define FTLCDC210_OFFSET_PIP_POS1         0x304
#define FTLCDC210_OFFSET_PIP_DIM1         0x308
#define FTLCDC210_OFFSET_PIP_POS2         0x30c
#define FTLCDC210_OFFSET_PIP_DIM2         0x310
#define FTLCDC210_OFFSET_CM0              0x400
#define FTLCDC210_OFFSET_CM1              0x404
#define FTLCDC210_OFFSET_CM2              0x408
#define FTLCDC210_OFFSET_CM3              0x40c
#define FTLCDC210_OFFSET_GAMMA_R          0x600
#define FTLCDC210_OFFSET_GAMMA_G          0x700
#define FTLCDC210_OFFSET_GAMMA_B          0x800
#define FTLCDC210_OFFSET_PALETTE          0xa00 /* 0xa00  - 0xbfc */
#define FTLCDC210_OFFSET_SCALE_IN_HRES    0x1100
#define FTLCDC210_OFFSET_SCALE_IN_VRES    0x1104
#define FTLCDC210_OFFSET_SCALE_OUT_HRES   0x1108
#define FTLCDC210_OFFSET_SCALE_OUT_VRES   0x110c
#define FTLCDC210_OFFSET_SCALE_CTRL       0x1110
#define FTLCDC210_OFFSET_SCALE_HHT        0x1114
#define FTLCDC210_OFFSET_SCALE_HLT        0x1118
#define FTLCDC210_OFFSET_SCALE_VHT        0x111c
#define FTLCDC210_OFFSET_SCALE_VLT        0x1120
#define FTLCDC210_OFFSET_SCALE_PAR        0x112c

/*
 * Complex OSD configuration
 */
#ifdef   CONFIG_FTLCDC210_COMPLEX_OSD

#define FTLCDC210_OFFSET_OSD_ENABLE       0x2000
#define FTLCDC210_OFFSET_OSD_FONT0        0x2004
#define FTLCDC210_OFFSET_OSD_FONT1        0x2008
#define FTLCDC210_OFFSET_OSD_W1C1         0x2010
#define FTLCDC210_OFFSET_OSD_W1C2         0x2014
#define FTLCDC210_OFFSET_OSD_W2C1         0x201c
#define FTLCDC210_OFFSET_OSD_W2C2         0x2020
#define FTLCDC210_OFFSET_OSD_W3C1         0x2028
#define FTLCDC210_OFFSET_OSD_W3C2         0x202c
#define FTLCDC210_OFFSET_OSD_W4C1         0x2034
#define FTLCDC210_OFFSET_OSD_W4C2         0x2038
#define FTLCDC210_OFFSET_OSD_MCFB         0x203c
#define FTLCDC210_OFFSET_OSD_PC0          0x2040
#define FTLCDC210_OFFSET_OSD_PC1          0x2044
#define FTLCDC210_OFFSET_OSD_PC2          0x2048
#define FTLCDC210_OFFSET_OSD_PC3          0x204c
#define FTLCDC210_OFFSET_OSD_PC4          0x2050
#define FTLCDC210_OFFSET_OSD_PC5          0x2054
#define FTLCDC210_OFFSET_OSD_PC6          0x2058
#define FTLCDC210_OFFSET_OSD_PC7          0x205c
#define FTLCDC210_OFFSET_OSD_PC8          0x2060
#define FTLCDC210_OFFSET_OSD_PC9          0x2064
#define FTLCDC210_OFFSET_OSD_PC10         0x2068
#define FTLCDC210_OFFSET_OSD_PC11         0x206c
#define FTLCDC210_OFFSET_OSD_PC12         0x2070
#define FTLCDC210_OFFSET_OSD_PC13         0x2074
#define FTLCDC210_OFFSET_OSD_PC14         0x2078
#define FTLCDC210_OFFSET_OSD_PC15         0x207c
#define FTLCDC210_OFFSET_OSD_FONTRAM      0x10000  /* 0x10000 - 0x18ffc */
#define FTLCDC210_OFFSET_OSD_DISPLAYRAM   0x20000  /* 0x20000 - 0x207fc */

/*
 * Simple OSD Configuration
 */
#else /* CONFIG_FTLCDC210_COMPLEX_OSD */

#define FTLCDC210_OFFSET_OSD_SDC          0x2000
#define FTLCDC210_OFFSET_OSD_POS          0x2004
#define FTLCDC210_OFFSET_OSD_FG           0x2008
#define FTLCDC210_OFFSET_OSD_BG           0x200c
#define FTLCDC210_OFFSET_OSD_FONT         0x8000   /* 0x8000 - 0xbffc */
#define FTLCDC210_OFFSET_OSD_ATTR         0xc000   /* 0xc000 - 0xc7fc */

#endif   /* CONFIG_FTLCDC210_COMPLEX_OSD */

/*
 * LCD Function Enable
 */
#define FTLCDC210_CTRL_ENABLE             (1 << 0)
#define FTLCDC210_CTRL_LCD                (1 << 1)
#define FTLCDC210_CTRL_YUV420             (1 << 2)
#define FTLCDC210_CTRL_YUV                (1 << 3)
#define FTLCDC210_CTRL_OSD                (1 << 4)
#define FTLCDC210_CTRL_SCALAR             (1 << 5)
#define FTLCDC210_CTRL_DITHER             (1 << 6)
#define FTLCDC210_CTRL_POP                (1 << 7)
#define FTLCDC210_CTRL_BLEND              (1 << 8)
#define FTLCDC210_CTRL_PIP_MASK           (3 << 9)
#define FTLCDC210_CTRL_PIP_SINGLE         (1 << 9)
#define FTLCDC210_CTRL_PIP_DOUBLE         (2 << 9)
#define FTLCDC210_CTRL_PATGEN             (1 << 11)

/*
 * LCD Panel Pixel
 */
#define FTLCDC210_PIXEL_BPP1             (0 << 0)
#define FTLCDC210_PIXEL_BPP2             (1 << 0)
#define FTLCDC210_PIXEL_BPP4             (2 << 0)
#define FTLCDC210_PIXEL_BPP8             (3 << 0)
#define FTLCDC210_PIXEL_BPP16            (4 << 0)
#define FTLCDC210_PIXEL_BPP24            (5 << 0)
#define FTLCDC210_PIXEL_PWROFF           (1 << 3)
#define FTLCDC210_PIXEL_BGR              (1 << 4)
#define FTLCDC210_PIXEL_LEB_LEP          (0 << 5)  /* little endian byte, little endian pixel */
#define FTLCDC210_PIXEL_BEB_BEP          (1 << 5)  /* big    endian byte, big    endian pixel */
#define FTLCDC210_PIXEL_LEB_BEP          (2 << 5)  /* little endian byte, big    endian pixel */
/* The following RGB bits indicate the inpu RGB format when BPP16 */
#define FTLCDC210_PIXEL_RGB565           (0 << 7)
#define FTLCDC210_PIXEL_RGB555           (1 << 7)
#define FTLCDC210_PIXEL_RGB444           (2 << 7)
#define FTLCDC210_PIXEL_VSYNC            (0 << 9)
#define FTLCDC210_PIXEL_VBACK            (1 << 9)
#define FTLCDC210_PIXEL_VACTIVE          (2 << 9)
#define FTLCDC210_PIXEL_VFRONT           (3 << 9)
#define FTLCDC210_PIXEL_24BIT_PANEL      (1 << 11)
/* The following DITHER bits indicate the dither algorithm used when dither enabled */
#define FTLCDC210_PIXEL_DITHER565        (0 << 12)
#define FTLCDC210_PIXEL_DITHER555        (1 << 12)
#define FTLCDC210_PIXEL_DITHER444        (2 << 12)
#define FTLCDC210_PIXEL_HRST             (1 << 14) /* HCLK domain reset */
#define FTLCDC210_PIXEL_LRST             (1 << 15) /* LC_CLK domain reset */

/*
 * LCD Interrupt Enable/Status/Clear
 */
#define FTLCDC210_INT_UNDERRUN           (1 << 0)
#define FTLCDC210_INT_NEXT_BASE          (1 << 1)
#define FTLCDC210_INT_VSTATUS            (1 << 2)
#define FTLCDC210_INT_BUS_ERROR          (1 << 3)

/*
 * Frame buffer
 */
#define FTLCDC210_POPSCALE_0_MASK        (3 << 8)
#define FTLCDC210_POPSCALE_0_QUARTER     (1 << 8)  /* 1/2 x 1/2 */
#define FTLCDC210_POPSCALE_0_HALF        (2 << 8)  /* 1/2 x 1   */
#define FTLCDC210_POPSCALE_1_MASK        (3 << 10)
#define FTLCDC210_POPSCALE_1_QUARTER     (1 << 10) /* 1/2 x 1/2 */
#define FTLCDC210_POPSCALE_1_HALF        (2 << 10) /* 1/2 x 1   */
#define FTLCDC210_POPSCALE_2_MASK        (3 << 12)
#define FTLCDC210_POPSCALE_2_QUARTER     (1 << 12) /* 1/2 x 1/2 */
#define FTLCDC210_POPSCALE_2_HALF        (2 << 12) /* 1/2 x 1   */
#define FTLCDC210_POPSCALE_3_MASK        (3 << 14)
#define FTLCDC210_POPSCALE_3_QUARTER     (1 << 14) /* 1/2 x 1/2 */
#define FTLCDC210_POPSCALE_3_HALF        (2 << 14) /* 1/2 x 1   */

/*
 * LCD Panel Frame Base Address 0/1/2/3
 */
#define FTLCDC210_FRAME_BASE(x)          ((x) & ~3)

/*
 * LCD Pattern Generator
 */
#define FTLCDC210_PATGEN_0_VCBAR         (0 << 0)
#define FTLCDC210_PATGEN_0_HCBAR         (1 << 0)
#define FTLCDC210_PATGEN_0_SC            (2 << 0)
#define FTLCDC210_PATGEN_1_VCBAR         (0 << 2)
#define FTLCDC210_PATGEN_1_HCBAR         (1 << 2)
#define FTLCDC210_PATGEN_1_SC            (2 << 2)
#define FTLCDC210_PATGEN_2_VCBAR         (0 << 4)
#define FTLCDC210_PATGEN_2_HCBAR         (1 << 4)
#define FTLCDC210_PATGEN_2_SC            (2 << 4)
#define FTLCDC210_PATGEN_3_VCBAR         (0 << 6)
#define FTLCDC210_PATGEN_3_HCBAR         (1 << 6)
#define FTLCDC210_PATGEN_3_SC            (2 << 6)

/*
 * LCD FIFO Threshold Control
 */
#define FTLCDC210_FIFO_THRESHOLD_0(x)    (((x) & 0xff) << 0)
#define FTLCDC210_FIFO_THRESHOLD_1(x)    (((x) & 0xff) << 8)
#define FTLCDC210_FIFO_THRESHOLD_2(x)    (((x) & 0xff) << 16)
#define FTLCDC210_FIFO_THRESHOLD_3(x)    (((x) & 0xff) << 24)

/*
 * LCD Horizontal Timing Control
 */
#define FTLCDC210_HTIMING_PL(x)          (((x) & 0xff) << 0)
#define FTLCDC210_HTIMING_HW(x)          (((x) & 0xff) << 8)   /* HW determines X-Position */
#define FTLCDC210_HTIMING_HFP(x)         (((x) & 0xff) << 16)
#define FTLCDC210_HTIMING_HBP(x)         (((x) & 0xff) << 24)

/*
 * LCD Vertial Timing Control 0
 */
#define FTLCDC210_VTIMING0_LF(x)         (((x) & 0xfff) << 0)
#define FTLCDC210_VTIMING0_VW(x)         (((x) & 0x3f) << 16)  /* VW determines Y-Position */
#define FTLCDC210_VTIMING0_VFP(x)        (((x) & 0xff) << 24)

/*
 * LCD Vertial Timing Control 1
 */
#define FTLCDC210_VTIMING1_VBP(x)        (((x) & 0xff) << 0)

/*
 * LCD Polarity Control
 */
#define FTLCDC210_POLARITY_IVS           (1 << 0)
#define FTLCDC210_POLARITY_IHS           (1 << 1)
#define FTLCDC210_POLARITY_ICK           (1 << 2)
#define FTLCDC210_POLARITY_IDE           (1 << 3)
#define FTLCDC210_POLARITY_IPWR          (1 << 4)
#define FTLCDC210_POLARITY_DIVNO(x)      (((x) & 0x7f) << 8)   /* bus clock rate / (x + 1) determines the frame rate */

/*
 * LCD Serial Panel Pixel
 */
#define FTLCDC210_SERIAL_SERIAL          (1 << 0)
#define FTLCDC210_SERIAL_DELTA_TYPE      (1 << 1)
#define FTLCDC210_SERIAL_RGB             (0 << 2)
#define FTLCDC210_SERIAL_BRG             (1 << 2)
#define FTLCDC210_SERIAL_GBR             (2 << 2)
#define FTLCDC210_SERIAL_LSR             (1 << 4)
#define FTLCDC210_SERIAL_AUO052          (1 << 5)

/*
 * CCIR656
 */
#define FTLCDC210_CCIR656_NTSC           (1 << 0)  /* 0: PAL, 1: NTSC */
#define FTLCDC210_CCIR656_P720           (1 << 1)  /* 0: 640, 1: 720 pixels per line */
#define FTLCDC210_CCIR656_PHASE          (1 << 2)

/*
 * PIP (Picture in Picture)
 */
#define FTLCDC210_PIP_BLEND_1(x)          (((x) & 0x1f) << 0)
#define FTLCDC210_PIP_BLEND_EXTRACT_1(pb) (((pb) >> 0) & 0x1f)
#define FTLCDC210_PIP_BLEND_MASK_1        (0x1f << 0)
#define FTLCDC210_PIP_BLEND_2(x)          (((x) & 0x1f) << 8)
#define FTLCDC210_PIP_BLEND_EXTRACT_2(pb) (((pb) >> 8) & 0x1f)
#define FTLCDC210_PIP_BLEND_MASK_2        (0x1f << 8)

/*
 * PIP Sub Picture 1/2 Position
 */
#define FTLCDC210_PIP_POS_V(y)            (((y) & 0x7ff) << 0)
#define FTLCDC210_PIP_POS_EXTRACT_V(pp)   (((pp) >> 0) & 0x7ff)
#define FTLCDC210_PIP_POS_H(x)            (((x) & 0x7ff) << 16)
#define FTLCDC210_PIP_POS_EXTRACT_H(pp)   (((pp) >> 16) & 0x7ff)

/*
 * PIP Sub Picture 1/2 Dimension
 */
#define FTLCDC210_PIP_DIM_V(y)          (((y) & 0x7ff) << 0)
#define FTLCDC210_PIP_DIM_H(x)          (((x) & 0x7ff) << 16)

/*
 * Color Management 0
 */
#define FTLCDC210_CM0_BRIGHT(x)         (((x) & 0x7f) << 0)
#define FTLCDC210_CM0_BRIGHT_NEGATIVE   (1 << 7)
#define FTLCDC210_CM0_SATURATE(x)       (((x) & 0x3f) << 8)

/*
 * Color Management 1
 */
#define FTLCDC210_CM1_HUE_SIN(x)        (((x) & 0x3f) << 0)
#define FTLCDC210_CM1_HUE_SIN_NEGATIVE  (1 << 6)
#define FTLCDC210_CM1_HUE_COS(x)        (((x) & 0x3f) << 8)
#define FTLCDC210_CM1_HUE_COS_NEGATIVE  (1 << 14)

/*
 * Color Management 2
 */
#define FTLCDC210_CM2_SHARP_TH0(x)      (((x) & 0xff) << 0)
#define FTLCDC210_CM2_SHARP_TH1(x)      (((x) & 0xff) << 8)
#define FTLCDC210_CM2_K0(x)             (((x) & 0xf) << 16)
#define FTLCDC210_CM2_K1(x)             (((x) & 0xf) << 20)

/*
 * Color Management 3
 */
#define FTLCDC210_CM3_CONTRAST_OFFSET(x)  (((x) & 0xfff) << 0)
#define FTLCDC210_CM3_CONTRAST_NEGATIVE   (1 << 12)
#define FTLCDC210_CM3_CONTRAST_SLOPE(x)   (((x) & 0x1f) << 16)

/*
 * Scale Input Horizontal Resolution
 */
#define FTLCDC210_SCALE_IN_HRES_MASK      0xfff

/*
 * Scale Input Vertical Resolution
 */
#define FTLCDC210_SCALE_IN_VRES_MASK      0xfff

/*
 * Scale Output Horizontal Resolution
 */
#define FTLCDC210_SCALE_OUT_HRES_MASK     0x3fff

/*
 * Scale Output Vertical Resolution
 */
#define FTLCDC210_SCALE_OUT_VRES_MASK     0x3fff

/*
 * Scale Resolution Parameters
 */
#define FTLCDC210_SCALE_PAR(h, v)         (((v) & 0xff) | (((h) & 0xff) << 8))

#ifdef   CONFIG_FTLCDC210_COMPLEX_OSD

/*
 * Complex OSD configuration
 */

/*
 * OSD Window On/Off
 */
#define FTLCDC210_OSD_ENABLE_W1           (1 << 0)
#define FTLCDC210_OSD_ENABLE_W2           (1 << 1)
#define FTLCDC210_OSD_ENABLE_W3           (1 << 2)
#define FTLCDC210_OSD_ENABLE_W4           (1 << 3)

/*
 * OSD Font Control 0
 */
#define FTLCDC210_OSD_FONT0_VZOOM_EN         (1 << 0)
#define FTLCDC210_OSD_FONT0_VZOOM_IN         (1 << 1)
#define FTLCDC210_OSD_FONT0_VZOOM_IN_X1      (0 << 2)
#define FTLCDC210_OSD_FONT0_VZOOM_IN_X2      (1 << 2)
#define FTLCDC210_OSD_FONT0_VZOOM_IN_X3      (2 << 2)
#define FTLCDC210_OSD_FONT0_VZOOM_IN_X4      (3 << 2)
#define FTLCDC210_OSD_FONT0_VZOOM_OUT_HALF   (1 << 4)
#define FTLCDC210_OSD_FONT0_HZOOM_EN         (1 << 5)
#define FTLCDC210_OSD_FONT0_HZOOM_IN         (1 << 6)
#define FTLCDC210_OSD_FONT0_HZOOM_IN_X1      (0 << 7)
#define FTLCDC210_OSD_FONT0_HZOOM_IN_X2      (1 << 7)
#define FTLCDC210_OSD_FONT0_HZOOM_IN_X3      (2 << 7)
#define FTLCDC210_OSD_FONT0_HZOOM_IN_X4      (3 << 7)
#define FTLCDC210_OSD_FONT0_HZOOM_OUT_HALF   (1 << 9)
#define FTLCDC210_OSD_FONT0_TRANSPARENCY0    (0 << 10)
#define FTLCDC210_OSD_FONT0_TRANSPARENCY25   (1 << 10)
#define FTLCDC210_OSD_FONT0_TRANSPARENCY50   (2 << 10)
#define FTLCDC210_OSD_FONT0_TRANSPARENCY75   (3 << 10)
#define FTLCDC210_OSD_FONT0_TRANSPARENCY100  (4 << 10)
#define FTLCDC210_OSD_FONT0_TYPE_NORMAL      (0 << 13)
#define FTLCDC210_OSD_FONT0_TYPE_BORDER      (1 << 13)
#define FTLCDC210_OSD_FONT0_TYPE_SHADOW      (2 << 13)
#define FTLCDC210_OSD_FONT0_BORDER_COLOR(x)  (((x) & 0xf) << 16)
#define FTLCDC210_OSD_FONT0_SHADOW_COLOR(x)  (((x) & 0xf) << 20)

/*
 * OSD Font Control 1
 */
#define FTLCDC210_OSD_FONT1_ROWSPACE_EN      (1 << 0)
#define FTLCDC210_OSD_FONT1_ROWSPACE(x)      (((x) & 0xf) << 1)
#define FTLCDC210_OSD_FONT1_COLSPACE_EN      (1 << 8)
#define FTLCDC210_OSD_FONT1_COLSPACE(x)      (((x) & 0xf) << 9)

/*
 * OSD Window 1/2/3/4 Control 1
 */
#define FTLCDC210_OSD_WC1_XDIM(x)            (((x) & 0x7f) << 0)
#define FTLCDC210_OSD_WC1_YDIM(y)            (((y) & 0x7f) << 8)
#define FTLCDC210_OSD_WC1_HIGHLIGHT          (1 << 16)
#define FTLCDC210_OSD_WC1_SHADOW             (1 << 17)
#define FTLCDC210_OSD_WC1_BORDER_WIDTH(x)    (((x) & 0x3) << 18)
#define FTLCDC210_OSD_WC1_BORDER_COLOR(x)    (((x) & 0xf) << 20)
#define FTLCDC210_OSD_WC1_SHADOW_WIDTH(x)    (((x) & 0x3) << 24)
#define FTLCDC210_OSD_WC1_SHADOW_COLOR(x)    (((x) & 0xf) << 26)

/*
 * OSD Window 1/2/3/4 Control 2
 */
#define FTLCDC210_OSD_WC2_FONT_BASE(x)       ((x) & 0x1ff)

/*
 * OSD Multi-Color Font Base Address
 */
#define FTLCDC210_OSD_MCFB(x)                ((x) & 0x3fff)

/*
 * OSD Palette Color 0/1/2/3/4/5/6/7/8/9/10/11/12/13/14/15
 */
#define FTLCDC210_OSD_PC_R(x)                (((x) & 0xff) << 0)
#define FTLCDC210_OSD_PC_G(x)                (((x) & 0xff) << 8)
#define FTLCDC210_OSD_PC_B(x)                (((x) & 0xff) << 16)

#else /* CONFIG_FTLCDC210_COMPLEX_OSD */

/*
 * Simple OSD Configuration
 */

/*
 * OSD Scaling and Dimension Control
 */
#define FTLCDC210_OSD_SDC_VSCALE_X1         (0 << 0)
#define FTLCDC210_OSD_SDC_VSCALE_X2         (1 << 0)
#define FTLCDC210_OSD_SDC_VSCALE_X3         (2 << 0)
#define FTLCDC210_OSD_SDC_VSCALE_X4         (3 << 0)
#define FTLCDC210_OSD_SDC_HSCALE_X1         (0 << 2)
#define FTLCDC210_OSD_SDC_HSCALE_X2         (1 << 2)
#define FTLCDC210_OSD_SDC_HSCALE_X3         (2 << 2)
#define FTLCDC210_OSD_SDC_HSCALE_X4         (3 << 2)
#define FTLCDC210_OSD_SDC_VDIM(x)           (((x) & 0x1f) << 4)
#define FTLCDC210_OSD_SDC_HDIM(x)           (((x) & 0x3f) << 12)

/*
 * OSD Position Control
 */
#define FTLCDC210_OSD_POS_V(x)              (((x) & 0x7ff) << 0)
#define FTLCDC210_OSD_POS_H(x)              (((x) & 0xfff) << 12)

/*
 * OSD Foreground Color Control
 */
#define FTLCDC210_OSD_FG_PAL0(x)            (((x) & 0xff) << 0)
#define FTLCDC210_OSD_FG_PAL1(x)            (((x) & 0xff) << 8)
#define FTLCDC210_OSD_FG_PAL2(x)            (((x) & 0xff) << 16)
#define FTLCDC210_OSD_FG_PAL3(x)            (((x) & 0xff) << 24)

/*
 * OSD Background Color Control
 */
#define FTLCDC210_OSD_BG_TRANSPARENCY25     (0 << 0)
#define FTLCDC210_OSD_BG_TRANSPARENCY50     (1 << 0)
#define FTLCDC210_OSD_BG_TRANSPARENCY75     (2 << 0)
#define FTLCDC210_OSD_BG_TRANSPARENCY100    (3 << 0)
#define FTLCDC210_OSD_BG_PAL1(x)            (((x) & 0xff) << 8)
#define FTLCDC210_OSD_BG_PAL2(x)            (((x) & 0xff) << 16)
#define FTLCDC210_OSD_BG_PAL3(x)            (((x) & 0xff) << 24)

#endif   /* CONFIG_FTLCDC210_COMPLEX_OSD */

extern void FTLCDC210_fb_fillrect(struct fb_info *p, const struct fb_fillrect *rect);
extern void FTLCDC210_fb_imageblit(struct fb_info *p, const struct fb_image *image);
extern void FTLCDC210_fb_copyarea(struct fb_info *p, const struct fb_copyarea *area);


#endif   /* __FTLCDC210_H */
