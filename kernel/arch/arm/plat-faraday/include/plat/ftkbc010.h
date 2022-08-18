/*
 * Faraday FTKBC010 Keyboard Controller
 *
 * (C) Copyright 2009 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
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

#ifndef __FTKBC010_H
#define __FTKBC010_H

#define FTKBC010_OFFSET_CR    0x00
#define FTKBC010_OFFSET_SRD   0x04
#define FTKBC010_OFFSET_RSC   0x08
#define FTKBC010_OFFSET_SR    0x0c
#define FTKBC010_OFFSET_ISR   0x10
#define FTKBC010_OFFSET_RX    0x14
#define FTKBC010_OFFSET_TX    0x18
#define FTKBC010_OFFSET_IMR   0x1c
#define FTKBC010_OFFSET_XC    0x30
#define FTKBC010_OFFSET_YC    0x34
#define FTKBC010_OFFSET_ASP   0x38
#define FTKBC010_OFFSET_REV   0x50
#define FTKBC010_OFFSET_FEAT  0x54

/*
 * Control register
 */
#define FTKBC010_CR_CLOCKDOWN (1 << 0)
#define FTKBC010_CR_DATADOWN  (1 << 1)
#define FTKBC010_CR_ENABLE    (1 << 2)
#define FTKBC010_CR_EN_TXINT  (1 << 3) /* must be set if EN_ACK is set */
#define FTKBC010_CR_EN_RXINT  (1 << 4)
#define FTKBC010_CR_EN_ACK    (1 << 5)
#define FTKBC010_CR_CL_TXINT  (1 << 6)
#define FTKBC010_CR_CL_RXINT  (1 << 7)
#define FTKBC010_CR_EN_PAD    (1 << 8)
#define FTKBC010_CR_AUTOSCAN  (1 << 9)
#define FTKBC010_CR_CL_PADINT (1 << 10)
#define FTKBC010_CR_RECINT    (1 << 11)

/*
 * Sample rate division register
 */
#define FTKBC010_SRD_DIV(x)   ((x) & 0x3f)

/*
 * Request to send counter register
 */
#define FTKBC010_RSC_COUNT(x) ((x) & 0x3fff)

/*
 * Status register
 */
#define FTKBC010_SR_RX_PARITY (1 << 0)
#define FTKBC010_SR_RX_BUSY   (1 << 1)
#define FTKBC010_SR_RX_FULL   (1 << 2)
#define FTKBC010_SR_TX_BUSY   (1 << 3)
#define FTKBC010_SR_TX_EMPTY  (1 << 4)

/*
 * Interrupt status register
 */
#define FTKBC010_ISR_RXINT    (1 << 0)
#define FTKBC010_ISR_TXINT    (1 << 1)
#define FTKBC010_ISR_PADINT   (1 << 2)

/*
 * Keyboard receive register
 */
#define FTKBC010_RX_DATA_MASK 0xff

/*
 * Keyboard transmit register
 */
#define FTKBC010_TX_DATA(x)   ((x) & 0xff)

/*
 * Interrupt mask register
 */
#define FTKBC010_IMR_RXINT    (1 << 0)
#define FTKBC010_IMR_TXINT    (1 << 1)
#define FTKBC010_IMR_PADINT   (1 << 2)

/*
 * Keypad code X register
 */
#define FTKBC010_XC_L(xc)     (((xc) >> 0) & 0xff)
#define FTKBC010_XC_PADINTA   (1 << 15)
#define FTKBC010_XC_H(xc)     (((xc) >> 16) & 0xff)
#define FTKBC010_XC_PADINTB   (1 << 31)

/*
 * Keypad code Y register
 */
#define FTKBC010_YC_L(yc)  (((yc) >> 0) & 0xffff)
#define FTKBC010_YC_H(yc)  (((yc) >> 16) & 0xffff)

/*
 * Auto scan period register
 */
#define FTKBC010_ASP_PERIOD(x)   ((x) & 0xffffff)

#endif   /* __FTKBC010_H */
