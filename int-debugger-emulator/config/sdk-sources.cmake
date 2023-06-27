
target_sources(${PROJECT_NAME}
PRIVATE ${CMAKE_SOURCE_DIR}/libs/usb/App/usb_device.c 
PRIVATE ${CMAKE_SOURCE_DIR}/libs/usb/App/usbd_desc.c 
PRIVATE ${CMAKE_SOURCE_DIR}/libs/usb/App/usbd_cdc_if.c 
PRIVATE ${CMAKE_SOURCE_DIR}/libs/usb/Target/usbd_conf.c 
PRIVATE ${CMAKE_SOURCE_DIR}/libs/usb/usb-irc-handler.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_ll_usb.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pcd.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pcd_ex.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_rcc.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_rcc_ex.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_gpio.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_dma.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_cortex.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pwr.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pwr_ex.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_flash.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_flash_ex.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_i2c.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_i2c_ex.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_exti.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_dac.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_dac_ex.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_sdadc.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_spi.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_spi_ex.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_tim.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_tim_ex.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c  
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/croutine.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/event_groups.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/list.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/queue.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/stream_buffer.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/tasks.c 
#PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/timers.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/portable/MemMang/heap_4.c 
PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/libs/usb/App
    PRIVATE ${CMAKE_SOURCE_DIR}/libs/usb/Target
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/CMSIS/Include
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Inc
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Src/
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/STM32F3xx_HAL_Driver/Inc/Legacy 
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/Drivers/CMSIS/Device/ST/STM32F3xx/Include
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/STM32_USB_Device_Library/Core/Inc
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ST/STM32_USB_Device_Library/Class/CDC/Inc
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/include 
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/CMSIS_RTOS_V2 
    PRIVATE ${CMAKE_SOURCE_DIR}/third-party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    )
