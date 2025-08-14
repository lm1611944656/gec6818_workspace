#define     WTCON           0xC0019000     // 看门狗的物理地址
#define     GPIOxALTFN      0xC001E020     // 复用功能寄存器的物理地址
#define     GPIOEOUTENB     0xC001E004     // GPIOE的输出使能寄存器的物理地址
#define     GPIOEOUT        0xC001E000     // GPIOE的输出数据寄存器的物理地址


.global _start

_start:
    // 关闭看门狗
    LDR R0, =WTCON
    LDR R1, [R0]            // R1获取0xC0019000地址内的值
    BIC R1, R1, #(1<<5)     // WTCON[5] = 0;  取出R1的值，设置立即数，最后放回R1中
    STR R1, [R0]            // 0xC0019000地址内的值 = R1

    // 复用功能寄存器设置  GPIOEALTFN[27:26] = 00
    LDR R0, =GPIOxALTFN
    LDR R1, [R0]
    BIC R1, R1, #(3<<26)
    STR R1, [R0]

    // GPIOE的输出使能寄存器设置 GPIOEOUTENB[13] = 1
    LDR R0, =GPIOEOUTENB
    LDR R1, [R0]
    ORR R1, R1, #(1<<13)
    STR R1, [R0]

loop:
    // GPIOE的输出数据寄存器设置 GPIOEOUT[13] = 1
    LDR R0, =GPIOEOUT
    LDR R1, [R0]
    ORR R1, R1, #(1<<13)
    STR R1, [R0]

    // 延时
    MOV R0, #0xFF00000
    MOV R1, #0x50000
    bl mdelay       // 跳到c语言的cdelay函数去执行，执行完毕在跳转回来

    // GPIOE的输出数据寄存器设置 GPIOEOUT[13] = 0
    LDR R0, =GPIOEOUT
    LDR R1, [R0]
    BIC R1, R1, #(1<<13)
    STR R1, [R0]

    // 延时
    MOV R0, #0xFF00000
    MOV R1, #0x50000
    bl mdelay       // 跳到c语言的cdelay函数去执行，执行完毕在跳转回来

    // 跳转到循环
    b loop

