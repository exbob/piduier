include(FetchContent)

# Suppress deprecation warnings for cleaner third-party fetch output.
cmake_policy(SET CMP0135 NEW)
set(_OLD_CMAKE_WARN_DEPRECATED "${CMAKE_WARN_DEPRECATED}")
set(CMAKE_WARN_DEPRECATED OFF)

# ===== cJSON module =====
# set upstream version and integrity metadata.
set(CJSON_VERSION "1.7.18")
set(CJSON_TARBALL_URL "https://github.com/DaveGamble/cJSON/archive/refs/tags/v${CJSON_VERSION}.tar.gz")
set(CJSON_TARBALL_SHA256 "3aa806844a03442c00769b83e99970be70fbef03735ff898f4811dd03b9f5ee5")
set(CJSON_FETCH_ROOT "${CMAKE_SOURCE_DIR}/thirdpart/json")

# Declare source fetch without automatic add_subdirectory.
FetchContent_Declare(cjson_upstream
	URL "${CJSON_TARBALL_URL}"
	URL_HASH SHA256=${CJSON_TARBALL_SHA256}
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	SOURCE_DIR "${CJSON_FETCH_ROOT}"
)

# Download/extract only when not already populated.
FetchContent_GetProperties(cjson_upstream)
if(NOT cjson_upstream_POPULATED)
	FetchContent_Populate(cjson_upstream)
endif()

# Prefer GitHub archive top directory; fallback to SOURCE_DIR root if needed.
set(CJSON_SOURCE_SUBDIR "${CJSON_FETCH_ROOT}/cJSON-${CJSON_VERSION}")
if(NOT EXISTS "${CJSON_SOURCE_SUBDIR}/CMakeLists.txt" AND EXISTS "${CJSON_FETCH_ROOT}/CMakeLists.txt")
	set(CJSON_SOURCE_SUBDIR "${CJSON_FETCH_ROOT}")
endif()

# Fail fast when source layout is unexpected.
if(NOT EXISTS "${CJSON_SOURCE_SUBDIR}/CMakeLists.txt")
	message(FATAL_ERROR "cJSON source not found at ${CJSON_SOURCE_SUBDIR} (clear thirdpart/json and reconfigure)")
endif()

# Build cJSON as static and disable extra test/compiler flags.
set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
set(ENABLE_CUSTOM_COMPILER_FLAGS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Exclude third-party target from default all target.
add_subdirectory("${CJSON_SOURCE_SUBDIR}" "${CMAKE_BINARY_DIR}/thirdpart/cjson-build" EXCLUDE_FROM_ALL)

# ===== zlog module =====
# set upstream version and integrity metadata.
set(ZLOG_VERSION "1.2.18")
set(ZLOG_TARBALL_URL "https://github.com/HardySimpson/zlog/archive/refs/tags/${ZLOG_VERSION}.tar.gz")
set(ZLOG_TARBALL_SHA256 "3977dc8ea0069139816ec4025b320d9a7fc2035398775ea91429e83cb0d1ce4e")
set(ZLOG_FETCH_ROOT "${CMAKE_SOURCE_DIR}/thirdpart/zlog")

# Declare source fetch without automatic add_subdirectory.
FetchContent_Declare(zlog_upstream
	URL "${ZLOG_TARBALL_URL}"
	URL_HASH SHA256=${ZLOG_TARBALL_SHA256}
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	SOURCE_DIR "${ZLOG_FETCH_ROOT}"
)

# Download/extract only when not already populated.
FetchContent_GetProperties(zlog_upstream)
if(NOT zlog_upstream_POPULATED)
	FetchContent_Populate(zlog_upstream)
endif()

# Prefer GitHub archive top directory; fallback to SOURCE_DIR root if needed.
set(ZLOG_SOURCE_SUBDIR "${ZLOG_FETCH_ROOT}/zlog-${ZLOG_VERSION}")
if(NOT EXISTS "${ZLOG_SOURCE_SUBDIR}/CMakeLists.txt" AND EXISTS "${ZLOG_FETCH_ROOT}/CMakeLists.txt")
	set(ZLOG_SOURCE_SUBDIR "${ZLOG_FETCH_ROOT}")
endif()

# Fail fast when source layout is unexpected.
if(NOT EXISTS "${ZLOG_SOURCE_SUBDIR}/CMakeLists.txt")
	message(FATAL_ERROR "zlog source not found at ${ZLOG_SOURCE_SUBDIR} (clear thirdpart/zlog and reconfigure)")
endif()

# Build zlog as static and keep third-party target out of default all target.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory("${ZLOG_SOURCE_SUBDIR}" "${CMAKE_BINARY_DIR}/thirdpart/zlog-build" EXCLUDE_FROM_ALL)
