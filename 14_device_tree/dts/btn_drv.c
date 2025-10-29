/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: btn_drv.c
 *   软件模块: 设备数驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-10-25
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

/**用于保存设备树中的 key4s-->> interrupts */
#define IRQ_ATTR_NUM    8           // 中断属性的数量
static unsigned int key4s_interrupts[IRQ_ATTR_NUM] = {0};

#define DEV_NAME		"gecBt"                // 设备名字 /dev/DEV_NAME
#define DRIVICE_NAME    "buttons_driver"       // 用于device和drivice的匹配名称

/**等待队列 */
static wait_queue_head_t button_waitq;

/**按键的数量 */
#define BTN_SIZE            4

/**按键信息 */
typedef struct button_desc {
	int irq;                                        // 中断号
	int number;                                     // 按键序号 第几个按键
	const char *name;	                            // 按键的名字
}TButtonDesc_t;

static TButtonDesc_t buttons[BTN_SIZE];            //定义按键结构体数组

/**平台总线获取到的硬件资源 */
//static struct resource *pBtnResource;

/**按键的状态。*/
static volatile int keyValues[BTN_SIZE] = {0, 0, 0, 0};

/**是否阻塞等待标准位。0：阻塞等待；1：不阻塞等待 */
static volatile int ev_press = 0;

// 按键值 字符型 为了方便拷贝到用户空间
static volatile char key_values[] = {
	'0', '0', '0', '0'
};

static int btn_open(struct inode *inode, struct file *pFile);
static int btn_close(struct inode *inode, struct file *pFile);
static unsigned int btn_poll( struct file *file, struct poll_table_struct *wait);
static ssize_t btn_read(struct file *pFile, char __user *buf, size_t count, loff_t *ppos);

static int __devinit gec6818_buttons_probe(struct platform_device *pdev);
static int __devexit gec6818_buttons_remove(struct platform_device *pdev);
static irqreturn_t btnIRQCallBack(int irq, void *dev_id);

/**文件操作集 */
static struct file_operations dev_fops = {
	.owner		= THIS_MODULE,
    .open       = btn_open,
	.release	= btn_close, 
	.read		= btn_read,
	.poll		= btn_poll,
};

/**杂项字符设备 */
static struct miscdevice misc = {
	.minor		= MISC_DYNAMIC_MINOR,   // 杂项设备主设备号默认为10 ，自动分配次设备号
	.name		= DEV_NAME,             // 生成设备文件的名字 /dev/DEV_NAME
	.fops		= &dev_fops,
};

/**定义设备匹配信息结构 */
static const struct of_device_id of_gec_key4s_match[] = {
    {.compatible = "gec,keyDemo", },
    {}
};

/**平台总线 */
static struct platform_driver gec6818_buttons_driver = {
	.probe  = gec6818_buttons_probe,
	.remove = __devexit_p(gec6818_buttons_remove),
	.driver = {
		.owner = THIS_MODULE,
		.name  = DRIVICE_NAME,	  // buttons_driver必须和设备的名字一致。
        .of_match_table = of_gec_key4s_match
	},	
};

static int __init btnDrvInit(void)
{
	int ret;
	ret = platform_driver_register(&gec6818_buttons_driver);
	return ret;
}

static void __exit btnDrvExit(void)
{
    platform_driver_unregister(&gec6818_buttons_driver);
}

