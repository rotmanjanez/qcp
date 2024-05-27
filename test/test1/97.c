#include <uchar.h>
#include <stddef.h>

char8_t invalid_utf8 = u8'\x80\x80'; // Invalid UTF-8 character constant (more than one character)