#include <stdio.h>
#include <nmmintrin.h>

int test() {
    unsigned int res = _mm_crc32_u8(1, 2);
}

int main() {
    auto result = test();
    printf("SSE4.2 supported and test function executed. Result: %d\n", result);
    return 0;
}
