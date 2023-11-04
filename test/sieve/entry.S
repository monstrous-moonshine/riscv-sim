#include "../../common.hpp"
.section .text.init
.global _entry
_entry:
    la sp, ADDR_BASE + MEM_SIZE
    call main
spin:
    j spin
