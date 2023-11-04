#ifndef COMMON_HPP
#define COMMON_HPP

#define ADDR_BASE 0x80000000
#define MEM_SIZE  (1 * 1024 * 1024)
#define ONES(n) ((1 << (n)) - 1)

#ifndef __ASSEMBLER__

static const char prog_name[] = "riscv_sim";

#endif

#endif
