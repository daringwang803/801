/*
 * FARADAY FTUART010 IrDA SIR Driver Header File
 *
 * Copyright (C) 2019 Faraday Technology Corp.
 * Jack Chain <jack_ch@faraday-tech.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ftuart010_irda_SIR_H
#define __ftuart010_irda_SIR_H

/******************************************************************************
 * define  
 *****************************************************************************/

//============================ Baund Rate  Mapping =============================
#define IRDA_CLK                             30000000//48000000//47923200
#define PSR_CLK                              IRDA_CLK

#define IRDA_BAUD_115200                     1
#define IRDA_BAUD_57600                      2
#define IRDA_BAUD_38400                      3
#define IRDA_BAUD_19200                      4
#define IRDA_BAUD_14400                      5
#define IRDA_BAUD_9600                       6


//============================ Register Address Mapping ========================

#define SERIAL_THR                     0x00        /*  Transmitter Holding Register(Write).*/
#define SERIAL_RBR                     0x00        /*  Receive Buffer register (Read).*/
#define SERIAL_IER                     0x04        /*  Interrupt Enable register.*/
#define SERIAL_IIR                     0x08        /*  Interrupt Identification register(Read).*/
#define SERIAL_FCR                     0x08        /*  FIFO control register(Write).*/
#define SERIAL_LCR                     0x0C        /*  Line Control register.*/
#define SERIAL_MCR                     0x10        /*  Modem Control Register.*/
#define SERIAL_LSR                     0x14        /*  Line status register(Read) .*/
#define SERIAL_MSR                     0x18        /*  Modem Status register (Read).*/
#define SERIAL_SPR                     0x1C        /*  Scratch pad register */
#define SERIAL_DLL                     0x00        /*  Divisor Register LSB */
#define SERIAL_DLM                     0x04        /*  Divisor Register MSB */
#define SERIAL_PSR                     0x08        /*  Prescale Divison Factor */

#define SERIAL_MDR                     0x20
#define SERIAL_ACR                     0x24
#define SERIAL_TXLENL                  0x28
#define SERIAL_TXLENH                  0x2C
#define SERIAL_MRXLENL                 0x30
#define SERIAL_MRXLENH                 0x34
#define SERIAL_PLR                     0x38
#define SERIAL_FMIIR_PIO               0x3C

//Registre Address Define (IrDA/FIR)
#define SERIAL_TST                     0x14
#define SERIAL_FMIIR_DMA               0x3C
#define SERIAL_FMIIR                   0x3C
#define SERIAL_FMIIER                  0x40
#define SERIAL_STFF_STS                0x44
#define SERIAL_STFF_RXLENL             0x48
#define SERIAL_STFF_RXLENH             0x4c
#define SERIAL_FMLSR                   0x50
#define SERIAL_FMLSIER                 0x54
#define SERIAL_RSR                     0x58
#define SERIAL_RXFF_CNTR               0x5c
#define SERIAL_LSTFMLENL               0x60
#define SERIAL_LSTFMLENH               0x64

//=========================== Register Value/Bit Define ========================
/* IER Register */
#define SERIAL_IER_DR                  0x1         /* Data ready Enable */
#define SERIAL_IER_TE                  0x2         /* THR Empty Enable */
#define SERIAL_IER_RLS                 0x4         /* Receive Line Status Enable */
#define SERIAL_IER_MS                  0x8         /* Modem Staus Enable */

/* IIR Register */
#define SERIAL_IIR_NONE                0x1         /* No interrupt pending */
#define SERIAL_IIR_RLS                 0x6         /* Receive Line Status */
#define SERIAL_IIR_DR                  0x4         /* Receive Data Ready */
#define SERIAL_IIR_TIMEOUT             0xc         /* Receive Time Out */
#define SERIAL_IIR_TE                  0x2         /* THR Empty */
#define SERIAL_IIR_MODEM               0x0         /* Modem Status */

/* FCR Register */
#define SERIAL_FCR_FE                  0x1         /* FIFO Enable */
#define SERIAL_FCR_RXFR                0x2         /* Rx FIFO Reset */
#define SERIAL_FCR_TXFR                0x4         /* Tx FIFO Reset */

/* LCR Register */
#define SERIAL_LCR_LEN5                0x0
#define SERIAL_LCR_LEN6                0x1
#define SERIAL_LCR_LEN7                0x2
#define SERIAL_LCR_LEN8                0x3
#define SERIAL_LCR_STOP                0x4
#define SERIAL_LCR_EVEN                0x18        /* Even Parity */
#define SERIAL_LCR_ODD                 0x8         /* Odd Parity */
#define SERIAL_LCR_PE                  0x8         /* Parity Enable */
#define SERIAL_LCR_SETBREAK            0x40        /* Set Break condition */
#define SERIAL_LCR_STICKPARITY         0x20        /* Stick Parity Enable */
#define SERIAL_LCR_DLAB                0x80        /* Divisor Latch Access Bit */

