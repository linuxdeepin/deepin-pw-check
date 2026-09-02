# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Root build entry for deepin-pw-check.
#
# The production library (lib/) is built via the repo's Makefile / Go tooling;
# this CMakeLists only drives the unit tests, gated behind the unified
# BUILD_TESTING switch so the standard three-command flow works from the repo
# root:
#
#   cmake -B build -DBUILD_TESTING=ON
#   cmake --build build --target test
#   cmake --build build --target run-test

cmake_minimum_required(VERSION 3.16)
project(deepin-pw-check LANGUAGES C CXX)

option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
