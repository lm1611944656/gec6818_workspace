# 在makefile中变量要大写
INSTALLDIR := /home/scholar/tftp/

default:
	arm-linux-gcc -o bbbs chrdev_app.c
	cp --target-dir=$(INSTALLDIR) ./bbbs
	file ./bbbs
clean:
	@rm -rf ./bbbs