/*
 *  Driver for Faraday LEO pin controller
 *
 *  Copyright (C) 2015 Faraday Technology
 *  Copyright (C) 2020 Faraday Linux Automation Tool
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

#define LEO_PMUR_MFS0   0x8500
#define LEO_PMUR_MFS1   0x8504
#define LEO_PMUR_MFS2   0x8508
#define LEO_PMUR_MFS3   0x850c
#define LEO_PMUR_MFS4   0x8510
#define LEO_PMUR_MFS5   0x8514
#define LEO_PMUR_MFS6   0x8518
#define LEO_PMUR_MFS7   0x851c
#define LEO_PMUR_MFS8   0x8520
#define LEO_PMUR_MFS9   0x8524
#define LEO_PMUR_MFS10  0x8528
#define LEO_PMUR_MFS11  0x852c
#define LEO_PMUR_MFS12  0x8530
#define LEO_PMUR_MFS13  0x8534
#define LEO_PMUR_MFS14  0x8538
#define LEO_PMUR_MFS15  0x853c
#define LEO_PMUR_MFS16  0x8540
#define LEO_PMUR_MFS17  0x8544
#define LEO_PMUR_MFS18  0x8548
#define LEO_PMUR_MFS19  0x854c
#define LEO_PMUR_MFS20  0x8550
#define LEO_PMUR_MFS21  0x8554
#define LEO_PMUR_MFS22  0x8558
#define LEO_PMUR_MFS23  0x855c
#define LEO_PMUR_MFS24  0x8560
#define LEO_PMUR_MFS25  0x8564
#define LEO_PMUR_MFS26  0x8568
#define LEO_PMUR_MFS27  0x856c
#define LEO_PMUR_MFS28  0x8570
#define LEO_PMUR_MFS29  0x8574
#define LEO_PMUR_MFS30  0x8578
#define LEO_PMUR_MFS31  0x857c
#define LEO_PMUR_MFS32  0x8580
#define LEO_PMUR_MFS33  0x8584
#define LEO_PMUR_MFS34  0x8588
#define LEO_PMUR_MFS35  0x858c
#define LEO_PMUR_MFS36  0x8590
#define LEO_PMUR_MFS37  0x8594
#define LEO_PMUR_MFS38  0x8598
#define LEO_PMUR_MFS39  0x859c
#define LEO_PMUR_MFS40  0x85a0
#define LEO_PMUR_MFS41  0x85a4
#define LEO_PMUR_MFS42  0x85a8
#define LEO_PMUR_MFS43  0x85ac
#define LEO_PMUR_MFS44  0x85b0
#define LEO_PMUR_MFS45  0x85b4
#define LEO_PMUR_MFS46  0x85b8
#define LEO_PMUR_MFS47  0x85bc
#define LEO_PMUR_MFS48  0x85c0
#define LEO_PMUR_MFS49  0x85c4
#define LEO_PMUR_MFS50  0x85c8
#define LEO_PMUR_MFS51  0x85cc
#define LEO_PMUR_MFS52  0x85d0
#define LEO_PMUR_MFS53  0x85d4
#define LEO_PMUR_MFS54  0x85d8
#define LEO_PMUR_MFS55  0x85dc
#define LEO_PMUR_MFS56  0x85e0
#define LEO_PMUR_MFS57  0x85e4
#define LEO_PMUR_MFS58  0x85e8
#define LEO_PMUR_MFS59  0x85ec
#define LEO_PMUR_MFS60  0x85f0
#define LEO_PMUR_MFS61  0x85f4
#define LEO_PMUR_MFS62  0x85f8
#define LEO_PMUR_MFS63  0x85fc
#define LEO_PMUR_MFS64  0x8600
#define LEO_PMUR_MFS65  0x8604
#define LEO_PMUR_MFS66  0x8608
#define LEO_PMUR_MFS67  0x860c
#define LEO_PMUR_MFS68  0x8610
#define LEO_PMUR_MFS69  0x8614
#define LEO_PMUR_MFS70  0x8618
#define LEO_PMUR_MFS71  0x861c
#define LEO_PMUR_MFS72  0x8620
#define LEO_PMUR_MFS73  0x8624
#define LEO_PMUR_MFS74  0x8628
#define LEO_PMUR_MFS75  0x862c
#define LEO_PMUR_MFS76  0x8630
#define LEO_PMUR_MFS77  0x8634
#define LEO_PMUR_MFS78  0x8638
#define LEO_PMUR_MFS79  0x863c
#define LEO_PMUR_MFS80  0x8640
#define LEO_PMUR_MFS81  0x8644
#define LEO_PMUR_MFS82  0x8648
#define LEO_PMUR_MFS83  0x864c
#define LEO_PMUR_MFS84  0x8650
#define LEO_PMUR_MFS85  0x8654
#define LEO_PMUR_MFS86  0x8658
#define LEO_PMUR_MFS87  0x865c
#define LEO_PMUR_MFS88  0x8660
#define LEO_PMUR_MFS89  0x8664
#define LEO_PMUR_MFS90  0x8668
#define LEO_PMUR_MFS91  0x866c
#define LEO_PMUR_MFS92  0x8670
#define LEO_PMUR_MFS93  0x8674
#define LEO_PMUR_MFS94  0x8678
#define LEO_PMUR_MFS95  0x867c
#define LEO_PMUR_MFS96  0x8680
#define LEO_PMUR_MFS97  0x8684
#define LEO_PMUR_MFS98  0x8688
#define LEO_PMUR_MFS99  0x868c
#define LEO_PMUR_MFS100 0x8690
#define LEO_PMUR_MFS101 0x8694
#define LEO_PMUR_MFS102 0x8698
#define LEO_PMUR_MFS103 0x869c
#define LEO_PMUR_MFS104 0x86a0
#define LEO_PMUR_MFS105 0x86a4
#define LEO_PMUR_MFS106 0x86a8
#define LEO_PMUR_MFS107 0x86ac
#define LEO_PMUR_MFS108 0x86b0
#define LEO_PMUR_MFS109 0x86b4
#define LEO_PMUR_MFS110 0x86b8
#define LEO_PMUR_MFS111 0x86bc
#define LEO_PMUR_MFS112 0x86c0
#define LEO_PMUR_MFS113 0x86c4
#define LEO_PMUR_MFS114 0x86c8
#define LEO_PMUR_MFS115 0x86cc
#define LEO_PMUR_MFS116 0x86d0
#define LEO_PMUR_MFS117 0x86d4
#define LEO_PMUR_MFS118 0x86d8
#define LEO_PMUR_MFS119 0x86dc
#define LEO_PMUR_MFS120 0x86e0
#define LEO_PMUR_MFS121 0x86e4
#define LEO_PMUR_MFS122 0x86e8
#define LEO_PMUR_MFS123 0x86ec
#define LEO_PMUR_MFS124 0x86f0
#define LEO_PMUR_MFS125 0x86f4
#define LEO_PMUR_MFS126 0x86f8
#define LEO_PMUR_MFS127 0x86fc
#define LEO_PMUR_MFS128 0x8700
#define LEO_PMUR_MFS129 0x8704
#define LEO_PMUR_MFS130 0x8708
#define LEO_PMUR_MFS131 0x870c
#define LEO_PMUR_MFS132 0x8710
#define LEO_PMUR_MFS133 0x8714
#define LEO_PMUR_MFS134 0x8718
#define LEO_PMUR_MFS135 0x871c
#define LEO_PMUR_MFS136 0x8720
#define LEO_PMUR_MFS137 0x8724
#define LEO_PMUR_MFS138 0x8728
#define LEO_PMUR_MFS139 0x872c
#define LEO_PMUR_MFS140 0x8730
#define LEO_PMUR_MFS141 0x8734
#define LEO_PMUR_MFS142 0x8738
#define LEO_PMUR_MFS143 0x873c
#define LEO_PMUR_MFS144 0x8740
#define LEO_PMUR_MFS145 0x8744
#define LEO_PMUR_MFS146 0x8748
#define LEO_PMUR_MFS147 0x874c
#define LEO_PMUR_MFS148 0x8750
#define LEO_PMUR_MFS149 0x8754
#define LEO_PMUR_MFS150 0x8758
#define LEO_PMUR_MFS151 0x875c
#define LEO_PMUR_MFS152 0x8760
#define LEO_PMUR_MFS153 0x8764
#define LEO_PMUR_MFS154 0x8768
#define LEO_PMUR_MFS155 0x876c
#define LEO_PMUR_MFS156 0x8770
#define LEO_PMUR_MFS157 0x8774
#define LEO_PMUR_MFS158 0x8778
#define LEO_PMUR_MFS159 0x877c
#define LEO_PMUR_MFS160 0x8780
#define LEO_PMUR_MFS161 0x8784
#define LEO_PMUR_MFS162 0x8788
#define LEO_PMUR_MFS163 0x878c
#define LEO_PMUR_MFS164 0x8790
#define LEO_PMUR_MFS165 0x8794
#define LEO_PMUR_MFS166 0x8798
#define LEO_PMUR_MFS167 0x879c
#define LEO_PMUR_MFS168 0x87a0
#define LEO_PMUR_MFS169 0x87a4
#define LEO_PMUR_MFS170 0x87a8
#define LEO_PMUR_MFS171 0x87ac
#define LEO_PMUR_MFS172 0x87b0
#define LEO_PMUR_MFS173 0x87b4
#define LEO_PMUR_MFS174 0x87b8
#define LEO_PMUR_MFS175 0x87bc
#define LEO_PMUR_MFS176 0x87c0
#define LEO_PMUR_MFS177 0x87c4
#define LEO_PMUR_MFS178 0x87c8
#define LEO_PMUR_MFS179 0x87cc
#define LEO_PMUR_MFS180 0x87d0
#define LEO_PMUR_MFS181 0x87d4
#define LEO_PMUR_MFS182 0x87d8
#define LEO_PMUR_MFS183 0x87dc
#define LEO_PMUR_MFS184 0x87e0
#define LEO_PMUR_MFS185 0x87e4
#define LEO_PMUR_MFS186 0x87e8
#define LEO_PMUR_MFS187 0x87ec

/* Pins */
enum{
	LEO_PIN_X_I2S0_FS,
	LEO_PIN_X_I2S0_RXD,
	LEO_PIN_X_I2S0_SCLK,
	LEO_PIN_X_I2S0_TXD,
	LEO_PIN_X_UART5_TX,
	LEO_PIN_X_UART5_RX,
	LEO_PIN_X_UART5_RX_h,
	LEO_PIN_X_UART6_TX,
	LEO_PIN_X_UART6_RX,
	LEO_PIN_X_UART9_TX,
	LEO_PIN_X_UART7_TX,
	LEO_PIN_X_UART7_RX,
	LEO_PIN_X_UART9_RX,
	LEO_PIN_X_UART8_TX,
	LEO_PIN_X_UART8_RX,
	LEO_PIN_X_UART8_RX_h,
	LEO_PIN_X_RGMII_RX_CK,
	LEO_PIN_X_RGMII_RXD_0,
	LEO_PIN_X_RGMII_RXD_1,
	LEO_PIN_X_RGMII_RXD_2,
	LEO_PIN_X_RGMII_RXD_3,
	LEO_PIN_X_RGMII_RXCTL,
	LEO_PIN_X_RGMII_GTX_CK,
	LEO_PIN_X_RGMII_TXCTL,
	LEO_PIN_X_RGMII_TXD_0,
	LEO_PIN_X_RGMII_TXD_1,
	LEO_PIN_X_RGMII_TXD_2,
	LEO_PIN_X_RGMII_TXD_3,
	LEO_PIN_X_RGMII_MDC,
	LEO_PIN_X_RGMII_MDIO,
	LEO_PIN_X_RMII0_TXD_0,
	LEO_PIN_X_RMII0_TXD_1,
	LEO_PIN_X_RMII0_TX_EN,
	LEO_PIN_X_RMII0_RX_CRSDV,
	LEO_PIN_X_RMII0_RX_ER,
	LEO_PIN_X_RMII0_RXD_0,
	LEO_PIN_X_RMII0_RXD_1,
	LEO_PIN_X_RMII0_REF_CLK,
	LEO_PIN_X_RMII0_MDC,
	LEO_PIN_X_RMII0_MDIO,
	LEO_PIN_X_MII_TXD_0,
	LEO_PIN_X_MII_TXD_1,
	LEO_PIN_X_MII_TX_EN,
	LEO_PIN_X_MII_RX_DV,
	LEO_PIN_X_MII_RX_ER,
	LEO_PIN_X_MII_RXD_0,
	LEO_PIN_X_MII_RXD_1,
	LEO_PIN_X_MII_MDC,
	LEO_PIN_X_MII_MDIO,
	LEO_PIN_X_MII_RX_CK,
	LEO_PIN_X_MII_RXD_2,
	LEO_PIN_X_MII_RXD_3,
	LEO_PIN_X_MII_TX_CK,
	LEO_PIN_X_MII_TX_ER,
	LEO_PIN_X_MII_TXD_2,
	LEO_PIN_X_MII_TXD_3,
	LEO_PIN_X_MII_CRS,
	LEO_PIN_X_MII_COL,
	LEO_PIN_X_EMMC1_D7,
	LEO_PIN_X_EMMC1_D6,
	LEO_PIN_X_EMMC1_D5,
	LEO_PIN_X_EMMC1_D4,
	LEO_PIN_X_EMMC1_D3,
	LEO_PIN_X_EMMC1_D2,
	LEO_PIN_X_EMMC1_D1,
	LEO_PIN_X_EMMC1_D0,
	LEO_PIN_X_EMMC1_RSTN,
	LEO_PIN_X_EMMC1_CMD,
	LEO_PIN_X_EMMC1_CLK,
	LEO_PIN_X_LC_PCLK,
	LEO_PIN_X_LC_VS,
	LEO_PIN_X_LC_HS,
	LEO_PIN_X_LC_DE,
	LEO_PIN_X_LC_DATA_0,
	LEO_PIN_X_LC_DATA_1,
	LEO_PIN_X_LC_DATA_2,
	LEO_PIN_X_LC_DATA_3,
	LEO_PIN_X_LC_DATA_4,
	LEO_PIN_X_LC_DATA_5,
	LEO_PIN_X_LC_DATA_6,
	LEO_PIN_X_LC_DATA_7,
	LEO_PIN_X_LC_DATA_8,
	LEO_PIN_X_LC_DATA_9,
	LEO_PIN_X_LC_DATA_10,
	LEO_PIN_X_LC_DATA_11,
	LEO_PIN_X_LC_DATA_12,
	LEO_PIN_X_LC_DATA_13,
	LEO_PIN_X_LC_DATA_14,
	LEO_PIN_X_LC_DATA_15,
	LEO_PIN_X_LC_DATA_16,
	LEO_PIN_X_LC_DATA_17,
	LEO_PIN_X_LC_DATA_18,
	LEO_PIN_X_LC_DATA_19,
	LEO_PIN_X_LC_DATA_20,
	LEO_PIN_X_LC_DATA_21,
	LEO_PIN_X_LC_DATA_22,
	LEO_PIN_X_LC_DATA_23,
	LEO_PIN_X_I2S_MCLK,
	LEO_PIN_X_SPI_DCX1,
	LEO_PIN_X_SPI_DCX2,
	LEO_PIN_X_UART2_NRTS,
	LEO_PIN_X_UART2_NCTS,
};

