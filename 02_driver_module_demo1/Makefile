# 在makefile中变量要大写

# 判断逗号两边的变量是否相等, 首次进入该文件所以 $(KERNELRELEASE) == nullptr
ifneq ($(KERNELRELEASE),)

obj-m := module.o

else

#指定内核所在位置
KERNELDIR := /home/scholar/test/6818GEC/kernel

# 指定目标平台 Platform
PLATFORM := arm

# 制定交叉编译路径
CROSS_COMPILE := /home/scholar/test/6818GEC/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-

# 获取当前源码路径
PWD := $(shell pwd)

default:
	$(MAKE)	ARCH=$(PLATFORM) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.order .*.cmd *.mod.c *.symvers *.ko

endif