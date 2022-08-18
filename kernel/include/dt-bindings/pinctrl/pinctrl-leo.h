/*
 * Provide constants for LEO pinctrl bindings
 *
 * (C) Copyright 2020 Faraday Technology
 * Bo-Cun Chen <bcchen@faraday-tech.com>
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
#ifndef __DT_BINDINGS_PINCTRL_LEO_H__
#define __DT_BINDINGS_PINCTRL_LEO_H__

#define LEO_MUX_FTCAN010            0
#define LEO_MUX_FTCAN010_1          1
#define LEO_MUX_FTGMAC030_MII       2
#define LEO_MUX_FTGMAC030_RGMII     3
#define LEO_MUX_FTGMAC030_RMII      4
#define LEO_MUX_FTGPIO010           5
#define LEO_MUX_FTGPIO010_1         6
#define LEO_MUX_FTIIC010_4          7
#define LEO_MUX_FTKBC010            8
#define LEO_MUX_FTLCDC210           9
#define LEO_MUX_FTPWMTMR010         10
#define LEO_MUX_FTPWMTMR010_1       11
#define LEO_MUX_FTPWMTMR010_2       12
#define LEO_MUX_FTPWMTMR010_3       13
#define LEO_MUX_FTSDC021_EMMC_1     14
#define LEO_MUX_FTSSP010_I2S        15
#define LEO_MUX_FTSSP010_I2S_1      16
#define LEO_MUX_FTSSP010_SPI        17
#define LEO_MUX_FTSSP010_SPI_1      18
#define LEO_MUX_FTSSP010_SPI_2      19
#define LEO_MUX_FTSSP010_SPI_3      20
#define LEO_MUX_FTSSP010_SPI_4      21
#define LEO_MUX_FTSSP010_SPI_5      22
#define LEO_MUX_FTSSP010_SPI_6      23
#define LEO_MUX_FTUART010_4         24
#define LEO_MUX_FTUART010_5         25
#define LEO_MUX_FTUART010_6         26
#define LEO_MUX_FTUART010_7         27
#define LEO_MUX_FTUART010_8         28
#define LEO_MUX_FTUART010_9         29
#define LEO_MUX_NA                  30

/* Pins */

#define LEO_PIN_X_I2S0_FS			0
#define LEO_PIN_X_I2S0_RXD			1
#define LEO_PIN_X_I2S0_SCLK			2
#define LEO_PIN_X_I2S0_TXD			3
#define LEO_PIN_X_UART5_TX			4
#define LEO_PIN_X_UART5_RX			5
#define LEO_PIN_X_UART5_RX_h		6
#define LEO_PIN_X_UART6_TX			7
#define LEO_PIN_X_UART6_RX			8
#define LEO_PIN_X_UART9_TX			9
#define LEO_PIN_X_UART7_TX			10
#define LEO_PIN_X_UART7_RX			11
#define LEO_PIN_X_UART9_RX			12
#define LEO_PIN_X_UART8_TX			13
#define LEO_PIN_X_UART8_RX			14
#define LEO_PIN_X_UART8_RX_h		15
#define LEO_PIN_X_RGMII_RX_CK		16
#define LEO_PIN_X_RGMII_RXD_0		17
#define LEO_PIN_X_RGMII_RXD_1		18
#define LEO_PIN_X_RGMII_RXD_2		19
#define LEO_PIN_X_RGMII_RXD_3		20
#define LEO_PIN_X_RGMII_RXCTL		21
#define LEO_PIN_X_RGMII_GTX_CK		22
#define LEO_PIN_X_RGMII_TXCTL		23
#define LEO_PIN_X_RGMII_TXD_0		24
#define LEO_PIN_X_RGMII_TXD_1		25
#define LEO_PIN_X_RGMII_TXD_2		26
#define LEO_PIN_X_RGMII_TXD_3		27
#define LEO_PIN_X_RGMII_MDC			28
#define LEO_PIN_X_RGMII_MDIO		29
#define LEO_PIN_X_RMII0_TXD_0		30
#define LEO_PIN_X_RMII0_TXD_1		31
#define LEO_PIN_X_RMII0_TX_EN		32
#define LEO_PIN_X_RMII0_RX_CRSDV	33
#define LEO_PIN_X_RMII0_RX_ER		34
#define LEO_PIN_X_RMII0_RXD_0		35
#define LEO_PIN_X_RMII0_RXD_1		36
#define LEO_PIN_X_RMII0_REF_CLK		37
#define LEO_PIN_X_RMII0_MDC			38
#define LEO_PIN_X_RMII0_MDIO		39
#define LEO_PIN_X_MII_TXD_0			40
#define LEO_PIN_X_MII_TXD_1			41
#define LEO_PIN_X_MII_TX_EN			42
#define LEO_PIN_X_MII_RX_DV			43
#define LEO_PIN_X_MII_RX_ER			44
#define LEO_PIN_X_MII_RXD_0			45
#define LEO_PIN_X_MII_RXD_1			46
#define LEO_PIN_X_MII_MDC			47
#define LEO_PIN_X_MII_MDIO			48
#define LEO_PIN_X_MII_RX_CK			49
#define LEO_PIN_X_MII_RXD_2			50
#define LEO_PIN_X_MII_RXD_3			51
#define LEO_PIN_X_MII_TX_CK			52
#define LEO_PIN_X_MII_TX_ER			53
#define LEO_PIN_X_MII_TXD_2			54
#define LEO_PIN_X_MII_TXD_3			55
#define LEO_PIN_X_MII_CRS			56
#define LEO_PIN_X_MII_COL			57
#define LEO_PIN_X_EMMC1_D7			58
#define LEO_PIN_X_EMMC1_D6			59
#define LEO_PIN_X_EMMC1_D5			60
#define LEO_PIN_X_EMMC1_D4			61
#define LEO_PIN_X_EMMC1_D3			62
#define LEO_PIN_X_EMMC1_D2			63
#define LEO_PIN_X_EMMC1_D1			64
#define LEO_PIN_X_EMMC1_D0			65
#define LEO_PIN_X_EMMC1_RSTN		66
#define LEO_PIN_X_EMMC1_CMD			67
#define LEO_PIN_X_EMMC1_CLK			68
#define LEO_PIN_X_LC_PCLK			69
#define LEO_PIN_X_LC_VS				70
#define LEO_PIN_X_LC_HS				71
#define LEO_PIN_X_LC_DE				72
#define LEO_PIN_X_LC_DATA_0			73
#define LEO_PIN_X_LC_DATA_1			74
#define LEO_PIN_X_LC_DATA_2			75
#define LEO_PIN_X_LC_DATA_3			76
#define LEO_PIN_X_LC_DATA_4			77
#define LEO_PIN_X_LC_DATA_5			78
#define LEO_PIN_X_LC_DATA_6			79
#define LEO_PIN_X_LC_DATA_7			80
#define LEO_PIN_X_LC_DATA_8			81
#define LEO_PIN_X_LC_DATA_9			82
#define LEO_PIN_X_LC_DATA_10		83
#define LEO_PIN_X_LC_DATA_11		84
#define LEO_PIN_X_LC_DATA_12		85
#define LEO_PIN_X_LC_DATA_13		86
#define LEO_PIN_X_LC_DATA_14		87
#define LEO_PIN_X_LC_DATA_15		88
#define LEO_PIN_X_LC_DATA_16		89
#define LEO_PIN_X_LC_DATA_17		90
#define LEO_PIN_X_LC_DATA_18		91
#define LEO_PIN_X_LC_DATA_19		92
#define LEO_PIN_X_LC_DATA_20		93
#define LEO_PIN_X_LC_DATA_21		94
#define LEO_PIN_X_LC_DATA_22		95
#define LEO_PIN_X_LC_DATA_23		96
#define LEO_PIN_X_I2S_MCLK			97
#define LEO_PIN_X_SPI_DCX1			98
#define LEO_PIN_X_SPI_DCX2			99
#define LEO_PIN_X_UART2_NRTS		100
#define LEO_PIN_X_UART2_NCTS		101

