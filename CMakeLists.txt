cmake_minimum_required(VERSION 3.10)
project(rapi-mo C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L")

# 添加源文件
set(SOURCES
    main.c
    mongoose.c
)

# 创建可执行文件
add_executable(server ${SOURCES})

# 链接 pthread 库
target_link_libraries(server pthread)

# 设置安装规则 - 将可执行文件安装到 bin/ 目录
install(TARGETS server
    RUNTIME DESTINATION bin
)

# 安装 index.html 到 bin/ 目录
install(FILES index.html
    DESTINATION bin
)

