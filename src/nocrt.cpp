// nocrt.cpp - the two C runtime functions that YAY.exe still needs.
//
// YAY.exe is linked without the C runtime (/NODEFAULTLIB, see build.cmd). That is what keeps it
// near 50 KB instead of 250 KB, and it means it needs nothing installed. The price: the compiler
// sometimes turns plain C++ into calls to memset and memcpy behind our back (for example
// `INPUT inputs[16] = {0};` or copying a struct), and with no C runtime nothing answers them.
//
// So we provide them here, using the processor's own block instructions through compiler
// intrinsics. If the linker ever reports another unresolved symbol such as strlen or memmove,
// either avoid the construct that causes it (use lstrlenW, lstrcpynW, wsprintfW and similar
// Windows functions, never the C library) or add a small replacement next to these.
//
// This file must be compiled WITHOUT /GL (whole-program optimisation), otherwise the compiler
// refuses to let us define a function it treats as built in. build.cmd does that.
#ifdef _MSC_VER
#include <windows.h>
#include <intrin.h>

extern "C" {
// "#pragma function" says: use this definition, not the compiler's inline version.
#pragma function(memset)
void *memset(void *dst, int c, size_t n) {
  __stosb((unsigned char *)dst, (unsigned char)c, n);  // "rep stosb": fill n bytes with c
  return dst;
}

#pragma function(memcpy)
void *memcpy(void *dst, const void *src, size_t n) {
  __movsb((unsigned char *)dst, (const unsigned char *)src, n);  // "rep movsb": copy n bytes
  return dst;
}
}
#endif
