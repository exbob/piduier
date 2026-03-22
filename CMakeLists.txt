cmake_minimum_required(VERSION 3.10)
project(piduier C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

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

# zlog 1.2.18 (vendored)
set(ZLOG_SOURCES
    src/third_party/zlog/src/buf.c
    src/third_party/zlog/src/category.c
    src/third_party/zlog/src/category_table.c
    src/third_party/zlog/src/conf.c
    src/third_party/zlog/src/event.c
    src/third_party/zlog/src/format.c
    src/third_party/zlog/src/level.c
    src/third_party/zlog/src/level_list.c
    src/third_party/zlog/src/lockfile.c
    src/third_party/zlog/src/mdc.c
    src/third_party/zlog/src/record.c
    src/third_party/zlog/src/record_table.c
    src/third_party/zlog/src/rotater.c
    src/third_party/zlog/src/rule.c
    src/third_party/zlog/src/spec.c
    src/third_party/zlog/src/thread.c
    src/third_party/zlog/src/zc_arraylist.c
    src/third_party/zlog/src/zc_hashtable.c
    src/third_party/zlog/src/zc_profile.c
    src/third_party/zlog/src/zc_util.c
    src/third_party/zlog/src/zlog.c
)
add_library(zlog STATIC ${ZLOG_SOURCES})
target_include_directories(zlog PUBLIC ${CMAKE_SOURCE_DIR}/src/third_party/zlog/src)
target_link_libraries(zlog PUBLIC pthread)
target_compile_options(zlog PRIVATE -Wno-sign-compare -Wno-unused-parameter)

# 添加源文件
set(SOURCES
    src/main.c
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
    src
    src/util
    src/third_party/mongoose
    src/third_party/zlog/src
    src/http
    src/system
    src/network
    src/hardware
)

# 链接库
target_link_libraries(piduier PRIVATE zlog pthread)

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
    install(FILES ${CMAKE_SOURCE_DIR}/config/zlog/zlog_debug.conf
        DESTINATION .
        RENAME zlog.conf)
else()
    install(FILES ${CMAKE_SOURCE_DIR}/config/zlog/zlog_release.conf
        DESTINATION .
        RENAME zlog.conf)
endif()
