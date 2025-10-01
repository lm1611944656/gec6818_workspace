/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: dht11.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-09-20
 *   作    者: lium
 *   功    能: 通过GPIO口读取DHT11
 *
 ************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>      // copy_from_user
#include <linux/fs.h>           // 文件操作集
#include <linux/device.h>       // create_device
#include <linux/gpio.h>         // gpio口相关函数
#include <cfg_type.h>           // 端口宏定义
#include <linux/delay.h>        // 延时函数

static unsigned int majorDevID = 0;     // 主设备号（动态分配）
static unsigned int minorDevID = 0;     // 次设备号
static struct cdev chrdev;
static dev_t dev_no;
struct class *pClass;
struct device *pDevice;

#define DHT11_DATA (PAD_GPIO_B + 29)

#define GET_DHT11_DATA _IOR('w', 0, unsigned long)

static int dht11_open(struct inode *inode, struct file *pFile);
static int dht11_close(struct inode *inode, struct file *pFile);
static long dht11_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg);
static unsigned char get_value(void);

static const struct file_operations dht11_fops = {
    .owner      = THIS_MODULE,
    .open       = dht11_open,
    .release    = dht11_close,
    .unlocked_ioctl      = dht11_ioctl,
};

static int __init chrDevInit(void)
{
    int ret;

    /**1. 申请设备号(推荐使用动态注册) */
    if (majorDevID) {
        dev_no = MKDEV(majorDevID, minorDevID);
        ret = register_chrdev_region(dev_no, 1, "dht11_device");
    } else {
        ret = alloc_chrdev_region(&dev_no, minorDevID, 1, "dht11_device");
        majorDevID = MAJOR(dev_no);
    }
    if (ret < 0) {
        printk(KERN_ERR "Failed to register device number\n");
        return ret;
    }

    /**2. 字符设备初始化 */
    cdev_init(&chrdev, &dht11_fops);
    chrdev.owner = THIS_MODULE;

    /**3. 将cdev注册到linux内核 */
    ret = cdev_add(&chrdev, dev_no, 1);
    if (ret < 0) {
        printk(KERN_ERR "cdev_add failed\n");
        goto err_cdev_add;
    }

    /**4. 在 /sys/class/ 下创建设备类 */
    pClass = class_create(THIS_MODULE, "dht11_class");
    if (IS_ERR_OR_NULL(pClass)) {
        printk(KERN_ERR "class_create failed\n");
        ret = PTR_ERR(pClass);
        pClass = NULL;
        goto err_class_create;
    }

    /**5. 在 /dev 下创建设备节点*/
    pDevice = device_create(pClass, NULL, dev_no, NULL, "dht11");
    if (IS_ERR_OR_NULL(pDevice)) {
        printk(KERN_ERR "device_create failed\n");
        ret = PTR_ERR(pDevice);
        pDevice = NULL;
        goto err_device_create;
    }

    /**6.申请GPIO资源 */
    if(gpio_request(DHT11_DATA, "DHT11_DATA") != 0){
        printk(KERN_ERR "gpio_request failed\n");
    }

    /**7.设置输入输出模式 */
    ret = gpio_direction_output(DHT11_DATA, 1);
    if(ret != 0){
        printk(KERN_ERR "gpio_direction_output failed\n");
        gpio_free(DHT11_DATA);
        goto err_gpio;
    }

    printk(KERN_INFO "dht11 driver initialized successfully (major=%d)\n", majorDevID);
    return 0;

/**错误处理：反向释放资源 */
err_gpio:
    device_destroy(pClass, dev_no);
err_device_create:
    class_destroy(pClass);
err_class_create:
    cdev_del(&chrdev);
err_cdev_add:
    unregister_chrdev_region(dev_no, 1);
    return ret;
}

static void __exit chrDevExit(void)
{
    gpio_free(DHT11_DATA);

    if (pDevice)
        device_destroy(pClass, dev_no);

    if (pClass)
        class_destroy(pClass);

    cdev_del(&chrdev);

    unregister_chrdev_region(dev_no, 1);

    printk(KERN_INFO "chrDevExit: dht11 driver unloaded\n");
}

static int dht11_open(struct inode *inode, struct file *pFile)
{
    /**打开时钟 */

    /**配置GPIO口 */

    gpio_set_value(DHT11_DATA, 1);

    /**使能GPIO */

    printk(KERN_INFO "dht11_open open success\n");
    return 0;
}

static int dht11_close(struct inode *inode, struct file *pFile)
{
    gpio_set_value(DHT11_DATA, 1);
    printk(KERN_INFO "dht11_open close success\n");
    return 0;
}

