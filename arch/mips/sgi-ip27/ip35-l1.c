// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/crc-ccitt.h>
#include <linux/circ_buf.h>
#include <linux/completion.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/serial_reg.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>

#include <asm/sn/irq_alloc.h>
#include <asm/sn/intr.h>
#include <asm/sn/ksys/l1.h> // XXX

#define L1_XMIT_SIZE	2048
#define L1_FIFO_SIZE	16

#define BRL1_FLAG_CH	0x7e
#define BRL1_ESC_CH	0x7d
#define BRL1_XOR_CH	0x20

#define INIT_CRC	0xffff  /* Initial FCS value */
#define GOOD_CRC	0xf0b8  /* Good final FCS value */

struct l1_ctrl;

struct l1_chan {
	struct l1_ctrl *l1;
	void	(*push_msg)(struct l1_chan *);
	void	*resp_data;
	size_t	resp_len;
	struct	completion comp;
	void	*priv;
	u8	type_chan;
};

struct l1_ctrl {
	u64 __iomem *regs;
	struct circ_buf tx_circ;
	u8 tx_buf[L1_XMIT_SIZE];
	u16 rx_crc;
	u8 rx_buf[512];
	int rx_idx;
	enum {
		L1_HDLC_IDLE,
		L1_HDLC_ESC,
		L1_HDLC_MSG
	} state;
	struct l1_chan channels[32];
};

struct l1_port {
	struct tty_port port;
	struct l1_chan *chan;
};



static inline u8 read_uart(struct l1_ctrl *l1, int reg)
{
	return readq(&l1->regs[reg]) & 0xff;
}

static inline void write_uart(struct l1_ctrl *l1, int reg, u8 val)
{
	writeq(val, &l1->regs[reg]);
}

static void ip35_l1_tty_push_msg(struct l1_chan *chan)
{
	struct l1_port *l1_port = chan->priv;
	struct l1_ctrl *l1 = chan->l1;

	tty_insert_flip_string(&l1_port->port, l1->rx_buf + 1, l1->rx_idx - 3);
	tty_flip_buffer_push(&l1_port->port);
}

static void ip35_l1_cmd_push_msg(struct l1_chan *chan)
{
	struct l1_ctrl *l1 = chan->l1;
	int len = l1->rx_idx - 3;

	chan->resp_data = kmemdup(l1->rx_buf + 1, len, GFP_KERNEL);
	if (!chan->resp_data)
		return;

	chan->resp_len = len;

	complete(&chan->comp);
}

static void ip35_l1_dispatch_msg(struct l1_ctrl *l1)
{
	struct l1_chan *chan;

	if (l1->rx_crc != GOOD_CRC) {
		pr_info("L1: wrong crc %x\n", l1->rx_crc);
		return;
	}

	chan = &l1->channels[l1->rx_buf[0] & 0x1f];
	if (!chan || !chan->push_msg)
		return;

	chan->push_msg(chan);
}

static void ip35_l1_recv(struct l1_ctrl *l1)
{
	u8 c;

	do {
		c = read_uart(l1, UART_RX);
		switch (l1->state) {
		case L1_HDLC_IDLE:
			if (c == BRL1_FLAG_CH) {
				l1->rx_crc = INIT_CRC;
				l1->rx_idx = 0;
				l1->state = L1_HDLC_MSG;
			}
			break;
		case L1_HDLC_MSG:
			switch (c) {
			case BRL1_FLAG_CH:
				ip35_l1_dispatch_msg(l1);
				l1->state = L1_HDLC_IDLE;
				break;
			case BRL1_ESC_CH:
				l1->state = L1_HDLC_ESC;
				break;
			default:
				l1->rx_crc = crc_ccitt_byte(l1->rx_crc, c);
				l1->rx_buf[l1->rx_idx++] = c;
				break;
			}
			break;
		case L1_HDLC_ESC:
			if (c == BRL1_FLAG_CH || c == BRL1_ESC_CH) {
				l1->state = L1_HDLC_IDLE;
			} else {
				c ^= BRL1_XOR_CH;
				l1->rx_crc = crc_ccitt_byte(l1->rx_crc, c);
				l1->rx_buf[l1->rx_idx++] = c;
			}
			break;
		}
	} while (read_uart(l1, UART_LSR) & UART_LSR_DR);
}

