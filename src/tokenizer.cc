// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "tokenizer.h"
// ---------------------------------------------------------------------------
#include <algorithm>
#include <cassert>
#include <string_view>
#include <utility>
#include <array>
// ---------------------------------------------------------------------------
using sv_it = typename std::string_view::const_iterator;
// ---------------------------------------------------------------------------
namespace { // anonymous
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
template <typename _NumberPredicate>
sv_it findEndOfINumber(sv_it begin, sv_it end, _NumberPredicate _p, qcp::DiagnosticTracker& diagnostics) {
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
      diagnostics << "invalid number: number may not end with '";
   }
   return numberEnd;
}
// ---------------------------------------------------------------------------
template <const char lowercaseExponentChar, const char uppercaseExponentChar>
sv_it findEndOfExponent(sv_it begin, sv_it end, qcp::DiagnosticTracker& diagnostics) {
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
template <const char quoteChar>
std::pair<std::string, sv_it> getCharSequence(sv_it progBegin, sv_it begin, sv_it end, qcp::DiagnosticTracker& diagnostics) {
   sv_it cSeqEnd{begin + 1};
   std::string literal{};
   while (cSeqEnd != end) {
      begin = cSeqEnd;
      cSeqEnd = std::find_if(cSeqEnd, end, [](const char c) { return c == '\\' || c == quoteChar || c == '\n'; });
      if (cSeqEnd == end) {
         diagnostics << "unterminated character sequence" << std::endl;
         break;
      }
      literal += std::string_view{begin, cSeqEnd};
      if (*cSeqEnd == '\\') {
         if (cSeqEnd + 1 == end) {
            diagnostics << "unterminated character sequence" << std::endl;
            break;
         }
         ++cSeqEnd;
         if (isOctalDigit(*cSeqEnd)) {
            // octal
            int octalCount;
            int value = 0;
            for (octalCount = 1; octalCount < 3 && (cSeqEnd + octalCount) != end; ++octalCount) {
               if (!isOctalDigit(*(cSeqEnd + octalCount))) {
                  diagnostics << "invalid octal escape sequence" << std::endl;
                  break;
               } else {
                  value = value * 8 + (*cSeqEnd - '0');
               }
            }
            cSeqEnd += octalCount;
            literal += static_cast<char>(value);
         } else if (*cSeqEnd == 'x') {
            // hex
            ++cSeqEnd;
            char* endPtr;
            long value = strtol(cSeqEnd, &endPtr, 16);
            // todo: too large char or w_char

            if (std::distance(&*cSeqEnd, static_cast<const char*>(endPtr)) == 0) {
               diagnostics << "invalid hex escape sequence" << std::endl;
            }
            cSeqEnd = endPtr;
            literal += static_cast<char>(value);
         } else if (*cSeqEnd == 'u' || *cSeqEnd == 'U') {
            // universal character name
            diagnostics << qcp::DiagnosticMessage::Kind::WARNING << "universal character name not supported" << std::endl;
         } else if (isSimpleEscapeSequenceChar(*cSeqEnd)) {
            literal += *cSeqEnd;
            ++cSeqEnd;
         } else {
            diagnostics << qcp::SrcLoc(progBegin, begin, cSeqEnd) << "unknown escape sequence" << std::endl;
         }
      } else if (*cSeqEnd == '\n') {
         diagnostics << "unterminated character sequence" << std::endl;
         break;
      } else {
         // end of sequence found
         break;
      }
   }

   if (cSeqEnd != end && *cSeqEnd == quoteChar) {
      ++cSeqEnd;
   }

   return {literal, cSeqEnd};
}
// ---------------------------------------------------------------------------
} // anonymous
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
// tokenizer
// ---------------------------------------------------------------------------
Tokenizer::const_iterator Tokenizer::begin() const {
   return const_iterator{prog_, diagnostics_};
}
// ---------------------------------------------------------------------------
Tokenizer::const_iterator Tokenizer::end() const {
   return const_iterator{};
}
// ---------------------------------------------------------------------------
Tokenizer::const_iterator Tokenizer::cbegin() const {
   return begin();
}
// ---------------------------------------------------------------------------
Tokenizer::const_iterator Tokenizer::cend() const {
   return end();
}
// ---------------------------------------------------------------------------
const std::string_view& Tokenizer::data() const {
   return prog_;
}
// ---------------------------------------------------------------------------
// const_iterator
// ---------------------------------------------------------------------------
sv_it Tokenizer::const_iterator::getNumberConst(sv_it begin) {
   sv_it valueEnd{begin};
   sv_it suffixEnd;
   int base;
   bool isFloat = false;
   bool valid = true;

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

      if (valueEnd == begin) {
         *diagnostics_ << "invalid hex number" << std::endl;
         valid = false;
      }
   } else if (*begin == '0' && (second == 'b' or second == 'B')) {
      // binary
      base = 2;
      begin += 2;
      valueEnd = findEndOfINumber(begin, prog_.end(), isBinaryDigit, *diagnostics_);
      if (valueEnd == begin) {
         *diagnostics_ << "invalid binary number" << std::endl;
         valid = false;
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
            *diagnostics_ << "invalid octal number" << std::endl;
            valid = false;
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

   // for hex float, strtod expects the '0x' prefix to be included
   if (isFloat && base == 16) {
      valueRepr = "0x" + valueRepr;
   }

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

   SrcLoc loc{progBegin_, begin, suffixEnd};
   if (isFloat) {
      double value = valid ? std::strtod(valueRepr.c_str(), &pos) : 0.0;
      if (floatSuffix) {
         token_ = Tokenizer::Token{loc, safe_cast<float>(value)};
      } else {
         token_ = Tokenizer::Token{loc, value};
      }
   } else if (unsignedSuffix) {
      unsigned long long value = valid ? std::strtoull(valueRepr.c_str(), &pos, base) : 0;
      if (longLongSuffix) {
         token_ = Tokenizer::Token{loc, value};
      } else if (longSuffix) {
         token_ = Tokenizer::Token{loc, safe_cast<unsigned long>(value)};
      } else {
         token_ = Tokenizer::Token{loc, safe_cast<unsigned>(value)};
      }
   } else {
      long long value = valid ? static_cast<long long>(std::strtoull(valueRepr.c_str(), &pos, base)) : 0;
      if (longLongSuffix) {
         token_ = Tokenizer::Token{loc, safe_cast<long long>(value)};
      } else if (longSuffix) {
         token_ = Tokenizer::Token{loc, safe_cast<long>(value)};
      } else {
         token_ = Tokenizer::Token{loc, safe_cast<int>(value)};
      }
   }

   if (errno == ERANGE) {
      *diagnostics_ << "range error" << std::endl;
   } else if (pos != valueRepr.c_str() + valueRepr.length()) {
      *diagnostics_ << "invalid number" << std::endl;
   }

   return suffixEnd;
}
// ---------------------------------------------------------------------------
sv_it Tokenizer::const_iterator::getPunctuator(sv_it begin) {
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
         *diagnostics_ << SrcLoc(progBegin_, prog_.begin(), prog_.begin()) << "unknown punctuator '" << chars[0] << '\'' << std::endl;
   }

   SrcLoc loc{progBegin_, begin, begin + len};
   token_ = Token{loc, type};
   return begin + len;
}
// ---------------------------------------------------------------------------
sv_it Tokenizer::const_iterator::getSCharSequence(sv_it begin) {
   auto [literal, sSeqEnd] = getCharSequence<'"'>(progBegin_, begin, prog_.end(), *diagnostics_);
   token_ = Token{SrcLoc(progBegin_, begin, sSeqEnd), std::move(literal), TK::SLITERAL};
   return sSeqEnd;
}
// ---------------------------------------------------------------------------
sv_it Tokenizer::const_iterator::getCCharSequence(sv_it begin) {
   auto [literal, cSeqEnd] = getCharSequence<'\''>(progBegin_, begin, prog_.end(), *diagnostics_);
   token_ = Token{SrcLoc(progBegin_, begin, cSeqEnd), std::move(literal), TK::CLITERAL}; // todo: check if move
   return cSeqEnd;
}
// ---------------------------------------------------------------------------
Tokenizer::const_iterator& Tokenizer::const_iterator::operator++() {
   prevLoc_ = token_.getLoc();
   if (prog_.empty()) {
      token_ = Token(TK::END);
      return *this;
   }

   // find begin of next word
   decltype(prog_)::const_iterator begin = prog_.begin();
find_token_start:
   begin = std::find_if(begin, prog_.end(), [](char c) { return isPunctuatorStart(c) or isIdentStart(c) or isDigit(c) or c == '"' or c == '\'' or c == '\n'; });
   if (begin != prog_.end() && *begin == '\n') {
      diagnostics_->registerLineBreak(std::distance(progBegin_, begin));
      if (pp) {
         token_ = Token(TK::PP_END);
         pp = 0;
         return *this;
      }
      ++begin;
      if (begin == prog_.end()) {
         token_ = Token(TK::END);
         return *this;
      } else if (*begin == '#') {
         token_ = Token(TK::PP_START);
         pp = 1;
         prog_ = prog_.substr(std::distance(prog_.begin(), begin) + 1);
         return *this;
      }
      goto find_token_start;
   }

   if (begin == prog_.end()) {
      token_ = Token(TK::END);
      return *this;
   }
   prog_ = prog_.substr(std::distance(prog_.begin(), begin));

   // todo: implement custom class with src code with a peek() method
   char second = (begin + 1 != prog_.end()) ? *(begin + 1) : 0;
   bool requiresSeparator = false;
   sv_it end;

   if (isPunctuatorStart(*begin) && !(*begin == '.' and isDigit(second))) {
      // todo: this should be preprocessor?
      if (*begin == '/' && second == '/') {
         end = std::find_if(begin + 2, prog_.end(), [](char c) { return c == '\n'; });
         diagnostics_->registerLineBreak(std::distance(progBegin_, end));
         prog_ = prog_.substr(std::distance(prog_.begin(), end) + 1);
         begin = prog_.begin();
         goto find_token_start;
      } else if (*begin == '/' && second == '*') {
         const char* commentEnd = "*/";
         end = std::search(begin + 1, prog_.end(), commentEnd, commentEnd + 2);
         if (end == prog_.end()) {
            *diagnostics_ << SrcLoc(progBegin_, begin, end) << "unterminated comment" << std::endl;
         }
         prog_ = prog_.substr(std::distance(prog_.begin(), end + 2));
         begin = prog_.begin();
         goto find_token_start;
      } else {
         end = getPunctuator(begin);
      }
      end = getPunctuator(begin);
   } else if (isIdentStart(*begin)) {
      requiresSeparator = true;
      // todo: (jr) u8, u, U, L prefix not supported
      end = std::find_if_not(begin, prog_.end(), isIdentCont);
      std::string_view ident{begin, end};
      const token::GPerfToken* t = token::ReservedKeywordHash::isInWordSet(ident.data(), ident.size());
      // todo: (jr) handle constants
      SrcLoc loc{progBegin_, begin, end};
      if (t) {
         token_ = Token{loc, *t};
      } else {
         token_ = Token{loc, Ident(ident)};
      }
   } else if (isDigit(*begin) or (*begin == '.' and isDigit(second))) {
      requiresSeparator = true;
      end = getNumberConst(begin);
   } else if (*begin == '"') {
      end = getSCharSequence(begin);
   } else if (*begin == '\'') {
      end = getCCharSequence(begin);
   } else {
      *diagnostics_ << "unknown token: '" << *begin << '\'' << std::endl;
   }

   prog_ = prog_.substr(std::distance(prog_.begin(), end));

   if (requiresSeparator && !prog_.empty()) {
      auto it = prog_.begin();
      while (it != prog_.end() && !isSeparator(*it) && !isPunctuatorStart(*it) && *it != ']') {
         ++it;
      }
      if (it != prog_.begin()) {
         if (token_.getKind() == TK::DCONST || token_.getKind() == TK::FCONST || token_.getKind() == TK::LDCONST) {
            *diagnostics_ << SrcLoc(progBegin_, prog_.begin(), prog_.begin()) << "invalid suffix '" << std::string_view(prog_.begin(), it) << "' on floating constant" << std::endl;
         } else {
            *diagnostics_ << SrcLoc(progBegin_, prog_.begin(), it) << "token not fully consumed" << std::endl;
         }
         prog_ = prog_.substr(std::distance(prog_.begin(), it));
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