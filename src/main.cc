#include <iostream>
#include <string>
#include <string_view>
// ---------------------------------------------------------------------------
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
int main(int argc, char* argv[]) {
   if (argc > 1) {
      std::cout << "Usage: " << argv[0] << std::endl;
      std::cout << "Reads code from stdin and tokenizes it." << std::endl;
      return 1;
   }

   std::string code;
   std::string line;
   while (std::getline(std::cin, line)) {
      code += line + '\n';
   }

   qcp::DiagnosticTracker diag{code};
   qcp::Tokenizer<qcp::DiagnosticTracker> ts{code, diag};
   for (const auto& token : ts) {
      std::cout << token << '\n';
   }

   qcp::Parser<qcp::DiagnosticTracker> parser{code, diag};

   parser.parse();

   if (!diag.empty()) {
      std::cout << diag << std::endl;
      return 1;
   }

   return 0;
}