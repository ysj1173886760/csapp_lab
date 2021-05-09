#include <stdio.h>

int a = 0, b = 1;
int extern sum(int, int);
int main() {
    int c = sum(a, b);
    printf("%d", c);
    return 0;
}
