#ifndef QCP_DIAGNOSTICS_H
#define QCP_DIAGNOSTICS_H
// ---------------------------------------------------------------------------
#include "token.h"
// ---------------------------------------------------------------------------
// Alexis hates iostream
#include <iostream>
#include <map>
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
   std::string_view file() const;
   std::size_t translationUnitLine() const;

   Kind kind() const {
      return kind_;
   }

   private:
   std::vector<long long>::const_iterator findLineBegin() const;
   std::map<std::size_t, std::pair<std::string, std::size_t>>::const_iterator findFileBegin() const;

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
   DiagnosticTracker(std::string filename, std::string_view prog) : filename{std::move(filename)}, prog_{prog} {
      fileStack_[0] = std::make_pair(this->filename, 0);
   }

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

   void registerFileMapping(std::string filename, std::size_t srcOffset) {
      fileStack_[lineBreaks_.size() - 1] = std::make_pair(std::move(filename), srcOffset);
   }

   private:
   DiagnosticMessage::Kind kind_{DiagnosticMessage::Kind::ERROR};
   std::stringstream os_{};
   std::optional<SrcLoc> loc_{};
   std::vector<DiagnosticMessage> diagnostics_{};
   std::vector<long long> lineBreaks_{-1}; // first linebreak is right before the first character
   std::map<std::size_t, std::pair<std::string, std::size_t>> fileStack_{};
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