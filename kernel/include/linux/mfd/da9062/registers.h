/*
 * Copyright (C) 2015-2017  Dialog Semiconductor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __DA9062_H__
#define __DA9062_H__

#define DA9062_PMIC_DEVICE_ID		0x62
#define DA9062_PMIC_VARIANT_MRC_AA	0x01
#define DA9062_PMIC_VARIANT_VRC_DA9061	0x01
#define DA9062_PMIC_VARIANT_VRC_DA9062	0x02

#define DA9062_I2C_PAGE_SEL_SHIFT	1

/*
 * Registers
 */

#define DA9062AA_PAGE_CON		0x000
#define DA9062AA_STATUS_A		0x001
#define DA9062AA_STATUS_B		0x002
#define DA9062AA_STATUS_D		0x004
#define DA9062AA_FAULT_LOG		0x005
#define DA9062AA_EVENT_A		0x006
#define DA9062AA_EVENT_B		0x007
#define DA9062AA_EVENT_C		0x008
#define DA9062AA_IRQ_MASK_A		0x00A
#define DA9062AA_IRQ_MASK_B		0x00B
#define DA9062AA_IRQ_MASK_C		0x00C
#define DA9062AA_CONTROL_A		0x00E
#define DA9062AA_CONTROL_B		0x00F
#define DA9062AA_CONTROL_C		0x010
#define DA9062AA_CONTROL_D		0x011
#define DA9062AA_CONTROL_E		0x012
#define DA9062AA_CONTROL_F		0x013
#define DA9062AA_PD_DIS			0x014
#define DA9062AA_GPIO_0_1		0x015
#define DA9062AA_GPIO_2_3		0x016
#define DA9062AA_GPIO_4			0x017
#define DA9062AA_GPIO_WKUP_MODE		0x01C
#define DA9062AA_GPIO_MODE0_4		0x01D
#define DA9062AA_GPIO_OUT0_2		0x01E
#define DA9062AA_GPIO_OUT3_4		0x01F
#define DA9062AA_BUCK2_CONT		0x020
#define DA9062AA_BUCK1_CONT		0x021
#define DA9062AA_BUCK4_CONT		0x022
#define DA9062AA_BUCK3_CONT		0x024
#define DA9062AA_LDO1_CONT		0x026
#define DA9062AA_LDO2_CONT		0x027
#define DA9062AA_LDO3_CONT		0x028
#define DA9062AA_LDO4_CONT		0x029
#define DA9062AA_DVC_1			0x032
#define DA9062AA_COUNT_S		0x040
#define DA9062AA_COUNT_MI		0x041
#define DA9062AA_COUNT_H		0x042
#define DA9062AA_COUNT_D		0x043
#define DA9062AA_COUNT_MO		0x044
#define DA9062AA_COUNT_Y		0x045
#define DA9062AA_ALARM_S		0x046
#define DA9062AA_ALARM_MI		0x047
#define DA9062AA_ALARM_H		0x048
#define DA9062AA_ALARM_D		0x049
#define DA9062AA_ALARM_MO		0x04A
#define DA9062AA_ALARM_Y		0x04B
#define DA9062AA_SECOND_A		0x04C
#define DA9062AA_SECOND_B		0x04D
#define DA9062AA_SECOND_C		0x04E
#define DA9062AA_SECOND_D		0x04F
#define DA9062AA_SEQ			0x081
#define DA9062AA_SEQ_TIMER		0x082
#define DA9062AA_ID_2_1			0x083
#define DA9062AA_ID_4_3			0x084
#define DA9062AA_ID_12_11		0x088
#define DA9062AA_ID_14_13		0x089
#define DA9062AA_ID_16_15		0x08A
#define DA9062AA_ID_22_21		0x08D
#define DA9062AA_ID_24_23		0x08E
#define DA9062AA_ID_26_25		0x08F
#define DA9062AA_ID_28_27		0x090
#define DA9062AA_ID_30_29		0x091
#define DA9062AA_ID_32_31		0x092
#define DA9062AA_SEQ_A			0x095
#define DA9062AA_SEQ_B			0x096
#define DA9062AA_WAIT			0x097
#define DA9062AA_EN_32K			0x098
#define DA9062AA_RESET			0x099
#define DA9062AA_BUCK_ILIM_A		0x09A
#define DA9062AA_BUCK_ILIM_B		0x09B
#define DA9062AA_BUCK_ILIM_C		0x09C
#define DA9062AA_BUCK2_CFG		0x09D
#define DA9062AA_BUCK1_CFG		0x09E
#define DA9062AA_BUCK4_CFG		0x09F
#define DA9062AA_BUCK3_CFG		0x0A0
#define DA9062AA_VBUCK2_A		0x0A3
#define DA9062AA_VBUCK1_A		0x0A4
#define DA9062AA_VBUCK4_A		0x0A5
#define DA9062AA_VBUCK3_A		0x0A7
#define DA9062AA_VLDO1_A		0x0A9
#define DA9062AA_VLDO2_A		0x0AA
#define DA9062AA_VLDO3_A		0x0AB
#define DA9062AA_VLDO4_A		0x0AC
#define DA9062AA_VBUCK2_B		0x0B4
#define DA9062AA_VBUCK1_B		0x0B5
#define DA9062AA_VBUCK4_B		0x0B6
#define DA9062AA_VBUCK3_B		0x0B8
#define DA9062AA_VLDO1_B		0x0BA
#define DA9062AA_VLDO2_B		0x0BB
#define DA9062AA_VLDO3_B		0x0BC
#define DA9062AA_VLDO4_B		0x0BD
#define DA9062AA_BBAT_CONT		0x0C5
#define DA9062AA_INTERFACE		0x105
#define DA9062AA_CONFIG_A		0x106
#define DA9062AA_CONFIG_B		0x107
#define DA9062AA_CONFIG_C		0x108
#define DA9062AA_CONFIG_D		0x109
#define DA9062AA_CONFIG_E		0x10A
#define DA9062AA_CONFIG_G		0x10C
#define DA9062AA_CONFIG_H		0x10D
#define DA9062AA_CONFIG_I		0x10E
#define DA9062AA_CONFIG_J		0x10F
#define DA9062AA_CONFIG_K		0x110
#define DA9062AA_CONFIG_M		0x112
#define DA9062AA_TRIM_CLDR		0x120
#define DA9062AA_GP_ID_0		0x121
#define DA9062AA_GP_ID_1		0x122
#define DA9062AA_GP_ID_2		0x123
#define DA9062AA_GP_ID_3		0x124
#define DA9062AA_GP_ID_4		0x125
#define DA9062AA_GP_ID_5		0x126
#define DA9062AA_GP_ID_6		0x127
#define DA9062AA_GP_ID_7		0x128
#define DA9062AA_GP_ID_8		0x129
#define DA9062AA_GP_ID_9		0x12A
#define DA9062AA_GP_ID_10		0x12B
#define DA9062AA_GP_ID_11		0x12C
#define DA9062AA_GP_ID_12		0x12D
#define DA9062AA_GP_ID_13		0x12E
#define DA9062AA_GP_ID_14		0x12F
#define DA9062AA_GP_ID_15		0x130
#define DA9062AA_GP_ID_16		0x131
#define DA9062AA_GP_ID_17		0x132
#define DA9062AA_GP_ID_18		0x133
#define DA9062AA_GP_ID_19		0x134
#define DA9062AA_DEVICE_ID		0x181
#define DA9062AA_VARIANT_ID		0x182
#define DA9062AA_CUSTOMER_ID		0x183
#define DA9062AA_CONFIG_ID		0x184

/*
 * Bit fields
 */

/* DA9062AA_PAGE_CON = 0x000 */
#define DA9062AA_PAGE_SHIFT		0
#define DA9062AA_PAGE_MASK		0x3f
#define DA9062AA_WRITE_MODE_SHIFT	6
#define DA9062AA_WRITE_MODE_MASK	BIT(6)
#define DA9062AA_REVERT_SHIFT		7
#define DA9062AA_REVERT_MASK		BIT(7)

