/*
 *  Driver for Faraday TC12NGRC pin controller
 *
 *  Copyright (C) 2015 Faraday Technology
 *  Copyright (C) 2019 Faraday Linux Automation Tool
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
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-ftscu010.h"

#define TC12NGRC_PMUR_MFS0 0x820
#define TC12NGRC_PMUR_MFS1 0x824

/* Pins */
enum{
	TC12NGRC_PIN_X_PWM_TMR_0,
	TC12NGRC_PIN_X_PWM_TMR_1,
	TC12NGRC_PIN_X_PWM_TMR_2,
	TC12NGRC_PIN_X_PWM_TMR_3,
	TC12NGRC_PIN_X_I2C_SDA,
	TC12NGRC_PIN_X_I2C_SCL,
	TC12NGRC_PIN_X_SPI1_RXD,
	TC12NGRC_PIN_X_SPI1_TXD,
	TC12NGRC_PIN_X_SPI1_SCLK,
	TC12NGRC_PIN_X_SPI1_CS0,
	TC12NGRC_PIN_X_UART_SI,
	TC12NGRC_PIN_X_UART_SO,
	TC12NGRC_PIN_X_UART1_SI,
	TC12NGRC_PIN_X_UART1_SO,
	TC12NGRC_PIN_X_USART_RXD,
	TC12NGRC_PIN_X_USART_TXD,
	TC12NGRC_PIN_X_USART_CTS,
	TC12NGRC_PIN_X_USART_RTS,
	TC12NGRC_PIN_X_USART_SCK,
	TC12NGRC_PIN_X_SPI1_CS1,
	TC12NGRC_PIN_X_CPU_TDI,
	TC12NGRC_PIN_X_CPU_TMS,
	TC12NGRC_PIN_X_CPU_TCK,
	TC12NGRC_PIN_X_CPU_NTRST,
	TC12NGRC_PIN_X_CPU_TDO,
	TC12NGRC_PIN_X_CPU_DBGRQ,
	TC12NGRC_PIN_X_CPU_DBGACK,
	TC12NGRC_PIN_X_UARTMPB_SOUT,
	TC12NGRC_PIN_X_UARTMPB_SIN,
	TC12NGRC_PIN_X_SPI1_CS2,
	TC12NGRC_PIN_X_SPI1_CS3,
};

#define TC12NGRC_PINCTRL_PIN(x) PINCTRL_PIN(TC12NGRC_PIN_ ## x, #x)

static const struct pinctrl_pin_desc tc12ngrc_pins[] = {
	TC12NGRC_PINCTRL_PIN(X_PWM_TMR_0),
	TC12NGRC_PINCTRL_PIN(X_PWM_TMR_1),
	TC12NGRC_PINCTRL_PIN(X_PWM_TMR_2),
	TC12NGRC_PINCTRL_PIN(X_PWM_TMR_3),
	TC12NGRC_PINCTRL_PIN(X_I2C_SDA),
	TC12NGRC_PINCTRL_PIN(X_I2C_SCL),
	TC12NGRC_PINCTRL_PIN(X_SPI1_RXD),
	TC12NGRC_PINCTRL_PIN(X_SPI1_TXD),
	TC12NGRC_PINCTRL_PIN(X_SPI1_SCLK),
	TC12NGRC_PINCTRL_PIN(X_SPI1_CS0),
	TC12NGRC_PINCTRL_PIN(X_UART_SI),
	TC12NGRC_PINCTRL_PIN(X_UART_SO),
	TC12NGRC_PINCTRL_PIN(X_UART1_SI),
	TC12NGRC_PINCTRL_PIN(X_UART1_SO),
	TC12NGRC_PINCTRL_PIN(X_USART_RXD),
	TC12NGRC_PINCTRL_PIN(X_USART_TXD),
	TC12NGRC_PINCTRL_PIN(X_USART_CTS),
	TC12NGRC_PINCTRL_PIN(X_USART_RTS),
	TC12NGRC_PINCTRL_PIN(X_USART_SCK),
	TC12NGRC_PINCTRL_PIN(X_SPI1_CS1),
	TC12NGRC_PINCTRL_PIN(X_CPU_TDI),
	TC12NGRC_PINCTRL_PIN(X_CPU_TMS),
	TC12NGRC_PINCTRL_PIN(X_CPU_TCK),
	TC12NGRC_PINCTRL_PIN(X_CPU_NTRST),
	TC12NGRC_PINCTRL_PIN(X_CPU_TDO),
	TC12NGRC_PINCTRL_PIN(X_CPU_DBGRQ),
	TC12NGRC_PINCTRL_PIN(X_CPU_DBGACK),
	TC12NGRC_PINCTRL_PIN(X_UARTMPB_SOUT),
	TC12NGRC_PINCTRL_PIN(X_UARTMPB_SIN),
	TC12NGRC_PINCTRL_PIN(X_SPI1_CS2),
	TC12NGRC_PINCTRL_PIN(X_SPI1_CS3),
};

