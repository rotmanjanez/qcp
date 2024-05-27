#include <uchar.h>
#include <stddef.h>

char32_t *invalid4 = U"Hello, " "World!";  // Mixing UTF-32 and non-prefixed string literals