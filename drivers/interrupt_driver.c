#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>

#define IOM_PUSH_MAJOR   263
#define IOM_PUSH_NAME    "fpga_push_switch"
#define IOM_PUSH_ADDRESS 0x082   /* 8-bit push switch register */

extern unsigned char iom_fpga_itf_read(unsigned int addr);
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned char value);

int iom_push_open(struct inode *inode, struct file *filp) { return 0; }
int iom_push_release(struct inode *inode, struct file *filp) { return 0; }

/*
 * Read: returns 1 byte bitmask of pressed buttons.
 * bit7 = SW1 (game start), bit0 = SW8.
 * Active-low: hardware returns 0 when pressed, driver inverts to 1=pressed.
 */
ssize_t iom_push_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    unsigned char value;
    value = ~iom_fpga_itf_read((unsigned int)IOM_PUSH_ADDRESS);
    if (copy_to_user(buf, &value, 1))
        return -EFAULT;
    return 1;
}

ssize_t iom_push_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    return count;
}

static struct file_operations iom_push_fops = {
    .read    = iom_push_read,
    .write   = iom_push_write,
    .open    = iom_push_open,
    .release = iom_push_release,
};

int iom_push_init(void)
{
    int result = register_chrdev(IOM_PUSH_MAJOR, IOM_PUSH_NAME, &iom_push_fops);
    if (result < 0)
        printk(KERN_WARNING "fpga_push_switch: can't get major %d\n", IOM_PUSH_MAJOR);
    printk(KERN_INFO "fpga_push_switch init, major = %d\n", IOM_PUSH_MAJOR);
    return result;
}

void iom_push_exit(void)
{
    unregister_chrdev(IOM_PUSH_MAJOR, IOM_PUSH_NAME);
    printk(KERN_INFO "fpga_push_switch exit\n");
}

module_init(iom_push_init);
module_exit(iom_push_exit);
MODULE_LICENSE("GPL");
