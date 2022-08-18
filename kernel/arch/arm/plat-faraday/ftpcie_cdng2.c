/*
 * Driver for the Cadence PCI-E Gen2 root complex controller
 *
 *  (C) Copyright 2017 Faraday Technology
 *  Mark Fu-Tsung <mark_hs@faraday-tech.com>
 *  Bo-Cun Chen <bcchen@faraday-tech.com>
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
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <asm/sizes.h>
#include <asm/mach/pci.h> 
#include <mach/hardware.h>
#ifdef CONFIG_ARM_ACP
#include <mach/memory.h>
#endif
#include <plat/ftpcie_cdng2.h>

#define REGION0 0
#define REGION1 1
#define REGION2 2

struct pcie_port {
	u8              index;
	u8              root_bus_nr;
	u32             axislave_base_pa;
	void __iomem    *axislave_base;
	void __iomem    *pcie_glue_base;
	void __iomem    *pcie_reg_base;
	void __iomem    *pcie_reg_lm_base;
	void __iomem    *pcie_reg_at_base;
	spinlock_t      conf_lock;
};

static struct pcie_port pcie_port[2];
static int num_pcie_ports;

int ftpcie_cdng2_linkup(phys_addr_t pcie_reg_base_pa);

static void __init add_pcie_port(int index, phys_addr_t pcie_glue_base_pa,
                                 phys_addr_t pcie_reg_base_pa, phys_addr_t axislave_base_pa)
{
	struct pcie_port *pp;

	printk(KERN_INFO "Add PCIe port %d\n", index);

	if (ftpcie_cdng2_linkup(pcie_glue_base_pa)) {
		pp = &pcie_port[num_pcie_ports++];

		pp->index = index;
		pp->root_bus_nr = -1;

		pp->axislave_base_pa = axislave_base_pa;
		pp->axislave_base = ioremap(axislave_base_pa, SZ_32M);
		printk("!!!!!!!!!!!axislave_base=0x%p\n", pp->axislave_base);

		pp->pcie_glue_base = ioremap(pcie_glue_base_pa, SZ_4K);
		printk("!!!!!!!!!!!pcie_glue_base=0x%p\n", pp->pcie_glue_base);

		pp->pcie_reg_base = ioremap(pcie_reg_base_pa, SZ_4K);
		printk("!!!!!!!!!!!pcie_reg_base=0x%p\n", pp->pcie_reg_base);

		pp->pcie_reg_lm_base = ioremap(pcie_reg_base_pa + LOCAL_MGMT_OFFSET, SZ_4K);
		printk("!!!!!!!!!!!pcie_reg_lm_base=0x%p\n", pp->pcie_reg_lm_base);

		pp->pcie_reg_at_base = ioremap(pcie_reg_base_pa + ADDR_TRANS_OFFSET, SZ_4K);
		printk("!!!!!!!!!!!pcie_reg_at_base=0x%p\n", pp->pcie_reg_at_base);

		spin_lock_init(&pp->conf_lock);
	} else {
		printk(KERN_INFO "!!!!!!!!!!!Link L0 failed, ignoring\n");
	}
}

static struct pcie_port* find_pcie_port_by_id(unsigned char id)
{
	struct pcie_port *pp;
	int i;

	for (i=0; i<num_pcie_ports; i++)
	{
		pp = &pcie_port[i];
		if (pp->index == id)
			return pp;
	}

	return NULL;
}

static struct pcie_port* find_pcie_port_by_bus(unsigned char bus_number)
{
	struct pcie_port *pp;
	int i;

	for (i=0; i<num_pcie_ports; i++)
	{
		pp = &pcie_port[i];
		if (pp->root_bus_nr == bus_number)
			return pp;
	}

	return NULL;
}

void ConfigAddrTrans(struct pcie_port *pp, unsigned int no_of_valid_bits , unsigned int trans_type , void __iomem *pcie_addr_trans_base,int region)
{
	if (region == 0) {
		writel((pp->axislave_base_pa)|(no_of_valid_bits-1),
		       pcie_addr_trans_base + REGION0_ADDR0);
		writel(0                    , pcie_addr_trans_base + REGION0_ADDR1);
		writel((1<<23)|trans_type   , pcie_addr_trans_base + REGION0_DESC0);
		writel(0                    , pcie_addr_trans_base + REGION0_DESC1);
		writel(0                    , pcie_addr_trans_base + REGION0_DESC2);
	} else if (region == 1) {
		writel((pp->axislave_base_pa+0x01000000)|(no_of_valid_bits-1),
		       pcie_addr_trans_base + REGION1_ADDR0);
		writel(0                    , pcie_addr_trans_base + REGION1_ADDR1);
		writel((1<<23)|trans_type   , pcie_addr_trans_base + REGION1_DESC0);
		writel(0                    , pcie_addr_trans_base + REGION1_DESC1);
		writel(0                    , pcie_addr_trans_base + REGION1_DESC2);

		writel((pp->axislave_base_pa+0x02000000)|(no_of_valid_bits-1),
		       pcie_addr_trans_base + REGION2_ADDR0);
		writel(0                    , pcie_addr_trans_base + REGION2_ADDR1);
		writel((1<<23)|trans_type   , pcie_addr_trans_base + REGION2_DESC0);
		writel(0                    , pcie_addr_trans_base + REGION2_DESC1);
		writel(0                    , pcie_addr_trans_base + REGION2_DESC2);

		writel((pp->axislave_base_pa+0x03000000)|(no_of_valid_bits-1),
		       pcie_addr_trans_base + REGION3_ADDR0);
		writel(0                    , pcie_addr_trans_base + REGION3_ADDR1);
		writel((1<<23)|trans_type   , pcie_addr_trans_base + REGION3_DESC0);
		writel(0                    , pcie_addr_trans_base + REGION3_DESC1);
		writel(0                    , pcie_addr_trans_base + REGION3_DESC2);
	}
}

void ConfigAddrTrans_Config(int id)	//need to modify
{
	struct pcie_port *pp;
	unsigned int no_of_valid_bits , trans_type;

	pp = find_pcie_port_by_id(id);

	no_of_valid_bits = 20;	//	28;
	trans_type = 0xa;
	ConfigAddrTrans(pp, no_of_valid_bits, trans_type, pp->pcie_reg_at_base, REGION0);
}

void ConfigAddrTrans_Mem(int id)
{
	struct pcie_port *pp;
	unsigned int no_of_valid_bits , trans_type;

	pp = find_pcie_port_by_id(id);

	no_of_valid_bits = 26;	//	20;
	trans_type = 0x2;
	ConfigAddrTrans(pp, no_of_valid_bits, trans_type, pp->pcie_reg_at_base, REGION1);
}

static int ftpcie_cdng2_read_config(struct pci_bus * bus, unsigned int devfn, int offset, int size, u32 *val)
{
	struct pcie_port *pp;
	unsigned long flags;
	unsigned int status = PCIBIOS_SUCCESSFUL;
	unsigned int request_id, tmp;

	if (devfn)
		return PCIBIOS_FUNC_NOT_SUPPORTED;

	pp = find_pcie_port_by_bus(bus->number);
	if (pp == NULL)
	{
		printk(KERN_INFO "PCIE port is not exist\n");
		return PCIBIOS_FUNC_NOT_SUPPORTED;
	}

	spin_lock_irqsave(&pp->conf_lock, flags);

	ConfigAddrTrans_Config(pp->index);

	tmp = (bus->number)<<20 | PCI_SLOT(devfn)<<15 | PCI_FUNC(devfn)<<12;

	request_id = (unsigned int)pp->axislave_base | tmp;

	switch (size)
	{
		case 1: 
			*val = readb((void __iomem*)request_id + offset); 
			//printk("1.offset = %08X,\tval = %08X\n",offset ,*val);
			break;
		case 2: 
			*val = readw((void __iomem*)request_id + offset);
			//printk("2.offset = %08X,\tval = %08X\n",offset ,*val);;
			break;
		case 4: 
			*val = readl((void __iomem*)request_id + offset); 
			//printk("4.offset = %08X,\tval = %08X\n",offset ,*val);
			break;
		default: 
			*val = 0;
			status = PCIBIOS_SET_FAILED;
			goto rd_config_out;
	}

rd_config_out:
	spin_unlock_irqrestore(&pp->conf_lock, flags);
	ConfigAddrTrans_Mem(pp->index);
//	printk("rdconfig done\n");
	return status;
}

static int ftpcie_cdng2_write_config(struct pci_bus * bus, unsigned int devfn, int offset, int size, u32 data)
{
	struct pcie_port *pp;
	unsigned long flags;
	unsigned int status = PCIBIOS_SUCCESSFUL;
	unsigned int request_id, tmp;

	if (devfn)
		return PCIBIOS_FUNC_NOT_SUPPORTED;

	pp = find_pcie_port_by_bus(bus->number);
	if (pp == NULL)
	{
		printk(KERN_INFO "PCIE port is not exist\n");
		return PCIBIOS_FUNC_NOT_SUPPORTED;
	}

	spin_lock_irqsave(&pp->conf_lock, flags);

	ConfigAddrTrans_Config(pp->index);

	tmp = (bus->number)<<20 | PCI_SLOT(devfn)<<15 | PCI_FUNC(devfn)<<12;

	request_id = (unsigned int)pp->axislave_base | tmp;
//	printk("W");
	switch (size)
	{
		case 1: 
			writeb((unsigned char)data, (void __iomem*)request_id + offset); 
			break;
		case 2: 
			writew((unsigned long)data, (void __iomem*)request_id + offset); 
			break;
		case 4: 
			writel(data, (void __iomem*)request_id + offset); 
			break;
		default: 
			status = PCIBIOS_SET_FAILED;
			goto wr_config_out;
	}

wr_config_out:
	spin_unlock_irqrestore(&pp->conf_lock, flags);
	ConfigAddrTrans_Mem(pp->index);
//	printk("wconfig done\n");
	return status;
}

static struct pci_ops ftpcie_cdng2_ops = 
{
	.read  = ftpcie_cdng2_read_config,
	.write = ftpcie_cdng2_write_config,
};

int FindPciCapPointer(struct pcie_port *pp)
{
	int offset, val;

	offset = readb(pp->axislave_base + PCI_CAPABILITY_LIST) & ~3;

	while(1)
	{
		val = readb(pp->axislave_base + offset);
		if (val == PCI_CAP_ID_EXP)
		{
			break;
		}
		else
		{
			offset += PCI_CAP_LIST_NEXT;
			val = readb(pp->axislave_base + offset);
			offset = val;
		}
	}

	return offset;
}

void ChagMaxRdReqSize(struct pcie_port *pp)
{
	volatile u32 data;
	u32 cap_offset;

	printk(KERN_INFO "Faraday PCI driver ChagMaxRdReqSize\n");

	cap_offset = FindPciCapPointer(pp) + PCI_EXP_DEVCTL;

	data = readw(pp->axislave_base + cap_offset);
	data &= ~(0x7000);
	writew(data, pp->axislave_base + cap_offset);
}

void ftpcie_cdng2_preinit_controller(int id)
{
	struct pcie_port *pp;
	unsigned int tmp;

	pp = find_pcie_port_by_id(id);
	if (pp == NULL)
		return;

	printk(KERN_INFO "Faraday PCI driver PreInit Controller %d\n", id);

	//Set BAR SIZE, Width, and cacheable
	writel(RC_BAR_CHECK_EN|RC_BAR0_MEM_NONPREFETCH|RC_BAR0_SIZE_1G,
	       pp->pcie_reg_lm_base + RC_BAR_CFG_OFFSET);

#ifndef CONFIG_ARM_ACP
	writel(PHYS_OFFSET, pp->pcie_reg_base + RC_BAR0_OFFSET);
	writel(0x00000000, pp->pcie_reg_base + RC_BAR1_OFFSET);
	writel(PHYS_OFFSET|0x1d, pp->pcie_reg_at_base + RP_IB_BAR0_ADDR0_OFFSET);
#else
	writel(ARM_ACPBUS_OFFSET, pp->pcie_reg_base + RC_BAR0_OFFSET);
	writel(0x00000000, pp->pcie_reg_base + RC_BAR1_OFFSET);
	writel(ARM_ACPBUS_OFFSET|0x1d, pp->pcie_reg_at_base + RP_IB_BAR0_ADDR0_OFFSET);

	//Disable Ordering Check
	tmp = readl(pp->pcie_reg_lm_base + DBG_MUX_CTLR_OFFSET);
	tmp |= (0x1 << 30);
	writel(tmp, pp->pcie_reg_lm_base + DBG_MUX_CTLR_OFFSET);
#endif

#if 0
	writel(0x0, pp->pcie_reg_base + CAP_OFFSET);    //Max payload
	tmp = readl(pp->pcie_reg_base + DEV_CTRL_OFFSET);
	tmp &= ~0x7000;
	writel(tmp, pp->pcie_reg_base + DEV_CTRL_OFFSET);
	ChagMaxRdReqSize(id);
#endif

	// Disable Completion Timeout
	tmp = readl(pp->pcie_reg_base + DEV_CTRL2_OFFSET);
	tmp |= (0x1 << 4);
	writel(tmp, pp->pcie_reg_base + DEV_CTRL2_OFFSET);

	// Mask fatal/non-fatal error interrupt
	tmp = readl(pp->pcie_glue_base + PCIE_INT_MASK_OFFSET);
	tmp |= 0x700;
	writel(tmp, pp->pcie_glue_base + PCIE_INT_MASK_OFFSET);

	// Enable IO/Mem access, Bus-master, Parity error response and Fatal/Non-fatal error
	writel(0x147, pp->pcie_reg_base + CMD_STS_OFFSET);

	ConfigAddrTrans_Mem(pp->index);
}

irqreturn_t ftpcie_cdng2_interrupt(int irq, void *dev_instance)
{
	volatile u32 data;
	unsigned long flags;
	struct pcie_port *pp;
	struct pci_dev *dev = dev_instance;

	//printk(KERN_INFO "Faraday PCI driver Interrupt\n");
	pp = find_pcie_port_by_bus(dev->bus->number);
	if (pp == NULL)
	{
		printk(KERN_INFO "PCIE port is not exist\n");
		return IRQ_NONE;
	}

	data = readl(pp->pcie_glue_base + PCIE_INT_STATUS_OFFSET);   //interrupt status
	if (data == 0) {
		return IRQ_NONE;
	}

	spin_lock_irqsave(&pp->conf_lock, flags);

	if ((data&0x3f) == 0x20)
	{
		printk(KERN_INFO "Wait Link to L0......\n");

		// Waiting for Linking to L0 State
		while(1) 
		{
			data = readl(pp->pcie_glue_base + PCIE_LINK_STATUS_OFFSET);
			if((data & 0x1f0000) == 0x100000)   // Ltssm state
				break;
		}
		printk(KERN_INFO "Link to L0\n");
	}

	writel(data, pp->pcie_glue_base + PCIE_INT_STATUS_OFFSET);

	spin_unlock_irqrestore(&pp->conf_lock, flags);

	return IRQ_HANDLED;
}

int ftpcie_cdng2_linkup(phys_addr_t pcie_glue_base_pa)
{
	void __iomem *pcie_glue_base;
	volatile u32 tmp;
	int ret = 0;
	unsigned long expire;

	printk(KERN_INFO "Faraday PCI driver Check Link to L0\n");

	pcie_glue_base = ioremap(pcie_glue_base_pa, SZ_4K);

	// Link Training Enable
	writel(0x03, pcie_glue_base + PCIE_CTRL3_OFFSET);

	expire = msecs_to_jiffies(1000) + jiffies;
	// Waiting for Linking to L0 State
	do {
		tmp = readl(pcie_glue_base + PCIE_LINK_STATUS_OFFSET);
		if ((tmp & 0x1f0000) == 0x100000 ) {  // Ltssm state
			ret = 1;
			break;
		}
	} while (time_before(jiffies, expire));

	iounmap(pcie_glue_base);
	return ret;
}

static struct resource pci_0_mem = 
{
	.name 	=	"Faraday PCI non-prefetchable Memory Space",
	.start	= 	PLAT_FTPCIE_REGION1_BASE,
	.end 	=	PLAT_FTPCIE_REGION1_BASE + PCIE_CDNG2_MEM_PA_LENGTH-1,
	.flags	=	IORESOURCE_MEM,
};

static struct resource pci_1_mem = 
{
	.name 	=	"Faraday PCI non-prefetchable Memory Space",
	.start	= 	PLAT_FTPCIE1_REGION1_BASE,
	.end 	=	PLAT_FTPCIE1_REGION1_BASE + PCIE_CDNG2_MEM_PA_LENGTH-1,
	.flags	=	IORESOURCE_MEM,
};

int __init ftpcie_cdng2_setup(int nr, struct pci_sys_data *sys)
{
	struct pcie_port *pp;

	pp = find_pcie_port_by_id(nr);
	if (pp == NULL)
		return 0;

	printk(KERN_INFO "**********Faraday PCI driver Setup\n");

	pp->root_bus_nr = sys->busnr;

	if(pp->index==0)
	{
		if(request_resource(&ioport_resource, &pci_0_mem)) 
		{
			printk(KERN_ERR " PCI: unable to allocate io region\n");
			return -EBUSY;  
		}
		pci_add_resource(&sys->resources, &pci_0_mem);
	}else
	{
		if(request_resource(&ioport_resource, &pci_1_mem)) 
		{
			printk(KERN_ERR " PCI: unable to allocate io region\n");
			return -EBUSY;  
		}
		pci_add_resource(&sys->resources, &pci_1_mem);
	}

	return 1;
}

struct pci_bus * __init ftpcie_cdng2_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pcie_port *pp;

	pp = find_pcie_port_by_id(nr);
	if (pp == NULL)
		return NULL;

	printk(KERN_INFO "**********Faraday PCI driver ScanBus\n");

	return pci_scan_root_bus(NULL, sys->busnr, &ftpcie_cdng2_ops, sys,&sys->resources);
}

void __init ftpcie_cdng2_preinit(void /**sysdata*/) 
{
	printk(KERN_INFO "**********Faraday PCI driver PreInit\n");

#if defined(CONFIG_FTPCIE_CDNG2_0)
	ftpcie_cdng2_preinit_controller(0);
#endif

#if defined(CONFIG_FTPCIE_CDNG2_1)
	ftpcie_cdng2_preinit_controller(1);
#endif
}

