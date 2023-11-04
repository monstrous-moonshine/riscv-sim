CC = riscv64-elf-gcc -march=rv32i -mabi=ilp32
CFLAGS = -ffreestanding -O2
LDFLAGS = -nostdlib -T riscv.ld
LDLIBS = -lgcc

PROG = sieve

$(PROG): $(PROG).o entry.o

dump.s: $(PROG)
	riscv64-elf-objdump -d $< > $@

clean:
	$(RM) $(PROG).o entry.o $(PROG) dump.s
