#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/can/dev.h>
#include <linux/can/error.h>
#include <linux/can/led.h>

/* CAN registers set */
enum ftcan010_reg {
	FTCAN010_CONTROL_OFFSET		= 0x00,
	FTCAN010_STATUS_OFFSET		= 0x04,
	FTCAN010_TR0_OFFSET			= 0x08,
	FTCAN010_TR1_OFFSET			= 0x10,
	FTCAN010_TR2_OFFSET			= 0x18,
	FTCAN010_AFIR0_OFFSET		= 0x20,
	FTCAN010_AFIR1_OFFSET		= 0x24,
	FTCAN010_AFIR2_OFFSET		= 0x28,
	FTCAN010_AFIR3_OFFSET		= 0x2C,
	FTCAN010_AFIR4_OFFSET		= 0x30,
	FTCAN010_AFIR5_OFFSET		= 0x34,
	FTCAN010_AFRDATA_OFFSET		= 0x38,
	FTCAN010_AFRCONTROL_OFFSET	= 0x3C,
	FTCAN010_AMIR0_OFFSET		= 0x40,
	FTCAN010_AMIR1_OFFSET		= 0x44,
	FTCAN010_AMRDATA_OFFSET		= 0x48,
	FTCAN010_AMRCONTROL_OFFSET	= 0x4C,
	FTCAN010_RR0_OFFSET			= 0x50,
	FTCAN010_RR1_OFFSET			= 0x58,
	FTCAN010_NBTC_OFFSET		= 0x60,
	FTCAN010_DBTC_OFFSET		= 0x68,
	FTCAN010_ERRCNT_OFFSET		= 0x6C,
	FTCAN010_ERRSTATUS_OFFSET	= 0x70,
	FTCAN010_TS_OFFSET			= 0x74,
	FTCAN010_IRR_OFFSET			= 0x78,
	FTCAN010_TX0DATA_OFFSET		= 0x7C,
	FTCAN010_TX1DATA_OFFSET		= 0xBC,
	FTCAN010_TX2DATA_OFFSET		= 0xFC,
	FTCAN010_RX0DATA_OFFSET		= 0x13C,
	FTCAN010_RX1DATA_OFFSET		= 0x17C,
};

/* CAN register bit masks - FTCAN010_<REG>_<BIT>_MASK */
#define FTCAN010_CONTROL_RT_MASK	0x18000000 /* Retransmission Times */
#define FTCAN010_CONTROL_RR_MASK	0x40000000 /* Soft Reset the CAN core */
#define FTCAN010_CONTROL_OMR_MASK	0x07000000 /* Operation Mode */
#define FTCAN010_CONTROL_WIE_MASK	0x00008000 /* Wake-up Interrupt Enable */
#define FTCAN010_CONTROL_OIE_MASK	0x00004000 /* Overrun Interrupt Enable */
#define FTCAN010_CONTROL_EIE_MASK	0x00002000 /* Error Interrupt Enable */
#define FTCAN010_CONTROL_TIE_MASK	0x00001000 /* Transmit Interrupt Enable */
#define FTCAN010_CONTROL_RIE_MASK	0x00000800 /* Receive Interrupt Enable */
#define FTCAN010_CONTROL_RRB0_MASK	0x00000080 /* Release Receive Buffer 0 */
#define FTCAN010_CONTROL_RRB1_MASK	0x00000040 /* Release Receive Buffer 1 */
#define FTCAN010_CONTROL_BTR0_MASK	0x00000020 /* Transmit Buffer 0 Request */
#define FTCAN010_CONTROL_BTR1_MASK	0x00000010 /* Transmit Buffer 1 Request */
#define FTCAN010_CONTROL_BTR2_MASK	0x00000008 /* Transmit Buffer 2 Request */
#define FTCAN010_CONTROL_BTRn_MASK	0x00000038 /* Transmit Buffer 0, 1, 2 Request */
#define FTCAN010_CONTROL_RRBA0_MASK	0x00000004 /* Release Receive FIFO 0 */
#define FTCAN010_CONTROL_RRBA1_MASK	0x00000002 /* Release Receive FIFO 1 */
#define FTCAN010_STATUS_BRS0_MASK	0x00001800 /* Receive Buffer (FIFO) 0 Status */
#define FTCAN010_STATUS_BRS1_MASK	0x00000600 /* Receive Buffer (FIFO) 1 Status */
#define FTCAN010_STATUS_BO_MASK		0x00000080 /* Bus-off */
#define FTCAN010_STATUS_EW_MASK		0x00000040 /* Error Warning */
#define FTCAN010_STATUS_TS_MASK		0x00000020 /* Transmit Status */
#define FTCAN010_STATUS_RS_MASK		0x00000010 /* Receive Status */
#define FTCAN010_STATUS_DO_MASK		0x0000000C /* Data Overrun Field */
#define FTCAN010_TR_BI_MASK			0x00FFE000 /* (Tx Buffer) Based Identifier */
#define FTCAN010_TR_DL_MASK			0x00000F00 /* (Tx Buffer) Data Lentgh Code */
#define FTCAN010_TR_EIH_MASK		0x000000FF /* (Tx Buffer) Extended Identifier High */
#define FTCAN010_TR_EIL_MASK		0xFFC00000 /* (Tx Buffer) Extended Identifier Low */
#define FTCAN010_TR_RTR_MASK		0x00008000 /* (Tx Buffer) Remote Transmission Request */
#define FTCAN010_TR_EIE_MASK		0x00004000 /* (Tx Buffer) Extended Identifier Enable */
#define FTCAN010_TR_EDL_MASK		0x00000080 /* (Tx Buffer) Extended Data Length (FD Enable) */
#define FTCAN010_TR_BRS_MASK		0x00000040 /* (Tx Buffer) Baud Rate Switch */
#define FTCAN010_AFRCONTROL_RBAFG0E_MASK 	\
	0x00000080 /* Receive Buffer Acceptance Filter Group 0 Enable */
#define FTCAN010_AFRCONTROL_RBAFG1E_MASK 	\
	0x00000080 /* Receive Buffer Acceptance Filter Group 1 Enable */
#define FTCAN010_AMRCONTROL_FMA_MASK 		\
	0x00000080 /* Filter Mask All */
#define FTCAN010_AMRCONTROL_RBAFM0E_MASK 	\
	0x00000040 /* Receive Buffer Acceptance Filter Mask Group 0 Enable */
#define FTCAN010_AMRCONTROL_RBAFM1E_MASK 	\
	0x00000040 /* Receive Buffer Acceptance Filter Mask Group 1 Enable */
