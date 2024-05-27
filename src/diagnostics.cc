// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag) {
   if (diag.hasLoc()) {
      os << "---\n";
      os << diag.tracker_.getSource(diag.getLoc()) << '\n';
      os << "---\n";
   } else {
      os << "no loc\n";
   }
   os << diag.message_ << '\n';
   return os;
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticTracker& tracker) {
   for (const auto& diag : tracker.diagnostics_) {
      os << diag;
   }
   return os;
}
} // namespace qcp
// ---------------------------------------------------------------------------
