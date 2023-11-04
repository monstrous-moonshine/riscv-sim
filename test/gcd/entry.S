.section .text.init
.global _entry
_entry:
    la sp, 0x80000000 + 128 * 1024 * 1024
    call main
spin:
    j spin
