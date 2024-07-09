#ifndef QCP_LOC_H
#define QCP_LOC_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include <algorithm>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
class SrcLoc {
   public:
   using loc_off_t = long long;
   using loc_len_t = unsigned;

   SrcLoc() : loc_{0}, len_{0} {}
   explicit SrcLoc(loc_off_t loc, loc_len_t len = 0) : loc_{loc}, len_{len} {}
   SrcLoc(loc_off_t loc, loc_off_t locEnd) : loc_{loc}, len_{static_cast<loc_len_t>(locEnd - loc)} {}

   // copy and move
   SrcLoc(const SrcLoc&) = default;
   SrcLoc(SrcLoc&&) noexcept = default;
   SrcLoc& operator=(const SrcLoc&) = default;
   SrcLoc& operator=(SrcLoc&&) noexcept = default;

   template <typename T>
   SrcLoc(T begin, T start, T end) : loc_{std::distance(begin, start)}, len_{static_cast<loc_len_t>(std::distance(start, end))} {}

   loc_off_t locEnd() const;
   loc_off_t loc() const;
   loc_len_t len() const;

   // idea from aengelke
   SrcLoc operator|(const SrcLoc& other) const;
   SrcLoc& operator|=(const SrcLoc& other);

   SrcLoc truncate(loc_len_t newLen) const;

   private:
   loc_off_t loc_;
   loc_len_t len_;
};
// ---------------------------------------------------------------------------
template <typename T>
class locatable : public T {
   public:
   using T::T;

   template <typename... Args>
   locatable(SrcLoc loc, Args&&... args) : T{std::forward<Args>(args)...}, loc_{loc} {}

   SrcLoc loc() const {
      return loc_;
   }

   private:
   SrcLoc loc_;
};
// ---------------------------------------------------------------------------
template <typename T>
class locatable<T*> {
   public:
   locatable(T* ptr, SrcLoc loc) : ptr_{ptr}, loc_{loc} {}

   T* operator->() {
      return ptr_;
   }

   T* operator->() const {
      return ptr_;
   }

   T& operator*() {
      return *ptr_;
   }

   T& operator*() const {
      return *ptr_;
   }

   operator T*() {
      return ptr_;
   }

   SrcLoc loc() const {
      return loc_;
   }

   private:
   T* ptr_;
   SrcLoc loc_;
};
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_LOC_H