/* DA9062AA_STATUS_A = 0x001 */
#define DA9062AA_NONKEY_SHIFT		0
#define DA9062AA_NONKEY_MASK		0x01
#define DA9062AA_DVC_BUSY_SHIFT		2
#define DA9062AA_DVC_BUSY_MASK		BIT(2)

/* DA9062AA_STATUS_B = 0x002 */
#define DA9062AA_GPI0_SHIFT		0
#define DA9062AA_GPI0_MASK		0x01
#define DA9062AA_GPI1_SHIFT		1
#define DA9062AA_GPI1_MASK		BIT(1)
#define DA9062AA_GPI2_SHIFT		2
#define DA9062AA_GPI2_MASK		BIT(2)
#define DA9062AA_GPI3_SHIFT		3
#define DA9062AA_GPI3_MASK		BIT(3)
#define DA9062AA_GPI4_SHIFT		4
#define DA9062AA_GPI4_MASK		BIT(4)

/* DA9062AA_STATUS_D = 0x004 */
#define DA9062AA_LDO1_ILIM_SHIFT	0
#define DA9062AA_LDO1_ILIM_MASK		0x01
#define DA9062AA_LDO2_ILIM_SHIFT	1
#define DA9062AA_LDO2_ILIM_MASK		BIT(1)
#define DA9062AA_LDO3_ILIM_SHIFT	2
#define DA9062AA_LDO3_ILIM_MASK		BIT(2)
#define DA9062AA_LDO4_ILIM_SHIFT	3
#define DA9062AA_LDO4_ILIM_MASK		BIT(3)

/* DA9062AA_FAULT_LOG = 0x005 */
#define DA9062AA_TWD_ERROR_SHIFT	0
#define DA9062AA_TWD_ERROR_MASK		0x01
#define DA9062AA_POR_SHIFT		1
#define DA9062AA_POR_MASK		BIT(1)
#define DA9062AA_VDD_FAULT_SHIFT	2
#define DA9062AA_VDD_FAULT_MASK		BIT(2)
#define DA9062AA_VDD_START_SHIFT	3
#define DA9062AA_VDD_START_MASK		BIT(3)
#define DA9062AA_TEMP_CRIT_SHIFT	4
#define DA9062AA_TEMP_CRIT_MASK		BIT(4)
#define DA9062AA_KEY_RESET_SHIFT	5
#define DA9062AA_KEY_RESET_MASK		BIT(5)
#define DA9062AA_NSHUTDOWN_SHIFT	6
#define DA9062AA_NSHUTDOWN_MASK		BIT(6)
#define DA9062AA_WAIT_SHUT_SHIFT	7
#define DA9062AA_WAIT_SHUT_MASK		BIT(7)

/* DA9062AA_EVENT_A = 0x006 */
#define DA9062AA_E_NONKEY_SHIFT		0
#define DA9062AA_E_NONKEY_MASK		0x01
#define DA9062AA_E_ALARM_SHIFT		1
#define DA9062AA_E_ALARM_MASK		BIT(1)
#define DA9062AA_E_TICK_SHIFT		2
#define DA9062AA_E_TICK_MASK		BIT(2)
#define DA9062AA_E_WDG_WARN_SHIFT	3
#define DA9062AA_E_WDG_WARN_MASK	BIT(3)
#define DA9062AA_E_SEQ_RDY_SHIFT	4
#define DA9062AA_E_SEQ_RDY_MASK		BIT(4)
#define DA9062AA_EVENTS_B_SHIFT		5
#define DA9062AA_EVENTS_B_MASK		BIT(5)
#define DA9062AA_EVENTS_C_SHIFT		6
#define DA9062AA_EVENTS_C_MASK		BIT(6)

/* DA9062AA_EVENT_B = 0x007 */
#define DA9062AA_E_TEMP_SHIFT		1
#define DA9062AA_E_TEMP_MASK		BIT(1)
#define DA9062AA_E_LDO_LIM_SHIFT	3
#define DA9062AA_E_LDO_LIM_MASK		BIT(3)
#define DA9062AA_E_DVC_RDY_SHIFT	5
#define DA9062AA_E_DVC_RDY_MASK		BIT(5)
#define DA9062AA_E_VDD_WARN_SHIFT	7
#define DA9062AA_E_VDD_WARN_MASK	BIT(7)

/* DA9062AA_EVENT_C = 0x008 */
#define DA9062AA_E_GPI0_SHIFT		0
#define DA9062AA_E_GPI0_MASK		0x01
#define DA9062AA_E_GPI1_SHIFT		1
#define DA9062AA_E_GPI1_MASK		BIT(1)
#define DA9062AA_E_GPI2_SHIFT		2
#define DA9062AA_E_GPI2_MASK		BIT(2)
#define DA9062AA_E_GPI3_SHIFT		3
#define DA9062AA_E_GPI3_MASK		BIT(3)
#define DA9062AA_E_GPI4_SHIFT		4
#define DA9062AA_E_GPI4_MASK		BIT(4)

/* DA9062AA_IRQ_MASK_A = 0x00A */
#define DA9062AA_M_NONKEY_SHIFT		0
#define DA9062AA_M_NONKEY_MASK		0x01
#define DA9062AA_M_ALARM_SHIFT		1
#define DA9062AA_M_ALARM_MASK		BIT(1)
#define DA9062AA_M_TICK_SHIFT		2
#define DA9062AA_M_TICK_MASK		BIT(2)
#define DA9062AA_M_WDG_WARN_SHIFT	3
#define DA9062AA_M_WDG_WARN_MASK	BIT(3)
#define DA9062AA_M_SEQ_RDY_SHIFT	4
#define DA9062AA_M_SEQ_RDY_MASK		BIT(4)

/* DA9062AA_IRQ_MASK_B = 0x00B */
#define DA9062AA_M_TEMP_SHIFT		1
#define DA9062AA_M_TEMP_MASK		BIT(1)
#define DA9062AA_M_LDO_LIM_SHIFT	3
#define DA9062AA_M_LDO_LIM_MASK		BIT(3)
#define DA9062AA_M_DVC_RDY_SHIFT	5
#define DA9062AA_M_DVC_RDY_MASK		BIT(5)
#define DA9062AA_M_VDD_WARN_SHIFT	7
#define DA9062AA_M_VDD_WARN_MASK	BIT(7)

/* DA9062AA_IRQ_MASK_C = 0x00C */
#define DA9062AA_M_GPI0_SHIFT		0
#define DA9062AA_M_GPI0_MASK		0x01
#define DA9062AA_M_GPI1_SHIFT		1
#define DA9062AA_M_GPI1_MASK		BIT(1)
#define DA9062AA_M_GPI2_SHIFT		2
#define DA9062AA_M_GPI2_MASK		BIT(2)
#define DA9062AA_M_GPI3_SHIFT		3
#define DA9062AA_M_GPI3_MASK		BIT(3)
#define DA9062AA_M_GPI4_SHIFT		4
#define DA9062AA_M_GPI4_MASK		BIT(4)

/* DA9062AA_CONTROL_A = 0x00E */
#define DA9062AA_SYSTEM_EN_SHIFT	0
#define DA9062AA_SYSTEM_EN_MASK		0x01
#define DA9062AA_POWER_EN_SHIFT		1
#define DA9062AA_POWER_EN_MASK		BIT(1)
#define DA9062AA_POWER1_EN_SHIFT	2
#define DA9062AA_POWER1_EN_MASK		BIT(2)
#define DA9062AA_STANDBY_SHIFT		3
#define DA9062AA_STANDBY_MASK		BIT(3)
#define DA9062AA_M_SYSTEM_EN_SHIFT	4
#define DA9062AA_M_SYSTEM_EN_MASK	BIT(4)
#define DA9062AA_M_POWER_EN_SHIFT	5
#define DA9062AA_M_POWER_EN_MASK	BIT(5)
#define DA9062AA_M_POWER1_EN_SHIFT	6
#define DA9062AA_M_POWER1_EN_MASK	BIT(6)

