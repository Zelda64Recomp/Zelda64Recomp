RAMBASE  = 0x80800100; /* Used to hold any new symbols */
EXTRA_RAM_SIZE = 0x01000000; /* Amount of extra ram allocated by recomp */

MEMORY {
    extram : ORIGIN = RAMBASE, LENGTH = 64M
    rom : ORIGIN = 0, LENGTH = 64M
}

SECTIONS {
    .ctors         : { *(.ctors*) *(.init_array*) } >extram AT >rom
    .dtors         : { *(.dtors*)                 } >extram AT >rom
    .rodata        : { *(.rodata*)                } >extram AT >rom
    .data          : { *(.data*)                  } >extram AT >rom
    
    /* The following sections will be removed from the objcopy */
    /* bss isn't noload to make .text rom addresses valid for the recompiler */
    .bss           : { *(.bss*)  *(COMMON)        } >extram AT >rom
    ASSERT(. < RAMBASE + EXTRA_RAM_SIZE, "Maxed out recomp extra ram")
    /* Padding to push .text to avoid conflicts with original function addresses */
    .pad           : { . += 0x1000000;            } >extram AT >rom
    .text          : { *(.text*)                  } >extram AT >rom
    
    .symtab 0 : { *(.symtab) }
    .strtab 0 : { *(.strtab) }
    .shstrtab 0 : { *(.shstrtab) }

    /DISCARD/ : { *(*); }
}
