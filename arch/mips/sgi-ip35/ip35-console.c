// SPDX-License-Identifier: GPL-2.0

#include <asm/sn/ksys/l1.h>
#include <linux/console.h>
#include <linux/init.h>

#define TTY_DRIVER_CONSOLE /* Console using parisc pdc_cons style driver */

void prom_putchar(char c)
{
	brl1_send(&c, 1, BRL1_EVENT | SC_CONS_SYSTEM);
}

static void l1_console_write(struct console *c, const char *s, unsigned int n)
{
	while (n-- && *s) {
		if (*s == '\n')
			prom_putchar('\r');
		prom_putchar(*s);
		s++;
	}
}

#ifdef TTY_DRIVER_CONSOLE
/*
 * From here through the definition of l1_console_device() cribbed
 * from parisc pdc_cons.c.
 */

#include <linux/tty.h>
#include <linux/tty_flip.h>

static struct tty_port tty_port;

static void l1_console_handle_packet(char *buffer, int length)
{
	int count;

	for (count = 1; count < length; count++)
		tty_insert_flip_char(&tty_port, buffer[count], TTY_NORMAL);

	tty_flip_buffer_push(&tty_port);
}

static int l1_console_tty_open(struct tty_struct *tty, struct file *filp)
{
	tty_port_tty_set(&tty_port, tty);
	l1_packet_handler_set(&l1_console_handle_packet, SC_CONS_SYSTEM);

	return 0;
}

static void l1_console_tty_close(struct tty_struct *tty, struct file *filp)
{
	if (tty->count == 1) {
		l1_packet_handler_clear(SC_CONS_SYSTEM);
		tty_port_tty_set(&tty_port, NULL);
	}
}

static int l1_console_tty_write(struct tty_struct *tty,
				const unsigned char *buf, int count)
{
	brl1_send(buf, count, BRL1_EVENT | SC_CONS_SYSTEM);
	return count;
}

static int l1_console_tty_write_room(struct tty_struct *tty)
{
	return 32768; /* no limit, no buffer used */
}

static int l1_console_tty_chars_in_buffer(struct tty_struct *tty)
{
	return 0; /* no buffer */
}

static const struct tty_operations l1_console_tty_ops = {
	.open            = l1_console_tty_open,
	.close           = l1_console_tty_close,
	.write           = l1_console_tty_write,
	.write_room      = l1_console_tty_write_room,
	.chars_in_buffer = l1_console_tty_chars_in_buffer,
};

static struct tty_driver *l1_console_tty_driver;

static int l1_console_tty_driver_init(void)
{
	int err;

	l1_console_tty_driver = alloc_tty_driver(1);

	if (!l1_console_tty_driver)
		return 0;

	tty_port_init(&tty_port);

	l1_console_tty_driver->driver_name = "ip35-console";
	l1_console_tty_driver->name = "l1";
	l1_console_tty_driver->major = MUX_MAJOR;
	l1_console_tty_driver->minor_start = 0;
	l1_console_tty_driver->type = TTY_DRIVER_TYPE_SYSTEM;
	l1_console_tty_driver->init_termios = tty_std_termios;
	l1_console_tty_driver->flags = TTY_DRIVER_REAL_RAW |
		TTY_DRIVER_RESET_TERMIOS;
	tty_set_operations(l1_console_tty_driver, &l1_console_tty_ops);
	tty_port_link_device(&tty_port, l1_console_tty_driver, 0);

	err = tty_register_driver(l1_console_tty_driver);
	if (err) {
		pr_err("Unable to register L1 console TTY driver\n");
		tty_port_destroy(&tty_port);

		return err;
	}

	return 0;
}

static struct tty_driver *l1_console_device(struct console *c, int *index)
{
	*index = c->index;
	return l1_console_tty_driver;
}
#endif

static struct console l1_console = {
	.name	= "l1",
	.write	= l1_console_write,
#if defined(TTY_DRIVER_CONSOLE)
	.device = l1_console_device,
#endif
	.index	= -1
};

static int __init ip35_setup_console(void)
{
	/*
	 * FIXME: Preferred console should depend on PROM variables.
	 */
	add_preferred_console("l1", 0, NULL);
	register_console(&l1_console);

return 0;
}

console_initcall(ip35_setup_console);
#if defined(TTY_DRIVER_CONSOLE)
arch_initcall(l1_console_tty_driver_init);
#endif
