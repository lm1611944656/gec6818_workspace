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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

/**驱动和应用层的命令要一致 */
#define GEC6818_ADC_IN0   _IOR('A', 0, unsigned long) // 读取通道0的命令
#define GEC6818_ADC_IN1   _IOR('A', 1, unsigned long) // 读取通道1的命令
#define GEC6818_ADC_IN2   _IOR('A', 2, unsigned long) // 读取通道2的命令
#define GEC6818_ADC_IN3   _IOR('A', 3, unsigned long) // 读取通道3的命令

int main(int argc, char **argv)
{
    int fd = -1;
    int ret = -1;
    int i = 0;
    unsigned long adc_vol = 0;
    int channel = 0;

    // 打开ADC设备
    fd = open("/dev/adc", O_RDWR);
    if (fd < 0) {
        perror("open /dev/adc_drv:");
        return -1;
    }

    // 提示用户输入通道号
    printf("1. Please input the channel (0, 1, 2, 3...): ");
    scanf("%d", &channel);
    while (1) {
        switch (channel) {
            case 0:
                ret = ioctl(fd, GEC6818_ADC_IN0, &adc_vol);
                break;
            case 1:
                ret = ioctl(fd, GEC6818_ADC_IN1, &adc_vol);
                break;
            case 2:
                ret = ioctl(fd, GEC6818_ADC_IN2, &adc_vol);
                break;
            case 3:
                ret = ioctl(fd, GEC6818_ADC_IN3, &adc_vol);
                break;
            default:
                printf("Invalid channel! Must be 0~3.\n");
                break;
        }

        // 检查 ioctl 是否成功
        if (ret != 0) {
            perror("ioctl failed");
            usleep(50 * 1000);
            continue;
        }

        // 输出结果
        printf("ADC Channel %d Voltage: %u mV\n", channel, adc_vol);

        sleep(1);
    }

    close(fd);
    return 0;
}



/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-10-02, lium
 * describe: 初始创建.
 *************************************************************************/
