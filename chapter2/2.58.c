#include <stdio.h>

typedef unsigned char *byte_pointer;

int is_little_endian() {
    int num = 1;
    return *((byte_pointer) &num);
}

int main() {
    printf("%d", is_little_endian());
    return 0;
}

//output
//1