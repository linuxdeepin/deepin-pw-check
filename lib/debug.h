// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _LIB_DEBUG_H_
#define _LIB_DEBUG_H_

#if 1

#include <stdio.h>

extern int get_debug_flag();

#define DEBUG(format, ...) do {     \
                    if (get_debug_flag()) {     \
                        printf("[FILE: %s] [FUNC: %s] [LINE: %d] : "format"\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);       \
                    }       \
                }while(0)

#else

#define DEBUG(format, ...)

#endif

#endif