#define LEO_PINCTRL_PIN(x) PINCTRL_PIN(LEO_PIN_ ## x, #x)

static const struct pinctrl_pin_desc leo_pins[] = {
	LEO_PINCTRL_PIN(X_I2S0_FS),
	LEO_PINCTRL_PIN(X_I2S0_RXD),
	LEO_PINCTRL_PIN(X_I2S0_SCLK),
	LEO_PINCTRL_PIN(X_I2S0_TXD),
	LEO_PINCTRL_PIN(X_UART5_TX),
	LEO_PINCTRL_PIN(X_UART5_RX),
	LEO_PINCTRL_PIN(X_UART5_RX_h),
	LEO_PINCTRL_PIN(X_UART6_TX),
	LEO_PINCTRL_PIN(X_UART6_RX),
	LEO_PINCTRL_PIN(X_UART9_TX),
	LEO_PINCTRL_PIN(X_UART7_TX),
	LEO_PINCTRL_PIN(X_UART7_RX),
	LEO_PINCTRL_PIN(X_UART9_RX),
	LEO_PINCTRL_PIN(X_UART8_TX),
	LEO_PINCTRL_PIN(X_UART8_RX),
	LEO_PINCTRL_PIN(X_UART8_RX_h),
	LEO_PINCTRL_PIN(X_RGMII_RX_CK),
	LEO_PINCTRL_PIN(X_RGMII_RXD_0),
	LEO_PINCTRL_PIN(X_RGMII_RXD_1),
	LEO_PINCTRL_PIN(X_RGMII_RXD_2),
	LEO_PINCTRL_PIN(X_RGMII_RXD_3),
	LEO_PINCTRL_PIN(X_RGMII_RXCTL),
	LEO_PINCTRL_PIN(X_RGMII_GTX_CK),
	LEO_PINCTRL_PIN(X_RGMII_TXCTL),
	LEO_PINCTRL_PIN(X_RGMII_TXD_0),
	LEO_PINCTRL_PIN(X_RGMII_TXD_1),
	LEO_PINCTRL_PIN(X_RGMII_TXD_2),
	LEO_PINCTRL_PIN(X_RGMII_TXD_3),
	LEO_PINCTRL_PIN(X_RGMII_MDC),
	LEO_PINCTRL_PIN(X_RGMII_MDIO),
	LEO_PINCTRL_PIN(X_RMII0_TXD_0),
	LEO_PINCTRL_PIN(X_RMII0_TXD_1),
	LEO_PINCTRL_PIN(X_RMII0_TX_EN),
	LEO_PINCTRL_PIN(X_RMII0_RX_CRSDV),
	LEO_PINCTRL_PIN(X_RMII0_RX_ER),
	LEO_PINCTRL_PIN(X_RMII0_RXD_0),
	LEO_PINCTRL_PIN(X_RMII0_RXD_1),
	LEO_PINCTRL_PIN(X_RMII0_REF_CLK),
	LEO_PINCTRL_PIN(X_RMII0_MDC),
	LEO_PINCTRL_PIN(X_RMII0_MDIO),
	LEO_PINCTRL_PIN(X_MII_TXD_0),
	LEO_PINCTRL_PIN(X_MII_TXD_1),
	LEO_PINCTRL_PIN(X_MII_TX_EN),
	LEO_PINCTRL_PIN(X_MII_RX_DV),
	LEO_PINCTRL_PIN(X_MII_RX_ER),
	LEO_PINCTRL_PIN(X_MII_RXD_0),
	LEO_PINCTRL_PIN(X_MII_RXD_1),
	LEO_PINCTRL_PIN(X_MII_MDC),
	LEO_PINCTRL_PIN(X_MII_MDIO),
	LEO_PINCTRL_PIN(X_MII_RX_CK),
	LEO_PINCTRL_PIN(X_MII_RXD_2),
	LEO_PINCTRL_PIN(X_MII_RXD_3),
	LEO_PINCTRL_PIN(X_MII_TX_CK),
	LEO_PINCTRL_PIN(X_MII_TX_ER),
	LEO_PINCTRL_PIN(X_MII_TXD_2),
	LEO_PINCTRL_PIN(X_MII_TXD_3),
	LEO_PINCTRL_PIN(X_MII_CRS),
	LEO_PINCTRL_PIN(X_MII_COL),
	LEO_PINCTRL_PIN(X_EMMC1_D7),
	LEO_PINCTRL_PIN(X_EMMC1_D6),
	LEO_PINCTRL_PIN(X_EMMC1_D5),
	LEO_PINCTRL_PIN(X_EMMC1_D4),
	LEO_PINCTRL_PIN(X_EMMC1_D3),
	LEO_PINCTRL_PIN(X_EMMC1_D2),
	LEO_PINCTRL_PIN(X_EMMC1_D1),
	LEO_PINCTRL_PIN(X_EMMC1_D0),
	LEO_PINCTRL_PIN(X_EMMC1_RSTN),
	LEO_PINCTRL_PIN(X_EMMC1_CMD),
	LEO_PINCTRL_PIN(X_EMMC1_CLK),
	LEO_PINCTRL_PIN(X_LC_PCLK),
	LEO_PINCTRL_PIN(X_LC_VS),
	LEO_PINCTRL_PIN(X_LC_HS),
	LEO_PINCTRL_PIN(X_LC_DE),
	LEO_PINCTRL_PIN(X_LC_DATA_0),
	LEO_PINCTRL_PIN(X_LC_DATA_1),
	LEO_PINCTRL_PIN(X_LC_DATA_2),
	LEO_PINCTRL_PIN(X_LC_DATA_3),
	LEO_PINCTRL_PIN(X_LC_DATA_4),
	LEO_PINCTRL_PIN(X_LC_DATA_5),
	LEO_PINCTRL_PIN(X_LC_DATA_6),
	LEO_PINCTRL_PIN(X_LC_DATA_7),
	LEO_PINCTRL_PIN(X_LC_DATA_8),
	LEO_PINCTRL_PIN(X_LC_DATA_9),
	LEO_PINCTRL_PIN(X_LC_DATA_10),
	LEO_PINCTRL_PIN(X_LC_DATA_11),
	LEO_PINCTRL_PIN(X_LC_DATA_12),
	LEO_PINCTRL_PIN(X_LC_DATA_13),
	LEO_PINCTRL_PIN(X_LC_DATA_14),
	LEO_PINCTRL_PIN(X_LC_DATA_15),
	LEO_PINCTRL_PIN(X_LC_DATA_16),
	LEO_PINCTRL_PIN(X_LC_DATA_17),
	LEO_PINCTRL_PIN(X_LC_DATA_18),
	LEO_PINCTRL_PIN(X_LC_DATA_19),
	LEO_PINCTRL_PIN(X_LC_DATA_20),
	LEO_PINCTRL_PIN(X_LC_DATA_21),
	LEO_PINCTRL_PIN(X_LC_DATA_22),
	LEO_PINCTRL_PIN(X_LC_DATA_23),
	LEO_PINCTRL_PIN(X_I2S_MCLK),
	LEO_PINCTRL_PIN(X_SPI_DCX1),
	LEO_PINCTRL_PIN(X_SPI_DCX2),
	LEO_PINCTRL_PIN(X_UART2_NRTS),
	LEO_PINCTRL_PIN(X_UART2_NCTS),
};

