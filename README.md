This is a simple instruction set simulator for riscv. Currently it supports the RV32I base instruction set.

## How to use
### Building
You'll need a riscv toolchain to build the test programs. The major Linux distributions provide binary packages for this. In Arch Linux, it's `riscv64-elf-gcc`. This package also pulls `riscv64-elf-binutils` as a dependency, and these two are all that's required to build freestanding programs. Other distributions will have similar packages.

After checking out the repository, you can build the host program (the simulator) and the test programs by:
```shell
$ make
$ make -C test/gcd
$ make -C test/sieve
$ make -C test/sqrt
```

You can add more test programs by copying the helper files (start routine, linker script, and makefile) from an existing example and adding your own source.
### Running
The simulator provides 1MB memory starting from address `0x80000000`, which is also where program execution starts. The linker scripts that come with the examples place a special start routine called `_entry` at this address. This routine initializes the stack pointer just past the 1MB memory end, calls `main` with the stack thus set up, and loops forever when `main` returns. The simulates detects the infinite loop and exits.

To step through the target program execution, it'd be helpful to have a disassembly of it at hand. This can be created with:
```shell
$ make -C test/sqrt dump.s
```
This creates a disassembly at `test/sqrt/dump.s`. Look at a [sample interaction](docs/sample_interaction.txt) to get a feel for how it's used.