/* DA9062AA_CONTROL_B = 0x00F */
#define DA9062AA_WATCHDOG_PD_SHIFT	1
#define DA9062AA_WATCHDOG_PD_MASK	BIT(1)
#define DA9062AA_FREEZE_EN_SHIFT	2
#define DA9062AA_FREEZE_EN_MASK		BIT(2)
#define DA9062AA_NRES_MODE_SHIFT	3
#define DA9062AA_NRES_MODE_MASK		BIT(3)
#define DA9062AA_NONKEY_LOCK_SHIFT	4
#define DA9062AA_NONKEY_LOCK_MASK	BIT(4)
#define DA9062AA_NFREEZE_SHIFT		5
#define DA9062AA_NFREEZE_MASK		(0x03 << 5)
#define DA9062AA_BUCK_SLOWSTART_SHIFT	7
#define DA9062AA_BUCK_SLOWSTART_MASK	BIT(7)

/* DA9062AA_CONTROL_C = 0x010 */
#define DA9062AA_DEBOUNCING_SHIFT	0
#define DA9062AA_DEBOUNCING_MASK	0x07
#define DA9062AA_AUTO_BOOT_SHIFT	3
#define DA9062AA_AUTO_BOOT_MASK		BIT(3)
#define DA9062AA_OTPREAD_EN_SHIFT	4
#define DA9062AA_OTPREAD_EN_MASK	BIT(4)
#define DA9062AA_SLEW_RATE_SHIFT	5
#define DA9062AA_SLEW_RATE_MASK		(0x03 << 5)
#define DA9062AA_DEF_SUPPLY_SHIFT	7
#define DA9062AA_DEF_SUPPLY_MASK	BIT(7)

/* DA9062AA_CONTROL_D = 0x011 */
#define DA9062AA_TWDSCALE_SHIFT		0
#define DA9062AA_TWDSCALE_MASK		0x07

/* DA9062AA_CONTROL_E = 0x012 */
#define DA9062AA_RTC_MODE_PD_SHIFT	0
#define DA9062AA_RTC_MODE_PD_MASK	0x01
#define DA9062AA_RTC_MODE_SD_SHIFT	1
#define DA9062AA_RTC_MODE_SD_MASK	BIT(1)
#define DA9062AA_RTC_EN_SHIFT		2
#define DA9062AA_RTC_EN_MASK		BIT(2)
#define DA9062AA_V_LOCK_SHIFT		7
#define DA9062AA_V_LOCK_MASK		BIT(7)

/* DA9062AA_CONTROL_F = 0x013 */
#define DA9062AA_WATCHDOG_SHIFT		0
#define DA9062AA_WATCHDOG_MASK		0x01
#define DA9062AA_SHUTDOWN_SHIFT		1
#define DA9062AA_SHUTDOWN_MASK		BIT(1)
#define DA9062AA_WAKE_UP_SHIFT		2
#define DA9062AA_WAKE_UP_MASK		BIT(2)

/* DA9062AA_PD_DIS = 0x014 */
#define DA9062AA_GPI_DIS_SHIFT		0
#define DA9062AA_GPI_DIS_MASK		0x01
#define DA9062AA_PMIF_DIS_SHIFT		2
#define DA9062AA_PMIF_DIS_MASK		BIT(2)
#define DA9062AA_CLDR_PAUSE_SHIFT	4
#define DA9062AA_CLDR_PAUSE_MASK	BIT(4)
#define DA9062AA_BBAT_DIS_SHIFT		5
#define DA9062AA_BBAT_DIS_MASK		BIT(5)
#define DA9062AA_OUT32K_PAUSE_SHIFT	6
#define DA9062AA_OUT32K_PAUSE_MASK	BIT(6)
#define DA9062AA_PMCONT_DIS_SHIFT	7
#define DA9062AA_PMCONT_DIS_MASK	BIT(7)

/* DA9062AA_GPIO_0_1 = 0x015 */
#define DA9062AA_GPIO0_PIN_SHIFT	0
#define DA9062AA_GPIO0_PIN_MASK		0x03
#define DA9062AA_GPIO0_TYPE_SHIFT	2
#define DA9062AA_GPIO0_TYPE_MASK	BIT(2)
#define DA9062AA_GPIO0_WEN_SHIFT	3
#define DA9062AA_GPIO0_WEN_MASK		BIT(3)
#define DA9062AA_GPIO1_PIN_SHIFT	4
#define DA9062AA_GPIO1_PIN_MASK		(0x03 << 4)
#define DA9062AA_GPIO1_TYPE_SHIFT	6
#define DA9062AA_GPIO1_TYPE_MASK	BIT(6)
#define DA9062AA_GPIO1_WEN_SHIFT	7
#define DA9062AA_GPIO1_WEN_MASK		BIT(7)

/* DA9062AA_GPIO_2_3 = 0x016 */
#define DA9062AA_GPIO2_PIN_SHIFT	0
#define DA9062AA_GPIO2_PIN_MASK		0x03
#define DA9062AA_GPIO2_TYPE_SHIFT	2
#define DA9062AA_GPIO2_TYPE_MASK	BIT(2)
#define DA9062AA_GPIO2_WEN_SHIFT	3
#define DA9062AA_GPIO2_WEN_MASK		BIT(3)
#define DA9062AA_GPIO3_PIN_SHIFT	4
#define DA9062AA_GPIO3_PIN_MASK		(0x03 << 4)
#define DA9062AA_GPIO3_TYPE_SHIFT	6
#define DA9062AA_GPIO3_TYPE_MASK	BIT(6)
#define DA9062AA_GPIO3_WEN_SHIFT	7
#define DA9062AA_GPIO3_WEN_MASK		BIT(7)

/* DA9062AA_GPIO_4 = 0x017 */
#define DA9062AA_GPIO4_PIN_SHIFT	0
#define DA9062AA_GPIO4_PIN_MASK		0x03
#define DA9062AA_GPIO4_TYPE_SHIFT	2
#define DA9062AA_GPIO4_TYPE_MASK	BIT(2)
#define DA9062AA_GPIO4_WEN_SHIFT	3
#define DA9062AA_GPIO4_WEN_MASK		BIT(3)

/* DA9062AA_GPIO_WKUP_MODE = 0x01C */
#define DA9062AA_GPIO0_WKUP_MODE_SHIFT	0
#define DA9062AA_GPIO0_WKUP_MODE_MASK	0x01
#define DA9062AA_GPIO1_WKUP_MODE_SHIFT	1
#define DA9062AA_GPIO1_WKUP_MODE_MASK	BIT(1)
#define DA9062AA_GPIO2_WKUP_MODE_SHIFT	2
#define DA9062AA_GPIO2_WKUP_MODE_MASK	BIT(2)
#define DA9062AA_GPIO3_WKUP_MODE_SHIFT	3
#define DA9062AA_GPIO3_WKUP_MODE_MASK	BIT(3)
#define DA9062AA_GPIO4_WKUP_MODE_SHIFT	4
#define DA9062AA_GPIO4_WKUP_MODE_MASK	BIT(4)

/* DA9062AA_GPIO_MODE0_4 = 0x01D */
#define DA9062AA_GPIO0_MODE_SHIFT	0
#define DA9062AA_GPIO0_MODE_MASK	0x01
#define DA9062AA_GPIO1_MODE_SHIFT	1
#define DA9062AA_GPIO1_MODE_MASK	BIT(1)
#define DA9062AA_GPIO2_MODE_SHIFT	2
#define DA9062AA_GPIO2_MODE_MASK	BIT(2)
#define DA9062AA_GPIO3_MODE_SHIFT	3
#define DA9062AA_GPIO3_MODE_MASK	BIT(3)
#define DA9062AA_GPIO4_MODE_SHIFT	4
#define DA9062AA_GPIO4_MODE_MASK	BIT(4)

