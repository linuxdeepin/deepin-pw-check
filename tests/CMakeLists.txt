# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)
project(deepin_pw_check_tests C CXX)

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- GTest ----
enable_testing()
find_package(GTest REQUIRED)

# ---- Coverage flags (gcov) ----
set(COVERAGE_FLAGS --coverage -g -O0)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

# ---- Source root ----
set(LIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
get_filename_component(LIB_SRC_DIR_ABS "${LIB_SRC_DIR}" ABSOLUTE)

# Test data directory (absolute path for compile definitions). Must be defined
# BEFORE the OBJECT-library target_compile_definitions below that reference it.
set(TESTDATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/testdata)

# ---- Helper: find iniparser header path ----
find_path(INIPARSER_INCLUDE_DIR iniparser/iniparser.h
    HINTS /usr/include /usr/local/include)
find_library(INIPARSER_LIB NAMES iniparser)
if(NOT INIPARSER_LIB)
    find_library(INIPARSER_LIB NAMES iniparser PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
endif()

# ---- Helper: find crypt library ----
find_library(CRYPT_LIB NAMES crypt)

# ---- Helper: find intl library (gettext) ----
find_library(INTL_LIB NAMES intl)

# =====================================================================
# OBJECT library: pw_check_core  (deepin_pw_check.c + debug.c)
#   - word_check.c excluded (cracklib dependency); stub provided in test
# =====================================================================
add_library(pw_check_core_obj OBJECT
    ${LIB_SRC_DIR}/deepin_pw_check.c
    ${LIB_SRC_DIR}/debug.c
)
target_compile_options(pw_check_core_obj PRIVATE -fPIC -DIN_CRACKLIB ${COVERAGE_FLAGS} -W -Wall)
target_compile_definitions(pw_check_core_obj PRIVATE static= PASSWD_CONF_FILE_GRUB2="${TESTDATA_DIR}/grub2_edit_auth.conf")
target_include_directories(pw_check_core_obj PRIVATE ${LIB_SRC_DIR})
if(INIPARSER_INCLUDE_DIR)
    target_include_directories(pw_check_core_obj PRIVATE ${INIPARSER_INCLUDE_DIR})
endif()

# =====================================================================
# OBJECT library: md5  (md5.c + md5_crypt.c)
# =====================================================================
add_library(md5_obj OBJECT
    ${LIB_SRC_DIR}/md5.c
    ${LIB_SRC_DIR}/md5_crypt.c
)
target_compile_options(md5_obj PRIVATE -fPIC ${COVERAGE_FLAGS} -W -Wall)
target_compile_definitions(md5_obj PRIVATE static=)
target_include_directories(md5_obj PRIVATE ${LIB_SRC_DIR})

# =====================================================================
# OBJECT library: bigcrypt  (bigcrypt.c)
# =====================================================================
add_library(bigcrypt_obj OBJECT
    ${LIB_SRC_DIR}/bigcrypt.c
)
target_compile_options(bigcrypt_obj PRIVATE -fPIC ${COVERAGE_FLAGS} -W -Wall)
target_include_directories(bigcrypt_obj PRIVATE ${LIB_SRC_DIR})

# =====================================================================
# OBJECT library: passwd_compare  (passwd_compare.c)
# =====================================================================
add_library(passwd_compare_obj OBJECT
    ${LIB_SRC_DIR}/passwd_compare.c
)
target_compile_options(passwd_compare_obj PRIVATE -fPIC ${COVERAGE_FLAGS} -W -Wall)
target_compile_definitions(passwd_compare_obj PRIVATE static=)
target_include_directories(passwd_compare_obj PRIVATE ${LIB_SRC_DIR})

# =====================================================================
# OBJECT library: debug  (debug.c)
# =====================================================================
add_library(debug_obj OBJECT
    ${LIB_SRC_DIR}/debug.c
)
target_compile_options(debug_obj PRIVATE -fPIC ${COVERAGE_FLAGS} -W -Wall)
target_compile_definitions(debug_obj PRIVATE static=)
target_include_directories(debug_obj PRIVATE ${LIB_SRC_DIR})

# =====================================================================
# Common link libraries
# =====================================================================
set(COMMON_LIBS GTest::gtest_main)
if(INIPARSER_LIB)
    list(APPEND COMMON_LIBS ${INIPARSER_LIB})
endif()
if(CRYPT_LIB)
    list(APPEND COMMON_LIBS ${CRYPT_LIB})
endif()
if(INTL_LIB)
    list(APPEND COMMON_LIBS ${INTL_LIB})
endif()

# =====================================================================
# Test target: test_pw_check_core
# =====================================================================
add_executable(test_pw_check_core
    test_pw_check_core.cpp
    $<TARGET_OBJECTS:pw_check_core_obj>
)
target_compile_options(test_pw_check_core PRIVATE ${COVERAGE_FLAGS})
target_compile_definitions(test_pw_check_core PRIVATE
    TESTDATA_DIR="${TESTDATA_DIR}"
)
target_include_directories(test_pw_check_core PRIVATE ${LIB_SRC_DIR})
if(INIPARSER_INCLUDE_DIR)
    target_include_directories(test_pw_check_core PRIVATE ${INIPARSER_INCLUDE_DIR})
endif()
target_link_libraries(test_pw_check_core PRIVATE ${COMMON_LIBS})
gtest_discover_tests(test_pw_check_core)

# =====================================================================
# Test target: test_md5_crypt
# =====================================================================
add_executable(test_md5_crypt
    test_md5_crypt.cpp
    $<TARGET_OBJECTS:md5_obj>
)
target_compile_options(test_md5_crypt PRIVATE ${COVERAGE_FLAGS})
target_include_directories(test_md5_crypt PRIVATE ${LIB_SRC_DIR})
target_link_libraries(test_md5_crypt PRIVATE ${COMMON_LIBS})
gtest_discover_tests(test_md5_crypt)

# =====================================================================
# Test target: test_passwd_compare
# =====================================================================
add_executable(test_passwd_compare
    test_passwd_compare.cpp
    $<TARGET_OBJECTS:passwd_compare_obj>
    $<TARGET_OBJECTS:md5_obj>
    $<TARGET_OBJECTS:bigcrypt_obj>
)
target_compile_options(test_passwd_compare PRIVATE ${COVERAGE_FLAGS})
target_include_directories(test_passwd_compare PRIVATE ${LIB_SRC_DIR})
target_link_libraries(test_passwd_compare PRIVATE ${COMMON_LIBS})
gtest_discover_tests(test_passwd_compare)

# =====================================================================
# Test target: test_bigcrypt
# =====================================================================
add_executable(test_bigcrypt
    test_bigcrypt.cpp
    $<TARGET_OBJECTS:bigcrypt_obj>
)
target_compile_options(test_bigcrypt PRIVATE ${COVERAGE_FLAGS})
target_include_directories(test_bigcrypt PRIVATE ${LIB_SRC_DIR})
target_link_libraries(test_bigcrypt PRIVATE ${COMMON_LIBS})
gtest_discover_tests(test_bigcrypt)

# =====================================================================
# Test target: test_debug
# =====================================================================
add_executable(test_debug
    test_debug.cpp
    $<TARGET_OBJECTS:debug_obj>
)
target_compile_options(test_debug PRIVATE ${COVERAGE_FLAGS})
target_include_directories(test_debug PRIVATE ${LIB_SRC_DIR})
target_link_libraries(test_debug PRIVATE ${COMMON_LIBS})
gtest_discover_tests(test_debug)

# =====================================================================
# coverage: execute all tests and generate coverage report (lcov-based)
# =====================================================================
find_program(LCOV_BIN lcov)
find_program(GENHTML_BIN genhtml)
set(_run_test_coverage "")
if(LCOV_BIN)
    set(_run_test_coverage
        # Collect raw coverage data
        COMMAND ${LCOV_BIN} --capture --directory ${CMAKE_BINARY_DIR}
                --rc lcov_branch_coverage=1
                --output-file ${CMAKE_BINARY_DIR}/coverage.info
        # Filter out generated/system files
        COMMAND ${LCOV_BIN} --extract ${CMAKE_BINARY_DIR}/coverage.info
                --rc lcov_branch_coverage=1
                "${LIB_SRC_DIR_ABS}/*"
                --output-file ${CMAKE_BINARY_DIR}/coverage_filtered.info
    )
    # Generate the HTML report only when genhtml is available. The whole command
    # (executable AND its arguments) is appended conditionally on purpose: a
    # generator expression such as $<$<BOOL:${GENHTML_BIN}>:${GENHTML_BIN}> would
    # expand the executable to nothing while the remaining arguments
    # (coverage_filtered.info --output-directory ...) still belong to the same
    # COMMAND.
    if(GENHTML_BIN)
        list(APPEND _run_test_coverage
            COMMAND ${GENHTML_BIN} ${CMAKE_BINARY_DIR}/coverage_filtered.info
                    --output-directory ${CMAKE_BINARY_DIR}/coverage_report
        )
    else()
        message(WARNING "genhtml not found: `coverage` will not generate an HTML report")
    endif()
    # Print summary
    list(APPEND _run_test_coverage
        COMMAND ${LCOV_BIN} --summary ${CMAKE_BINARY_DIR}/coverage_filtered.info
                --rc lcov_branch_coverage=1
    )
endif()
add_custom_target(coverage
    # Run tests first to refresh .gcda files
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    ${_run_test_coverage}
    DEPENDS test_pw_check_core test_md5_crypt test_passwd_compare test_bigcrypt test_debug
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating coverage report in ${CMAKE_BINARY_DIR}/coverage_report/"
    VERBATIM
)

