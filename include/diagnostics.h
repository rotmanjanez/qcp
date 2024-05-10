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
class DiagnosticMessage {
   public:
   explicit DiagnosticMessage(std::string message) : message_{std::move(message)}, loc_{} {}

   template <typename T>
   DiagnosticMessage(T message, SrcLoc loc) : message_{std::forward<T>(message)}, loc_{loc} {}

   friend std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag);

   bool hasLoc() const {
      return loc_.has_value();
   }

   const SrcLoc& getLoc() const {
      return loc_.value();
   }

   private:
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
      diagnostics_.emplace_back(DiagnosticMessage(message));
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
         std::cerr << os_.str() << std::endl;
         report(DiagnosticMessage(os_.str(), loc_));
         os_.clear();
         loc_ = {};
      }
      return *this;
   }

   const std::string_view getSource(const SrcLoc& loc) const {
      return prog_.substr(loc.loc, loc.len);
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