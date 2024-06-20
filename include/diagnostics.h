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
   enum class Kind {
      INFO,
      WARNING,
      ERROR,
   };

   explicit DiagnosticMessage(const DiagnosticTracker& tracker, std::string message, Kind kind = Kind::ERROR) : tracker_{tracker}, kind{kind}, message_{std::move(message)}, loc_{} {}

   template <typename T>
   DiagnosticMessage(const DiagnosticTracker& tracker, T message, SrcLoc loc, Kind kind = Kind::ERROR) : tracker_{tracker}, kind{kind}, message_{std::forward<T>(message)}, loc_{loc} {}

   friend std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag);

   bool hasLoc() const {
      return loc_.has_value();
   }

   const SrcLoc& getLoc() const {
      return loc_.value();
   }

   std::size_t line() const;
   std::size_t column() const;

   private:
   const DiagnosticTracker& tracker_;
   Kind kind;
   std::string message_;
   std::optional<SrcLoc> loc_;
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag);
// ---------------------------------------------------------------------------
class DiagnosticTracker {
   friend std::ostream& operator<<(std::ostream& os, const DiagnosticTracker& tracker);
   friend std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag);
   friend class DiagnosticMessage;
   // todo: (jr) make this a singleton and change methods to static
   public:
   DiagnosticTracker(std::string filename, std::string_view prog) : filename{std::move(filename)}, prog_{prog} {}

   void report(DiagnosticMessage diag) {
      diagnostics_.push_back(diag);
   }

   void report(const char* message) {
      diagnostics_.emplace_back(DiagnosticMessage(*this, message));
   }

   bool empty() const {
      return diagnostics_.empty();
   }

   void registerLineBreak(long long pos) {
      if (pos == lineBreaks_.back()) {
         return; // todo: this is only neccesary because operator++ may be called multiple times for a token. this should be removed.
      }
      lineBreaks_.push_back(pos);
   }

   template <typename T>
   DiagnosticTracker& operator<<(const T& value) {
      os_ << value;
      return *this;
   }

   DiagnosticTracker& operator<<(std::ostream& (*pf)(std::ostream&) );

   const std::string_view getSource(const SrcLoc& loc) const {
      return prog_.substr(loc.loc, loc.len);
   }

   std::string getSourceLine(const SrcLoc& loc) const;

   DiagnosticTracker& operator<<(SrcLoc loc) {
      loc_ = loc;
      return *this;
   }

   void unsilence() {
      silenced = false;
   }

   private:
   std::stringstream os_{};
   SrcLoc loc_{};
   std::vector<DiagnosticMessage> diagnostics_{};
   std::vector<long long> lineBreaks_{-1}; // first linebreak is right before the first character
   bool silenced = false;
   std::string filename;
   std::string_view prog_;
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticTracker& tracker);
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_DIAGNOSTICS_H