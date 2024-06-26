/* C++ code produced by gperf version 3.1 */
/* Command-line: /usr/bin/gperf --output-file=/home/joshy/university/bacherlorthesis/qcp/src/keywords.cc /home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf  */
/* Computed positions: -k'1,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 10 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"

#ifndef QCP_TOKEN_RESERVED_KEYWORD_HASH_H
#define QCP_TOKEN_RESERVED_KEYWORD_HASH_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "keywords.h"
#include "token.h"
// ---------------------------------------------------------------------------
#include <cstring>
// ---------------------------------------------------------------------------
using TK = qcp::token::Kind;
// ---------------------------------------------------------------------------
namespace qcp {
namespace token {
// ---------------------------------------------------------------------------
#line 27 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
struct GPerfToken;
/* maximum key range = 74, duplicates = 0 */

class ReservedKeywordHash
{
private:
  static inline unsigned int hash (const char *str, size_t len);
public:
  static const struct GPerfToken *isInWordSet (const char *str, size_t len);
};

inline unsigned int
ReservedKeywordHash::hash (const char *str, size_t len)
{
  static const unsigned char asso_values[] =
    {
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      40, 80, 35, 80, 80, 80, 40, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80,  0, 80, 30, 15, 25,
      10, 20, 15, 25,  0, 40, 80,  0, 50, 40,
      15,  5, 80, 80, 25,  0,  5, 55,  0,  5,
       5,  5, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
      80, 80, 80, 80, 80, 80
    };
  return len + asso_values[static_cast<unsigned char>(str[len - 1])] + asso_values[static_cast<unsigned char>(str[0])];
}

const struct GPerfToken *
ReservedKeywordHash::isInWordSet (const char *str, size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 59,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 14,
      MIN_HASH_VALUE = 6,
      MAX_HASH_VALUE = 79
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  0,  0,  0,  0,  6,  0,  8,  0,  5,  6,  7,  8,
       4, 10,  6,  2, 13, 14,  5,  6,  7,  8,  9,  5,  6,  7,
       8,  4,  5,  6,  7,  8,  4,  5,  6,  7,  8,  4,  5,  6,
       0,  3,  4, 10,  6,  7,  3,  4, 10, 11,  7,  8,  4,  5,
       0,  2,  8,  9,  0,  0,  0, 13,  4,  0,  6, 12, 13,  4,
       0,  0,  0,  8,  0,  5,  0,  0,  0,  4
    };
  static const struct GPerfToken wordlist[] =
    {
      {""}, {""}, {""}, {""}, {""}, {""},
#line 67 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"switch", static_cast<int>(TK::SWITCH)},
      {""},
#line 30 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Alignas", static_cast<int>(TK::ALIGNAS)},
      {""},
#line 60 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"short", static_cast<int>(TK::SHORT)},
#line 66 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"struct", static_cast<int>(TK::STRUCT)},
#line 80 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_BitInt", static_cast<int>(TK::BITINT)},
#line 81 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Complex", static_cast<int>(TK::COMPLEX)},
#line 76 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"void", static_cast<int>(TK::VOID)},
#line 86 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Imaginary", static_cast<int>(TK::IMAGINARY)},
#line 61 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"signed", static_cast<int>(TK::SIGNED)},
#line 43 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"do", static_cast<int>(TK::DO)},
#line 64 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"static_assert", static_cast<int>(TK::STATIC_ASSERT)},
#line 65 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Static_assert", static_cast<int>(TK::STATIC_ASSERT)},
#line 36 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"break", static_cast<int>(TK::BREAK)},
#line 62 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"sizeof", static_cast<int>(TK::SIZEOF)},
#line 42 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"default", static_cast<int>(TK::DEFAULT)},
#line 32 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Alignof", static_cast<int>(TK::ALIGNOF)},
#line 87 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Noreturn", static_cast<int>(TK::NORETURN)},
#line 49 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"float", static_cast<int>(TK::FLOAT)},
#line 72 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"typeof", static_cast<int>(TK::TYPEOF)},
#line 71 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"typedef", static_cast<int>(TK::TYPEDEF)},
#line 77 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"volatile", static_cast<int>(TK::VOLATILE)},
#line 70 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"true", static_cast<int>(TK::TRUE)},
#line 78 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"while", static_cast<int>(TK::WHILE)},
#line 63 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"static", static_cast<int>(TK::STATIC)},
#line 79 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Atomic", static_cast<int>(TK::ATOMIC)},
#line 85 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Generic", static_cast<int>(TK::GENERIC)},
#line 51 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"goto", static_cast<int>(TK::GOTO)},
#line 39 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"const", static_cast<int>(TK::CONST)},
#line 44 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"double", static_cast<int>(TK::DOUBLE)},
#line 29 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"alignas", static_cast<int>(TK::ALIGNAS)},
#line 58 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"restrict", static_cast<int>(TK::RESTRICT)},
#line 33 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"auto", static_cast<int>(TK::AUTO)},
#line 48 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"false", static_cast<int>(TK::FALSE)},
#line 47 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"extern", static_cast<int>(TK::EXTERN)},
      {""},
#line 50 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"for", static_cast<int>(TK::FOR)},
#line 45 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"else", static_cast<int>(TK::ELSE)},
#line 84 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Decimal64", static_cast<int>(TK::DECIMAL64)},
#line 59 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"return", static_cast<int>(TK::RETURN)},
#line 56 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"nullptr", static_cast<int>(TK::NULLPTR)},
#line 54 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"int", static_cast<int>(TK::INT)},
#line 37 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"case", static_cast<int>(TK::CASE)},
#line 83 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Decimal32", static_cast<int>(TK::DECIMAL32)},
#line 82 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Decimal128", static_cast<int>(TK::DECIMAL128)},
#line 31 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"alignof", static_cast<int>(TK::ALIGNOF)},
#line 41 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"continue", static_cast<int>(TK::CONTINUE)},
#line 38 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"char", static_cast<int>(TK::CHAR)},
#line 35 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Bool", static_cast<int>(TK::BOOL)},
      {""},
#line 52 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"if", static_cast<int>(TK::IF)},
#line 57 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"register", static_cast<int>(TK::REGISTER)},
#line 40 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"constexpr", static_cast<int>(TK::CONSTEXPR)},
      {""}, {""}, {""},
#line 69 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"_Thread_local", static_cast<int>(TK::THREAD_LOCAL)},
#line 46 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"enum", static_cast<int>(TK::ENUM)},
      {""},
#line 53 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"inline", static_cast<int>(TK::INLINE)},
#line 68 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"thread_local", static_cast<int>(TK::THREAD_LOCAL)},
#line 73 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"typeof_unqual", static_cast<int>(TK::TYPEOF_UNQUAL)},
#line 34 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"bool", static_cast<int>(TK::BOOL)},
      {""}, {""}, {""},
#line 75 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"unsigned", static_cast<int>(TK::UNSIGNED)},
      {""},
#line 74 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"union", static_cast<int>(TK::UNION)},
      {""}, {""}, {""},
#line 55 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"
      {"long", static_cast<int>(TK::LONG)}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            const char *s = wordlist[key].keyword;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &wordlist[key];
          }
    }
  return 0;
}
#line 88 "/home/joshy/university/bacherlorthesis/qcp/src/keywords.gperf"

// ---------------------------------------------------------------------------
} // end namespace token
} // end namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKEN_RESERVED_KEYWORD_HASH_H