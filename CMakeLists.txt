cmake_minimum_required(VERSION 3.20)

# Set toolchain file automatically if not supplied via command line
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/arm-none-eabi-gcc.cmake")
endif()

project(radio-rover-controller C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Debug build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# Common compile flags
add_compile_options(
    ${MCU_FLAGS}
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wextra
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-Os>
)

# Common link flags
add_link_options(
    ${MCU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/Linker/STM32G491RETX_FLASH.ld
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
    -specs=nano.specs
    -specs=nosys.specs
)

# Preprocessor definitions
add_compile_definitions(
    STM32G491xx
    USE_FULL_LL_DRIVER
)

# Project Include Directories
set(PROJECT_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/Bsp/Inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/Components/NRF24L01"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32G4xx/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32G4xx_LL_Driver/Inc"
)

# Application Sources (Core, BSP, Components)
file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/*.cpp"
)

file(GLOB_RECURSE BSP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Bsp/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Bsp/Src/*.cpp"
)

file(GLOB_RECURSE COMPONENT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Components/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Components/*.cpp"
)

set(APP_SOURCES
    ${CORE_SOURCES}
    ${BSP_SOURCES}
    ${COMPONENT_SOURCES}
)

# Startup file
set(STARTUP_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/startup_stm32g491xx.s"
)

# STM32 Low Layer (LL) Driver sources (pure LL drivers; excluding fmc, usb which require HAL headers)
file(GLOB LL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32G4xx_LL_Driver/Src/*.c"
)
list(FILTER LL_SOURCES EXCLUDE REGEX ".*(fmc|usb)\\.c$")

# LL Driver Library
add_library(stm32g4_ll STATIC ${LL_SOURCES})
target_include_directories(stm32g4_ll PUBLIC ${PROJECT_INCLUDES})

# Executable target
add_executable(${PROJECT_NAME}.elf
    ${APP_SOURCES}
    ${STARTUP_SOURCE}
)

target_include_directories(${PROJECT_NAME}.elf PUBLIC ${PROJECT_INCLUDES})
target_link_libraries(${PROJECT_NAME}.elf PRIVATE stm32g4_ll -lc -lm -lnosys)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Building ${PROJECT_NAME}.hex, ${PROJECT_NAME}.bin and printing memory usage:"
)