/* DA9062AA_GPIO_OUT0_2 = 0x01E */
#define DA9062AA_GPIO0_OUT_SHIFT	0
#define DA9062AA_GPIO0_OUT_MASK		0x07
#define DA9062AA_GPIO1_OUT_SHIFT	3
#define DA9062AA_GPIO1_OUT_MASK		(0x07 << 3)
#define DA9062AA_GPIO2_OUT_SHIFT	6
#define DA9062AA_GPIO2_OUT_MASK		(0x03 << 6)

/* DA9062AA_GPIO_OUT3_4 = 0x01F */
#define DA9062AA_GPIO3_OUT_SHIFT	0
#define DA9062AA_GPIO3_OUT_MASK		0x07
#define DA9062AA_GPIO4_OUT_SHIFT	3
#define DA9062AA_GPIO4_OUT_MASK		(0x03 << 3)

/* DA9062AA_BUCK2_CONT = 0x020 */
#define DA9062AA_BUCK2_EN_SHIFT		0
#define DA9062AA_BUCK2_EN_MASK		0x01
#define DA9062AA_BUCK2_GPI_SHIFT	1
#define DA9062AA_BUCK2_GPI_MASK		(0x03 << 1)
#define DA9062AA_BUCK2_CONF_SHIFT	3
#define DA9062AA_BUCK2_CONF_MASK	BIT(3)
#define DA9062AA_VBUCK2_GPI_SHIFT	5
#define DA9062AA_VBUCK2_GPI_MASK	(0x03 << 5)

/* DA9062AA_BUCK1_CONT = 0x021 */
#define DA9062AA_BUCK1_EN_SHIFT		0
#define DA9062AA_BUCK1_EN_MASK		0x01
#define DA9062AA_BUCK1_GPI_SHIFT	1
#define DA9062AA_BUCK1_GPI_MASK		(0x03 << 1)
#define DA9062AA_BUCK1_CONF_SHIFT	3
#define DA9062AA_BUCK1_CONF_MASK	BIT(3)
#define DA9062AA_VBUCK1_GPI_SHIFT	5
#define DA9062AA_VBUCK1_GPI_MASK	(0x03 << 5)

/* DA9062AA_BUCK4_CONT = 0x022 */
#define DA9062AA_BUCK4_EN_SHIFT		0
#define DA9062AA_BUCK4_EN_MASK		0x01
#define DA9062AA_BUCK4_GPI_SHIFT	1
#define DA9062AA_BUCK4_GPI_MASK		(0x03 << 1)
#define DA9062AA_BUCK4_CONF_SHIFT	3
#define DA9062AA_BUCK4_CONF_MASK	BIT(3)
#define DA9062AA_VBUCK4_GPI_SHIFT	5
#define DA9062AA_VBUCK4_GPI_MASK	(0x03 << 5)

/* DA9062AA_BUCK3_CONT = 0x024 */
#define DA9062AA_BUCK3_EN_SHIFT		0
#define DA9062AA_BUCK3_EN_MASK		0x01
#define DA9062AA_BUCK3_GPI_SHIFT	1
#define DA9062AA_BUCK3_GPI_MASK		(0x03 << 1)
#define DA9062AA_BUCK3_CONF_SHIFT	3
#define DA9062AA_BUCK3_CONF_MASK	BIT(3)
#define DA9062AA_VBUCK3_GPI_SHIFT	5
#define DA9062AA_VBUCK3_GPI_MASK	(0x03 << 5)

/* DA9062AA_LDO1_CONT = 0x026 */
#define DA9062AA_LDO1_EN_SHIFT		0
#define DA9062AA_LDO1_EN_MASK		0x01
#define DA9062AA_LDO1_GPI_SHIFT		1
#define DA9062AA_LDO1_GPI_MASK		(0x03 << 1)
#define DA9062AA_LDO1_PD_DIS_SHIFT	3
#define DA9062AA_LDO1_PD_DIS_MASK	BIT(3)
#define DA9062AA_VLDO1_GPI_SHIFT	5
#define DA9062AA_VLDO1_GPI_MASK		(0x03 << 5)
#define DA9062AA_LDO1_CONF_SHIFT	7
#define DA9062AA_LDO1_CONF_MASK		BIT(7)

/* DA9062AA_LDO2_CONT = 0x027 */
#define DA9062AA_LDO2_EN_SHIFT		0
#define DA9062AA_LDO2_EN_MASK		0x01
#define DA9062AA_LDO2_GPI_SHIFT		1
#define DA9062AA_LDO2_GPI_MASK		(0x03 << 1)
#define DA9062AA_LDO2_PD_DIS_SHIFT	3
#define DA9062AA_LDO2_PD_DIS_MASK	BIT(3)
#define DA9062AA_VLDO2_GPI_SHIFT	5
#define DA9062AA_VLDO2_GPI_MASK		(0x03 << 5)
#define DA9062AA_LDO2_CONF_SHIFT	7
#define DA9062AA_LDO2_CONF_MASK		BIT(7)

/* DA9062AA_LDO3_CONT = 0x028 */
#define DA9062AA_LDO3_EN_SHIFT		0
#define DA9062AA_LDO3_EN_MASK		0x01
#define DA9062AA_LDO3_GPI_SHIFT		1
#define DA9062AA_LDO3_GPI_MASK		(0x03 << 1)
#define DA9062AA_LDO3_PD_DIS_SHIFT	3
#define DA9062AA_LDO3_PD_DIS_MASK	BIT(3)
#define DA9062AA_VLDO3_GPI_SHIFT	5
#define DA9062AA_VLDO3_GPI_MASK		(0x03 << 5)
#define DA9062AA_LDO3_CONF_SHIFT	7
#define DA9062AA_LDO3_CONF_MASK		BIT(7)

/* DA9062AA_LDO4_CONT = 0x029 */
#define DA9062AA_LDO4_EN_SHIFT		0
#define DA9062AA_LDO4_EN_MASK		0x01
#define DA9062AA_LDO4_GPI_SHIFT		1
#define DA9062AA_LDO4_GPI_MASK		(0x03 << 1)
#define DA9062AA_LDO4_PD_DIS_SHIFT	3
#define DA9062AA_LDO4_PD_DIS_MASK	BIT(3)
#define DA9062AA_VLDO4_GPI_SHIFT	5
#define DA9062AA_VLDO4_GPI_MASK		(0x03 << 5)
#define DA9062AA_LDO4_CONF_SHIFT	7
#define DA9062AA_LDO4_CONF_MASK		BIT(7)

/* DA9062AA_DVC_1 = 0x032 */
#define DA9062AA_VBUCK1_SEL_SHIFT	0
#define DA9062AA_VBUCK1_SEL_MASK	0x01
#define DA9062AA_VBUCK2_SEL_SHIFT	1
#define DA9062AA_VBUCK2_SEL_MASK	BIT(1)
#define DA9062AA_VBUCK4_SEL_SHIFT	2
#define DA9062AA_VBUCK4_SEL_MASK	BIT(2)
#define DA9062AA_VBUCK3_SEL_SHIFT	3
#define DA9062AA_VBUCK3_SEL_MASK	BIT(3)
#define DA9062AA_VLDO1_SEL_SHIFT	4
#define DA9062AA_VLDO1_SEL_MASK		BIT(4)
#define DA9062AA_VLDO2_SEL_SHIFT	5
#define DA9062AA_VLDO2_SEL_MASK		BIT(5)
#define DA9062AA_VLDO3_SEL_SHIFT	6
#define DA9062AA_VLDO3_SEL_MASK		BIT(6)
#define DA9062AA_VLDO4_SEL_SHIFT	7
#define DA9062AA_VLDO4_SEL_MASK		BIT(7)