int __init ftpcie_cdng2_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	int rc;
	struct pcie_port *pp;

	printk(KERN_INFO "**********Faraday PCI driver map_irq\n");
	printk(KERN_INFO "ftpcie_cdngen2_map_irq: slot=%d pin=%d\n", PCI_SLOT(dev->devfn), pin); 

	pp = find_pcie_port_by_bus(dev->bus->number);
	if (pp == NULL)
	{
		printk(KERN_INFO "PCIE port is not exist\n");
		goto map_irq_out;
	}

	if (pp->index == 0)
		rc = request_irq(PLAT_FTPCIE_IRQ, ftpcie_cdng2_interrupt, IRQF_SHARED, "ftpcie_cdng2", (void *)dev);
	else
		rc = request_irq(PLAT_FTPCIE_1_IRQ, ftpcie_cdng2_interrupt, IRQF_SHARED, "ftpcie_cdng2", (void *)dev);

	if (rc) 
	{
		printk(KERN_INFO "some issue in request_irq - ftpcie_cdng2_interrupt, rc=%d\n", rc);
	}

	switch(PCI_SLOT(dev->devfn))
	{
		case 0:
		case 1:
		case 2:
		case 3:
			if(pp->index == 0)
				return PLAT_FTPCIE_INTA_IRQ;
			else
				return PLAT_FTPCIE_INTA_1_IRQ;
		default:
			printk(KERN_ERR "Not Support Slot %d\n", slot);
			break;
	}

