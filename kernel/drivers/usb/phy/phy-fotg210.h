/*
 * FOTG210 USB otg driver.
 *
 * Copyright (C) 2016-2019 Faraday Technology Corp.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef	__FOTG210_USB_CONTROLLER__
#define	__FOTG210_USB_CONTROLLER__

/* Host controller register */
#define FOTG210_USBCMD		0x10
#define USBCMD_ASCH_EN		(1 << 5)
#define USBCMD_PSCH_EN		(1 << 4)
#define USBCMD_HC_RESET		(1 << 1)  /* reset HC not bus */
#define USBCMD_RUN		(1 << 0)

#define FOTG210_USBSTS		0x14
#define USBSTS_HALT		(1 << 12)

#define FOTG210_USBINTR		0x18
#define FOTG210_PORTSC		0x30

#define USBSTS_ASCH		(1 << 15)
#define USBSTS_PSCH		(1 << 14)

#define PORTSC_PO_RESET		(1 << 8)
#define PORTSC_PO_SUSP		(1 << 7)
#define PORTSC_PO_EN		(1 << 2)
#define PORTSC_CONN_STS		(1 << 0)

/* Global register */
/* Global Mask of HC/OTG/DEV interrupt Register(0xC4) */
#define FOTG210_GMIR		0xC4
#define GMIR_INT_POLARITY	(1 << 3) /* Active High */
#define GMIR_MHC_INT		(1 << 2)
#define GMIR_MOTG_INT		(1 << 1)
#define GMIR_MDEV_INT		(1 << 0)

/* Device register */
/*  Device Main Control Register(0x100) */
#define FOTG210_DEVMCR		0x100
#define DEVMCR_CHIP_EN		(1 << 5)
#define DEVMCR_SFRST		(1 << 4)
#define DEVMCR_GLINT_EN		(1 << 2)

/* PHY Test Mode Selector register(0x114) */
#define FOTG210_PHYTMSR		0x114
#define PHYTMSR_UNPLUG		(1 << 0)

/* Device Mask of Interrupt Group Register (0x130) */
#define FOTG210_DEVMIGR		0x130
#define DEVMIGR_MINT_G0		(1 << 0)
#define DEVMIGR_MINT_G1		(1 << 1)
#define DEVMIGR_MINT_G2		(1 << 2)
#define DEVMIGR_MINT_G3		(1 << 3)

/* Device Mask of Interrupt Source Group 0(0x134) */
#define FOTG210_DEVMISGR0		0x134
#define DEVMISGR0_MCX_COMEND		(1 << 3)

/* Device Mask of Interrupt Source Group 1 Register(0x138)*/
#define FOTG210_DEVMISGR1		0x138

/* Device Mask of Interrupt Source Group 2 Register (0x13C) */
#define FOTG210_DEVMISGR2		0x13C
#define DEVMISGR2_MDEV_WAKEUP_VBUS	(1 << 10)
#define DEVMISGR2_MDEV_IDLE		(1 << 9)


/* OTG register */
#define FOTG210_OTGCSR		0x80	/* OTG Control Status Register */
#define FOTG210_OTGISR		0x84	/* OTG Interrupt Status Register */
#define FOTG210_OTGIER		0x88	/* OTG Interrupt Enable Register */

/* OTG Control Status Register Bit (0x080) */
#define OTGCSR_B_BUS_REQ		(1 << 0)
#define OTGCSR_B_HNP_EN			(1 << 1)
#define OTGCSR_B_DSCHRG_VBUS		(1 << 2)
#define OTGCSR_A_BUS_REQ		(1 << 4)
#define OTGCSR_A_BUS_DROP		(1 << 5)
#define OTGCSR_A_SET_B_HNP_EN		(1 << 6)
#define OTGCSR_A_SRP_DET_EN		(1 << 7)
#define OTGCSR_A_SRP_RESP_TYPE		(1 << 8)
#define OTGCSR_ID_FLT_SEL		(1 << 9)
#define OTGCSR_VBUS_FLT_SEL		(1 << 10)
#define OTGCSR_B_SESS_END		(1 << 16)
#define OTGCSR_B_SESS_VLD		(1 << 17)
#define OTGCSR_A_SESS_VLD		(1 << 18)
#define OTGCSR_VBUS_VLD			(1 << 19)
#define OTGCSR_CROLE			(1 << 20)
#define OTGCSR_ID			(1 << 21)
#define OTGCSR_HOST_SPD_TYP		(0x3 << 22)

/* OTG Interrupt Status/Enable Register Bit */
#define INT_B_SRP_DN		(1 << 0)
#define INT_A_SRP_DET		(1 << 4)
#define INT_A_VBUS_ERR		(1 << 5)
#define INT_B_SESS_END		(1 << 6)
#define INT_ROLECHG		(1 << 8)
#define INT_IDCHG		(1 << 9)
#define INT_OVC			(1 << 10)
#define INT_B_PLGRMV		(1 << 11)
#define INT_A_PLGRMV		(1 << 12)

#define ID_A_TYPE	0
#define ID_B_TYPE	1 //bit21

#define CROLE_Host		0
#define CROLE_Peripheral	1 //bit20


struct fotg210_otg {
	struct usb_phy phy;

	/* base address */
	void __iomem *regs;

	struct platform_device *pdev;
	int irq;
	u32 irq_en;

	struct delayed_work work;
	struct workqueue_struct *qwork;

	spinlock_t wq_lock;
};

#endif

