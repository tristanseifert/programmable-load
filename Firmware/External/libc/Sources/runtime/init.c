/**
 * @file
 *
 * @brief Runtime initialization
 */

/**
 * @brief Invoke static initializers
 */
__attribute__((section(".startup"))) void __libc_init_constructors() {
    extern void (*__preinit_array_start)();
    extern void (*__preinit_array_end)();
    extern void (*__init_array_start)();
    extern void (*__init_array_end)();

    // pre-initializers first
    for (void (**p)() = &__preinit_array_start; p < &__preinit_array_end; ++p) {
        (*p)();
    }

    // then the regular initializers
    for (void (**p)() = &__init_array_start; p < &__init_array_end; ++p) {
        (*p)();
    }
}
