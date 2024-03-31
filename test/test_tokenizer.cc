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
// todo: (jr) add edge cases
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
};
// ---------------------------------------------------------------------------
/*template <std::size_t len>
std::pair<std::string, std::array<std::pair<qcp::Token, std::function<void(qcp::Token)>>, len>> randomTokenSequence() {
   std::stringstream src{};
   std::array<std::pair<qcp::Token, std::function<void(qcp::Token)>>, len> tokens;

   for (auto& token : tokens) {
      qcp::TokenType type = static_cast<qcp::TokenType>(rand() % static_cast<int>(qcp::TokenType::END));
      int value = rand();
      if (value < 0) {
         value *= -1;
      }
      switch (type) {
         case qcp::TokenType::ICONST:
            src << value;
            token = qcp::Token(value);
            break;
         case qcp::TokenType::U_ICONST:
            src << value << "u";
            token = qcp::Token(static_cast<unsigned>(value));
            break;
         case qcp::TokenType::L_ICONST:
            src << value << "l";
            token = qcp::Token(static_cast<unsigned>(value));
            break;
         case qcp::TokenType::UL_ICONST:
            src << value << "lu";
            token = qcp::Token(static_cast<unsigned long>(value));
            break;
         case qcp::TokenType::LL_ICONST:
            src << value << "ll";
            token = qcp::Token(static_cast<long long>(value));
            break;
         case qcp::TokenType::ULL_ICONST:
            src << value << "llu";
            token = qcp::Token(static_cast<unsigned long long>(value));
            break;
         case qcp::TokenType::IDENT:
            src << qcp::Token{identifierList[rand() % identifierList.size()]};
            break;
         default:
            break;
      }
   }
   return result;
}*/
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
TEST_P(TT, GetsRecognized) {
   auto [token, word] = GetParam();
   std::array whiteSpaces{"", " ", "   ", "\t", "\n", "\r", "\r\n", " \t ", " \n ", " \r ", " \r\n "};
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix + word + suffix};
         qcp::Tokenizer ts{src};
         auto it = ts.begin();
         ASSERT_EQ(it->getType(), token);
         ASSERT_EQ(++it, ts.end());
      }
   }
}
// ---------------------------------------------------------------------------
TEST_P(IdentTT, GetsRecognized) {
   auto word = GetParam();
   std::array whiteSpaces{"", " ", "   ", "\t", "\n", "\r", "\r\n", " \t ", " \n ", " \r ", " \r\n "};
   for (auto& prefix : whiteSpaces) {
      for (auto& suffix : whiteSpaces) {
         std::string src{prefix + word + suffix};
         qcp::Tokenizer ts{src};
         auto it = ts.begin();
         ASSERT_EQ(it->getType(), tt::IDENT);
         ASSERT_EQ(++it, ts.end());
      }
   }
}
// ---------------------------------------------------------------------------
template <typename T>
void test_iconst(const std::string& word, const std::vector<std::string>& suffixes, tt type, T value) {
   std::array whiteSpaces{"", " ", "   ", "\t", "\n", "\r", "\r\n", " \t ", " \n ", " \r ", " \r\n "};
   for (auto& suffix : suffixes) {
      for (auto& wsPrefix : whiteSpaces) {
         for (auto& wsSuffix : whiteSpaces) {
            std::string src{wsPrefix + word + suffix + wsSuffix};
            qcp::Tokenizer ts{src};
            auto it = ts.begin();
            if (it->getType() != type) {
               std::cerr << std::quoted(wsPrefix + word + suffix + wsSuffix) << std::endl;
            }
            ASSERT_EQ(it->getType(), type);
            ASSERT_EQ(it->getValue<T>(), value);
            ASSERT_EQ(++it, ts.end());
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
INSTANTIATE_TEST_CASE_P(Punctuator, TT, ::testing::ValuesIn(punctuatorMap));
INSTANTIATE_TEST_CASE_P(Keyword, TT, ::testing::ValuesIn(keywordMap));
INSTANTIATE_TEST_CASE_P(Identifier, IdentTT, ::testing::ValuesIn(identifierList));
INSTANTIATE_TEST_CASE_P(IntegerConstant, IConstTT, ::testing::ValuesIn(iconstList));
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
