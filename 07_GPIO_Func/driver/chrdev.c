/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: chrdev.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-09-08
 *   作    者: lium
 *   功    能: 通过GPIO口函数控制LED灯
 *
 ************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>      // copy_from_user
#include <linux/fs.h>           // 文件操作集
#include <linux/device.h>       // create_device
#include <linux/gpio.h>         // gpio口相关函数
#include <cfg_type.h>

#define BUF_SIZE    2                   // 接收应用层数据的个数
static char dataBuf[BUF_SIZE];

static unsigned int majorDevID = 0;     // 主设备号（动态分配）
static unsigned int minorDevID = 0;     // 次设备号
static struct cdev chrdev;
static dev_t dev_no;

struct class *pClassLed;
struct device *pDeviceLed;

#define GPIOE13  (PAD_GPIO_E + 13)
#define GPIOC17  (PAD_GPIO_C + 17)
#define GPIOC8   (PAD_GPIO_C + 8)
#define GPIOC7   (PAD_GPIO_C + 7)

unsigned int led_gpio[4] = {GPIOE13, GPIOC17, GPIOC8, GPIOC7};

static int led_open(struct inode *inode, struct file *pFile);
static int led_close(struct inode *inode, struct file *pFile);
static ssize_t led_write(struct file *file, const char __user *buf, size_t len, loff_t *off);

static const struct file_operations led_fops = {
    .owner      = THIS_MODULE,
    .open       = led_open,
    .release    = led_close,
    .write      = led_write,
};

static int __init chrDevInit(void)
{
    int ret;

    /**1. 申请设备号(推荐使用动态注册) */
    if (majorDevID) {
        dev_no = MKDEV(majorDevID, minorDevID);
        ret = register_chrdev_region(dev_no, 1, "led_device");
    } else {
        ret = alloc_chrdev_region(&dev_no, minorDevID, 1, "led_device");
        majorDevID = MAJOR(dev_no);
    }
    if (ret < 0) {
        printk(KERN_ERR "Failed to register device number\n");
        return ret;
    }

    /**2. 字符设备初始化 */
    cdev_init(&chrdev, &led_fops);
    chrdev.owner = THIS_MODULE;

    /**3. 将cdev注册到linux内核 */
    ret = cdev_add(&chrdev, dev_no, 1);
    if (ret < 0) {
        printk(KERN_ERR "cdev_add failed\n");
        goto err_cdev_add;
    }

    /**4. 在 /sys/class/ 下创建设备类 */
    pClassLed = class_create(THIS_MODULE, "led_class");
    if (IS_ERR_OR_NULL(pClassLed)) {
        printk(KERN_ERR "class_create failed\n");
        ret = PTR_ERR(pClassLed);
        pClassLed = NULL;
        goto err_class_create;
    }

    /**5. 在 /dev 下创建设备节点*/
    pDeviceLed = device_create(pClassLed, NULL, dev_no, NULL, "LED4");
    if (IS_ERR_OR_NULL(pDeviceLed)) {
        printk(KERN_ERR "device_create failed\n");
        ret = PTR_ERR(pDeviceLed);
        pDeviceLed = NULL;
        goto err_device_create;
    }

    printk(KERN_INFO "LED driver initialized successfully (major=%d)\n", majorDevID);
    return 0;

/**错误处理：反向释放资源 */
err_device_create:
    class_destroy(pClassLed);
err_class_create:
    cdev_del(&chrdev);
err_cdev_add:
    unregister_chrdev_region(dev_no, 1);
    return ret;
}

static void __exit chrDevExit(void)
{
    if (pDeviceLed)
        device_destroy(pClassLed, dev_no);

    if (pClassLed)
        class_destroy(pClassLed);

    cdev_del(&chrdev);

    unregister_chrdev_region(dev_no, 1);

    printk(KERN_INFO "chrDevExit: LED driver unloaded\n");
}

static int led_open(struct inode *inode, struct file *pFile)
{
    int ret, i;
    for(i = 0; i < 4; i++){
        /**申请GPIO口来使用 */
        ret = gpio_request(led_gpio[i], "LedIndex");
        if(ret < 0){
            printk(KERN_ERR "gpio_request failed! \n");
            return ret;
        }

        /**GPIO口设置为输出模式，并且默认为高电平 */
        gpio_direction_output(led_gpio[i], 1);
    }
    printk(KERN_INFO "led_open success! \n");
    return ret;
}

static int led_close(struct inode *inode, struct file *pFile)
{
    int i;

    /**释放GPIO */
    for(i = 0; i < 4; i++){
        gpio_free(led_gpio[i]);
    }
    printk(KERN_INFO "led_close success! \n");
    return 0;
}

/**
 * @brief 控制LED灯；
 * @param buf[0] 表示控制哪一盏灯
 * @param buf[1] 表示亮还什么灭
 */
static ssize_t led_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    int ret, i;
    int ledNum;     // 那一盏灯
    int status;     // 灯的状状态
    size_t copy_len = len;

    if (copy_len > BUF_SIZE)
    {
        copy_len = BUF_SIZE;
    }

    /**将用户数据拷贝到缓冲区 */
    ret = copy_from_user(dataBuf, buf, copy_len);
    if (ret != 0) {
        printk(KERN_ERR "led_write: copy_from_user failed, %d bytes not copied\n", ret);
        return -EFAULT;
    }

    /**打印接收到的数据 */
    printk(KERN_INFO "led_write: received %zu bytes: %.*s\n", copy_len, (int)copy_len, dataBuf);
    for (i = 0; i < copy_len; i++) {
        printk(KERN_INFO "received [%d]: '%c' (0x%02X)\n", i, dataBuf[i], dataBuf[i]);
    }

    /**控制LED的逻辑 */
    ledNum = dataBuf[0] - '0';
    status = dataBuf[1] - '0';
    gpio_set_value(led_gpio[ledNum], status);
    return 0;
}

module_init(chrDevInit);
module_exit(chrDevExit);

MODULE_AUTHOR("lium <123456@qq.com>");
MODULE_DESCRIPTION("LED Control Driver using GPIOE[13]");
MODULE_LICENSE("GPL");
MODULE_VERSION("2025-09-08_V1.1");
/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-08-31, lium
 * describe: 初始创建.
 *************************************************************************/