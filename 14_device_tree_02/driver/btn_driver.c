/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: btn_drv.c
 *   软件模块: 设备数驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-10-29
 *   作    者: lium
 *   功    能: 字符设备驱动
 *
 ************************************************************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include <linux/of.h>           // 设备树核心 API，如 of_find_node_by_name, of_property_read_string 等
#include <linux/of_gpio.h>     // 从设备树中获取 GPIO 信息
#include <linux/of_address.h>  // 获取设备树中的内存地址映射（如寄存器基地址）
#include <linux/of_fdt.h>      // 设备树扁平化格式（FDT）相关函数（一般内核内部使用）
#include <linux/of_platform.h> // 支持 platform_device 的设备树绑定







module_init(btnDrvInit);
module_exit(btnDrvExit);
MODULE_AUTHOR("lium <123456@qq.com>");
MODULE_DESCRIPTION("btn Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("2025-10-02_V1.0.1");
/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-10-05, lium
 * describe: 初始创建.
 *************************************************************************/