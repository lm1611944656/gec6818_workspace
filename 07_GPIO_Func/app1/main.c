/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: chrdev.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-09-08
 *   作    者: lium
 *   功    能:  任务A：在 t=0, 2s, 4s, 6s... 执行 → 开灯
 *              任务B：在 t=1s, 3s, 5s, 7s... 执行 → 关灯
 *              每秒闪烁一次，且完全不用 sleep，靠主循环轮询当前时间触发任务。
 ************************************************************************/
#include "heap_inl.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#define LED_DEVICE "/dev/LED4"

// 全局文件描述符
static int g_led_fd = -1;

// container_of 宏
#ifndef container_of
# define container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - (size_t)&((type*)0)->member))
#endif

/**任务结构体 */
typedef struct TTaskControlBlock_t {
    struct heap_node node;
    unsigned int expire_time;    /**初始时执行任务的时刻 */
    unsigned int interval;       /**下一次执行任务的时刻 */
    char name[32];               /**执行任务的名称 */     
    void (*callback)(struct TTaskControlBlock_t*);
} TTaskControlBlock_t;

// 堆比较函数
int timer_less_than(const struct heap_node *a, const struct heap_node *b) {
    const TTaskControlBlock_t *ta = container_of(a, TTaskControlBlock_t, node);
    const TTaskControlBlock_t *tb = container_of(b, TTaskControlBlock_t, node);
    return ta->expire_time < tb->expire_time;
}

// 获取当前时间（毫秒）
unsigned int get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned int)(ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL);
}

// 重新调度周期性任务
void reschedule_timer(struct heap *heap, TTaskControlBlock_t *timer, unsigned int current_time) {
    heap_remove(heap, &timer->node, timer_less_than);
    timer->expire_time = current_time + timer->interval;
    heap_insert(heap, &timer->node, timer_less_than);
}

// --- 回调函数 ---
void task_led0_on(TTaskControlBlock_t *t) {
    unsigned char buf[] = "00"; // 第0盏：状态为0
    ssize_t ret = write(g_led_fd, &buf, 2);
    if (ret < 0) {
        perror("write [ON]");
        fprintf(stderr, "Failed to turn on LED\n");
    } else {
        printf("[ON]  LED turned ON at %d ms\n", get_current_time_ms());
    }
}

void task_led0_off(TTaskControlBlock_t *t) {
    unsigned char buf[] = "01";  // 第0盏：状态为1
    ssize_t ret = write(g_led_fd, &buf, 2);
    if (ret < 0) {
        perror("write [OFF]");
        fprintf(stderr, "Failed to turn off LED\n");
    } else {
        printf("[OFF] LED turned OFF at %d ms\n", get_current_time_ms());
    }
}

void task_led1_on(TTaskControlBlock_t *t) {
    unsigned char buf[] = "10";     // 第1盏：状态为0
    ssize_t ret = write(g_led_fd, &buf, 2);
    if (ret < 0) {
        perror("write [ON]");
        fprintf(stderr, "Failed to turn on LED\n");
    } else {
        printf("[ON]  LED turned ON at %d ms\n", get_current_time_ms());
    }
}

void task_led1_off(TTaskControlBlock_t *t) {
    unsigned char buf[] = "11";     // 第1盏：状态为1
    ssize_t ret = write(g_led_fd, &buf, 2);
    if (ret < 0) {
        perror("write [OFF]");
        fprintf(stderr, "Failed to turn off LED\n");
    } else {
        printf("[OFF] LED turned OFF at %d ms\n", get_current_time_ms());
    }
}

void task_led2_on(TTaskControlBlock_t *t) {
    unsigned char buf[] = "20";     // 第2盏：状态为0
    ssize_t ret = write(g_led_fd, &buf, 2);
    if (ret < 0) {
        perror("write [ON]");
        fprintf(stderr, "Failed to turn on LED\n");
    } else {
        printf("[ON]  LED turned ON at %d ms\n", get_current_time_ms());
    }
}

void task_led2_off(TTaskControlBlock_t *t) {
    unsigned char buf[] = "21";     // 第2盏：状态为1
    ssize_t ret = write(g_led_fd, &buf, 2);
    if (ret < 0) {
        perror("write [OFF]");
        fprintf(stderr, "Failed to turn off LED\n");
    } else {
        printf("[OFF] LED turned OFF at %d ms\n", get_current_time_ms());
    }
}

void task_led3_on(TTaskControlBlock_t *t) {
    unsigned char buf[] = "30";     // 第3盏：状态为0
    ssize_t ret = write(g_led_fd, &buf, 2);
    if (ret < 0) {
        perror("write [ON]");
        fprintf(stderr, "Failed to turn on LED\n");
    } else {
        printf("[ON]  LED turned ON at %d ms\n", get_current_time_ms());
    }
}

