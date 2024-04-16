
#include "benchmark/benchmark.h"
#include "csmith.h"
#include "tokenizer.h"
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
void Tokenizer(benchmark::State& state) {
   std::string src{qcp::tool::random_c_program("gcc", 0)};
   std::size_t n = 0;
   for (auto _ : state) {
      qcp::DiagnosticTracker diag{src};
      qcp::Tokenizer ts{src, diag};
      for (auto token : ts) {
         ++n;
         benchmark::DoNotOptimize(token);
      }
   }
   state.SetItemsProcessed(n);
}
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
BENCHMARK(Tokenizer)->Unit(benchmark::kNanosecond);
