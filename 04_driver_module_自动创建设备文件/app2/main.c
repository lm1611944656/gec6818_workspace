#include "heap_inl.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>  // usleep
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define LED_DEVICE "/dev/SelfDeviceName"

// 全局文件描述符，供所有任务回调使用
static int g_led_fd = -1;  // 初始化为 -1，表示未打开

/**
 * @brief 初始化 LED 设备
 * @param device_path 设备文件路径
 * @return 成功返回 0，失败返回 -1
 */
static int led_init(const char *device_path)
{
    g_led_fd = open(device_path, O_RDWR);
    if (g_led_fd < 0) {
        perror("open");
        fprintf(stderr, "Failed to open device: %s\n", device_path);
        return -1;
    }
    printf("Application: opened %s successfully\n", device_path);
    return 0;
}

/**
 * @brief 关闭 LED 设备
 */
static void led_close(void)
{
    if (g_led_fd >= 0) {
        close(g_led_fd);
        g_led_fd = -1;
        printf("Application: closed LED device successfully\n");
    }
}

// 定义 container_of 宏
#ifndef container_of
# define container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - (size_t)&((type*)0)->member))
#endif

/* 任务调度结构体 */
typedef struct TTaskControlBlock_t {
    struct heap_node node;                              
    int expire_time;                                    
    int interval;
    char name[16];
    void (*callback)(struct TTaskControlBlock_t *);
} TTaskControlBlock_t;

// 比较函数
int timer_less_than(const struct heap_node *a, const struct heap_node *b) {
    const TTaskControlBlock_t *ta = container_of(a, TTaskControlBlock_t, node);
    const TTaskControlBlock_t *tb = container_of(b, TTaskControlBlock_t, node);
    return ta->expire_time < tb->expire_time;
}

// 任务A：写入 {1, 2, 3, 4, 5}
void task_a_callback(TTaskControlBlock_t *t) {
    const unsigned char buf[] = {1, 2, 3, 4, 5};
    ssize_t ret = write(g_led_fd, buf, sizeof(buf));
    if (ret < 0) {
        perror("write task_a");
        fprintf(stderr, "Failed to write to LED device\n");
    } else {
        printf("[A] Wrote {1,2,3,4,5} to LED, %zd bytes\n", ret);
    }
    //fflush(stdout);
}

// 任务B：写入 {11, 22, 33, 44, 55}
void task_b_callback(TTaskControlBlock_t *t) {
    const unsigned char buf[] = {11, 22, 33, 44, 55};
    ssize_t ret = write(g_led_fd, buf, sizeof(buf));
    if (ret < 0) {
        perror("write task_b");
        fprintf(stderr, "Failed to write to LED device\n");
    } else {
        printf("[B] Wrote {11,22,33,44,55} to LED, %zd bytes\n", ret);
    }
    //fflush(stdout);
}

// 任务C：写入 {6, 7, 8, 9, 10}
void task_c_callback(TTaskControlBlock_t *t) {
    const unsigned char buf[] = {6, 7, 8, 9, 10};
    ssize_t ret = write(g_led_fd, buf, sizeof(buf));
    if (ret < 0) {
        perror("write task_c");
        fprintf(stderr, "Failed to write to LED device\n");
    } else {
        printf("[C] Wrote {111,222,333,444,555} to LED, %zd bytes\n", ret);
    }
    //fflush(stdout);
}

// 重新调度周期任务
void reschedule_timer(struct heap *heap, TTaskControlBlock_t *timer, int current_time) {
    heap_remove(heap, &timer->node, timer_less_than);
    timer->expire_time = current_time + timer->interval;
    heap_insert(heap, &timer->node, timer_less_than);
}

// 获取当前时间（毫秒）
int get_current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int)(ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL);
}

int main() {
    // Step 1: 初始化 LED 设备
    if (led_init(LED_DEVICE) != 0) {
        fprintf(stderr, "LED initialization failed\n");
        return EXIT_FAILURE;
    }

    // Step 2: 初始化堆结构
    struct heap timer_heap;
    heap_init(&timer_heap);

    // Step 3: 创建定时器任务
    TTaskControlBlock_t t1 = {0};
    t1.expire_time = 1000;
    t1.interval = 1000;
    strcpy(t1.name, "LED_Write_12345");
    t1.callback = task_a_callback;

    TTaskControlBlock_t t2 = {0};
    t2.expire_time = 2000;
    t2.interval = 2000;
    strcpy(t2.name, "LED_Write_1122334455");
    t2.callback = task_b_callback;

    TTaskControlBlock_t t3 = {0};
    t3.expire_time = 1500;
    t3.interval = 1500;
    strcpy(t3.name, "LED_Write_111222333444555");
    t3.callback = task_c_callback;

    /**将任务插入到最小堆中 */
    heap_insert(&timer_heap, &t1.node, timer_less_than);
    heap_insert(&timer_heap, &t2.node, timer_less_than);
    heap_insert(&timer_heap, &t3.node, timer_less_than);

    printf("LED定时控制任务启动！按 Ctrl+C 停止。\n\n");
    fflush(stdout);

    // 主循环
    while (1) {
        int current_time = get_current_time_ms();

        while (heap_min(&timer_heap) != NULL) {
            TTaskControlBlock_t *timer = container_of(heap_min(&timer_heap), TTaskControlBlock_t, node);

            if (timer->expire_time <= current_time) {
                timer->callback(timer);
                reschedule_timer(&timer_heap, timer, current_time);
            } else {
                break;
            }
        }

        //usleep(10000); // 休眠 10ms
    }

    // 正常情况下不会到这里，但建议加信号处理
    led_close();
    return 0;
}