/* DA9062AA_COUNT_S = 0x040 */
#define DA9062AA_COUNT_SEC_SHIFT	0
#define DA9062AA_COUNT_SEC_MASK		0x3f
#define DA9062AA_RTC_READ_SHIFT		7
#define DA9062AA_RTC_READ_MASK		BIT(7)

/* DA9062AA_COUNT_MI = 0x041 */
#define DA9062AA_COUNT_MIN_SHIFT	0
#define DA9062AA_COUNT_MIN_MASK		0x3f

/* DA9062AA_COUNT_H = 0x042 */
#define DA9062AA_COUNT_HOUR_SHIFT	0
#define DA9062AA_COUNT_HOUR_MASK	0x1f

/* DA9062AA_COUNT_D = 0x043 */
#define DA9062AA_COUNT_DAY_SHIFT	0
#define DA9062AA_COUNT_DAY_MASK		0x1f

/* DA9062AA_COUNT_MO = 0x044 */
#define DA9062AA_COUNT_MONTH_SHIFT	0
#define DA9062AA_COUNT_MONTH_MASK	0x0f

/* DA9062AA_COUNT_Y = 0x045 */
#define DA9062AA_COUNT_YEAR_SHIFT	0
#define DA9062AA_COUNT_YEAR_MASK	0x3f
#define DA9062AA_MONITOR_SHIFT		6
#define DA9062AA_MONITOR_MASK		BIT(6)

/* DA9062AA_ALARM_S = 0x046 */
#define DA9062AA_ALARM_SEC_SHIFT	0
#define DA9062AA_ALARM_SEC_MASK		0x3f
#define DA9062AA_ALARM_STATUS_SHIFT	6
#define DA9062AA_ALARM_STATUS_MASK	(0x03 << 6)

/* DA9062AA_ALARM_MI = 0x047 */
#define DA9062AA_ALARM_MIN_SHIFT	0
#define DA9062AA_ALARM_MIN_MASK		0x3f

/* DA9062AA_ALARM_H = 0x048 */
#define DA9062AA_ALARM_HOUR_SHIFT	0
#define DA9062AA_ALARM_HOUR_MASK	0x1f

/* DA9062AA_ALARM_D = 0x049 */
#define DA9062AA_ALARM_DAY_SHIFT	0
#define DA9062AA_ALARM_DAY_MASK		0x1f

/* DA9062AA_ALARM_MO = 0x04A */
#define DA9062AA_ALARM_MONTH_SHIFT	0
#define DA9062AA_ALARM_MONTH_MASK	0x0f
#define DA9062AA_TICK_TYPE_SHIFT	4
#define DA9062AA_TICK_TYPE_MASK		BIT(4)
#define DA9062AA_TICK_WAKE_SHIFT	5
#define DA9062AA_TICK_WAKE_MASK		BIT(5)

/* DA9062AA_ALARM_Y = 0x04B */
#define DA9062AA_ALARM_YEAR_SHIFT	0
#define DA9062AA_ALARM_YEAR_MASK	0x3f
#define DA9062AA_ALARM_ON_SHIFT		6
#define DA9062AA_ALARM_ON_MASK		BIT(6)
#define DA9062AA_TICK_ON_SHIFT		7
#define DA9062AA_TICK_ON_MASK		BIT(7)

/* DA9062AA_SECOND_A = 0x04C */
#define DA9062AA_SECONDS_A_SHIFT	0
#define DA9062AA_SECONDS_A_MASK		0xff

/* DA9062AA_SECOND_B = 0x04D */
#define DA9062AA_SECONDS_B_SHIFT	0
#define DA9062AA_SECONDS_B_MASK		0xff

/* DA9062AA_SECOND_C = 0x04E */
#define DA9062AA_SECONDS_C_SHIFT	0
#define DA9062AA_SECONDS_C_MASK		0xff

/* DA9062AA_SECOND_D = 0x04F */
#define DA9062AA_SECONDS_D_SHIFT	0
#define DA9062AA_SECONDS_D_MASK		0xff

/* DA9062AA_SEQ = 0x081 */
#define DA9062AA_SEQ_POINTER_SHIFT	0
#define DA9062AA_SEQ_POINTER_MASK	0x0f
#define DA9062AA_NXT_SEQ_START_SHIFT	4
#define DA9062AA_NXT_SEQ_START_MASK	(0x0f << 4)

/* DA9062AA_SEQ_TIMER = 0x082 */
#define DA9062AA_SEQ_TIME_SHIFT		0
#define DA9062AA_SEQ_TIME_MASK		0x0f
#define DA9062AA_SEQ_DUMMY_SHIFT	4
#define DA9062AA_SEQ_DUMMY_MASK		(0x0f << 4)

/* DA9062AA_ID_2_1 = 0x083 */
#define DA9062AA_LDO1_STEP_SHIFT	0
#define DA9062AA_LDO1_STEP_MASK		0x0f
#define DA9062AA_LDO2_STEP_SHIFT	4
#define DA9062AA_LDO2_STEP_MASK		(0x0f << 4)

/* DA9062AA_ID_4_3 = 0x084 */
#define DA9062AA_LDO3_STEP_SHIFT	0
#define DA9062AA_LDO3_STEP_MASK		0x0f
#define DA9062AA_LDO4_STEP_SHIFT	4
#define DA9062AA_LDO4_STEP_MASK		(0x0f << 4)

/* DA9062AA_ID_12_11 = 0x088 */
#define DA9062AA_PD_DIS_STEP_SHIFT	4
#define DA9062AA_PD_DIS_STEP_MASK	(0x0f << 4)

/* DA9062AA_ID_14_13 = 0x089 */
#define DA9062AA_BUCK1_STEP_SHIFT	0
#define DA9062AA_BUCK1_STEP_MASK	0x0f
#define DA9062AA_BUCK2_STEP_SHIFT	4
#define DA9062AA_BUCK2_STEP_MASK	(0x0f << 4)

/* DA9062AA_ID_16_15 = 0x08A */
#define DA9062AA_BUCK4_STEP_SHIFT	0
#define DA9062AA_BUCK4_STEP_MASK	0x0f
#define DA9062AA_BUCK3_STEP_SHIFT	4
#define DA9062AA_BUCK3_STEP_MASK	(0x0f << 4)

/* DA9062AA_ID_22_21 = 0x08D */
#define DA9062AA_GP_RISE1_STEP_SHIFT	0
#define DA9062AA_GP_RISE1_STEP_MASK	0x0f
#define DA9062AA_GP_FALL1_STEP_SHIFT	4
#define DA9062AA_GP_FALL1_STEP_MASK	(0x0f << 4)

/* DA9062AA_ID_24_23 = 0x08E */
#define DA9062AA_GP_RISE2_STEP_SHIFT	0
#define DA9062AA_GP_RISE2_STEP_MASK	0x0f
#define DA9062AA_GP_FALL2_STEP_SHIFT	4
#define DA9062AA_GP_FALL2_STEP_MASK	(0x0f << 4)

/* DA9062AA_ID_26_25 = 0x08F */
#define DA9062AA_GP_RISE3_STEP_SHIFT	0
#define DA9062AA_GP_RISE3_STEP_MASK	0x0f
#define DA9062AA_GP_FALL3_STEP_SHIFT	4
#define DA9062AA_GP_FALL3_STEP_MASK	(0x0f << 4)

/* DA9062AA_ID_28_27 = 0x090 */
#define DA9062AA_GP_RISE4_STEP_SHIFT	0
#define DA9062AA_GP_RISE4_STEP_MASK	0x0f
#define DA9062AA_GP_FALL4_STEP_SHIFT	4
#define DA9062AA_GP_FALL4_STEP_MASK	(0x0f << 4)

