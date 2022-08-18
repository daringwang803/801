/*
 * Driver for the Faraday A380 pin controller
 *
 * (C) Copyright 2014 Faraday Technology
 * Yan-Pai Chen <ypchen@faraday-tech.com>
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-ftscu010.h"

#define A380_PMUR_MFS0	0x1120
#define A380_PMUR_MFS1	0x1124
#define A380_PMUR_MFS2	0x1128
#define A380_PMUR_MFS3	0x112c

/*
 * Pins
 */
enum {
	A380_PIN_X_GPIO0,
	A380_PIN_X_GPIO1,
	A380_PIN_X_GPIO2,
	A380_PIN_X_GPIO3,
	A380_PIN_X_GPIO4,
	A380_PIN_X_GPIO5,
	A380_PIN_X_GPIO6,
	A380_PIN_X_GPIO7,
	A380_PIN_X_EXT_INT0,
	A380_PIN_X_EXT_INT1,
	A380_PIN_X_RESERV0,	/* a placeholder, not exist */
	A380_PIN_X_TSCLK,
	A380_PIN_X_HM4_BURSREQ,
	A380_PIN_X_HM4_HLOCK,
	A380_PIN_X_HM4_HGRANT,
	A380_PIN_X_HS1_HSEL,
	A380_PIN_X_HS2_HSEL,
	A380_PIN_X_HS3_HSEL,
	A380_PIN_X_HS4_HSEL,
	A380_PIN_X_HS5_HSEL,
	A380_PIN_X_HS6_HSEL,
	A380_PIN_X_HS7_HSEL,
	A380_PIN_X_HS8_HSEL,
	A380_PIN_X_LCD_CLK,
	A380_PIN_X_LCD_DAT0,
	A380_PIN_X_LCD_DAT1,
	A380_PIN_X_LCD_DAT2,
	A380_PIN_X_LCD_DAT3,
	A380_PIN_X_LCD_DAT4,
	A380_PIN_X_LCD_DAT5,
	A380_PIN_X_LCD_DAT6,
	A380_PIN_X_LCD_DAT7,
	A380_PIN_X_LCD_DAT8,
	A380_PIN_X_LCD_DAT9,
	A380_PIN_X_LCD_DAT10,
	A380_PIN_X_LCD_DAT11,
	A380_PIN_X_LCD_DAT12,
	A380_PIN_X_LCD_DAT13,
	A380_PIN_X_LCD_DAT14,
	A380_PIN_X_LCD_DAT15,
	A380_PIN_X_LCD_PCLK,
	A380_PIN_X_I2C_SCL,
	A380_PIN_X_I2C_SDA,
	A380_PIN_X_SSP_RXD,
	A380_PIN_X_SSP_TXD,
	A380_PIN_X_SSP_SCLK,
	A380_PIN_X_SSP_FS,
	A380_PIN_X_FFUART_RXD,
	A380_PIN_X_FFUART_TXD,
	A380_PIN_X_FFUART_CTS,
	A380_PIN_X_FFUART_RTS,
	A380_PIN_X_FFUART_DSR,
	A380_PIN_X_FFUART_DCD,
	A380_PIN_X_FFUART_RI,
	A380_PIN_X_FFUART_DTR,
	A380_PIN_X_GM_MDC,
	A380_PIN_X_GM_MDIO,
	A380_PIN_X_LCD_DAT18,
	A380_PIN_X_LCD_DAT19,
	A380_PIN_X_LCD_DAT20,
	A380_PIN_X_LCD_DAT21,
	A380_PIN_X_LCD_DAT22,
	A380_PIN_X_LCD_DAT23,
};

#define A380_PINCTRL_PIN(x) PINCTRL_PIN(A380_PIN_ ## x, #x)

