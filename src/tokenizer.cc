#include "keywords.h"
#include "tokenizer.h"
// ---------------------------------------------------------------------------
#include <algorithm>
#include <array>
#include <cassert>
#include <iomanip>
#include <tuple>
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using TokenType = qcp::TokenType;
using sv_it = std::string_view::const_iterator;
// ---------------------------------------------------------------------------
bool isSeparator(const char c) {
   return c == ' ' or c == '\t' or c == '\n';
}
// ---------------------------------------------------------------------------
bool isNoneDigit(const char c) {
   return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}
// ---------------------------------------------------------------------------
bool isDigit(const char c) {
   return c >= '0' and c <= '9';
}
// ---------------------------------------------------------------------------
bool isIdentStart(const char c) {
   return isNoneDigit(c);
}
// ---------------------------------------------------------------------------
bool isIdentCont(const char c) {
   return isNoneDigit(c) || isDigit(c);
}
// ---------------------------------------------------------------------------
bool isOctalDigit(const char c) {
   return c >= '0' and c <= '7';
}
// ---------------------------------------------------------------------------
bool isHexDigit(const char c) {
   return (c >= '0' and c <= '9') or (c >= 'A' and c <= 'F') or (c >= 'a' and c <= 'f');
}
// ---------------------------------------------------------------------------
bool isBinaryDigit(const char c) {
   return c == '0' or c == '1';
}
// ---------------------------------------------------------------------------
bool isPunctuatorStart(const char c) {
   switch (c) {
      case '[':
      case ']':
      case '(':
      case ')':
      case '{':
      case '}':
      case '.':
      case '-':
      case '+':
      case '&':
      case '*':
      case '~':
      case '!':
      case '/':
      case '%':
      case '<':
      case '>':
      case '=':
      case '^':
      case '|':
      case '?':
      case ':':
      case ';':
      case '#':
      case ',':
         return true;
      default:
         return false;
   }
}
// ---------------------------------------------------------------------------
bool isSimpleEscapeSequenceChar(const char c) {
   switch (c) {
      case '\'':
      case '\"':
      case '\?':
      case '\\':
      case 'a':
      case 'b':
      case 'f':
      case 'n':
      case 'r':
      case 't':
      case 'v':
         return true;
      default:
         return false;
   }
}
// ---------------------------------------------------------------------------
std::pair<size_t, TokenType> getPunctuator(sv_it beginIt, sv_it endIt) {
   TokenType type;
   unsigned len = 1;
   std::array<char, 3> chars = {0, 0, 0};

   for (int i = 0; i < 3; ++i) {
      if (beginIt + i == endIt) {
         break;
      }
      chars[i] = *(beginIt + i);
   }

   switch (chars[0]) {
      case '[':
         type = TokenType::L_SQ_BRKT;
         break;
      case ']':
         type = TokenType::R_SQ_BRKT;
         break;
      case '(':
         type = TokenType::L_BRKT;
         break;
      case ')':
         type = TokenType::R_BRKT;
         break;
      case '{':
         type = TokenType::L_C_BRKT;
         break;
      case '}':
         type = TokenType::R_C_BRKT;
         break;
      case '~':
         type = TokenType::BW_INV;
         break;
      case ',':
         type = TokenType::COMMA;
         break;
      case ';':
         type = TokenType::SEMICOLON;
         break;
      case '?':
         type = TokenType::QMARK;
         break;
      case '+':
         switch (chars[1]) {
            case '+':
               len = 2;
               type = TokenType::INC;
               break;
            case '=':
               len = 2;
               type = TokenType::ADD_ASSIGN;
               break;
            default:
               type = TokenType::PLUS;
               break;
         }
         break;
      case '-':
         len = 2;
         switch (chars[1]) {
            case '-':
               type = TokenType::DEC;
               break;
            case '>':
               type = TokenType::DEREF;
               break;
            case '=':
               type = TokenType::SUB_ASSIGN;
               break;
            default:
               type = TokenType::MINUS;
               len = 1;
               break;
         }
         break;
      case '*':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TokenType::MUL_ASSIGN;
               break;
            default:
               type = TokenType::MUL;
               break;
         }
         break;
      case '/':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TokenType::DIV_ASSIGN;
               break;
            default:
               type = TokenType::DIV;
               break;
         }
         break;
      case '%':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TokenType::MOD_ASSIGN;
               break;
            default:
               type = TokenType::MOD;
               break;
         }
         break;
      case '&':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TokenType::BW_AND_ASSIGN;
               break;
            case '&':
               len = 2;
               type = TokenType::L_AND;
               break;
            default:
               type = TokenType::BW_AND;
               break;
         }
         break;
      case '|':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TokenType::BW_OR_ASSIGN;
               break;
            case '|':
               len = 2;
               type = TokenType::L_OR;
               break;
            default:
               type = TokenType::BW_OR;
               break;
         }
         break;
      case '^':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TokenType::BW_XOR_ASSIGN;
               break;
            default:
               type = TokenType::BW_XOR;
               break;
         }
         break;
      case '!':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TokenType::NE;
               break;
            default:
               type = TokenType::NEG;
               break;
         }
         break;
      case '=':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TokenType::EQ;
               break;
            default:
               type = TokenType::ASSIGN;
               break;
         }
         break;
      case ':':
         switch (chars[1]) {
            case ':':
               len = 2;
               type = TokenType::D_COLON;
               break;
            default:
               type = TokenType::COLON;
               break;
         }
         break;
      case '<':
         switch (chars[1]) {
            case '<':
               len = 2;
               switch (chars[2]) {
                  case '=':
                     len = 3;
                     type = TokenType::SHL_ASSIGN;
                     break;
                  default:
                     type = TokenType::SHL;
                     break;
               }
               break;
            case '=':
               len = 2;
               type = TokenType::LE;
               break;
            default:
               type = TokenType::L_P_BRKT;
               break;
         }
         break;
      case '>':
         switch (chars[1]) {
            case '>':
               len = 2;
               switch (chars[2]) {
                  case '=':
                     len = 3;
                     type = TokenType::SHR_ASSIGN;
                     break;
                  default:
                     type = TokenType::SHR;
                     break;
               }
               break;
            case '=':
               len = 2;
               type = TokenType::GE;
               break;
            default:
               type = TokenType::R_P_BRKT;
               break;
         }
         break;
      case '.':
         if (chars[1] == '.' && chars[2] == '.') {
            len = 3;
            type = TokenType::ELLIPSIS;
         } else {
            type = TokenType::PERIOD;
         }
         break;
      default:
         assert(false && "unknown punctuator");
   }
   return std::make_pair(len, type);
}
// ---------------------------------------------------------------------------
template <typename _NumberPredicate>
sv_it findEndOfINumber(sv_it beginIt, sv_it endIt, _NumberPredicate _p) {
   sv_it it = beginIt;
   char prev;
   do {
      it = std::find_if_not(it, endIt, _p);
      if (it == endIt) {
         break;
      }
      prev = *it;
      if (*it == '\'') {
         ++it;
      }
   } while (it != endIt && !(prev == '\'' && *it == '\'') && _p(*it));
   if (prev == '\'') {
      assert(false && "invalid number: number may not end with '");
   }
   return it;
}
// ---------------------------------------------------------------------------
struct BinaryExpontent {
   static constexpr char exponentCharUpper = 'P';
   static constexpr char exponentCharLower = 'p';
};
// ---------------------------------------------------------------------------
struct DecimalExponent {
   static constexpr char exponentCharUpper = 'E';
   static constexpr char exponentCharLower = 'e';
};
// ---------------------------------------------------------------------------
template <typename _Exponent>
   requires std::is_same_v<_Exponent, BinaryExpontent> or std::is_same_v<_Exponent, DecimalExponent>
