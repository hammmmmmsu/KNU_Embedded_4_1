#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>

#define IOM_FPGA_DOT_MAJOR   262
#define IOM_FPGA_DOT_NAME    "fpga_dot"
#define IOM_FPGA_DOT_ADDRESS 0x210
#define DOT_ROW_COUNT        10

extern unsigned char iom_fpga_itf_read(unsigned int addr);
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned char value);

/* Predefined digit patterns (rows 0-9, 7 columns, bits 6:0) */
static unsigned char fpga_number[10][DOT_ROW_COUNT] = {
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x63,0x3E,0x00,0x00},
    {0x0C,0x1C,0x3C,0x0C,0x0C,0x0C,0x0C,0x3F,0x00,0x00},
    {0x3E,0x63,0x03,0x06,0x0C,0x18,0x30,0x7F,0x00,0x00},
    {0x3E,0x63,0x03,0x1E,0x03,0x03,0x63,0x3E,0x00,0x00},
    {0x06,0x0E,0x1E,0x36,0x66,0x7F,0x06,0x06,0x00,0x00},
    {0x7F,0x60,0x60,0x7E,0x03,0x03,0x63,0x3E,0x00,0x00},
    {0x1E,0x30,0x60,0x7E,0x63,0x63,0x63,0x3E,0x00,0x00},
    {0x7F,0x63,0x06,0x0C,0x18,0x18,0x18,0x18,0x00,0x00},
    {0x3E,0x63,0x63,0x3E,0x63,0x63,0x63,0x3E,0x00,0x00},
    {0x3E,0x63,0x63,0x63,0x3F,0x03,0x06,0x3C,0x00,0x00},
};

static void write_rows(const unsigned char *rows)
{
    int i;
    for (i = 0; i < DOT_ROW_COUNT; i++)
        iom_fpga_itf_write(IOM_FPGA_DOT_ADDRESS + i, rows[i] & 0x7F);
}

int iom_dot_open(struct inode *inode, struct file *filp) { return 0; }
int iom_dot_release(struct inode *inode, struct file *filp) { return 0; }

ssize_t iom_dot_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    return 0;
}

/*
 * Write protocol:
 *   count == 10  →  treat buf[0..9] as raw row pattern (direct pixel control)
 *   count == 1   →  treat buf[0] as digit index 0-9 (backward-compatible)
 */
ssize_t iom_dot_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    if (count == DOT_ROW_COUNT) {
        unsigned char pattern[DOT_ROW_COUNT];
        if (copy_from_user(pattern, buf, DOT_ROW_COUNT))
            return -EFAULT;
        write_rows(pattern);
    } else {
        unsigned char value;
        if (copy_from_user(&value, buf, 1))
            return -EFAULT;
        if (value > 9) value = 0;
        write_rows(fpga_number[value]);
    }
    return count;
}

static struct file_operations iom_dot_fops = {
    .read    = iom_dot_read,
    .write   = iom_dot_write,
    .open    = iom_dot_open,
    .release = iom_dot_release,
};

int iom_dot_init(void)
{
    return register_chrdev(IOM_FPGA_DOT_MAJOR, IOM_FPGA_DOT_NAME, &iom_dot_fops);
}

void iom_dot_exit(void)
{
    unregister_chrdev(IOM_FPGA_DOT_MAJOR, IOM_FPGA_DOT_NAME);
}

module_init(iom_dot_init);
module_exit(iom_dot_exit);
MODULE_LICENSE("GPL");
