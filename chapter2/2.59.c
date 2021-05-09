#include <stdio.h>
#include <assert.h>

int main() {
    size_t mask = 0xff;
    size_t x = 0x89ABCDEF;
    size_t y = 0x76543210;
    size_t ans = (x & mask) | (y & ~mask);
    assert(ans == 0x765432EF);
    return 0;
}