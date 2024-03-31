#include "keywords.h"
#include "tokenizer.h"
// ---------------------------------------------------------------------------
#include <algorithm>
#include <cassert>
#include <iomanip>
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using TokenType = qcp::TokenType;
using sv_it = std::string_view::const_iterator;
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
         std::cerr << "unknown punctuator: " << *beginIt << std::endl;
         assert(false && "unknown punctuator");
   }
   return std::make_pair(len, type);
}
// ---------------------------------------------------------------------------
std::tuple<std::size_t, qcp::Token> getIConst(sv_it beginIt, sv_it endIt) {
   sv_it valueEndIt;
   sv_it suffixEndIt;
   int offset = 0;
   int base;

   if (*beginIt == '0') {
      const char second = (beginIt + 1 != endIt) ? *(beginIt + 1) : 0;
      if (second == 'x' or second == 'X') {
         // hex
         valueEndIt = std::find_if_not(beginIt + 2, endIt, isHexDigit);
         base = 16;
         offset = 2;
      } else if (second == 'b' or second == 'B') {
         // binary
         valueEndIt = std::find_if_not(beginIt + 2, endIt, isBinaryDigit);
         base = 2;
         offset = 2;
      } else {
         // octal
         valueEndIt = std::find_if_not(beginIt + 1, endIt, isOctalDigit);
         base = 8;
      }
   } else {
      // decimal
      valueEndIt = std::find_if_not(beginIt, endIt, isDigit);
      base = 10;
   }
   std::string valueRepr{beginIt + offset, valueEndIt};

   bool unsignedSuffix = false;
   bool longSuffix = false;
   bool longLongSuffix = false;
   bool bitPreciseIntSuffix = false;

   suffixEndIt = valueEndIt;
   if (suffixEndIt != endIt) {
      const char first = *suffixEndIt;
      if (first == 'u' or first == 'U') {
         unsignedSuffix = true;
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
   std::size_t pos;
   if (unsignedSuffix && longLongSuffix) {
      unsigned long long value = std::stoull(valueRepr, &pos, base);
      t = qcp::Token{value};
   } else if (unsignedSuffix && longSuffix) {
      unsigned long value = std::stoul(valueRepr, &pos, base);
      t = qcp::Token{value};
   } else if (unsignedSuffix && bitPreciseIntSuffix) {
      assert(false && "bit precise int suffix not supported");
   } else if (longLongSuffix) {
      long long value = std::stoll(valueRepr, &pos, base);
      t = qcp::Token{value};
   } else if (longSuffix) {
      long value = std::stol(valueRepr, &pos, base);
      t = qcp::Token{value};
   } else if (bitPreciseIntSuffix) {
      assert(false && "bit precise int suffix not supported");
   } else if (unsignedSuffix) {
      // todo: (jr) handle to large values
      unsigned value = std::stoul(valueRepr, &pos, base);
      t = qcp::Token{value};

   } else {
      int value = std::stoi(valueRepr, &pos, base);
      t = qcp::Token{value};
   }

   if (errno == ERANGE) {
      std::cerr << "range error: " << valueRepr << std::endl;
      assert(false && "range error");
   } else if (pos != valueRepr.length()) {
      std::cerr << "invalid number: " << valueRepr << std::endl;
      assert(false && "invalid number");
   }

   return std::make_tuple(std::distance(beginIt, suffixEndIt), t);
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
   std::size_t whitespace = std::distance(remainder.begin(), beginIt);

   if (isPunctuatorStart(*beginIt)) {
      auto [len, type] = getPunctuator(beginIt, remainder.end());
      remainder = remainder.substr(len + whitespace);
      token = Token{type};
   } else if (isIdentStart(*beginIt)) {
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
   } else if (isDigit(*beginIt)) {
      auto [len, t] = getIConst(beginIt, remainder.end());
      remainder = remainder.substr(len + whitespace);
      token = t;
   } else if (*beginIt == '"') {
      assert(false && "string literal not supported");
   } else if (*beginIt == '\'') {
      assert(false && "char literal not supported");
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