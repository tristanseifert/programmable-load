
add_library(vendor-atmel STATIC
    # HPL sources
    Sources/hpl/core/hpl_core_m4.c
    Sources/hpl/wdt/hpl_wdt.c
    Sources/hpl/spi/spi_lite.c
    # not needed, since we use TinyUSB-provided USB code
    #Sources/hpl/usb/hpl_usb.c
    Sources/hpl/pm/hpl_pm.c
    Sources/hpl/core/hpl_init.c
    Sources/hpl/gclk/hpl_gclk.c
    Sources/hpl/dmac/hpl_dmac.c
    Sources/hpl/oscctrl/hpl_oscctrl.c
    Sources/hpl/trng/hpl_trng.c
    Sources/hpl/mclk/hpl_mclk.c
    Sources/hpl/ramecc/hpl_ramecc.c
    Sources/hpl/osc32kctrl/hpl_osc32kctrl.c
    Sources/hpl/rtc/hpl_rtc.c
    Sources/hpl/tc/tc_lite.c
    Sources/hpl/i2c/i2c_lite.c
    Sources/hpl/cmcc/hpl_cmcc.c

    # HAL sources
    #Sources/hal/src/hal_io.c
    #Sources/hal/src/hal_calendar.c
    #Sources/hal/utils/src/utils_syscalls.c
    #Sources/hal/src/hal_delay.c
    #Sources/hal/src/hal_wdt.c
    #Sources/hal/utils/src/utils_assert.c
    #Sources/hal/src/hal_rand_sync.c
    #Sources/hal/src/hal_init.c
    #Sources/hal/src/hal_usb_device.c
    #Sources/hal/utils/src/utils_list.c
    #Sources/hal/src/hal_gpio.c
    #Sources/hal/utils/src/utils_event.c
    #Sources/hal/src/hal_sleep.c
    Sources/hal/src/hal_cache.c
    #Sources/hal/src/hal_atomic.c
)

target_include_directories(vendor-atmel PUBLIC Sources/hal/include)
target_include_directories(vendor-atmel PUBLIC Sources/hal/utils/include)
target_include_directories(vendor-atmel PUBLIC Sources/hri)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/cmcc)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/core)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/dmac)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/gclk)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/i2c)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/mclk)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/osc32kctrl)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/oscctrl)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/pm)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/port)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/ramecc)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/rtc)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/spi)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/tc)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/trng)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/usb)
target_include_directories(vendor-atmel PUBLIC Sources/hpl/wdt)
target_include_directories(vendor-atmel PUBLIC Sources/config)
target_include_directories(vendor-atmel PRIVATE Includes)
#target_include_directories(vendor-atmel PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Includes/vendor)
target_include_directories(vendor-atmel PRIVATE ${CMSIS_INCLUDE_DIR})
target_include_directories(vendor-atmel PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Includes)

target_compile_options(vendor-atmel PRIVATE -Wno-cast-qual)