/* Mode_X */

/* X_UART8_TX pin-mux by UART8_TX pin
   X_UART8_RX pin-mux by UART8_RX pin */
#define LEO_Mode_X_UART_8			(1 << 18)
/* X_RGMII_TXD[3] pin-mux by SPI6_CS1 pin */
#define LEO_Mode_X_SPI6_CS1			(1 << 17)
/* X_RGMII_TXD[2] pin-mux by SPI6_CS0 pin */
/* X_RGMII_TXD[1] pin-mux by SPI6_CLK pin */
/* X_RGMII_TXD[0] pin-mux by SPI6_MOSI pin */
/* X_RGMII_RX_CK pin-mux by SPI6_MISO pin */
#define LEO_Mode_X_SPI6				(1 << 16)
/* X_RGMII_RXD[1] pin-mux by UART9_TX pin
   X_RGMII_RXD[0] pin-mux by UART9_RX pin */
#define LEO_Mode_X_UART_9			(1 << 15)
/* X_RGMII_RX_CTL pin-mux by UART6_TX pin
   X_RGMII_RX_MDC pin-mux by UART6_RX pin */
#define LEO_Mode_X_UART_6			(1 << 14)
/* X_UART7_RX pin-mux by GPIO0_30 */
#define LEO_Mode_X_GPIO0_30			(1 << 13)
/* X_UART7_TX pin-mux by GPIO0_29 */
#define LEO_Mode_X_GPIO0_29			(1 << 12)
/* X_RMII_RXD[1] pin-mux by GPIO0_26 */
#define LEO_Mode_X_GPIO0_26			(1 << 11)
/* X_I2S0_FS pin-mux by GPIO0_25 */
#define LEO_Mode_X_GPIO0_25			(1 << 10)
/* X_MII_TX_EN pin-mux by GPIO0_24 */
#define LEO_Mode_X_GPIO0_24			(1 << 9)
/* X_I2S0_SCLK pin-mux by GPIO0_23 */
#define LEO_Mode_X_GPIO0_23			(1 << 8)
/* X_I2S0_RXD pin-mux by GPIO0_22 */
#define LEO_Mode_X_GPIO0_22			(1 << 7)
/* X_LC_DATA[5] pin-mux by GPIO0_21 */
#define LEO_Mode_X_GPIO0_21			(1 << 6)
/* X_UART2_NCTS pin-mux by GPIO0_20 */
#define LEO_Mode_X_GPIO0_20			(1 << 5)
/* X_UART2_NRTS pin-mux by GPIO0_19 */
#define LEO_Mode_X_GPIO0_19			(1 << 4)
/* X_I2S0_TXD pin-mux by GPIO0_18 */
#define LEO_Mode_X_GPIO0_18			(1 << 3)
/* X_LC_DATA[6] pin-mux by GPIO0_17 */
#define LEO_Mode_X_GPIO0_17			(1 << 2)
/* X_UART5_RX pin-mux by GPIO0_3 */
#define LEO_Mode_X_GPIO0_3			(1 << 1)
/* X_UART5_TX pin-mux by GPIO0_2 */
#define LEO_Mode_X_GPIO0_2			(1 << 0)

#endif	/* __DT_BINDINGS_PINCTRL_LEO_H__ */
