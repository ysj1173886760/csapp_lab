#include <stdio.h>
#include <assert.h>

typedef unsigned char *byte_pointer;

unsigned replace_byte(unsigned x, int i, unsigned char b) {
   *(((byte_pointer) &x) + i) = b;
   return x;
}
// small endian
// in memory 78 56 AB 12
int main() {
    assert(replace_byte(0x12345678, 2, 0xAB) == 0x12AB5678);
    assert(replace_byte(0x12345678, 0, 0xAB) == 0x123456AB);
    printf("%d %d", replace_byte(0x12345678, 2, 0xAB), 0x12AB5678);
    return 0;
}