#ifndef QCP_LOC_H
#define QCP_LOC_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include <algorithm>
#include <string_view>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
struct SrcLoc {
   std::size_t loc;
   unsigned len;

   using sv_it = typename std::string_view::const_iterator;

   SrcLoc() : loc{0}, len{0} {}
   SrcLoc(std::size_t loc, unsigned len) : loc{loc}, len{len} {}
   SrcLoc(std::size_t loc, std::size_t locEnd) : loc{loc}, len{static_cast<unsigned int>(locEnd - loc)} {}
   SrcLoc(sv_it progBegin, sv_it begin, sv_it end) : loc{static_cast<std::size_t>(begin - progBegin)}, len{static_cast<unsigned int>(end - begin)} {}

   std::size_t locEnd() const {
      return loc + len;
   }

   // idea from aengelke
   SrcLoc operator|(const SrcLoc& other) const {
      std::size_t newLoc{std::min(loc, other.loc)};
      return {newLoc, std::max(locEnd(), static_cast<unsigned int>(other.locEnd()) - newLoc)};
   }

   SrcLoc& operator|=(const SrcLoc& other) {
      *this = *this | other;
      return *this;
   }
};
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_LOC_H