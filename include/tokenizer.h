#ifndef QCP_TOKENIZER_H
#define QCP_TOKENIZER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
#include "keywords.h"
#include "tokenizer.h"
// ---------------------------------------------------------------------------
#include <algorithm>
#include <array>
#include <cassert>
#include <iomanip>
#include <limits>
#include <tuple>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
using sv_it = std::string_view::const_iterator;
// ---------------------------------------------------------------------------
class DiagnosticTracker;
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker = DiagnosticTracker>
class Tokenizer {
   using TK = token::Kind;
   using Token = token::Token;

   public:
   explicit Tokenizer(const std::string_view prog, _DiagnosticTracker& diagnistics) : prog_{prog}, diagnostics_{diagnistics} {}

   explicit Tokenizer(const Tokenizer&) = delete;
   Tokenizer& operator=(const Tokenizer&) = delete;

   class const_iterator {
      public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = const Token;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type*;
      using reference = value_type&;

      private:
      public:
      explicit const_iterator() : token_{TK::END}, prog_{}, diagnostics_{nullptr} {}
      explicit const_iterator(const std::string_view prog_, _DiagnosticTracker& diagnistics) : token_{TK::UNKNOWN}, prog_{prog_}, diagnostics_{&diagnistics} {
         ++(*this);
      }

      const_iterator& operator++();
      const_iterator operator++(int);

      reference operator*() const {
         return token_;
      }

      pointer operator->() const {
         return &token_;
      }

      bool operator==(const const_iterator& other) const {
         return token_ == other.token_;
      }

      auto operator<=>(const const_iterator& other) const {
         return token_ <=> other.token_;
      }

      private:
      sv_it getNumberConst(sv_it begin);
      sv_it getPunctuator(sv_it begin);
      sv_it getSCharSequence(sv_it begin);
      sv_it getCCharSequence(sv_it begin);

      Token token_;
      std::string_view prog_;
      DiagnosticTracker* diagnostics_;
   };

   const_iterator begin() const {
      return const_iterator{prog_, diagnostics_};
   }

   const_iterator end() const {
      return const_iterator{};
   }

   const_iterator cbegin() const {
      return begin();
   }

   const_iterator cend() const {
      return end();
   }

   const std::string_view& data() const {
      return prog_;
   }

