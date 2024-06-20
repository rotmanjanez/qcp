// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
// ---------------------------------------------------------------------------
#include <algorithm>
#include <iomanip>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag) {
   if (diag.hasLoc()) {
      std::string lineNo = std::to_string(diag.line());
      os << diag.tracker_.filename << ':' << diag.line() << ':' << diag.column() << ": ";
      switch (diag.kind) {
         case DiagnosticMessage::Kind::INFO:
            os << "info: ";
            break;
         case DiagnosticMessage::Kind::WARNING:
            os << "warning: ";
            break;
         case DiagnosticMessage::Kind::ERROR:
            os << "error: ";
            break;
      }
      os << diag.message_;
      os << std::setw(5) << std::right << lineNo << " | ";
      os << diag.tracker_.getSourceLine(diag.getLoc()) << '\n';
      os << std::setw(5) << std::right << ""
         << " | ";
      os << std::setw(diag.column()) << '^';
   } else {
      os << "no loc\n";
      os << diag.message_;
   }
   return os;
}
// ---------------------------------------------------------------------------
DiagnosticTracker& DiagnosticTracker::operator<<(std::ostream& (*pf)(std::ostream&) ) {
   pf(os_);
   if (pf == static_cast<std::ostream& (*) (std::ostream&)>(std::endl)) {
      DiagnosticMessage diag{*this, os_.str(), loc_};
      if (!silenced) {
         report(diag);
      }
      os_.str("");
      loc_ = {};
      silenced = true;
   }
   return *this;
}
// ---------------------------------------------------------------------------
std::size_t DiagnosticMessage::line() const {
   if (tracker_.lineBreaks_.back() < getLoc().loc) {
      return tracker_.lineBreaks_.size();
   }
   auto it = std::lower_bound(tracker_.lineBreaks_.begin(), tracker_.lineBreaks_.end(), getLoc().loc) - 1;
   return std::distance(tracker_.lineBreaks_.begin(), it) + 1;
}
// ---------------------------------------------------------------------------
std::size_t DiagnosticMessage::column() const {
   std::size_t lineNo = line();
   auto lineBegin = tracker_.lineBreaks_[lineNo - 1];
   return getLoc().loc - lineBegin;
}
// ---------------------------------------------------------------------------
std::string DiagnosticTracker::getSourceLine(const SrcLoc& loc) const {
   auto lineBegin = std::lower_bound(lineBreaks_.begin(), lineBreaks_.end(), loc.loc) - 1;
   auto newLine = std::find(prog_.begin() + *lineBegin + 1, prog_.end(), '\n');
   // do not include the newline character at the beginning and end of the line
   return std::string(prog_.substr(*lineBegin + 1, newLine - prog_.begin() - *lineBegin - 1));
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticTracker& tracker) {
   for (const auto& diag : tracker.diagnostics_) {
      os << diag << "\n\n";
   }
   if (tracker.diagnostics_.size() == 1) {
      os << "1 error generated.\n";
      return os;
   } else {
      os << tracker.diagnostics_.size() << " errors generated.\n";
   }
   return os;
}
} // namespace qcp
// ---------------------------------------------------------------------------
