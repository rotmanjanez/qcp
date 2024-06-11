// ---------------------------------------------------------------------------
#include "diagnostics.h"
#include "gtest/gtest.h"
#include "parser.h"
#include "type.h"
// ---------------------------------------------------------------------------
#include <algorithm>
#include <array>
#include <fstream>
#include <utility>
#include <vector>
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using TK = qcp::token::Kind;
using Token = qcp::token::Token;
using TY = qcp::type::Type<std::string>;
using TYK = qcp::type::Kind;
using Parser = qcp::Parser<qcp::DiagnosticTracker>;
using TypeFactory = qcp::type::TypeFactory<std::string>;
// ---------------------------------------------------------------------------
bool SINGED = false;
bool UNSIGNED = true;
TY intTy{TYK::INT, SINGED};
TY unsignedIntTy{TYK::INT, UNSIGNED};
TY longTy{TYK::LONG, SINGED};
TY unsignedLongTy{TYK::LONG, UNSIGNED};
TY longLongTy{TYK::LONGLONG, SINGED};
TY unsignedLongLongTy{TYK::LONGLONG, UNSIGNED};
TY voidTy{TYK::VOID};
std::ofstream devNull{"/dev/null"};
// ---------------------------------------------------------------------------
std::pair<std::vector<std::string>, TY> validSpecifierQualifierList[] = {
    {{"int"}, intTy},
    {{"unsigned"}, unsignedIntTy},
    {{"signed"}, intTy},
    {{"unsigned", "int"}, unsignedIntTy},
    {{"long"}, longTy},
    {{"long", "int"}, longTy},
    {{"unsigned", "long"}, unsignedLongTy},
    {{"unsigned", "long", "int"}, unsignedLongTy},
    {{"long", "long"}, longLongTy},
    {{"long", "long", "int"}, longLongTy},
    {{"unsigned", "long", "long"}, unsignedLongLongTy},
    {{"unsigned", "long", "long", "int"}, unsignedLongLongTy},
    {{"float"}, TY{TYK::FLOAT, SINGED}},
    {{"double"}, TY{TYK::DOUBLE, SINGED}},
    {{"long", "double"}, TY{TYK::LONGDOUBLE, SINGED}},
    {{"char"}, TY{TYK::CHAR, SINGED}},
    {{"unsigned", "char"}, TY{TYK::CHAR, UNSIGNED}},
    {{"short"}, TY{TYK::SHORT, SINGED}},
    {{"unsigned", "short"}, TY{TYK::SHORT, UNSIGNED}},
    {{"bool"}, TY{TYK::BOOL, SINGED}},
    {{"void"}, TY{TYK::VOID}},
};
// ---------------------------------------------------------------------------
std::pair<std::string, TY> validTypeNames[] = {
    {"int", intTy},
    {"int *", TY::ptrTo(intTy)},
    {"int *[3]", TY::arrayOf(TY::ptrTo(intTy), 3)},
    {"int (*)[3]", TY::ptrTo(TY::arrayOf(intTy, 3))},
    {"int (*)[*]", TY::ptrTo(TY::arrayOf(intTy, 0, true, true))},
    {"int *()", TY::function(TY::ptrTo(intTy), {})},
    {"int (*)(void)", TY::ptrTo(TY::function(intTy, {voidTy}))},
    {"int (*const [])(unsigned int, ...)", TY::arrayOf(TY::qualified(TY::ptrTo(TY::function(intTy, {unsignedIntTy}, true)), TK::CONST))},
};
// ---------------------------------------------------------------------------
class ValidSpecifierQualifierList : public ::testing::TestWithParam<std::pair<std::vector<std::string>, TY>> {
};
// ---------------------------------------------------------------------------
class ValidTypeNames : public ::testing::TestWithParam<std::pair<std::string, TY>> {
};
// ---------------------------------------------------------------------------
void testPermutations(std::vector<std::string> tyStr, TY expectedTy) {
   do {
      std::string tyStrJoined;
      for (const auto& s : tyStr) {
         tyStrJoined += s + " ";
      }

      qcp::DiagnosticTracker diag{tyStrJoined};
      Parser parser{tyStrJoined, diag, devNull};
      TY parsedTy = parser.parseSpecifierQualifierList();
      EXPECT_EQ(parsedTy, expectedTy) << "for " << tyStrJoined;
      EXPECT_TRUE(diag.empty());

   } while (std::next_permutation(tyStr.begin(), tyStr.end()));
}
// ---------------------------------------------------------------------------
TEST_P(ValidSpecifierQualifierList, parse) {
   auto [tyStr, expectedTy] = GetParam();
   testPermutations(tyStr, expectedTy);
   auto constTyStr = tyStr;
   constTyStr.push_back("const");
   testPermutations(constTyStr, TY::qualified(expectedTy, TK::CONST));
   auto volatileTyStr = tyStr;
   volatileTyStr.push_back("volatile");
   testPermutations(volatileTyStr, TY::qualified(expectedTy, TK::VOLATILE));
   auto restrictTyStr = tyStr;
   restrictTyStr.push_back("restrict");
   testPermutations(restrictTyStr, TY::qualified(expectedTy, TK::RESTRICT));
}
// ---------------------------------------------------------------------------
TEST_P(ValidTypeNames, parse) {
   auto [tyStr, expectedTy] = GetParam();
   qcp::DiagnosticTracker diag{tyStr};
   Parser parser{tyStr, diag, devNull};
   TY parsedTy = parser.parseTypeName();
   EXPECT_EQ(parsedTy, expectedTy);
   EXPECT_TRUE(diag.empty());
}
// ---------------------------------------------------------------------------
INSTANTIATE_TEST_CASE_P(Types, ValidSpecifierQualifierList, ::testing::ValuesIn(validSpecifierQualifierList));
INSTANTIATE_TEST_CASE_P(Types, ValidTypeNames, ::testing::ValuesIn(validTypeNames));
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