#define FTCAN010_RR_BI_MASK			0x00FFE000 /* (Rx Buffer) Based Identifier */
#define FTCAN010_RR_DL_MASK			0x00000F00 /* (Rx Buffer) Data Lentgh Code */
#define FTCAN010_RR_EIH_MASK		0x000000FF /* (Rx Buffer) Extended Identifier High */
#define FTCAN010_RR_EIL_MASK		0xFFC00000 /* (Rx Buffer) Extended Identifier Low */
#define FTCAN010_RR_RTR_MASK		0x00008000 /* (Rx Buffer) Remote Transmission Request */
#define FTCAN010_RR_EIE_MASK		0x00004000 /* (Rx Buffer) Extended Identifier Enable */
#define FTCAN010_RR_EDL_MASK		0x00000080 /* (Rx Buffer) Extended Data Length (FD Enable) */
#define FTCAN010_RR_BRS_MASK		0x00000040 /* (Rx Buffer) Baud Rate Switch */
#define FTCAN010_RR_ESI_MASK		0x00000020 /* (Rx Buffer) Error State Indicator */
#define FTCAN010_ERRCNT_TEC_MASK	0xFFFF0000 /* Transmitter Error Counter (TEC) */
#define FTCAN010_ERRCNT_REC_MASK	0x0000FFFF /* Receiver Error Counter (REC) */
#define FTCAN010_ERRSTATUS_BE_MASK	0x00000080 /* Bit Error */
#define FTCAN010_ERRSTATUS_SE_MASK	0x00000040 /* Stuff Error */
#define FTCAN010_ERRSTATUS_CE_MASK	0x00000020 /* CRC Error */
#define FTCAN010_ERRSTATUS_FE_MASK	0x00000010 /* Form Error */
#define FTCAN010_ERRSTATUS_AE_MASK	0x00000008 /* ACK Error */
#define FTCAN010_ERRSTATUS_OE_MASK	0x00000004 /* Overrun Error */
#define FTCAN010_IRR_WIR_MASK		0x00000080 /* Wake-up Interrupt Request */
#define FTCAN010_IRR_OIR_MASK		0x00000040 /* Overrun Interrupt Request */
#define FTCAN010_IRR_EIR_MASK		0x00000020 /* Error Interrupt Request */
#define FTCAN010_IRR_TBI0_MASK		0x00000010 /* Transmit Buffer Interrupt 0 */
#define FTCAN010_IRR_TBI1_MASK		0x00000008 /* Transmit Buffer Interrupt 1 */
#define FTCAN010_IRR_TBI2_MASK		0x00000004 /* Transmit Buffer Interrupt 2 */
#define FTCAN010_IRR_TBIn_MASK		0x0000001C /* TBIn (n=0,1,2) field */
#define FTCAN010_IRR_RBI0_MASK		0x00000002 /* Receive Buffer Interrupt 0 */
#define FTCAN010_IRR_RBI1_MASK		0x00000001 /* Receive Buffer Interrupt 1 */
#define FTCAN010_IRR_RBIn_MASK		0x00000003 /* RBIn (n=0,1) field */

#define FTCAN010_INTR_ALL (FTCAN010_CONTROL_OMR_MASK | 	\
	FTCAN010_CONTROL_WIE_MASK | 						\
	FTCAN010_CONTROL_OIE_MASK | 						\
	FTCAN010_CONTROL_EIE_MASK | 						\
	FTCAN010_CONTROL_TIE_MASK | 						\
	FTCAN010_CONTROL_RIE_MASK)

/* CAN register bit shift - FTCAN010_<REG>_<BIT>_SHIFT */
#define FTCAN010_CONTROL_RT_SHIFT	27 /* Retransmission Times */
#define FTCAN010_CONTROL_OMR_SHIFT	24 /* Operation Mode */
#define FTCAN010_STATUS_DO_SHIFT	2  /* Data Overrun */
#define FTCAN010_TR_BI_SHIFT		13 /* (Tx Buffer) Based Identifier */
#define FTCAN010_TR_DL_SHIFT		8  /* (Tx Buffer) Data Lentgh Code */
#define FTCAN010_TR_EIL_SHIFT		22 /* (Tx Buffer) Extended Identifier */
#define FTCAN010_RR_BI_SHIFT		13 /* (Rx Buffer) Based Identifier */
#define FTCAN010_RR_DL_SHIFT		8  /* (Rx Buffer) Data Lentgh Code */
#define FTCAN010_RR_EIL_SHIFT		22 /* (Rx Buffer) Extended Identifier */
#define FTCAN010_NBTC_NSJW_SHIFT	3  /* Nominal Synchronous Jump Width */
#define FTCAN010_NBTC_NBRP_SHIFT	8  /* Nominal Prescalar Baud Rate */
#define FTCAN010_NBTC_NPRSEG_SHIFT	17 /* Nominal Propagation Segment */
#define FTCAN010_NBTC_NPSEG1_SHIFT	27 /* Nominal Phase 1 Segment */
#define FTCAN010_NBTC_NPSEG2_SHIFT	3  /* Nominal Phase 2 Segment */
#define FTCAN010_DBTC_DSJW_SHIFT	9  /* Data Synchronous Jump Width */
#define FTCAN010_DBTC_DBRP_SHIFT	0  /* Data Prescalar Baud Rate */
#define FTCAN010_DBTC_DPRSEG_SHIFT	11 /* Data Propagation Segment */
#define FTCAN010_DBTC_DPSEG1_SHIFT	21 /* Data Phase 1 Segment */
#define FTCAN010_DBTC_DPSEG2_SHIFT	29 /* Data Phase 2 Segment */
#define FTCAN010_ERRCNT_TEC_SHIFT	16 /* Transmitter Error Counter (TEC) */
#define FTCAN010_ERRCNT_REC_SHIFT	0  /* Receiver Error Counter (REC) */

#define FTCAN010_CONFIG_MODE		0x0
#define FTCAN010_NORMAL_MODE		0x1
#define FTCAN010_SLEEP_MODE			0x2
#define FTCAN010_LISTEN_MODE		0x3
#define FTCAN010_LOOPBACK_MODE		0x4

#define FTCAN010_RT_ALWAYS		0
#define FTCAN010_RT_1_TIME		1
#define FTCAN010_RT_3_TIME		2
#define FTCAN010_RT_8_TIME		3

#define FTCAN010_TIMEOUT		(1 * HZ)
#define FTCAN010_RX_FIFO_DEPTH		3
#define FTCAN_GETEID(reg_h, reg_l) 				\
	(((rb_h & FTCAN010_RR_BI_MASK) << 5) | 		\
	 ((rb_h & FTCAN010_RR_EIH_MASK) << 10) | 	\
	 ((rb_l & FTCAN010_RR_EIL_MASK) >> FTCAN010_RR_EIL_SHIFT))
#define FTCAN_GETSID(reg_h) \
	((rb_h & FTCAN010_RR_BI_MASK) >> FTCAN010_RR_BI_SHIFT)
#define FTCAN_EID2REGH(id) \
	(((((id) & CAN_EFF_MASK) & 0x1FFC0000) >> 5) | \
	 ((((id) & CAN_EFF_MASK) & 0x0003FC00) >> 10))
#define FTCAN_EID2REGL(id) \
	((((id) & CAN_EFF_MASK) & 0x000003FF) << FTCAN010_TR_EIL_SHIFT)
#define FTCAN_SID2REGH(id) \
	(((id) & CAN_SFF_MASK) << FTCAN010_TR_BI_SHIFT)
enum ftcan010_rx_fifo { 
	FTCAN010_NOFIFO = 0, 
	FTCAN010_RXFIFO0   , 
	FTCAN010_RXFIFO1   , 
	FTCAN010_BOTHFIFOS , 
};