sv_it findEndOfExponent(sv_it beginIt, sv_it endIt) {
   if (beginIt != endIt && (*beginIt == _Exponent::exponentCharUpper or *beginIt == _Exponent::exponentCharLower)) {
      ++beginIt;
      if (beginIt != endIt && (*beginIt == '+' or *beginIt == '-')) {
         ++beginIt;
      }
      beginIt = findEndOfINumber(beginIt, endIt, isDigit);
   }
   return beginIt;
}
// ---------------------------------------------------------------------------
template <typename T, typename U>
T safe_cast(U value) {
   if (value > std::numeric_limits<T>::max()) {
      assert(false && "value to large for safe cast");
   }
   return static_cast<T>(value);
}
// ---------------------------------------------------------------------------
std::tuple<std::size_t, qcp::Token> getNumberConst(sv_it beginIt, sv_it endIt) {
   /*
   floating-constant
      decimal-floating-constant
         fractional-constant exponent-partopt floating-suffixop
         digit-sequence exponent-part floating-suffixop
      hexadecimal-floating-constant
         hexadecimal-prefix hexadecimal-fractional-constant binary-exponent-part floating-suffixop
         hexadecimal-prefix hexadecimal-digit-sequence binary-exponent-part floating-suffixop
   integer-constant:
      decimal-constant integer-suffixopt
      octal-constant integer-suffixopt
      hexadecimal-constant integer-suffixopt
      binary-constant integer-suffixopt
   

   fractional-constant:
      digit-sequenceopt . digit-sequence
      digit-sequence .
   exponent-part:
      e signopt digit-sequence
      E signopt digit-sequence
   */
   sv_it valueEndIt = beginIt;
   sv_it suffixEndIt;
   int offset = 0;
   int base;
   bool isFloat = false;

   const char second = (beginIt + 1 != endIt) ? *(beginIt + 1) : 0;
   if (*beginIt == '0' && (second == 'x' or second == 'X')) {
      // hex or hex-float
      auto first = beginIt + 2;
      base = 16;
      offset = 2;
      if (first != endIt && *first == '.') {
         isFloat = true;
      }
      valueEndIt = findEndOfINumber(first, endIt, isHexDigit);
      if (valueEndIt != endIt && *valueEndIt == '.') {
         isFloat = true;
         valueEndIt = findEndOfINumber(valueEndIt + 1, endIt, isHexDigit);
      }
      if (valueEndIt != endIt && (*valueEndIt == 'p' or *valueEndIt == 'P')) {
         isFloat = true;
      }

      if (isFloat) {
         offset = 0;
      } else {
         assert(valueEndIt != first && "invalid hex number");
      }
   } else if (*beginIt == '0' && (second == 'b' or second == 'B')) {
      // binary
      valueEndIt = findEndOfINumber(beginIt + 2, endIt, isBinaryDigit);
      base = 2;
      offset = 2;
      assert(valueEndIt != beginIt + 2 && "invalid binary number");
   } else {
      // decimal, octal or decimal-float
      auto first = beginIt;
      if (*first == '.') {
         isFloat = true;
      }
      valueEndIt = findEndOfINumber(first, endIt, isDigit);
      if (valueEndIt != endIt && *valueEndIt == '.') {
         isFloat = true;
         valueEndIt = findEndOfINumber(valueEndIt + 1, endIt, isDigit);
      }
      if (!isFloat && *beginIt == '0') {
         base = 8;
         if (findEndOfINumber(beginIt + 1, endIt, isOctalDigit) != valueEndIt) {
            assert(false && "invalid octal number");
         }
      } else {
         base = 10;
         if (valueEndIt != endIt && (*valueEndIt == 'e' or *valueEndIt == 'E')) {
            isFloat = true;
         }
      }
   }
   if (isFloat) {
      if (base == 16) {
         valueEndIt = findEndOfExponent<BinaryExpontent>(valueEndIt, endIt);
      } else {
         valueEndIt = findEndOfExponent<DecimalExponent>(valueEndIt, endIt);
      }
   }

   std::string valueRepr{beginIt + offset, valueEndIt};
   auto valueRepreEndIt = std::remove(valueRepr.begin(), valueRepr.end(), '\'');
   valueRepr.erase(valueRepreEndIt, valueRepr.end());

   bool unsignedSuffix = false;
   bool longSuffix = false;
   bool longLongSuffix = false;
   bool bitPreciseIntSuffix = false;
   bool floatSuffix = false;
   bool doubleSuffix = false;

   (void) doubleSuffix; // todo: (jr)
   (void) floatSuffix; // todo: (jr)
   (void) bitPreciseIntSuffix; // todo: (jr)

   suffixEndIt = valueEndIt;
   if (suffixEndIt != endIt) {
      const char first = *suffixEndIt;
      if (first == 'u' or first == 'U') {
         unsignedSuffix = true;
         ++suffixEndIt;
      } else if (first == 'f' or first == 'F') {
         floatSuffix = true;
         ++suffixEndIt;
      }

      if (suffixEndIt != endIt) {
         const char second = *suffixEndIt;
         const char third = suffixEndIt + 1 != endIt ? *(suffixEndIt + 1) : 0;
         if (second == 'l' or second == 'L') {
            longSuffix = true;
            ++suffixEndIt;
            if (suffixEndIt != endIt) {
               if (second == third) {
                  longLongSuffix = true;
                  longSuffix = false;
                  ++suffixEndIt;
               }
            }
         } else if (second == 'w' and third == 'b') {
            bitPreciseIntSuffix = true;
            suffixEndIt += 2;
         } else if (second == 'W' and third == 'B') {
            bitPreciseIntSuffix = true;
            suffixEndIt += 2;
         }
      }
      if (!unsignedSuffix && suffixEndIt != endIt) {
         const char third = *suffixEndIt;
         if (third == 'u' or third == 'U') {
            unsignedSuffix = true;
            ++suffixEndIt;
         }
      }
   }

   qcp::Token t;
   errno = 0;
   char* pos;

   if (isFloat) {
      double value = std::strtod(valueRepr.c_str(), &pos);
      t = qcp::Token{value};
   } else if (unsignedSuffix) {
      unsigned long long value = std::strtoull(valueRepr.c_str(), &pos, base);
      if (longLongSuffix) {
         t = qcp::Token{value};
      } else if (longSuffix) {
         t = qcp::Token{safe_cast<unsigned long>(value)};
      } else {
         t = qcp::Token{safe_cast<unsigned>(value)};
      }
   } else {
      long long value = static_cast<long long>(std::strtoull(valueRepr.c_str(), &pos, base));
      if (longLongSuffix) {
         t = qcp::Token{safe_cast<long long>(value)};
      } else if (longSuffix) {
         t = qcp::Token{safe_cast<long>(value)};
      } else {
         t = qcp::Token{safe_cast<int>(value)};
      }
   }

   if (errno == ERANGE) {
      assert(false && "range error");
   } else if (pos != valueRepr.c_str() + valueRepr.length()) {
      assert(false && "invalid number");
   }

   return std::make_tuple(std::distance(beginIt, suffixEndIt), t);
}
// ---------------------------------------------------------------------------
struct CCharSequence {
   static constexpr char quoteChar = '\'';
};
// ---------------------------------------------------------------------------
struct SCharSequence {
   static constexpr char quoteChar = '"';
};
// ---------------------------------------------------------------------------
/**
 * @brief Returns the length of the character sequence that is part of a string literal or character constant.
 * 
 * @tparam _CharSequence The character sequence type. (CCharSequence or SCharSequence)
 * @param beginIt An iterator pointing to the first character of the string literal.
 * @param endIt An iterator pointing to the end of the string literal (excluding the "/' character).
 * @return The length of the character sequence.
 */
