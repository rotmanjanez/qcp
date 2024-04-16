// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag) {
   os << diag.message_ << '\n';
   return os;
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticTracker& tracker) {
   for (const auto& diag : tracker.diagnostics_) {
      if (diag.hasLoc()) {
         os << tracker.getSource(diag.getLoc()) << '\n';
      }
      os << diag;
   }
   return os;
}
} // namespace qcp
// ---------------------------------------------------------------------------