/* DA9062AA_ID_30_29 = 0x091 */
#define DA9062AA_GP_RISE5_STEP_SHIFT	0
#define DA9062AA_GP_RISE5_STEP_MASK	0x0f
#define DA9062AA_GP_FALL5_STEP_SHIFT	4
#define DA9062AA_GP_FALL5_STEP_MASK	(0x0f << 4)

/* DA9062AA_ID_32_31 = 0x092 */
#define DA9062AA_WAIT_STEP_SHIFT	0
#define DA9062AA_WAIT_STEP_MASK		0x0f
#define DA9062AA_EN32K_STEP_SHIFT	4
#define DA9062AA_EN32K_STEP_MASK	(0x0f << 4)

/* DA9062AA_SEQ_A = 0x095 */
#define DA9062AA_SYSTEM_END_SHIFT	0
#define DA9062AA_SYSTEM_END_MASK	0x0f
#define DA9062AA_POWER_END_SHIFT	4
#define DA9062AA_POWER_END_MASK		(0x0f << 4)

/* DA9062AA_SEQ_B = 0x096 */
#define DA9062AA_MAX_COUNT_SHIFT	0
#define DA9062AA_MAX_COUNT_MASK		0x0f
#define DA9062AA_PART_DOWN_SHIFT	4
#define DA9062AA_PART_DOWN_MASK		(0x0f << 4)

/* DA9062AA_WAIT = 0x097 */
#define DA9062AA_WAIT_TIME_SHIFT	0
#define DA9062AA_WAIT_TIME_MASK		0x0f
#define DA9062AA_WAIT_MODE_SHIFT	4
#define DA9062AA_WAIT_MODE_MASK		BIT(4)
#define DA9062AA_TIME_OUT_SHIFT		5
#define DA9062AA_TIME_OUT_MASK		BIT(5)
#define DA9062AA_WAIT_DIR_SHIFT		6
#define DA9062AA_WAIT_DIR_MASK		(0x03 << 6)

/* DA9062AA_EN_32K = 0x098 */
#define DA9062AA_STABILISATION_TIME_SHIFT	0
#define DA9062AA_STABILISATION_TIME_MASK	0x07
#define DA9062AA_CRYSTAL_SHIFT			3
#define DA9062AA_CRYSTAL_MASK			BIT(3)
#define DA9062AA_DELAY_MODE_SHIFT		4
#define DA9062AA_DELAY_MODE_MASK		BIT(4)
#define DA9062AA_OUT_CLOCK_SHIFT		5
#define DA9062AA_OUT_CLOCK_MASK			BIT(5)
#define DA9062AA_RTC_CLOCK_SHIFT		6
#define DA9062AA_RTC_CLOCK_MASK			BIT(6)
#define DA9062AA_EN_32KOUT_SHIFT		7
#define DA9062AA_EN_32KOUT_MASK			BIT(7)

/* DA9062AA_RESET = 0x099 */
#define DA9062AA_RESET_TIMER_SHIFT	0
#define DA9062AA_RESET_TIMER_MASK	0x3f
#define DA9062AA_RESET_EVENT_SHIFT	6
#define DA9062AA_RESET_EVENT_MASK	(0x03 << 6)

/* DA9062AA_BUCK_ILIM_A = 0x09A */
#define DA9062AA_BUCK3_ILIM_SHIFT	0
#define DA9062AA_BUCK3_ILIM_MASK	0x0f

/* DA9062AA_BUCK_ILIM_B = 0x09B */
#define DA9062AA_BUCK4_ILIM_SHIFT	0
#define DA9062AA_BUCK4_ILIM_MASK	0x0f

/* DA9062AA_BUCK_ILIM_C = 0x09C */
#define DA9062AA_BUCK1_ILIM_SHIFT	0
#define DA9062AA_BUCK1_ILIM_MASK	0x0f
#define DA9062AA_BUCK2_ILIM_SHIFT	4
#define DA9062AA_BUCK2_ILIM_MASK	(0x0f << 4)

/* DA9062AA_BUCK2_CFG = 0x09D */
#define DA9062AA_BUCK2_PD_DIS_SHIFT	5
#define DA9062AA_BUCK2_PD_DIS_MASK	BIT(5)
#define DA9062AA_BUCK2_MODE_SHIFT	6
#define DA9062AA_BUCK2_MODE_MASK	(0x03 << 6)

/* DA9062AA_BUCK1_CFG = 0x09E */
#define DA9062AA_BUCK1_PD_DIS_SHIFT	5
#define DA9062AA_BUCK1_PD_DIS_MASK	BIT(5)
#define DA9062AA_BUCK1_MODE_SHIFT	6
#define DA9062AA_BUCK1_MODE_MASK	(0x03 << 6)

/* DA9062AA_BUCK4_CFG = 0x09F */
#define DA9062AA_BUCK4_VTTR_EN_SHIFT	3
#define DA9062AA_BUCK4_VTTR_EN_MASK	BIT(3)
#define DA9062AA_BUCK4_VTT_EN_SHIFT	4
#define DA9062AA_BUCK4_VTT_EN_MASK	BIT(4)
#define DA9062AA_BUCK4_PD_DIS_SHIFT	5
#define DA9062AA_BUCK4_PD_DIS_MASK	BIT(5)
#define DA9062AA_BUCK4_MODE_SHIFT	6
#define DA9062AA_BUCK4_MODE_MASK	(0x03 << 6)

/* DA9062AA_BUCK3_CFG = 0x0A0 */
#define DA9062AA_BUCK3_PD_DIS_SHIFT	5
#define DA9062AA_BUCK3_PD_DIS_MASK	BIT(5)
#define DA9062AA_BUCK3_MODE_SHIFT	6
#define DA9062AA_BUCK3_MODE_MASK	(0x03 << 6)

/* DA9062AA_VBUCK2_A = 0x0A3 */
#define DA9062AA_VBUCK2_A_SHIFT		0
#define DA9062AA_VBUCK2_A_MASK		0x7f
#define DA9062AA_BUCK2_SL_A_SHIFT	7
#define DA9062AA_BUCK2_SL_A_MASK	BIT(7)

/* DA9062AA_VBUCK1_A = 0x0A4 */
#define DA9062AA_VBUCK1_A_SHIFT		0
#define DA9062AA_VBUCK1_A_MASK		0x7f
#define DA9062AA_BUCK1_SL_A_SHIFT	7
#define DA9062AA_BUCK1_SL_A_MASK	BIT(7)

/* DA9062AA_VBUCK4_A = 0x0A5 */
#define DA9062AA_VBUCK4_A_SHIFT		0
#define DA9062AA_VBUCK4_A_MASK		0x7f
#define DA9062AA_BUCK4_SL_A_SHIFT	7
#define DA9062AA_BUCK4_SL_A_MASK	BIT(7)

/* DA9062AA_VBUCK3_A = 0x0A7 */
#define DA9062AA_VBUCK3_A_SHIFT		0
#define DA9062AA_VBUCK3_A_MASK		0x7f
#define DA9062AA_BUCK3_SL_A_SHIFT	7
#define DA9062AA_BUCK3_SL_A_MASK	BIT(7)

/* DA9062AA_VLDO1_A = 0x0A9 */
#define DA9062AA_VLDO1_A_SHIFT		0
#define DA9062AA_VLDO1_A_MASK		0x3f
#define DA9062AA_LDO1_SL_A_SHIFT	7
#define DA9062AA_LDO1_SL_A_MASK		BIT(7)

/* DA9062AA_VLDO2_A = 0x0AA */
#define DA9062AA_VLDO2_A_SHIFT		0
#define DA9062AA_VLDO2_A_MASK		0x3f
#define DA9062AA_LDO2_SL_A_SHIFT	7
#define DA9062AA_LDO2_SL_A_MASK		BIT(7)