/**
 * struct ftcan010_priv - This definition define CAN driver instance
 * @can:			CAN private data structure.
 * @napi:			NAPI structure
 * @read_reg:			For reading data from CAN registers
 * @write_reg:			For writing data to CAN registers
 * @dev:			Network device data structure
 * @reg_base:			Ioremapped address to registers
 * @irq_flags:			For request_irq()
 * @can_clk:			Pointer to struct clk
 */
struct ftcan010_priv {
	struct can_priv can;
	struct napi_struct napi;
	u32 (*read_reg)(const struct ftcan010_priv *priv, 
			enum ftcan010_reg reg);
	void (*write_reg)(const struct ftcan010_priv *priv, 
			enum ftcan010_reg reg, 
			u32 val);
	struct net_device *dev;
	void __iomem *reg_base;
	unsigned long irq_flags;
	struct clk *can_clk;
};

/* CAN Bittiming constants as per FTCAN010 CAN specs */
static const struct can_bittiming_const ftcan010_bittiming_const = {
	.name = "FTCAN010",
	.tseg1_min = 1,
	.tseg1_max = 32,
	.tseg2_min = 1,
	.tseg2_max = 32,
	.sjw_max = 32,
	.brp_min = 2,
	.brp_max = 256,
	.brp_inc = 1,
};

/* CAN Bittiming constants as per FTCAN010 CAN specs */
static const struct can_bittiming_const ftcan010_data_bittiming_const = {
	.name = "FTCAN010",
	.tseg1_min = 1,
	.tseg1_max = 16,
	.tseg2_min = 1,
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 2,
	.brp_max = 256,
	.brp_inc = 1,
};

/**
 * ftcan010_write_reg_le - Write a value to the device register little endian
 * @priv:	Driver private data structure
 * @reg:	Register offset
 * @val:	Value to write at the Register offset
 *
 * Write data to the paricular CAN register
 */
static void ftcan010_write_reg_le(const struct ftcan010_priv *priv, enum ftcan010_reg reg,
	u32 val)
{
	iowrite32(val, priv->reg_base + reg);
}

/**
 * ftcan010_read_reg_le - Read a value from the device register little endian
 * @priv:	Driver private data structure
 * @reg:	Register offset
 *
 * Read data from the particular CAN register
 * Return: value read from the CAN register
 */
static u32 ftcan010_read_reg_le(const struct ftcan010_priv *priv, enum ftcan010_reg reg)
{
	return ioread32(priv->reg_base + reg);
}

/**
 * set_reset_mode - Resets the CAN device mode
 * @ndev:	Pointer to net_device structure
 *
 * This is the driver reset mode routine.The driver
 * enters into configuration mode.
 *
 * Return: 0 on success and failure value on error
 */
static int set_reset_mode(struct net_device *ndev)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	unsigned long timeout;

	/* Enable Reset Register (RR) bit */
	priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, FTCAN010_CONTROL_RR_MASK);

	/* Check RR bit is automatically cleared by HW, because the soft reset finish.*/
	timeout = jiffies + FTCAN010_TIMEOUT;
	while (priv->read_reg(priv, FTCAN010_CONTROL_OFFSET) & 
		FTCAN010_CONTROL_RR_MASK) {
		if (time_after(jiffies, timeout)) {
			netdev_warn(ndev, "timed out for entry config mode after Soft Reset\n");
			return -ETIMEDOUT;
		}
		usleep_range(500, 10000);
	}

	return 0;
}

/**
 * ftcan010_set_bittiming - CAN set bit timing routine
 * @ndev:	Pointer to net_device structure
 *
 * This is the driver set bittiming routine.
 * Return: 0 on success and failure value on error
 */
static int ftcan010_set_bittiming(struct net_device *ndev)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	struct can_bittiming *nbt = &priv->can.bittiming;
	u32 nbtc0 = 0, nbtc1 = 0;
	u32 is_config_mode;

	/* Check whether Faraday CAN is in configuration mode.
	 * It cannot set bit timing if Faraday CAN is not in configuration mode.
	 */
	is_config_mode = (priv->read_reg(priv, FTCAN010_CONTROL_OFFSET) & 
		FTCAN010_CONTROL_OMR_MASK);
	if (is_config_mode) {
		netdev_alert(ndev,
			"ERROR! Cannot set bittiming - CAN is not in config mode (%d)\n",
			(is_config_mode >> FTCAN010_CONTROL_OMR_SHIFT));
		return -EPERM;
	}

	/* Setting Nominal Baud Rate prescalar value in NBRP Register */
	nbtc0 |= (nbt->brp - 1) << FTCAN010_NBTC_NBRP_SHIFT;

	/* Setting Nominal Propagation Segment in NPRSEG Register */
	nbtc0 |= (nbt->prop_seg - 1) << FTCAN010_NBTC_NPRSEG_SHIFT;

	/* Setting Nominal Phase 1 Segment in NPSEG1 Register */
	nbtc0 |= (nbt->phase_seg1 - 1) << FTCAN010_NBTC_NPSEG1_SHIFT;

	/* Setting Nominal Phase 2 Segment in NPSEG2 Register */
	nbtc1 |= (nbt->phase_seg2 - 1) << FTCAN010_NBTC_NPSEG2_SHIFT;

	/* Setting Synchronous jump width in BTR Register */
	nbtc0 |= (nbt->sjw - 1) << FTCAN010_NBTC_NSJW_SHIFT;

	priv->write_reg(priv, FTCAN010_NBTC_OFFSET, nbtc0);
	priv->write_reg(priv, FTCAN010_NBTC_OFFSET + 4, nbtc1);
	netdev_dbg(ndev, "NBTC0=0x%08x, NBTC1=0x%08x\n",
		priv->read_reg(priv, FTCAN010_NBTC_OFFSET),
		priv->read_reg(priv, FTCAN010_NBTC_OFFSET + 4));
	netdev_dbg(ndev, "Nominal Phase: Prescalar:%d, Prop_seg:%d, PS1_seg:%d , PS2_seg:%d, SJW:%d\n",
		nbt->brp, 
		nbt->prop_seg, 
		nbt->phase_seg1, 
		nbt->phase_seg2,
		nbt->sjw);
	return 0;
}

/**
 * ftcan010_set_data_bittiming - CAN set bit timing routine
 * @ndev:	Pointer to net_device structure
 *
 * This is the driver set bittiming  routine.
 * Return: 0 on success and failure value on error
 */
