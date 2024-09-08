#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/device.h>
#include "oled_font.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Abhinav");
MODULE_DESCRIPTION("I2C based OLED display driver");

#define DRIVER_NAME     "oled_driver"
#define DRIVER_CLASS    "oled_driver_class"
#define I2C_DEVICE_BUS  1
#define I2C_DEVICE_NAME "OLED"
#define I2C_DEVICE_ADDR 0x3C
#define I2C_DEVICE_ID   0

enum omode {CMD, DATA};

static dev_t oled_dev;
static struct class* oled_class;
static struct cdev oled_cdev;

static struct i2c_adapter* oled_i2c_adapter = NULL;
static struct i2c_client* oled_i2c_client = NULL;

static const struct i2c_device_id oled_device_id[] = 
{
    {I2C_DEVICE_NAME, I2C_DEVICE_ID},
    {}
};

static struct i2c_driver oled_driver = 
{
    .driver = 
    {
        .name  = I2C_DEVICE_NAME,
        .owner = THIS_MODULE
    }
};

static struct i2c_board_info oled_board_info = 
{
    I2C_BOARD_INFO(I2C_DEVICE_NAME, I2C_DEVICE_ADDR)
};

int  oled_send(struct i2c_client* client, enum omode m, char data);
void oled_putc(struct i2c_client* client, char c);
void oled_puts(struct i2c_client* client, char* str);
void oled_clear(struct i2c_client* client);
void oled_cursor(struct i2c_client*, char row, char col);

static int device_open(struct inode* device_file, struct file* instance)
{
    printk("Device open\r\n");
    return 0;
}
    
static ssize_t device_read(struct file* file, char* user_buffer, size_t count, loff_t* offset)
{
    printk("device read\r\n");
    return 0;
}

static ssize_t device_write(struct file* file, const char* user_buffer, size_t count, loff_t* offset)
{
    char dbuffer[255];
    printk("device write\r\n");
    unsigned long copy_bytes = 0, not_copy_bytes = 0;
    /* get amount of data to copy */
    copy_bytes = min(count, sizeof(dbuffer));
    /* copy data from user */
    memset(dbuffer, 0, 255);
    not_copy_bytes = copy_from_user(dbuffer, user_buffer, copy_bytes);
    oled_clear(oled_i2c_client);
    oled_puts(oled_i2c_client, dbuffer);
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

int oled_send(struct i2c_client* client, enum omode m, char data)
{
    char buf[2] = {0x00, data};
    switch (m)
    {
        case CMD:  buf[0] = 0x00; break;
        case DATA: buf[0] = 0x40; break;
    }
    return i2c_master_send(client, buf, 2);
}

void oled_putc(struct i2c_client* client, char c)
{
    c -= 32;
    for (int i = 0; i < 5; i++)
    {
        oled_send(client, DATA, oled_font[c][i]); 
    }
    oled_send(client, DATA, 0x00);
}

void oled_puts(struct i2c_client* client, char* str)
{
    while (*str)
    {
        oled_putc(client, *str++);
    }
}

void oled_cursor(struct i2c_client* client, char row, char col)
{
    if (!((row <= 7) && (col <= 128))) return;

    oled_send(client, CMD, 0x21);
    oled_send(client, CMD, col);
    oled_send(client, CMD, 127);

    oled_send(client, CMD, 0x22);
    oled_send(client, CMD, row);
    oled_send(client, CMD, 7);
}

void oled_clear(struct i2c_client* client)
{
    for (int i = 0; i < (128*8); i++)
    {
        oled_send(client, DATA, 0x00);
    }
    oled_cursor(client, 0, 0);
}

void oled_init(struct i2c_client* client)
{
    char cmd[26] = {0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x00,0xA1,
                    0xC8,0xDA,0x12,0x81,0x80,0xD9,0xF1,0xDB,0x20,0xA4,0xA6,0x2E,0xAF};
    for (int i = 0; i < 26; i++)
    {
        oled_send(client, CMD, cmd[i]);
    }
    oled_clear(client);
    oled_puts(client, "Start");
}

static int __init oled_driver_init(void)
{
    /* allocate device minor number */
    if (alloc_chrdev_region(&oled_dev, 0, 1, DRIVER_NAME) < 0)
    {
        printk("device minor cannot be allocated\r\n");
        return -1;
    }

    /* create a device class  */
    if ((oled_class = class_create(DRIVER_CLASS)) == NULL)
    {
        printk("device class cannot be created\r\n");
        return -1;
    }

    /* creates a device and registers it with sysfs */
    if (device_create(oled_class, NULL, oled_dev, NULL, DRIVER_NAME) == NULL)
    {
        printk("cannot create the device class\r\n");
        return -1;
    }

    /* init device file with file operations */
    cdev_init(&oled_cdev, &fops);
    
    /* add register the device to the system */
    if (cdev_add(&oled_cdev, oled_dev, 1) < 0)
    {
        printk("unable to add/register chrdev to kernel\r\n");
        return -1;
    }

    oled_i2c_adapter = i2c_get_adapter(I2C_DEVICE_BUS);
    if (oled_i2c_adapter == NULL)
    {
        printk("unable to get i2c adapter\r\n");
        return -1;
    }
    oled_i2c_client = i2c_new_client_device(oled_i2c_adapter, &oled_board_info);
    if (oled_i2c_client == NULL)
    {
        printk("unable to get i2c client\r\n");
        return -1;
    }
    if (i2c_add_driver(&oled_driver) < 0)
    {
        printk("unable to register i2c driver\r\n");
        return -1;
    }
    i2c_put_adapter(oled_i2c_adapter); 
    printk("Init %s module\r\n", DRIVER_NAME);
    oled_init(oled_i2c_client);
    printk("Init oled\r\n");
    return 0;
}

static void __exit oled_driver_exit(void)
{
    i2c_unregister_device(oled_i2c_client);
    i2c_del_driver(&oled_driver);
    cdev_del(&oled_cdev);
    device_destroy(oled_class, oled_dev);
    class_destroy(oled_class);
    unregister_chrdev_region(oled_dev, 1);
    printk("Exit %s module\r\n", DRIVER_NAME);
}

module_init(oled_driver_init);
module_exit(oled_driver_exit);