/* DA9062AA_VLDO3_A = 0x0AB */
#define DA9062AA_VLDO3_A_SHIFT		0
#define DA9062AA_VLDO3_A_MASK		0x3f
#define DA9062AA_LDO3_SL_A_SHIFT	7
#define DA9062AA_LDO3_SL_A_MASK		BIT(7)

/* DA9062AA_VLDO4_A = 0x0AC */
#define DA9062AA_VLDO4_A_SHIFT		0
#define DA9062AA_VLDO4_A_MASK		0x3f
#define DA9062AA_LDO4_SL_A_SHIFT	7
#define DA9062AA_LDO4_SL_A_MASK		BIT(7)

/* DA9062AA_VBUCK2_B = 0x0B4 */
#define DA9062AA_VBUCK2_B_SHIFT		0
#define DA9062AA_VBUCK2_B_MASK		0x7f
#define DA9062AA_BUCK2_SL_B_SHIFT	7
#define DA9062AA_BUCK2_SL_B_MASK	BIT(7)

/* DA9062AA_VBUCK1_B = 0x0B5 */
#define DA9062AA_VBUCK1_B_SHIFT		0
#define DA9062AA_VBUCK1_B_MASK		0x7f
#define DA9062AA_BUCK1_SL_B_SHIFT	7
#define DA9062AA_BUCK1_SL_B_MASK	BIT(7)

/* DA9062AA_VBUCK4_B = 0x0B6 */
#define DA9062AA_VBUCK4_B_SHIFT		0
#define DA9062AA_VBUCK4_B_MASK		0x7f
#define DA9062AA_BUCK4_SL_B_SHIFT	7
#define DA9062AA_BUCK4_SL_B_MASK	BIT(7)

/* DA9062AA_VBUCK3_B = 0x0B8 */
#define DA9062AA_VBUCK3_B_SHIFT		0
#define DA9062AA_VBUCK3_B_MASK		0x7f
#define DA9062AA_BUCK3_SL_B_SHIFT	7
#define DA9062AA_BUCK3_SL_B_MASK	BIT(7)

/* DA9062AA_VLDO1_B = 0x0BA */
#define DA9062AA_VLDO1_B_SHIFT		0
#define DA9062AA_VLDO1_B_MASK		0x3f
#define DA9062AA_LDO1_SL_B_SHIFT	7
#define DA9062AA_LDO1_SL_B_MASK		BIT(7)

/* DA9062AA_VLDO2_B = 0x0BB */
#define DA9062AA_VLDO2_B_SHIFT		0
#define DA9062AA_VLDO2_B_MASK		0x3f
#define DA9062AA_LDO2_SL_B_SHIFT	7
#define DA9062AA_LDO2_SL_B_MASK		BIT(7)

/* DA9062AA_VLDO3_B = 0x0BC */
#define DA9062AA_VLDO3_B_SHIFT		0
#define DA9062AA_VLDO3_B_MASK		0x3f
#define DA9062AA_LDO3_SL_B_SHIFT	7
#define DA9062AA_LDO3_SL_B_MASK		BIT(7)

/* DA9062AA_VLDO4_B = 0x0BD */
#define DA9062AA_VLDO4_B_SHIFT		0
#define DA9062AA_VLDO4_B_MASK		0x3f
#define DA9062AA_LDO4_SL_B_SHIFT	7
#define DA9062AA_LDO4_SL_B_MASK		BIT(7)

/* DA9062AA_BBAT_CONT = 0x0C5 */
#define DA9062AA_BCHG_VSET_SHIFT	0
#define DA9062AA_BCHG_VSET_MASK		0x0f
#define DA9062AA_BCHG_ISET_SHIFT	4
#define DA9062AA_BCHG_ISET_MASK		(0x0f << 4)

/* DA9062AA_INTERFACE = 0x105 */
#define DA9062AA_IF_BASE_ADDR_SHIFT	4
#define DA9062AA_IF_BASE_ADDR_MASK	(0x0f << 4)

/* DA9062AA_CONFIG_A = 0x106 */
#define DA9062AA_PM_I_V_SHIFT		0
#define DA9062AA_PM_I_V_MASK		0x01
#define DA9062AA_PM_O_TYPE_SHIFT	2
#define DA9062AA_PM_O_TYPE_MASK		BIT(2)
#define DA9062AA_IRQ_TYPE_SHIFT		3
#define DA9062AA_IRQ_TYPE_MASK		BIT(3)
#define DA9062AA_PM_IF_V_SHIFT		4
#define DA9062AA_PM_IF_V_MASK		BIT(4)
#define DA9062AA_PM_IF_FMP_SHIFT	5
#define DA9062AA_PM_IF_FMP_MASK		BIT(5)
#define DA9062AA_PM_IF_HSM_SHIFT	6
#define DA9062AA_PM_IF_HSM_MASK		BIT(6)

/* DA9062AA_CONFIG_B = 0x107 */
#define DA9062AA_VDD_FAULT_ADJ_SHIFT	0
#define DA9062AA_VDD_FAULT_ADJ_MASK	0x0f
#define DA9062AA_VDD_HYST_ADJ_SHIFT	4
#define DA9062AA_VDD_HYST_ADJ_MASK	(0x07 << 4)

/* DA9062AA_CONFIG_C = 0x108 */
#define DA9062AA_BUCK_ACTV_DISCHRG_SHIFT	2
#define DA9062AA_BUCK_ACTV_DISCHRG_MASK		BIT(2)
#define DA9062AA_BUCK1_CLK_INV_SHIFT		3
#define DA9062AA_BUCK1_CLK_INV_MASK		BIT(3)
#define DA9062AA_BUCK4_CLK_INV_SHIFT		4
#define DA9062AA_BUCK4_CLK_INV_MASK		BIT(4)
#define DA9062AA_BUCK3_CLK_INV_SHIFT		6
#define DA9062AA_BUCK3_CLK_INV_MASK		BIT(6)

/* DA9062AA_CONFIG_D = 0x109 */
#define DA9062AA_GPI_V_SHIFT		0
#define DA9062AA_GPI_V_MASK		0x01
#define DA9062AA_NIRQ_MODE_SHIFT	1
#define DA9062AA_NIRQ_MODE_MASK		BIT(1)
#define DA9062AA_SYSTEM_EN_RD_SHIFT	2
#define DA9062AA_SYSTEM_EN_RD_MASK	BIT(2)
#define DA9062AA_FORCE_RESET_SHIFT	5
#define DA9062AA_FORCE_RESET_MASK	BIT(5)

/* DA9062AA_CONFIG_E = 0x10A */
#define DA9062AA_BUCK1_AUTO_SHIFT	0
#define DA9062AA_BUCK1_AUTO_MASK	0x01
#define DA9062AA_BUCK2_AUTO_SHIFT	1
#define DA9062AA_BUCK2_AUTO_MASK	BIT(1)
#define DA9062AA_BUCK4_AUTO_SHIFT	2
#define DA9062AA_BUCK4_AUTO_MASK	BIT(2)
#define DA9062AA_BUCK3_AUTO_SHIFT	4
#define DA9062AA_BUCK3_AUTO_MASK	BIT(4)

/* DA9062AA_CONFIG_G = 0x10C */
#define DA9062AA_LDO1_AUTO_SHIFT	0
#define DA9062AA_LDO1_AUTO_MASK		0x01
#define DA9062AA_LDO2_AUTO_SHIFT	1
#define DA9062AA_LDO2_AUTO_MASK		BIT(1)
#define DA9062AA_LDO3_AUTO_SHIFT	2
#define DA9062AA_LDO3_AUTO_MASK		BIT(2)
#define DA9062AA_LDO4_AUTO_SHIFT	3
#define DA9062AA_LDO4_AUTO_MASK		BIT(3)

