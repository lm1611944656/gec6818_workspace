# 目标: 依赖
ledSC.bin: LedS.o cdelay.o
	arm-linux-ld -Ttext 0x40000000 -o ledSC.elf $^
	arm-linux-objcopy -O binary ledSC.elf ledSC.bin
	arm-linux-objdump -D ledSC.elf > ledSC_elf.dis

%.o: %.S
	arm-linux-gcc -o $@ $< -c -nostdlib

%.o: %.c
	arm-linux-gcc -o $@ $< -c -nostdlib

clean:
	rm *.o *.elf *.bin *.dis -f

