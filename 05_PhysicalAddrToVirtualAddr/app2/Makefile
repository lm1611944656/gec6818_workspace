# 在makefile中变量要大写
INSTALLDIR := /home/scholar/tftp/

default:
	arm-linux-gcc -o zsf main.c
	cp --target-dir=$(INSTALLDIR) ./zsf
clean:
	@rm -rf ./zsf