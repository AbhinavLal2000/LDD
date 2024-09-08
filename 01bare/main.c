#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Abhinav");
MODULE_DESCRIPTION("Bare Minimum Driver");

static int __init bare_init(void)
{
    printk("Init bare module\r\n");
    return 0;
}

static void __exit bare_exit(void)
{
    printk("Exit bare module\r\n");
}

module_init(bare_init);
module_exit(bare_exit);

