// Minimal replacements for the two C runtime functions the compiler may emit calls to
// (zero-initialising structs, copying structs). Lets the exe link without the C runtime.
#ifdef _MSC_VER
#include <windows.h>
#include <intrin.h>

extern "C" {
#pragma function(memset)
void *memset(void *dst, int c, size_t n) {
  __stosb((unsigned char *)dst, (unsigned char)c, n);
  return dst;
}

#pragma function(memcpy)
void *memcpy(void *dst, const void *src, size_t n) {
  __movsb((unsigned char *)dst, (const unsigned char *)src, n);
  return dst;
}
}
#endif
