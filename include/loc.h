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
   using loc_off_t = long long;
   using loc_len_t = unsigned;

   loc_off_t loc;
   loc_len_t len;

   using sv_it = typename std::string_view::const_iterator;

   SrcLoc() : loc{0}, len{0} {}
   SrcLoc(loc_off_t loc, unsigned len = 0) : loc{loc}, len{len} {}
   SrcLoc(loc_off_t loc, loc_off_t locEnd) : loc{loc}, len{static_cast<loc_len_t>(locEnd - loc)} {}
   SrcLoc(sv_it progBegin, sv_it begin, sv_it end) : loc{std::distance(progBegin, begin)}, len{static_cast<loc_len_t>(std::distance(begin, end))} {}

   loc_off_t locEnd() const {
      return loc + len;
   }

   // idea from aengelke
   SrcLoc operator|(const SrcLoc& other) const {
      loc_off_t newLoc{std::min(loc, other.loc)};
      return {newLoc, std::max(locEnd(), other.locEnd() - newLoc)};
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