/* DA9062AA_CONFIG_H = 0x10D */
#define DA9062AA_BUCK1_2_MERGE_SHIFT	3
#define DA9062AA_BUCK1_2_MERGE_MASK	BIT(3)
#define DA9062AA_BUCK2_OD_SHIFT		5
#define DA9062AA_BUCK2_OD_MASK		BIT(5)
#define DA9062AA_BUCK1_OD_SHIFT		6
#define DA9062AA_BUCK1_OD_MASK		BIT(6)

/* DA9062AA_CONFIG_I = 0x10E */
#define DA9062AA_NONKEY_PIN_SHIFT	0
#define DA9062AA_NONKEY_PIN_MASK	0x03
#define DA9062AA_nONKEY_SD_SHIFT	2
#define DA9062AA_nONKEY_SD_MASK		BIT(2)
#define DA9062AA_WATCHDOG_SD_SHIFT	3
#define DA9062AA_WATCHDOG_SD_MASK	BIT(3)
#define DA9062AA_KEY_SD_MODE_SHIFT	4
#define DA9062AA_KEY_SD_MODE_MASK	BIT(4)
#define DA9062AA_HOST_SD_MODE_SHIFT	5
#define DA9062AA_HOST_SD_MODE_MASK	BIT(5)
#define DA9062AA_INT_SD_MODE_SHIFT	6
#define DA9062AA_INT_SD_MODE_MASK	BIT(6)
#define DA9062AA_LDO_SD_SHIFT		7
#define DA9062AA_LDO_SD_MASK		BIT(7)

/* DA9062AA_CONFIG_J = 0x10F */
#define DA9062AA_KEY_DELAY_SHIFT	0
#define DA9062AA_KEY_DELAY_MASK		0x03
#define DA9062AA_SHUT_DELAY_SHIFT	2
#define DA9062AA_SHUT_DELAY_MASK	(0x03 << 2)
#define DA9062AA_RESET_DURATION_SHIFT	4
#define DA9062AA_RESET_DURATION_MASK	(0x03 << 4)
#define DA9062AA_TWOWIRE_TO_SHIFT	6
#define DA9062AA_TWOWIRE_TO_MASK	BIT(6)
#define DA9062AA_IF_RESET_SHIFT		7
#define DA9062AA_IF_RESET_MASK		BIT(7)

/* DA9062AA_CONFIG_K = 0x110 */
#define DA9062AA_GPIO0_PUPD_SHIFT	0
#define DA9062AA_GPIO0_PUPD_MASK	0x01
#define DA9062AA_GPIO1_PUPD_SHIFT	1
#define DA9062AA_GPIO1_PUPD_MASK	BIT(1)
#define DA9062AA_GPIO2_PUPD_SHIFT	2
#define DA9062AA_GPIO2_PUPD_MASK	BIT(2)
#define DA9062AA_GPIO3_PUPD_SHIFT	3
#define DA9062AA_GPIO3_PUPD_MASK	BIT(3)
#define DA9062AA_GPIO4_PUPD_SHIFT	4
#define DA9062AA_GPIO4_PUPD_MASK	BIT(4)

/* DA9062AA_CONFIG_M = 0x112 */
#define DA9062AA_NSHUTDOWN_PU_SHIFT	1
#define DA9062AA_NSHUTDOWN_PU_MASK	BIT(1)
#define DA9062AA_WDG_MODE_SHIFT		3
#define DA9062AA_WDG_MODE_MASK		BIT(3)
#define DA9062AA_OSC_FRQ_SHIFT		4
#define DA9062AA_OSC_FRQ_MASK		(0x0f << 4)

/* DA9062AA_TRIM_CLDR = 0x120 */
#define DA9062AA_TRIM_CLDR_SHIFT	0
#define DA9062AA_TRIM_CLDR_MASK		0xff

/* DA9062AA_GP_ID_0 = 0x121 */
#define DA9062AA_GP_0_SHIFT		0
#define DA9062AA_GP_0_MASK		0xff

/* DA9062AA_GP_ID_1 = 0x122 */
#define DA9062AA_GP_1_SHIFT		0
#define DA9062AA_GP_1_MASK		0xff

/* DA9062AA_GP_ID_2 = 0x123 */
#define DA9062AA_GP_2_SHIFT		0
#define DA9062AA_GP_2_MASK		0xff

/* DA9062AA_GP_ID_3 = 0x124 */
#define DA9062AA_GP_3_SHIFT		0
#define DA9062AA_GP_3_MASK		0xff

/* DA9062AA_GP_ID_4 = 0x125 */
#define DA9062AA_GP_4_SHIFT		0
#define DA9062AA_GP_4_MASK		0xff

/* DA9062AA_GP_ID_5 = 0x126 */
#define DA9062AA_GP_5_SHIFT		0
#define DA9062AA_GP_5_MASK		0xff

/* DA9062AA_GP_ID_6 = 0x127 */
#define DA9062AA_GP_6_SHIFT		0
#define DA9062AA_GP_6_MASK		0xff

/* DA9062AA_GP_ID_7 = 0x128 */
#define DA9062AA_GP_7_SHIFT		0
#define DA9062AA_GP_7_MASK		0xff

/* DA9062AA_GP_ID_8 = 0x129 */
#define DA9062AA_GP_8_SHIFT		0
#define DA9062AA_GP_8_MASK		0xff

/* DA9062AA_GP_ID_9 = 0x12A */
#define DA9062AA_GP_9_SHIFT		0
#define DA9062AA_GP_9_MASK		0xff

/* DA9062AA_GP_ID_10 = 0x12B */
#define DA9062AA_GP_10_SHIFT		0
#define DA9062AA_GP_10_MASK		0xff

/* DA9062AA_GP_ID_11 = 0x12C */
#define DA9062AA_GP_11_SHIFT		0
#define DA9062AA_GP_11_MASK		0xff

/* DA9062AA_GP_ID_12 = 0x12D */
#define DA9062AA_GP_12_SHIFT		0
#define DA9062AA_GP_12_MASK		0xff

/* DA9062AA_GP_ID_13 = 0x12E */
#define DA9062AA_GP_13_SHIFT		0
#define DA9062AA_GP_13_MASK		0xff

/* DA9062AA_GP_ID_14 = 0x12F */
#define DA9062AA_GP_14_SHIFT		0
#define DA9062AA_GP_14_MASK		0xff

/* DA9062AA_GP_ID_15 = 0x130 */
#define DA9062AA_GP_15_SHIFT		0
#define DA9062AA_GP_15_MASK		0xff

/* DA9062AA_GP_ID_16 = 0x131 */
#define DA9062AA_GP_16_SHIFT		0
#define DA9062AA_GP_16_MASK		0xff

/* DA9062AA_GP_ID_17 = 0x132 */
#define DA9062AA_GP_17_SHIFT		0
#define DA9062AA_GP_17_MASK		0xff

/* DA9062AA_GP_ID_18 = 0x133 */
#define DA9062AA_GP_18_SHIFT		0
#define DA9062AA_GP_18_MASK		0xff

/* DA9062AA_GP_ID_19 = 0x134 */
#define DA9062AA_GP_19_SHIFT		0
#define DA9062AA_GP_19_MASK		0xff

/* DA9062AA_DEVICE_ID = 0x181 */
#define DA9062AA_DEV_ID_SHIFT		0
#define DA9062AA_DEV_ID_MASK		0xff

/* DA9062AA_VARIANT_ID = 0x182 */
#define DA9062AA_VRC_SHIFT		0
#define DA9062AA_VRC_MASK		0x0f
#define DA9062AA_MRC_SHIFT		4
#define DA9062AA_MRC_MASK		(0x0f << 4)

/* DA9062AA_CUSTOMER_ID = 0x183 */
#define DA9062AA_CUST_ID_SHIFT		0
#define DA9062AA_CUST_ID_MASK		0xff

/* DA9062AA_CONFIG_ID = 0x184 */
#define DA9062AA_CONFIG_REV_SHIFT	0
#define DA9062AA_CONFIG_REV_MASK	0xff

#endif /* __DA9062_H__ */