/* Pin groups */
static unsigned ftcan010_pins[] = {
#if 0 //20200518@BC: CAN signals can output to below three PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_LC_PCLK,
	LEO_PIN_X_LC_VS,
#elif 0
	LEO_PIN_X_RMII0_TXD_0,
	LEO_PIN_X_RMII0_TXD_1,
#else
	LEO_PIN_X_UART5_RX,
	LEO_PIN_X_UART5_TX,
#endif
};
static unsigned ftcan010_1_pins[] = {
#if 0 //20200518@BC: CAN signals can output to below three PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_LC_DE,
	LEO_PIN_X_LC_HS,
#elif 0
	LEO_PIN_X_RMII0_RX_CRSDV,
	LEO_PIN_X_RMII0_TX_EN,
#else
	LEO_PIN_X_UART8_RX,
	LEO_PIN_X_UART8_TX,
#endif
};
static unsigned ftgmac030_mii_pins[] = {
	LEO_PIN_X_MII_COL,
	LEO_PIN_X_MII_CRS,
	LEO_PIN_X_MII_MDC,
	LEO_PIN_X_MII_MDIO,
	LEO_PIN_X_MII_RXD_0,
	LEO_PIN_X_MII_RXD_1,
	LEO_PIN_X_MII_RXD_2,
	LEO_PIN_X_MII_RXD_3,
	LEO_PIN_X_MII_RX_CK,
	LEO_PIN_X_MII_RX_DV,
	LEO_PIN_X_MII_RX_ER,
	LEO_PIN_X_MII_TXD_0,
	LEO_PIN_X_MII_TXD_1,
	LEO_PIN_X_MII_TXD_2,
	LEO_PIN_X_MII_TXD_3,
	LEO_PIN_X_MII_TX_CK,
	LEO_PIN_X_MII_TX_EN,
	LEO_PIN_X_MII_TX_ER,
};
static unsigned ftgmac030_rgmii_pins[] = {
	LEO_PIN_X_RGMII_GTX_CK,
	LEO_PIN_X_RGMII_MDC,
	LEO_PIN_X_RGMII_MDIO,
	LEO_PIN_X_RGMII_RXCTL,
	LEO_PIN_X_RGMII_RXD_0,
	LEO_PIN_X_RGMII_RXD_1,
	LEO_PIN_X_RGMII_RXD_2,
	LEO_PIN_X_RGMII_RXD_3,
	LEO_PIN_X_RGMII_RX_CK,
	LEO_PIN_X_RGMII_TXCTL,
	LEO_PIN_X_RGMII_TXD_0,
	LEO_PIN_X_RGMII_TXD_1,
	LEO_PIN_X_RGMII_TXD_2,
	LEO_PIN_X_RGMII_TXD_3,
};
static unsigned ftgmac030_rmii_pins[] = {
	LEO_PIN_X_RMII0_MDC,
	LEO_PIN_X_RMII0_MDIO,
	LEO_PIN_X_RMII0_REF_CLK,
	LEO_PIN_X_RMII0_RXD_0,
	LEO_PIN_X_RMII0_RXD_1,
	LEO_PIN_X_RMII0_RX_CRSDV,
	LEO_PIN_X_RMII0_RX_ER,
	LEO_PIN_X_RMII0_TXD_0,
	LEO_PIN_X_RMII0_TXD_1,
	LEO_PIN_X_RMII0_TX_EN,
};
#ifdef CONFIG_PINCTRL_LEO_MODEX
static unsigned ftgpio010_pins[] = {

	//LEO_PIN_X_UART2_NRTS,    //GPIO0[19] - ModeX
	//LEO_PIN_X_UART2_NCTS,    //GPIO0[20] - ModeX
	LEO_PIN_X_I2S_MCLK,      //GPIO0[28]
	//LEO_PIN_X_I2S0_SCLK,     //GPIO0[23] - ModeX
	//LEO_PIN_X_I2S0_TXD,      //GPIO0[18] - ModeX
	//LEO_PIN_X_I2S0_RXD,      //GPIO0[22] - ModeX
	//LEO_PIN_X_I2S0_FS,       //GPIO0[25] - ModeX
	LEO_PIN_X_MII_TXD_3,     //GPIO0[15]        (key4)
	LEO_PIN_X_MII_TXD_2,     //GPIO0[14]
	//LEO_PIN_X_MII_TX_EN,     //GPIO0[24] - ModeX
	LEO_PIN_X_MII_CRS,       //GPIO0[16]
	//LEO_PIN_X_UART5_TX,      //GPIO0[2] - ModeX
	//LEO_PIN_X_UART5_RX,      //GPIO0[3] - ModeX
	LEO_PIN_X_UART8_TX,      //GPIO0[9]
	LEO_PIN_X_UART8_RX,      //GPIO0[10]
	LEO_PIN_X_UART8_RX_h,    //GPIO0[11]
	LEO_PIN_X_LC_DATA_0,     //GPIO0[4]
	LEO_PIN_X_LC_DATA_1,     //GPIO0[5]
	LEO_PIN_X_LC_DATA_2,     //GPIO0[6]
	LEO_PIN_X_LC_DATA_3,     //GPIO0[7]
	LEO_PIN_X_LC_DATA_4,     //GPIO0[8]
	//LEO_PIN_X_LC_DATA_5,     //GPIO0[21] - ModeX
	//LEO_PIN_X_LC_DATA_6,     //GPIO0[17] - ModeX
	LEO_PIN_X_LC_DATA_8,     //GPIO0[12]
	LEO_PIN_X_LC_DATA_9,     //GPIO0[13]
	LEO_PIN_X_LC_DATA_23,    //GPIO0[27]
	LEO_PIN_X_LC_PCLK,       //GPIO0[0]
	LEO_PIN_X_LC_VS,         //GPIO0[1]
	LEO_PIN_X_LC_DATA_22,    //GPIO0[26]
	
#if 0
	LEO_PIN_X_LC_PCLK,       //GPIO0[0]
	LEO_PIN_X_LC_VS,         //GPIO0[1]
	LEO_PIN_X_UART5_TX,      //GPIO0[2] - ModeX
	LEO_PIN_X_UART5_RX,      //GPIO0[3] - ModeX

#if 1 //20200602@BC: GPIO[4]~GPIO[5] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_LC_DATA_0,     //GPIO0[4]
	LEO_PIN_X_LC_DATA_1,     //GPIO0[5]
#else
	LEO_PIN_X_UART6_RX,      //GPIO0[4]
	LEO_PIN_X_UART9_TX,      //GPIO0[5]
#endif

	LEO_PIN_X_LC_DATA_2,     //GPIO0[6]
	LEO_PIN_X_LC_DATA_3,     //GPIO0[7]

#if 1 //20200602@BC: GPIO[8] signals can output to below two PAD.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_LC_DATA_4,     //GPIO0[8]
#else
	LEO_PIN_X_UART9_RX,      //GPIO0[8]
#endif

#if 1 //20200602@BC: GPIO[9]~GPIO[10] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_MII_RX_CK,     //GPIO0[9]
	LEO_PIN_X_MII_RXD_2,     //GPIO0[10]
#else
	LEO_PIN_X_UART8_TX,      //GPIO0[9]
	LEO_PIN_X_UART8_RX,      //GPIO0[10]
#endif

#if 1 //20200602@BC: GPIO[11] signal can output to below three PAD.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_LC_DATA_7,     //GPIO0[11]
#elif 0
	LEO_PIN_X_MII_RXD_3,     //GPIO0[11]
#else
	LEO_PIN_X_UART8_RX_h,    //GPIO0[11]
#endif

#if 1 //20200602@BC: GPIO[12]~GPIO[16] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_LC_DATA_8,     //GPIO0[12]
	LEO_PIN_X_LC_DATA_9,     //GPIO0[13]
	LEO_PIN_X_LC_DATA_10,    //GPIO0[14]
	LEO_PIN_X_LC_DATA_11,    //GPIO0[15]
	LEO_PIN_X_LC_DATA_12,    //GPIO0[16]
#else
	LEO_PIN_X_MII_TX_CK,     //GPIO0[12]
	LEO_PIN_X_MII_TX_ER,     //GPIO0[13]
	LEO_PIN_X_MII_TXD_2,     //GPIO0[14]
	LEO_PIN_X_MII_TXD_3,     //GPIO0[15]
	LEO_PIN_X_MII_CRS,       //GPIO0[16]
#endif

	LEO_PIN_X_LC_DATA_6,     //GPIO0[17] - ModeX
	LEO_PIN_X_I2S0_TXD,      //GPIO0[18] - ModeX
	LEO_PIN_X_UART2_NRTS,    //GPIO0[19] - ModeX
	LEO_PIN_X_UART2_NCTS,    //GPIO0[20] - ModeX
	LEO_PIN_X_LC_DATA_5,     //GPIO0[21] - ModeX
	LEO_PIN_X_I2S0_RXD,      //GPIO0[22] - ModeX
	LEO_PIN_X_I2S0_SCLK,     //GPIO0[23] - ModeX
	LEO_PIN_X_MII_TX_EN,     //GPIO0[24] - ModeX
	LEO_PIN_X_I2S0_FS,       //GPIO0[25] - ModeX
	LEO_PIN_X_RMII0_RXD_0,   //GPIO0[26] - ModeX
	LEO_PIN_X_LC_DATA_23,    //GPIO0[27]
	LEO_PIN_X_I2S_MCLK,      //GPIO0[28]
	LEO_PIN_X_UART7_TX,      //GPIO0[29] - ModeX
	LEO_PIN_X_UART7_RX,      //GPIO0[30] - ModeX
	LEO_PIN_X_MII_MDC,       //GPIO0[31]
#endif
};
#else
static unsigned ftgpio010_pins[] = {

#if 0
#if 1 //20200525@BC: GPIO[0]~GPIO[8] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_LC_PCLK,       //GPIO0[0]
	LEO_PIN_X_LC_VS,         //GPIO0[1]
	LEO_PIN_X_LC_HS,         //GPIO0[2]
	LEO_PIN_X_LC_DE,         //GPIO0[3]
	LEO_PIN_X_LC_DATA_0,     //GPIO0[4]
	LEO_PIN_X_LC_DATA_1,     //GPIO0[5]
	LEO_PIN_X_LC_DATA_2,     //GPIO0[6]
	LEO_PIN_X_LC_DATA_3,     //GPIO0[7]
	LEO_PIN_X_LC_DATA_4,     //GPIO0[8]
#else
	LEO_PIN_X_UART5_TX,      //GPIO0[0]
	LEO_PIN_X_UART5_RX,      //GPIO0[1]
	LEO_PIN_X_UART5_RX_h,    //GPIO0[2]
	LEO_PIN_X_UART6_TX,      //GPIO0[3]
	LEO_PIN_X_UART6_RX,      //GPIO0[4]
	LEO_PIN_X_UART9_TX,      //GPIO0[5]
	LEO_PIN_X_UART7_TX,      //GPIO0[6]
	LEO_PIN_X_UART7_RX,      //GPIO0[7]
	LEO_PIN_X_UART9_RX,      //GPIO0[8]
#endif

#if 1 //20200525@BC: GPIO[9]~GPIO[11] signals can output to below three PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_LC_DATA_5,     //GPIO0[9]
	LEO_PIN_X_LC_DATA_6,     //GPIO0[10]
	LEO_PIN_X_LC_DATA_7,     //GPIO0[11]
#elif 0
	LEO_PIN_X_MII_RX_CK,     //GPIO0[9]
	LEO_PIN_X_MII_RXD_2,     //GPIO0[10]
	LEO_PIN_X_MII_RXD_3,     //GPIO0[11]
#else
	LEO_PIN_X_UART8_TX,      //GPIO0[9]
	LEO_PIN_X_UART8_RX,      //GPIO0[10]
	LEO_PIN_X_UART8_RX_h,    //GPIO0[11]
#endif

#if 1 //20200525@BC: GPIO[12]~GPIO[17] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_LC_DATA_8,     //GPIO0[12]
	LEO_PIN_X_LC_DATA_9,     //GPIO0[13]
	LEO_PIN_X_LC_DATA_10,    //GPIO0[14]
	LEO_PIN_X_LC_DATA_11,    //GPIO0[15]
	LEO_PIN_X_LC_DATA_12,    //GPIO0[16]
	LEO_PIN_X_LC_DATA_13,    //GPIO0[17]
#else
	LEO_PIN_X_MII_TX_CK,     //GPIO0[12]
	LEO_PIN_X_MII_TX_ER,     //GPIO0[13]
	LEO_PIN_X_MII_TXD_2,     //GPIO0[14]
	LEO_PIN_X_MII_TXD_3,     //GPIO0[15]
	LEO_PIN_X_MII_CRS,       //GPIO0[16]
	LEO_PIN_X_MII_COL,       //GPIO0[17]
#endif

      //20200525@BC: GPIO[18]~GPIO[31] always output to below PADs.
	LEO_PIN_X_LC_DATA_14,    //GPIO0[18]
	LEO_PIN_X_LC_DATA_15,    //GPIO0[19]
	LEO_PIN_X_LC_DATA_16,    //GPIO0[20]
	LEO_PIN_X_LC_DATA_17,    //GPIO0[21]
	LEO_PIN_X_LC_DATA_18,    //GPIO0[22]
	LEO_PIN_X_LC_DATA_19,    //GPIO0[23]
	LEO_PIN_X_LC_DATA_20,    //GPIO0[24]
	LEO_PIN_X_LC_DATA_21,    //GPIO0[25]
	LEO_PIN_X_LC_DATA_22,    //GPIO0[26]
	LEO_PIN_X_LC_DATA_23,    //GPIO0[27]
	LEO_PIN_X_I2S_MCLK,      //GPIO0[28]
	LEO_PIN_X_SPI_DCX1,      //GPIO0[29]
	LEO_PIN_X_SPI_DCX2,      //GPIO0[30]
	LEO_PIN_X_MII_MDC,       //GPIO0[31]
#endif
};
#endif
static  unsigned ftgpio010_1_pins[] = {
#ifndef CONFIG_PINCTRL_LEO_RMII
	LEO_PIN_X_RMII0_MDC,     //GPIO1[13]
	LEO_PIN_X_RMII0_MDIO,    //GPIO1[14]
	LEO_PIN_X_RMII0_RXD_0,   //GPIO1[10]
	LEO_PIN_X_RMII0_RXD_1,   //GPIO1[11]
	LEO_PIN_X_RMII0_RX_ER,   //GPIO1[9]
	LEO_PIN_X_RMII0_RX_CRSDV,//GPIO1[8]
	LEO_PIN_X_RMII0_TX_EN,   //GPIO1[7]
	LEO_PIN_X_RMII0_TXD_0,   //GPIO1[5]
#endif
	
#ifndef CONFIG_PINCTRL_LEO_MII
	LEO_PIN_X_MII_TXD_1,     //GPIO1[24]
	LEO_PIN_X_MII_TXD_0,     //GPIO1[23]
	LEO_PIN_X_MII_TX_ER,     //GPIO1[15]
	LEO_PIN_X_MII_TX_CK,     //GPIO1[4]
	LEO_PIN_X_MII_RXD_3,     //GPIO1[3]
	LEO_PIN_X_MII_RXD_2,     //GPIO1[2]   (key3)
	LEO_PIN_X_MII_RXD_1,     //GPIO1[29]  (key1)
	LEO_PIN_X_MII_RXD_0,     //GPIO1[28]  (key2)
	LEO_PIN_X_MII_RX_CK,     //GPIO1[1]
	LEO_PIN_X_MII_RX_ER,     //GPIO1[27]
	LEO_PIN_X_MII_RX_DV,     //GPIO1[26]
	LEO_PIN_X_MII_MDC,       //GPIO1[25]
	LEO_PIN_X_MII_MDIO,      //GPIO1[0]
	LEO_PIN_X_EMMC1_D1,      //GPIO1[6]
#endif
	
#if 0
	  //20200525@BC: GPIO[0] always output to below PAD.
#if 0
	LEO_PIN_X_EMMC1_D7,      //GPIO1[0]
#else
	LEO_PIN_X_MII_MDIO,      //GPIO1[0]
#endif

#if 1 //20200525@BC: GPIO[1]~GPIO[4] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_EMMC1_D6,      //GPIO1[1]
	LEO_PIN_X_EMMC1_D5,      //GPIO1[2]
	LEO_PIN_X_EMMC1_D4,      //GPIO1[3]
	LEO_PIN_X_EMMC1_D3,      //GPIO1[4]
#elif 0
	LEO_PIN_X_MII_RX_CK,     //GPIO1[1]
	LEO_PIN_X_MII_RXD_2,     //GPIO1[2]
	LEO_PIN_X_MII_RXD_3,     //GPIO1[3]
	LEO_PIN_X_MII_TX_CK,     //GPIO1[4]
#endif

#if 1 //20200525@BC: GPIO[5]~GPIO[10] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_EMMC1_D2,      //GPIO1[5]
	LEO_PIN_X_EMMC1_D1,      //GPIO1[6]
	LEO_PIN_X_EMMC1_D0,      //GPIO1[7]
	LEO_PIN_X_EMMC1_RSTN,    //GPIO1[8]
	LEO_PIN_X_EMMC1_CMD,     //GPIO[9]
	LEO_PIN_X_EMMC1_CLK,     //GPIO[10]
#elif 0
	LEO_PIN_X_RMII0_TXD_0,   //GPIO1[5]
	LEO_PIN_X_RMII0_TXD_1,   //GPIO1[6]
	LEO_PIN_X_RMII0_TX_EN,   //GPIO1[7]
	LEO_PIN_X_RMII0_RX_CRSDV,//GPIO1[8]
	LEO_PIN_X_RMII0_RX_ER,   //GPIO1[9]
	LEO_PIN_X_RMII0_RXD_0,   //GPIO1[10]
#endif

#if 1 //20200525@BC: GPIO[11]~GPIO[14] signals can output to below three PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_UART9_RX,      //GPIO1[11]
	//LEO_PIN_X_UART8_TX,      //GPIO1[12]
	//LEO_PIN_X_UART8_RX,      //GPIO1[13]
	//LEO_PIN_X_UART8_RX_h,    //GPIO1[14]
#elif 0
	LEO_PIN_X_RMII0_RXD_1,   //GPIO1[11]
	LEO_PIN_X_RMII0_REF_CLK, //GPIO1[12]
	LEO_PIN_X_RMII0_MDC,     //GPIO1[13]
	LEO_PIN_X_RMII0_MDIO,    //GPIO[14]
#else
	LEO_PIN_X_I2S0_FS,       //GPIO1[11]
	LEO_PIN_X_I2S0_RXD,      //GPIO1[12]
	LEO_PIN_X_I2S0_SCLK,     //GPIO1[13]
	LEO_PIN_X_I2S0_TXD,      //GPIO1[14]
#endif

#if 1 //20200525@BC: GPIO[15]~GPIO[18] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_MII_TX_ER,     //GPIO1[15]
	LEO_PIN_X_MII_TXD_2,     //GPIO1[16]
	LEO_PIN_X_MII_TXD_3,     //GPIO1[17]
	LEO_PIN_X_MII_CRS,       //GPIO1[18]
#else
	LEO_PIN_X_RGMII_RX_CK,   //GPIO1[15]
	LEO_PIN_X_RGMII_RXD_0,   //GPIO1[16]
	LEO_PIN_X_RGMII_RXD_1,   //GPIO1[17]
	LEO_PIN_X_RGMII_RXD_2,   //GPIO1[18]
#endif

#if 1 //20200525@BC: GPIO[19] can output to below three PAD.
	LEO_PIN_X_MII_COL,       //GPIO1[19]
#elif 0
	LEO_PIN_X_RGMII_RXD_3,   //GPIO1[19]
#else
	LEO_PIN_X_RGMII_TXD_0,   //GPIO1[19]
#endif

#if 1 //20200525@BC: GPIO[20]~GPIO[22] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_RGMII_RXCTL,   //GPIO1[20]
	LEO_PIN_X_RGMII_GTX_CK,  //GPIO1[21]
	LEO_PIN_X_RGMII_TXCTL,   //GPIO1[22]
#else
	LEO_PIN_X_RGMII_TXD_1,   //GPIO1[20]
	LEO_PIN_X_RGMII_TXD_2,   //GPIO1[21]
	LEO_PIN_X_RGMII_TXD_3,   //GPIO1[22]
#endif

#if 0 //20200525@BC: GPIO[23]~GPIO[26] signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_MII_TXD_0,     //GPIO1[23]
	LEO_PIN_X_MII_TXD_1,     //GPIO1[24]
	LEO_PIN_X_MII_MDC,       //GPIO1[25]
	// delete //LEO_PIN_X_MII_MDIO,      //GPIO1[26]
	
#else
	LEO_PIN_X_RGMII_MDC,     //GPIO1[23]
	LEO_PIN_X_RGMII_MDIO,    //GPIO1[24]
	LEO_PIN_X_MII_TX_EN,     //GPIO1[25]
	LEO_PIN_X_MII_RX_DV,     //GPIO1[26]
#endif

	  //20200525@BC: GPIO[27]~GPIO[29] always output to below PADs.
	LEO_PIN_X_MII_RX_ER,     //GPIO1[27]
	LEO_PIN_X_MII_RXD_0,     //GPIO1[28]
	LEO_PIN_X_MII_RXD_1,     //GPIO1[29]
#endif
};
static unsigned ftiic010_4_pins[] = {
#if 1 //20200513@BC: I2C signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_EMMC1_D2,
	LEO_PIN_X_EMMC1_D3,
#else
	LEO_PIN_X_LC_DATA_8,
	LEO_PIN_X_LC_DATA_9,
#endif
};
static unsigned ftkbc010_pins[] = {
#if 0 //20200514@BC: CODE_X0~X3 signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_I2S0_FS,          //CODE_X0
	LEO_PIN_X_I2S0_RXD,         //CODE_X1
	LEO_PIN_X_I2S0_SCLK,        //CODE_X2
	LEO_PIN_X_I2S0_TXD,         //CODE_X3
#else
	LEO_PIN_X_RGMII_RX_CK,      //CODE_X0
	LEO_PIN_X_RGMII_RXD_0,      //CODE_X1
	LEO_PIN_X_RGMII_RXD_1,      //CODE_X2
	LEO_PIN_X_RGMII_RXD_2,      //CODE_X3
#endif
	  //20200514@BC: CODE_X4~X7 signals always ouput to below PADs and don't turn OFF.
	LEO_PIN_X_RGMII_RXD_3,      //CODE_X4
	LEO_PIN_X_RGMII_RXCTL,      //CODE_X5
	LEO_PIN_X_RGMII_GTX_CK,     //CODE_X6
	LEO_PIN_X_RGMII_TXCTL,      //CODE_X7

#if 0 //20200514@BC: CODE_Y0~Y6 signals can output to below three PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_MII_TXD_0,        //CODE_Y0
	LEO_PIN_X_MII_TXD_1,        //CODE_Y1
	LEO_PIN_X_MII_TX_EN,        //CODE_Y2
	LEO_PIN_X_MII_RX_DV,        //CODE_Y3
	LEO_PIN_X_MII_RX_ER,        //CODE_Y4
	LEO_PIN_X_MII_RXD_0,        //CODE_Y5
	LEO_PIN_X_MII_RXD_1,        //CODE_Y6
#elif 1
	LEO_PIN_X_RMII0_TXD_0,      //CODE_Y0
	LEO_PIN_X_RMII0_TXD_1,      //CODE_Y1
	LEO_PIN_X_RMII0_TX_EN,      //CODE_Y2
	LEO_PIN_X_RMII0_RX_CRSDV,   //CODE_Y3
	LEO_PIN_X_RMII0_RX_ER,      //CODE_Y4
	LEO_PIN_X_RMII0_RXD_0,      //CODE_Y5
	LEO_PIN_X_RMII0_RXD_1,      //CODE_Y6
#else
	LEO_PIN_X_UART5_TX,         //CODE_Y0
	LEO_PIN_X_UART5_RX,         //CODE_Y1
	LEO_PIN_X_UART5_RX_h,       //CODE_Y2
	LEO_PIN_X_UART6_TX,         //CODE_Y3
	LEO_PIN_X_UART6_RX,         //CODE_Y4
	LEO_PIN_X_UART9_TX,         //CODE_Y5
	LEO_PIN_X_UART7_TX,         //CODE_Y6
#endif

#if 1 //20200514@BC: CODE_Y7 signal can output to below two PADs.
      //             User must to choose one PAD that depend on the EVB design.
	LEO_PIN_X_RMII0_REF_CLK,    //CODE_Y7
#else
	LEO_PIN_X_UART7_RX,         //CODE_Y7
#endif
};
static unsigned ftlcdc210_pins[] = {
	LEO_PIN_X_LC_DATA_0,
	LEO_PIN_X_LC_DATA_1,
	LEO_PIN_X_LC_DATA_10,
	LEO_PIN_X_LC_DATA_11,
	LEO_PIN_X_LC_DATA_12,
	LEO_PIN_X_LC_DATA_13,
	LEO_PIN_X_LC_DATA_14,
	LEO_PIN_X_LC_DATA_15,
	LEO_PIN_X_LC_DATA_16,
	LEO_PIN_X_LC_DATA_17,
	LEO_PIN_X_LC_DATA_18,
	LEO_PIN_X_LC_DATA_19,
	LEO_PIN_X_LC_DATA_2,
	LEO_PIN_X_LC_DATA_20,
	LEO_PIN_X_LC_DATA_21,
	LEO_PIN_X_LC_DATA_22,
	LEO_PIN_X_LC_DATA_23,
	LEO_PIN_X_LC_DATA_3,
	LEO_PIN_X_LC_DATA_4,
	LEO_PIN_X_LC_DATA_5,
	LEO_PIN_X_LC_DATA_6,
	LEO_PIN_X_LC_DATA_7,
	LEO_PIN_X_LC_DATA_8,
	LEO_PIN_X_LC_DATA_9,
	LEO_PIN_X_LC_DE,
	LEO_PIN_X_LC_HS,
	LEO_PIN_X_LC_PCLK,
	LEO_PIN_X_LC_VS,
};
static unsigned ftpwmtmr010_pins[] = {
	LEO_PIN_X_EMMC1_CLK,
	LEO_PIN_X_I2S0_FS,
	LEO_PIN_X_I2S0_RXD,
	LEO_PIN_X_I2S0_SCLK,
	LEO_PIN_X_LC_DATA_0,
	LEO_PIN_X_LC_DATA_1,
	LEO_PIN_X_LC_DATA_2,
	LEO_PIN_X_LC_DATA_3,
	LEO_PIN_X_MII_MDC,
	LEO_PIN_X_MII_MDIO,
	LEO_PIN_X_RGMII_TXD_0,
	LEO_PIN_X_RGMII_TXD_1,
	LEO_PIN_X_RMII0_MDIO,
	LEO_PIN_X_UART5_RX_h,
	LEO_PIN_X_UART8_RX_h,
};
static unsigned ftpwmtmr010_1_pins[] = {
	LEO_PIN_X_I2S0_TXD,
	LEO_PIN_X_RGMII_MDC,
	LEO_PIN_X_RGMII_MDIO,
	LEO_PIN_X_RGMII_TXD_2,
	LEO_PIN_X_RGMII_TXD_3,
};
static unsigned ftpwmtmr010_2_pins[] = {
	LEO_PIN_X_RMII0_RX_CRSDV,
	LEO_PIN_X_RMII0_TXD_0,
	LEO_PIN_X_RMII0_TXD_1,
	LEO_PIN_X_RMII0_TX_EN,
};
static unsigned ftpwmtmr010_3_pins[] = {
	LEO_PIN_X_RMII0_RXD_0,
	LEO_PIN_X_RMII0_RX_ER,
};
static unsigned ftsdc021_emmc_1_pins[] = {
	LEO_PIN_X_EMMC1_CLK,
	LEO_PIN_X_EMMC1_CMD,
	LEO_PIN_X_EMMC1_D0,
	LEO_PIN_X_EMMC1_D1,
	LEO_PIN_X_EMMC1_D2,
	LEO_PIN_X_EMMC1_D3,
	LEO_PIN_X_EMMC1_D4,
	LEO_PIN_X_EMMC1_D5,
	LEO_PIN_X_EMMC1_D6,
	LEO_PIN_X_EMMC1_D7,
	LEO_PIN_X_EMMC1_RSTN,
};
static unsigned ftssp010_i2s_pins[] = {
	LEO_PIN_X_I2S_MCLK,
	LEO_PIN_X_I2S0_FS,
	LEO_PIN_X_I2S0_RXD,
	LEO_PIN_X_I2S0_SCLK,
	LEO_PIN_X_I2S0_TXD,
};
static unsigned ftssp010_i2s_1_pins[] = {
	LEO_PIN_X_I2S_MCLK,
#if 1 //20200513@BC: I2S signals can output to below two PAD sets.
      //             User must to choose one PAD set that depend on the EVB design.
	LEO_PIN_X_EMMC1_D4,
	LEO_PIN_X_EMMC1_D5,
	LEO_PIN_X_EMMC1_D6,
	LEO_PIN_X_EMMC1_D7,
#else
	LEO_PIN_X_LC_DATA_4,
	LEO_PIN_X_LC_DATA_5,
	LEO_PIN_X_LC_DATA_6,
	LEO_PIN_X_LC_DATA_7,
#endif
};
static unsigned ftssp010_spi_pins[] = {
	LEO_PIN_X_LC_DATA_12,
	LEO_PIN_X_LC_DATA_13,
	LEO_PIN_X_SPI_DCX1,
};
static unsigned ftssp010_spi_1_pins[] = {
	LEO_PIN_X_LC_DATA_14,
	LEO_PIN_X_LC_DATA_15,
	LEO_PIN_X_SPI_DCX2,
};
static unsigned ftssp010_spi_2_pins[] = {
	LEO_PIN_X_LC_DATA_16,
	LEO_PIN_X_LC_DATA_17,
	LEO_PIN_X_SPI_DCX1,
};
static unsigned ftssp010_spi_3_pins[] = {
	LEO_PIN_X_LC_DATA_18,
	LEO_PIN_X_LC_DATA_19,
	LEO_PIN_X_SPI_DCX2,
};
static unsigned ftssp010_spi_4_pins[] = {
	LEO_PIN_X_LC_DATA_20,
	LEO_PIN_X_LC_DATA_21,
	LEO_PIN_X_LC_DATA_22,
	LEO_PIN_X_LC_DATA_23,
	LEO_PIN_X_SPI_DCX1,
};
static unsigned ftssp010_spi_5_pins[] = {
	LEO_PIN_X_SPI_DCX2,
};
#ifdef CONFIG_PINCTRL_LEO_MODEX
static unsigned ftssp010_spi_6_pins[] = {
#if 0
	LEO_PIN_X_RGMII_TXD_2,  // SPI_CS0 - ModeX
	LEO_PIN_X_RGMII_TXD_1,  // SPI_CLK - ModeX
	LEO_PIN_X_RGMII_TXD_0,  // SPI_MOSI - ModeX
	LEO_PIN_X_RGMII_RX_CK,  // SPI_MISO - ModeX
	LEO_PIN_X_RGMII_TXD_3,  // SPI_CS1 - ModeX
#endif
};
#else
static unsigned ftssp010_spi_6_pins[] = {
	LEO_PIN_X_EMMC1_CMD,
	LEO_PIN_X_EMMC1_RSTN,
	LEO_PIN_X_SPI_DCX1,
};
#endif
static unsigned ftuart010_4_pins[] = {
	LEO_PIN_X_EMMC1_D0,
	LEO_PIN_X_EMMC1_D1,
	LEO_PIN_X_LC_DATA_10,
	LEO_PIN_X_LC_DATA_11,
};
static unsigned ftuart010_5_pins[] = {
	LEO_PIN_X_UART5_RX,
	LEO_PIN_X_UART5_RX_h,
	LEO_PIN_X_UART5_TX,
};
#ifdef CONFIG_PINCTRL_LEO_MODEX
static unsigned ftuart010_6_pins[] = {
#if 0
	LEO_PIN_X_RGMII_MDC,    // UART_RX - ModeX
	LEO_PIN_X_RGMII_RXCTL,  // UART_TX - ModeX
#endif 
};
#else
static unsigned ftuart010_6_pins[] = {
	LEO_PIN_X_UART6_RX,
	LEO_PIN_X_UART6_TX,
};
#endif
static unsigned ftuart010_7_pins[] = {
	LEO_PIN_X_UART7_RX,
	LEO_PIN_X_UART7_TX,
};
static unsigned ftuart010_8_pins[] = {
	LEO_PIN_X_UART8_RX,
	LEO_PIN_X_UART8_RX_h,
	LEO_PIN_X_UART8_TX,
};
#ifdef CONFIG_PINCTRL_LEO_MODEX
static unsigned ftuart010_9_pins[] = {
#if 0
	LEO_PIN_X_RGMII_RXD_0,  // UART_RX - ModeX
	LEO_PIN_X_RGMII_RXD_1,  // UART_TX - ModeX
#endif
};
#else
static unsigned ftuart010_9_pins[] = {
	LEO_PIN_X_UART9_RX,
	LEO_PIN_X_UART9_TX,
};
#endif
static unsigned na_pins[] = {
	LEO_PIN_X_EMMC1_CLK,
	LEO_PIN_X_EMMC1_CMD,
	LEO_PIN_X_EMMC1_D0,
	LEO_PIN_X_EMMC1_D1,
	LEO_PIN_X_EMMC1_D2,
	LEO_PIN_X_EMMC1_D3,
	LEO_PIN_X_EMMC1_D4,
	LEO_PIN_X_EMMC1_D5,
	LEO_PIN_X_EMMC1_D6,
	LEO_PIN_X_EMMC1_D7,
	LEO_PIN_X_EMMC1_RSTN,
	LEO_PIN_X_I2S0_FS,
	LEO_PIN_X_I2S0_RXD,
	LEO_PIN_X_I2S0_SCLK,
	LEO_PIN_X_I2S0_TXD,
	LEO_PIN_X_I2S_MCLK,
	LEO_PIN_X_LC_DATA_0,
	LEO_PIN_X_LC_DATA_1,
	LEO_PIN_X_LC_DATA_10,
	LEO_PIN_X_LC_DATA_11,
	LEO_PIN_X_LC_DATA_12,
	LEO_PIN_X_LC_DATA_13,
	LEO_PIN_X_LC_DATA_14,
	LEO_PIN_X_LC_DATA_15,
	LEO_PIN_X_LC_DATA_16,
	LEO_PIN_X_LC_DATA_17,
	LEO_PIN_X_LC_DATA_18,
	LEO_PIN_X_LC_DATA_19,
	LEO_PIN_X_LC_DATA_2,
	LEO_PIN_X_LC_DATA_20,
	LEO_PIN_X_LC_DATA_21,
	LEO_PIN_X_LC_DATA_22,
	LEO_PIN_X_LC_DATA_23,
	LEO_PIN_X_LC_DATA_3,
	LEO_PIN_X_LC_DATA_4,
	LEO_PIN_X_LC_DATA_5,
	LEO_PIN_X_LC_DATA_6,
	LEO_PIN_X_LC_DATA_7,
	LEO_PIN_X_LC_DATA_8,
	LEO_PIN_X_LC_DATA_9,
	LEO_PIN_X_LC_DE,
	LEO_PIN_X_LC_HS,
	LEO_PIN_X_LC_PCLK,
	LEO_PIN_X_LC_VS,
	LEO_PIN_X_MII_COL,
	LEO_PIN_X_MII_CRS,
	LEO_PIN_X_MII_MDC,
	LEO_PIN_X_MII_MDIO,
	LEO_PIN_X_MII_RXD_0,
	LEO_PIN_X_MII_RXD_1,
	LEO_PIN_X_MII_RXD_2,
	LEO_PIN_X_MII_RXD_3,
	LEO_PIN_X_MII_RX_CK,
	LEO_PIN_X_MII_RX_DV,
	LEO_PIN_X_MII_RX_ER,
	LEO_PIN_X_MII_TXD_0,
	LEO_PIN_X_MII_TXD_1,
	LEO_PIN_X_MII_TXD_2,
	LEO_PIN_X_MII_TXD_3,
	LEO_PIN_X_MII_TX_CK,
	LEO_PIN_X_MII_TX_EN,
	LEO_PIN_X_MII_TX_ER,
	LEO_PIN_X_RGMII_GTX_CK,
	LEO_PIN_X_RGMII_MDC,
	LEO_PIN_X_RGMII_MDIO,
	LEO_PIN_X_RGMII_RXCTL,
	LEO_PIN_X_RGMII_RXD_0,
	LEO_PIN_X_RGMII_RXD_1,
	LEO_PIN_X_RGMII_RXD_2,
	LEO_PIN_X_RGMII_RXD_3,
	LEO_PIN_X_RGMII_RX_CK,
	LEO_PIN_X_RGMII_TXCTL,
	LEO_PIN_X_RGMII_TXD_0,
	LEO_PIN_X_RGMII_TXD_1,
	LEO_PIN_X_RGMII_TXD_2,
	LEO_PIN_X_RGMII_TXD_3,
	LEO_PIN_X_RMII0_MDC,
	LEO_PIN_X_RMII0_MDIO,
	LEO_PIN_X_RMII0_REF_CLK,
	LEO_PIN_X_RMII0_RXD_0,
	LEO_PIN_X_RMII0_RXD_1,
	LEO_PIN_X_RMII0_RX_CRSDV,
	LEO_PIN_X_RMII0_RX_ER,
	LEO_PIN_X_RMII0_TXD_0,
	LEO_PIN_X_RMII0_TXD_1,
	LEO_PIN_X_RMII0_TX_EN,
	LEO_PIN_X_SPI_DCX1,
	LEO_PIN_X_SPI_DCX2,
	LEO_PIN_X_UART5_RX,
	LEO_PIN_X_UART5_RX_h,
	LEO_PIN_X_UART5_TX,
	LEO_PIN_X_UART6_RX,
	LEO_PIN_X_UART6_TX,
	LEO_PIN_X_UART7_RX,
	LEO_PIN_X_UART7_TX,
	LEO_PIN_X_UART8_RX,
	LEO_PIN_X_UART8_RX_h,
	LEO_PIN_X_UART8_TX,
	LEO_PIN_X_UART9_RX,
	LEO_PIN_X_UART9_TX,
};

