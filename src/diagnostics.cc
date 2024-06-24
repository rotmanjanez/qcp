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
// DiagnosticMessage
// ---------------------------------------------------------------------------
std::vector<long long>::const_iterator DiagnosticMessage::findLineBegin() const {
   return std::lower_bound(tracker_.lineBreaks_.begin(), tracker_.lineBreaks_.end(), loc_.value().loc()) - 1;
}
// ---------------------------------------------------------------------------
std::size_t DiagnosticMessage::line() const {
   if (tracker_.lineBreaks_.back() < loc_.value().loc()) {
      return tracker_.lineBreaks_.size();
   }
   return std::distance(tracker_.lineBreaks_.begin(), findLineBegin()) + 1;
}
// ---------------------------------------------------------------------------
std::size_t DiagnosticMessage::column() const {
   std::size_t lineNo = line();
   auto lineBegin = tracker_.lineBreaks_[lineNo - 1];
   return loc_.value().loc() - lineBegin;
}
// ---------------------------------------------------------------------------
std::string_view DiagnosticMessage::getSourceLine() const {
   auto lineBegin = findLineBegin();
   if (lineBegin == tracker_.lineBreaks_.end()) {
      return {};
   } else if (*lineBegin == tracker_.lineBreaks_.back()) {
      return tracker_.prog_.substr(*lineBegin + 1);
   }
   // do not include the newline character at the beginning and end of the line
   auto off = *lineBegin + 1;
   auto len = *(lineBegin + 1) - off;
   return tracker_.prog_.substr(off, len);
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag) {
   if (diag.loc_.has_value()) {
      std::string lineNo = std::to_string(diag.line());
      os << diag.tracker_.filename << ':' << diag.line() << ':' << diag.column() << ": ";
      switch (diag.kind()) {
         case DiagnosticMessage::Kind::INFO:
            os << "info: ";
            break;
         case DiagnosticMessage::Kind::WARNING:
            os << "warning: ";
            break;
         case DiagnosticMessage::Kind::ERROR:
            os << "error: ";
            break;
         case DiagnosticMessage::Kind::NOTE:
            os << "note: ";
            break;
      }
      os << diag.message_;
      os << std::setw(5) << std::right << lineNo << " | ";
      os << diag.getSourceLine() << '\n';
      os << std::setw(5) << std::right << ""
         << " | ";
      os << std::setw(diag.column()) << '^';
      SrcLoc loc = diag.loc_.value();
      if (loc.len() >= 2) {
         os << std::string(loc.len() - 1, '~');
      }
   } else {
      os << "no loc\n";
      os << diag.message_;
   }
   return os;
}
// ---------------------------------------------------------------------------
// DiagnosticTracker
// ---------------------------------------------------------------------------
bool DiagnosticTracker::empty() const {
   return diagnostics_.empty();
}
// ---------------------------------------------------------------------------
std::size_t DiagnosticTracker::count(DiagnosticMessage::Kind kind) const {
   return std::count_if(diagnostics_.begin(), diagnostics_.end(), [kind](const auto& diag) {
      return diag.kind() == kind;
   });
}
// ---------------------------------------------------------------------------
void DiagnosticTracker::registerLineBreak(long long pos) {
   if (pos <= lineBreaks_.back()) {
      return; // todo: this is only neccesary because operator++ may be called multiple times for a token. this should be removed.
   }
   lineBreaks_.push_back(pos);
}
// ---------------------------------------------------------------------------
DiagnosticTracker& DiagnosticTracker::operator<<(std::ostream& (*pf)(std::ostream&) ) {
   pf(os_);
   if (pf == static_cast<std::ostream& (*) (std::ostream&)>(std::endl)) {
      DiagnosticMessage diag{*this, os_.str(), loc_, kind_};
      if (!silenced || kind_ == DiagnosticMessage::Kind::NOTE) {
         diagnostics_.push_back(diag);
      }
      os_.str("");
      loc_.reset();
      silenced = true;
      kind_ = DiagnosticMessage::Kind::ERROR;
   }
   return *this;
}
// ---------------------------------------------------------------------------
DiagnosticTracker& DiagnosticTracker::operator<<(SrcLoc loc) {
   loc_ = loc;
   return *this;
}
// ---------------------------------------------------------------------------
DiagnosticTracker& DiagnosticTracker::operator<<(DiagnosticMessage::Kind kind) {
   kind_ = kind;
   return *this;
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticTracker& tracker) {
   unsigned errorCount = 0;
   unsigned warningCount = 0;
   for (const auto& diag : tracker.diagnostics_) {
      if (diag.kind() == DiagnosticMessage::Kind::ERROR) {
         ++errorCount;
      } else if (diag.kind() == DiagnosticMessage::Kind::WARNING) {
         ++warningCount;
      }
      os << diag << "\n";
   }
   const char* what = errorCount == 1 ? "error" :
       errorCount > 1                 ? "errors" :
       warningCount == 1              ? "warning" :
       warningCount > 1               ? "warnings" :
                                        nullptr;

   unsigned count = errorCount ? errorCount : warningCount;

   if (what) {
      os << count << ' ' << what << " generated.\n";
   }
   return os;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
