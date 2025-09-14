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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/io.h>

#define BUF_SIZE    5                   // 接收应用层数据的个数
static char dataBuf[BUF_SIZE];

static unsigned int majorDevID = 0;     // 主设备号
static unsigned int minorDevID = 0;     // 次设备号
static struct cdev chrdev;
static dev_t dev_no;

struct class *pClassLed;
struct device *pDeviceLed;

/**一个物理地址是4个字节，所以要申请8个字节 */
#define GPIOEOUT_PA 0xC001E000      // GPIOE的输出数据寄存器的物理地址
#define GPIOEOUTENB_PA 0xC001E004   // GPIOE的输出使能寄存器的物理地址

static struct resource *GPIOEOUT_RES;
static unsigned int *GPIOEOUT_VA;        // 输出数据寄存器的虚拟地址
static unsigned int *GPIOEOUTENB_VA;     // 输出使能寄存器的虚拟地址

static int led_open(struct inode *inode, struct file *pFile)
{
    unsigned int temp;
    printk(KERN_INFO "Led Open start! \n");

    /* 输出使能配置 [13] = 1 */
    temp = ioread32(GPIOEOUTENB_VA);
    temp |= (1 << 13);
    iowrite32(temp, GPIOEOUTENB_VA);  

    /* 初始时刻先关闭LED [13] = 1 */
    temp = ioread32(GPIOEOUT_VA);
    temp |= (1 << 13);
    iowrite32(temp, GPIOEOUT_VA);     

    printk(KERN_INFO "Led Open success! \n");
    return 0;
}

static int led_close(struct inode *inode, struct file *pFile)
{
    /**恢复硬件寄存器中的内容 */
    printk("<6> Led close! \n");
    return 0;
}

/**
 * @brief 打开或关闭LED的函数
 * @param buf   当buf[0] = 0--->on，buff[1] = 1----off
 */
static ssize_t led_write(struct file *file, const char __user *buf, size_t len, loff_t *off) 
{
    int ret;
    int i;

    // 限制写入长度，防止溢出
    if (len > BUF_SIZE) {
        len = BUF_SIZE;
    }

    /**将用户空间的数据复制到内核空间 */
    ret = copy_from_user(dataBuf, buf, len);
    if (ret != 0) {
        printk(KERN_ERR "led_write: copy_from_user failed, %d bytes not copied\n", ret);
        return -EFAULT;
    }
    printk(KERN_INFO "led_write: received %zu bytes: %.*s\n", len, (int)len, dataBuf);
    for(i = 0; i < len; i++){
        printk(KERN_INFO "received [%d]: %d\n", i, *(dataBuf + i));
    }

    /**关灯 */
    if(buf[0] == '1'){
        *GPIOEOUT_VA |= (1 << 13);
    }
    
    /**开灯 */
    if(buf[0] == '0'){
        *GPIOEOUT_VA &= ~(1 << 13);
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

    /**4. 在内核空间申请一片内存，用于表示为GPIO口的虚拟地址 */
    GPIOEOUT_RES = request_mem_region(GPIOEOUT_PA, 8, "GPIOEOUT_MEM_NAME");
    if(GPIOEOUT_RES == NULL){
        printk("<6>request_mem_region failed\n");
        regResult = -EBUSY;
        goto request_mem_failed;
    }
    GPIOEOUT_VA = ioremap(GPIOEOUT_PA, 8);
    if(GPIOEOUT_VA == NULL){
        printk("<6>ioremap failed\n");
        regResult = -EBUSY;
        goto ioremap_failed;
    }
    GPIOEOUTENB_VA = GPIOEOUT_VA + 1;
    printk(KERN_INFO "GPIOEOUT_VA addr: %p, GPIOEOUTENB_VA addr: %p\n", GPIOEOUT_VA, GPIOEOUTENB_VA);

    /**4.自动创建设备文件 */
    pClassLed = class_create(THIS_MODULE, "SelfClassName");
    if(pClassLed == NULL){
        printk("<6>class_create failed\n");
        goto class_create_failed;
    }
    pDeviceLed = device_create(pClassLed, NULL, dev_no, NULL, "SelfDeviceName");
    if(pDeviceLed == NULL){
        printk("<6>device_create failed\n");
        goto device_create_failed;
    }

    printk("<6>char device init success\n");
    return 0;

device_create_failed:
    class_destroy(pClassLed);

class_create_failed:
    iounmap(GPIOEOUT_VA);

ioremap_failed:
    release_mem_region(GPIOEOUT_PA, 8);

request_mem_failed:
    cdev_del(&chrdev);

/**注销设备号 */
err_cdev_add:
    unregister_chrdev_region(dev_no, 1);
    return regResult;
}

static void __exit chrDevExit(void)
{
    /**0.自动销毁设备文件 */
    device_destroy(pClassLed, dev_no);

    /**1.销毁类 */
    class_destroy(pClassLed);

    /**2. 注销设备号 */
    unregister_chrdev_region(dev_no, 1); 

    /**3.解除映射 */
    iounmap(GPIOEOUT_VA);

    /**4.释放内存 */
    unregister_chrdev_region(dev_no, 1);

    /**5. 从linux内核中注销字符设备 */
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