#define GROUP(gname)					\
	{						\
		.name = #gname,				\
		.pins = gname##_pins,			\
		.npins = ARRAY_SIZE(gname##_pins),	\
	}
	
static struct ftscu010_pin_group leo_groups[] = {
	GROUP(ftcan010),
	GROUP(ftcan010_1),
	GROUP(ftgmac030_mii),
	GROUP(ftgmac030_rgmii),
	GROUP(ftgmac030_rmii),
	GROUP(ftgpio010),
	GROUP(ftgpio010_1),
	GROUP(ftiic010_4),
	GROUP(ftkbc010),
	GROUP(ftlcdc210),
	GROUP(ftpwmtmr010),
	GROUP(ftpwmtmr010_1),
	GROUP(ftpwmtmr010_2),
	GROUP(ftpwmtmr010_3),
	GROUP(ftsdc021_emmc_1),
	GROUP(ftssp010_i2s),
	GROUP(ftssp010_i2s_1),
	GROUP(ftssp010_spi),
	GROUP(ftssp010_spi_1),
	GROUP(ftssp010_spi_2),
	GROUP(ftssp010_spi_3),
	GROUP(ftssp010_spi_4),
	GROUP(ftssp010_spi_5),
	GROUP(ftssp010_spi_6),
	GROUP(ftuart010_4),
	GROUP(ftuart010_5),
	GROUP(ftuart010_6),
	GROUP(ftuart010_7),
	GROUP(ftuart010_8),
	GROUP(ftuart010_9),
	GROUP(na),
};

/* Pin function groups */
static const char * const ftcan010_groups[] = { "ftcan010" };
static const char * const ftcan010_1_groups[] = { "ftcan010_1" };
static const char * const ftgmac030_mii_groups[] = { "ftgmac030_mii" };
static const char * const ftgmac030_rgmii_groups[] = { "ftgmac030_rgmii" };
static const char * const ftgmac030_rmii_groups[] = { "ftgmac030_rmii" };
static const char * const ftgpio010_groups[] = { "ftgpio010" };
static const char * const ftgpio010_1_groups[] = { "ftgpio010_1" };
static const char * const ftiic010_4_groups[] = { "ftiic010_4" };
static const char * const ftkbc010_groups[] = { "ftkbc010" };
static const char * const ftlcdc210_groups[] = { "ftlcdc210" };
static const char * const ftpwmtmr010_groups[] = { "ftpwmtmr010" };
static const char * const ftpwmtmr010_1_groups[] = { "ftpwmtmr010_1" };
static const char * const ftpwmtmr010_2_groups[] = { "ftpwmtmr010_2" };
static const char * const ftpwmtmr010_3_groups[] = { "ftpwmtmr010_3" };
static const char * const ftsdc021_emmc_1_groups[] = { "ftsdc021_emmc_1" };
static const char * const ftssp010_i2s_groups[] = { "ftssp010_i2s" };
static const char * const ftssp010_i2s_1_groups[] = { "ftssp010_i2s_1" };
static const char * const ftssp010_spi_groups[] = { "ftssp010_spi" };
static const char * const ftssp010_spi_1_groups[] = { "ftssp010_spi_1" };
static const char * const ftssp010_spi_2_groups[] = { "ftssp010_spi_2" };
static const char * const ftssp010_spi_3_groups[] = { "ftssp010_spi_3" };
static const char * const ftssp010_spi_4_groups[] = { "ftssp010_spi_4" };
static const char * const ftssp010_spi_5_groups[] = { "ftssp010_spi_5" };
static const char * const ftssp010_spi_6_groups[] = { "ftssp010_spi_6" };
static const char * const ftuart010_4_groups[] = { "ftuart010_4" };
static const char * const ftuart010_5_groups[] = { "ftuart010_5" };
static const char * const ftuart010_6_groups[] = { "ftuart010_6" };
static const char * const ftuart010_7_groups[] = { "ftuart010_7" };
static const char * const ftuart010_8_groups[] = { "ftuart010_8" };
static const char * const ftuart010_9_groups[] = { "ftuart010_9" };
static const char * const na_groups[] = { "na" };

/* Mux functions */
enum{
	LEO_MUX_FTCAN010,
	LEO_MUX_FTCAN010_1,
	LEO_MUX_FTGMAC030_MII,
	LEO_MUX_FTGMAC030_RGMII,
	LEO_MUX_FTGMAC030_RMII,
	LEO_MUX_FTGPIO010,
	LEO_MUX_FTGPIO010_1,
	LEO_MUX_FTIIC010_4,
	LEO_MUX_FTKBC010,
	LEO_MUX_FTLCDC210,
	LEO_MUX_FTPWMTMR010,
	LEO_MUX_FTPWMTMR010_1,
	LEO_MUX_FTPWMTMR010_2,
	LEO_MUX_FTPWMTMR010_3,
	LEO_MUX_FTSDC021_EMMC_1,
	LEO_MUX_FTSSP010_I2S,
	LEO_MUX_FTSSP010_I2S_1,
	LEO_MUX_FTSSP010_SPI,
	LEO_MUX_FTSSP010_SPI_1,
	LEO_MUX_FTSSP010_SPI_2,
	LEO_MUX_FTSSP010_SPI_3,
	LEO_MUX_FTSSP010_SPI_4,
	LEO_MUX_FTSSP010_SPI_5,
	LEO_MUX_FTSSP010_SPI_6,
	LEO_MUX_FTUART010_4,
	LEO_MUX_FTUART010_5,
	LEO_MUX_FTUART010_6,
	LEO_MUX_FTUART010_7,
	LEO_MUX_FTUART010_8,
	LEO_MUX_FTUART010_9,
	LEO_MUX_NA,
};

#define FUNCTION(fname)					\
	{						\
		.name = #fname,				\
		.groups = fname##_groups,		\
		.ngroups = ARRAY_SIZE(fname##_groups),	\
	}

static const struct ftscu010_pmx_function leo_pmx_functions[] = {
	FUNCTION(ftcan010),
	FUNCTION(ftcan010_1),
	FUNCTION(ftgmac030_mii),
	FUNCTION(ftgmac030_rgmii),
	FUNCTION(ftgmac030_rmii),
	FUNCTION(ftgpio010),
	FUNCTION(ftgpio010_1),
	FUNCTION(ftiic010_4),
	FUNCTION(ftkbc010),
	FUNCTION(ftlcdc210),
	FUNCTION(ftpwmtmr010),
	FUNCTION(ftpwmtmr010_1),
	FUNCTION(ftpwmtmr010_2),
	FUNCTION(ftpwmtmr010_3),
	FUNCTION(ftsdc021_emmc_1),
	FUNCTION(ftssp010_i2s),
	FUNCTION(ftssp010_i2s_1),
	FUNCTION(ftssp010_spi),
	FUNCTION(ftssp010_spi_1),
	FUNCTION(ftssp010_spi_2),
	FUNCTION(ftssp010_spi_3),
	FUNCTION(ftssp010_spi_4),
	FUNCTION(ftssp010_spi_5),
	FUNCTION(ftssp010_spi_6),
	FUNCTION(ftuart010_4),
	FUNCTION(ftuart010_5),
	FUNCTION(ftuart010_6),
	FUNCTION(ftuart010_7),
	FUNCTION(ftuart010_8),
	FUNCTION(ftuart010_9),
	FUNCTION(na),
};

#define LEO_PIN(pin, reg, f0, f1, f2, f3, f4, f5, f6, f7)		\
	[LEO_PIN_ ## pin] = {				\
			.offset = LEO_PMUR_ ## reg,	\
			.functions = {			\
				LEO_MUX_ ## f0,	\
				LEO_MUX_ ## f1,	\
				LEO_MUX_ ## f2,	\
				LEO_MUX_ ## f3,	\
				LEO_MUX_ ## f4,	\
				LEO_MUX_ ## f5,	\
				LEO_MUX_ ## f6,	\
				LEO_MUX_ ## f7,	\
			},				\
		}

static struct ftscu010_pin leo_pinmux_map[] = {
	/*		pin					reg		f0					f1				f2				f3				f4				f5				f6				f7*/
	LEO_PIN(X_I2S0_FS,			MFS97,	FTSSP010_I2S,		NA,				FTGPIO010_1,	NA,				NA,				FTPWMTMR010,	FTKBC010,		NA),
	LEO_PIN(X_I2S0_RXD,			MFS98,	FTSSP010_I2S,		NA,				FTGPIO010_1,	NA,				NA,				FTPWMTMR010,	FTKBC010,		NA),
	LEO_PIN(X_I2S0_SCLK,		MFS99,	FTSSP010_I2S,		NA,				FTGPIO010_1,	NA,				NA,				FTPWMTMR010,	FTKBC010,		NA),
	LEO_PIN(X_I2S0_TXD,			MFS100,	FTSSP010_I2S,		NA,				FTGPIO010_1,	NA,				NA,				FTPWMTMR010_1,	FTKBC010,		NA),
	LEO_PIN(X_UART5_TX,			MFS101,	FTUART010_5,		NA,				FTCAN010,		FTGPIO010,		NA,				NA,				FTKBC010,		NA),
	LEO_PIN(X_UART5_RX,			MFS102,	FTUART010_5,		NA,				FTCAN010,		FTGPIO010,		NA,				NA,				FTKBC010,		NA),
	LEO_PIN(X_UART5_RX_h,		MFS103,	FTUART010_5,		NA,				FTPWMTMR010,	FTGPIO010,		NA,				NA,				FTKBC010,		NA),
	LEO_PIN(X_UART6_TX,			MFS104,	FTUART010_6,		NA,				NA,				FTGPIO010,		NA,				NA,				FTKBC010,		NA),
	LEO_PIN(X_UART6_RX,			MFS105,	FTUART010_6,		NA,				NA,				FTGPIO010,		NA,				NA,				FTKBC010,		NA),
	LEO_PIN(X_UART9_TX,			MFS112,	FTUART010_9,		NA,				NA,				FTGPIO010,		NA,				NA,				FTKBC010,		NA),
	LEO_PIN(X_UART7_TX,			MFS106,	FTUART010_7,		NA,				NA,				FTGPIO010,		NA,				NA,				FTKBC010,		NA),
	LEO_PIN(X_UART7_RX,			MFS107,	FTUART010_7,		NA,				NA,				FTGPIO010,		NA,				NA,				FTKBC010,		NA),
	LEO_PIN(X_UART9_RX,			MFS111,	FTUART010_9,		NA,				NA,				FTGPIO010,		NA,				NA,				FTGPIO010_1,	NA),
	LEO_PIN(X_UART8_TX,			MFS108,	FTUART010_8,		NA,				FTCAN010_1,		FTGPIO010,		NA,				NA,				FTGPIO010_1,	NA),
	LEO_PIN(X_UART8_RX,			MFS109,	FTUART010_8,		NA,				FTCAN010_1,		FTGPIO010,		NA,				NA,				FTGPIO010_1,	NA),
	LEO_PIN(X_UART8_RX_h,		MFS110,	FTUART010_8,		NA,				FTPWMTMR010,	FTGPIO010,		NA,				NA,				FTGPIO010_1,	NA),
	LEO_PIN(X_RGMII_RX_CK,		MFS113,	FTGMAC030_RGMII,	FTKBC010,		NA,				NA,				NA,				FTGPIO010_1,	NA,				NA),
	LEO_PIN(X_RGMII_RXD_0,		MFS114,	FTGMAC030_RGMII,	FTKBC010,		NA,				NA,				NA,				FTGPIO010_1,	NA,				NA),
	LEO_PIN(X_RGMII_RXD_1,		MFS115,	FTGMAC030_RGMII,	FTKBC010,		NA,				NA,				NA,				FTGPIO010_1,	NA,				NA),
	LEO_PIN(X_RGMII_RXD_2,		MFS116,	FTGMAC030_RGMII,	FTKBC010,		NA,				NA,				NA,				FTGPIO010_1,	NA,				NA),
	LEO_PIN(X_RGMII_RXD_3,		MFS117,	FTGMAC030_RGMII,	FTKBC010,		NA,				NA,				NA,				FTGPIO010_1,	NA,				NA),
	LEO_PIN(X_RGMII_RXCTL,		MFS118,	FTGMAC030_RGMII,	FTKBC010,		NA,				NA,				NA,				FTGPIO010_1,	NA,				NA),
	LEO_PIN(X_RGMII_GTX_CK,		MFS119,	FTGMAC030_RGMII,	FTKBC010,		NA,				NA,				NA,				FTGPIO010_1,	NA,				NA),
	LEO_PIN(X_RGMII_TXCTL,		MFS120,	FTGMAC030_RGMII,	FTKBC010,		NA,				NA,				NA,				FTGPIO010_1,	NA,				NA),
	LEO_PIN(X_RGMII_TXD_0,		MFS121,	FTGMAC030_RGMII,	FTGPIO010_1,	NA,				FTPWMTMR010,	NA,				NA,				NA,				NA),
	LEO_PIN(X_RGMII_TXD_1,		MFS122,	FTGMAC030_RGMII,	FTGPIO010_1,	NA,				FTPWMTMR010,	NA,				NA,				NA,				NA),
	LEO_PIN(X_RGMII_TXD_2,		MFS123,	FTGMAC030_RGMII,	FTGPIO010_1,	NA,				FTPWMTMR010_1,	NA,				NA,				NA,				NA),
	LEO_PIN(X_RGMII_TXD_3,		MFS124,	FTGMAC030_RGMII,	FTGPIO010_1,	NA,				FTPWMTMR010_1,	NA,				NA,				NA,				NA),
	LEO_PIN(X_RGMII_MDC,		MFS125,	FTGMAC030_RGMII,	FTGPIO010_1,	NA,				FTPWMTMR010_1,	NA,				NA,				NA,				NA),
	LEO_PIN(X_RGMII_MDIO,		MFS126,	FTGMAC030_RGMII,	FTGPIO010_1,	NA,				FTPWMTMR010_1,	NA,				NA,				NA,				NA),
	LEO_PIN(X_RMII0_TXD_0,		MFS127,	FTGMAC030_RMII,		NA,				FTKBC010,		FTCAN010,		NA,				FTPWMTMR010_2,	NA,				FTGPIO010_1),
	LEO_PIN(X_RMII0_TXD_1,		MFS128,	FTGMAC030_RMII,		NA,				FTKBC010,		FTCAN010,		NA,				FTPWMTMR010_2,	NA,				FTGPIO010_1),
	LEO_PIN(X_RMII0_TX_EN,		MFS129,	FTGMAC030_RMII,		NA,				FTKBC010,		FTCAN010_1,		NA,				FTPWMTMR010_2,	NA,				FTGPIO010_1),
	LEO_PIN(X_RMII0_RX_CRSDV,	MFS130,	FTGMAC030_RMII,		NA,				FTKBC010,		FTCAN010_1,		NA,				FTPWMTMR010_2,	NA,				FTGPIO010_1),
	LEO_PIN(X_RMII0_RX_ER,		MFS131,	FTGMAC030_RMII,		NA,				FTKBC010,		FTPWMTMR010_3,	NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_RMII0_RXD_0,		MFS132,	FTGMAC030_RMII,		NA,				FTKBC010,		FTPWMTMR010_3,	NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_RMII0_RXD_1,		MFS133,	FTGMAC030_RMII,		NA,				FTKBC010,		FTGPIO010_1,	NA,				NA,				NA,				NA),
	LEO_PIN(X_RMII0_REF_CLK,	MFS134,	FTGMAC030_RMII,		NA,				FTKBC010,		FTGPIO010_1,	NA,				NA,				NA,				NA),
	LEO_PIN(X_RMII0_MDC,		MFS135,	FTGMAC030_RMII,		NA,				FTGPIO010_1,				NA,	NA,				NA,				NA,				NA),
	LEO_PIN(X_RMII0_MDIO,		MFS136,	FTGMAC030_RMII,		NA,				FTPWMTMR010,	FTGPIO010_1,	NA,				NA,				NA,				NA),
	LEO_PIN(X_MII_TXD_0,		MFS137,	FTGMAC030_MII,		FTKBC010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_TXD_1,		MFS138,	FTGMAC030_MII,		FTKBC010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_TX_EN,		MFS141,	FTGMAC030_MII,		FTKBC010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_RX_DV,		MFS142,	FTGMAC030_MII,		FTKBC010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_RX_ER,		MFS143,	FTGMAC030_MII,		FTKBC010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_RXD_0,		MFS144,	FTGMAC030_MII,		FTKBC010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_RXD_1,		MFS145,	FTGMAC030_MII,		FTKBC010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_MDC,			MFS152,	FTGMAC030_MII,		FTGPIO010_1,	NA,				NA,				FTPWMTMR010,	NA,				NA,				FTGPIO010),
	LEO_PIN(X_MII_MDIO,			MFS153,	FTGMAC030_MII,		NA,				NA,				NA,				FTPWMTMR010,	NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_RX_CK,		MFS154,	FTGMAC030_MII,		FTGPIO010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_RXD_2,		MFS146,	FTGMAC030_MII,		FTGPIO010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_RXD_3,		MFS147,	FTGMAC030_MII,		FTGPIO010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_TX_CK,		MFS151,	FTGMAC030_MII,		FTGPIO010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_TX_ER,		MFS148,	FTGMAC030_MII,		FTGPIO010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_TXD_2,		MFS139,	FTGMAC030_MII,		FTGPIO010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_TXD_3,		MFS140,	FTGMAC030_MII,		FTGPIO010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_CRS,			MFS149,	FTGMAC030_MII,		FTGPIO010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_MII_COL,			MFS150,	FTGMAC030_MII,		FTGPIO010,		NA,				NA,				NA,				NA,				NA,				FTGPIO010_1),
	LEO_PIN(X_EMMC1_D7,			MFS86,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTSSP010_I2S_1,	NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_D6,			MFS87,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTSSP010_I2S_1,	NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_D5,			MFS88,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTSSP010_I2S_1,	NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_D4,			MFS89,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTSSP010_I2S_1,	NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_D3,			MFS90,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTIIC010_4,		NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_D2,			MFS91,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTIIC010_4,		NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_D1,			MFS92,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTUART010_4,	NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_D0,			MFS93,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTUART010_4,	NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_RSTN,		MFS94,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTSSP010_SPI_6,	NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_CMD,		MFS95,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTSSP010_SPI_6,	NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_EMMC1_CLK,		MFS96,	FTSDC021_EMMC_1,	FTGPIO010_1,	FTPWMTMR010,	NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_LC_PCLK,			MFS155,	FTLCDC210,			NA,				FTCAN010,		NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_VS,			MFS156,	FTLCDC210,			NA,				FTCAN010,		NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_HS,			MFS157,	FTLCDC210,			NA,				FTCAN010_1,		NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DE,			MFS158,	FTLCDC210,			NA,				FTCAN010_1,		NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_0,		MFS159,	FTLCDC210,			NA,				FTPWMTMR010,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_1,		MFS160,	FTLCDC210,			NA,				FTPWMTMR010,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_2,		MFS161,	FTLCDC210,			NA,				FTPWMTMR010,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_3,		MFS162,	FTLCDC210,			NA,				FTPWMTMR010,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_4,		MFS163,	FTLCDC210,			NA,				FTSSP010_I2S_1,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_5,		MFS164,	FTLCDC210,			NA,				FTSSP010_I2S_1,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_6,		MFS165,	FTLCDC210,			NA,				FTSSP010_I2S_1,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_7,		MFS166,	FTLCDC210,			NA,				FTSSP010_I2S_1,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_8,		MFS167,	FTLCDC210,			NA,				FTIIC010_4,		NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_9,		MFS168,	FTLCDC210,			NA,				FTIIC010_4,		NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_10,		MFS169,	FTLCDC210,			NA,				FTUART010_4,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_11,		MFS170,	FTLCDC210,			NA,				FTUART010_4,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_12,		MFS171,	FTLCDC210,			NA,				FTSSP010_SPI,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_13,		MFS172,	FTLCDC210,			NA,				FTSSP010_SPI,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_14,		MFS173,	FTLCDC210,			NA,				FTSSP010_SPI_1,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_15,		MFS174,	FTLCDC210,			NA,				FTSSP010_SPI_1,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_16,		MFS175,	FTLCDC210,			NA,				FTSSP010_SPI_2,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_17,		MFS176,	FTLCDC210,			NA,				FTSSP010_SPI_2,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_18,		MFS177,	FTLCDC210,			NA,				FTSSP010_SPI_3,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_19,		MFS178,	FTLCDC210,			NA,				FTSSP010_SPI_3,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_20,		MFS179,	FTLCDC210,			NA,				FTSSP010_SPI_4,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_21,		MFS180,	FTLCDC210,			NA,				FTSSP010_SPI_4,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_22,		MFS181,	FTLCDC210,			NA,				FTSSP010_SPI_4,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_LC_DATA_23,		MFS182,	FTLCDC210,			NA,				FTSSP010_SPI_4,	NA,				NA,				FTGPIO010,		NA,				NA),
	LEO_PIN(X_I2S_MCLK,			MFS183,	FTSSP010_I2S,		NA,				FTGPIO010,		NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_SPI_DCX1,			MFS184,	FTGPIO010,			FTSSP010_SPI,	FTSSP010_SPI_2,	FTSSP010_SPI_4,	FTSSP010_SPI_6,	NA,				NA,				NA),
	LEO_PIN(X_SPI_DCX2,			MFS185,	FTGPIO010,			FTSSP010_SPI_1,	FTSSP010_SPI_3,	FTSSP010_SPI_5,	NA,				NA,				NA,				NA),
	LEO_PIN(X_UART2_NRTS,		MFS74,	NA,					NA,				NA,				NA,				NA,				NA,				NA,				NA),
	LEO_PIN(X_UART2_NCTS,		MFS75,	NA,					NA,				NA,				NA,				NA,				NA,				NA,				NA),
};

static struct ftscu010_pinctrl_soc_data leo_pinctrl_soc_data = {
	.pins = leo_pins,
	.npins = ARRAY_SIZE(leo_pins),
	.functions = leo_pmx_functions,
	.nfunctions = ARRAY_SIZE(leo_pmx_functions),
	.groups = leo_groups,
	.ngroups = ARRAY_SIZE(leo_groups),
	.map = leo_pinmux_map,
};

static int leo_pinctrl_probe(struct platform_device *pdev)
{
	return ftscu010_pinctrl_probe(pdev, &leo_pinctrl_soc_data);
}

static struct of_device_id faraday_pinctrl_of_match[] = {
	{ .compatible = "ftscu010-pinmux", "pinctrl-leo"},
	{ },
};

static struct platform_driver leo_pinctrl_driver = {
	.driver = {
		.name = "pinctrl-leo",
		.owner = THIS_MODULE,
		.of_match_table = faraday_pinctrl_of_match,
	},
	.remove = ftscu010_pinctrl_remove,
};

static int __init leo_pinctrl_init(void)
{
	return platform_driver_probe(&leo_pinctrl_driver, leo_pinctrl_probe);
}
arch_initcall(leo_pinctrl_init);

static void __exit leo_pinctrl_exit(void)
{
	platform_driver_unregister(&leo_pinctrl_driver);
}
module_exit(leo_pinctrl_exit);

MODULE_AUTHOR("Bo-Cun Chen <bcchen@faraday-tech.com");
MODULE_DESCRIPTION("Faraday LEO pinctrl driver");
MODULE_LICENSE("GPL");
