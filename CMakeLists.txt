cmake_minimum_required(VERSION 3.15)
project(piduier C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L")

string(TIMESTAMP BUILD_TIME "%Y.%m.%d %H:%M" UTC)

execute_process(
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/git-version.sh"
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_VERSION)
    set(GIT_VERSION "unknown")
endif()
configure_file(Config.h.in Config.h)

# Determine runtime arch string from compiler target first (works for cross compile).
set(PIDUIER_BUILD_ARCH_STR "${CMAKE_SYSTEM_PROCESSOR}")
if(CMAKE_C_COMPILER)
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -dumpmachine
        OUTPUT_VARIABLE PIDUIER_COMPILER_MACHINE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(PIDUIER_COMPILER_MACHINE)
        string(TOLOWER "${PIDUIER_COMPILER_MACHINE}" PIDUIER_COMPILER_MACHINE_LOWER)
        if(PIDUIER_COMPILER_MACHINE_LOWER MATCHES "^(aarch64|arm64)")
            set(PIDUIER_BUILD_ARCH_STR "arm64")
        elseif(PIDUIER_COMPILER_MACHINE_LOWER MATCHES "^(x86_64|amd64)")
            set(PIDUIER_BUILD_ARCH_STR "x86_64")
        elseif(PIDUIER_COMPILER_MACHINE_LOWER MATCHES "^arm")
            set(PIDUIER_BUILD_ARCH_STR "arm")
        endif()
    endif()
endif()
add_definitions(-DPIDUIER_BUILD_ARCH="${PIDUIER_BUILD_ARCH_STR}")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/thirdparty.cmake")

# 添加源文件
set(SOURCES
    src/main.c
    src/config/app_config.c
    src/util/log.c
    src/http/router.c
    src/http/mongoose_log.c
    src/system/system_info.c
    src/system/datetime.c
    src/system/cpu.c
    src/system/memory.c
    src/system/uptime.c
    src/system/temperature.c
    src/system/storage.c
    src/network/network_manager.c
    src/network/ethernet.c
    src/network/wifi.c
    src/hardware/gpio.c
    src/third_party/mongoose/mongoose.c
)

# 创建可执行文件
add_executable(piduier ${SOURCES})

target_compile_definitions(piduier PRIVATE MG_ENABLE_CUSTOM_LOG=1)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(piduier PRIVATE PIDUIER_DEBUG_LOG=1)
endif()

# 包含目录
target_include_directories(piduier PRIVATE
    ${PROJECT_BINARY_DIR}
    src
    src/config
    src/util
    src/third_party/mongoose
    src/third_party/zlog/src
    src/http
    src/system
    src/network
    src/hardware
)

# 链接库
target_link_libraries(piduier PRIVATE cjson zlog pthread m)

# 安装到 prefix（build.sh 使用 --prefix ./deploy）
install(TARGETS piduier
    RUNTIME DESTINATION .
)

install(DIRECTORY web/
    DESTINATION web
    FILES_MATCHING
        PATTERN "*.html"
        PATTERN "*.css"
        PATTERN "*.js"
    PATTERN ".git*" EXCLUDE
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    install(FILES ${CMAKE_SOURCE_DIR}/config/piduier_debug.json
        DESTINATION .
        RENAME piduier.conf)
else()
    install(FILES ${CMAKE_SOURCE_DIR}/config/piduier_release.json
        DESTINATION .
        RENAME piduier.conf)
endif()

install(PROGRAMS ${CMAKE_SOURCE_DIR}/config/install.sh
    DESTINATION .
    RENAME install.sh)

install(PROGRAMS ${CMAKE_SOURCE_DIR}/config/uninstall.sh
    DESTINATION .
    RENAME uninstall.sh)

install(FILES ${CMAKE_SOURCE_DIR}/config/piduier.service
    DESTINATION .)
