####################################################################################################
# ST vendor libraries
#
# Export a static library that builds all of the ST vendor code. This is mostly just used for the
# basic peripheral setup/configuration; we don't use their janky HAL layer.
####################################################################################################

add_library(vendor-st STATIC
    Sources/Startup/startup_stm32mp151aacx.s
    Sources/Startup/system_stm32mp1xx.c

    # HAL
    Sources/hal/stm32mp1xx_hal.c
    Sources/hal/stm32mp1xx_hal_adc_ex.c
    Sources/hal/stm32mp1xx_hal_adc.c
    Sources/hal/stm32mp1xx_hal_cortex.c
    Sources/hal/stm32mp1xx_hal_crc_ex.c
    Sources/hal/stm32mp1xx_hal_crc.c
    Sources/hal/stm32mp1xx_hal_dac_ex.c
    Sources/hal/stm32mp1xx_hal_dac.c
    Sources/hal/stm32mp1xx_hal_dma_ex.c
    Sources/hal/stm32mp1xx_hal_dma.c
    Sources/hal/stm32mp1xx_hal_exti.c
    Sources/hal/stm32mp1xx_hal_gpio.c
    Sources/hal/stm32mp1xx_hal_hsem.c
    Sources/hal/stm32mp1xx_hal_ipcc.c
    Sources/hal/stm32mp1xx_hal_mdma.c
    Sources/hal/stm32mp1xx_hal_pwr_ex.c
    Sources/hal/stm32mp1xx_hal_pwr.c
    Sources/hal/stm32mp1xx_hal_rcc_ex.c
    Sources/hal/stm32mp1xx_hal_rcc.c
    Sources/hal/stm32mp1xx_hal_rng.c
    Sources/hal/stm32mp1xx_hal_tim_ex.c
    Sources/hal/stm32mp1xx_hal_tim.c
    # HAL low level
    Sources/hal/stm32mp1xx_ll_adc.c
    Sources/hal/stm32mp1xx_ll_dma.c
    Sources/hal/stm32mp1xx_ll_exti.c
    Sources/hal/stm32mp1xx_ll_gpio.c
    Sources/hal/stm32mp1xx_ll_i2c.c
    Sources/hal/stm32mp1xx_ll_rcc.c
    Sources/hal/stm32mp1xx_ll_spi.c
    Sources/hal/stm32mp1xx_ll_usart.c
    Sources/hal/stm32mp1xx_ll_utils.c
    # runtime support (for working with the Linux side)
    Sources/Runtime/LockResource.c
)

target_include_directories(vendor-st PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Includes)

target_include_directories(vendor-st PUBLIC Includes/CMSIS-Device-ST/STM32MP1xx/Include)
target_include_directories(vendor-st PUBLIC Includes/config)
target_include_directories(vendor-st PUBLIC Includes/hal)
target_include_directories(vendor-st PUBLIC Includes/Runtime)

target_include_directories(vendor-st PRIVATE ${CMSIS_INCLUDE_DIR})

# shut up some errors
target_compile_options(vendor-st PRIVATE -Wno-cast-qual -Wno-parentheses-equality)

# enable the low-level drivers
target_compile_definitions(vendor-st PUBLIC USE_FULL_LL_DRIVER)
