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
// BRAIN DUMP
// ---------------------------------------------------------------------------
// keyword                      | all keywords are also identifiers
// identifier                   | all identifiers start with a letter or an underscore
// constant
// - integer-constant       | all integer constants start with a digit
// - floating-constant      | all floating constants start with a digit or a period
// - enumeration-constant   | all enumeration constants are identifiers
// - character-constant     | all character constants start with a single quote or with u8, u, U, or L followed by a single quote
// - predefined-constant    | all predefined constants are keywords
// string-literal               | all string literals start with a double quote or with u8, u, U, or L followed by a double quote
// punctuator                   | all punctuators start with one of [](){}.-+&*~!/%<>=^|?:;#,

// identifiert_start, punctuator_start, ' or "
// ---------------------------------------------------------------------------
using Parser = qcp::Parser<qcp::emitter::LLVMEmitter>;
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
   std::ifstream file(argv[1]);
   std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

   // print code
   std::cout << code << std::endl;

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