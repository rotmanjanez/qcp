#include <uchar.h>
#include <stddef.h>

char16_t invalid_utf16 = u'\x8000\x8000'; // Invalid UTF-16 character constant (more than one character)