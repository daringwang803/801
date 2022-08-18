/*
 * FTUSART010 Serial Driver
 *
 * Copyright (C) 2019 Faraday Technology Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#define DRIVER_NAME "ftusart010"

/* Register offsets */
#define FTUSART010_STS		0x00
#define FTUSART010_RDR		0x04
#define FTUSART010_TDR		0x04
#define FTUSART010_BAUDR	0x08
#define FTUSART010_CTRL1	0x0c
#define FTUSART010_CTRL2	0x10
#define FTUSART010_CTRL3	0x14
#define FTUSART010_GTP		0x18

/* FTUSART010_STS */
#define USART_STS_PERR		BIT(0)
#define USART_STS_FERR		BIT(1)
#define USART_STS_NERR		BIT(2)
#define USART_STS_ORERR		BIT(3)
#define USART_STS_IDLEF		BIT(4)
#define USART_STS_RDNE		BIT(5)
#define USART_STS_TRAC		BIT(6)
#define USART_STS_TDE		BIT(7)
#define USART_STS_BDF		BIT(8)
#define USART_STS_CTSF		BIT(9)
#define USART_STS_ERR_MASK	(USART_STS_BDF | USART_STS_ORERR | \
				 USART_STS_FERR | USART_STS_PERR)
/* FTUSART010_DT */
#define USART_DT_MASK		GENMASK(8, 0)

/* FTUSART010_BAUDR */
#define USART_BAUDR_DIV_D_MASK	GENMASK(3, 0)
#define USART_BAUDR_DIV_I_MASK	GENMASK(15, 4)
#define USART_BAUDR_DIV_I_SHIFT	4

/* FTUSART010_CTRL1 */
#define USART_CTRL1_SBRK	BIT(0)
#define USART_CTRL1_RECMUTE	BIT(1)
#define USART_CTRL1_REN		BIT(2)
#define USART_CTRL1_TEN		BIT(3)
#define USART_CTRL1_IDLEIEN	BIT(4)
#define USART_CTRL1_RDNEIEN	BIT(5)
#define USART_CTRL1_TRACIEN	BIT(6)
#define USART_CTRL1_TDEIEN	BIT(7)
#define USART_CTRL1_PERRIEN	BIT(8)
#define USART_CTRL1_PSEL	BIT(9)
#define USART_CTRL1_PCEN	BIT(10)
#define USART_CTRL1_WUMODE	BIT(11)
#define USART_CTRL1_LEN		BIT(12)
#define USART_CTRL1_UEN		BIT(13)
#define USART_CTRL1_IEN_MASK	GENMASK(8, 4)

/* FTUSART010_CTRL2 */
#define USART_CTRL2_ADDR_MASK	GENMASK(3, 0)
#define USART_CTRL2_LBDLEN	BIT(5)
#define USART_CTRL2_BDIEN	BIT(6)
#define USART_CTRL2_LBCP	BIT(8)
#define USART_CTRL2_CLKPHA	BIT(9)
#define USART_CTRL2_CLKPOL	BIT(10)
#define USART_CTRL2_CLKEN	BIT(11)
#define USART_CTRL2_STOPB_2B	BIT(13)
#define USART_CTRL2_STOPB_MASK	GENMASK(13, 12)
#define USART_CTRL2_LINEN	BIT(14)

/* FTUSART010_CTRL3 */
#define USART_CTRL3_ERRIEN	BIT(0)
#define USART_CTRL3_IRDAEN	BIT(1)
#define USART_CTRL3_IRDALP	BIT(2)
#define USART_CTRL3_HALFSEL	BIT(3)
#define USART_CTRL3_NACKEN	BIT(4)
#define USART_CTRL3_SCMEN	BIT(5)
#define USART_CTRL3_DMAREN	BIT(6)
#define USART_CTRL3_DMATEN	BIT(7)
#define USART_CTRL3_RTSEN	BIT(8)
#define USART_CTRL3_CTSEN	BIT(9)
#define USART_CTRL3_CTSIEN	BIT(10)

/* FTUSART010_GTP */
#define USART_GTP_DIV_MASK	GENMASK(7, 0)
#define USART_GTP_GTVAL_MASK	GENMASK(15, 8)

#define FTUSART010_SERIAL_NAME 	"ttyS"
#define FTUSART010_MAX_PORTS 	8

#define RX_BUF_L 1024		/* dma rx buffer length     */
#define RX_BUF_P 1024		/* dma rx buffer period     */
#define TX_BUF_L 1024		/* dma tx buffer length     */

struct ftusart010_port {
	struct uart_port port;
	struct clk *clk;
	struct dma_chan *rx_ch;  /* dma rx channel            */
	dma_addr_t rx_dma_buf;   /* dma rx buffer bus address */
	unsigned char *rx_buf;   /* dma rx buffer cpu address */
	struct dma_chan *tx_ch;  /* dma tx channel            */
	dma_addr_t tx_dma_buf;   /* dma tx buffer bus address */
	unsigned char *tx_buf;   /* dma tx buffer cpu address */
	int last_res;
	volatile bool tx_dma_busy;   /* dma tx busy           */
};

static struct ftusart010_port ftusart010_ports[FTUSART010_MAX_PORTS];
static struct uart_driver ftusart010_driver;
