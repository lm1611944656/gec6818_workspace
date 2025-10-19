/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: adc.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-10-2
 *   作    者: lium
 *   功    能: 通过片上ADC采集烟雾传感器数据
 *
 ************************************************************************/
#include <linux/kernel.h>      // printk、内核日志宏和常用内核函数
#include <linux/module.h>      // module_init/module_exit，模块宏定义
#include <linux/fs.h>          // file_operations、struct inode、struct file 等文件操作接口
#include <linux/uaccess.h>     // copy_to_user/copy_from_user，用户态与内核态数据拷贝
#include <linux/io.h>          // ioremap/iounmap/ioread32/iowrite32，内存映射IO访问
#include <linux/ioctl.h>       // _IOR/_IOW/_IO 宏，用于 ioctl 命令定义
#include <linux/ioport.h>      // request_mem_region/release_mem_region，申请物理地址资源
#include <linux/cdev.h>        // struct cdev、cdev_init、cdev_add、cdev_del，字符设备注册管理
#include <linux/device.h>      // class_create/device_create/device_destroy，生成 /dev/xxx 节点

/**初始化设备类和设备节点 */
#define CLASS_NAME      "adc_class"                         // 设备类：sys/class/adc_class 
#define DEVICE_NAME     "adc"                               // 设备节点：/dev/adc

/**初始化主设备号和次设备号 */
static unsigned int majorDevID = 0;                         // 主设备号（动态分配）
static unsigned int minorDevID = 0;                         // 次设备号

/**字符设备结构 */
static dev_t dev_no;                                        // 主设备号 + 次设备号
static struct cdev adc_cdev;                                // cdev 对象
static struct class *adc_class;                             // sysfs 类
static struct device *adc_device;                           // 设备节点对象

/**物理地址 */
#define GEC6818_ADC_PHY_ADDR        0xC0053000              // ADC的起始基地址
#define GPIO_MAP_SIZE               0x14                    // 需要申请的虚拟地址空间大小

/**物理地址转换为虚拟地址，用于保存ADC的虚拟地址 */
static void __iomem *adc_base_va;                           // adc的虚拟地址基址
static void __iomem *adcon_va;
static void __iomem *adcdat_va;
static void __iomem *prescalercon_va;

/**ioctl */
#define GEC6818_ADC_IN0   _IOR('A', 0, unsigned long)       // ADC通道0
#define GEC6818_ADC_IN1   _IOR('A', 1, unsigned long)       // ADC通道1
#define GEC6818_ADC_IN2   _IOR('A', 2, unsigned long)       // ADC通道2
#define GEC6818_ADC_IN3   _IOR('A', 3, unsigned long)       // ADC通道3

/**文件操作集 */
static int adc_open(struct inode *inode, struct file *pFile);
static int adc_close(struct inode *inode, struct file *pFile);
static ssize_t adc_read(struct file *pFile, char __user *buf, size_t count, loff_t *ppos);
static long adc_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg);

/**文件操作集 */
static const struct file_operations adc_fops = {
    .owner      = THIS_MODULE,
    .open       = adc_open,
    .release    = adc_close,
    .read       = adc_read,
    .unlocked_ioctl      = adc_ioctl,
};

/**主函数入口 */
static int __init adcInit(void)
{
    int ret = -1;

    /**1. 申请设备号(自动分配) */
    if (majorDevID) {
        dev_no = MKDEV(majorDevID, minorDevID);
        ret = register_chrdev_region(dev_no, 1, "adc_device");
    } else {
        ret = alloc_chrdev_region(&dev_no, minorDevID, 1, "adc_device");
        majorDevID = MAJOR(dev_no);
    }
    if (ret < 0) {
        printk(KERN_ERR "Failed to register device number\n");
        return ret;
    }

    /**2. 字符设备初始化 */
    cdev_init(&adc_cdev, &adc_fops);
    adc_cdev.owner = THIS_MODULE;

    /**3. 将cdev注册到linux内核 */
    ret = cdev_add(&adc_cdev, dev_no, 1);
    if (ret) {
        printk(KERN_ERR "cdev_add failed\n");
        goto err_cdev_add;
    }

    /**4. 创建设备类 /sys/class/adc_class */
    adc_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(adc_class)) {
        printk(KERN_ERR "class_create failed\n");
        ret = PTR_ERR(adc_class);
        goto err_class_create;
    }

    /**5. 创建设备节点 /dev/adc */
    adc_device = device_create(adc_class, NULL, dev_no, NULL, DEVICE_NAME);
    if (IS_ERR(adc_device)) {
        printk(KERN_ERR "device_create failed\n");
        ret = PTR_ERR(adc_device);
        goto err_device_create;
    }

    /**6. 地址映射 */
    adc_base_va = ioremap(GEC6818_ADC_PHY_ADDR, GPIO_MAP_SIZE);
    if (!adc_base_va) {
        printk(KERN_ERR "ioremap failed\n");
        ret = -ENOMEM;
        goto err_ioremap;
    }
    adcon_va        = adc_base_va + 0x00;
    adcdat_va       = adc_base_va + 0x04;
    prescalercon_va = adc_base_va + 0x10;

    printk(KERN_INFO "adc char driver init success\n");
    return 0;

