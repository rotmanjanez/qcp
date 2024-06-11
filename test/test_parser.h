#ifndef TEST_PARSER_H
#define TEST_PARSER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
#include "gtest/gtest.h"
#include "llvmemitter.h"
#include "parser.h"
#include "tokenizer.h"
// ---------------------------------------------------------------------------
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using Parser = qcp::Parser<qcp::emitter::LLVMEmitter>;
class IntParamTest : public testing::TestWithParam<int> {};
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
TEST_P(IntParamTest, LLVM_DIFF) {
   int i = GetParam();
   std::string input_filename = "test1/" + std::to_string(i) + ".c";

   // Check if the file exists
   if (!std::filesystem::exists(input_filename)) {
      throw std::runtime_error("Input file does not exist: " + input_filename);
   }

   // Read the C code from the input file
   std::ifstream input_file(input_filename);
   std::stringstream code;
   code << input_file.rdbuf();

   std::string parser_of = "/tmp/parser_llvm_ir_";
   parser_of += std::to_string(i);
   parser_of += ".ll";

   // Call your parser
   std::string input_code = code.str();
   std::stringstream log;
   qcp::DiagnosticTracker diag{input_code};
   Parser parser{input_code, diag, log};
   parser.parse();

   parser.getEmitter().dumpToFile(parser_of);

   // Generate LLVM IR using clang
   std::string clang_of = "/tmp/clang_temp_code_";
   clang_of += std::to_string(i);
   clang_of += ".ll";

   std::string clang_command = "clang -S -emit-llvm ";
   clang_command += input_filename;
   clang_command += " -o ";
   clang_command += clang_of;
   int ret_val = std::system(clang_command.c_str());

   if (ret_val != 0) {
      std::cerr << diag << std::endl;
      ASSERT_FALSE(diag.empty());
   } else {
      ASSERT_TRUE(diag.empty()) << diag;

      std::string llvmdiff_command = "llvm-diff ";
      llvmdiff_command += parser_of;
      llvmdiff_command += " ";
      llvmdiff_command += clang_of;
      ASSERT_EQ(std::system(llvmdiff_command.c_str()), 0) << llvmdiff_command << "\nLLVM IRs are different\n"
                                                          << log.str() << std::endl;
   }
}
// ---------------------------------------------------------------------------
INSTANTIATE_TEST_CASE_P(LLVM_DIFF, IntParamTest, testing::Range(33, 35));
// ---------------------------------------------------------------------------
#endif // TEST_PARSER_H