# sk::coro example — C++20 coroutine demos
#
# Targets:
#   demo_coro_basics      — Generator & Task fundamentals
#   demo_coro_task_chain  — Task chaining, co_await composition, avoiding callback hell
#   demo_coro_sync_await  — sync_await usage
#   demo_coro_callback_hell — comparison: callback style vs coroutine style
#   demo_coro_io_uring    — io_uring async IO (Linux only, requires liburing)
#   demo_coro_asio        — asio coroutine comparison
#   demo_coro_concurrencpp — concurrencpp comparison

include(FetchContent)

# ── asio (standalone, header-only) ──
# asio doesn't have a CMakeLists.txt, so we fetch the source and
# create an interface library manually.
if(NOT TARGET asio::asio)
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG        asio-1-30-2
    )
    FetchContent_MakeAvailable(asio)

    add_library(asio_asio INTERFACE)
    add_library(asio::asio ALIAS asio_asio)
    target_include_directories(asio_asio INTERFACE
        ${asio_SOURCE_DIR}/asio/include
    )
    target_compile_definitions(asio_asio INTERFACE
        ASIO_STANDALONE
        ASIO_NO_DEPRECATED
    )
    find_package(Threads REQUIRED)
    target_link_libraries(asio_asio INTERFACE Threads::Threads)
endif()

# ── concurrencpp — a modern C++20 coroutine library for comparison ──
if(NOT TARGET concurrencpp::concurrencpp)
    FetchContent_Declare(
        concurrencpp
        GIT_REPOSITORY https://github.com/David-Haim/concurrencpp.git
        GIT_TAG        v.0.1.7
    )
    FetchContent_MakeAvailable(concurrencpp)
endif()

# ── Common settings for all coro examples ──
set(CORO_INCLUDE_DIR ${INCLUDE_DIR})

# ── demo_coro_basics ──
add_executable(demo_coro_basics demo_coro_basics.cpp)
target_include_directories(demo_coro_basics PRIVATE ${CORO_INCLUDE_DIR})
target_compile_features(demo_coro_basics PRIVATE cxx_std_20)

# ── demo_coro_task_chain ──
add_executable(demo_coro_task_chain demo_coro_task_chain.cpp)
target_include_directories(demo_coro_task_chain PRIVATE ${CORO_INCLUDE_DIR})
target_compile_features(demo_coro_task_chain PRIVATE cxx_std_20)

# ── demo_coro_sync_await ──
add_executable(demo_coro_sync_await demo_coro_sync_await.cpp)
target_include_directories(demo_coro_sync_await PRIVATE ${CORO_INCLUDE_DIR})
target_compile_features(demo_coro_sync_await PRIVATE cxx_std_20)

# ── demo_coro_callback_hell ──
add_executable(demo_coro_callback_hell demo_coro_callback_hell.cpp)
target_include_directories(demo_coro_callback_hell PRIVATE ${CORO_INCLUDE_DIR})
target_compile_features(demo_coro_callback_hell PRIVATE cxx_std_20)

# ── demo_coro_asio ──
if(TARGET asio::asio)
    add_executable(demo_coro_asio demo_coro_asio.cpp)
    target_include_directories(demo_coro_asio PRIVATE ${CORO_INCLUDE_DIR})
    target_compile_features(demo_coro_asio PRIVATE cxx_std_20)
    target_link_libraries(demo_coro_asio PRIVATE asio::asio)
    target_compile_definitions(demo_coro_asio PRIVATE ASIO_STANDALONE ASIO_NO_DEPRECATED)
endif()

# ── demo_coro_concurrencpp ──
if(TARGET concurrencpp::concurrencpp)
    add_executable(demo_coro_concurrencpp demo_coro_concurrencpp.cpp)
    target_include_directories(demo_coro_concurrencpp PRIVATE ${CORO_INCLUDE_DIR})
    target_compile_features(demo_coro_concurrencpp PRIVATE cxx_std_20)
    target_link_libraries(demo_coro_concurrencpp PRIVATE concurrencpp::concurrencpp)
endif()

# ── io_uring example (Linux only) ──
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBURING liburing QUIET)
    endif()
    if(NOT LIBURING_FOUND)
        # Try find_package as fallback
        find_package(liburing QUIET)
        if(liburing_FOUND)
            set(LIBURING_FOUND TRUE)
            set(LIBURING_LIBRARIES liburing::liburing)
        endif()
    endif()

    if(LIBURING_FOUND)
        # Generate config.h so clangd can discover SK_HAS_IO_URING
        set(SK_HAS_IO_URING 1)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
            "${CMAKE_CURRENT_BINARY_DIR}/config.h"
            @ONLY
        )

        add_executable(demo_coro_io_uring demo_coro_io_uring.cpp)
        target_include_directories(demo_coro_io_uring PRIVATE
            ${CORO_INCLUDE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}    # for generated config.h
        )
        target_compile_features(demo_coro_io_uring PRIVATE cxx_std_20)
        target_link_libraries(demo_coro_io_uring PRIVATE ${LIBURING_LIBRARIES})
    else()
        message(STATUS "[coro] liburing not found — skipping io_uring example")
        message(STATUS "[coro] Install with: sudo apt install liburing-dev")
    endif()
endif()