template <typename _CharSequence>
   requires std::is_same_v<_CharSequence, CCharSequence> or std::is_same_v<_CharSequence, SCharSequence>
std::size_t getCharSequencLength(sv_it beginIt, sv_it endIt) {
   sv_it it = beginIt;
   while (it != endIt) {
      it = std::find_if(it, endIt, [](const char c) { return c == '\\' || c == _CharSequence::quoteChar || c == '\n'; });
      if (it == endIt) {
         break;
      }
      if (*it == '\\') {
         if (it + 1 == endIt) {
            break;
         }
         ++it;
         if (isOctalDigit(*it)) {
            // octal
            int octalCount;
            for (octalCount = 1; octalCount < 3 && (it + octalCount) != endIt; ++octalCount) {
               if (!isOctalDigit(*(it + octalCount))) {
                  break;
               }
            }
            it += octalCount;
            // todo: (jr) handle octal values
         } else if (*it == 'x') {
            // hex
            ++it;
            int hexCount = 0;
            while (it != endIt && isHexDigit(*it)) {
               ++it;
               ++hexCount;
            }
            if (hexCount == 0) {
               assert(false && "invalid hex escape sequence");
            }
            // todo: (jr) handle hex values
         } else if (isSimpleEscapeSequenceChar(*it)) {
            ++it;
         } else {
            assert(false && "unknown escape sequence");
         }

      } else {
         break;
      }
   }
   return std::distance(beginIt, it);
}
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
Tokenizer::const_iterator& Tokenizer::const_iterator::operator++() {
   if (remainder.empty()) {
      token = Token{TokenType::END};
      return *this;
   }

   // find begin of next word
   auto beginIt = std::find_if(remainder.begin(), remainder.end(), [](char c) { return isPunctuatorStart(c) or isIdentStart(c) or isDigit(c) or c == '"' or c == '\''; });
   if (beginIt == remainder.end()) {
      token = Token{TokenType::END};
      return *this;
   }
   // todo: implement custom class with src code with a peek() method
   char second = (beginIt + 1 != remainder.end()) ? *(beginIt + 1) : 0;
   std::size_t whitespace = std::distance(remainder.begin(), beginIt);
   bool requiresSeparator = false;

   if (isPunctuatorStart(*beginIt) and not(*beginIt == '.' and isDigit(second))) {
      auto [len, type] = getPunctuator(beginIt, remainder.end());
      remainder = remainder.substr(len + whitespace);
      token = Token{type};
   } else if (isIdentStart(*beginIt)) {
      requiresSeparator = true;
      // todo: (jr) u8, u, U, L prefix not supported
      auto endIt = std::find_if_not(beginIt, remainder.end(), isIdentCont);
      std::string_view ident{beginIt, endIt};
      remainder = remainder.substr(ident.length() + whitespace);
      const GPerfToken* t = ReservedKeywordHash::isInWordSet(ident.data(), ident.size());
      // todo: (jr) handle constants
      if (t) {
         token = Token{*t};
      } else {
         token = Token{ident};
      }
   } else if (isDigit(*beginIt) or (*beginIt == '.' and isDigit(second))) {
      requiresSeparator = true;
      auto [len, t] = getNumberConst(beginIt, remainder.end());
      remainder = remainder.substr(len + whitespace);
      token = t;
   } else if (*beginIt == '"' or *beginIt == '\'') {
      // todo: (jr) handle encoding prefix
      std::size_t len;
      if (*beginIt == '"') {
         len = getCharSequencLength<SCharSequence>(beginIt + 1, remainder.end());
      } else {
         len = getCharSequencLength<CCharSequence>(beginIt + 1, remainder.end());
      }
      auto endIt = beginIt + len + 1;
      if (endIt == remainder.end()) {
         assert(false && "missing closing '\"' or '\\''");
      } else if (*endIt != *beginIt) {
         assert(false && "invalid string/char literal. missing closing '\"' or '\\''");
      }
      remainder = remainder.substr(len + 2 + whitespace);
      token = Token{std::string_view{beginIt + 1, len}, TokenType::LITERAL};
   }

   if (requiresSeparator && !remainder.empty()) {
      const char c = remainder.front();
      if (!(isSeparator(c) || isPunctuatorStart(c) || c == ']')) {
         assert(false && "Token not fully consumed");
      }
   }
   return *this;
}
// ---------------------------------------------------------------------------
Tokenizer::const_iterator Tokenizer::const_iterator::operator++(int) {
   const_iterator tmp = *this;
   ++(*this);
   return tmp;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------