#ifndef QCP_LOC_H
#define QCP_LOC_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include <algorithm>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
struct SrcLoc {
   std::size_t loc;
   unsigned len;

   SrcLoc() : loc{0}, len{0} {}
   SrcLoc(std::size_t loc, unsigned len) : loc{loc}, len{len} {}
   SrcLoc(std::size_t loc, std::size_t locEnd) : loc{loc}, len{static_cast<unsigned int>(locEnd - loc)} {}

   std::size_t locEnd() const {
      return loc + len;
   }

   // idea from aengelke
   SrcLoc operator|(const SrcLoc& other) {
      std::size_t newLoc{std::min(loc, other.loc)};
      return {newLoc, std::max(locEnd(), static_cast<unsigned int>(other.locEnd()) - newLoc)};
   }
};
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_LOC_H