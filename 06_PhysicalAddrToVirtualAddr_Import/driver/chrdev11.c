/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: chrdev.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-09-08
 *   作    者: lium
 *   功    能: 应用层控制LED（通过GPIOE[13]）
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
#include <linux/string.h>

#define BUF_SIZE    5                   // 接收应用层数据的个数
static char dataBuf[BUF_SIZE];

static unsigned int majorDevID = 0;     // 主设备号（动态分配）
static unsigned int minorDevID = 0;     // 次设备号
static struct cdev chrdev;
static dev_t dev_no;

struct class *pClassLed;
struct device *pDeviceLed;

// GPIO 基地址定义（物理地址）
#define GPIOA_BASE      0xC001A000UL    // GPIOA 基地址
#define GPIOB_BASE      0xC001B000UL    // GPIOB 基地址
#define GPIOC_BASE      0xC001C000UL    // GPIOC 基地址
#define GPIOD_BASE      0xC001D000UL    // GPIOD 基地址
#define GPIOE_BASE      0xC001E000UL    // GPIOE 基地址

// GPIO 寄存器结构体（与硬件布局完全一致）
typedef struct {
    volatile unsigned int GPIOXOUT;                    // 0x0000: 输出数据寄存器
    volatile unsigned int GPIOXOUTENB;                 // 0x0004: 输出使能寄存器
    volatile unsigned int GPIOXDETMODE0;               // 0x0008: 事件检测模式寄存器0
    volatile unsigned int GPIOXDETMODE1;               // 0x000C: 事件检测模式寄存器1
    volatile unsigned int GPIOXINTENB;                 // 0x0010: 中断使能寄存器
    volatile unsigned int GPIOXDET;                    // 0x0014: 事件检测寄存器
    volatile unsigned int GPIOXPAD;                    // 0x0018: PAD状态寄存器
    volatile unsigned int RSVD;                        // 0x001C: 保留
    volatile unsigned int GPIOXALTFN0;                 // 0x0020: 复用功能选择寄存器0
    volatile unsigned int GPIOXALTFN1;                 // 0x0024: 复用功能选择寄存器1
    volatile unsigned int GPIOXDETMODEEX;              // 0x0028: 事件检测模式扩展寄存器
    volatile unsigned int RESERVED1[5];                // 0x002C ~ 0x003C: 保留
    volatile unsigned int GPIOXDETENB;                 // 0x003C: 检测使能寄存器
    volatile unsigned int GPIOX_SLEW;                  // 0x0040: 转换速率控制寄存器
    volatile unsigned int GPIOX_SLEW_DISABLE_DEFAULT;  // 0x0044: 转换速率禁用默认值寄存器
    volatile unsigned int GPIOX_DRV1;                  // 0x0048: 驱动强度控制寄存器1
    volatile unsigned int GPIOX_DRV1_DISABLE_DEFAULT;  // 0x004C: 驱动强度1禁用默认值寄存器
    volatile unsigned int GPIOX_DRV0;                  // 0x0050: 驱动强度控制寄存器0
    volatile unsigned int GPIOX_DRV0_DISABLE_DEFAULT;  // 0x0054: 驱动强度0禁用默认值寄存器
    volatile unsigned int GPIOX_PULLSEL;               // 0x0058: 上拉/下拉选择寄存器
    volatile unsigned int GPIOX_PULLSEL_DISABLE_DEFAULT; // 0x005C: 上拉/下拉选择禁用默认值寄存器
    volatile unsigned int GPIOX_PULLENB;               // 0x0060: 上拉/下拉使能寄存器
    volatile unsigned int GPIOX_PULLENB_DISABLE_DEFAULT; // 0x0064: 上拉/下拉使能禁用默认值寄存器
} GPIO_TypeDef;

// 映射大小：覆盖所有寄存器
#define GPIO_MAP_SIZE   (sizeof(GPIO_TypeDef))

// 定义 GPIO 结构体指针
#define GPIOA           ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB           ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC           ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD           ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE           ((GPIO_TypeDef *) GPIOE_BASE)

// 虚拟地址指针
static volatile GPIO_TypeDef *GPIOE_VA = NULL;
static struct resource *GPIOE_RES = NULL;

/**
 * @brief 打开设备：配置 GPIOE[13] 为输出
 */
static int led_open(struct inode *inode, struct file *pFile)
{
    unsigned int reg;

    printk(KERN_INFO "Led Open start!\n");

    // 使能 GPIOE[13] 输出功能
    reg = ioread32(&GPIOE_VA->GPIOXOUTENB);
    reg |= (1 << 13);
    iowrite32(reg, &GPIOE_VA->GPIOXOUTENB);

    // 初始状态：关闭 LED（高电平关灯）
    reg = ioread32(&GPIOE_VA->GPIOXOUT);
    reg |= (1 << 13);
    iowrite32(reg, &GPIOE_VA->GPIOXOUT);

    printk(KERN_INFO "Led Open success! GPIOE[13] configured as output.\n");
    return 0;
}

