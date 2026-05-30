# 基本设置
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 基本路径设置
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# 库文件查找路径
list(APPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_DEFAULT_TRIPLET}")
message(STATUS ${CMAKE_PREFIX_PATH})

# 安装位置
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Default install path")
endif()

# 安装头文件
install(DIRECTORY ${INCLUDE_DIR}/skutils DESTINATION include)

# 自动生成单文件头（header-only打包产物）
set(SKUTILS_SINGLE_HEADER ${INCLUDE_DIR}/header_only/skutils.h)
set(MERGE_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/merge_skutils.py)
set(SKUTILS_MERGE_INPUTS
    ${INCLUDE_DIR}/skutils/noncopyable.h
    ${INCLUDE_DIR}/skutils/spinlock.h
    ${INCLUDE_DIR}/skutils/config.h
    ${INCLUDE_DIR}/skutils/string_utils.h
    ${INCLUDE_DIR}/skutils/time_utils.h
    ${INCLUDE_DIR}/skutils/printer.h
    ${INCLUDE_DIR}/skutils/logger.h
)

add_custom_command(
    OUTPUT ${SKUTILS_SINGLE_HEADER}
    COMMAND python3 ${MERGE_SCRIPT} ${SKUTILS_SINGLE_HEADER} ${SKUTILS_MERGE_INPUTS}
    DEPENDS ${MERGE_SCRIPT} ${SKUTILS_MERGE_INPUTS}
    COMMENT "Generating single-header skutils.h -> include/header_only/skutils.h"
)
add_custom_target(gen_skutils_h DEPENDS ${SKUTILS_SINGLE_HEADER})

# INTERFACE library for consumers
add_library(skutils_single INTERFACE)
add_dependencies(skutils_single gen_skutils_h)
target_include_directories(skutils_single INTERFACE ${INCLUDE_DIR}/header_only)

install(FILES ${SKUTILS_SINGLE_HEADER} DESTINATION include)

include(${CMAKE_SOURCE_DIR}/cmake/tools.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

# CXX相关设置
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

option(WERROR "if -Werror" OFF)
if(WERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

option(BUILD_WITH_COVERAGE "Enable coverage reporting using gcov" OFF)
if(BUILD_WITH_COVERAGE)
    include(${CMAKE_SOURCE_DIR}/cmake/coverage.cmake)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(test)

option(BUILD_EXAMPLE "whether contains some lib demo" ON)

if(BUILD_EXAMPLE)
    add_subdirectory(example)
endif()
