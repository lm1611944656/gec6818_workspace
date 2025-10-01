/*************************************************************************
 *
 *   Copyright (C), 2017-2037, BPG. Co., Ltd.
 *
 *   文件名称: chrdev.c
 *   软件模块: 字符设备驱动
 *   版 本 号: 1.0
 *   生成日期: 2025-09-08
 *   作    者: lium
 *   功    能:  任务A：每隔2秒读取一次数据
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

#define PIR_DEVICE "/dev/PIR"

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
void task_read_pir(TTaskControlBlock_t *t) {
    unsigned char buf[1] = {'0'}; 
    ssize_t ret = read(g_led_fd, buf, 1);
    if (ret < 0) {
        perror("read failed");
        fprintf(stderr, "read data failed from PIR \n");
    } else {
        printf("application layer read data success from PIR %c \n", buf[0]);
    }
}

int main() {
    // 1. 打开设备
    g_led_fd = open(PIR_DEVICE, O_RDWR);
    if (g_led_fd < 0) {
        perror("open");
        fprintf(stderr, "Failed to open device: %s\n", PIR_DEVICE);
        return EXIT_FAILURE;
    }
    printf("Application: opened %s successfully\n", PIR_DEVICE);

    // 2. 初始化任务调度器
    struct heap timer_heap;
    heap_init(&timer_heap);

    // 3. 创建PIR任务
    TTaskControlBlock_t task_on = {0};
    task_on.expire_time = get_current_time_ms();
    task_on.interval     = 2000;
    strcpy(task_on.name, "PIR_TASK");
    task_on.callback     = task_read_pir;  // 类型匹配

    // 5. 插入任务
    heap_insert(&timer_heap, &task_on.node, timer_less_than);

    printf("PIR_TASK blinking started (1Hz). Press Ctrl+C to stop.\n");

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