/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: btn_dev.c
 *   软件模块: platform总线设备与驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-10-05
 *   作    者: lium
 *   功    能: platform总线设备
 *
 ************************************************************************/
#include <linux/module.h>
#include <mach/platform.h>
#include <linux/platform_device.h>

/**1. 指定设备名称 */
#define DEVICE_NAME         "buttons_driver"

/**按键的数量 */
#define BTN_SIZE            4

/**2. 硬件资源 */
static struct resource gec6818_buttons_resource[BTN_SIZE] = {
    [0] = {
        .start = (IRQ_GPIO_B_START + 9),    //PAD_GPIO_B + 9  中断号147
		.end   = (IRQ_GPIO_B_START + 9),
		.name  = "KEY0" ,
		.flags = IORESOURCE_IRQ,            //资源类型 中断
    },
    [1] = {
        .start = (IRQ_GPIO_A_START + 28),   //PAD_GPIO_A + 28
		.end   = (IRQ_GPIO_A_START + 28),
		.name  = "KEY1" ,
		.flags = IORESOURCE_IRQ,
    },
    [2] = {
        .start = (IRQ_GPIO_B_START + 30),   //30 PAD_GPIO_B + 30
		.end   = (IRQ_GPIO_B_START + 30),
		.name  = "KEY2", 
		.flags = IORESOURCE_IRQ, 
    },
    [3] = {
        .start = (IRQ_GPIO_B_START + 31),   //31 PAD_GPIO_B + 31
		.end   = (IRQ_GPIO_B_START + 31),
		.name  = "KEY3",
		.flags = IORESOURCE_IRQ,
    },
};

void buttons_release(struct device *dev)
{
	printk(KERN_INFO "buttons device is released\n");
}

struct platform_device gec6818_device_buttons = {
    .id = -1,                       // 让系统自动分配序号
    .name = DEVICE_NAME,            // 指定匹配名
    .dev = {
        .release = buttons_release,
    },
    .num_resources = ARRAY_SIZE(gec6818_buttons_resource),      // 挂载到platform总线上的硬件资源有多少
    .resource = gec6818_buttons_resource                        // 挂载到platform总线上的硬件资源是哪个
};

static int __init btnDevInit(void)
{
    int ret = -1;
	ret = platform_device_register(&gec6818_device_buttons);
	return ret;
}

static void __exit btnDevExit(void)
{
    platform_device_unregister(&gec6818_device_buttons);
}


module_init(btnDevInit);
module_exit(btnDevExit);
MODULE_AUTHOR("lium <123456@qq.com>");
MODULE_DESCRIPTION("btn device");
MODULE_LICENSE("GPL");
MODULE_VERSION("2025-10-02_V1.0.1");
/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-10-05, lium
 * describe: 初始创建.
 *************************************************************************/