/* LSR Register */
#define SERIAL_LSR_DR                  0x1         /* Data Ready */
#define SERIAL_LSR_OE                  0x2         /* Overrun Error */
#define SERIAL_LSR_PE                  0x4         /* Parity Error */
#define SERIAL_LSR_FE                  0x8         /* Framing Error */
#define SERIAL_LSR_BI                  0x10        /* Break Interrupt */
#define SERIAL_LSR_THRE                0x20        /* THR Empty */
#define SERIAL_LSR_TE                  0x40        /* Transmitte Empty */
#define SERIAL_LSR_DE                  0x80        /* FIFO Data Error */

/* MCR Register */
#define SERIAL_MCR_DTR                 0x1      /* Data Terminal Ready */
#define SERIAL_MCR_RTS                 0x2      /* Request to Send */
#define SERIAL_MCR_OUT1                0x4      /* output   1 */
#define SERIAL_MCR_OUT2                0x8      /* output2 or global interrupt enable */
#define SERIAL_MCR_LPBK                0x10     /* loopback mode */
#define SERIAL_MCR_DMAMODE2            0x20     /* DMA mode2 */
#define SERIAL_MCR_OUT3                0x40     /* output 3 */

/* MSR Register */
#define SERIAL_MSR_DELTACTS            0x1      /* Delta CTS */
#define SERIAL_MSR_DELTADSR            0x2      /* Delta DSR */
#define SERIAL_MSR_TERI                0x4      /* Trailing Edge RI */
#define SERIAL_MSR_DELTACD             0x8      /* Delta CD */
#define SERIAL_MSR_CTS                 0x10     /* Clear To Send */
#define SERIAL_MSR_DSR                 0x20     /* Data Set Ready */
#define SERIAL_MSR_RI                  0x40     /* Ring Indicator */
#define SERIAL_MSR_DCD                 0x80     /* Data Carrier Detect */

/* MDR register */
#define SERIAL_MDR_MODE_SEL            0x3
#define SERIAL_MDR_UART                0x0
#define SERIAL_MDR_SIR                 0x1
#define SERIAL_MDR_FIR                 0x2

/* ACR register */
#define SERIAL_ACR_TXENABLE            0x1
#define SERIAL_ACR_RXENABLE            0x2
#define SERIAL_ACR_SET_EOT             0x4

/* FCR register */
#define SERIAL_FCR_TX_FIFO_RESET       0x4
#define SERIAL_FCR_RX_FIFO_RESET       0x2
#define SERIAL_FCR_TXRX_FIFO_ENABLE    0x1

/* MCR register */
#define SERIAL_MCR_DMA2                0x20

/* TST register */
#define SERIAL_TST_TEST_PAR_ERR        0x1
#define SERIAL_TST_TEST_FRM_ERR        0x2
#define SERIAL_TST_TEST_BAUDGEN        0x4
#define SERIAL_TST_TEST_PHY_ERR        0x8
#define SERIAL_TST_TEST_CRC_ERR        0x10

/* MDR register */
#define SERIAL_MDR_IITx                0x40
#define SERIAL_MDR_FIRx                0x20
#define SERIAL_MDR_MDSEL               0x1c
#define SERIAL_MDR_DMA_ENABLE          0x10
#define SERIAL_MDR_FMEND_ENABLE        0x8
#define SERIAL_MDR_SIP_BY_CPU          0x4
#define SERIAL_MDR_UART_MODE           0x0
#define SERIAL_MDR_SIR_MODE            0x1
#define SERIAL_MDR_FIR_MODE            0x2
#define SERIAL_MDR_TX_INV              0x40
#define SERIAL_MDR_RX_INV              0x20

/* ACR register */
#define SERIAL_ACR_TX_ENABLE           0x1
#define SERIAL_ACR_RX_ENABLE           0x2
#define SERIAL_ACR_FIR_SET_EOT         0x4
#define SERIAL_ACR_FORCE_ABORT         0x8
#define SERIAL_ACR_SEND_SIP            0x10
#define SERIAL_ACR_STFF_TRGL_1         0x1
#define SERIAL_ACR_STFF_TRGL_4         0x4
#define SERIAL_ACR_STFF_TRGL_7         0x7
#define SERIAL_ACR_STFF_TRGL_8         0x8
#define SERIAL_ACR_STFF_TRG1           0x0
#define SERIAL_ACR_STFF_TRG2           0x20
#define SERIAL_ACR_STFF_TRG3           0x40
#define SERIAL_ACR_STFF_TRG4           0x60
#define SERIAL_ACR_STFF_CLEAR          0x9F

/******************************************************************************
 * SIR mode functions 
 *****************************************************************************/
static void fLib_SIR_Init(unsigned int port, unsigned int baudrate,
   unsigned int SIP_PW_Value, unsigned int dwRX_Tri_Value,unsigned int bInv);

//TX  
static void fLib_SIR_TX_Open(unsigned int port);
static void fLib_SIR_TX_Close(unsigned int port); 
static int  fLib_SIR_TX_Write_Data(unsigned int port, char data);
static void fLib_SIR_TX_Int_init(unsigned int port);

//RX
static void fLib_SIR_RX_Open(unsigned int port);
static void fLib_SIR_RX_Close(unsigned int port);
static int  fLib_SIR_RX_Read_Data(unsigned int port);
static void fLib_SIR_RX_Int_init(unsigned int port);

#endif //end of __ftuart010_irda_SIR_H