void task_led3_off(TTaskControlBlock_t *t) {
    unsigned char buf[] = "31";     // 第3盏：状态为1
    ssize_t ret = write(g_led_fd, &buf, 2);
    if (ret < 0) {
        perror("write [OFF]");
        fprintf(stderr, "Failed to turn off LED\n");
    } else {
        printf("[OFF] LED turned OFF at %d ms\n", get_current_time_ms());
    }
}

int main() {
    // 1. 打开设备
    g_led_fd = open(LED_DEVICE, O_RDWR);
    if (g_led_fd < 0) {
        perror("open");
        fprintf(stderr, "Failed to open device: %s\n", LED_DEVICE);
        return EXIT_FAILURE;
    }
    printf("Application: opened %s successfully\n", LED_DEVICE);

    // 2. 初始化任务调度器
    struct heap timer_heap;
    heap_init(&timer_heap);

    // 3. 创建Led0任务
    TTaskControlBlock_t task_on = {0};
    task_on.expire_time = get_current_time_ms();
    task_on.interval     = 2000;
    strcpy(task_on.name, "LED0_ON");
    task_on.callback     = task_led0_on;  // 类型匹配
    TTaskControlBlock_t task_off = {0};
    task_off.expire_time = get_current_time_ms() + 1000;
    task_off.interval    = 2000;
    strcpy(task_off.name, "LED0_OFF");
    task_off.callback    = task_led0_off;  // 类型匹配

    /**任务2 */
    TTaskControlBlock_t task2_on = {0};
    task2_on.expire_time = get_current_time_ms();
    task2_on.interval     = 2000;
    strcpy(task2_on.name, "LED1_ON");
    task2_on.callback     = task_led1_on;  // 类型匹配
    TTaskControlBlock_t task2_off = {0};
    task2_off.expire_time = get_current_time_ms() + 1000;
    task2_off.interval    = 2000;
    strcpy(task2_off.name, "LED1_OFF");
    task2_off.callback    = task_led1_off;  // 类型匹配

    /**任务3 */
    TTaskControlBlock_t task3_on = {0};
    task3_on.expire_time = get_current_time_ms();
    task3_on.interval     = 1000;
    strcpy(task3_on.name, "LED2_ON");
    task3_on.callback     = task_led2_on;  // 类型匹配
    TTaskControlBlock_t task3_off = {0};
    task3_off.expire_time = get_current_time_ms() + 1000;
    task3_off.interval    = 1000;
    strcpy(task3_off.name, "LED2_OFF");
    task3_off.callback    = task_led2_off;  // 类型匹配

    /**任务4 */
    TTaskControlBlock_t task4_on = {0};
    task4_on.expire_time = get_current_time_ms();
    task4_on.interval     = 1000;
    strcpy(task4_on.name, "LED3_ON");
    task4_on.callback     = task_led3_on;  // 类型匹配
    TTaskControlBlock_t task4_off = {0};
    task4_off.expire_time = get_current_time_ms() + 1000;
    task4_off.interval    = 1000;
    strcpy(task4_off.name, "LED3_OFF");
    task4_off.callback    = task_led3_off;  // 类型匹配

    // 5. 插入任务
    heap_insert(&timer_heap, &task_on.node, timer_less_than);
    heap_insert(&timer_heap, &task_off.node, timer_less_than);
    heap_insert(&timer_heap, &task2_on.node, timer_less_than);
    heap_insert(&timer_heap, &task2_off.node, timer_less_than);
    heap_insert(&timer_heap, &task3_on.node, timer_less_than);
    heap_insert(&timer_heap, &task3_off.node, timer_less_than);
    heap_insert(&timer_heap, &task4_on.node, timer_less_than);
    heap_insert(&timer_heap, &task4_off.node, timer_less_than);

    printf("LED blinking started (1Hz). Press Ctrl+C to stop.\n");

    // 6. 主循环
    while (1) {
        unsigned int current_time = get_current_time_ms();

        while (heap_min(&timer_heap) != NULL) {
            TTaskControlBlock_t *timer = container_of(heap_min(&timer_heap), TTaskControlBlock_t, node);
            if (timer->expire_time <= current_time) {
                timer->callback(timer);  
                reschedule_timer(&timer_heap, timer, current_time);
            } else {
                break;
            }
        }
    }

    close(g_led_fd);
    return 0;
}
/*************************************************************************
 * 改动历史纪录：
 * Revision 1.0, 2025-08-31, lium
 * describe: 初始创建.
 *************************************************************************/