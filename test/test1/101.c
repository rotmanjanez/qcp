#include <uchar.h>
#include <stddef.h>

char *invalid1 = "Hello, " u8"World!";    // Mixing non-prefixed and UTF-8 string literals