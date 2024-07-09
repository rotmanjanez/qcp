#ifndef QCP_TOKENIZER_H
#define QCP_TOKENIZER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
#include "keywords.h"
#include "stringpool.h"
// ---------------------------------------------------------------------------
#include <limits>
#include <string_view>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
class Tokenizer {
   using sv_it = typename std::string_view::const_iterator;
   using TK = token::Kind;
   using Token = token::Token;

   public:
   explicit Tokenizer(const std::string_view prog, DiagnosticTracker& diagnistics) : prog_{prog}, diagnostics_{diagnistics} {}

   explicit Tokenizer(const Tokenizer&) = delete;
   Tokenizer& operator=(const Tokenizer&) = delete;

   class const_iterator {
      using sv_it = typename std::string_view::const_iterator;

      public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = const Token;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type*;
      using reference = value_type&;

      private:
      public:
      explicit const_iterator() : token_{TK::END}, prog_{}, progBegin_{prog_.begin()}, pp{0}, diagnostics_{nullptr} {}
      explicit const_iterator(const std::string_view prog_, DiagnosticTracker& diagnistics) : token_{TK::UNKNOWN}, prog_{prog_}, progBegin_{prog_.begin()}, pp{!prog_.empty() && prog_.front() == '#' ? 1 : 0}, diagnostics_{&diagnistics} {
         if (pp) {
            token_ = Token{SrcLoc{0, 1u}, TK::PP_START};
            this->prog_ = prog_.substr(1);
         } else {
            ++(*this);
         }
      }

      const_iterator& operator++();
      const_iterator operator++(int);

      reference operator*() const {
         return token_;
      }

      pointer operator->() const {
         return &token_;
      }

      bool operator==(const const_iterator& other) const {
         return token_ == other.token_;
      }

      auto operator<=>(const const_iterator& other) const {
         return token_ <=> other.token_;
      }

      token::Kind peek() const {
         auto next = *this;
         ++next;
         return next->getKind();
      }

      SrcLoc getPrevLoc() const {
         return prevLoc_;
      }

      operator bool() const {
         return token_.getKind() != TK::END;
      }

      private:
      sv_it getNumberConst(sv_it begin);
      sv_it getPunctuator(sv_it begin);
      sv_it getSCharSequence(sv_it begin);
      sv_it getCCharSequence(sv_it begin);

      Token token_;
      std::string_view prog_;
      sv_it progBegin_;
      SrcLoc prevLoc_{};
      int pp;
      DiagnosticTracker* diagnostics_;
   };

   const_iterator begin() const;
   const_iterator end() const;
   const_iterator cbegin() const;
   const_iterator cend() const;

   const std::string_view& data() const;

   private:
   const std::string_view prog_;
   DiagnosticTracker& diagnostics_;
};
// ---------------------------------------------------------------------------
template <typename T, typename U>
T safe_cast(U value) {
   assert(value <= std::numeric_limits<T>::max() && "value to large for safe cast");
   return static_cast<T>(value);
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKENIZER_H