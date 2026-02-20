/* linker.ld — LM3S6965 (QEMU target: -M lm3s6965evb) */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

/* Initial stack pointer = top of SRAM (stack grows downward) */
PROVIDE ( _estack = ORIGIN(SRAM) + LENGTH(SRAM) );

ENTRY(Reset_Handler)

SECTIONS
{

     .text :
    {
        KEEP(*(.isr_vector)) /* Vector table must be first in Flash */
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        _etext = .; /* End of Flash content */
 } > FLASH

    /* .data: initialised globals — stored in Flash, copied to SRAM at startup */
    .data : AT(_etext)
    {
        _sdata = .;
        *(.data)
        *(.data*)
        _edata = .;
    } > SRAM
 
    /* .bss: zero-initialised globals — just zeroed in SRAM, no Flash storage */
    .bss :
    {
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM
    
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        . = . + 0x400;
        . = ALIGN(8);
    } > SRAM
}