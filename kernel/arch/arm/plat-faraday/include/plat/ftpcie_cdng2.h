/*
 *  arch/arm/mach-wideband001/include/mach/ftpcie_cdn_g2.h
 *
 *  (C) Copyright 2015 Faraday Technology
 *  Mark Fu-Tsung <mark_hs@faraday-tech.com>
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

#ifndef __FTPCIE_CDNG2_H
#define __FTPCIE_CDNG2_H

/******************Base Address******************/
#define PLAT_FTPCIE_AXISLV_PA_BASE          PLAT_FTPCIE_AXISLAVE_BASE
#define PLAT_FTPCIE_AXISLV_1_PA_BASE        PLAT_FTPCIE_AXISLAVE_1_BASE

#define PLAT_FTPCIE_REG_PA_BASE             PLAT_FTPCIE_WRAP_BASE
#define PLAT_FTPCIE_REG_1_PA_BASE           PLAT_FTPCIE_WRAP_1_BASE

#define PLAT_FTPCIE_WRAP_PA_BASE            PLAT_FTPCIE_WRAP_GLUE_BASE
#define PLAT_FTPCIE_WRAP_1_PA_BASE          PLAT_FTPCIE_WRAP_GLUE_1_BASE

#define PLAT_FTDDR_PA_BASE                  0x80000000

#define PCIE_CDNG2_MEM_PA_LENGTH            0x10000000
#define PCIE_CDNG2_IO_PA_LENGTH             0x100000

/***********PCIE Glue register offset************/
#define PCIE_CTRL3_OFFSET                   0x1C	//PCIE Control 3 Register
#define PCIE_INT_MASK_OFFSET                0x38	//PCIE Interrupt Mask Register
#define PCIE_INT_STATUS_OFFSET              0x48	//PCIE Interrupt Status Register
#define PCIE_LINK_STATUS_OFFSET             0x4C	//PCIE Link Status Register

/**************PCIE register offset**************/
/*
 * Root Port Configuration Registers
 */
#define CMD_STS_OFFSET                      0x04	//command and status register
#define RC_BAR0_OFFSET                      0x10	//Root Complex Base Address Register 0 (RC BAR 0)
#define RC_BAR1_OFFSET                      0x14	//Root Complex Base Address Register 1 (RC BAR 1)#define ISE                                 0x1<<0	//IO-Space Enable
#define CAP_OFFSET                          0xC4    //PCI Express Device Capabilities Register
#define DEV_CTRL_OFFSET                     0xC8	//PCI Express Device Control and Status Register
#define MAX_READ_PAYLOAD_128BYTES           (0x0 << 5)
#define MAX_READ_PAYLOAD_256BYTES           (0x1 << 5)
#define MAX_READ_PAYLOAD_512BYTES           (0x2 << 5)
#define MAX_READ_PAYLOAD_1024BYTES          (0x3 << 5)
#define MAX_READ_PAYLOAD_2048BYTES          (0x4 << 5)
#define MAX_READ_PAYLOAD_4096BYTES          (0x5 << 5)
#define MAX_READ_REQ_128BYTES               (0x0 << 12)
#define MAX_READ_REQ_256BYTES               (0x1 << 12)
#define MAX_READ_REQ_512BYTES               (0x2 << 12)
#define MAX_READ_REQ_1024BYTES              (0x3 << 12)
#define MAX_READ_REQ_2048BYTES              (0x4 << 12)
#define MAX_READ_REQ_4096BYTES              (0x5 << 12)
#define LINK_CTRL_OFFSET                    0xD0	//link control and register
#define DEV_CTRL2_OFFSET                    0xE8	//PCI Express Device Control and Status 2 Register

/*
 * Local Management Registers
 */
#define LOCAL_MGMT_OFFSET                   0x00100000

#define DBG_MUX_CTLR_OFFSET                 0x208	//Debug Mux Control
#define RC_BAR_CFG_OFFSET                   0x300	//Root Complex BAR Configuration Register
#define RC_BAR0_SIZE_1M                     (0x12 << 0)
#define RC_BAR0_SIZE_2M                     (0x13 << 0)
#define RC_BAR0_SIZE_4M                     (0x14 << 0)
#define RC_BAR0_SIZE_8M                     (0x15 << 0)
#define RC_BAR0_SIZE_16M                    (0x16 << 0)
#define RC_BAR0_SIZE_32M                    (0x17 << 0)
#define RC_BAR0_SIZE_64M                    (0x18 << 0)
#define RC_BAR0_SIZE_128M                   (0x19 << 0)
#define RC_BAR0_SIZE_256M                   (0x1A << 0)
#define RC_BAR0_SIZE_512M                   (0x1B << 0)
#define RC_BAR0_SIZE_1G                     (0x1C << 0)
#define RC_BAR0_IO                          (0x1 << 6)	//32-bit IO BAR
#define RC_BAR0_MEM_NONPREFETCH             (0x4 << 6)	//32-bit memory BAR, nonprefetchable
#define RC_BAR0_MEM_PREFETCH                (0x5 << 6)	//32-bit memory BAR, prefetchable
#define RC_BAR_CHECK_EN                     (0x1 << 31)

/*
 * Address Translation Registers
 */
#define ADDR_TRANS_OFFSET                   0x00400000
#define REGION0_ADDR0                       0x000
#define REGION0_ADDR1                       0x004
#define REGION0_DESC0                       0x008
#define REGION0_DESC1                       0x00C
#define REGION0_DESC2                       0x010
#define REGION0_DESC3                       0x014
#define REGION1_ADDR0                       0x020
#define REGION1_ADDR1                       0x024
#define REGION1_DESC0                       0x028
#define REGION1_DESC1                       0x02C
#define REGION1_DESC2                       0x030
#define REGION1_DESC3                       0x034
#define REGION2_ADDR0                       0x040
#define REGION2_ADDR1                       0x044
#define REGION2_DESC0                       0x048
#define REGION2_DESC1                       0x04C
#define REGION2_DESC2                       0x050
#define REGION2_DESC3                       0x054
#define REGION3_ADDR0                       0x060
#define REGION3_ADDR1                       0x064
#define REGION3_DESC0                       0x068
#define REGION3_DESC1                       0x06C
#define REGION3_DESC2                       0x070
#define REGION3_DESC3                       0x074
#define RP_IB_BAR0_ADDR0_OFFSET             0x800
#define RP_IB_BAR0_ADDR1_OFFSET             0x804

#endif //__FTPCIE_CDNG2_H
