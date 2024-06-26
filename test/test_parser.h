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
#include <span>
#include <sstream>
#include <string>
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using Parser = qcp::Parser<qcp::emitter::LLVMEmitter>;
class qcptest : public testing::TestWithParam<std::tuple<std::string, int>> {};
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
void printDiff(std::ostream& os, std::span<std::string> a, std::span<std::string> b) {
   if (a.empty() && b.empty()) {
      return;
   }
   auto ait = a.begin();
   auto bit = b.begin();
   while (ait != a.end() && bit != b.end() && *ait == *bit) {
      os << ' ' << *ait << std::endl;
      ++ait;
      ++bit;
   }
   a = std::span<std::string>(ait, a.end());
   b = std::span<std::string>(bit, b.end());
   unsigned int aIdx = std::numeric_limits<unsigned int>::max();
   unsigned int bIdx = std::numeric_limits<unsigned int>::max();
   for (unsigned int i = 0; i < a.size(); ++i) {
      for (unsigned int j = 0; j < b.size(); ++j) {
         if (a[i] == b[j] && (aIdx + bIdx) > (i + j)) {
            aIdx = i;
            bIdx = j;
         }
      }
   }
   aIdx = std::min(aIdx, static_cast<unsigned int>(a.size()));
   bIdx = std::min(bIdx, static_cast<unsigned int>(b.size()));
   // print a and b up to aIdx and bIdx
   for (unsigned int i = 0; i < aIdx; ++i) {
      os << '<' << a[i] << std::endl;
   }
   for (unsigned int i = 0; i < bIdx; ++i) {
      os << '>' << b[i] << std::endl;
   }
   printDiff(os, a.subspan(aIdx), b.subspan(bIdx));
}
// ---------------------------------------------------------------------------
void printDiff(std::ostream& os, std::stringstream& a, std::stringstream& b) {
   std::vector<std::string> aLines;
   std::vector<std::string> bLines;
   std::string line;
   while (std::getline(a, line)) {
      aLines.emplace_back(std::move(line));
   }
   while (std::getline(b, line)) {
      bLines.emplace_back(std::move(line));
   }
   printDiff(os, std::span<std::string>(aLines), std::span<std::string>(bLines));
}
// ---------------------------------------------------------------------------
TEST_P(qcptest, output) {
   auto [dir, i] = GetParam();
   std::stringstream input_filename;
   input_filename << dir << "/" << std::to_string(i) << ".c";

   // Read the C code from the input file
   std::ifstream input_file(input_filename.str());
   std::stringstream code;
   code << input_file.rdbuf();

   std::string parser_of = "/tmp/parser_llvm_ir_";
   parser_of += std::to_string(i);
   parser_of += ".ll";

   // Call your parser
   std::string input_code = code.str();
   std::stringstream log;
   qcp::DiagnosticTracker diag{input_filename.str(), input_code};
   Parser parser{input_code, diag, log};
   parser.parse();

   parser.getEmitter().dumpToFile(parser_of);

   // Generate LLVM IR using clang
   std::string clang_of = dir + "/expected/llvm/";
   clang_of += std::to_string(i);
   clang_of += ".ll";

   std::string clang_stderr{};
   std::string clang_stderr_file = dir + "/expected/stderr/";
   clang_stderr_file += std::to_string(i);
   clang_stderr_file += ".txt";

   if (std::filesystem::exists(clang_stderr_file)) {
      std::ifstream clang_stderr_input(clang_stderr_file);
      clang_stderr = std::string((std::istreambuf_iterator<char>(clang_stderr_input)), std::istreambuf_iterator<char>());
   }

   if (!clang_stderr.empty()) {
      std::stringstream err;
      err << diag;
      if (err.str() != clang_stderr) {
         std::stringstream clangerr(clang_stderr);
         printDiff(std::cout, err, clangerr);
         FAIL() << "Diagnostic messages are different\n";
      }
   } else {
      ASSERT_EQ(diag.count(qcp::DiagnosticMessage::Kind::ERROR), 0) << diag;

      std::string llvmdiff_command = "llvm-diff ";
      llvmdiff_command += parser_of;
      llvmdiff_command += " ";
      llvmdiff_command += clang_of;
      ASSERT_EQ(std::system(llvmdiff_command.c_str()), 0) << llvmdiff_command << "\nLLVM IRs are different\n"
                                                          << std::endl;
   }
}
// ---------------------------------------------------------------------------
INSTANTIATE_TEST_CASE_P(DiagnosticMessages, qcptest, testing::Combine(testing::Values("diagnostics"), testing::Range(1, 80)));
INSTANTIATE_TEST_CASE_P(LLVM_DIFF1, qcptest, testing::Combine(testing::Values("test1"), testing::Range(1, 128)));
// ---------------------------------------------------------------------------
#endif // TEST_PARSER_H