static const struct pinctrl_pin_desc a380_pins[] = {
	A380_PINCTRL_PIN(X_GPIO0),
	A380_PINCTRL_PIN(X_GPIO1),
	A380_PINCTRL_PIN(X_GPIO2),
	A380_PINCTRL_PIN(X_GPIO3),
	A380_PINCTRL_PIN(X_GPIO4),
	A380_PINCTRL_PIN(X_GPIO5),
	A380_PINCTRL_PIN(X_GPIO6),
	A380_PINCTRL_PIN(X_GPIO7),
	A380_PINCTRL_PIN(X_EXT_INT0),
	A380_PINCTRL_PIN(X_EXT_INT1),
	A380_PINCTRL_PIN(X_RESERV0),	/* a placeholder, not exist */
	A380_PINCTRL_PIN(X_TSCLK),
	A380_PINCTRL_PIN(X_HM4_BURSREQ),
	A380_PINCTRL_PIN(X_HM4_HLOCK),
	A380_PINCTRL_PIN(X_HM4_HGRANT),
	A380_PINCTRL_PIN(X_HS1_HSEL),
	A380_PINCTRL_PIN(X_HS2_HSEL),
	A380_PINCTRL_PIN(X_HS3_HSEL),
	A380_PINCTRL_PIN(X_HS4_HSEL),
	A380_PINCTRL_PIN(X_HS5_HSEL),
	A380_PINCTRL_PIN(X_HS6_HSEL),
	A380_PINCTRL_PIN(X_HS7_HSEL),
	A380_PINCTRL_PIN(X_HS8_HSEL),
	A380_PINCTRL_PIN(X_LCD_CLK),
	A380_PINCTRL_PIN(X_LCD_DAT0),
	A380_PINCTRL_PIN(X_LCD_DAT1),
	A380_PINCTRL_PIN(X_LCD_DAT2),
	A380_PINCTRL_PIN(X_LCD_DAT3),
	A380_PINCTRL_PIN(X_LCD_DAT4),
	A380_PINCTRL_PIN(X_LCD_DAT5),
	A380_PINCTRL_PIN(X_LCD_DAT6),
	A380_PINCTRL_PIN(X_LCD_DAT7),
	A380_PINCTRL_PIN(X_LCD_DAT8),
	A380_PINCTRL_PIN(X_LCD_DAT9),
	A380_PINCTRL_PIN(X_LCD_DAT10),
	A380_PINCTRL_PIN(X_LCD_DAT11),
	A380_PINCTRL_PIN(X_LCD_DAT12),
	A380_PINCTRL_PIN(X_LCD_DAT13),
	A380_PINCTRL_PIN(X_LCD_DAT14),
	A380_PINCTRL_PIN(X_LCD_DAT15),
	A380_PINCTRL_PIN(X_LCD_PCLK),
	A380_PINCTRL_PIN(X_I2C_SCL),
	A380_PINCTRL_PIN(X_I2C_SDA),
	A380_PINCTRL_PIN(X_SSP_RXD),
	A380_PINCTRL_PIN(X_SSP_TXD),
	A380_PINCTRL_PIN(X_SSP_SCLK),
	A380_PINCTRL_PIN(X_SSP_FS),
	A380_PINCTRL_PIN(X_FFUART_RXD),
	A380_PINCTRL_PIN(X_FFUART_TXD),
	A380_PINCTRL_PIN(X_FFUART_CTS),
	A380_PINCTRL_PIN(X_FFUART_RTS),
	A380_PINCTRL_PIN(X_FFUART_DSR),
	A380_PINCTRL_PIN(X_FFUART_DCD),
	A380_PINCTRL_PIN(X_FFUART_RI),
	A380_PINCTRL_PIN(X_FFUART_DTR),
	A380_PINCTRL_PIN(X_GM_MDC),
	A380_PINCTRL_PIN(X_GM_MDIO),
	A380_PINCTRL_PIN(X_LCD_DAT18),
	A380_PINCTRL_PIN(X_LCD_DAT19),
	A380_PINCTRL_PIN(X_LCD_DAT20),
	A380_PINCTRL_PIN(X_LCD_DAT21),
	A380_PINCTRL_PIN(X_LCD_DAT22),
	A380_PINCTRL_PIN(X_LCD_DAT23),
};

