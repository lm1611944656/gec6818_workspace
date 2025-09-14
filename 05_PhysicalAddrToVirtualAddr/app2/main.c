/**
 * 任务A：在 t=0, 2s, 4s, 6s... 执行 → 开灯
 * 任务B：在 t=1s, 3s, 5s, 7s... 执行 → 关灯
 * 每秒闪烁一次，且完全不用 sleep，靠主循环轮询当前时间触发任务。
 */
#include "heap_inl.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#define LED_DEVICE "/dev/SelfDeviceName"

// 全局文件描述符
static int g_led_fd = -1;

// container_of 宏
#ifndef container_of
# define container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - (size_t)&((type*)0)->member))
#endif

// 提前定义结构体类型（关键！）
typedef struct TTaskControlBlock_t {
    struct heap_node node;
    int expire_time;    // 下次执行时间（ms）
    int interval;       // 周期间隔（ms）
    char name[32];
    void (*callback)(struct TTaskControlBlock_t*);
} TTaskControlBlock_t;

// 堆比较函数
int timer_less_than(const struct heap_node *a, const struct heap_node *b) {
    const TTaskControlBlock_t *ta = container_of(a, TTaskControlBlock_t, node);
    const TTaskControlBlock_t *tb = container_of(b, TTaskControlBlock_t, node);
    return ta->expire_time < tb->expire_time;
}

// 获取当前时间（毫秒）
int get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int)(ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL);
}

// 重新调度周期性任务
void reschedule_timer(struct heap *heap, TTaskControlBlock_t *timer, int current_time) {
    heap_remove(heap, &timer->node, timer_less_than);
    timer->expire_time = current_time + timer->interval;
    heap_insert(heap, &timer->node, timer_less_than);
}

// --- 回调函数 ---
void task_turn_on_led(TTaskControlBlock_t *t) {
    unsigned char buf = '0';
    ssize_t ret = write(g_led_fd, &buf, 1);
    if (ret < 0) {
        perror("write [ON]");
        fprintf(stderr, "Failed to turn on LED\n");
    } else {
        printf("[ON]  LED turned ON at %d ms\n", get_current_time_ms());
    }
}

void task_turn_off_led(TTaskControlBlock_t *t) {
    unsigned char buf = '1';
    ssize_t ret = write(g_led_fd, &buf, 1);
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

    // 3. 创建“开灯”任务
    TTaskControlBlock_t task_on = {0};
    task_on.expire_time = get_current_time_ms();
    task_on.interval     = 2000;
    strcpy(task_on.name, "LED_ON");
    task_on.callback     = task_turn_on_led;  // 类型匹配

    // 4. 创建“关灯”任务
    TTaskControlBlock_t task_off = {0};
    task_off.expire_time = get_current_time_ms() + 1000;
    task_off.interval    = 2000;
    strcpy(task_off.name, "LED_OFF");
    task_off.callback    = task_turn_off_led;  // 类型匹配

    // 5. 插入任务
    heap_insert(&timer_heap, &task_on.node, timer_less_than);
    heap_insert(&timer_heap, &task_off.node, timer_less_than);

    printf("LED blinking started (1Hz). Press Ctrl+C to stop.\n");

    // 6. 主循环
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

        // usleep(1000);  // 可选：降低 CPU 占用
    }

    close(g_led_fd);
    return 0;
}