static int ftcan010_set_data_bittiming(struct net_device *ndev)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	struct can_bittiming *dbt = &priv->can.data_bittiming;
	u32 dbtc = 0;
	u32 is_config_mode;
    
	/* Check whether Faraday CAN is in configuration mode.
	 * It cannot set bit timing if Faraday CAN is not in configuration mode.
	 */
	is_config_mode = (priv->read_reg(priv, FTCAN010_CONTROL_OFFSET) & 
		FTCAN010_CONTROL_OMR_MASK);
	if (is_config_mode) {
		netdev_alert(ndev,
			"ERROR! Cannot set bittiming - CAN is not in config mode (%d)\n",
			(is_config_mode >> FTCAN010_CONTROL_OMR_SHIFT));
		return -EPERM;
	}

	/* Setting Nominal Baud Rate prescalar value in NBRP Register */
	dbtc |= (dbt->brp - 1) << FTCAN010_DBTC_DBRP_SHIFT;

	/* Setting Nominal Propagation Segment in NPRSEG Register */
	dbtc |= (dbt->prop_seg) << FTCAN010_DBTC_DPRSEG_SHIFT;

	/* Setting Nominal Phase 1 Segment in NPSEG1 Register */
	dbtc |= (dbt->phase_seg1 - 1) << FTCAN010_DBTC_DPSEG1_SHIFT;

	/* Setting Nominal Phase 2 Segment in NPSEG2 Register */
	dbtc |= (dbt->phase_seg2 - 1) << FTCAN010_DBTC_DPSEG2_SHIFT;

	/* Setting Synchronous jump width in BTR Register */
	dbtc |= (dbt->sjw - 1) << FTCAN010_DBTC_DSJW_SHIFT;

	priv->write_reg(priv, FTCAN010_DBTC_OFFSET, dbtc);

	netdev_dbg(ndev, "DBTC=0x%08x\n",
			priv->read_reg(priv, FTCAN010_DBTC_OFFSET));
	netdev_dbg(ndev, "Data Phase: Prescalar:%d, Prop_seg:%d, PS1_seg:%d , PS2_seg:%d, SJW:%d\n",
		dbt->brp, 
		dbt->prop_seg, 
		dbt->phase_seg1, 
		dbt->phase_seg2,
		dbt->sjw);
	return 0;
}

/**
 * ftcan010_chip_start - This the drivers start routine
 * @ndev:	Pointer to net_device structure
 *
 * This is the drivers start routine.
 * Based on the State of the CAN device it puts
 * the CAN device into a proper mode.
 *
 * Return: 0 on success and failure value on error
 */
static int ftcan010_chip_start(struct net_device *ndev)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	u32 err, reg32 = 0;
	unsigned long timeout;

	/* Check if it is has been done soft reset before */
	err = set_reset_mode(ndev);
	if (err < 0)
		return err;

	/* Accept all CAN ID (Acceptance Filter Mask Setting) 
	 * Here temporarily use Rx fifo 0 to receive all frames from the CAN bus
	 * SocketCAN does not support hardware filters yet, all frames must be
	 * accepted.
	 */
	priv->write_reg(priv, FTCAN010_AFRCONTROL_OFFSET, 
		FTCAN010_AFRCONTROL_RBAFG0E_MASK);
	priv->write_reg(priv, FTCAN010_AMRCONTROL_OFFSET, 
		(FTCAN010_AMRCONTROL_FMA_MASK |
		 FTCAN010_AMRCONTROL_RBAFM0E_MASK));

	/* Enable all interrupts */
	reg32 = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);
	priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, (reg32 | FTCAN010_INTR_ALL));
	
	/* Retransmission once time */
	reg32 = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);
	priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, 
		(reg32 | (FTCAN010_RT_1_TIME << FTCAN010_CONTROL_RT_SHIFT)));

	/* Check whether it is loopback mode, listen mode or normal mode */
	reg32 = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);
	reg32 &= ~FTCAN010_CONTROL_OMR_MASK;
	if (priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK) {
		reg32 |= (FTCAN010_LOOPBACK_MODE << FTCAN010_CONTROL_OMR_SHIFT);
	} else if(priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY){
		reg32 |= (FTCAN010_LISTEN_MODE << FTCAN010_CONTROL_OMR_SHIFT);

		/* Mask Error Interrupt for Listen-only mode
		 * The error event on the bus should not affect the CAN node that in the
		 * listen-only mode.
		 */
		reg32 &= ~FTCAN010_CONTROL_EIE_MASK;
	} else {
		reg32 |= (FTCAN010_NORMAL_MODE << FTCAN010_CONTROL_OMR_SHIFT);
	}
	priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, reg32);

	timeout = jiffies + FTCAN010_TIMEOUT;
	while ((priv->read_reg(priv, FTCAN010_CONTROL_OFFSET) & 
		FTCAN010_CONTROL_OMR_MASK) != (reg32 & FTCAN010_CONTROL_OMR_MASK)) {
		if (time_after(jiffies, timeout)) {
			netdev_warn(ndev,
				"timed out for correct mode\n");
			return -ETIMEDOUT;
		}
	}
	netdev_dbg(ndev, "Control register (0x00)-0x%08x\n",
			priv->read_reg(priv, FTCAN010_CONTROL_OFFSET));

	priv->can.state = CAN_STATE_ERROR_ACTIVE;
	return 0;
}

/**
 * ftcan010_do_set_mode - This sets the mode of the driver
 * @ndev:	Pointer to net_device structure
 * @mode:	Tells the mode of the driver
 *
 * This check the drivers state and calls the
 * the corresponding modes to set.
 *
 * Return: 0 on success and failure value on error
 */
static int ftcan010_do_set_mode(struct net_device *ndev, enum can_mode mode)
{
	int ret;

	switch (mode) {
	case CAN_MODE_START:
		ret = ftcan010_chip_start(ndev);
		if (ret < 0) {
			netdev_err(ndev, "ftcan010_chip_start failed!\n");
			return ret;
		}
		netif_wake_queue(ndev);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}


/**
 * ftcan010_tx_interrupt - Tx Done Isr
 * @ndev:	net_device pointer
 * @irr:	Interrupt request register value
 */
static void ftcan010_tx_interrupt(struct net_device *ndev, u32 irr)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	struct net_device_stats *stats = &ndev->stats;
	u32 ctrlr;

	ctrlr = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);

	if (irr & FTCAN010_IRR_TBI0_MASK) {
		priv->write_reg(priv, FTCAN010_IRR_OFFSET, FTCAN010_IRR_TBI0_MASK);//w1c
		can_get_echo_skb(ndev, 0);
		stats->tx_packets++;
	} else if (irr & FTCAN010_IRR_TBI1_MASK) {
		priv->write_reg(priv, FTCAN010_IRR_OFFSET, FTCAN010_IRR_TBI1_MASK);//w1c
		can_get_echo_skb(ndev, 0);
		stats->tx_packets++;
	} else if (irr & FTCAN010_IRR_TBI2_MASK) {
		priv->write_reg(priv, FTCAN010_IRR_OFFSET, FTCAN010_IRR_TBI2_MASK);//w1c
		can_get_echo_skb(ndev, 0);
		stats->tx_packets++;
	}

	netif_wake_queue(ndev);
}

/**
 * ftcan010_err_interrupt - error frame Isr
 * @ndev:	net_device pointer
 * @isr:	interrupt status register value
 *
 * This is the CAN error interrupt and it will
 * check the the type of error and forward the error
 * frame to upper layers.
 */
