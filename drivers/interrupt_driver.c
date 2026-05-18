#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>

#define GPIO_SW             27
#define INTERRUPT_MAJOR     264
#define INTERRUPT_NAME      "fpga_interrupt"
#define DEBOUNCE_MS         80

static int irq_num = -1;
static unsigned int press_count;
static unsigned long last_jiffies;

static irqreturn_t yellow_button_irq(int irq, void *dev_id)
{
    unsigned long now = jiffies;
    if (time_before(now, last_jiffies + msecs_to_jiffies(DEBOUNCE_MS)))
        return IRQ_HANDLED;

    last_jiffies = now;
    press_count++;
    printk(KERN_INFO "fpga_interrupt: count=%u level=%d\n",
           press_count, gpio_get_value(GPIO_SW));
    return IRQ_HANDLED;
}

static ssize_t interrupt_read(struct file *file, char __user *buf,
                              size_t count, loff_t *ppos)
{
    if (copy_to_user(buf, &press_count, sizeof(press_count)))
        return -EFAULT;
    return sizeof(press_count);
}

static struct file_operations interrupt_fops = {
    .owner = THIS_MODULE,
    .read  = interrupt_read,
};

static int __init interrupt_init(void)
{
    int ret;

    ret = gpio_request(GPIO_SW, INTERRUPT_NAME);
    if (ret < 0)
        printk(KERN_WARNING "fpga_interrupt: gpio_request failed (%d), continuing\n", ret);

    gpio_direction_input(GPIO_SW);
    irq_num = gpio_to_irq(GPIO_SW);

    /*
     * The breadboard circuit idles low, then rises when pressed.
     * Previous lab code used FALLING only, which misses that press edge.
     */
    ret = request_irq(irq_num, yellow_button_irq, IRQF_TRIGGER_RISING,
                      INTERRUPT_NAME, NULL);
    if (ret < 0) {
        printk(KERN_ERR "fpga_interrupt: request_irq failed (%d)\n", ret);
        gpio_free(GPIO_SW);
        return ret;
    }

    ret = register_chrdev(INTERRUPT_MAJOR, INTERRUPT_NAME, &interrupt_fops);
    if (ret < 0) {
        free_irq(irq_num, NULL);
        gpio_free(GPIO_SW);
        printk(KERN_ERR "fpga_interrupt: register_chrdev failed (%d)\n", ret);
        return ret;
    }

    printk(KERN_INFO "fpga_interrupt: loaded major=%d gpio=%d irq=%d\n",
           INTERRUPT_MAJOR, GPIO_SW, irq_num);
    return 0;
}

static void __exit interrupt_exit(void)
{
    unregister_chrdev(INTERRUPT_MAJOR, INTERRUPT_NAME);
    if (irq_num >= 0)
        free_irq(irq_num, NULL);
    gpio_free(GPIO_SW);
    printk(KERN_INFO "fpga_interrupt: unloaded\n");
}

module_init(interrupt_init);
module_exit(interrupt_exit);
MODULE_LICENSE("GPL");