static void ip35_l1_xmit(struct l1_ctrl *l1)
{
	struct circ_buf *circ = &l1->tx_circ;
	int i, count;

	count = CIRC_CNT(circ->head, circ->tail, L1_XMIT_SIZE);
	if (count > L1_FIFO_SIZE)
		count = L1_FIFO_SIZE;
	for (i = 0; i < count; i++) {
		write_uart(l1, UART_TX, circ->buf[circ->tail]);
		circ->tail = (circ->tail + 1) & (L1_XMIT_SIZE - 1);
	}
	if (CIRC_CNT(circ->head, circ->tail, L1_XMIT_SIZE) == 0)
		write_uart(l1, UART_IER, UART_IER_RDI);
}

static irqreturn_t ip35_l1_interrupt(int irq, void *dev_id)
{
	struct l1_ctrl *l1 = dev_id;
	u8 iir;

	iir = read_uart(l1, UART_IIR) & UART_IIR_ID;
	if (iir != UART_IIR_NO_INT) {
		if (iir & UART_IIR_RDI)
			ip35_l1_recv(l1);
		if (iir & UART_IIR_THRI)
			ip35_l1_xmit(l1);
	}
	return IRQ_HANDLED;
}

static void ip35_l1_putc(struct circ_buf *circ, u8 c, u16 *crc, bool esc)
{
	if (crc)
		*crc = crc_ccitt_byte(*crc, c);

	if (esc && (c == BRL1_FLAG_CH || c == BRL1_ESC_CH)) {
		circ->buf[circ->head] = BRL1_ESC_CH;
		circ->head = (circ->head + 1) & (L1_XMIT_SIZE - 1);
		c ^= BRL1_XOR_CH;
	}
	circ->buf[circ->head] = c;
	circ->head = (circ->head + 1) & (L1_XMIT_SIZE - 1);
}

static int ip35_l1_write(struct l1_chan *chan, const unsigned char *buf,
			 int count)
{
	struct l1_ctrl *l1 = chan->l1;
	struct circ_buf *circ = &l1->tx_circ;
	int ret = 0;
	u16 crc = INIT_CRC;
	int i;

	// check minimum space
	ret = count;

	ip35_l1_putc(circ, BRL1_FLAG_CH, NULL, false);
	ip35_l1_putc(circ, chan->type_chan, &crc, true);

	for (i = 0; i < ret; i++)
		ip35_l1_putc(circ, buf[i], &crc, true);

	crc ^= 0xffff;

	ip35_l1_putc(circ, crc & 0xff, NULL, true);
	ip35_l1_putc(circ, crc >> 8, NULL, true);

	ip35_l1_putc(circ, BRL1_FLAG_CH, NULL, false);

	if (CIRC_CNT(circ->head, circ->tail, L1_XMIT_SIZE))
		write_uart(l1, UART_IER, UART_IER_RDI | UART_IER_THRI);
	
	return ret;
}

static void ip35_l1_start(struct l1_ctrl *l1)
{
	l1->state = L1_HDLC_IDLE;

	write_uart(l1, UART_MCR, UART_MCR_RTS | UART_MCR_AFE);
	write_uart(l1, UART_FCR, UART_FCR_ENABLE_FIFO);
	write_uart(l1, UART_FCR, UART_FCR_ENABLE_FIFO | UART_FCR_CLEAR_RCVR |
				 UART_FCR_CLEAR_XMIT | UART_FCR_TRIGGER_8);
	write_uart(l1, UART_IER, UART_IER_RDI);
}

static int ip35_l1_tty_open(struct tty_struct *tty, struct file *filp)
{
	return tty_port_open(tty->port, tty, filp);
}

static void ip35_l1_tty_close(struct tty_struct *tty, struct file *filp)
{
	tty_port_close(tty->port, tty, filp);
}

static int ip35_l1_tty_write(struct tty_struct *tty, const unsigned char *buf,
			     int count)
{
	struct l1_port *l1_port = tty->driver_data;

	return ip35_l1_write(l1_port->chan, buf, count);
}

static unsigned int ip35_l1_tty_write_room(struct tty_struct *tty)
{
	struct l1_port *l1_port = tty->driver_data;
	struct circ_buf *circ = &l1_port->chan->l1->tx_circ;

	return CIRC_SPACE(circ->head, circ->tail, L1_XMIT_SIZE);
}

static const struct tty_operations ip35_l1_tty_ops = {
	.open = ip35_l1_tty_open,
	.close = ip35_l1_tty_close,
	.write = ip35_l1_tty_write,
	.write_room = ip35_l1_tty_write_room,
};

static int ip35_l1_port_activate(struct tty_port *port, struct tty_struct *ttys)
{
	struct l1_port *l1_port = container_of(port, struct l1_port, port);

	ttys->driver_data = l1_port;
	return 0;
}

