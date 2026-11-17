/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2001, 2002 Ralf Baechle
 */

#include <asm/page.h>
#include <asm/setup.h>
#include <asm/sn/addrs.h>
#include <asm/sn/agent.h>
#include <asm/sn/klconfig.h>
#include <asm/sn/ioc3.h>

#include <linux/serial.h>
#include <linux/serial_core.h>

#include "ip27-common.h"

#define IOC3_CLK	(22000000 / 3)
#define IOC3_FLAGS	(0)

#if 1

static inline struct ioc3_uartregs *console_uart(void)
{
#if 0
	struct ioc3 *ioc3;
	nasid_t nasid;

	nasid = (master_nasid == INVALID_NASID) ? get_nasid() : master_nasid;
	ioc3 = (struct ioc3 *)KL_CONFIG_CH_CONS_INFO(nasid)->memory_base;

	return &ioc3->sregs.uarta;
	return (struct ioc3_uartregs *)((unsigned long)ioc3 + 0x380);
#else
	return (struct ioc3_uartregs *)0x920000000f200380;
#endif
}

void prom_putchar(char c)
{
	struct ioc3_uartregs *uart = console_uart();

	while ((readb(&uart->iu_lsr) & 0x20) == 0)
		;
	writeb(c, &uart->iu_thr);
}
#else

#include <linux/crc-ccitt.h>
#include <linux/serial_reg.h>
#include <asm/sn/ksys/l1.h>

#define BRL1_FLAG_CH    0x7e
#define BRL1_ESC_CH     0x7d
#define BRL1_XOR_CH     0x20

#define INIT_CRC        0xffff  /* Initial FCS value */
#define GOOD_CRC        0xf0b8  /* Good final FCS value */

static u64 *regs = (u64 *)REMOTE_HSPEC_ADDR(0, 0x80);

static void ip35_l1_putc(u8 c, u16 *crc, bool esc)
{
	if (crc)
		*crc = crc_ccitt_byte(*crc, c);


	if (esc && (c == BRL1_FLAG_CH || c == BRL1_ESC_CH)) {
		while ((readq(&regs[UART_LSR]) & 0x20) == 0)
			;
		writeq(BRL1_ESC_CH, &regs[0]);
		c ^= BRL1_XOR_CH;
	}
	while ((readq(&regs[UART_LSR]) & 0x20) == 0)
		;
	writeq(c, &regs[0]);
}

void prom_putchar(char c)
{
	u16 crc = INIT_CRC;

	ip35_l1_putc(BRL1_FLAG_CH, NULL, false);
	ip35_l1_putc(BRL1_EVENT | SC_CONS_SYSTEM, &crc, true);
	ip35_l1_putc(c, &crc, true);
	crc ^= 0xffff;
	ip35_l1_putc(crc & 0xff, NULL, true);
	ip35_l1_putc(crc >> 8, NULL, true);
	ip35_l1_putc(BRL1_FLAG_CH, NULL, false);
}
#endif
