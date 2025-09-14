/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: chrdev.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-08-31
 *   作    者: lium
 *   功    能: 应用层向驱动层的字符设备传递参数
 *
 ************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/device.h>


#define BUF_SIZE    5                   // 接收应用层数据的个数
static char dataBuf[BUF_SIZE];

static unsigned int majorDevID = 0;     // 主设备号
static unsigned int minorDevID = 0;     // 次设备号
static struct cdev chrdev;
static dev_t dev_no;

struct class *pClassLed;
struct device *pDeviceLed;

static int led_open(struct inode *inode, struct file *pFile)
{
    /**硬件初始化 */
    printk("<6> Led Open! \n");
    return 0;
}

static int led_close(struct inode *inode, struct file *pFile)
{
    /**恢复硬件寄存器中的内容 */
    printk("<6> Led close! \n");
    return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t len, loff_t *off) 
{
    int ret;
    int i;

    // 限制写入长度，防止溢出
    if (len > BUF_SIZE) {
        len = BUF_SIZE;
    }

    // 从用户空间复制数据到内核空间
    ret = copy_from_user(dataBuf, buf, len);
    if (ret != 0) {
        printk(KERN_ERR "led_write: copy_from_user failed, %d bytes not copied\n", ret);
        return -EFAULT;
    }
    printk(KERN_INFO "led_write: received %zu bytes: %.*s\n", len, (int)len, dataBuf);
    for(i = 0; i < len; i++){
        printk(KERN_INFO "received [%d]: %d\n", i, *(dataBuf + i));
    }

    return len; // 返回成功写入的字节数
}

/**文件操作集 */
static const struct file_operations led_fops = {
    .owner      = THIS_MODULE,
    .open       = led_open,
    .release    = led_close,
    .write      = led_write
};

static int __init chrDevInit(void)
{
    int regResult;

    /**1. 申请设备号 */
    dev_no = MKDEV(majorDevID, minorDevID);     // 创建一个设备号
    if(dev_no > 0){
        regResult = register_chrdev_region(dev_no, 1, "DevNameTest");
    }else{
        regResult = alloc_chrdev_region(&dev_no, minorDevID, 1, "DevNameTest");
    }

    if(regResult < 0){
        printk("<6>Failed to register device number\n");
        return regResult;
    }

    /**2. 字符设备初始化 */
    cdev_init(&chrdev, &led_fops);
    chrdev.owner = THIS_MODULE;

    /**3. 注册字符设备到Linux内核 */
    regResult = cdev_add(&chrdev, dev_no, 1);
    if(regResult < 0){
        printk("<6>char device add failed\n");
        goto err_cdev_add;
    }

    /**4.自动创建设备文件 */
    pClassLed = class_create(THIS_MODULE, "SelfClassName");
    pDeviceLed = device_create(pClassLed, NULL, dev_no, NULL, "SelfDeviceName");
    if(pDeviceLed == NULL){
        printk("<6>char device create failed\n");
        goto err_cdev_add;
    }

    printk("<6>char device add success\n");
    return 0;

err_cdev_add:
    /**注销设备号 */
    unregister_chrdev_region(dev_no, 1);
    return regResult;
}

static void __exit chrDevExit(void)
{
    /**0.自动销毁设备文件 */
    device_destroy(pClassLed, dev_no);
    class_destroy(pClassLed);

    /**1. 注销设备号 */
    unregister_chrdev_region(dev_no, 1); 

    /**2. 从linux内核中注销字符设备 */
    cdev_del(&chrdev); 
    printk(KERN_INFO "chrDevExit: Driver unloaded\n");
}

module_init(chrDevInit);
module_exit(chrDevExit);

MODULE_AUTHOR("123456@qq.com");
MODULE_DESCRIPTION("应用层向驱动层的字符设备传递参数");
MODULE_LICENSE("GPL");
MODULE_VERSION("2025-08-31_V1.0");

/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-08-31, lium
 * describe: 初始创建.
 *************************************************************************/