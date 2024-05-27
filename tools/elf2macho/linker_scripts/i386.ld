OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
    . = SEGMENT_START("text-segment", 0x4000000) + 0x360;
    /* text sections collectively */
    .text : {
        *(.text .text.*)
    }
    /* rodata sections collectively */
    .rodata ALIGN(0x1000) : {
        *(.rodata .rodata.*)
    }
    /* data sections collectively */
    .data ALIGN(0x1000) : {
        *(.data .data.*)
    }
    .preinit_array : {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    }
    .init_array : {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP (*(SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP (*(.init_array .ctors))
        PROVIDE_HIDDEN (__init_array_end = .);
    }
    .fini_array : {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP (*(SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP (*(.fini_array .dtors))
        PROVIDE_HIDDEN (__fini_array_end = .);
    }
    /* bss section */
    .bss ALIGN(0x1000) : {
        *(.bss .bss.*)
    }
    /DISCARD/ : {
    }
}
