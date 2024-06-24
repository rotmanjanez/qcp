#ifndef QCP_TOKENCOUNTER_H
#define QCP_TOKENCOUNTER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
// alexis hates iostream
#include <iostream>
// ---------------------------------------------------------------------------
namespace qcp {
namespace token {
// ---------------------------------------------------------------------------
enum class Kind;
// ---------------------------------------------------------------------------
// an array of counters for each token kind
// useful for determining type specifiers
template <Kind FROM, Kind TO>
class TokenCounter : public std::array<uint8_t, (static_cast<std::size_t>(TO) - static_cast<std::size_t>(FROM) + 1)> {
   public:
   uint8_t& operator[](Kind kind);
   uint8_t operator[](Kind kind) const;
   Kind tokenAt(std::size_t i) const;

   uint8_t consume(Kind kind);
};
// ---------------------------------------------------------------------------
template <Kind FROM, Kind TO>
uint8_t& TokenCounter<FROM, TO>::operator[](Kind kind) {
   return *(this->data() + static_cast<std::size_t>(kind) - static_cast<std::size_t>(FROM));
}
// ---------------------------------------------------------------------------
template <Kind FROM, Kind TO>
uint8_t TokenCounter<FROM, TO>::operator[](Kind kind) const {
   return *(this->data() + static_cast<std::size_t>(kind) - static_cast<std::size_t>(FROM));
}
// ---------------------------------------------------------------------------
template <Kind FROM, Kind TO>
Kind TokenCounter<FROM, TO>::tokenAt(std::size_t i) const {
   return static_cast<Kind>(i + static_cast<std::size_t>(FROM));
}
// ---------------------------------------------------------------------------
template <Kind FROM, Kind TO>
uint8_t TokenCounter<FROM, TO>::consume(Kind kind) {
   if ((*this)[kind]) {
      return (*this)[kind]--;
   }
   return 0;
}
// ---------------------------------------------------------------------------
template <Kind FROM, Kind TO>
std::ostream& operator<<(std::ostream& os, const TokenCounter<FROM, TO>& tc) {
   for (std::size_t i = 0; i < tc.size(); ++i) {
      os << tc.tokenAt(i) << ": " << static_cast<int>(tc[i]);
      if (i < tc.size() - 1) {
         os << ", ";
      }
   }
   return os;
}
// ---------------------------------------------------------------------------
} // namespace token
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKENCOUNTER_H