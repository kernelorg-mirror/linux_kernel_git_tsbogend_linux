// SPDX-License-Identifier: GPL-2.0

#include <asm/sn/ksys/l1.h>
#include <linux/crc-ccitt.h>
#include <asm/sn/types.h>
#include <asm/sn/arch.h>
#include <asm/sn/addrs.h>
#include <linux/serial_reg.h>
#include <linux/timer.h>

#define L1_CONS_POLL_DELAY (30 * HZ / 1000)

/* error codes */
#define UART_LINK       (-2)
#define UART_NO_CHAR    (-3)

/*
 * Checksumming like PPP
 */
#define INIT_CRC	0xffff	/* Initial FCS value */
#define GOOD_CRC	0xf0b8	/* Good final FCS value */

/*
 */
#define BRL1_FLAG_CH	0x7e
#define BRL1_ESC_CH	0x7d
#define BRL1_XOR_CH	0x20


#define LD(x)			(*(volatile u64 *)(x))
#define SD(x, v)		(LD(x) = (u64) (v))

static inline unsigned long L1_UART_BASE(nasid_t node)
{
	return (unsigned long) REMOTE_HSPEC_ADDR(node, 0x00000080);
}

static inline unsigned long ADDR_L1_REG(nasid_t node, unsigned long r)
{
	return L1_UART_BASE(node) | ((r) << 3);
}

static inline unsigned long READ_L1_UART_REG(nasid_t node, unsigned long r)
{
	return LD(ADDR_L1_REG(node, r));
}

static inline void WRITE_L1_UART_REG(nasid_t node, unsigned long r,
	unsigned long v)
{
	SD(ADDR_L1_REG(node, r), v);
}

static void l1_putc(unsigned char c)
{
	nasid_t nasid = 0;

	while ((!(READ_L1_UART_REG(nasid, UART_LSR) & UART_LSR_THRE)) ||
	       (!(READ_L1_UART_REG(nasid, UART_MSR) & UART_MSR_CTS)))
		;

	WRITE_L1_UART_REG(nasid, UART_TX, c);
}

static int l1_getc(void)
{
	u8 lsr_reg;
	nasid_t nasid = 0;

	lsr_reg = READ_L1_UART_REG(nasid, UART_LSR);
	if (lsr_reg & (UART_LSR_DR | UART_LSR_PE | UART_LSR_FE)) {
		if (lsr_reg & UART_LSR_DR)
			return (u8)READ_L1_UART_REG(nasid, UART_RX);

		return UART_LINK;
	}

	return UART_NO_CHAR;
}

static void early_l1_serial_out(char *str, int len)
{
	while (len) {
		l1_putc(*str);
		str++;
		len--;
	}
}

/*
 * brl1_send formats up a packet and (at least begins to) send it
 * to the uart.  If the send buffer is in use when this routine obtains
 * the lock, it will behave differently depending on the "wait" parameter.
 * For wait == 0 (most I/O), it will return 0 (as in "zero bytes sent"),
 * hopefully encouraging the caller to back off (unlock any high-level
 * spinlocks) and allow the buffer some time to drain.  For wait==1 (high-
 * priority I/O along the lines of kernel error messages), we will flush
 * the current contents of the send buffer and beat on the uart
 * until our message has been completely transmitted.
 */

int brl1_send(const char *msg, int len, unsigned char type_and_subch)
{
	unsigned short crc = INIT_CRC;
	static char send[BRL1_BUFSZ];	/* send buffer */
	char *send_ptr = send;
	int send_len;			/* number of characters in send buf */
	int index;

	*send_ptr++ = BRL1_FLAG_CH;
	*send_ptr++ = type_and_subch;
	crc = crc_ccitt_byte(crc, type_and_subch);

	/*
	 * Limit number of characters accepted to max payload size
	 */
	if (len > (BRL1_QSIZE - 1))
		len = (BRL1_QSIZE - 1);

	/*
	 * Copy in the message buffer inserting PPP framing info as necessary
	 */
	for (index = 0; index < len; index++) {
		switch (*msg) {
		case BRL1_FLAG_CH:
		case BRL1_ESC_CH:
			*send_ptr++ = BRL1_ESC_CH;
			*send_ptr++ = (*msg) ^ BRL1_XOR_CH;
			break;

		default:
			*send_ptr++ = *msg;
		}
		crc = crc_ccitt_byte(crc, *msg);
		msg++;
	}
	crc ^= 0xffff;

	for (index = 0; index < sizeof(crc); index++) {
		char crc_char = (char) (crc & 0x00ff);

		if ((crc_char == BRL1_ESC_CH)
		    || (crc_char == BRL1_FLAG_CH)) {
			*send_ptr++ = BRL1_ESC_CH;
			crc_char ^= BRL1_XOR_CH;
		}
		*send_ptr++ = crc_char;
		crc >>= 8;
	}

	*send_ptr++ = BRL1_FLAG_CH;

	send_len = send_ptr - send;

	early_l1_serial_out(send, send_len);

	return len;
}

