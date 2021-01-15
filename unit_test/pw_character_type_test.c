#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main() {
    extern bool is_type_valid(const char* pw,char* character_type, int character_num_required);
    if( is_type_valid("123456","12345",1) != false) {
        return -1;
    }

    if( is_type_valid("123456A","1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ",2) != true) {
        return -1;
    }

    if( is_type_valid("123456a","1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ",2) != false) {
        return -1;
    }

    if( is_type_valid("123456Aa!","1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ",2) != false) {
        return -1;
    }

    return 0;
}