/*
 * Pin groups
 */
static const unsigned ftahbb020_pins[] = {
	A380_PIN_X_HS1_HSEL,
	A380_PIN_X_HS2_HSEL,
	A380_PIN_X_HS3_HSEL,
	A380_PIN_X_HS4_HSEL,
	A380_PIN_X_HS8_HSEL,
	A380_PIN_X_GM_MDC,
};

static const unsigned ftahbwrap_pins[] = {
};

static const unsigned ftdmac020_pins[] = {
	A380_PIN_X_HS1_HSEL,
	A380_PIN_X_HS2_HSEL,
	A380_PIN_X_HS3_HSEL,
	A380_PIN_X_HS4_HSEL,
};

static const unsigned ftgpio010_pins[] = {
	A380_PIN_X_GPIO0,
	A380_PIN_X_GPIO1,
	A380_PIN_X_GPIO2,
	A380_PIN_X_GPIO3,
	A380_PIN_X_GPIO4,
	A380_PIN_X_GPIO5,
	A380_PIN_X_GPIO6,
	A380_PIN_X_GPIO7,
};

static const unsigned fti2c010_0_pins[] = {
};

static const unsigned fti2c010_1_pins[] = {
	A380_PIN_X_FFUART_RXD,
	A380_PIN_X_FFUART_TXD,
};

static const unsigned ftintc030_pins[] = {
};

static const unsigned ftkbc010_pins[] = {
	A380_PIN_X_GPIO0,
	A380_PIN_X_GPIO1,
	A380_PIN_X_GPIO2,
	A380_PIN_X_GPIO3,
	A380_PIN_X_GPIO4,
	A380_PIN_X_GPIO5,
	A380_PIN_X_GPIO6,
	A380_PIN_X_GPIO7,
	A380_PIN_X_EXT_INT1,
	A380_PIN_X_HS7_HSEL,
	A380_PIN_X_HS8_HSEL,
	A380_PIN_X_LCD_DAT19,
	A380_PIN_X_LCD_DAT22,
};

static const unsigned ftlcd210_pins[] = {
};

static const unsigned ftlcd210tv_pins[] = {

};

static const unsigned ftnand024_pins[] = {
};

static const unsigned ftssp010_0_pins[] = {
	A380_PIN_X_TSCLK,
};

static const unsigned ftssp010_1_pins[] = {
	//A380_PIN_X_HS8_HSEL, // Conflict with KBC
	A380_PIN_X_FFUART_CTS,
	A380_PIN_X_FFUART_RTS,
	A380_PIN_X_FFUART_DSR,
	A380_PIN_X_FFUART_DCD,
};

static const unsigned ftssp010_2_pins[] = {
};

static const unsigned ftuart010_0_irda_pins[] = {
};

static const unsigned ftuart010_1_pins[] = {
	A380_PIN_X_FFUART_RXD,
	A380_PIN_X_FFUART_TXD,
	A380_PIN_X_FFUART_CTS,
	A380_PIN_X_FFUART_RTS,
	A380_PIN_X_FFUART_DSR,
};

static const unsigned ftuart010_2_pins[] = {
};

static const unsigned gbe_pins[] = {
	A380_PIN_X_TSCLK,
	A380_PIN_X_GM_MDC,
};

static const unsigned vcap300_pins[] = {
};