   private:
   const std::string_view prog_;
   _DiagnosticTracker& diagnostics_;
};
// ---------------------------------------------------------------------------
// Implementation
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
template <typename T, typename U>
T safe_cast(U value) {
   if (value > std::numeric_limits<T>::max()) {
      assert(false && "value to large for safe cast");
   }
   return static_cast<T>(value);
}
// ---------------------------------------------------------------------------
template <typename _NumberPredicate, typename _DiagnosticTracker>
sv_it findEndOfINumber(sv_it begin, sv_it end, _NumberPredicate _p, _DiagnosticTracker& diagnostics) {
   auto& numberEnd{begin};
   char prev;
   do {
      numberEnd = std::find_if_not(numberEnd, end, _p);
      if (numberEnd == end) {
         break;
      }
      prev = *numberEnd;
      if (*numberEnd == '\'') {
         ++numberEnd;
      }
   } while (numberEnd != end && !(prev == '\'' && *numberEnd == '\'') && _p(*numberEnd));
   if (prev == '\'') {
      diagnostics.report("invalid number: number may not end with '");
   }
   return numberEnd;
}
// ---------------------------------------------------------------------------
template <const char lowercaseExponentChar, const char uppercaseExponentChar, typename _DiagnosticTracker>
sv_it findEndOfExponent(sv_it begin, sv_it end, _DiagnosticTracker& diagnostics) {
   auto& expEnd{begin};

   if (expEnd != end && (*expEnd == lowercaseExponentChar or *expEnd == uppercaseExponentChar)) {
      ++expEnd;
      if (expEnd != end && (*expEnd == '+' or *expEnd == '-')) {
         ++expEnd;
      }
      expEnd = findEndOfINumber(expEnd, end, isDigit, diagnostics);
   }
   return expEnd;
}
// ---------------------------------------------------------------------------
template <const char quoteChar, typename _DiagnosticTracker>
sv_it getCharSequence(sv_it begin, sv_it end, _DiagnosticTracker& diagnostics) {
   sv_it cSeqEnd{begin + 1};
   while (cSeqEnd != end) {
      cSeqEnd = std::find_if(cSeqEnd, end, [](const char c) { return c == '\\' || c == '"' || c == '\n'; });
      if (cSeqEnd == end) {
         diagnostics << "unterminated character sequence";
         break;
      }

      if (*cSeqEnd == '\\') {
         if (cSeqEnd + 1 == end) {
            diagnostics << "unterminated character sequence";
            break;
         }
         ++cSeqEnd;
         if (isOctalDigit(*cSeqEnd)) {
            // octal
            int octalCount;
            for (octalCount = 1; octalCount < 3 && (cSeqEnd + octalCount) != end; ++octalCount) {
               if (!isOctalDigit(*(cSeqEnd + octalCount))) {
                  diagnostics << "invalid octal escape sequence";
                  break;
               }
            }
            cSeqEnd += octalCount;
            // todo: (jr) handle octal values
         } else if (*cSeqEnd == 'x') {
            // hex
            ++cSeqEnd;
            int hexCount = 0;
            while (cSeqEnd != end && isHexDigit(*cSeqEnd)) {
               ++cSeqEnd;
               ++hexCount;
            }
            if (hexCount == 0) {
               diagnostics << "invalid hex escape sequence";
            }
            // todo: (jr) handle hex values
         } else if (isSimpleEscapeSequenceChar(*cSeqEnd)) {
            ++cSeqEnd;
         } else {
            diagnostics << "unknown escape sequence";
         }
      } else if (*cSeqEnd == '\n') {
         diagnostics << "unterminated character sequence";
         break;
      } else {
         // end of sequence found
         break;
      }
   }

   if (cSeqEnd == end || *cSeqEnd != quoteChar) {
      return cSeqEnd;
   }

   ++cSeqEnd;
   return cSeqEnd;
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
sv_it Tokenizer<_DiagnosticTracker>::const_iterator::getNumberConst(sv_it begin) {
   sv_it valueEnd{begin};
   sv_it suffixEnd;
   int base;
   bool isFloat = false;

   const char second = (begin + 1 != prog_.end()) ? *(begin + 1) : 0;
   if (*begin == '0' && (second == 'x' or second == 'X')) {
      // hex or hex-float
      begin += 2;
      valueEnd = begin;
      base = 16;

      if (valueEnd != prog_.end() && *valueEnd == '.') {
         isFloat = true;
      }
      valueEnd = findEndOfINumber(valueEnd, prog_.end(), isHexDigit, *diagnostics_);
      if (valueEnd != prog_.end() && *valueEnd == '.') {
         isFloat = true;
         valueEnd = findEndOfINumber(valueEnd + 1, prog_.end(), isHexDigit, *diagnostics_);
      }
      if (valueEnd != prog_.end() && (*valueEnd == 'p' || *valueEnd == 'P')) {
         isFloat = true;
      }

      if (isFloat) {
         // for hex float, strtod expects the '0x' prefix to be included
         begin -= 2;
      } else if (valueEnd == begin) {
         *diagnostics_ << "invalid hex number";
      }
   } else if (*begin == '0' && (second == 'b' or second == 'B')) {
      // binary
      base = 2;
      begin += 2;
      valueEnd = findEndOfINumber(begin, prog_.end(), isBinaryDigit, *diagnostics_);
      if (valueEnd == begin) {
         *diagnostics_ << "invalid binary number";
      }
   } else {
      // decimal, octal or decimal-float
      valueEnd = begin;
      if (*valueEnd == '.') {
         isFloat = true;
      }
      valueEnd = findEndOfINumber(valueEnd, prog_.end(), isDigit, *diagnostics_);
      if (valueEnd != prog_.end() && *valueEnd == '.') {
         isFloat = true;
         valueEnd = findEndOfINumber(valueEnd + 1, prog_.end(), isDigit, *diagnostics_);
      }
      if (!isFloat && *begin == '0') {
         base = 8;
         if (findEndOfINumber(begin + 1, prog_.end(), isOctalDigit, *diagnostics_) != valueEnd) {
            *diagnostics_ << "invalid octal number";
         }
      } else {
         base = 10;
         if (valueEnd != prog_.end() && (*valueEnd == 'e' or *valueEnd == 'E')) {
            isFloat = true;
         }
      }
   }
   if (isFloat) {
      if (base == 16) {
         valueEnd = findEndOfExponent<'p', 'P'>(valueEnd, prog_.end(), *diagnostics_);
      } else {
         valueEnd = findEndOfExponent<'e', 'E'>(valueEnd, prog_.end(), *diagnostics_);
      }
   }

   std::string valueRepr{begin, valueEnd};
   // sanitize value representation (remove single quotes)
   auto valueReprEnd = std::remove(valueRepr.begin(), valueRepr.end(), '\'');
   valueRepr.erase(valueReprEnd, valueRepr.end());

   bool unsignedSuffix = false;
   bool longSuffix = false;
   bool longLongSuffix = false;
   bool bitPreciseIntSuffix = false;
   bool floatSuffix = false;
   bool doubleSuffix = false;

   (void) doubleSuffix; // todo: (jr) longdouble
   (void) bitPreciseIntSuffix; // todo: (jr)

   suffixEnd = valueEnd;
   if (suffixEnd != prog_.end()) {
      const char first = *suffixEnd;
      if (first == 'u' or first == 'U') {
         unsignedSuffix = true;
         ++suffixEnd;
      } else if (first == 'f' or first == 'F') {
         floatSuffix = true;
         ++suffixEnd;
      }

      if (suffixEnd != prog_.end()) {
         const char second = *suffixEnd;
         const char third = suffixEnd + 1 != prog_.end() ? *(suffixEnd + 1) : 0;
         if (second == 'l' or second == 'L') {
            longSuffix = true;
            ++suffixEnd;
            if (suffixEnd != prog_.end()) {
               if (second == third) {
                  longLongSuffix = true;
                  longSuffix = false;
                  ++suffixEnd;
               }
            }
         } else if (second == 'w' and third == 'b') {
            bitPreciseIntSuffix = true;
            suffixEnd += 2;
         } else if (second == 'W' and third == 'B') {
            bitPreciseIntSuffix = true;
            suffixEnd += 2;
         }
      }
      if (!unsignedSuffix && suffixEnd != prog_.end()) {
         const char third = *suffixEnd;
         if (third == 'u' or third == 'U') {
            unsignedSuffix = true;
            ++suffixEnd;
         }
      }
   }

   errno = 0;
   char* pos;

   if (isFloat) {
      double value = std::strtod(valueRepr.c_str(), &pos);
      if (floatSuffix) {
         token_ = Tokenizer::Token{safe_cast<float>(value)};
      } else {
         token_ = Tokenizer::Token{value};
      }
   } else if (unsignedSuffix) {
      unsigned long long value = std::strtoull(valueRepr.c_str(), &pos, base);
      if (longLongSuffix) {
         token_ = Tokenizer::Token{value};
      } else if (longSuffix) {
         token_ = Tokenizer::Token{safe_cast<unsigned long>(value)};
      } else {
         token_ = Tokenizer::Token{safe_cast<unsigned>(value)};
      }
   } else {
      long long value = static_cast<long long>(std::strtoull(valueRepr.c_str(), &pos, base));
      if (longLongSuffix) {
         token_ = Tokenizer::Token{safe_cast<long long>(value)};
      } else if (longSuffix) {
         token_ = Tokenizer::Token{safe_cast<long>(value)};
      } else {
         token_ = Tokenizer::Token{safe_cast<int>(value)};
      }
   }

   if (errno == ERANGE) {
      *diagnostics_ << "range error";
   } else if (pos != valueRepr.c_str() + valueRepr.length()) {
      *diagnostics_ << "invalid number";
   }

   return suffixEnd;
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
sv_it Tokenizer<_DiagnosticTracker>::const_iterator::getPunctuator(sv_it begin) {
   TK type;
   unsigned len = 1;
   std::array<char, 3> chars = {0, 0, 0};

   for (int i = 0; i < 3; ++i) {
      if (begin + i == prog_.end()) {
         break;
      }
      chars[i] = *(begin + i);
   }

   switch (chars[0]) {
      case '[':
         type = TK::L_BRACKET;
         break;
      case ']':
         type = TK::R_BRACKET;
         break;
      case '(':
         type = TK::L_BRACE;
         break;
      case ')':
         type = TK::R_BRACE;
         break;
      case '{':
         type = TK::L_C_BRKT;
         break;
      case '}':
         type = TK::R_C_BRKT;
         break;
      case '~':
         type = TK::BW_INV;
         break;
      case ',':
         type = TK::COMMA;
         break;
      case ';':
         type = TK::SEMICOLON;
         break;
      case '?':
         type = TK::QMARK;
         break;
      case '+':
         switch (chars[1]) {
            case '+':
               len = 2;
               type = TK::INC;
               break;
            case '=':
               len = 2;
               type = TK::ADD_ASSIGN;
               break;
            default:
               type = TK::PLUS;
               break;
         }
         break;
      case '-':
         len = 2;
         switch (chars[1]) {
            case '-':
               type = TK::DEC;
               break;
            case '>':
               type = TK::DEREF;
               break;
            case '=':
               type = TK::SUB_ASSIGN;
               break;
            default:
               type = TK::MINUS;
               len = 1;
               break;
         }
         break;
      case '*':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TK::MUL_ASSIGN;
               break;
            default:
               type = TK::ASTERISK;
               break;
         }
         break;
      case '/':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TK::DIV_ASSIGN;
               break;
            default:
               type = TK::DIV;
               break;
         }
         break;
      case '%':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TK::REM_ASSIGN;
               break;
            default:
               type = TK::PERCENT;
               break;
         }
         break;
      case '&':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TK::BW_AND_ASSIGN;
               break;
            case '&':
               len = 2;
               type = TK::L_AND;
               break;
            default:
               type = TK::BW_AND;
               break;
         }
         break;
      case '|':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TK::BW_OR_ASSIGN;
               break;
            case '|':
               len = 2;
               type = TK::L_OR;
               break;
            default:
               type = TK::BW_OR;
               break;
         }
         break;
      case '^':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TK::BW_XOR_ASSIGN;
               break;
            default:
               type = TK::BW_XOR;
               break;
         }
         break;
      case '!':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TK::NE;
               break;
            default:
               type = TK::NEG;
               break;
         }
         break;
      case '=':
         switch (chars[1]) {
            case '=':
               len = 2;
               type = TK::EQ;
               break;
            default:
               type = TK::ASSIGN;
               break;
         }
         break;
      case ':':
         switch (chars[1]) {
            case ':':
               len = 2;
               type = TK::D_COLON;
               break;
            default:
               type = TK::COLON;
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
                     type = TK::SHL_ASSIGN;
                     break;
                  default:
                     type = TK::SHL;
                     break;
               }
               break;
            case '=':
               len = 2;
               type = TK::LE;
               break;
            default:
               type = TK::L_ANGLE;
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
                     type = TK::SHR_ASSIGN;
                     break;
                  default:
                     type = TK::SHR;
                     break;
               }
               break;
            case '=':
               len = 2;
               type = TK::GE;
               break;
            default:
               type = TK::R_ANGLE;
               break;
         }
         break;
      case '.':
         if (chars[1] == '.' && chars[2] == '.') {
            len = 3;
            type = TK::ELLIPSIS;
         } else {
            type = TK::PERIOD;
         }
         break;
      default:
         type = TK::UNKNOWN;
         *diagnostics_ << "unknown punctuator";
   }
   token_ = Token{type};
   return begin + len;
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
sv_it Tokenizer<_DiagnosticTracker>::const_iterator::getSCharSequence(sv_it begin) {
   sv_it cSeqEnd = getCharSequence<'"'>(begin, prog_.end(), *diagnostics_);
   token_ = Token{std::string_view{begin + 1, cSeqEnd - 1}, TK::SLITERAL};
   return cSeqEnd;
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
sv_it Tokenizer<_DiagnosticTracker>::const_iterator::getCCharSequence(sv_it begin) {
   sv_it cSeqEnd = getCharSequence<'\''>(begin, prog_.end(), *diagnostics_);
   token_ = Token{std::string_view{begin + 1, cSeqEnd - 1}, TK::CLITERAL};
   return cSeqEnd;
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
Tokenizer<_DiagnosticTracker>::const_iterator& Tokenizer<_DiagnosticTracker>::const_iterator::operator++() {
   if (prog_.empty()) {
      token_ = Token{TK::END};
      return *this;
   }

   // find begin of next word
   auto begin = std::find_if(prog_.begin(), prog_.end(), [](char c) { return isPunctuatorStart(c) or isIdentStart(c) or isDigit(c) or c == '"' or c == '\''; });
   if (begin == prog_.end()) {
      token_ = Token{TK::END};
      return *this;
   }
   prog_ = prog_.substr(std::distance(prog_.begin(), begin));

   // todo: implement custom class with src code with a peek() method
   char second = (begin + 1 != prog_.end()) ? *(begin + 1) : 0;
   bool requiresSeparator = false;
   sv_it end;

   *diagnostics_ << SrcLoc{static_cast<std::size_t>(std::distance(prog_.begin(), begin)), 1u};

   if (isPunctuatorStart(*begin) && !(*begin == '.' and isDigit(second))) {
      end = getPunctuator(begin);
   } else if (isIdentStart(*begin)) {
      requiresSeparator = true;
      // todo: (jr) u8, u, U, L prefix not supported
      end = std::find_if_not(begin, prog_.end(), isIdentCont);
      std::string_view ident{begin, end};
      const token::GPerfToken* t = token::ReservedKeywordHash::isInWordSet(ident.data(), ident.size());
      // todo: (jr) handle constants
      if (t) {
         token_ = Token{*t};
      } else {
         token_ = Token{ident};
      }
   } else if (isDigit(*begin) or (*begin == '.' and isDigit(second))) {
      requiresSeparator = true;
      end = getNumberConst(begin);
   } else if (*begin == '"') {
      end = getSCharSequence(begin);
   } else if (*begin == '\'') {
      end = getCCharSequence(begin);
   } else {
      *diagnostics_ << "unknown token";
   }

   *diagnostics_ << SrcLoc{static_cast<std::size_t>(std::distance(prog_.begin(), begin)), static_cast<unsigned>(std::distance(begin, end))};

   prog_ = prog_.substr(std::distance(prog_.begin(), end));

   if (requiresSeparator && !prog_.empty()) {
      const char c = prog_.front();
      if (!(isSeparator(c) || isPunctuatorStart(c) || c == ']')) {
         *diagnostics_ << "token not fully consumed";
      }
   }

   return *this;
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
Tokenizer<_DiagnosticTracker>::const_iterator Tokenizer<_DiagnosticTracker>::const_iterator::operator++(int) {
   const_iterator tmp = *this;
   ++(*this);
   return tmp;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKENIZER_H