/* Pin groups */
static const unsigned fa616te_pins[] = {
	TC12NGRC_PIN_X_CPU_DBGACK,
	TC12NGRC_PIN_X_CPU_DBGRQ,
	TC12NGRC_PIN_X_CPU_NTRST,
	TC12NGRC_PIN_X_CPU_TCK,
	TC12NGRC_PIN_X_CPU_TDI,
	TC12NGRC_PIN_X_CPU_TDO,
	TC12NGRC_PIN_X_CPU_TMS,
};
static const unsigned ftgpio010_pins[] = {
	TC12NGRC_PIN_X_CPU_DBGACK,
	TC12NGRC_PIN_X_CPU_DBGRQ,
	TC12NGRC_PIN_X_CPU_NTRST,
	TC12NGRC_PIN_X_CPU_TCK,
	TC12NGRC_PIN_X_CPU_TDI,
	TC12NGRC_PIN_X_CPU_TDO,
	TC12NGRC_PIN_X_CPU_TMS,
	TC12NGRC_PIN_X_I2C_SCL,
	TC12NGRC_PIN_X_I2C_SDA,
	TC12NGRC_PIN_X_PWM_TMR_0,
	TC12NGRC_PIN_X_PWM_TMR_1,
	TC12NGRC_PIN_X_PWM_TMR_2,
	TC12NGRC_PIN_X_PWM_TMR_3,
	TC12NGRC_PIN_X_SPI1_CS0,
	TC12NGRC_PIN_X_SPI1_CS1,
	TC12NGRC_PIN_X_SPI1_CS2,
	TC12NGRC_PIN_X_SPI1_CS3,
	TC12NGRC_PIN_X_SPI1_RXD,
	TC12NGRC_PIN_X_SPI1_SCLK,
	TC12NGRC_PIN_X_SPI1_TXD,
	TC12NGRC_PIN_X_UART1_SI,
	TC12NGRC_PIN_X_UART1_SO,
	TC12NGRC_PIN_X_UARTMPB_SIN,
	TC12NGRC_PIN_X_UARTMPB_SOUT,
	TC12NGRC_PIN_X_UART_SI,
	TC12NGRC_PIN_X_UART_SO,
	TC12NGRC_PIN_X_USART_CTS,
	TC12NGRC_PIN_X_USART_RTS,
	TC12NGRC_PIN_X_USART_RXD,
	TC12NGRC_PIN_X_USART_SCK,
	TC12NGRC_PIN_X_USART_TXD,
};
static const unsigned ftiic010_pins[] = {
	TC12NGRC_PIN_X_I2C_SCL,
	TC12NGRC_PIN_X_I2C_SDA,
};
static const unsigned ftpwmtmr010_pins[] = {
	TC12NGRC_PIN_X_PWM_TMR_0,
	TC12NGRC_PIN_X_PWM_TMR_1,
	TC12NGRC_PIN_X_PWM_TMR_2,
	TC12NGRC_PIN_X_PWM_TMR_3,
	TC12NGRC_PIN_X_USART_CTS,
};
static const unsigned ftspi020_pins[] = {
	TC12NGRC_PIN_X_SPI1_RXD,
	TC12NGRC_PIN_X_SPI1_SCLK,
	TC12NGRC_PIN_X_SPI1_TXD,
};
static const unsigned ftssp010_pins[] = {
	TC12NGRC_PIN_X_SPI1_CS0,
	TC12NGRC_PIN_X_SPI1_CS1,
	TC12NGRC_PIN_X_SPI1_CS2,
	TC12NGRC_PIN_X_SPI1_CS3,
	TC12NGRC_PIN_X_SPI1_RXD,
	TC12NGRC_PIN_X_SPI1_SCLK,
	TC12NGRC_PIN_X_SPI1_TXD,
};
static const unsigned ftuart010_pins[] = {
	TC12NGRC_PIN_X_UART_SI,
	TC12NGRC_PIN_X_UART_SO,
};
static const unsigned ftuart010_1_pins[] = {
	TC12NGRC_PIN_X_UART1_SI,
	TC12NGRC_PIN_X_UART1_SO,
};
static const unsigned ftusart010_pins[] = {
	TC12NGRC_PIN_X_USART_CTS,
	TC12NGRC_PIN_X_USART_RTS,
	TC12NGRC_PIN_X_USART_RXD,
	TC12NGRC_PIN_X_USART_SCK,
	TC12NGRC_PIN_X_USART_TXD,
};
static const unsigned mpblock_pins[] = {
	TC12NGRC_PIN_X_UARTMPB_SIN,
	TC12NGRC_PIN_X_UARTMPB_SOUT,
};
static const unsigned na_pins[] = {
	TC12NGRC_PIN_X_CPU_DBGACK,
	TC12NGRC_PIN_X_CPU_DBGRQ,
	TC12NGRC_PIN_X_CPU_NTRST,
	TC12NGRC_PIN_X_CPU_TCK,
	TC12NGRC_PIN_X_CPU_TDI,
	TC12NGRC_PIN_X_CPU_TDO,
	TC12NGRC_PIN_X_CPU_TMS,
	TC12NGRC_PIN_X_I2C_SCL,
	TC12NGRC_PIN_X_I2C_SDA,
	TC12NGRC_PIN_X_PWM_TMR_0,
	TC12NGRC_PIN_X_PWM_TMR_1,
	TC12NGRC_PIN_X_PWM_TMR_2,
	TC12NGRC_PIN_X_PWM_TMR_3,
	TC12NGRC_PIN_X_SPI1_CS0,
	TC12NGRC_PIN_X_SPI1_CS1,
	TC12NGRC_PIN_X_SPI1_CS2,
	TC12NGRC_PIN_X_SPI1_CS3,
	TC12NGRC_PIN_X_SPI1_RXD,
	TC12NGRC_PIN_X_SPI1_SCLK,
	TC12NGRC_PIN_X_SPI1_TXD,
	TC12NGRC_PIN_X_UART1_SI,
	TC12NGRC_PIN_X_UART1_SO,
	TC12NGRC_PIN_X_UARTMPB_SIN,
	TC12NGRC_PIN_X_UARTMPB_SOUT,
	TC12NGRC_PIN_X_UART_SI,
	TC12NGRC_PIN_X_UART_SO,
	TC12NGRC_PIN_X_USART_CTS,
	TC12NGRC_PIN_X_USART_RTS,
	TC12NGRC_PIN_X_USART_RXD,
	TC12NGRC_PIN_X_USART_SCK,
	TC12NGRC_PIN_X_USART_TXD,
};

