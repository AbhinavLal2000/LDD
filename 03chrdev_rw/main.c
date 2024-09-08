#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Abhinav");
MODULE_DESCRIPTION("chrdev with read/write ops");

#define DRIVER_NAME  "chrdev_rw"
#define DRIVER_CLASS "chrdev_rw_class"

static char buffer[255];
static size_t buf_ptr = 0;

static int device_open(struct inode* device_file, struct file* instance)
{
    printk("Device open\r\n");
    return 0;
}

static ssize_t device_read(struct file* file, char* user_buffer, size_t count, loff_t* offset)
{
    printk("device read\r\n");
    int copy_bytes = 0, not_copy_bytes = 0;
    /* get amount of data to copy */
    copy_bytes = min(count, buf_ptr);
    /* copy data to user */
    not_copy_bytes = copy_to_user(user_buffer, buffer, copy_bytes);
    /* Delta */
    return copy_bytes - not_copy_bytes;
}

static ssize_t device_write(struct file* file, const char* user_buffer, size_t count, loff_t* offset)
{ 
    printk("device write\r\n");
    unsigned long copy_bytes = 0, not_copy_bytes = 0;
    /* get amount of data to copy */
    copy_bytes = min(count, sizeof(buffer));
    /* copy data from user */
    not_copy_bytes = copy_from_user(buffer, user_buffer, copy_bytes);
    buf_ptr = copy_bytes;
    /* calculate data */
    return copy_bytes - not_copy_bytes;
}

static int device_release(struct inode* device_file, struct file* instance)
{
    printk("Device released\r\n");
    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open  = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write
};

static dev_t chrdevrw_dev;
static struct class* chrdevrw_class;
static struct cdev chrdevrw_cdev;

static int __init bare_init(void)
{
    /* allocate device minor number */
    if (alloc_chrdev_region(&chrdevrw_dev, 0, 1, DRIVER_NAME) < 0)
    {
        printk("device minor cannot be allocated\r\n");
        return -1;
    }

    /* create a device class  */
    if ((chrdevrw_class = class_create(DRIVER_CLASS)) == NULL)
    {
        printk("device class cannot be created\r\n");
        return -1;
    }

    /* creates a device and registers it with sysfs */
    if (device_create(chrdevrw_class, NULL, chrdevrw_dev, NULL, DRIVER_NAME) == NULL)
    {
        printk("cannot create the device class\r\n");
        return -1;
    }

    /* init device file with file operations */
    cdev_init(&chrdevrw_cdev, &fops);
    
    /* add register the device to the system */
    if (cdev_add(&chrdevrw_cdev, chrdevrw_dev, 1) < 0)
    {
        printk("unable to add/register chrdev to kernel\r\n");
        return -1;
    }

    printk("Init %s module\r\n", DRIVER_NAME);
    return 0;
}

static void __exit bare_exit(void)
{
    cdev_del(&chrdevrw_cdev);
    device_destroy(chrdevrw_class, chrdevrw_dev);
    class_destroy(chrdevrw_class);
    unregister_chrdev_region(chrdevrw_dev, 1);
    printk("Exit %s module\r\n", DRIVER_NAME);
}

module_init(bare_init);
module_exit(bare_exit);

