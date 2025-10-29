/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: btn_dev.c
 *   软件模块: platform总线应用层
 *   版 本 号: 1.0
 *   生成日期: 2025-10-05
 *   作    者: lium
 *   功    能: platform总线设备
 *
 ************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>


#define DEV_NAME		"/dev/gecBt"                // 设备名字 /dev/DEV_NAME
#define BTN_SIZE            4                       /**按键的数量 */


int main(int argc, char **argv){
    int button_fd, ret;
    char prior_button_value[BTN_SIZE] = {'0', '0', '0', '0'};          // 按键的最初状态
    char current_button_value[BTN_SIZE] = {'0', '0', '0', '0'};        // 保存当前按键的状态
    
    /**1. 打开设备文件 */
    button_fd = open(DEV_NAME, O_RDWR);  // 打开设备
    if (button_fd < 0) {
        perror("open device");
        exit(1);
    }
    
    // 主循环：持续读取按钮状态
    while (1) {
        int i;

        // 从设备读取当前按钮值
        ret = read(button_fd, current_button_value, BTN_SIZE);
        if (ret != BTN_SIZE) {
            perror("read buttons");
            exit(1);
        }

        // 遍历每个按钮位，判断是否发生变化
        for (i = 0; i < BTN_SIZE; i++) {
            // 如果当前值与上一次不同，说明按键有动作
            if (prior_button_value[i] != current_button_value[i]) {
                prior_button_value[i] = current_button_value[i];  // 更新旧值

                printf("\n************************************\n");

                switch (i) {
                    case 0:
                        printf("K1 \t%s\n", current_button_value[i] == '0' ? "Release, up" : "Pressed, down");
                        break;
                    case 1:
                        printf("K2 \t%s\n", current_button_value[i] == '0' ? "Release, up" : "Pressed, down");
                        break;
                    case 2:
                        printf("K3 \t%s\n", current_button_value[i] == '0' ? "Release, up" : "Pressed, down");
                        break;
                    case 3:
                        printf("K4 \t%s\n", current_button_value[i] == '0' ? "Release, up" : "Pressed, down");
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // 理论上不会执行到这里，但为了防止警告可以加上
    close(button_fd);
    return 0;
}


/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-10-05, lium
 * describe: 初始创建.
 *************************************************************************/