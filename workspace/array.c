#include <stdio.h>

int main() {
    int a[2][3] = {{1, 2, 3}, {4, 5, 6}};
    printf("%p\n%p\n", a[0], &a[0]);
    printf("%p\n%p\n%p\n%p\n", a[0] + 1, (&a[0] + 1), (&a[0]) + 1, a + 1);
    int (*p)[3] = &a[0];
    int (*q) = a[0];
    // printf("%p\n%p", )
    printf("%ld %ld %ld\n", sizeof(a), sizeof(p), sizeof(a + 0));
    printf("%ld %ld %ld\n", sizeof(*(a + 0)), sizeof(*(a + 0) + 0), sizeof(*(*(a + 0) + 0)));
    printf("%ld", sizeof(a[0]));
    return 0;
}

/*

即便是在二维数组中取出来的a[0]，q仍然被认为是一个一维数组，所以对q+1 相当于取到了a[0][1], 也就是q[1]
如果对a[0]取地址，则会将a[0]视为二维数组, 其实就等效于a
这里的p是一个指针，指向的元素为长度为3的数组，类型为数组指针
而a就是 int[2][3], 其实本质类型还是不同的，用sizeof就可以看出来
数组下标的计算机制也解释了为什么一定要规定除第一维以外的大小
因为p指向的是长度为3的int数组，所以p+1 相当于字节加了4 * 3
而数组a + 1 相当于 a + (1 * 3) * sizeof(int)
同时可以看到，只有a被认为是int[2][3], 而a+0被认为是一维的数组
猜测：对a做的第一次加法相当于对a的第一维的偏移计算出来，即消除了第一个维度
     或者是看成将第一维转换成了指针形式, 即变成了指向数组的指针
对 a+0 取解引用后发现，大小为一维数组的大小，即为一维数组
那么同理进一步进行加法运算将这一维数组消除掉，取地址应该可以发现最终结果为int
回头验证一下刚才的说法，我们对a[0]解引用得到的就是12

总结一下
只有a本身才是二维数组，类型为int[2][3]
a + 0            int (*)[3]
*(a + 0)         int[3]
*(a + 0) + 0     int *
*(*(a + 0) + 0)  int
a[0]             int[3]
这也就是为什么可以通过指针运算得到相同的结果
同时进一步验证了 *配合+ 与[] 的说法?

0x7fff2125f080
0x7fff2125f080
0x7fff2125f084
0x7fff2125f08c
0x7fff2125f08c
0x7fff2125f08c
24 8 8
12 8 4
12

*/