static void ftcan010_err_interrupt(struct net_device *ndev, u32 irr)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	struct net_device_stats *stats = &ndev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	u32 err_status, can_status, err_counter, tec = 0, rec = 0, rxfifo;

	skb = alloc_can_err_skb(ndev, &cf);

	err_status = priv->read_reg(priv, FTCAN010_ERRSTATUS_OFFSET);
	can_status = priv->read_reg(priv, FTCAN010_STATUS_OFFSET);
	
	err_counter = priv->read_reg(priv, FTCAN010_ERRCNT_OFFSET);
	tec = ((err_counter & FTCAN010_ERRCNT_TEC_MASK) >> FTCAN010_ERRCNT_TEC_SHIFT);
	rec = ((err_counter & FTCAN010_ERRCNT_REC_MASK) >> FTCAN010_ERRCNT_REC_SHIFT);

	/* Check this CAN node's Error status with TEC and REC */
	if (can_status & FTCAN010_STATUS_BO_MASK) { //Bus-off
		priv->can.state = CAN_STATE_BUS_OFF;
		priv->can.can_stats.bus_off++;

		can_bus_off(ndev);

		if (skb)
			cf->can_id |= CAN_ERR_BUSOFF;
	} else if ((tec > 127) || (rec > 127)) { //Passive Error stauts
		priv->can.state = CAN_STATE_ERROR_PASSIVE;
		priv->can.can_stats.error_passive++;
		
		if (skb) {
			cf->can_id |= CAN_ERR_CRTL;
			cf->data[1] = (rec > 127) ? CAN_ERR_CRTL_RX_PASSIVE : 
				CAN_ERR_CRTL_TX_PASSIVE;
			cf->data[6] = tec;
			cf->data[7] = rec;
		}
	} else if (can_status & FTCAN010_STATUS_EW_MASK) { //Warning Error stauts
		priv->can.state = CAN_STATE_ERROR_WARNING;
		priv->can.can_stats.error_warning++;
		
		if (skb) {
			cf->can_id |= CAN_ERR_CRTL;
			cf->data[1] |= (tec > rec) ?
					CAN_ERR_CRTL_TX_WARNING :
					CAN_ERR_CRTL_RX_WARNING;
			cf->data[6] = tec;
			cf->data[7] = rec;
		}
	}

	/* Check for RX FIFO Overflow interrupt */
	if (irr & FTCAN010_IRR_OIR_MASK) {
		stats->rx_over_errors++;
		stats->rx_errors++;

		/* Handle the data overrun through releasing all buffers 
		 * in that overrun fifo. 
		 */
		rxfifo = ((priv->read_reg(priv, FTCAN010_STATUS_OFFSET) & 
			FTCAN010_STATUS_DO_MASK) >> FTCAN010_STATUS_DO_SHIFT);

		switch (rxfifo) {
		case FTCAN010_NOFIFO:
			break;
		case FTCAN010_RXFIFO0:
			priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, 
			FTCAN010_CONTROL_RRBA0_MASK);
			break;
		case FTCAN010_RXFIFO1:
			priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, 
			FTCAN010_CONTROL_RRBA1_MASK);
			break;
		case FTCAN010_BOTHFIFOS:
			priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, 
			(FTCAN010_CONTROL_RRBA0_MASK | FTCAN010_CONTROL_RRBA1_MASK));
			break;
		}
		
		priv->write_reg(priv, FTCAN010_IRR_OFFSET, FTCAN010_IRR_OIR_MASK); //w1c
		
		if (skb) {
			cf->can_id |= CAN_ERR_CRTL;
			cf->data[1] |= CAN_ERR_CRTL_RX_OVERFLOW;
		}
	}

	/* Check for error interrupt */
	if (irr & FTCAN010_IRR_EIR_MASK) {
		if (skb) {
			cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;
			cf->data[2] |= CAN_ERR_PROT_UNSPEC;
		}

		/* Check for Ack error interrupt */
		if (err_status & FTCAN010_ERRSTATUS_AE_MASK) {
			stats->tx_errors++;
			if (skb) {
				cf->can_id |= CAN_ERR_ACK;
				cf->data[3] |= CAN_ERR_PROT_LOC_ACK;
			}
		}

		/* Check for Bit error interrupt */
		if (err_status & FTCAN010_ERRSTATUS_BE_MASK) {
			stats->tx_errors++;
			if (skb) {
				cf->can_id |= CAN_ERR_PROT;
				cf->data[2] = CAN_ERR_PROT_BIT;
			}
		}

		/* Check for Stuff error interrupt */
		if (err_status & FTCAN010_ERRSTATUS_SE_MASK) {
			stats->rx_errors++;
			if (skb) {
				cf->can_id |= CAN_ERR_PROT;
				cf->data[2] = CAN_ERR_PROT_STUFF;
			}
		}

		/* Check for Form error interrupt */
		if (err_status & FTCAN010_ERRSTATUS_FE_MASK) {
			stats->rx_errors++;
			if (skb) {
				cf->can_id |= CAN_ERR_PROT;
				cf->data[2] = CAN_ERR_PROT_FORM;
			}
		}

		/* Check for CRC error interrupt */
		if (err_status & FTCAN010_ERRSTATUS_CE_MASK) {
			stats->rx_errors++;
			if (skb) {
				cf->can_id |= CAN_ERR_PROT;
				cf->data[3] = CAN_ERR_PROT_LOC_CRC_SEQ |
						CAN_ERR_PROT_LOC_CRC_DEL;
			}
		}

		priv->can.can_stats.bus_error++;
		priv->write_reg(priv, FTCAN010_IRR_OFFSET, FTCAN010_IRR_EIR_MASK); //w1c
	}

	if (skb) {
		stats->rx_packets++;
		stats->rx_bytes += cf->can_dlc;
		netif_rx(skb);
	}

	netdev_dbg(ndev, "%s: error status register:0x%x\n",
			__func__, priv->read_reg(priv, FTCAN010_ERRSTATUS_OFFSET));
	netdev_dbg(ndev, "%s: TEC:%ud REC:%ud\n",
			__func__, tec, rec);
}

/**
 * ftcan010_interrupt - CAN Isr
 * @irq:	irq number
 * @dev_id:	device id poniter
 *
 * This is the faraday CAN Isr. It checks for the type of interrupt
 * and invokes the corresponding ISR.
 *
 * Return:
 * IRQ_NONE - If CAN device is in sleep mode, IRQ_HANDLED otherwise
 */
static irqreturn_t ftcan010_interrupt(int irq, void *dev_id)
{
	struct net_device *ndev = (struct net_device *)dev_id;
	struct ftcan010_priv *priv = netdev_priv(ndev);
	u32 irr, ier;

	/* Get the interrupt status from IR register */
	irr = priv->read_reg(priv, FTCAN010_IRR_OFFSET);
	if (!irr)
		return IRQ_NONE;

	/* Check for the type of Wake-up interrupt and Processing it */
	if (irr & FTCAN010_IRR_WIR_MASK) {
		priv->can.state = CAN_STATE_ERROR_ACTIVE;
		priv->write_reg(priv, FTCAN010_IRR_OFFSET, FTCAN010_IRR_WIR_MASK); //write-1-clear this bit
	}

	/* Check for Tx interrupt and Processing it */
	if (irr & (FTCAN010_IRR_TBI0_MASK | FTCAN010_IRR_TBI1_MASK | FTCAN010_IRR_TBI2_MASK))
		ftcan010_tx_interrupt(ndev, irr);

	/* Check for the type of error interrupt and Processing it */
	if (irr & (FTCAN010_IRR_EIR_MASK | FTCAN010_IRR_OIR_MASK))
		ftcan010_err_interrupt(ndev, irr);

	/* Check for the type of receive interrupt and Processing it */
	if ((irr & FTCAN010_IRR_RBI0_MASK) || (irr & (FTCAN010_IRR_RBI1_MASK))) {
		/* Disable RIE */
		ier = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);
		ier &= ~FTCAN010_CONTROL_RIE_MASK;
		priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, ier); 

		priv->write_reg(priv, FTCAN010_IRR_OFFSET,
			(FTCAN010_IRR_RBI0_MASK | FTCAN010_IRR_RBI1_MASK));//w1c

		napi_schedule(&priv->napi);
	}

	return IRQ_HANDLED;
}

