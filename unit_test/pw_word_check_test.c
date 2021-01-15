#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main() {
    extern bool is_word(const char* pw, const char* dict_path);
    
    // if( is_word("yellowing",NULL) != true) {
    //     return -1;
    // }

    if( is_word("fdajflkajsdl",NULL) != false) {
        return -1;
    }

    return 0;
}