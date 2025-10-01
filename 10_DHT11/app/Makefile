# 在makefile中变量要大写
INSTALLDIR := /home/scholar/tftp/

default:
	arm-linux-gcc -o zsf10 main.c
	cp --target-dir=$(INSTALLDIR) ./zsf10
clean:
	@rm -rf ./zsf10