static const struct tty_port_operations ip35_l1_port_ops = {
	.activate = ip35_l1_port_activate,
	// .shutdown = ip35_l1_port_shutdown,
};

static int ip35_l1_tty_create(struct platform_device *pdev, struct l1_ctrl *l1)
{
	struct l1_port *l1_port;
	struct tty_driver *tty;
	struct l1_chan *chan;
	int ret;

	tty = tty_alloc_driver(1, TTY_DRIVER_DYNAMIC_DEV | TTY_DRIVER_REAL_RAW);
	if (tty == NULL)
		return -ENOMEM;

	l1_port = devm_kmalloc(&pdev->dev, sizeof(*l1_port), GFP_KERNEL);
	if (!l1_port)
		return -ENOMEM;

	tty->driver_name = "ip35-l1";
	tty->name = "ttySC";
	tty->minor_start = 0;
	tty->major = 0;
	tty->type = TTY_DRIVER_TYPE_SERIAL;
	tty->subtype = SERIAL_TYPE_NORMAL;
	tty->init_termios = tty_std_termios;

	tty_set_operations(tty, &ip35_l1_tty_ops);

	ret = tty_register_driver(tty);
	if (ret) {
		tty_driver_kref_put(tty);
		return ret;
	}

	chan = &l1->channels[4]; // XXX

	tty_port_init(&l1_port->port);
	l1_port->port.ops = &ip35_l1_port_ops;
	l1_port->chan = chan;
	tty_port_register_device(&l1_port->port, tty, 0, &pdev->dev);

	chan->push_msg = ip35_l1_tty_push_msg; 
	chan->priv = l1_port;
	chan->l1 = l1;
	chan->type_chan = BRL1_EVENT | SC_CONS_SYSTEM;

	return 0;
}

#define LOCAL_GENERAL(x) ((x << 28) | (0x3ff << 18) | (0x3f << 12) | 0x06)

static void ip35_put_cmd_u16(u8 *buf, u16 val)
{
	*buf++ = (val >>  8) & 0xff;
	*buf++ = (val >>  0) & 0xff;
}

static void ip35_put_cmd_u32(u8 *buf, u32 val)
{
	*buf++ = (val >> 24) & 0xff;
	*buf++ = (val >> 16) & 0xff;
	*buf++ = (val >>  8) & 0xff;
	*buf++ = (val >>  0) & 0xff;
}

static void ip35_put_cmd_type_u32(u8 *buf, u32 val)
{
	*buf++ = 0;
	ip35_put_cmd_u32(buf, val);
}

#define L1_REQ_EEPROM		0x0006
#define L1_EEP_BOARD		2

int ip35_l1_get_board_eeprom(struct l1_ctrl *l1, u32 addr, int idx)
{
	struct l1_chan *chan;
	u8 cmd[64];
	int len;

	chan = &l1->channels[16];

	init_completion(&chan->comp);

	chan->push_msg = ip35_l1_cmd_push_msg;
	chan->type_chan = BRL1_REQUEST | 16;
	chan->l1 = l1;

	ip35_put_cmd_u32(&cmd[0], addr);
	ip35_put_cmd_u16(&cmd[4], L1_REQ_EEPROM);
	cmd[6] = 4;
	ip35_put_cmd_type_u32(&cmd[7], idx);
	ip35_put_cmd_type_u32(&cmd[12], L1_EEP_BOARD);
	ip35_put_cmd_type_u32(&cmd[17], 0);
	ip35_put_cmd_type_u32(&cmd[22], 96);

	len = 27;

	pr_info("sending eeprom command\n");

	ip35_l1_write(chan, cmd, len);

	wait_for_completion(&chan->comp);
	pr_info("got answer len %ld\n", chan->resp_len);
	if (chan->resp_data)
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1, chan->resp_data, chan->resp_len, true);

	kfree(chan->resp_data);
	chan->resp_data = NULL;

	return 0;
}

