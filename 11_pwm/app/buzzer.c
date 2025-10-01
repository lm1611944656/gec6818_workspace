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
 *             使用方法(人耳能听到的频率为20~20000Hz)：
 *              ./zsf11 on 20           # 打开蜂鸣器，频率为200Hz
 *              ./zsf11 on 2000         # 打开蜂鸣器，频率为2000Hz
 *              ./zsf11 on 20000        # 打开蜂鸣器，频率为20000Hz
 *              ./zsf11 on 500          # 打开蜂鸣器，频率为20000Hz
 *              ./zsf11 off
 *          
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#define PWM_MAGIC 'x' // 定义幻数
#define PWM_MAX_NR 2  // 定义命令的最大序数

// 生成命令
#define IOCTL_BEEP_OFF _IOW(PWM_MAGIC, 0, unsigned long)         // 第0条命令
#define IOCTL_BEEP_ON _IOW(PWM_MAGIC, 1, unsigned long)          // 第1条命令
 

void Usage(char *args)
{
    printf("Usage: %s <on/off> <freq>\n", args);
}


int main(int argc, char **argv)
{
    int buzzer_fd;
    unsigned long freq;
    char *endstr, *str;

    if(argc == 3){
        buzzer_fd = open("/dev/pwm", O_RDWR);
        if(buzzer_fd < 0){
            perror("open device:");
            exit(1);
        }

        str = argv[2];
        errno = 0;

        freq = strtol(str, &endstr, 0);
        if((errno == ERANGE && (freq == LONG_MAX || freq == LONG_MIN)) || (errno != 0 && freq == 0)){
            perror("freq:");
            exit(EXIT_FAILURE);
        }

        if(endstr == str){
            fprintf(stderr, "please input digits for freq\n");
            exit(EXIT_FAILURE);
        } 
        
        if(!strncmp(argv[1], "on", 2)){
            ioctl(buzzer_fd, IOCTL_BEEP_ON, freq);
        } else if(!strncmp(argv[1], "off", 3)){
            ioctl(buzzer_fd, IOCTL_BEEP_OFF, freq);
        } else {
            close(buzzer_fd);
            exit(EXIT_FAILURE);
        }
    
    } else if(argc == 2){
        buzzer_fd = open("/dev/pwm", O_RDWR);
        if(buzzer_fd < 0){
            perror("open device:");
            exit(1);
        }

        if(!strncmp(argv[1], "off", 3)){
            ioctl(buzzer_fd, IOCTL_BEEP_OFF, freq);
        } else {
            close(buzzer_fd);
            exit(EXIT_FAILURE);
        }

    } else {
        Usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    close(buzzer_fd);

    return 0;
}

/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-10-1, lium
 * describe: 初始创建.
 *************************************************************************/