/**
 * ftcan010_start_xmit - Starts the transmission
 * @skb:	sk_buff pointer that contains data to be Txed
 * @ndev:	Pointer to net_device structure
 *
 * This function is invoked from upper layers to initiate transmission. This
 * function uses the next available free txbuff and populates their fields to
 * start the transmission.
 *
 * Return: 0 on success and failure value on error
 */
static int ftcan010_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	struct net_device_stats *stats = &ndev->stats;
	struct can_frame *cf;
	struct canfd_frame *cff;
	u32 can_ctrl_reg, tb_l = 0, tb_h = 0, data[2] = {0, 0}, 
		tb_l_base, tb_h_base, tb_data_base, btr_bit;
	int i;

	if (can_dropped_invalid_skb(ndev, skb))
		return NETDEV_TX_OK;

	can_ctrl_reg = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);

	/* Check if the TX buffer is full */
	if (unlikely(can_ctrl_reg & FTCAN010_CONTROL_BTRn_MASK) == FTCAN010_CONTROL_BTRn_MASK) {
		netif_stop_queue(ndev);
		netdev_err(ndev, "BUG!, TX FIFO full when queue awake!\n");
		return NETDEV_TX_BUSY;
	}

	/* Find the empty Tx buffer */
	if (!(can_ctrl_reg & FTCAN010_CONTROL_BTR0_MASK)) {
		btr_bit = FTCAN010_CONTROL_BTR0_MASK;
		tb_l_base = FTCAN010_TR0_OFFSET;
		tb_h_base = FTCAN010_TR0_OFFSET + 4;
		tb_data_base = FTCAN010_TX0DATA_OFFSET;
	} else if (!(can_ctrl_reg & FTCAN010_CONTROL_BTR1_MASK)) {
		btr_bit = FTCAN010_CONTROL_BTR1_MASK;
		tb_l_base = FTCAN010_TR1_OFFSET;
		tb_h_base = FTCAN010_TR1_OFFSET + 4;
		tb_data_base = FTCAN010_TX1DATA_OFFSET;
	} else {
		btr_bit = FTCAN010_CONTROL_BTR2_MASK;
		tb_l_base = FTCAN010_TR2_OFFSET;
		tb_h_base = FTCAN010_TR2_OFFSET + 4;
		tb_data_base = FTCAN010_TX2DATA_OFFSET;
	}

	if (skb->len == CANFD_MTU) { //4.14 has can_is_canfd_skb function
		/* FD frame part */
		cff = (struct canfd_frame *)skb->data;

		/* Change Socket ID format to FTCAN ID format */
		if (cff->can_id & CAN_EFF_FLAG) {
			/* Extended CAN ID format */
			tb_h |= FTCAN_EID2REGH(cff->can_id);
			tb_l |= FTCAN_EID2REGL(cff->can_id);

			tb_l |= FTCAN010_TR_EIE_MASK; 
		} else {
			/* Standard CAN ID format */
			tb_h |= FTCAN_SID2REGH(cff->can_id);
		}

		/* Set DLC */
		tb_h |= ((can_len2dlc(cff->len)) << FTCAN010_TR_DL_SHIFT);

		/* Set BRS bit if this frame would speed up in data phase */
		if (cff->flags & CANFD_BRS)
		tb_l |= FTCAN010_TR_BRS_MASK;

		/* Set EDL bit due to the FD frame need */
		tb_l |= FTCAN010_TR_EDL_MASK;

		/* Data */
		if (cff->len) {
			// netdev_dbg(ndev, "%s: Length:%d\n", __func__, cff->len);
			// netdev_dbg(ndev, "%s: Data[0]:%x\n", __func__, *(u32 *)cff->data);
			// netdev_dbg(ndev, "%s: Data[0]:%x\n", __func__, le32_to_cpup((u32 *)(cff->data)));

			for (i = 0; i < cff->len; i+=4) {
				priv->write_reg(priv, tb_data_base + i, 
					le32_to_cpup((__le32 *)(cff->data + i)));
			}
		}

		stats->tx_bytes += cff->len;

	} else {
		/* Classical frame part */
		cf = (struct can_frame *)skb->data;


		/* Change Socket ID format to FTCAN ID format */
		if (cf->can_id & CAN_EFF_FLAG) {
			/* Extended CAN ID format */
			tb_h |= FTCAN_EID2REGH(cf->can_id);
			tb_l |= FTCAN_EID2REGL(cf->can_id);

			tb_l |= FTCAN010_TR_EIE_MASK; 
		} else {
			/* Standard CAN ID format */
			tb_h |= FTCAN_SID2REGH(cf->can_id);
		}

		/* Set DLC */
		tb_h |= cf->can_dlc << FTCAN010_TR_DL_SHIFT;

		if (cf->can_id & CAN_RTR_FLAG) {
			/* Set RTR bit if this frame is a remote frame */
			tb_l |= FTCAN010_TR_RTR_MASK;
		} else {
			/* Set data pattern for this data frame */
			if (cf->can_dlc > 0) {
				data[0] = le32_to_cpup((__le32 *)(cf->data + 0));
				priv->write_reg(priv, tb_data_base, data[0]);
			}
			if (cf->can_dlc > 4) {
				data[1] = le32_to_cpup((__le32 *)(cf->data + 4));
				priv->write_reg(priv, tb_data_base + 4, data[1]);
			}

			stats->tx_bytes += cf->can_dlc;
		}
	}

	can_put_echo_skb(skb, ndev, 0);
	/* Write the Frame to FTCAN Tx buffer */
	priv->write_reg(priv, tb_l_base, tb_l);
	priv->write_reg(priv, tb_h_base, tb_h);
	
	/* Set this empty Tx buffer request (BTR) bit for transmission */
	can_ctrl_reg = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);
	can_ctrl_reg |= btr_bit;
	priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, can_ctrl_reg);

	return NETDEV_TX_OK;
}

/**
 * ftcan010_rx -  Is called from CAN isr to complete the received
 *		frame  processing in Rx FIFO 0
 * @ndev:	Pointer to net_device structure
 *
 * This function is invoked from the CAN isr(poll) to process the Rx frames. It
 * does minimal processing and invokes "netif_receive_skb" to complete further
 * processing.
 * Return: 1 on success and 0 on failure.
 */
