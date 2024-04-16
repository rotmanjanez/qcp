// todo: (jr) add edge cases
// todo: (jr) add unicode tests
// todo: (jr) add ansii escape tests
// ---------------------------------------------------------------------------
#include "csmith.h"
#include "diagnostics.h"
#include "gtest/gtest.h"
#include "token.h"
#include "tokenizer.h"
// ---------------------------------------------------------------------------
#include <array>
#include <unordered_map>
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using tk = qcp::token::Kind;
using Token = qcp::token::Token;
// ---------------------------------------------------------------------------
// todo: (jr) " \r ", " \r\n " "\r\n" "\r",
std::array whiteSpaces{"", " ", "   ", "\t", "\n", " \t ", " \n "};
// ---------------------------------------------------------------------------
std::pair<tk, std::string> punctuatorMap[] = {
    {tk::L_BRACKET, "["},
    {tk::R_BRACKET, "]"},
    {tk::L_BRACE, "("},
    {tk::R_BRACE, ")"},
    {tk::L_C_BRKT, "{"},
    {tk::R_C_BRKT, "}"},
    {tk::PERIOD, "."},
    {tk::DEREF, "->"},
    {tk::INC, "++"},
    {tk::DEC, "--"},
    {tk::BW_AND, "&"},
    {tk::ASTERISK, "*"},
    {tk::PLUS, "+"},
    {tk::MINUS, "-"},
    {tk::BW_INV, "~"},
    {tk::NEG, "!"},
    {tk::DIV, "/"},
    {tk::PERCENT, "%"},
    {tk::SHL, "<<"},
    {tk::SHR, ">>"},
    {tk::L_ANGLE, "<"},
    {tk::R_ANGLE, ">"},
    {tk::LE, "<="},
    {tk::GE, ">="},
    {tk::EQ, "=="},
    {tk::NE, "!="},
    {tk::BW_XOR, "^"},
    {tk::BW_OR, "|"},
    {tk::L_AND, "&&"},
    {tk::L_OR, "||"},
    {tk::QMARK, "?"},
    {tk::COLON, ":"},
    {tk::D_COLON, "::"},
    {tk::SEMICOLON, ";"},
    {tk::ELLIPSIS, "..."},
    {tk::ASSIGN, "="},
    {tk::MUL_ASSIGN, "*="},
    {tk::DIV_ASSIGN, "/="},
    {tk::REM_ASSIGN, "%="},
    {tk::ADD_ASSIGN, "+="},
    {tk::SUB_ASSIGN, "-="},
    {tk::SHL_ASSIGN, "<<="},
    {tk::SHR_ASSIGN, ">>="},
    {tk::BW_AND_ASSIGN, "&="},
    {tk::BW_XOR_ASSIGN, "^="},
    {tk::BW_OR_ASSIGN, "|="},
    {tk::COMMA, ","},
};
// ---------------------------------------------------------------------------
std::pair<tk, std::string> keywordMap[] = {
    {tk::AUTO, "auto"},
    {tk::BREAK, "break"},
    {tk::CASE, "case"},
    {tk::CHAR, "char"},
    {tk::CONST, "const"},
    {tk::CONSTEXPR, "constexpr"},
    {tk::CONTINUE, "continue"},
    {tk::DEFAULT, "default"},
    {tk::DO, "do"},
    {tk::DOUBLE, "double"},
    {tk::ELSE, "else"},
    {tk::ENUM, "enum"},
    {tk::EXTERN, "extern"},
    {tk::FALSE, "false"},
    {tk::FLOAT, "float"},
    {tk::FOR, "for"},
    {tk::GOTO, "goto"},
    {tk::IF, "if"},
    {tk::INLINE, "inline"},
    {tk::INT, "int"},
    {tk::LONG, "long"},
    {tk::NULLPTR, "nullptr"},
    {tk::REGISTER, "register"},
    {tk::RESTRICT, "restrict"},
    {tk::RETURN, "return"},
    {tk::SHORT, "short"},
    {tk::SIGNED, "signed"},
    {tk::SIZEOF, "sizeof"},
    {tk::STATIC, "static"},
    {tk::STRUCT, "struct"},
    {tk::SWITCH, "switch"},
    {tk::TRUE, "true"},
    {tk::TYPEDEF, "typedef"},
    {tk::TYPEOF, "typeof"},
    {tk::TYPEOF_UNQUAL, "typeof_unqual"},
    {tk::UNION, "union"},
    {tk::UNSIGNED, "unsigned"},
    {tk::VOID, "void"},
    {tk::VOLATILE, "volatile"},
    {tk::WHILE, "while"},
    {tk::ATOMIC, "_Atomic"},
    {tk::BITINT, "_BitInt"},
    {tk::COMPLEX, "_Complex"},
    {tk::DECIMAL128, "_Decimal128"},
    {tk::DECIMAL32, "_Decimal32"},
    {tk::DECIMAL64, "_Decimal64"},
    {tk::GENERIC, "_Generic"},
    {tk::IMAGINARY, "_Imaginary"},
    {tk::NORETURN, "_Noreturn"},

    // alternative spellig
    {tk::THREAD_LOCAL, "_Thread_local"},
    {tk::THREAD_LOCAL, "thread_local"},

    {tk::STATIC_ASSERT, "_Static_assert"},
    {tk::STATIC_ASSERT, "static_assert"},

    {tk::ALIGNAS, "_Alignas"},
    {tk::ALIGNAS, "alignas"},

    {tk::ALIGNOF, "_Alignof"},
    {tk::ALIGNOF, "alignof"},

    {tk::BOOL, "bool"},
    {tk::BOOL, "_Bool"},
};
// ---------------------------------------------------------------------------
std::array<std::string, 27> identifierList{
    "a",
    "z",
    "A",
    "Z",
    "_",
    "aa",
    "zz",
    "AA",
    "ZZ",
    "__",
    "a0",
    "z9",
    "A0",
    "Z9",
    "_0",
    "_9",
    "aA",
    "zZ",
    "Aa",
    "Zz",
    "_A",
    "_Z",
    "a_",
    "z_",
    "A_",
    "Z_",
    "a0a0",
};
// ---------------------------------------------------------------------------
std::pair<std::string, std::size_t> iconstList[] = {
    {"0", 0},
    {"1", 1},
    {"9", 9},
    {"00", 0},
    {"01", 1},
    {"07", 7},
    {"010", 8},
    {"10", 10},
    {"11", 11},
    {"123", 123},
    {"075", 61},
    {"0xf", 15},
    {"0XF", 15},
    {"0XaF", 175},
    {"0x1A3F", 0x1A3F},
    {"0b1", 1},
    {"0B1", 1},
    {"0b101010", 0b101010},
    {"4294967295", 4294967295},
    {"01234567", 01234567},
    {"0xFFFFFFFF", 0xFFFFFFFF},
    {"0b111111111111111111111111111111", 0b111111111111111111111111111111},
    {"0XDeadBEEF", 0XDeadBEEF},
    {"0b1101", 0b1101},
    {"1'234", 1234},
    {"0'123", 83}, // Assuming octal based on leading 0
    {"0x1'2A3F", 0x12A3F},
    {"0XDE'ADBE'EF", 0xDEADBEEF},
    {"0b10'1010", 0b101010},
    {"4'294'967'295", 4294967295}, // Maximum unsigned 32-bit integer
    {"2'147'483'647", 2147483647}, // Maximum signed 32-bit integer
};
// ---------------------------------------------------------------------------
std::string invalidIconstList[] = {
    "0x",
    "0b",
    "0xG",
    "0b2",
    "0b12",
    "1'a",
    "0xZ123",
    "0b102",
    "08",
    "123'",
    "123'456'789'012'345'678'901'234",
    "0x",
    "0b",
    "0xGHIJ",
};
// ---------------------------------------------------------------------------
std::array<std::string, 20> stringLiteralList{
    "\"Simple string\"",
    "\"\"", // Empty string
    "\"single quote (')\"",
    "\"double quote (\\\")\"",
    "\"backslash (\\\\)\"",
    "\"tab (\\t)\"",
    "\"newline (\\n)\"",
    "\"carriage return (\\r)\"",
    "\"escape sequence (\\x1B[31m) for red color\"",
    {"\"embedded (\\0) null in text\"", 28},
    // "\"unicodes (\\u03A9 \\u03C0)\"",
    "\"hex escape (\\x48\\x65\\x6C\\x6C\\x6F)\"",
    "\"octal escape (\\1\\12\\110\\145\\154\\154\\157)\"",
    "\"escape sequence (\\n\\t\\\'\\\"\\\\)\"",
    "\"\\\"starts and ends with quote\\\"\"",
    "\"\\\'String starts and ends with a single quote but is not char sequence\\\'\"",
    "\"a /* block comment */ inside\"",
    "\"a // line comment inside\"",
    "\"`backticks`\"",
    "\"special characters $!@#%^&*()-+=[]{}|;:,.<>?/\"",
    "\"octal escape \\7 \\42 \\123 \"",
    // "\"non-standard escape \\e for ASCII escape \"",
};
// ---------------------------------------------------------------------------
class TokenMap : public ::testing::TestWithParam<std::pair<tk, std::string>> {
};
// ---------------------------------------------------------------------------
class IdentTK : public ::testing::TestWithParam<std::string> {
};
// ---------------------------------------------------------------------------
class IConstTK : public ::testing::TestWithParam<std::pair<std::string, std::size_t>> {
};
// ---------------------------------------------------------------------------
class InvalidIConstTK : public ::testing::TestWithParam<std::string> {
};
// ---------------------------------------------------------------------------
class StringLiteralTK : public ::testing::TestWithParam<std::string> {
};
// ---------------------------------------------------------------------------
TEST_P(TokenMap, GetsRecognized) {
   auto [token, word] = GetParam();
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix + word + suffix};
         qcp::DiagnosticTracker diagnostics{src};
         qcp::Tokenizer ts{src, diagnostics};
         auto it = ts.begin();
         ASSERT_EQ(it->getType(), token) << "Input string: " << std::quoted(src);
         ASSERT_EQ(++it, ts.end()) << "Input string: " << std::quoted(src);
      }
   }
}
// ---------------------------------------------------------------------------
TEST_P(IdentTK, GetsRecognized) {
   auto word = GetParam();
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix};
         src += word;
         src += suffix;

         qcp::DiagnosticTracker diagnostics{src};
         qcp::Tokenizer ts{src, diagnostics};
         auto it = ts.begin();
         ASSERT_EQ(it->getType(), tk::IDENT) << "Input string: " << std::quoted(src);
         ASSERT_EQ(++it, ts.end()) << "Input string: " << std::quoted(src);
      }
   }
}
// ---------------------------------------------------------------------------
template <typename T>
void test_iconst(const std::string& word, const std::vector<std::string>& suffixes, tk type, T value);
// ---------------------------------------------------------------------------
template <typename T>
void test_iconst(const std::string& word, const std::vector<std::string>& suffixes, tk type, T value) {
   for (auto& suffix : suffixes) {
      for (auto& wsPrefix : whiteSpaces) {
         for (auto& wsSuffix : whiteSpaces) {
            std::string src{wsPrefix};

            src += word;
            src += suffix;
            src += wsSuffix;

            qcp::DiagnosticTracker diagnostics{src};
            qcp::Tokenizer ts{src, diagnostics};
            auto it = ts.begin();
            ASSERT_EQ(it->getType(), type) << "Input string: " << std::quoted(src);
            ASSERT_EQ(it->getValue<T>(), value) << "Input string: " << std::quoted(src);
            ASSERT_EQ(++it, ts.end()) << "Input string: " << std::quoted(src);
         }
      }
   }
}
// ---------------------------------------------------------------------------
TEST_P(IConstTK, GetsRecognized) {
   auto [word, value] = GetParam();
   std::vector<std::string> uSuffix = {"u", "U"};
   std::vector<std::string> lSuffix = {"l", "L"};
   std::vector<std::string> ulSuffix = {"ul", "uL", "Ul", "UL", "lu", "lU", "Lu", "LU"};
   std::vector<std::string> llSuffix = {"ll", "LL"};
   std::vector<std::string> ullSuffix = {"ull", "uLL", "Ull", "ULL", "llu", "llU", "LLu", "LLU"};
   std::vector<std::string> wbSuffix = {"wb", "WB"};
   std::vector<std::string> uwbSuffix = {"wbu", "WBU", "wbU", "WBu", "uwb", "uWB", "UWB", "Uwb"};

   test_iconst<int>(word, {}, tk::ICONST, value);
   test_iconst<unsigned>(word, uSuffix, tk::U_ICONST, value);
   test_iconst<long>(word, lSuffix, tk::L_ICONST, value);
   test_iconst<unsigned long>(word, ulSuffix, tk::UL_ICONST, value);
   test_iconst<long long>(word, llSuffix, tk::LL_ICONST, value);
   test_iconst<unsigned long long>(word, ullSuffix, tk::ULL_ICONST, value);
   // todo: (jr) wb, uwb
}
// ---------------------------------------------------------------------------
TEST_P(StringLiteralTK, GetsRecognized) {
   auto word = GetParam();
   std::array whiteSpaces{"", " ", "   ", "\t", "\n", "\r", "\r\n", " \t ", " \n ", " \r ", " \r\n "};
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix + word + suffix};

         qcp::DiagnosticTracker diagnostics{src};
         qcp::Tokenizer ts{src, diagnostics};
         auto it = ts.begin();
         ASSERT_EQ(it->getType(), tk::SLITERAL);
         ASSERT_EQ(it->getValue<std::string_view>(), word.substr(1, word.size() - 2));
         ASSERT_EQ(++it, ts.end());
      }
   }
}
// ---------------------------------------------------------------------------
TEST_P(InvalidIConstTK, invalidIConstNotParsed) {
   auto word = GetParam();
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix};
         src += word;
         src += suffix;
         qcp::DiagnosticTracker diagnostics{src};
         qcp::Tokenizer<qcp::DiagnosticTracker> ts{src, diagnostics};
         for (auto token : ts) {
            (void) token;
         }
         ASSERT_FALSE(diagnostics.empty()) << "Input string: " << std::quoted(src);
      }
   }
}
// ---------------------------------------------------------------------------
INSTANTIATE_TEST_CASE_P(Punctuator, TokenMap, ::testing::ValuesIn(punctuatorMap));
INSTANTIATE_TEST_CASE_P(Keyword, TokenMap, ::testing::ValuesIn(keywordMap));
INSTANTIATE_TEST_CASE_P(Identifier, IdentTK, ::testing::ValuesIn(identifierList));
INSTANTIATE_TEST_CASE_P(IntegerConstant, IConstTK, ::testing::ValuesIn(iconstList));
INSTANTIATE_TEST_CASE_P(InvalidIntegerConstant, InvalidIConstTK, ::testing::ValuesIn(invalidIconstList));
INSTANTIATE_TEST_CASE_P(StringLiteral, StringLiteralTK, ::testing::ValuesIn(stringLiteralList));
// ---------------------------------------------------------------------------
// TEST(Tokenizer, Random) {
//    std::string src{qcp::tool::random_c_program("gcc", 0)};
//    qcp::Tokenizer ts{src};
//    std::ofstream ofs{"out.log"};
//    std::cout << "Random test started" << std::endl;
//    for (auto token : ts) {
//       ASSERT_TRUE(token.getType() != tk::UNKNOWN);
//       ofs << token << ' ';
//       if (token.getType() == tk::SEMICOLON) {
//          ofs << std::endl;
//       }
//    }
//    std::cout << "Random test passed" << std::endl;
// }
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
