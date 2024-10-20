RAMBASE  = 0x80801000; /* Used to hold any new symbols */
PATCH_RAM_END = 0x81000000; /* Amount of extra ram allocated by recomp */

MEMORY {
    extram : ORIGIN = RAMBASE, LENGTH = 64M
    rom : ORIGIN = 0, LENGTH = 64M
}

SECTIONS {
    .ctors         : { *(.ctors*) *(.init_array*)                } >extram AT >rom
    .dtors         : { *(.dtors*)                                } >extram AT >rom
    .text          : { *(.text*)                                 } >extram AT >rom
    .recomp_patch  : { *(.recomp_patch*) *(.recomp_force_patch*) } >extram AT >rom
    .recomp_export : { *(.recomp_export*)                        } >extram AT >rom
    .recomp_event  : { *(.recomp_event*)                         } >extram AT >rom
    .rodata        : { *(.rodata*)                               } >extram AT >rom
    .data          : { *(.data*)                                 } >extram AT >rom
    .bss  (NOLOAD) : { *(.bss*)  *(COMMON)                       } >extram
    ASSERT(. <= PATCH_RAM_END, "Maxed out recomp extra ram")
    
    .reloc 0 : { *(.reloc*) }
    .symtab 0 : { *(.symtab) }
    .strtab 0 : { *(.strtab) }
    .shstrtab 0 : { *(.shstrtab) }

    /DISCARD/ : { *(*); }
}
