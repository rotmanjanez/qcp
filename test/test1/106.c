#include <uchar.h>
#include <stddef.h>

unsigned char valid_oct = '\123';  // Valid octal escape sequence
unsigned char valid_hex = '\x7F';  // Valid hexadecimal escape sequence
char8_t valid_utf8 = u8'\x7F';     // Valid single UTF-8 character
char16_t valid_utf16 = u'\x7FFF';  // Valid single UTF-16 character
char32_t valid_utf32 = U'\x7FFFFFFF'; // Valid single UTF-32 character
wchar_t valid_wchar = L'\x7FFF';   // Valid single wide character

char *valid1 = "Hello, " "World!";        // Adjacent non-prefixed string literals
char *valid2 = u8"Hello, " u8"World!";    // Adjacent UTF-8 string literals
wchar_t *valid3 = L"Hello, " L"World!";   // Adjacent wide string literals
char16_t *valid4 = u"Hello, " u"World!";  // Adjacent UTF-16 string literals
char32_t *valid5 = U"Hello, " U"World!";  // Adjacent UTF-32 string literals