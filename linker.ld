ENTRY(_start)
SECTIONS
{
    . = 0x40000000;
    .text : { *(.text.boot) *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
    
    . = ALIGN(16);
    . += 0x4000; /* 16KB space */
    _stack_top = .;
}