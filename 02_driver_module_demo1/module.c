#include <linux/kernel.h>
#include <linux/module.h>

/**函数入口要执行的函数 */
static int __init testInit(void)
{
    printk("<6>Hello world\n");
    return 0;
}

/**函数出口要执行的函数 */
static void __exit testExit(void)
{
    printk("<6>good bye\n");
}

/**指定函数入口和出口函数 */
module_init(testInit);
module_exit(testExit);

/**声明内核模块的描述信息 */
MODULE_AUTHOR("123456@qq.com");
MODULE_DESCRIPTION("first driver kernel module!");
MODULE_LICENSE("GPL");
MODULE_VERSION("20250830_V1.0");



