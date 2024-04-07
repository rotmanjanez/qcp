// todo: (jr) add edge cases
// todo: (jr) add unicode tests
// todo: (jr) add ansii escape tests
// ---------------------------------------------------------------------------
#include "csmith.h"
#include "gtest/gtest.h"
#include "token.h"
#include "tokenizer.h"
// ---------------------------------------------------------------------------
#include <array>
#include <unordered_map>
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using tt = qcp::TokenType;
using Token = qcp::Token;
// ---------------------------------------------------------------------------
// todo: (jr) " \r ", " \r\n " "\r\n" "\r",
std::array whiteSpaces{"", " ", "   ", "\t", "\n", " \t ", " \n "};
// ---------------------------------------------------------------------------
std::pair<tt, std::string> punctuatorMap[] = {
    {tt::L_SQ_BRKT, "["},
    {tt::R_SQ_BRKT, "]"},
    {tt::L_BRKT, "("},
    {tt::R_BRKT, ")"},
    {tt::L_C_BRKT, "{"},
    {tt::R_C_BRKT, "}"},
    {tt::PERIOD, "."},
    {tt::DEREF, "->"},
    {tt::INC, "++"},
    {tt::DEC, "--"},
    {tt::BW_AND, "&"},
    {tt::MUL, "*"},
    {tt::PLUS, "+"},
    {tt::MINUS, "-"},
    {tt::BW_INV, "~"},
    {tt::NEG, "!"},
    {tt::DIV, "/"},
    {tt::MOD, "%"},
    {tt::SHL, "<<"},
    {tt::SHR, ">>"},
    {tt::L_P_BRKT, "<"},
    {tt::R_P_BRKT, ">"},
    {tt::LE, "<="},
    {tt::GE, ">="},
    {tt::EQ, "=="},
    {tt::NE, "!="},
    {tt::BW_XOR, "^"},
    {tt::BW_OR, "|"},
    {tt::L_AND, "&&"},
    {tt::L_OR, "||"},
    {tt::QMARK, "?"},
    {tt::COLON, ":"},
    {tt::D_COLON, "::"},
    {tt::SEMICOLON, ";"},
    {tt::ELLIPSIS, "..."},
    {tt::ASSIGN, "="},
    {tt::MUL_ASSIGN, "*="},
    {tt::DIV_ASSIGN, "/="},
    {tt::MOD_ASSIGN, "%="},
    {tt::ADD_ASSIGN, "+="},
    {tt::SUB_ASSIGN, "-="},
    {tt::SHL_ASSIGN, "<<="},
    {tt::SHR_ASSIGN, ">>="},
    {tt::BW_AND_ASSIGN, "&="},
    {tt::BW_XOR_ASSIGN, "^="},
    {tt::BW_OR_ASSIGN, "|="},
    {tt::COMMA, ","},
};
// ---------------------------------------------------------------------------
std::pair<tt, std::string> keywordMap[] = {
    {tt::AUTO, "auto"},
    {tt::BREAK, "break"},
    {tt::CASE, "case"},
    {tt::CHAR, "char"},
    {tt::CONST, "const"},
    {tt::CONSTEXPR, "constexpr"},
    {tt::CONTINUE, "continue"},
    {tt::DEFAULT, "default"},
    {tt::DO, "do"},
    {tt::DOUBLE, "double"},
    {tt::ELSE, "else"},
    {tt::ENUM, "enum"},
    {tt::EXTERN, "extern"},
    {tt::FALSE, "false"},
    {tt::FLOAT, "float"},
    {tt::FOR, "for"},
    {tt::GOTO, "goto"},
    {tt::IF, "if"},
    {tt::INLINE, "inline"},
    {tt::INT, "int"},
    {tt::LONG, "long"},
    {tt::NULLPTR, "nullptr"},
    {tt::REGISTER, "register"},
    {tt::RESTRICT, "restrict"},
    {tt::RETURN, "return"},
    {tt::SHORT, "short"},
    {tt::SIGNED, "signed"},
    {tt::SIZEOF, "sizeof"},
    {tt::STATIC, "static"},
    {tt::STRUCT, "struct"},
    {tt::SWITCH, "switch"},
    {tt::TRUE, "true"},
    {tt::TYPEDEF, "typedef"},
    {tt::TYPEOF, "typeof"},
    {tt::TYPEOF_UNQUAL, "typeof_unqual"},
    {tt::UNION, "union"},
    {tt::UNSIGNED, "unsigned"},
    {tt::VOID, "void"},
    {tt::VOLATILE, "volatile"},
    {tt::WHILE, "while"},
    {tt::ATOMIC, "_Atomic"},
    {tt::BITINT, "_BitInt"},
    {tt::COMPLEX, "_Complex"},
    {tt::DECIMAL128, "_Decimal128"},
    {tt::DECIMAL32, "_Decimal32"},
    {tt::DECIMAL64, "_Decimal64"},
    {tt::GENERIC, "_Generic"},
    {tt::IMAGINARY, "_Imaginary"},
    {tt::NORETURN, "_Noreturn"},

    // alternative spellig
    {tt::THREAD_LOCAL, "_Thread_local"},
    {tt::THREAD_LOCAL, "thread_local"},

    {tt::STATIC_ASSERT, "_Static_assert"},
    {tt::STATIC_ASSERT, "static_assert"},

    {tt::ALIGNAS, "_Alignas"},
    {tt::ALIGNAS, "alignas"},

    {tt::ALIGNOF, "_Alignof"},
    {tt::ALIGNOF, "alignof"},

    {tt::BOOL, "bool"},
    {tt::BOOL, "_Bool"},
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
std::array<std::string, 19> stringLiteralList{
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
    // "\"non-standard escape \\e for ASCII escape \"",
};
// ---------------------------------------------------------------------------
class TT : public ::testing::TestWithParam<std::pair<tt, std::string>> {
};
// ---------------------------------------------------------------------------
class IdentTT : public ::testing::TestWithParam<std::string> {
};
// ---------------------------------------------------------------------------
class IConstTT : public ::testing::TestWithParam<std::pair<std::string, std::size_t>> {
};
// ---------------------------------------------------------------------------
class InvalidIConstTT : public ::testing::TestWithParam<std::string> {
};
// ---------------------------------------------------------------------------
class StringLiteralTT : public ::testing::TestWithParam<std::string> {
};
// ---------------------------------------------------------------------------
TEST_P(TT, GetsRecognized) {
   auto [token, word] = GetParam();
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix + word + suffix};
         qcp::Tokenizer ts{src};
         auto it = ts.begin();
         ASSERT_EQ(it->getType(), token) << "Input string: " << std::quoted(src);
         ASSERT_EQ(++it, ts.end()) << "Input string: " << std::quoted(src);
      }
   }
}
// ---------------------------------------------------------------------------
TEST_P(IdentTT, GetsRecognized) {
   auto word = GetParam();
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix};
         src += word;
         src += suffix;
         qcp::Tokenizer ts{src};
         auto it = ts.begin();
         ASSERT_EQ(it->getType(), tt::IDENT) << "Input string: " << std::quoted(src);
         ASSERT_EQ(++it, ts.end()) << "Input string: " << std::quoted(src);
      }
   }
}
// ---------------------------------------------------------------------------
template <typename T>
void test_iconst(const std::string& word, const std::vector<std::string>& suffixes, tt type, T value);
// ---------------------------------------------------------------------------
template <typename T>
void test_iconst(const std::string& word, const std::vector<std::string>& suffixes, tt type, T value) {
   for (auto& suffix : suffixes) {
      for (auto& wsPrefix : whiteSpaces) {
         for (auto& wsSuffix : whiteSpaces) {
            std::string src{wsPrefix};

            src += word;
            src += suffix;
            src += wsSuffix;

            qcp::Tokenizer ts{src};
            auto it = ts.begin();
            ASSERT_EQ(it->getType(), type) << "Input string: " << std::quoted(src);
            ASSERT_EQ(it->getValue<T>(), value) << "Input string: " << std::quoted(src);
            ASSERT_EQ(++it, ts.end()) << "Input string: " << std::quoted(src);
         }
      }
   }
}
// ---------------------------------------------------------------------------
TEST_P(IConstTT, GetsRecognized) {
   auto [word, value] = GetParam();
   std::vector<std::string> uSuffix = {"u", "U"};
   std::vector<std::string> lSuffix = {"l", "L"};
   std::vector<std::string> ulSuffix = {"ul", "uL", "Ul", "UL", "lu", "lU", "Lu", "LU"};
   std::vector<std::string> llSuffix = {"ll", "LL"};
   std::vector<std::string> ullSuffix = {"ull", "uLL", "Ull", "ULL", "llu", "llU", "LLu", "LLU"};
   std::vector<std::string> wbSuffix = {"wb", "WB"};
   std::vector<std::string> uwbSuffix = {"wbu", "WBU", "wbU", "WBu", "uwb", "uWB", "UWB", "Uwb"};

   test_iconst<int>(word, {}, tt::ICONST, value);
   test_iconst<unsigned>(word, uSuffix, tt::U_ICONST, value);
   test_iconst<long>(word, lSuffix, tt::L_ICONST, value);
   test_iconst<unsigned long>(word, ulSuffix, tt::UL_ICONST, value);
   test_iconst<long long>(word, llSuffix, tt::LL_ICONST, value);
   test_iconst<unsigned long long>(word, ullSuffix, tt::ULL_ICONST, value);
   // todo: (jr) wb, uwb
}
// ---------------------------------------------------------------------------
TEST_P(StringLiteralTT, GetsRecognized) {
   auto word = GetParam();
   std::array whiteSpaces{"", " ", "   ", "\t", "\n", "\r", "\r\n", " \t ", " \n ", " \r ", " \r\n "};
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix + word + suffix};
         qcp::Tokenizer ts{src};
         auto it = ts.begin();
         ASSERT_EQ(it->getType(), tt::LITERAL);
         ASSERT_EQ(it->getValue<std::string_view>(), word.substr(1, word.size() - 2));
         ASSERT_EQ(++it, ts.end());
      }
   }
}
// ---------------------------------------------------------------------------
TEST_P(InvalidIConstTT, invalidIConstNotParsed) {
   auto word = GetParam();
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix};
         src += word;
         src += suffix;
         ASSERT_EXIT({
            qcp::Tokenizer ts{src};
            (void) ts.begin();
         },
                     testing::KilledBySignal(6), ".*");
      }
   }
}
// ---------------------------------------------------------------------------
INSTANTIATE_TEST_CASE_P(Punctuator, TT, ::testing::ValuesIn(punctuatorMap));
INSTANTIATE_TEST_CASE_P(Keyword, TT, ::testing::ValuesIn(keywordMap));
INSTANTIATE_TEST_CASE_P(Identifier, IdentTT, ::testing::ValuesIn(identifierList));
INSTANTIATE_TEST_CASE_P(IntegerConstant, IConstTT, ::testing::ValuesIn(iconstList));
INSTANTIATE_TEST_CASE_P(InvalidIntegerConstant, InvalidIConstTT, ::testing::ValuesIn(invalidIconstList));
INSTANTIATE_TEST_CASE_P(StringLiteral, StringLiteralTT, ::testing::ValuesIn(stringLiteralList));
// ---------------------------------------------------------------------------
TEST(Tokenizer, Random) {
   std::string src{qcp::tool::random_c_program("gcc", 0)};
   qcp::Tokenizer ts{src};
   std::ofstream ofs{"out.log"};
   std::cout << "Random test started" << std::endl;
   for (auto token : ts) {
      ASSERT_TRUE(token.getType() != tt::UNKNOWN);
      ofs << token << ' ';
      if (token.getType() == tt::SEMICOLON) {
         ofs << std::endl;
      }
   }
   std::cout << "Random test passed" << std::endl;
}
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
