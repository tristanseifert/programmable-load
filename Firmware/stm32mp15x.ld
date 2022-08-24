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
    /* initialization code, read/write data, bss, stack */
    retram      (rwx)   : ORIGIN = 0x00000000, LENGTH = 64K
    /* runtime used executable, read-only data */
    sram1       (rwx)   : ORIGIN = 0x10000000, LENGTH = 128K
    /* heap */
    sram2       (rw)    : ORIGIN = 0x10020000, LENGTH = 128K
    /* IPC buffers */
    sram3       (rw)    : ORIGIN = 0x10040000, LENGTH = 64K
    /* linux reserved */
    sram4       (rw)    : ORIGIN = 0x10050000, LENGTH = 64K
}

/* program headers (for loading stuff) */
PHDRS {
    /* startup .text section (in retram) */
    text_startup        PT_LOAD;
    /* coprocessor resources (in retram) */
    coprorsrc           PT_LOAD;
    /* rw data (in retram, read-only copy stored in sram1) */
    data                PT_LOAD;
    /* bss (in retram) */
    bss                 PT_LOAD;

    /* app text section (in sram1) */
    text                PT_LOAD;
    /* read-only data (in sram1) */
    rodata              PT_LOAD;
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

/* symbols for heap location and size (consumed by libc) */
PROVIDE_HIDDEN(_sheap = ORIGIN(sram2));
PROVIDE_HIDDEN(_eheap = ORIGIN(sram2) + LENGTH(sram2));

/* Section Definitions */
SECTIONS
{
    /* vector table and startup code gets placed into retram */
    .text_startup :
    {
        KEEP(*(.isr_vector))
        KEEP(*(.vectors .vectors.*))
        KEEP(*(.startup .startup.*))
    } >retram :text_startup

    /* OpenAMP resource table */
    .resource_table ALIGN(16) :
    {
        KEEP (*(.resource_table .resource_table.*))
        . = ALIGN(4);
    } >retram :coprorsrc

    /* read/write data */
    . = ALIGN(256);

    __DATA_ROM = .;
    _sidata = LOADADDR(.data);

    .data ALIGN(256) :
    {
        _sdata = .;
        *(.data .data*);
        . = ALIGN(4);
        _edata = .;
    } >retram AT> sram1 :data

    __DATA_END = __DATA_ROM + (_edata - _sdata);

    /* .bss section which is used for uninitialized data */
    .bss ALIGN(16) (NOLOAD) :
    {
        _sbss = .;
        _szero = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        _ezero = .;
    } >retram :bss

    /* stack section */
    .stack ALIGN(16) (NOLOAD):
    {
        _sstack = .;
        . = . + STACK_SIZE;
        . = ALIGN(16);
        _estack = .;
    } >retram :bss

    /* executable code and read-only data gets shoved into the same output section */
    .text ALIGN(16) :
    {
        KEEP(*(.isr_vector))
        KEEP(*(.vectors .vectors.*))
        KEEP(*(.startup .startup.*))

        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7t) *(.glue_7)
    } >sram1 :text

    /* read-only data */
    .rodata ALIGN(16) :
    {
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
    } >sram1 :rodata

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    PROVIDE_HIDDEN(__exidx_start = .);
    .ARM.exidx :
    {
      *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } >sram1 :rodata
    PROVIDE_HIDDEN(__exidx_end = .);
}

