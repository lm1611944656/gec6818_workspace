# 在makefile中变量要大写
INSTALLDIR := /home/scholar/tftp/

default:
	arm-linux-gcc -o zsf07 main.c
	cp --target-dir=$(INSTALLDIR) ./zsf07
clean:
	@rm -rf ./zsf07