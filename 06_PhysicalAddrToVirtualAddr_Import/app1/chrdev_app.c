#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define LED_DEVICE "/dev/SelfDeviceName"

/**
 * @brief 初始化 LED 设备
 * @param device_path 设备文件路径
 * @return 成功返回文件描述符（>=0），失败返回 -1
 */
static int led_init(const char *device_path)
{
    int fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("open");
        fprintf(stderr, "Failed to open device: %s\n", device_path);
        return -1;
    }
    printf("Application: opened %s successfully\n", device_path);
    return fd;
}

/**
 * @brief 打开 LED（向设备写入控制数据）
 * @param fd 文件描述符
 * @return 成功返回 0，失败返回 -1
 */
static int open_led(int fd)
{
    // 局部定义控制数据
    unsigned char buf[] = {1, 2, 3, 4, 5};
    ssize_t ret;

    while(1){

        /**打开LED灯 */
        buf[0] = '0';
        ret = write(fd, buf, sizeof(buf));
        if (ret < 0) {
            perror("write");
            fprintf(stderr, "Failed to write to device: %s\n", strerror(errno));
            return -1;
        }
        sleep(1);

        /**关闭LED灯 */
        buf[0] = '1';
        ret = write(fd, buf, sizeof(buf));
        if (ret < 0) {
            perror("write");
            fprintf(stderr, "Failed to write to device: %s\n", strerror(errno));
            return -1;
        }
        sleep(1);
    }

    printf("Wrote %zd bytes to device\n", ret);
    return 0;
}

/**
 * @brief 关闭 LED 设备
 * @param fd 文件描述符
 * @param device_path 设备路径（用于打印）
 */
static void close_led(int fd, const char *device_path)
{
    if (fd >= 0) {
        close(fd);
        printf("Application: closed %s successfully\n", device_path);
    }
}

/**
 * @brief 主函数
 */
int main(void)
{
    int fd;

    // Step 1: 初始化（打开设备）
    fd = led_init(LED_DEVICE);
    if (fd < 0) {
        fprintf(stderr, "LED initialization failed\n");
        return EXIT_FAILURE;
    }

    // Step 2: 控制 LED（写入数据）
    if (open_led(fd) != 0) {
        fprintf(stderr, "Failed to open LED\n");
        close_led(fd, LED_DEVICE);  // 确保关闭
        return EXIT_FAILURE;
    }

    // Step 3: 关闭设备
    close_led(fd, LED_DEVICE);

    return EXIT_SUCCESS;
}