/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: chrdev.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-09-08
 *   作    者: lium
 *   功    能:  每2s钟读取DHT11的数据
 ************************************************************************/

#include <stdio.h>
#include <fcntl.h>          // O_RDWR
#include <unistd.h>         // close
#include <sys/ioctl.h>
#include <errno.h>

#define GET_DHT11_DATA _IOR('w', 0, unsigned long)

int main(int argc, char **argv)
{
    int ret;
    unsigned char buff[4] = {0};
    int fd = open("/dev/dht11", O_RDWR);
    if(fd < 0){
        perror("open dht11_dev driver");
        return -1;
    }

    while(1){
        printf("检测中\n");
        ret  = ioctl(fd, GET_DHT11_DATA, buff);
        if (ret != 0) {
            perror("GET_DHT11_DATA error");
        } else {
            printf("温度 = %hhu.%hhu, 湿度 = %hhu.%hhu  \n", buff[0], buff[1], buff[2], buff[3]);
        }

        sleep(2);
    }
    close(fd);
    return 0;
}


/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-09-20, lium
 * describe: 初始创建.
 *************************************************************************/