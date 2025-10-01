/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: pwm.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-09-20
 *   作    者: lium
 *   功    能: 通过GPIO口不同周期，50%占空比的方波。
 *
 ************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>           // 文件操作集
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/gpio.h>         // gpio口相关函数
#include <cfg_type.h>           // 端口宏定义
#include <linux/err.h>
#include <linux/pwm.h>

#define DEVICE_NAME     "pwm"   // /dev/pwm

#define PWM_MAGIC       'x'     // 定义幻数
#define PWM_MAX_NR      2       // 定义命令的最大序数

#define PWM_IOCTL_SET_FREQ  1      // 如果是1，表示需要设置蜂鸣器的频率
#define PWM_IOCTL_STOP      0      // 如果是0，表示停止蜂鸣器

// 1秒 = 1, 000, 000, 000纳秒
#define NS_IN_1HZ   (1000000000UL)

#define BUZZER_PWM_ID       2                   // 芯片设计了多个端口可以输出PWM。此次从2通道输出
#define BUZZER_PWM_GPIO     (PAD_GPIO_C + 14)   // 指定哪一个GPIO

static struct pwm_device *pwm2buzzer;

static int pwm_open(struct inode *inode, struct file *pFile);
static int pwm_close(struct inode *inode, struct file *pFile);
static long pwm_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg);
static void pwm_set_freq(unsigned long freq);
static void pwm_stop(void);

/**文件操作集 */
static const struct file_operations pwm_fops = {
    .owner      = THIS_MODULE,
    .open       = pwm_open,
    .release    = pwm_close,
    .unlocked_ioctl      = pwm_ioctl,
};

/**杂项字符设备 */
static struct miscdevice mis_dev = {
    .minor = MISC_DYNAMIC_MINOR,        // 由系统自动分配次设备号，杂项字符设备的设备号为10
    .name = DEVICE_NAME,                // 指定 /dev/pwm
    .fops = &pwm_fops,
};

static int __init buzzerInit(void)
{
    int ret;

    /**1.申请GPIO */
    ret = gpio_request(BUZZER_PWM_GPIO, DEVICE_NAME);
    if(ret){
        printk(KERN_ERR "gpio_request failed\n");
        return ret;
    }

    /**2.设置输入输出模式 */
    ret = gpio_direction_output(BUZZER_PWM_GPIO, 0);
    if(ret != 0){
        printk(KERN_ERR "gpio_direction_output failed\n");
        gpio_free(BUZZER_PWM_GPIO);
        return ret;
    }

    /**3. 申请PWM的2通道 */
    pwm2buzzer = pwm_request(BUZZER_PWM_ID, DEVICE_NAME);
    if(IS_ERR(pwm2buzzer)){
        printk(KERN_ERR "pwm_request failed\n");
        ret = -ENODEV;
        goto err_free_gpio;
    }

    /**4. 关闭脉冲宽度调制 */
    pwm_stop();

    /**5. 注册杂项设备。杂项设备模型是对普通字符设备的一个封装（目的是简化开发流程） */
    ret = misc_register(&mis_dev);
    if(ret != 0){
        printk(KERN_ERR "mis_register failed\n");
        goto err_free_pwm;
    }
    return 0;

err_free_pwm:
    pwm_free(pwm2buzzer);
err_free_gpio:
    gpio_free(BUZZER_PWM_GPIO);
    return ret;
}

static void __exit buzzerExit(void)
{
    pwm_stop();

    gpio_free(BUZZER_PWM_GPIO);

    /**注销杂项字符设备 */
    misc_deregister(&mis_dev);
}

static int pwm_open(struct inode *inode, struct file *pFile)
{
    return 0;
}

static int pwm_close(struct inode *inode, struct file *pFile)
{
    return 0;
}

static long pwm_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
    if (_IOC_TYPE(cmd) != PWM_MAGIC) {
        return -EINVAL;
    }
    if (_IOC_NR(cmd) > PWM_MAX_NR) {
        return -EINVAL;
    }

    switch (_IOC_NR(cmd)) {
        case PWM_IOCTL_SET_FREQ:
            {
                if(arg == 0){
                   return -EINVAL; 
                }
                pwm_set_freq(arg);
            } 
            break;
        case PWM_IOCTL_STOP:
            {

            }
            break;
        default:
            {
                pwm_stop();
            }
            break;
    }
    return 0;
}

/**
 * @brief 设置pwm的频率
 * @parma   freq 一多少HZ的频率控制蜂鸣器
 */
static void pwm_set_freq(unsigned long freq)
{
    int period_ns = NS_IN_1HZ / freq;

    // 该函数的单位是ns
    //pwm_config(pwm2buzzer, 高电平持续时间, 周期)
    pwm_config(pwm2buzzer, period_ns/2, period_ns);
    pwm_enable(pwm2buzzer);
}

static void pwm_stop(void)
{
    pwm_config(pwm2buzzer, 0, NS_IN_1HZ/1000);
    pwm_disable(pwm2buzzer);
}

module_init(buzzerInit);
module_exit(buzzerExit);
MODULE_AUTHOR("lium <123456@qq.com>");
MODULE_DESCRIPTION("buzzer Control Driver using GPIOC[14]");
MODULE_LICENSE("GPL");
MODULE_VERSION("2025-10-01_V1.0.1");
/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-10-1, lium
 * describe: 初始创建.
 *************************************************************************/