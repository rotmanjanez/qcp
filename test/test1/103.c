#include <uchar.h>
#include <stddef.h>

char16_t *invalid3 = u"Hello, " U"World!"; // Mixing UTF-16 and UTF-32 string literals