static int btn_open(struct inode *inode, struct file *pFile)
{
    int i;
    int err = 0;
    int tempIRQ;

    /**1. 初始化等待队列 */
    init_waitqueue_head(&button_waitq);

    /**2. 将获取到的中断号注册到内核 */
    for (i = 0; i < BTN_SIZE; i++){
        
        // 打印调试信息
        printk(KERN_INFO "buttons:[%d], irq is %d.", i, buttons[i].irq);
	    printk("<0>\n");

        // 如果中断号为0，直接调出循环
        if (!buttons[i].irq) {
            continue;       
        }

        tempIRQ = buttons[i].irq;

        // 注册中断号, 并且将每个按键 都设置 为双边向沿 触发模式
		err = request_irq(tempIRQ, btnIRQCallBack, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, buttons[i].name, (void *)&buttons[i]);
        if (err)
		{ 
            //表示以十进制形式打印一个整数，并且至少打印两位数字。如果数字不足两位，会在前面补零。
			pr_err("irq = %.2d is failed\n",i); 
			break;
		}
    }

    /**中断申请失败的时候，释放中断 */
	if (err) {
		i--;
		for (; i >= 0; i--) {
			if (!buttons[i].irq)
				continue;

			tempIRQ = buttons[i].irq;
			disable_irq(tempIRQ);
			free_irq(tempIRQ, (void *)&buttons[i]);
      
		}
		return -EBUSY;
	}
    return 0;
}

static int btn_close(struct inode *inode, struct file *pFile)
{
    int irq, i;	

    /**释放中断 */
	for (i = 0; i < ARRAY_SIZE(buttons); i++) {
		if (!buttons[i].irq) {
            continue;
        }
		irq = buttons[i].irq;
		free_irq(irq, (void *)&buttons[i]);
	}
    return 0;
}

static unsigned int btn_poll( struct file *file, struct poll_table_struct *wait)
{
    return 0;
}

static ssize_t btn_read(struct file *pFile, char __user *buf, size_t count, loff_t *ppos)
{
    int i;
    unsigned long err;

    // ev_press = 0，当前读进程进入等待队列并且进入睡眠状态，让出 CPU
    wait_event_interruptible(button_waitq, ev_press);   

    for(i = 0; i < BTN_SIZE; i++){
        key_values[i] += keyValues[i];
    }

    err = copy_to_user((void *)buf, (const void *)(&key_values),
        min(sizeof(key_values), count));

    ev_press = 0;

    // 每次读取按键值后，清零
    for(i = 0; i < BTN_SIZE; i++) {
        key_values[i] = '0';
    }

    return err ? -EFAULT : min(sizeof(key_values), count);;
}

static int __devinit gec6818_buttons_probe(struct platform_device *pdev)
{
    int i;
    int ret;

    /**1. 从设备树中获取设备节点属性 */
    struct device_node *key_node;       // key_node 就是表示对象 设备树中的key4s
    key_node = pdev->dev.of_node;
    printk(KERN_INFO "dt node name is : %s \n", key_node->name);

    /**1.1 查找属性值 */
    ret = of_property_read_u32_array(key_node, "interrupts", key4s_interrupts, IRQ_ATTR_NUM);
    if(ret){
        printk(KERN_ERR "of_property_read_u32_array failed!\n");
        return -ENOENT;
    }

    for(i = 0; i < IRQ_ATTR_NUM / 2; i++){
        buttons[i].number = i;                          // 第几个按键
        buttons[i].irq = key4s_interrupts[2 * i + 0];   // 第几个按键对应的中断号
    }

    /**2. 向内核注册杂项设备 */
    ret = misc_register(&misc);
    printk(KERN_INFO "gec6818_buttons_probe successed!\n");
    return ret;
}

static int __devexit gec6818_buttons_remove(struct platform_device *pdev)
{
	misc_deregister(&misc);
	return 0;
}

static irqreturn_t btnIRQCallBack(int irq, void *dev_id)
{
    TButtonDesc_t *pBtnData = (TButtonDesc_t *)dev_id;

    /**按键的键值赋值 */
    keyValues[pBtnData->number] = !keyValues[pBtnData->number];

    /**按键按下的标志位 */
    ev_press = 1;

    /**唤醒进程 */
	wake_up_interruptible(&button_waitq);

    // IRQ_HANDLED已经捕获到中断信号，并且以正确处理
    return IRQ_HANDLED;
}

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