#include <uchar.h>
#include <stddef.h>

char32_t invalid_utf32 = U'\x80000000\x80000000'; // Invalid UTF-32 character constant (more than one character)