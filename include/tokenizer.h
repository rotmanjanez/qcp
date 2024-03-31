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
   Tokenizer(const std::string_view src) : src{src} {}

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
      explicit const_iterator() : token{TokenType::END}, remainder{} {}
      explicit const_iterator(const std::string_view src) : token{TokenType::UNKNOWN}, remainder{src} {
         ++(*this);
      }

      const_iterator& operator++();
      const_iterator operator++(int);

      reference operator*() const {
         return token;
      }

      pointer operator->() const {
         return &token;
      }

      bool operator==(const const_iterator& other) const {
         return token == other.token;
      }

      auto operator<=>(const const_iterator& other) const {
         return token <=> other.token;
      }

      private:
      Token token;
      std::string_view remainder;
   };

   const_iterator begin() const {
      return const_iterator{src};
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

   private:
   const std::string_view src;
};
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKENIZER_H