err_ioremap:
    device_destroy(adc_class, dev_no);
err_device_create:
    class_destroy(adc_class);
err_class_create:
    cdev_del(&adc_cdev);
err_cdev_add:
    unregister_chrdev_region(dev_no, 1);
    return ret;
}

/**主函数出口 */
static void __exit adcExit(void)
{
    iounmap(adc_base_va);
    device_destroy(adc_class, dev_no);
    class_destroy(adc_class);
    cdev_del(&adc_cdev);
    unregister_chrdev_region(dev_no, 1);

    printk(KERN_INFO "adc char driver exit\n");
}

/**-------- open/release/read/ioctl -------- */
static int adc_open(struct inode *inode, struct file *pFile)
{
    printk(KERN_INFO "adc_open success\n");
    return 0;
}

static int adc_close(struct inode *inode, struct file *pFile)
{
    printk(KERN_INFO "adc device close\n");
    return 0;
}

static ssize_t adc_read(struct file *pFile, char __user *buf, size_t count, loff_t *ppos)
{
    printk(KERN_INFO "adc adc_read success\n");
    return 0;
}

static long adc_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
    unsigned long adc_value = 0;
    unsigned long adc_vol = 0;
    unsigned int reg_val;
    int ret = -1;

    printk(KERN_INFO "adc ioctl: cmd = %d\n", cmd);

    switch (cmd) {
        case GEC6818_ADC_IN0:   // [5:3]=000 通道0
            reg_val = ioread32(adcon_va);
            reg_val &= (~(7 << 3));
            iowrite32(reg_val, adcon_va);
            break;
        case GEC6818_ADC_IN1:   // [5:3]=001 通道1
            iowrite32((ioread32(adcon_va) & ((~(7 << 3)))), adcon_va);
            iowrite32((ioread32(adcon_va) | ((1 << 3))), adcon_va);
            break;
        case GEC6818_ADC_IN2:   // [5:3]=010 通道2
            iowrite32((ioread32(adcon_va) & ((~(7 << 3)))), adcon_va);
            iowrite32((ioread32(adcon_va) | ((2 << 3))), adcon_va);
            break;
        case GEC6818_ADC_IN3:   // [5:3]=011 通道3
            iowrite32((ioread32(adcon_va) & ((~(7 << 3)))), adcon_va);
            iowrite32((ioread32(adcon_va) | ((3 << 3))), adcon_va);
            break;
        default:
            printk(KERN_ERR "adc_ioctl failed\n");
            return -ENOIOCTLCMD;
    }

    // 将ADC的电源开启 [2] = 0，开启电源
    iowrite32(ioread32(adcon_va) & ~(1 << 2), adcon_va);

    // [9:0] = 199 + 1 = 200，预分频值设置为 199+1，ADC 工作频率 = 200MHz / (199+1) = 1MHz
    iowrite32(ioread32(prescalercon_va) & ~(0x3FF << 0), prescalercon_va);  // 清除低10位
    iowrite32(ioread32(prescalercon_va) | (199 << 0), prescalercon_va);     // 设置为199

    // 预分频值使能 [15] = 1，启用预分频
    iowrite32(ioread32(prescalercon_va) | (1 << 15), prescalercon_va);

    // ADC 使能 [0] = 1，启动 ADC 转换
    iowrite32(ioread32(adcon_va) | (1 << 0), adcon_va);

    // 等待 AD 转换结束 [0] = 0，表示转换完成
    while (ioread32(adcon_va) & (1 << 0));

    // 读取12bit数据（低12位有效）
    adc_value = ioread32(adcdat_va) & 0xFFF;

    // 关闭CLKIN时钟输入 [15]=0, disable
    iowrite32(ioread32(prescalercon_va) & ~(1 << 15), prescalercon_va);

    // 关闭ADC电源 [2]=1, power off
    iowrite32(ioread32(adcon_va) | (1 << 2), adcon_va);

    // 将AD转换的结果值换算为电压值（假设参考电压为1.8V）
    // 12位ADC最大值：4095，ADC的参考电压为：1.8V
    adc_vol = adc_value * 1800 / 4095; // 单位：mV

    // 将电压值复制到用户空间
    ret = copy_to_user((void *)arg, &adc_vol, 4);

    if (ret != 0)
        return -EFAULT;

    return 0;
}

module_init(adcInit);
module_exit(adcExit);
MODULE_AUTHOR("lium <123456@qq.com>");
MODULE_DESCRIPTION("ADC Driver for smoke sensor using on-chip ADC");
MODULE_LICENSE("GPL");
MODULE_VERSION("2025-10-02_V1.0.1");
/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-10-02, lium
 * describe: 初始创建.
 *************************************************************************/