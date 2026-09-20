# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Root build entry for deepin-pw-check.
#
# The production library (lib/) is built via the repo's Makefile / Go tooling;
# this CMakeLists only drives the unit tests, gated behind the unified
# BUILD_TESTING switch so the standard flow works from the repo root:
#
#   cmake -B build -DBUILD_TESTING=ON
#   cmake --build build                     # build the unit-test binaries
#   cmake --build build --target test       # run them (CTest)
#   cmake --build build --target coverage   # run them + generate the lcov HTML report
#
# (`test` only runs CTest, it does not build the binaries first — hence the
#  explicit build step. `coverage` builds them itself via its DEPENDS.)

cmake_minimum_required(VERSION 3.16)
project(deepin-pw-check LANGUAGES C CXX)

option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
