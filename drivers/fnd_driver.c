#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>

#define IOM_FND_MAJOR   261
#define IOM_FND_NAME    "fpga_fnd"
#define IOM_FND_ADDRESS 0x030   /* 4 consecutive registers: digit0..digit3 */

extern unsigned char iom_fpga_itf_read(unsigned int addr);
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned char value);

int iom_fnd_open(struct inode *inode, struct file *filp) { return 0; }
int iom_fnd_release(struct inode *inode, struct file *filp) { return 0; }

ssize_t iom_fnd_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    return 0;
}

/*
 * Write protocol:
 *   count == 4  → buf[0..3] = BCD digits left to right (0-9, 0xF=blank)
 *   count == 1  → display value 0-99 in rightmost two digits
 */
ssize_t iom_fnd_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    unsigned char digits[4] = {0x0F, 0x0F, 0x00, 0x00};
    int i;

    if (count >= 4) {
        if (copy_from_user(digits, buf, 4))
            return -EFAULT;
    } else if (count == 1) {
        unsigned char val;
        if (copy_from_user(&val, buf, 1))
            return -EFAULT;
        digits[0] = 0x0F;
        digits[1] = 0x0F;
        digits[2] = (val / 10) % 10;
        digits[3] =  val       % 10;
    }

    for (i = 0; i < 4; i++)
        iom_fpga_itf_write(IOM_FND_ADDRESS + i, digits[i] & 0x0F);

    return count;
}

static struct file_operations iom_fnd_fops = {
    .read    = iom_fnd_read,
    .write   = iom_fnd_write,
    .open    = iom_fnd_open,
    .release = iom_fnd_release,
};

int iom_fnd_init(void)
{
    int result = register_chrdev(IOM_FND_MAJOR, IOM_FND_NAME, &iom_fnd_fops);
    if (result < 0)
        printk(KERN_WARNING "fpga_fnd: can't get major %d\n", IOM_FND_MAJOR);
    return result;
}

void iom_fnd_exit(void)
{
    unregister_chrdev(IOM_FND_MAJOR, IOM_FND_NAME);
}

module_init(iom_fnd_init);
module_exit(iom_fnd_exit);
MODULE_LICENSE("GPL");
