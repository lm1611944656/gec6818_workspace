#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

// 定义一个字符数组 buf，初始化为 {1, 2, 3, 4, 5}
char buf[5] = {1, 2, 3, 4, 5};

int main()
{
    int fd; // 文件描述符
    ssize_t ret; // 使用 ssize_t 类型来存储 write 返回值，因为它是有符号的

    // 打开设备文件 /dev/DevNameTest，使用 O_RDWR 模式（读写）
    fd = open("/dev/DevNameTest", O_RDWR);
    if (fd < 0)
    {
        perror("open:"); // 输出错误信息
        return -1;
    }

    printf("application workspace open /dev/DevNameTest is ok\n");

    // 将 buf 写入设备文件
    ret = write(fd, buf, sizeof(buf));
    if (ret < 0)
    {
        perror("write:"); // 输出错误信息
        return -1;
    }
    else
    {
        printf("wrote %zd bytes to device\n", ret); // 输出实际写入的字节数
    }

    // 关闭文件描述符
    close(fd);
    printf("application workspace close /dev/DevNameTest is ok\n");
    return 0;
}