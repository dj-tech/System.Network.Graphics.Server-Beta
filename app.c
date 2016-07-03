#include <linux/input.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <asm/irq.h> 
#include <asm/io.h> 
static struct input_dev *button_dev; 
static irqreturn_t button_interrupt(int irq, void *dummy) { input_report_key(button_dev, BTN_0, inb(BUTTON_PORT) & 1); input_sync(button_dev); 
return IRQ_HANDLED; }
 static int __init button_init(void) { int error; if (request_irq(BUTTON_IRQ, button_interrupt, 0, "button", NULL)) { printk(KERN_ERR "button.c: Can't allocate irq %d\n", button_irq);
 return -EBUSY; } button_dev = input_allocate_device();
 if (!button_dev) { printk(KERN_ERR "button.c: Not enough memory\n");
error = -ENOMEM; goto err_free_irq; } button_dev->evbit[0] = BIT_MASK(EV_KEY);
 button_dev->keybit[BIT_WORD(BTN_0)] = BIT_MASK(BTN_0);
error = input_register_device(button_dev);
 if (error) { printk(KERN_ERR "button.c: Failed to register device\n");
 goto err_free_dev; } return 0; 
err_free_dev: input_free_device(button_dev); 
err_free_irq: free_irq(BUTTON_IRQ, button_interrupt);
 return error; } static void __exit button_exit(void) { input_unregister_device(button_dev);
 free_irq(BUTTON_IRQ, button_interrupt); 
} module_init(button_init);
 module_exit(button_exit);
/*
 * livepatch-sample.c - Kernel Live Patching Sample Module
 *
 * Copyright (C) 2014 Seth Jennings <sjenning@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/livepatch.h>

/*
 * This (dumb) live patch overrides the function that prints the
 * kernel boot cmdline when /proc/cmdline is read.
 *
 * Example:
 *
 * $ cat /proc/cmdline
 * <your cmdline>
 *
 * $ insmod livepatch-sample.ko
 * $ cat /proc/cmdline
 * this has been live patched
 *
 * $ echo 0 > /sys/kernel/livepatch/livepatch_sample/enabled
 * $ cat /proc/cmdline
 * <your cmdline>
 */

#include <linux/seq_file.h>
static int livepatch_cmdline_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", "this has been live patched");
	return 0;
}

static struct klp_func funcs[] = {
	{
		.old_name = "cmdline_proc_show",
		.new_func = livepatch_cmdline_proc_show,
	}, { }
};

static struct klp_object objs[] = {
	{
		/* name being NULL means vmlinux */
		.funcs = funcs,
	}, { }
};

static struct klp_patch patch = {
	.mod = THIS_MODULE,
	.objs = objs,
};

static int livepatch_init(void)
{
	int ret;

	ret = klp_register_patch(&patch);
	if (ret)
		return ret;
	ret = klp_enable_patch(&patch);
	if (ret) {
		WARN_ON(klp_unregister_patch(&patch));
		return ret;
	}
	return 0;
}

static void livepatch_exit(void)
{
	WARN_ON(klp_disable_patch(&patch));
	WARN_ON(klp_unregister_patch(&patch));
}

module_init(livepatch_init);
module_exit(livepatch_exit);
MODULE_LICENSE("GPL");
MODULE_INFO(livepatch, "Y");
