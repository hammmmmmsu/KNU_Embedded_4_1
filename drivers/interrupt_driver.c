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
#define DEBOUNCE_MS         200

static int irq_num = -1;
static unsigned int press_count;
static unsigned long last_jiffies;

static irqreturn_t yellow_button_irq(int irq, void *dev_id)
{
    unsigned long now = jiffies;
    int level = gpio_get_value(GPIO_SW);

    if (time_before(now, last_jiffies + msecs_to_jiffies(DEBOUNCE_MS)))
        return IRQ_HANDLED;

    last_jiffies = now;
    press_count++;

    printk(KERN_INFO "fpga_interrupt: accepted count=%u level=%d\n",
           press_count, level);

    return IRQ_HANDLED;
}

static ssize_t interrupt_read(struct file *file, char __user *buf,
                              size_t count, loff_t *ppos)
{
    if (count < sizeof(press_count))
        return -EINVAL;

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
    if (ret < 0) {
        printk(KERN_ERR "fpga_interrupt: gpio_request failed (%d)\n", ret);
        return ret;
    }

    ret = gpio_direction_input(GPIO_SW);
    if (ret < 0) {
        printk(KERN_ERR "fpga_interrupt: gpio_direction_input failed (%d)\n", ret);
        gpio_free(GPIO_SW);
        return ret;
    }

    ret = gpio_set_debounce(GPIO_SW, DEBOUNCE_MS);
    if (ret < 0)
        printk(KERN_WARNING "fpga_interrupt: gpio_set_debounce failed (%d)\n", ret);

    irq_num = gpio_to_irq(GPIO_SW);
    if (irq_num < 0) {
        printk(KERN_ERR "fpga_interrupt: gpio_to_irq failed (%d)\n", irq_num);
        gpio_free(GPIO_SW);
        return irq_num;
    }

    ret = request_irq(irq_num, yellow_button_irq,
                      IRQF_TRIGGER_RISING,
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

    printk(KERN_INFO "fpga_interrupt: loaded major=%d gpio=%d irq=%d level=%d\n",
           INTERRUPT_MAJOR, GPIO_SW, irq_num, gpio_get_value(GPIO_SW));
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
