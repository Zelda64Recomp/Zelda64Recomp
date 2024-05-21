#include <stdio.h>

#if defined _WIN32 && (defined(_M_ARM) || defined(_M_ARM64))
#include <Intrin.h>
#include <arm_neon.h>
#define CV_NEON 1
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#define CV_NEON 1
#endif

void test_aliased_type(const uint8x16_t &a) {}
void test_aliased_type(const int8x16_t &a) {}

#if defined CV_NEON
int test() {
  const float src[] = {0.0f, 0.0f, 0.0f, 0.0f};
  float32x4_t val = vld1q_f32((const float32_t *)(src));
  return (int)vgetq_lane_f32(val, 0);
}
#else
#error "NEON is not supported"
#endif

int main() {
  auto result = test();
  printf("NEON supported and test function executed. Result: %d\n", result);
  return 0;
}