static long dht11_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
    int ret;
    int i;
    int timeout;                            // 超时时间
    unsigned long flags;                    // 保存中断值
    unsigned char dht11Arr[5] = {0};        // 数组    
    unsigned char check_sum = 0;             // 校验和

    if(cmd == GET_DHT11_DATA){
        // 关闭中断(我在采集DHT11的数据的期间，不希望CPU打断执行)
        local_irq_save(flags);

        // 设置为输出模式
        ret = gpio_direction_output(DHT11_DATA, 0);
        if(ret != 0){
            printk(KERN_ERR "gpio_direction_output failed\n");
        }

        // 步骤2：发送起始信号。主机拉低18ms信号
        gpio_set_value(DHT11_DATA, 0);
        msleep(20);

        // 设置为输入模型
        ret = gpio_direction_input(DHT11_DATA);
        if(ret != 0){
            printk(KERN_ERR "gpio_direction_input failed\n");
            local_irq_restore(flags);
            return ret;
        }
        
        // 设置为输入之后，延时10us在去读取引脚电平。
        udelay(10);
        // 如果10us都过去了，引脚还是没有响应(响应低电平)。那就判断为DHT11不鸟MCU
        if(gpio_get_value(DHT11_DATA)){
            printk(KERN_ERR "DHT11 timeout error\n");
            local_irq_restore(flags);
            return -1;
        }

        // 步骤3：DHT11响应信号判断
        timeout = 0;
        while(!gpio_get_value(DHT11_DATA))
        {
            timeout++;
            udelay(1);
            if(timeout > 160){
                printk(KERN_ERR "DHT11 timeout error\n");
                local_irq_restore(flags);
                return -1;
            }
        }

        timeout = 0;
        while(gpio_get_value(DHT11_DATA))
        {
            timeout++;
            udelay(1);
            if(timeout > 160){
                printk(KERN_ERR "DHT11 timeout error\n");
                local_irq_restore(flags);
                return -1;
            }
        }

        // 步骤4：获取DHT11数据
        for(i = 0; i < 5; i++){
            dht11Arr[i] = get_value();
        }

        /* 判断效验和 */
        check_sum = (dht11Arr[0] + dht11Arr[1] + dht11Arr[2] + dht11Arr[3]) & 0xFF;
        if(check_sum != dht11Arr[4]){
            printk(KERN_ERR "DHT11 check_sum error\n");
            return -1;
        }
        local_irq_restore(flags);
    } else {
        printk(KERN_INFO "ioctl cmd error\n");
        return -1;
    }

    // 仅仅打印湿度和温度的整数部分
    printk(KERN_INFO "th_data = %hhu temp_data = %hhu\n", dht11Arr[0], dht11Arr[2]);

    // 将湿度传递到用户空间
    ret = copy_to_user(((unsigned char *)arg) + 2, &dht11Arr[0], 2);
    if(ret != 0){
        printk(KERN_ERR "copy_to_user failed\n");
        return -1;
    }

    // 将温度传递到用户空间
    ret = copy_to_user(((unsigned char *)arg) + 0, &dht11Arr[2], 2);
    if(ret != 0){
        printk(KERN_ERR "copy_to_user failed\n");
        return -1;
    }
    return 0;
}

static unsigned char get_value(void)
{
    int i;
    int timeout = 0;
    int ret_data = 0;

    for(i = 0; i < 8; i++){
        timeout = 0;
        while(!gpio_get_value(DHT11_DATA)){
            timeout++;
            udelay(1);
            if(timeout > 160){
                printk(KERN_ERR "DHT11 timeout error\n");
                return -ETIMEDOUT; // 直接返回错误
            }
        }

        udelay(22);
        ret_data <<= 1;
        if(gpio_get_value(DHT11_DATA)){
            ret_data |= 0x01;
        }

        timeout = 0;
        while(gpio_get_value(DHT11_DATA)){
            timeout++;
            udelay(1);
            if(timeout > 200){
                printk(KERN_ERR "DHT11 timeout error\n");
                return -ETIMEDOUT; // 直接返回错误
            }
        }
    }
    return ret_data;
}

module_init(chrDevInit);
module_exit(chrDevExit);
MODULE_AUTHOR("lium <123456@qq.com>");
MODULE_DESCRIPTION("LED Control Driver using GPIOE[13]");
MODULE_LICENSE("GPL");
MODULE_VERSION("2025-09-20_V1.0.1");
/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-09-20, lium
 * describe: 初始创建.
 *************************************************************************/