static int ip35_l1_cmd(struct l1_ctrl *l1) // XXX
{
	struct l1_chan *chan;
	u8 cmd[64];
	int len;

	chan = &l1->channels[16];

	init_completion(&chan->comp);

	chan->push_msg = ip35_l1_cmd_push_msg;
	chan->type_chan = BRL1_REQUEST | 16;
	chan->l1 = l1;

#if 0
#define REQ_CODE	0x0008

	ip35_put_cmd_u32(&cmd[0], LOCAL_GENERAL(0));
	ip35_put_cmd_u16(&cmd[4], REQ_CODE);
	len = 6;
	ip35_put_cmd_type_u32(&cmd[7], 1);
#else
#define REQ_CODE	0x0006

	ip35_put_cmd_u32(&cmd[0], LOCAL_GENERAL(0));
	ip35_put_cmd_u16(&cmd[4], REQ_CODE);
	cmd[6] = 4;
	ip35_put_cmd_type_u32(&cmd[7], 0);
	ip35_put_cmd_type_u32(&cmd[12], 2); // EEP_BOARD
	ip35_put_cmd_type_u32(&cmd[17], 0);
	ip35_put_cmd_type_u32(&cmd[22], 96);

	len = 27;
#endif

	pr_info("sending command\n");

	ip35_l1_write(chan, cmd, len);

	wait_for_completion(&chan->comp);
	pr_info("got answer len %ld\n", chan->resp_len);
	if (chan->resp_data)
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1, chan->resp_data, chan->resp_len, true);

	kfree(chan->resp_data);
	chan->resp_data = NULL;

	return 0;
}


#define L1_NASID	0	// XXX

#define HSPEC_SYNERGY0_0	0x04000000	// XXX move
#define HS_NON_CBRICK_SHFT 	12
#define HS_NON_CBRICK_MASK	(0xf << HS_NON_CBRICK_SHFT)

#define HS_PEBRICK_BITS		(0xc << HS_NON_CBRICK_SHFT)/*PE-brick*/
#define HS_SN10_BITS		(0x8 << HS_NON_CBRICK_SHFT)/*speedo2 */
#define HS_SN11_BITS		(0x4 << HS_NON_CBRICK_SHFT)/*Asterix */
#define HS_IP53_BITS		(0x2 << HS_NON_CBRICK_SHFT)/*IP53    */

static void ip35_get_part_num(struct l1_ctrl *l1, char *partnum)
{
#if 0
	u8 eeprom[256];

	ip35_read_eeprom(l1, L1_EEP_LOGIC, eeprom);
#endif
	ip35_l1_cmd(l1);
}

static int ip35_l1_discover(void *data)
{
	struct l1_ctrl *l1 = data;
	char partnum[16];
	u64 synergy0;

	// discover brick type and create nvmem stuff

	// check for mac addresses and create nvmem stuff

	// done
	synergy0 = readq((u64 *)REMOTE_HSPEC_ADDR(L1_NASID, HSPEC_SYNERGY0_0));
	pr_info("l1 discover synergy0 %llx\n", synergy0);

	switch (synergy0 & HS_NON_CBRICK_MASK) {
		case HS_IP53_BITS:
			pr_info("IP53 detected\n");
			break;
		case HS_SN11_BITS:
			pr_info("Fuel detected\n");
			break;
		case HS_SN10_BITS:
			pr_info("Origin 300 detected\n");
			break;
		case HS_PEBRICK_BITS:
			pr_info("PE-Brick detected\n");
			break;
		default:
			pr_info("unknown brick so far\n");
			break;
	}
	ip35_get_part_num(l1, partnum);

	return 0;
}

static int ip35_l1_probe(struct platform_device *pdev)
{
	struct l1_ctrl *l1;
	int irq, ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
#if 0
	l1->regs = devm_platform_ioremap_resource(pdev, 0);
	if (!l1->regs)
		return -ENOMEM;
#endif

	l1 = devm_kzalloc(&pdev->dev, sizeof(*l1), GFP_KERNEL);
	if (!l1)
		return -ENOMEM;

	l1->regs = (u64 *)REMOTE_HSPEC_ADDR(0, 0x80);
	l1->tx_circ.buf = l1->tx_buf;

	ret = devm_request_irq(&pdev->dev, irq, ip35_l1_interrupt, 0,
			       "ip35-l1", l1);
	if (ret < 0)
		return ret;

	ip35_l1_tty_create(pdev, l1);
	ip35_l1_start(l1);

	kthread_run(ip35_l1_discover, l1, "l1 discover");

	return 0;
}

static int ip35_l1_remove(struct platform_device *pdev)
{
	// XXX, needed at all ?
	return 0;
}

static struct platform_driver ip35_l1_driver = {
	.probe  = ip35_l1_probe,
	.remove = ip35_l1_remove,
	.driver = {
		.name = "ip35-l1",
	}
};
builtin_platform_driver(ip35_l1_driver);

