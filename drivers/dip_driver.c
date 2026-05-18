#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>

#define IOM_DIP_SWITCH_MAJOR 266
#define IOM_DIP_SWITCH_NAME "fpga_dip_switch"
#define IOM_DIP_SWITCH_ADDRESS 0x000

extern unsigned char iom_fpga_itf_read(unsigned int addr);
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned char value);

static int iom_dip_open(struct inode *inode, struct file *filp);
static int iom_dip_release(struct inode *inode, struct file *filp);
static ssize_t iom_dip_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t iom_dip_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

static struct file_operations iom_dip_fops = {
    .owner = THIS_MODULE,
    .read = iom_dip_read,
    .write = iom_dip_write,
    .open = iom_dip_open,
    .release = iom_dip_release,
};

static int iom_dip_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int iom_dip_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t iom_dip_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    return count;
}

static ssize_t iom_dip_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    unsigned char value;

    value = iom_fpga_itf_read((unsigned int)IOM_DIP_SWITCH_ADDRESS);

    if (copy_to_user(buf, &value, 1))
        return -EFAULT;

    return 1;
}

static int __init iom_dip_init(void)
{
    int result;

    result = register_chrdev(IOM_DIP_SWITCH_MAJOR, IOM_DIP_SWITCH_NAME, &iom_dip_fops);
    if (result < 0) {
        printk(KERN_WARNING "fpga_dip_switch: can't get major number\n");
        return result;
    }

    printk(KERN_INFO "fpga_dip_switch init, major = %d\n", IOM_DIP_SWITCH_MAJOR);
    return 0;
}

static void __exit iom_dip_exit(void)
{
    unregister_chrdev(IOM_DIP_SWITCH_MAJOR, IOM_DIP_SWITCH_NAME);
    printk(KERN_INFO "fpga_dip_switch exit\n");
}

module_init(iom_dip_init);
module_exit(iom_dip_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("FPGA DIP Switch Driver");
