file(GLOB_RECURSE SKTEST_SOURCES "test_sktest_*.cpp")
file(GLOB_RECURSE GTEST_SOURCES "test_gtest_*.cpp")

foreach(SKTEST_FILE ${SKTEST_SOURCES})
    get_filename_component(SKTEST_TARGET ${SKTEST_FILE} NAME_WE)
    add_executable(${SKTEST_TARGET} ${SKTEST_FILE})
    target_include_directories(${SKTEST_TARGET} PRIVATE ${INCLUDE_DIR})
    string(TOUPPER ${SKTEST_TARGET} SK_CTEST_NAME)

    add_test(NAME ${SK_CTEST_NAME} COMMAND ${SKTEST_TARGET})
endforeach()

# if(Boost_FOUND AND NOT ${SYSTEM} STREQUAL "win")
#     target_compile_definitions(test_sktest_stackprinter PRIVATE I_HAVE_BOOST STACKTRACE_NO_WARNING
#                                                                 STACKTRACE_OUTPUT_FULLPATH)
#     target_compile_options(test_sktest_stackprinter PRIVATE -g)
# else()
#     target_compile_definitions(test_sktest_stackprinter PRIVATE STACKTRACE_NO_WARNING STACKTRACE_OUTPUT_FULLPATH)
#     target_compile_options(test_sktest_stackprinter PRIVATE -rdynamic)
# endif()

if(GTest_FOUND)
    foreach(GTEST_FILE ${GTEST_SOURCES})
        get_filename_component(GTEST_TARGET ${GTEST_FILE} NAME_WE)
        add_executable(${GTEST_TARGET} ${GTEST_FILE})
        target_include_directories(${GTEST_TARGET} PRIVATE ${INCLUDE_DIR})
        target_link_libraries(${GTEST_TARGET} GTest::gtest_main)
        string(TOUPPER ${GTEST_TARGET} G_CTEST_NAME)

        add_test(NAME ${G_CTEST_NAME} COMMAND ${GTEST_TARGET})
    endforeach()
endif()

# header-only single-file library test (depends on auto-generated skutils.h)
add_executable(test_skutils_headeronly test_skutils_headeronly.cpp)
target_link_libraries(test_skutils_headeronly PRIVATE skutils_single)
add_test(NAME TEST_SKUTILS_HEADERONLY COMMAND test_skutils_headeronly)
