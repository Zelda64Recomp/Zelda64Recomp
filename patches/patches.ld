RAMBASE  = 0x80800100; /* Used to hold any new symbols */

MEMORY {
    extram : ORIGIN = RAMBASE, LENGTH = 1M
    rom : ORIGIN = 0, LENGTH = 1M
}

SECTIONS {
    .text          : { *(.text*)                  } >extram AT >rom
    .ctors         : { *(.ctors*) *(.init_array*) } >extram AT >rom
    .dtors         : { *(.dtors*)                 } >extram AT >rom
    .rodata        : { *(.rodata*)                } >extram AT >rom
    .data          : { *(.data*)                  } >extram AT >rom
    .bss  (NOLOAD) : { *(.bss*)  *(COMMON)        } >extram
    
    .symtab 0 : { *(.symtab) }
    .strtab 0 : { *(.strtab) }
    .shstrtab 0 : { *(.shstrtab) }

    /DISCARD/ : { *(*); }
}