/**
 * @brief 关闭设备
 */
static int led_close(struct inode *inode, struct file *pFile)
{
    printk(KERN_INFO "Led close!\n");
    return 0;
}

/**
 * @brief 写入数据：控制LED开关
 * @param buf   '0' -> 开灯, '1' -> 关灯
 */
static ssize_t led_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    int ret;
    int i;
    size_t copy_len = len;

    // 限制长度
    if (copy_len > BUF_SIZE)
        copy_len = BUF_SIZE;

    // 复制用户数据
    ret = copy_from_user(dataBuf, buf, copy_len);
    if (ret != 0) {
        printk(KERN_ERR "led_write: copy_from_user failed, %d bytes not copied\n", ret);
        return -EFAULT;
    }

    // 打印接收到的数据
    printk(KERN_INFO "led_write: received %zu bytes: %.*s\n", copy_len, (int)copy_len, dataBuf);
    for (i = 0; i < copy_len; i++) {
        printk(KERN_INFO "received [%d]: '%c' (0x%02X)\n", i, dataBuf[i], dataBuf[i]);
    }

    // 控制LED
    if (dataBuf[0] == '1') {
        // 关灯：输出高电平
        iowrite32(ioread32(&GPIOE_VA->GPIOXOUT) | (1 << 13), &GPIOE_VA->GPIOXOUT);
        printk(KERN_INFO "LED OFF\n");
    } else if (dataBuf[0] == '0') {
        // 开灯：输出低电平
        iowrite32(ioread32(&GPIOE_VA->GPIOXOUT) & ~(1 << 13), &GPIOE_VA->GPIOXOUT);
        printk(KERN_INFO "LED ON\n");
    } else {
        printk(KERN_WARNING "led_write: unknown command '%c'\n", dataBuf[0]);
    }

    return len;
}

/**
 * @brief 文件操作集
 */
static const struct file_operations led_fops = {
    .owner      = THIS_MODULE,
    .open       = led_open,
    .release    = led_close,
    .write      = led_write,
};

/**
 * @brief 驱动初始化
 */
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

    // 4. 请求并映射 GPIOE 内存区域
    GPIOE_RES = request_mem_region((unsigned long)GPIOE, GPIO_MAP_SIZE, "gpioe_region");
    if (!GPIOE_RES) {
        printk(KERN_ERR "request_mem_region failed for GPIOE\n");
        ret = -EBUSY;
        goto err_request_mem;
    }

    GPIOE_VA = (volatile GPIO_TypeDef *)ioremap((unsigned long)GPIOE, GPIO_MAP_SIZE);
    if (!GPIOE_VA) {
        printk(KERN_ERR "ioremap failed for GPIOE\n");
        ret = -EBUSY;
        goto err_ioremap;
    }

    printk(KERN_INFO "GPIOE mapped: PA=0x%08X VA=%p\n", (unsigned int)GPIOE_BASE, (void*)GPIOE_VA);

    // 5. 自动创建设备文件
    pClassLed = class_create(THIS_MODULE, "led_class");
    if (IS_ERR_OR_NULL(pClassLed)) {
        printk(KERN_ERR "class_create failed\n");
        ret = PTR_ERR(pClassLed);
        pClassLed = NULL;
        goto err_class_create;
    }

    pDeviceLed = device_create(pClassLed, NULL, dev_no, NULL, "LEDE");
    if (IS_ERR_OR_NULL(pDeviceLed)) {
        printk(KERN_ERR "device_create failed\n");
        ret = PTR_ERR(pDeviceLed);
        pDeviceLed = NULL;
        goto err_device_create;
    }

    printk(KERN_INFO "LED driver initialized successfully (major=%d)\n", majorDevID);
    return 0;

err_device_create:
    class_destroy(pClassLed);
err_class_create:
    iounmap((void __iomem *)GPIOE_VA);
err_ioremap:
    release_mem_region(GPIOE_BASE, GPIO_MAP_SIZE);
err_request_mem:
    cdev_del(&chrdev);
err_cdev_add:
    unregister_chrdev_region(dev_no, 1);
    return ret;
}

/**
 * @brief 驱动卸载
 */
static void __exit chrDevExit(void)
{
    if (pDeviceLed)
        device_destroy(pClassLed, dev_no);

    if (pClassLed)
        class_destroy(pClassLed);

    cdev_del(&chrdev);
    unregister_chrdev_region(dev_no, 1);

    if (GPIOE_VA) {
        iounmap((void __iomem *)GPIOE_VA);
        GPIOE_VA = NULL;
    }

    if (GPIOE_RES) {
        release_mem_region(GPIOE_BASE, GPIO_MAP_SIZE);
        GPIOE_RES = NULL;
    }

    printk(KERN_INFO "chrDevExit: LED driver unloaded\n");
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
 * Revision 1.1, 2025-09-08, lium
 * describe: 使用 GPIO_TypeDef 结构体统一映射，优化代码结构。
 *************************************************************************/