static int ftcan010_rx(struct net_device *ndev, int fifo)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	struct net_device_stats *stats = &ndev->stats;
	struct can_frame *cf;
	struct canfd_frame *cff;
	struct sk_buff *skb;
	u32 rb_l, rb_h, fifo_base, fifo_data_base, control_reg,
		dlc, data[2] = {0, 0};
	int i;

	if (fifo == 0) {
		fifo_base = FTCAN010_RR0_OFFSET;
		fifo_data_base = FTCAN010_RX0DATA_OFFSET;
	}
	else {
		fifo_base = FTCAN010_RR1_OFFSET;
		fifo_data_base = FTCAN010_RX1DATA_OFFSET;
	}

	/* Read a frame from FTCAN010 Rx FIFO */
	rb_l = priv->read_reg(priv, fifo_base);
	rb_h = priv->read_reg(priv, fifo_base + 4);

	dlc = ((rb_h & FTCAN010_RR_DL_MASK) >> FTCAN010_RR_DL_SHIFT);

	if (rb_l & FTCAN010_RR_EDL_MASK) {
		/* FD CAN part */
		skb = alloc_canfd_skb(ndev, &cff);
		if (unlikely(!skb)) {
			stats->rx_dropped++;
			return 0;
		}

		cff->len = can_dlc2len(get_canfd_dlc(dlc));

		/* Change FTCAN ID format to socketCAN ID format */
		if (rb_l & FTCAN010_RR_EIE_MASK) {
			/* The received frame is an Extended format frame */
			cff->can_id |= FTCAN_GETEID(rb_h, rb_l);
			cff->can_id |= CAN_EFF_FLAG;
		} else {
			/* The received frame is a standard format frame */
			cff->can_id |= FTCAN_GETSID(rb_h);
		}

		if (rb_l & FTCAN010_RR_BRS_MASK)
			cff->flags |= CANFD_BRS;

		if (rb_l & FTCAN010_RR_ESI_MASK)
			cff->flags |= CANFD_ESI;

		/* Data */
		for (i = 0; i < cff->len; i += 4) {
			*(__le32 *)(cff->data + i) = cpu_to_le32( 
				priv->read_reg(priv, fifo_data_base + i));
		}

		stats->rx_bytes += cff->len;
		stats->rx_packets++;
		netif_receive_skb(skb);

	} else {
		/* Classical CAN part */
		skb = alloc_can_skb(ndev, &cf);
		if (unlikely(!skb)) {
			stats->rx_dropped++;
			return 0;
		}

		/* Change FTCAN data length format to socketCAN data format */
		cf->can_dlc = get_can_dlc(dlc);

		/* Change FTCAN ID format to socketCAN ID format */
		if (rb_l & FTCAN010_RR_EIE_MASK) {
			/* The received frame is an Extended format frame */
			cf->can_id |= FTCAN_GETEID(rb_h, rb_l);
			cf->can_id |= CAN_EFF_FLAG;
		} else {
			/* The received frame is a standard format frame */
			cf->can_id |= FTCAN_GETSID(rb_h);
		}

		if (rb_l & FTCAN010_RR_RTR_MASK) {
			/* Remote frame */
			cf->can_id |= CAN_RTR_FLAG;
		} else {
			/* Data frame */
			data[0] = priv->read_reg(priv, fifo_data_base);
			data[1] = priv->read_reg(priv, fifo_data_base + 4);

			/* Change FTCAN data format to socketCAN data format */
			if (cf->can_dlc > 0)
				*(__le32 *)(cf->data) = cpu_to_le32(data[0]);
			if (cf->can_dlc > 4)
				*(__le32 *)(cf->data + 4) = cpu_to_le32(data[1]);
		}

		stats->rx_bytes += cf->can_dlc;
		stats->rx_packets++;
		netif_receive_skb(skb);
	}
	
	/* Release one Rx FIFO buffer */
	control_reg = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);
	control_reg |= (fifo) ? FTCAN010_CONTROL_RRB1_MASK : 
		FTCAN010_CONTROL_RRB0_MASK;
	priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, control_reg);
	
	return 1;
}

/**
 * ftcan010_chip_stop - Driver stop routine
 * @ndev:	Pointer to net_device structure
 *
 * This is the drivers stop routine. It will disable the
 * interrupts and put the device into configuration mode.
 */
static void ftcan010_chip_stop(struct net_device *ndev)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	u32 control_reg;

	/* Disable interrupts and leave the can in configuration mode */
	control_reg = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);
	control_reg &= ~FTCAN010_INTR_ALL;
	control_reg &= ~FTCAN010_CONTROL_OMR_MASK;
	priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, control_reg);
	
	priv->can.state = CAN_STATE_STOPPED;
}

/**
 * ftcan010_rx_poll - Poll routine for rx packets (NAPI)
 * @napi:	napi structure pointer
 * @quota:	Max number of rx packets to be processed.
 *
 * This is the poll routine for rx part.
 * It will process the packets maximux quota value.
 *
 * Return: number of packets received
 */
static int ftcan010_rx_poll(struct napi_struct *napi, int quota)
{
	struct net_device *ndev = napi->dev;
	struct ftcan010_priv *priv = netdev_priv(ndev);
	u32 can_status, ier;
	int work_done = 0;

	can_status = priv->read_reg(priv, FTCAN010_STATUS_OFFSET);
	//irr = priv->read_reg(priv, FTCAN010_IRR_OFFSET);

	/* Check for Rx FIFO 0 */
	while ((can_status & FTCAN010_STATUS_BRS0_MASK) && (work_done < quota)) {
		work_done += ftcan010_rx(ndev, 0);
		can_status = priv->read_reg(priv, FTCAN010_STATUS_OFFSET);
	}

	/* Check for Rx FIFO 1 */
	while ((can_status & FTCAN010_STATUS_BRS1_MASK) && (work_done < quota)) {
		work_done += ftcan010_rx(ndev, 0);
		can_status = priv->read_reg(priv, FTCAN010_STATUS_OFFSET);
	}

	if (work_done < quota) {
		napi_complete(napi);

		/* Enable RIE */
		ier = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);
		ier |= FTCAN010_CONTROL_RIE_MASK;
		priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, ier); 
	}

	return work_done;
}


/**
 * ftcan010_close - Driver close routine
 * @ndev:	Pointer to net_device structure
 *
 * Return: 0 always
 */
static int ftcan010_close(struct net_device *ndev)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	u32 control_reg = 0;

	/* Back to the Configure mode (Clear OMR field to 3b'000) */
	control_reg = priv->read_reg(priv, FTCAN010_CONTROL_OFFSET);
	control_reg &= ~FTCAN010_CONTROL_OMR_MASK;
	priv->write_reg(priv, FTCAN010_CONTROL_OFFSET, control_reg); 

	netif_stop_queue(ndev);
	napi_disable(&priv->napi);
	ftcan010_chip_stop(ndev);
	clk_disable_unprepare(priv->can_clk);
	free_irq(ndev->irq, ndev);
	close_candev(ndev);

	return 0;
}

/**
 * ftcan010_open - Driver open routine
 * @ndev:	Pointer to net_device structure
 *
 * This is the driver open routine.
 * Return: 0 on success and failure value on error
 */
