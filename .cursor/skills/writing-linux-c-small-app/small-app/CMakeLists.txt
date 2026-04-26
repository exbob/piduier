cmake_minimum_required(VERSION 3.15)
project(small-app C)

# ===== Language / project baseline =====
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ===== Build type defaults (single-config generators) =====
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
	Debug
	Release
)

# ===== Build metadata =====
string(TIMESTAMP BUILD_TIME "%Y.%m.%d %H:%M" UTC)
message(STATUS "Build Time: ${BUILD_TIME}")

execute_process(
	COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/git-version.sh"
	OUTPUT_VARIABLE GIT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET
)
if(NOT GIT_VERSION)
	set(GIT_VERSION "unknown")
endif()
message(STATUS "Git Version: ${GIT_VERSION}")

# ===== Generated configuration header =====
configure_file(Config.h.in Config.h)

# ===== Target definition =====
set(PROGRAM_NAME small-app)
set(PROGRAM_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
)

add_executable(${PROGRAM_NAME} ${PROGRAM_SOURCES})

target_include_directories(${PROGRAM_NAME}
	PRIVATE
	"${PROJECT_BINARY_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/inc"
)

# ===== Target compile settings =====
target_compile_options(${PROGRAM_NAME}
	PRIVATE
	-Wall
	-Wextra
	$<$<CONFIG:Debug>:-g;-O0>
	$<$<CONFIG:Release>:-O2>
)

target_compile_definitions(${PROGRAM_NAME}
	PRIVATE
	$<$<CONFIG:Debug>:APP_DEBUG>
)

# ===== Install layout =====

install(TARGETS ${PROGRAM_NAME}
	RUNTIME DESTINATION "."
)
