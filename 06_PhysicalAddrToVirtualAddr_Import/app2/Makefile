# 在makefile中变量要大写
INSTALLDIR := /home/scholar/tftp/

default:
	arm-linux-gcc -o zsf11 main.c
	cp --target-dir=$(INSTALLDIR) ./zsf11
clean:
	@rm -rf ./zsf