/*
 * brl1_receive_core is the start of being able to read events and
 * responses from L1.  It is intended to be called from an interrupt
 * handler or in polling fashion, and deals with recieving data from
 * the UART and stripping off the HDLC framing.  It returns 0 when a
 * packet has been read, updating the buffer and length parameters
 * with the bounds of the packet.  It is presumed that no packet will
 * be longer than BRL1_QSIZE.  It does not do any interpretation of
 * the packets.
 *
 * This will eventually need to be rewritten to handle dealing with
 * multiple L1 controllers.
 */

#define HDLC_IDLE 0
#define HDLC_MSG 2
#define HDLC_ESC 3

int brl1_receive_core(unsigned char **buffer, int *length)
{
	static int state = HDLC_IDLE;
	static char buf[BRL1_QSIZE+2]; /* 2 bytes for CRC */
	static char *bufptr;
	static unsigned short crc;
	int data;

	while (1) {
		data = l1_getc();
		if ((data == UART_LINK) || (data == UART_NO_CHAR))
			return data;

		switch (state) {
		default:
			/* Some unknown state, reset to IDLE */
			state = HDLC_IDLE;
			/* And drop through to IDLE */

		case HDLC_IDLE:
			/* Transition to MSG if we see a flag */
			if (data == BRL1_FLAG_CH) {
				bufptr = buf;
				crc = INIT_CRC;
				state = HDLC_MSG;
			}
			break;

		case HDLC_MSG:
			if (data == BRL1_ESC_CH) {
				state = HDLC_ESC;
				break;
			} else if (data == BRL1_FLAG_CH) {
				/* Packet (possibly) complete */
				if ((crc != GOOD_CRC) || ((bufptr - buf) < 2)) {
					/* Drop bogus packets */
					state = HDLC_IDLE;
				} else {
					*buffer = buf;
					*length = (bufptr - buf) - 2;
					state = HDLC_IDLE;
					return 0; /* Success! */
				}
			} else {
				/* FIXME: bounds-check! */
				*bufptr++ = data;
				crc = crc_ccitt_byte(crc, data);
			}
			break;

		case HDLC_ESC:
			if ((data == BRL1_FLAG_CH) ||
			    (data == BRL1_ESC_CH)) {
				/* Someone's playing sillybuggers */
				state = HDLC_IDLE;
			} else {
				*bufptr++ = data ^ BRL1_XOR_CH;
				crc = crc_ccitt_byte(crc, data ^ BRL1_XOR_CH);
				state = HDLC_MSG;
			}
			break;
		}
	}
}

static void l1_console_poll(struct timer_list *unused);
static DEFINE_TIMER(l1_console_timer, l1_console_poll);

static l1_packet_handler *l1_console_packet_handler;

int l1_packet_handler_set(l1_packet_handler *handler, u8 type_and_subch)
{
	if (type_and_subch != SC_CONS_SYSTEM) {
		pr_err("l1_packet_handler_set: unsupported packet type 0x%02x\n", type_and_subch);
		return -ENODEV;
	}

	l1_console_packet_handler = handler;
	mod_timer(&l1_console_timer, jiffies + L1_CONS_POLL_DELAY);

	return 0;
}

void l1_packet_handler_clear(u8 type_and_subch)
{
	if (type_and_subch != SC_CONS_SYSTEM) {
		pr_err("l1_packet_handler_clear: unsupported packet type 0x%02x\n", type_and_subch);
		return;
	}

	del_timer_sync(&l1_console_timer);
	l1_console_packet_handler = NULL;
}

static void l1_console_poll(struct timer_list *unused)
{
	unsigned char *buffer;
	int length;

	/*
	 * KLUDGE: Poll for packets from L1, disarding any that aren't
	 * console input.  This will likely need to be rewritten soon.
	 */
	while (!brl1_receive_core(&buffer, &length)) {
		if ((length > 1) &&
		    (buffer[0] == SC_CONS_SYSTEM) &&
		    l1_console_packet_handler)
			l1_console_packet_handler(buffer, length);
	}

	if (l1_console_packet_handler)
		mod_timer(&l1_console_timer, jiffies + L1_CONS_POLL_DELAY);
}
