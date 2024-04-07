#ifndef QCP_TOOL_CSMITH_H
#define QCP_TOOL_CSMITH_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
// ---------------------------------------------------------------------------
namespace qcp {
namespace tool {
// ---------------------------------------------------------------------------
std::string random_c_program(std::string compiler, int seed) {
   char name[] = "/tmp/csmith-XXXXXX";
   mkstemp(name);

   std::stringstream ss;
   // todo: (jr) relative paths not optimal
   ss << "./vendor/csmith/bin/csmith -s " << seed << " | " << compiler << " -I vendor/csmith/include/* -E -x c - > " << name;
   auto ret = std::system(ss.str().c_str());
   assert(ret == 0 && "csmith failed");

   std::stringstream src{};
   std::ifstream ifs{name};

   for (std::string line; std::getline(ifs, line);) {
      if (line[0] != '#') {
         src << line << '\n';
      }
   }

   return src.str();
}
// ---------------------------------------------------------------------------
} // namespace tool
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOOL_CSMITH_H