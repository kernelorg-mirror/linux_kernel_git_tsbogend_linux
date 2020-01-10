#ifndef _ASM_SN_KSYS_L1_H
#define _ASM_SN_KSYS_L1_H

#include <asm/sn/types.h>
#include <asm/sn/arch.h>

#define BRL1_QSIZE	128	/* power of 2 is more efficient */
#define BRL1_BUFSZ	264	/* needs to be large enough
				 * to hold 2 flags, escaped
				 * CRC, type/subchannel byte,
				 * and escaped payload
				 */

/* L1<->Bedrock packet types */
#define BRL1_REQUEST	0x00
#define BRL1_RESPONSE	0x20
#define BRL1_EVENT	0x40

/* L1<->Bedrock reserved subchannels */

/* console channels */
#define SC_CONS_CPU0	0x00
#define SC_CONS_CPU1	0x01
#define SC_CONS_CPU2	0x02
#define SC_CONS_CPU3	0x03

#define L1_ELSCUART_SUBCH(p)	(p)
#define L1_ELSCUART_CPU(ch)	(ch)

#define SC_CONS_SYSTEM	CPUS_PER_NODE


typedef void l1_packet_handler(char *, int);

int brl1_send(const char *msg, int len, unsigned char type_and_subch);
int l1_packet_handler_set(l1_packet_handler *handler, u8 type_and_subch);
void l1_packet_handler_clear(u8 type_and_subch);


#endif /* _ASM_SN_KSYS_L1_H */