static int ftcan010_open(struct net_device *ndev)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	int ret;

	ret = request_irq(ndev->irq, ftcan010_interrupt, priv->irq_flags,
			ndev->name, ndev);
	if (ret < 0) {
		netdev_err(ndev, "irq allocation for CAN failed\n");
		goto err_irq;
	}

	ret = clk_prepare_enable(priv->can_clk);
	if (ret) {
		netdev_err(ndev, "unable to enable device clock\n");
		goto err_can_clk;
	}

	/* Set chip into reset mode */
	ret = set_reset_mode(ndev);
	if (ret < 0) {
		netdev_err(ndev, "mode resetting failed!\n");
		goto err;
	}

	/* Common open */
	ret = open_candev(ndev);
	if (ret)
		goto err_candev;

	ret = ftcan010_chip_start(ndev);
	if (ret < 0) {
		netdev_err(ndev, "ftcan010_chip_start failed!\n");
		goto err;
	}

	napi_enable(&priv->napi);
	netif_start_queue(ndev);

	return 0;

err_candev:
	close_candev(ndev);
err_can_clk:
	clk_disable_unprepare(priv->can_clk);
err_irq:
	free_irq(ndev->irq, ndev);
err:
	return ret;
}

static const struct net_device_ops ftcan010_netdev_ops = {
	.ndo_open	= ftcan010_open,
	.ndo_stop	= ftcan010_close,
	.ndo_start_xmit	= ftcan010_start_xmit,
};

/**
 * ftcan010_get_berr_counter - error counter routine
 * @ndev:	Pointer to net_device structure
 * @bec:	Pointer to can_berr_counter structure
 *
 * This is the driver error counter routine.
 * Return: 0 on success and failure value on error
 */
static int ftcan010_get_berr_counter(const struct net_device *ndev,
					struct can_berr_counter *bec)
{
	struct ftcan010_priv *priv = netdev_priv(ndev);
	u32 ect;
	int ret;

	ret = clk_prepare_enable(priv->can_clk);
	if (ret)
		goto err;

	ect = priv->read_reg(priv, FTCAN010_ERRCNT_OFFSET);

	bec->txerr = ((ect & FTCAN010_ERRCNT_TEC_MASK) >> FTCAN010_ERRCNT_TEC_SHIFT);
	bec->rxerr = ((ect & FTCAN010_ERRCNT_REC_MASK) >> FTCAN010_ERRCNT_REC_SHIFT);

	clk_disable_unprepare(priv->can_clk);

	return 0;

err:
	clk_disable_unprepare(priv->can_clk);
	return ret;
}

static int ftcan010_suspend(struct device *dev)
{
  return 0;
}

static int ftcan010_resume(struct device *dev)
{
  return 0;
}

static SIMPLE_DEV_PM_OPS(ftcan010_dev_pm_ops, ftcan010_suspend, ftcan010_resume);

/**
 * ftcan010_probe - Platform registration call
 * @pdev:	Handle to the platform device structure
 *
 * This function does all the memory allocation and registration for the CAN
 * device.
 *
 * Return: 0 on success and failure value on error
 */
static int ftcan010_probe(struct platform_device *pdev)
{
	struct resource *res; /* IO mem resources */
	struct net_device *ndev;
	struct ftcan010_priv *priv;
	struct clk *can_clk = NULL;
	void __iomem *addr;
	int ret;
	u32 clock_freq = 0;

	/* Get the virtual base address for the device */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(addr)) {
		ret = PTR_ERR(addr);
		goto err;
	}

	/* Create a CAN device instance */
	ndev = alloc_candev(sizeof(struct ftcan010_priv), 1);
	if (!ndev)
		return -ENOMEM;

	priv = netdev_priv(ndev);
	priv->dev = ndev;
	priv->can.bittiming_const = &ftcan010_bittiming_const; //nominal phase bit rate
	priv->can.data_bittiming_const = &ftcan010_data_bittiming_const; //data phase bit rate
	priv->can.do_set_mode = ftcan010_do_set_mode;
	priv->can.do_get_berr_counter = ftcan010_get_berr_counter;
	priv->can.do_set_bittiming = ftcan010_set_bittiming;
	priv->can.do_set_data_bittiming = ftcan010_set_data_bittiming;
	priv->can.ctrlmode_supported = CAN_CTRLMODE_LOOPBACK |
					CAN_CTRLMODE_LISTENONLY |
					CAN_CTRLMODE_FD; ///include/uapi/linux/can/netlink.h
	priv->reg_base = addr;

	/* Get IRQ for the device */
	ndev->irq = platform_get_irq(pdev, 0);
	
	ndev->flags |= IFF_ECHO;

	platform_set_drvdata(pdev, ndev);
	SET_NETDEV_DEV(ndev, &pdev->dev);
	ndev->netdev_ops = &ftcan010_netdev_ops;

	if (pdev->dev.of_node)
		of_property_read_u32(pdev->dev.of_node, "clock-frequency", &clock_freq);

	if (!clock_freq) {
		can_clk = devm_clk_get(&pdev->dev, "pclk");
		if (IS_ERR(can_clk)) {
			dev_err(&pdev->dev, "clk or clock-frequency not defined\n");
			ret = PTR_ERR(can_clk);
			goto err_free;
		}
		clock_freq = clk_get_rate(can_clk);
	}

	priv->write_reg = ftcan010_write_reg_le;
	priv->read_reg = ftcan010_read_reg_le;

	priv->can.clock.freq = clock_freq;
	priv->can_clk = can_clk;

	netif_napi_add(ndev, &priv->napi, ftcan010_rx_poll, FTCAN010_RX_FIFO_DEPTH);

	ret = register_candev(ndev);
	if (ret) {
		dev_err(&pdev->dev, "fail to register failed (err=%d)\n", ret);
		goto err_unprepare_disable_dev;
	}

	clk_disable_unprepare(priv->can_clk);
	netdev_dbg(ndev, "reg_base=0x%p irq=%d clock=%d, tx fifo depth:%d\n",
			priv->reg_base, ndev->irq, priv->can.clock.freq,
			1);

	return 0;

err_unprepare_disable_dev:
	clk_disable_unprepare(priv->can_clk);
err_free:
	free_candev(ndev);
err:
	return ret;
}

/**
 * ftcan010_remove - Unregister the device after releasing the resources
 * @pdev:	Handle to the platform device structure
 *
 * This function frees all the resources allocated to the device.
 * Return: 0 always
 */
static int ftcan010_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct ftcan010_priv *priv = netdev_priv(ndev);

	unregister_candev(ndev);
	netif_napi_del(&priv->napi);
	free_candev(ndev);

	return 0;
}

/* Match table for OF platform binding */
static const struct of_device_id ftcan010_of_match[] = {
	{ .compatible = "faraday,ftcan010", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, ftcan010_of_match);

static struct platform_driver ftcan010_driver = {
	.probe = ftcan010_probe,
	.remove = ftcan010_remove,
	.driver = {
		.name = "FTCAN010",
		.pm = &ftcan010_dev_pm_ops,
		.of_match_table = ftcan010_of_match,
	},
};
module_platform_driver(ftcan010_driver);
