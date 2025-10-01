# 在makefile中变量要大写
INSTALLDIR := /home/scholar/tftp/

# 目标文件
TARGET := zsf11

# 遍历源文件
SRCS = $(wildcard *.c)

# 文件转换(*.c  -->  *.o)
OBJS = $(patsubst %.c, %.o, $(SRCS))

# 编译器
CC = arm-linux-gcc

# 目标:依赖
%.o:%.c
	$(CC) -o $@ -c $<							# $@ 表示目标文件；$<表示第一个依赖文件

$(TARGET):$(OBJS)
	$(CC) -o $@  $^								# $@ 表示目标文件；$^表示所有的依赖文件
	cp --target-dir=$(INSTALLDIR) $(TARGET)

clean:
	rm -rf *.o 