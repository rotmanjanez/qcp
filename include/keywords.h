#ifndef QCP_TOKEN_RESERVED_KEYWORD_HASH_H
#define QCP_TOKEN_RESERVED_KEYWORD_HASH_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "token.h"
// ---------------------------------------------------------------------------
namespace qcp {
namespace token {
// ---------------------------------------------------------------------------
struct GPerfToken;

class ReservedKeywordHash {
   private:
   static inline unsigned int hash(const char* str, size_t len);

   public:
   static const struct GPerfToken* isInWordSet(const char* str, size_t len);
};
// ---------------------------------------------------------------------------
} // end namespace token
} // end namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKEN_RESERVED_KEYWORD_HASH_H