#define GROUP(gname)					\
	{						\
		.name = #gname,				\
		.pins = gname##_pins,			\
		.npins = ARRAY_SIZE(gname##_pins),	\
	}
	
static const struct ftscu010_pin_group tc12ngrc_groups[] = {
	GROUP(fa616te),
	GROUP(ftgpio010),
	GROUP(ftiic010),
	GROUP(ftpwmtmr010),
	GROUP(ftspi020),
	GROUP(ftssp010),
	GROUP(ftuart010),
	GROUP(ftuart010_1),
	GROUP(ftusart010),
	GROUP(mpblock),
	GROUP(na),
};

/* Pin function groups */
static const char * const fa616te_groups[] = { "fa616te" };
static const char * const ftgpio010_groups[] = { "ftgpio010" };
static const char * const ftiic010_groups[] = { "ftiic010" };
static const char * const ftpwmtmr010_groups[] = { "ftpwmtmr010" };
static const char * const ftspi020_groups[] = { "ftspi020" };
static const char * const ftssp010_groups[] = { "ftssp010" };
static const char * const ftuart010_groups[] = { "ftuart010" };
static const char * const ftuart010_1_groups[] = { "ftuart010_1" };
static const char * const ftusart010_groups[] = { "ftusart010" };
static const char * const mpblock_groups[] = { "mpblock" };
static const char * const na_groups[] = { "na" };

/* Mux functions */
enum{
	TC12NGRC_MUX_FA616TE,
	TC12NGRC_MUX_FTGPIO010,
	TC12NGRC_MUX_FTIIC010,
	TC12NGRC_MUX_FTPWMTMR010,
	TC12NGRC_MUX_FTSPI020,
	TC12NGRC_MUX_FTSSP010,
	TC12NGRC_MUX_FTUART010,
	TC12NGRC_MUX_FTUART010_1,
	TC12NGRC_MUX_FTUSART010,
	TC12NGRC_MUX_MPBLOCK,
	TC12NGRC_MUX_NA,
};

#define FUNCTION(fname)					\
	{						\
		.name = #fname,				\
		.groups = fname##_groups,		\
		.ngroups = ARRAY_SIZE(fname##_groups),	\
	}

static const struct ftscu010_pmx_function tc12ngrc_pmx_functions[] = {
	FUNCTION(fa616te),
	FUNCTION(ftgpio010),
	FUNCTION(ftiic010),
	FUNCTION(ftpwmtmr010),
	FUNCTION(ftspi020),
	FUNCTION(ftssp010),
	FUNCTION(ftuart010),
	FUNCTION(ftuart010_1),
	FUNCTION(ftusart010),
	FUNCTION(mpblock),
	FUNCTION(na),
};

#define TC12NGRC_PIN(pin, reg, f0, f1, f2, f3)		\
	[TC12NGRC_PIN_ ## pin] = {				\
			.offset = TC12NGRC_PMUR_ ## reg,	\
			.functions = {			\
				TC12NGRC_MUX_ ## f0,	\
				TC12NGRC_MUX_ ## f1,	\
				TC12NGRC_MUX_ ## f2,	\
				TC12NGRC_MUX_ ## f3,	\
			},				\
		}

static struct ftscu010_pin tc12ngrc_pinmux_map[] = {
	/*               pin            reg     f0              f1              f2              f3  */
	TC12NGRC_PIN(X_PWM_TMR_0,       MFS0,   FTPWMTMR010,    FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_PWM_TMR_1,       MFS0,   FTPWMTMR010,    FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_PWM_TMR_2,       MFS0,   FTPWMTMR010,    FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_PWM_TMR_3,       MFS0,   FTPWMTMR010,    FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_I2C_SDA,         MFS0,   FTIIC010,       FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_I2C_SCL,         MFS0,   FTIIC010,       FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_SPI1_RXD,        MFS0,   FTSSP010,       FTGPIO010,      FTSPI020,       NA),
	TC12NGRC_PIN(X_SPI1_TXD,        MFS0,   FTSSP010,       FTGPIO010,      FTSPI020,       NA),
	TC12NGRC_PIN(X_SPI1_SCLK,       MFS0,   FTSSP010,       FTGPIO010,      FTSPI020,       NA),
	TC12NGRC_PIN(X_SPI1_CS0,        MFS0,   FTSSP010,       FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_UART_SI,         MFS0,   FTUART010,      FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_UART_SO,         MFS0,   FTUART010,      FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_UART1_SI,        MFS0,   FTUART010_1,    FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_UART1_SO,        MFS0,   FTUART010_1,    FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_USART_RXD,       MFS0,   FTUSART010,     FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_USART_TXD,       MFS0,   FTUSART010,     FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_USART_CTS,       MFS1,   FTUSART010,     FTGPIO010,      FTPWMTMR010,    NA),
	TC12NGRC_PIN(X_USART_RTS,       MFS1,   FTUSART010,     FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_USART_SCK,       MFS1,   FTUSART010,     FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_SPI1_CS1,        MFS1,   FTSSP010,       FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_CPU_TDI,         MFS1,   FA616TE,        FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_CPU_TMS,         MFS1,   FA616TE,        FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_CPU_TCK,         MFS1,   FA616TE,        FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_CPU_NTRST,       MFS1,   FA616TE,        FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_CPU_TDO,         MFS1,   FA616TE,        FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_CPU_DBGRQ,       MFS1,   FA616TE,        FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_CPU_DBGACK,      MFS1,   FA616TE,        FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_UARTMPB_SOUT,    MFS1,   MPBLOCK,        FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_UARTMPB_SIN,     MFS1,   MPBLOCK,        FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_SPI1_CS2,        MFS1,   FTSSP010,       FTGPIO010,      NA,             NA),
	TC12NGRC_PIN(X_SPI1_CS3,        MFS1,   FTSSP010,       FTGPIO010,      NA,             NA),
};

static const struct ftscu010_pinctrl_soc_data tc12ngrc_pinctrl_soc_data = {
	.pins = tc12ngrc_pins,
	.npins = ARRAY_SIZE(tc12ngrc_pins),
	.functions = tc12ngrc_pmx_functions,
	.nfunctions = ARRAY_SIZE(tc12ngrc_pmx_functions),
	.groups = tc12ngrc_groups,
	.ngroups = ARRAY_SIZE(tc12ngrc_groups),
	.map = tc12ngrc_pinmux_map,
};

static int tc12ngrc_pinctrl_probe(struct platform_device *pdev)
{
	return ftscu010_pinctrl_probe(pdev, &tc12ngrc_pinctrl_soc_data);
}

static struct of_device_id faraday_pinctrl_of_match[] = {
	{ .compatible = "ftscu010-pinmux", },
	{ },
};

static struct platform_driver tc12ngrc_pinctrl_driver = {
	.driver = {
		.name = "pinctrl-tc12ngrc",
		.owner = THIS_MODULE,
		.of_match_table = faraday_pinctrl_of_match,
	},
	.remove = ftscu010_pinctrl_remove,
};

static int __init tc12ngrc_pinctrl_init(void)
{
	return platform_driver_probe(&tc12ngrc_pinctrl_driver, tc12ngrc_pinctrl_probe);
}
arch_initcall(tc12ngrc_pinctrl_init);

static void __exit tc12ngrc_pinctrl_exit(void)
{
	platform_driver_unregister(&tc12ngrc_pinctrl_driver);
}
module_exit(tc12ngrc_pinctrl_exit);

MODULE_AUTHOR("Bo-Cun Chen <bcchen@faraday-tech.com");
MODULE_DESCRIPTION("Faraday TC12NGRC pinctrl driver");
MODULE_LICENSE("GPL");