static int __init ip35_l1_init(void)
{
	struct resource ip35_l1_res[1];
	struct irq_alloc_info info;
	struct irq_domain *domain;
	int irq;

	domain = irq_get_default_host();
	if (!domain)
		return -ENODEV;

	info.ctrl = NULL;
	info.nasid = L1_NASID;
	info.pin = 0;
	info.bit = UART_INTR;

	irq = irq_domain_alloc_irqs(domain, 1, L1_NASID, &info);
	if (irq < 0)
		return irq;

	memset(ip35_l1_res, 0, sizeof(ip35_l1_res));
#if 0
	ip35_l1_res[1].start = REMOTE_HSPEC_ADDR(L1_NASID, 0x80);
	ip35_l1_res[1].end = ip35_l1_res[0].start + 0x80;
	ip35_l1_res[1].flags = IORESOURCE_MEM;
#endif
	ip35_l1_res[0].start = irq;
	ip35_l1_res[0].end = irq;
	ip35_l1_res[0].flags = IORESOURCE_IRQ;

	platform_device_register_simple("ip35-l1", PLATFORM_DEVID_AUTO,
					ip35_l1_res, ARRAY_SIZE(ip35_l1_res));

	return 0;
}
fs_initcall(ip35_l1_init);

#include <linux/crc16.h>
#include <linux/nvmem-provider.h>

struct ip35_nvmem {
	unsigned int size;
	char nvmem_name[32];
	u8 data[64];
};

static int ip35_nvmem_read(void *priv, unsigned int off, void *buf,
			  size_t count)
{
	struct ip35_nvmem *nv = priv;

	if (off > nv->size)
		return -EINVAL;

	if ((off + count) > nv->size)
		count = nv->size - off;

	memcpy(buf, &nv->data[off], count);
	return 0;
}

static void ip35_nvmem_create(struct ip35_nvmem *nv, char *pdev_name,
			      unsigned int size)
{
	struct platform_device *pdev;
	struct nvmem_device *nvmem;
	struct nvmem_config nvmem_cfg = {
		.reg_read = ip35_nvmem_read,
		.type = NVMEM_TYPE_OTP,
		.read_only = true,
		.word_size = 1,
		.priv = nv,
		.id = -1
	};

	pdev = platform_device_alloc(pdev_name, -1);
	platform_device_add(pdev);

	nvmem_cfg.dev = &pdev->dev;
	nvmem_cfg.name = nv->nvmem_name;
	nvmem_cfg.size = size;
	nv->size = size;

	nvmem = nvmem_register(&nvmem_cfg);
}

static int __init ip35_nvmem_setup(void)
{
	struct ip35_nvmem *nv;
	char partnum[] = "030-1707-004";
	u16 crc;

	nv = kzalloc(sizeof(struct ip35_nvmem), GFP_KERNEL);
	strcpy(nv->nvmem_name, "bridge-00000f000000-0b-000000000000");
	memset(&nv->data[11], ' ', 19);
	memset(&nv->data[32], ' ', 6);
	memcpy(&nv->data[11], partnum, strlen(partnum));
	crc = ~crc16(0, nv->data, 30);
	nv->data[30] = crc & 0xff;
	nv->data[31] = crc >> 8;
	crc = ~crc16(0, &nv->data[32], 30);
	nv->data[62] = crc & 0xff;
	nv->data[63] = crc >> 8;
	ip35_nvmem_create(nv, "ip35-nvmem-1", 64);

	nv = kzalloc(sizeof(struct ip35_nvmem), GFP_KERNEL);
	strcpy(nv->nvmem_name, "ioc3-00000f800000-09-000000000000");
	nv->data[0] = 0x0a;
	nv->data[10] = 0x08;
	nv->data[9] = 0x00;
	nv->data[8] = 0x69;
	nv->data[7] = 0x10;
	nv->data[6] = 0x65;
	nv->data[5] = 0xc2;
	crc = ~crc16(0, nv->data, 11);
	nv->data[11] = crc & 0xff;
	nv->data[12] = crc >> 8;
	ip35_nvmem_create(nv, "ip35-nvmem-2", 16);

	nv = kzalloc(sizeof(struct ip35_nvmem), GFP_KERNEL);
	strcpy(nv->nvmem_name, "bridge-00000f800000-0b-000000000000");
	memset(&nv->data[11], ' ', 19);
	memset(&nv->data[32], ' ', 6);
	memcpy(&nv->data[11], partnum, strlen(partnum));
	crc = ~crc16(0, nv->data, 30);
	nv->data[30] = crc & 0xff;
	nv->data[31] = crc >> 8;
	crc = ~crc16(0, &nv->data[32], 30);
	nv->data[62] = crc & 0xff;
	nv->data[63] = crc >> 8;
	ip35_nvmem_create(nv, "ip35-nvmem-3", 64);


	return 0;
}

fs_initcall(ip35_nvmem_setup);
