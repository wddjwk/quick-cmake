# ANSI COLOR
set(FG_RED \\033[0;31m)
set(FG_GREEN \\033[0;32m)
set(FG_YELLOW \\033[0;33m)
set(FG_BLUE \\033[0;36m)
set(BG_RED \\033[41m)
set(BG_GREEN \\033[42m)
set(BG_YELLOW \\033[43m)
set(BG_BLUE \\033[44m)
set(COLOR_RESET \\033[0m)
# message red
function(msg_red)
    execute_process(COMMAND echo -en ${FG_RED}${ARGN}\n${COLOR_RESET})
endfunction()
# message green
function(msg_green)
    execute_process(COMMAND echo -en ${FG_GREEN}${ARGN}\n${COLOR_RESET})
endfunction()
# message yellow
function(msg_yellow)
    execute_process(COMMAND echo -en ${FG_YELLOW}${ARGN}\n${COLOR_RESET})
endfunction()
# message blue
function(msg_blue)
    execute_process(COMMAND echo -en ${FG_BLUE}${ARGN}\n${COLOR_RESET})
endfunction()

# find package and specify
macro(local_find_package)
    # 第一个参数作为包名
    set(pkgname ${ARGV0})

    find_package(${ARGN} QUIET)

    if(${pkgname}_FOUND)
        msg_green("[FOUNDED_PKG]: ${pkgname}")

        # 尝试收集并显示包的所有重要路径信息
        set(path_vars
            ${pkgname}_INCLUDE_DIRS
            ${pkgname}_INCLUDE_DIR
            ${pkgname}_INCLUDES
            ${pkgname}_LIBRARY
            ${pkgname}_LIBRARIES
            ${pkgname}_DIRS
            ${pkgname}_DIR
            ${pkgname}_ROOT
            ${pkgname}_PATH
            ${pkgname}_CONFIG)

        # 循环检查每个可能的路径变量
        foreach(path_var ${path_vars})
            if(DEFINED ${path_var} AND NOT "${${path_var}}" STREQUAL "")
                msg_blue("  - ${path_var}: ${${path_var}}")
            endif()
        endforeach()

        # 显示版本信息(如果有)
        if(DEFINED ${pkgname}_VERSION AND NOT "${${pkgname}_VERSION}" STREQUAL "")
            msg_blue("  - Version: ${${pkgname}_VERSION}")
        endif()
    else()
        # 包未找到 - 以黄色显示警告信息
        msg_yellow("[UNFOUNDED_PKG]: ${pkgname}")
    endif()
endmacro()

# Run cppcheck static analysis for a target.
# Prerequisites (set before calling this function):
#   CPP_CHECK      - path to cppcheck executable (use find_program(CPP_CHECK cppcheck))
#   CPP_CHECK_DIR  - output directory for reports (default: ${CMAKE_BINARY_DIR}/cppcheck_reports)
function(run_cppcheck target)
    if(NOT CPP_CHECK)
        return()
    endif()

    if(NOT CPP_CHECK_DIR)
        set(CPP_CHECK_DIR ${CMAKE_BINARY_DIR}/cppcheck_reports)
    endif()

    message(STATUS "Running cppcheck on ${target}")
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CPP_CHECK_DIR}
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CPP_CHECK_DIR}/${target}.log
        COMMAND
            ${CPP_CHECK} --enable=all --suppress=missingIncludeSystem --std=c++20
            --output-file=${CPP_CHECK_DIR}/${target}.log --force --quiet
            --project=${CMAKE_BINARY_DIR}/compile_commands.json
        COMMENT "Running cppcheck on ${target}")
endfunction()

# 普通target添加
function(add_targets GROUP_NAME SOURCES)
    set(TARGETS "[${GROUP_NAME}]: ")

    # 【这里可能还需要打磨：遍历的时候必须要 ARGV1 ARGN，才能保证不漏掉源文件】
    foreach(file IN LISTS ARGV1 ARGN)
        get_filename_component(target ${file} NAME_WE)
        add_executable(${target} ${file})
        target_include_directories(${target} PUBLIC ${INCLUDE_DIR})
        set(TARGETS "${TARGETS} ${target}")
    endforeach()

    message(STATUS --${TARGETS})
endfunction()

# fetch package from git repository via FetchContent
# Usage: fetch_git_package(name git_repo [git_tag])
#   name    - target name (e.g., fmt → fmt::fmt)
#   git_repo - git repository URL
#   git_tag  - optional git tag/branch/commit (default: latest/default branch)
function(fetch_git_package name git_repo)
    include(FetchContent)

    if(TARGET ${name}::${name})
        msg_green("[FETCH_PKG]: ${name} (already exists)")
        _fetch_git_package_show_info(${name})
        return()
    endif()

    msg_yellow("[FETCHING_PKG]: ${name}")
    msg_blue("  - Repository: ${git_repo}")

    if(ARGC GREATER 2)
        set(_tag "${ARGV2}")
        msg_blue("  - Git Tag: ${_tag}")
        FetchContent_Declare(
            ${name}
            GIT_REPOSITORY ${git_repo}
            GIT_TAG        ${_tag}
        )
    else()
        msg_blue("  - Git Tag: latest (default branch)")
        FetchContent_Declare(
            ${name}
            GIT_REPOSITORY ${git_repo}
        )
    endif()

    FetchContent_MakeAvailable(${name})

    _fetch_git_package_show_info(${name})
endfunction()

# helper: show targets or variables for a fetched package
function(_fetch_git_package_show_info name)
    # 1) show canonical namespaced target (e.g. fmt::fmt)
    if(TARGET ${name}::${name})
        get_target_property(_type ${name}::${name} TYPE)
        msg_blue("  - Target: ${name}::${name} (${_type})")
        set(_found_target TRUE)
    endif()

    # 2) if no target at all, fallback to variable-style display
    if(NOT _found_target)
        set(_vars
            ${name}_INCLUDE_DIRS
            ${name}_INCLUDE_DIR
            ${name}_INCLUDES
            ${name}_LIBRARY
            ${name}_LIBRARIES
            ${name}_SOURCE_DIR
            ${name}_BINARY_DIR)
        foreach(_v ${_vars})
            if(DEFINED ${_v} AND NOT "${${_v}}" STREQUAL "")
                msg_blue("  - ${_v}: ${${_v}}")
            endif()
        endforeach()
    endif()

    # show version info if available
    if(DEFINED ${name}_VERSION AND NOT "${${name}_VERSION}" STREQUAL "")
        msg_blue("  - Version: ${${name}_VERSION}")
    endif()

    msg_green("[${name}] Done!")
endfunction()
