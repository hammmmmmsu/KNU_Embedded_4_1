#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>

#define IOM_LED_MAJOR 260
#define IOM_LED_NAME "fpga_led"
#define IOM_LED_ADDRESS 0x016

extern unsigned char iom_fpga_itf_read(unsigned int addr);
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned char value);

int iom_led_open(struct inode *inode, struct file *filp) {
    return 0;
}

int iom_led_release(struct inode *inode, struct file *filp) {
    return 0;
}

ssize_t iom_led_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    unsigned char value;
    copy_from_user(&value, buf, 1);
    iom_fpga_itf_write((unsigned int)IOM_LED_ADDRESS, value);
    return count;
}

ssize_t iom_led_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    unsigned char value;
    value = iom_fpga_itf_read((unsigned int)IOM_LED_ADDRESS);
    copy_to_user(buf, &value, count);
    return count;
}

static struct file_operations iom_led_fops = {
    .read = iom_led_read,
    .write = iom_led_write,
    .open = iom_led_open,
    .release = iom_led_release,
};

int iom_led_init(void) {
    int result;
    result = register_chrdev(IOM_LED_MAJOR, IOM_LED_NAME, &iom_led_fops);
    if (result < 0) {
        printk(KERN_WARNING "Can't get any major\n");
        return result;
    }
    printk("init module, %s major number %d\n", IOM_LED_NAME, IOM_LED_MAJOR);
    return 0;
}

void iom_led_exit(void) {
    unregister_chrdev(IOM_LED_MAJOR, IOM_LED_NAME);
}

module_init(iom_led_init);
module_exit(iom_led_exit);
MODULE_LICENSE("GPL");
