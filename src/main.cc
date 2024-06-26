// Alexis hates iostream
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
// ---------------------------------------------------------------------------
#include "llvmemitter.h"
#include "parser.h"
#include "tokenizer.h"
// ---------------------------------------------------------------------------
using Parser = qcp::Parser<qcp::emitter::LLVMEmitter>;
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
   std::ifstream file(argv[1]);
   if (!file) {
      std::cerr << argv[1] << " no such file\n";
   }
   std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

   // print code
   std::cout << code << std::endl;

   {
      qcp::DiagnosticTracker diag{argv[1], code};
      qcp::Tokenizer tokenizer{code, diag};
      for (const auto& token : tokenizer) {
         std::cout << token << std::endl;
      }
   }

   // Call your parser
   qcp::DiagnosticTracker diag{argv[1], code};
   Parser parser{code, diag};
   parser.parse();
   parser.getEmitter().dumpToStdout();

   if (!diag.empty()) {
      std::cout << diag << std::endl;
      return 1;
   }

   return 0;
}