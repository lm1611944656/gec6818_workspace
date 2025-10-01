/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: chrdev.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-09-08
 *   作    者: lium
 *   功    能: 通过GPIO口函数读取传感的数据
 *              传感器的数据有三类：
 *              1. 数字量信号，热释电传感器(PIR)。有人来产生一个高电平，没有人来是低电平
 *              2. 模拟量信号，ADC传感器
 *              3. 时序信号，DHT11传感器。
 ************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>      // copy_from_user
#include <linux/fs.h>           // 文件操作集
#include <linux/device.h>       // create_device
#include <linux/gpio.h>         // gpio口相关函数
#include <cfg_type.h>

#define BUF_SIZE    1                   // 读取到的PIR信号
static char g_kerner_buf[BUF_SIZE];

static unsigned int majorDevID = 0;     // 主设备号（动态分配）
static unsigned int minorDevID = 0;     // 次设备号
static struct cdev chrdev;
static dev_t dev_no;

struct class *pClassPIR;
struct device *pDevicePIR;

#define GPIOC25  (PAD_GPIO_C + 25)

unsigned int pir_arr[1] = {GPIOC25};

static int pir_open(struct inode *inode, struct file *pFile);
static int pir_close(struct inode *inode, struct file *pFile);
static ssize_t pir_read(struct file *filp, char __user *pBuff, size_t count, loff_t *ppos);

static const struct file_operations PIR_fops = {
    .owner      = THIS_MODULE,
    .open       = pir_open,
    .release    = pir_close,
    .read      = pir_read,
};

static int __init chrDevInit(void)
{
    int ret;

    /**1. 申请设备号(推荐使用动态注册) */
    if (majorDevID) {
        dev_no = MKDEV(majorDevID, minorDevID);
        ret = register_chrdev_region(dev_no, 1, "Sensor_Device");
    } else {
        ret = alloc_chrdev_region(&dev_no, minorDevID, 1, "Sensor_Device");
        majorDevID = MAJOR(dev_no);
    }
    if (ret < 0) {
        printk(KERN_ERR "Failed to register device number\n");
        return ret;
    }

    /**2. 字符设备初始化 */
    cdev_init(&chrdev, &PIR_fops);
    chrdev.owner = THIS_MODULE;

    /**3. 将cdev注册到linux内核。执行成功可以通过 cat /proc/devices  查看字符设备"Sensor_Device" */
    ret = cdev_add(&chrdev, dev_no, 1);
    if (ret < 0) {
        printk(KERN_ERR "cdev_add failed\n");
        goto err_cdev_add;
    }

    /**4. 在 /sys/class/ 下创建设备类 */
    pClassPIR = class_create(THIS_MODULE, "PIR_class");
    if (IS_ERR_OR_NULL(pClassPIR)) {
        printk(KERN_ERR "class_create failed\n");
        ret = PTR_ERR(pClassPIR);
        pClassPIR = NULL;
        goto err_class_create;
    }

    /**5. 在 /dev 下创建设备节点*/
    pDevicePIR = device_create(pClassPIR, NULL, dev_no, NULL, "PIR");
    if (IS_ERR_OR_NULL(pDevicePIR)) {
        printk(KERN_ERR "device_create failed\n");
        ret = PTR_ERR(pDevicePIR);
        pDevicePIR = NULL;
        goto err_device_create;
    }

    printk(KERN_INFO "PIR driver initialized successfully (major=%d)\n", majorDevID);
    return 0;

/**错误处理：反向释放资源 */
err_device_create:
    class_destroy(pClassPIR);
err_class_create:
    cdev_del(&chrdev);
err_cdev_add:
    unregister_chrdev_region(dev_no, 1);
    return ret;
}

static void __exit chrDevExit(void)
{
    if (pDevicePIR)
        device_destroy(pDevicePIR, dev_no);

    if (pClassPIR)
        class_destroy(pClassPIR);

    cdev_del(&chrdev);

    unregister_chrdev_region(dev_no, 1);

    printk(KERN_INFO "chrDevExit: PIR driver unloaded\n");
}

static int pir_open(struct inode *inode, struct file *pFile)
{
    int ret, i;
    for(i = 0; i < 1; i++){
        /**申请GPIO口来使用 */
        ret = gpio_request(pir_arr[i], "PIRIndex");
        if(ret < 0){
            printk(KERN_ERR "gpio_request failed! \n");
            return ret;
        }

        /**GPIO口设置为输入模式 */
        gpio_direction_input(pir_arr[i]);
    }
    printk(KERN_INFO "pir_open success! \n");
    return ret;
}

static int pir_close(struct inode *inode, struct file *pFile)
{
    int i;

    /**释放GPIO */
    for(i = 0; i < 1; i++){
        gpio_free(pir_arr[i]);
    }
    printk(KERN_INFO "PIR_close success! \n");
    return 0;
}

/**
 * @brief 读取热释电传感器的数据；
 * @param buf[0] 表示控制哪一盏灯
 * @param buf[1] 表示亮还什么灭
 */
static ssize_t pir_read(struct file *filp, char __user *pBuff, size_t count, loff_t *ppos)
{
    int gpio_val;

    if(count > BUF_SIZE){
       count = BUF_SIZE;    // 只能读取一个字节
    }

    if (count == 0){
        return 0;
    }

    /**读取GPIO口的电平 */ 
    gpio_val = gpio_get_value(GPIOC25);
    g_kerner_buf[0] = gpio_val + '0';    // 转为字符 '0' 或 '1'

    /**将读取到的数据传递给用户空将 */
    if(copy_to_user(pBuff, g_kerner_buf, count)){
        printk(KERN_ERR "copy_to_user failed! \n");
        return -EFAULT;
    }
    printk(KERN_INFO "kernel layer pir_read: read value '%c'\n", g_kerner_buf[0]);
    return count;  // 返回实际读取的字节数
}

module_init(chrDevInit);
module_exit(chrDevExit);

MODULE_AUTHOR("lium <123456@qq.com>");
MODULE_DESCRIPTION("PIR Control Driver using GPIOC[25]");
MODULE_LICENSE("GPL");
MODULE_VERSION("2025-09-08_V1.1");
/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-08-31, lium
 * describe: 初始创建.
 *************************************************************************/