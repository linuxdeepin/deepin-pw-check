#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main() {
    extern bool is_palindrome(const char * pw, int palindrome_min_num);
    
    if( is_palindrome("1234432",4) != false) {
        return -1;
    }

    if( is_palindrome("12344321",4) != true) {
        return -1;
    }

    if( is_palindrome("123454321",5) != false) {
        return -1;
    }

    if( is_palindrome("123454321",4) != true) {
        return -1;
    }

    return 0;
}