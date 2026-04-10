add_library(cjson STATIC "${CMAKE_SOURCE_DIR}/src/third_party/cjson/cJSON.c")
target_include_directories(cjson PUBLIC "${CMAKE_SOURCE_DIR}/src/third_party/cjson")

set(ZLOG_SOURCES
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/buf.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/category.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/category_table.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/conf.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/event.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/format.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/level.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/level_list.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/lockfile.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/mdc.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/record.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/record_table.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/rotater.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/rule.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/spec.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/thread.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/zc_arraylist.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/zc_hashtable.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/zc_profile.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/zc_util.c"
    "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src/zlog.c"
)
add_library(zlog STATIC ${ZLOG_SOURCES})
target_include_directories(zlog PUBLIC "${CMAKE_SOURCE_DIR}/src/third_party/zlog/src")
target_compile_options(zlog PRIVATE -Wno-sign-compare -Wno-unused-parameter)
target_link_libraries(zlog PUBLIC pthread)
