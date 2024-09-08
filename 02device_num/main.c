#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Abhinav");
MODULE_DESCRIPTION("Device number");

#define DMAJOR 64

static int device_open(struct inode* device_file, struct file* instance)
{
    printk("Device open\r\n");
    return 0;
}

static int device_release(struct inode* device_file, struct file* instance)
{
    printk("Device released\r\n");
    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open  = device_open,
    .release = device_release
};

static int __init bare_init(void)
{
    int ret = 0;
    
    printk("Init device_number module\r\n");
    ret = register_chrdev(DMAJOR, "device_number", &fops); /* this driver will be visible under /proc/devices */
    if (ret == 0)
    {
        printk("device_number registered\r\n");
    }
    else if (ret > 0)
    {
        printk("device_number registered\r\n");
    }
    else
    {
        printk("device_number not registered\r\n");
        return -1;
    }

    return 0;
}

static void __exit bare_exit(void)
{
    unregister_chrdev(DMAJOR, "device_number");
    printk("Exit device_number module\r\n");
}

module_init(bare_init);
module_exit(bare_exit);

