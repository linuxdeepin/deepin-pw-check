// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

static int debug_flag = 0;

void set_debug_flag(int flag) {
    debug_flag = flag;
}

int get_debug_flag(){
    return debug_flag;
}