map_irq_out:
	return -1;
}

static struct hw_pci ftpcie_cdng2  __initdata = 
{
	.nr_controllers = 2,
	.setup          = ftpcie_cdng2_setup,  
	.scan           = ftpcie_cdng2_scan_bus,
	.preinit        = ftpcie_cdng2_preinit,	/* The first called init function */
	.map_irq        = ftpcie_cdng2_map_irq,
};

static int __init ftpcie_cdng2_init(void)
{
	printk(KERN_INFO "**********Init Cadence PCI Express Root Complex\n");
	/* Register I/O address range of this PCI Bridge Controller  */

	num_pcie_ports = 0; //init static variable

#if defined(CONFIG_FTPCIE_CDNG2_0)
	add_pcie_port(0, PLAT_FTPCIE_WRAP_PA_BASE,
	              PLAT_FTPCIE_REG_PA_BASE, PLAT_FTPCIE_AXISLV_PA_BASE);
#endif
#if defined(CONFIG_FTPCIE_CDNG2_1)
	add_pcie_port(1, PLAT_FTPCIE_WRAP_1_PA_BASE,
	              PLAT_FTPCIE_REG_1_PA_BASE, PLAT_FTPCIE_AXISLV_1_PA_BASE);
#endif

	pci_common_init(&ftpcie_cdng2);

	return 0;
}

subsys_initcall(ftpcie_cdng2_init);

MODULE_AUTHOR("Mark Fu-Tsung <mark_hs@faraday-tech.com");
MODULE_DESCRIPTION("Cadence PCI-E Gen2 root complex driver for HGU10G platform");
MODULE_LICENSE("GPL");
