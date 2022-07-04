/**
 * @file
 *
 * @brief Linker script for executing from RAM (STM32MP15x M4 core)
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)
ENTRY(Reset_Handler)

/* Memory Spaces Definitions */
MEMORY
{
    /* .text and .rodata */
    retram      (rwx)   : ORIGIN = 0x00000000, LENGTH = 64K
    /* .data, .bss, and stacks */
    sram1       (rwx)   : ORIGIN = 0x10000000, LENGTH = 128K
    /* heap */
    sram2       (rwx)   : ORIGIN = 0x10020000, LENGTH = 128K
    /* IPC buffers */
    sram3       (rwx)   : ORIGIN = 0x10040000, LENGTH = 64K
    /* linux reserved */
    sram4       (rwx)   : ORIGIN = 0x10050000, LENGTH = 64K
}

/* program headers (for loading stuff) */
PHDRS {
    text        PT_LOAD;
    rodata      PT_LOAD;
    data        PT_LOAD;
    bss         PT_LOAD;
}

/*
 * Size of the stack
 *
 * This is only used at initialization, before the FreeRTOS scheduler is started; as well as for
 * interrupt handlers.
 */
STACK_SIZE = 0x800;

/* Symbols needed for OpenAMP to enable rpmsg */
__OPENAMP_region_start__ = ORIGIN(sram3);
__OPENAMP_region_end__   = ORIGIN(sram3) + LENGTH(sram3);

/* Section Definitions */
SECTIONS
{
    /* executable code and read-only data gets shoved into the same output section */
    .text :
    {
        KEEP(*(.isr_vector))
        KEEP(*(.vectors .vectors.*))

        .= ALIGN(4);
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7t) *(.glue_7)
    } >retram :text

    /* read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata .rodata* .gnu.linkonce.r.*)

        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)

        /* Support C constructors, and C destructors in both user code
           and the C library. This also provides support for C++ code. */
        . = ALIGN(4);
        KEEP(*(.init))
        . = ALIGN(4);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);

        . = ALIGN(4);
        KEEP(*(.fini))

        . = ALIGN(4);
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >retram :rodata

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    PROVIDE_HIDDEN(__exidx_start = .);
    .ARM.exidx :
    {
      *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } >retram :rodata
    PROVIDE_HIDDEN(__exidx_end = .);

    /* OpenAMP resource table */
    .resource_table :
    {
        . = ALIGN(4);
        KEEP (*(.resource_table*))
        . = ALIGN(4);
    } >retram :rodata

    . = ALIGN(4);
    _etext = .;

    /* read/write data */
    __DATA_ROM = .;
    _sidata = LOADADDR(.data);

    .data :
    {
        _sdata = .;
        *(.data .data*);
        . = ALIGN(4);
        _edata = .;
    } >sram1 AT> retram :data

    __DATA_END = __DATA_ROM + (_edata - _sdata);
    text_end = ORIGIN(retram) + LENGTH(retram);
    ASSERT(__DATA_END <= text_end, "region SRAM1_text overflowed with text and data")

    /* .bss section which is used for uninitialized data */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        _szero = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        _ezero = .;
    } >sram1 :bss

    /* stack section */
    .stack (NOLOAD):
    {
        . = ALIGN(16);
        _sstack = .;
        . = . + STACK_SIZE;
        . = ALIGN(16);
        _estack = .;
    } >sram1 :bss

    /* system heap */
    /*.heap (NOLOAD):
    {
        PROVIDE_HIDDEN(_sheap = .);
        . += LENGTH(sram2);
        PROVIDE_HIDDEN(_eheap = .);
    } >sram2*/

    _end = . ;
}

