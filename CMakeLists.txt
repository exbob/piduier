cmake_minimum_required(VERSION 3.10)
project(piduier C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L")

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

# 查找 libgpiod（可选，仅用于硬件控制功能）
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(GPIOD libgpiod)
endif()

# 链接库
target_link_libraries(piduier pthread)

# 如果找到 libgpiod，链接它
if(GPIOD_FOUND)
    target_link_libraries(piduier ${GPIOD_LIBRARIES})
    target_include_directories(piduier PRIVATE ${GPIOD_INCLUDE_DIRS})
    add_definitions(-DHAVE_LIBGPIOD)
    message(STATUS "libgpiod found - GPIO support enabled")
else()
    message(STATUS "libgpiod not found - GPIO support disabled")
endif()

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

