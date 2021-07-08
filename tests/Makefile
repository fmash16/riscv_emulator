test.bin: test.c
	/opt/riscv/bin/riscv64-unknown-elf-gcc -S test.c
	/opt/riscv/bin/riscv64-unknown-elf-gcc -Wl,-Ttext=0x0 -nostdlib -march=rv64i -mabi=lp64 -o test test.s
	/opt/riscv/bin/riscv64-unknown-elf-objcopy -O binary test test.bin

clean:
	rm -f test
	rm -f test.bin
