// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

extern "C" {
#include "debug.h"

/* set_debug_flag is declared in deepin_pw_check.h, not debug.h */
void set_debug_flag(int flag);

/* debug_flag is formerly static, exposed via static= macro on debug_obj */
extern int debug_flag;
}

/* =====================================================================
 * set_debug_flag / get_debug_flag
 * ===================================================================== */
TEST(DebugFlagTest, DefaultIsZero) {
    /* Reset to known state */
    set_debug_flag(0);
    EXPECT_EQ(0, get_debug_flag());
}

TEST(DebugFlagTest, SetToOne) {
    set_debug_flag(1);
    EXPECT_EQ(1, get_debug_flag());
    set_debug_flag(0);
}

TEST(DebugFlagTest, SetToZero) {
    set_debug_flag(1);
    set_debug_flag(0);
    EXPECT_EQ(0, get_debug_flag());
}

TEST(DebugFlagTest, ToggleFlag) {
    set_debug_flag(0);
    EXPECT_EQ(0, get_debug_flag());
    set_debug_flag(1);
    EXPECT_EQ(1, get_debug_flag());
    set_debug_flag(0);
    EXPECT_EQ(0, get_debug_flag());
}

TEST(DebugFlagTest, SetArbitraryValue) {
    set_debug_flag(42);
    EXPECT_EQ(42, get_debug_flag());
    set_debug_flag(0);
}

TEST(DebugFlagTest, SetNegativeValue) {
    set_debug_flag(-1);
    EXPECT_EQ(-1, get_debug_flag());
    set_debug_flag(0);
}

/* =====================================================================
 * debug_flag variable (exposed via static= macro)
 * ===================================================================== */
TEST(DebugFlagVarTest, DirectAccessAfterSet) {
    set_debug_flag(1);
    EXPECT_EQ(1, debug_flag);
    set_debug_flag(0);
    EXPECT_EQ(0, debug_flag);
}

TEST(DebugFlagVarTest, DirectWriteReflectsInGet) {
    debug_flag = 99;
    EXPECT_EQ(99, get_debug_flag());
    /* Reset */
    debug_flag = 0;
    EXPECT_EQ(0, get_debug_flag());
}