/* Pin groups */
#define GROUP(gname)					\
	{						\
		.name = #gname,				\
		.pins = gname##_pins,			\
		.npins = ARRAY_SIZE(gname##_pins),	\
	}

static const struct ftscu010_pin_group a380_groups[] = {
	GROUP(ftahbb020),
	GROUP(ftahbwrap),
	GROUP(ftdmac020),
	GROUP(ftgpio010),
	GROUP(fti2c010_0),
	GROUP(fti2c010_1),
	GROUP(ftintc030),
	GROUP(ftkbc010),
	GROUP(ftlcd210),
	GROUP(ftlcd210tv),
	GROUP(ftnand024),
	GROUP(ftssp010_0),
	GROUP(ftssp010_1),
	GROUP(ftssp010_2),
	GROUP(ftuart010_0_irda),
	GROUP(ftuart010_1),
	GROUP(ftuart010_2),
	GROUP(gbe),
	GROUP(vcap300),
};


/* Pin function groups */
static const char * const ftahbb020_groups[] = { "ftahbb020" };
static const char * const ftahbwrap_groups[] = { "ftahbwrap" };
static const char * const ftdmac020_groups[] = { "ftdmac020" };
static const char * const ftgpio010_groups[] = { "ftgpio010" };
static const char * const fti2c010_0_groups[] = { "fti2c010_0" };
static const char * const fti2c010_1_groups[] = { "fti2c010_1" };
static const char * const ftintc030_groups[] = { "ftinct030" };
static const char * const ftkbc010_groups[] = { "ftkbc010" };
static const char * const ftlcd210_groups[] = { "ftlcd210" };
static const char * const ftlcd210tv_groups[] = { "ftlcd210tv" };
static const char * const ftnand024_groups[] = { "ftnand024" };
static const char * const ftssp010_0_groups[] = { "ftssp010_0" };
static const char * const ftssp010_1_groups[] = { "ftssp010_1" };
static const char * const ftssp010_2_groups[] = { "ftssp010_2" };
static const char * const ftuart010_0_irda_groups[] = { "ftuart010_0_irda" };
static const char * const ftuart010_1_groups[] = { "ftuart010_1" };
static const char * const ftuart010_2_groups[] = { "ftuart010_2" };
static const char * const gbe_groups[] = { "gbe" };
static const char * const na_groups[] = { /* N/A */ };
static const char * const vcap300_groups[] = { "vcap300" };

/* Mux functions */
enum {
	A380_MUX_FTAHBB020,
	A380_MUX_FTAHBWRAP,
	A380_MUX_FTDMAC020,
	A380_MUX_FTGPIO010,
	A380_MUX_FTI2C010_0,
	A380_MUX_FTI2C010_1,
	A380_MUX_FTINTC030,
	A380_MUX_FTKBC010,
	A380_MUX_FTLCD210,
	A380_MUX_FTLCD210TV,
	A380_MUX_FTNAND024,
	A380_MUX_FTSSP010_0,
	A380_MUX_FTSSP010_1,
	A380_MUX_FTSSP010_2,
	A380_MUX_FTUART010_0,
	A380_MUX_FTUART010_1,
	A380_MUX_FTUART010_2,
	A380_MUX_GBE,
	A380_MUX_NA,		/* N/A */
	A380_MUX_VCAP300,
};

#define FUNCTION(fname)					\
	{						\
		.name = #fname,				\
		.groups = fname##_groups,		\
		.ngroups = ARRAY_SIZE(fname##_groups),	\
	}

static const struct ftscu010_pmx_function a380_pmx_functions[] = {
	FUNCTION(ftahbb020),
	FUNCTION(ftahbwrap),
	FUNCTION(ftdmac020),
	FUNCTION(ftgpio010),
	FUNCTION(fti2c010_0),
	FUNCTION(fti2c010_1),
	FUNCTION(ftintc030),
	FUNCTION(ftkbc010),
	FUNCTION(ftlcd210),
	FUNCTION(ftlcd210tv),
	FUNCTION(ftnand024),
	FUNCTION(ftssp010_0),
	FUNCTION(ftssp010_1),
	FUNCTION(ftssp010_2),
	FUNCTION(ftuart010_0_irda),
	FUNCTION(ftuart010_1),
	FUNCTION(ftuart010_2),
	FUNCTION(gbe),
	FUNCTION(na),
	FUNCTION(vcap300),
};

#define A380_PIN(pin, reg, f0, f1, f2, f3)		\
	[A380_PIN_ ## pin] = {				\
			.offset = A380_PMUR_ ## reg,	\
			.functions = {			\
				A380_MUX_ ## f0,	\
				A380_MUX_ ## f1,	\
				A380_MUX_ ## f2,	\
				A380_MUX_ ## f3,	\
			},				\
		}

static struct ftscu010_pin a380_pinmux_map[] = {
	/*	 pin	 		reg	f0		f1		f2		f3 */
	A380_PIN(X_GPIO0, 		MFS0,	NA, 		FTGPIO010,	FTKBC010,	NA),
	A380_PIN(X_GPIO1, 		MFS0, 	FTGPIO010, 	FTKBC010,	NA, 		NA),
	A380_PIN(X_GPIO2, 		MFS0, 	FTGPIO010, 	FTKBC010, 	NA, 		NA),
	A380_PIN(X_GPIO3, 		MFS0,	FTGPIO010, 	FTKBC010, 	NA, 		NA),
	A380_PIN(X_GPIO4, 		MFS0, 	FTGPIO010, 	FTKBC010, 	NA, 		NA),
	A380_PIN(X_GPIO5, 		MFS0, 	FTGPIO010, 	FTKBC010, 	NA, 		NA),
	A380_PIN(X_GPIO6, 		MFS0, 	FTGPIO010, 	FTKBC010, 	NA, 		NA),
	A380_PIN(X_GPIO7, 		MFS0, 	FTGPIO010, 	FTKBC010, 	NA, 		NA),
	A380_PIN(X_EXT_INT0,	MFS0, 	NA, 		NA, 		NA,		 	NA),
	A380_PIN(X_EXT_INT1,	MFS0, 	NA, 		FTKBC010, 	NA, 		NA),
	A380_PIN(X_RESERV0,		MFS0, 	NA, 		NA, 		NA, 		NA),
	A380_PIN(X_TSCLK,		MFS0, 	GBE, 		FTSSP010_0, NA, 		NA),
	A380_PIN(X_HM4_BURSREQ,	MFS0, 	NA,			NA,		 	NA,		 	NA),
	A380_PIN(X_HM4_HLOCK,	MFS0, 	NA,			NA,		 	NA, 		NA),
	A380_PIN(X_HM4_HGRANT,	MFS0, 	NA,			NA, 		NA, 		NA),
	A380_PIN(X_HS1_HSEL,	MFS0, 	FTAHBB020, 	NA, 		FTDMAC020, 	NA),
	A380_PIN(X_HS2_HSEL,	MFS1, 	FTAHBB020,	NA, 		FTDMAC020, 	NA),
	A380_PIN(X_HS3_HSEL,	MFS1, 	FTAHBB020, 	NA, 		FTDMAC020, 	NA),
	A380_PIN(X_HS4_HSEL,	MFS1, 	FTAHBB020, 	NA, 		FTDMAC020, 	NA),
	A380_PIN(X_HS5_HSEL,	MFS1, 	NA, 		NA, 		NA, 		NA),
	A380_PIN(X_HS6_HSEL,	MFS1, 	NA, 		NA, 		NA, 		NA),
	A380_PIN(X_HS7_HSEL,	MFS1, 	NA, 		FTKBC010, 	NA, 		NA),
	A380_PIN(X_HS8_HSEL,	MFS1, 	FTAHBB020, 	FTKBC010, 	FTSSP010_1, NA),
	A380_PIN(X_LCD_CLK,		MFS1, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT0,	MFS1, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT1,	MFS1, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT2,	MFS1, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT3,	MFS1, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT4,	MFS1, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT5,	MFS1, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT6,	MFS1, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT7,	MFS1, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT8,	MFS2, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT9,	MFS2, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT10,	MFS2, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT11,	MFS2, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT12,	MFS2, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT13,	MFS2, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT14,	MFS2, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT15,	MFS2, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_PCLK,	MFS2, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_I2C_SCL,		MFS2, 	NA,			NA, 		NA, 		NA),
	A380_PIN(X_I2C_SDA,		MFS2, 	NA, 		NA, 		NA, 		NA),
	A380_PIN(X_SSP_RXD,		MFS2, 	NA, 		NA, 		NA, 		NA),
	A380_PIN(X_SSP_TXD,		MFS2, 	NA, 		NA, 		NA, 		NA),
	A380_PIN(X_SSP_SCLK,	MFS2, 	NA, 		NA, 		NA, 		NA),
	A380_PIN(X_SSP_FS,		MFS2, 	NA, 		NA, 		NA, 		NA),
	A380_PIN(X_FFUART_RXD,	MFS2, 	FTUART010_1,FTI2C010_1, NA, 		NA),
	A380_PIN(X_FFUART_TXD,	MFS3, 	FTUART010_1,FTI2C010_1, NA,			NA),
	A380_PIN(X_FFUART_CTS,	MFS3, 	FTUART010_1,FTSSP010_1, NA, 		NA),
	A380_PIN(X_FFUART_RTS,	MFS3, 	FTUART010_1,FTSSP010_1,	NA, 		NA),
	A380_PIN(X_FFUART_DSR,	MFS3, 	FTUART010_1,FTSSP010_1, NA, 		NA),
	A380_PIN(X_FFUART_DCD,	MFS3, 	NA,			FTSSP010_1,	NA, 		FTUART010_2),
	A380_PIN(X_FFUART_RI,	MFS3, 	NA,			FTUART010_2,NA, 		NA),
	A380_PIN(X_FFUART_DTR,	MFS3, 	NA,			NA,			NA, 		NA),
	A380_PIN(X_GM_MDC, 		MFS3, 	FTAHBB020,	GBE,        NA, 		NA),
	A380_PIN(X_GM_MDIO, 	MFS3, 	NA, 		NA, 		NA, 		NA),
	A380_PIN(X_LCD_DAT18,	MFS3, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT19,	MFS3, 	NA, 		FTKBC010,	NA, 		NA),
	A380_PIN(X_LCD_DAT20,	MFS3, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT21,	MFS3, 	NA, 		NA,			NA, 		NA),
	A380_PIN(X_LCD_DAT22,	MFS3, 	NA, 		FTKBC010, 	NA, 		NA),
	A380_PIN(X_LCD_DAT23,	MFS3, 	NA, 		NA, 		NA, 		NA),
};

static const struct ftscu010_pinctrl_soc_data a380_pinctrl_soc_data = {
	.pins = a380_pins,
	.npins = ARRAY_SIZE(a380_pins),
	.functions = a380_pmx_functions,
	.nfunctions = ARRAY_SIZE(a380_pmx_functions),
	.groups = a380_groups,
	.ngroups = ARRAY_SIZE(a380_groups),
	.map = a380_pinmux_map,
};

static int a380_pinctrl_probe(struct platform_device *pdev)
{
	return ftscu010_pinctrl_probe(pdev, &a380_pinctrl_soc_data);
}

static struct of_device_id faraday_pinctrl_of_match[] = {
	{ .compatible = "ftscu010-pinmux", },
	{ },
};

static struct platform_driver a380_pinctrl_driver = {
	.driver = {
		.name = "pinctrl-a380",
		.owner = THIS_MODULE,
		.of_match_table = faraday_pinctrl_of_match,
	},
	.remove = ftscu010_pinctrl_remove,
};

static int __init a380_pinctrl_init(void)
{
	return platform_driver_probe(&a380_pinctrl_driver, a380_pinctrl_probe);
}
arch_initcall(a380_pinctrl_init);

static void __exit a380_pinctrl_exit(void)
{
	platform_driver_unregister(&a380_pinctrl_driver);
}
module_exit(a380_pinctrl_exit);

MODULE_AUTHOR("Yan-Pai Chen <ypchen@faraday-tech.com");
MODULE_DESCRIPTION("Faraday A380 pinctrl driver");
MODULE_LICENSE("GPL");
