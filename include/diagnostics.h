#ifndef QCP_DIAGNOSTICS_H
#define QCP_DIAGNOSTICS_H
// ---------------------------------------------------------------------------
#include "token.h"
// ---------------------------------------------------------------------------
// Alexis hates iostream
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
class DiagnosticTracker;
// ---------------------------------------------------------------------------
class DiagnosticMessage {
   public:
   explicit DiagnosticMessage(const DiagnosticTracker& tracker, std::string message) : tracker_{tracker}, message_{std::move(message)}, loc_{} {}

   template <typename T>
   DiagnosticMessage(const DiagnosticTracker& tracker, T message, SrcLoc loc) : tracker_{tracker}, message_{std::forward<T>(message)}, loc_{loc} {}

   friend std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag);

   bool hasLoc() const {
      return loc_.has_value();
   }

   const SrcLoc& getLoc() const {
      return loc_.value();
   }

   private:
   const DiagnosticTracker& tracker_;
   std::string_view prog_;
   std::string message_;
   std::optional<SrcLoc> loc_;
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag);
// ---------------------------------------------------------------------------
class DiagnosticTracker {
   friend std::ostream& operator<<(std::ostream& os, const DiagnosticTracker& tracker);
   // todo: (jr) make this a singleton and change methods to static
   public:
   explicit DiagnosticTracker(std::string_view prog) : prog_{prog} {}

   void report(DiagnosticMessage diag) {
      diagnostics_.push_back(diag);
   }

   void report(const char* message) {
      diagnostics_.emplace_back(DiagnosticMessage(*this, message));
   }

   bool empty() const {
      return diagnostics_.empty();
   }

   void registerLineBreak(std::size_t pos) {
      lineBreaks_.push_back(pos);
   }

   template <typename T>
   DiagnosticTracker& operator<<(const T& value) {
      os_ << value;
      return *this;
   }

   DiagnosticTracker& operator<<(std::ostream& (*pf)(std::ostream&) ) {
      pf(os_);
      if (pf == static_cast<std::ostream& (*) (std::ostream&)>(std::endl)) {
         DiagnosticMessage diag{*this, os_.str(), loc_};
         report(diag);
         std::cerr << diag << std::endl;
         os_.clear();
         loc_ = {};
      }
      return *this;
   }

   const std::string_view getSource(const SrcLoc& loc) const {
      return prog_.substr(loc.loc, loc.len);
   }

   std::string_view getSourceLine(const SrcLoc& loc) const {
      auto lineBegin = std::lower_bound(lineBreaks_.begin(), lineBreaks_.end(), loc.loc) - 1;
      auto newLine = std::find(prog_.begin() + *lineBegin + 1, prog_.end(), '\n');
      auto line = prog_.substr(*lineBegin + 1, newLine - prog_.begin() - *lineBegin);
      // todo: use upper_bound to find the end of the line
      return line;
   }

   DiagnosticTracker& operator<<(SrcLoc loc) {
      loc_ = loc;
      return *this;
   }

   private:
   std::stringstream os_{};
   SrcLoc loc_{};
   std::vector<DiagnosticMessage> diagnostics_{};
   std::vector<std::size_t> lineBreaks_{};
   std::string_view prog_;
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticTracker& tracker);
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_DIAGNOSTICS_H