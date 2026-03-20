cmake_minimum_required(VERSION 3.10)
project(piduier C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L")

# Inject git describe version for runtime API/about page
execute_process(
    COMMAND git describe --tags --always --long
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE PIDUIER_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT PIDUIER_VERSION)
    set(PIDUIER_VERSION "unknown")
endif()
add_definitions(-DPIDUIER_VERSION="${PIDUIER_VERSION}")
add_definitions(-DPIDUIER_BUILD_ARCH="${CMAKE_SYSTEM_PROCESSOR}")

# 添加源文件
set(SOURCES
    src/main.c
    src/http/router.c
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
    mongoose.c
)

# 创建可执行文件
add_executable(piduier ${SOURCES})

# 包含目录
target_include_directories(piduier PRIVATE
    .
    src
    src/http
    src/system
    src/network
    src/hardware
)

# 链接库
target_link_libraries(piduier pthread)

# 设置安装规则 - 将可执行文件安装到 bin/ 目录
install(TARGETS piduier
    RUNTIME DESTINATION bin
)

# 安装 web 文件到 bin/ 目录（保持目录结构）
install(DIRECTORY web/
    DESTINATION bin/web
    FILES_MATCHING 
        PATTERN "*.html" 
        PATTERN "*.css" 
        PATTERN "*.js"
    PATTERN ".git*" EXCLUDE
)

