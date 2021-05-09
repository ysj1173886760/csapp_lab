#include<stdio.h>

typedef unsigned char *byte_pointer;

void show_bytes(byte_pointer start, size_t len) {
    for (size_t i = 0; i < len; i++)
        printf(" %.2x", start[i]);
    printf("\n");
}

void show_long(long x) {
    show_bytes((byte_pointer) &x, sizeof(long));
}

void show_short(int x) {
    show_bytes((byte_pointer) &x, sizeof(short));
}

void show_double(int x) {
    show_bytes((byte_pointer) &x, sizeof(double));
}

void test_show_bytes(int val) {
    long lval = (long) val;
    short sval = (short) val;
    double dval = (double) val;

    show_long(lval);
    show_short(sval);
    show_double(dval);
}

int main() {
    test_show_bytes(1319);
    return 0;
}

// output
// 27 05 00 00 00 00 00 00
// 27 05
// 27 05 00 00 c0 4d 8a 5c