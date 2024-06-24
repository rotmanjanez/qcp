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
      NOTE,
   };

   template <typename T>
   DiagnosticMessage(const DiagnosticTracker& tracker, T message, std::optional<SrcLoc> loc, Kind kind) : tracker_{tracker}, kind_{kind}, message_{std::forward<T>(message)}, loc_{loc} {}

   friend std::ostream& operator<<(std::ostream& os, const DiagnosticMessage& diag);

   std::string_view getSourceLine() const;

   std::size_t line() const;
   std::size_t column() const;

   Kind kind() const {
      return kind_;
   }

   private:
   std::vector<long long>::const_iterator findLineBegin() const;

   const DiagnosticTracker& tracker_;
   Kind kind_;
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

   public:
   DiagnosticTracker(std::string filename, std::string_view prog) : filename{std::move(filename)}, prog_{prog} {}

   template <typename T>
   DiagnosticTracker& operator<<(const T& value);

   DiagnosticTracker& operator<<(std::ostream& (*pf)(std::ostream&) );
   DiagnosticTracker& operator<<(SrcLoc loc);
   DiagnosticTracker& operator<<(DiagnosticMessage::Kind kind);

   bool empty() const;
   std::size_t count(DiagnosticMessage::Kind kind) const;

   void registerLineBreak(long long pos);

   void unsilence() {
      silenced = false;
   }

   private:
   DiagnosticMessage::Kind kind_{DiagnosticMessage::Kind::ERROR};
   std::stringstream os_{};
   std::optional<SrcLoc> loc_{};
   std::vector<DiagnosticMessage> diagnostics_{};
   std::vector<long long> lineBreaks_{-1}; // first linebreak is right before the first character
   bool silenced = false;
   std::string filename;
   std::string_view prog_;
};
// ---------------------------------------------------------------------------
template <typename T>
DiagnosticTracker& DiagnosticTracker::operator<<(const T& value) {
   os_ << value;
   return *this;
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DiagnosticTracker& tracker);
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_DIAGNOSTICS_H