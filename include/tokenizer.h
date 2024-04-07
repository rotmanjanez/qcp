#ifndef QCP_TOKENIZER_H
#define QCP_TOKENIZER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include <string_view>
// ---------------------------------------------------------------------------
#include "token.h"
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
class Tokenizer {
   public:
   explicit Tokenizer(const std::string_view prog) : prog_{prog} {}

   class const_iterator {
      public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = const Token;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type*;
      using reference = value_type&;

      private:
      using it = std::string_view::const_iterator;

      public:
      explicit const_iterator() : token_{TokenType::END}, remainder{} {}
      explicit const_iterator(const std::string_view prog_) : token_{TokenType::UNKNOWN}, remainder{prog_} {
         ++(*this);
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

      private:
      Token token_;
      std::string_view remainder;
   };

   const_iterator begin() const {
      return const_iterator{prog_};
   }

   const_iterator end() const {
      return const_iterator{};
   }

   const_iterator cbegin() const {
      return begin();
   }

   const_iterator cend() const {
      return end();
   }

   const std::string_view& data() const {
      return prog_;
   }

   private:
   const std::string_view prog_;
};
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKENIZER_H