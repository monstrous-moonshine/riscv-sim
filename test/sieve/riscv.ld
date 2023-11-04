OUTPUT_ARCH("riscv")
ENTRY(_entry)

SECTIONS
{
    . = 0x80000000;

    .text.init : {
        *(.text.init .text.init.*)
    }

    .text : {
        *(.text .text.*)
    }

    . = ALIGN(CONSTANT(COMMONPAGESIZE));
    
    .data : {
        . = ALIGN(16);
        *(.sdata .sdata.*)
        . = ALIGN(16);
        *(.data .data.*)
    }

    .bss : {
        . = ALIGN(16);
        *(.sbss .sbss.*)
        . = ALIGN(16);
        *(.bss .bss.*)
    }

    PROVIDE(end = .);
}
