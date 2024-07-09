#ifndef QCP_TYPE_H
#define QCP_TYPE_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "operator.h"
#include "token.h"
// ---------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <variant>
#include <vector>
#include <cstdint>
#include <limits>
// ---------------------------------------------------------------------------
namespace qcp {
namespace type {
// ---------------------------------------------------------------------------
enum class Kind {
#include "defs/types.def"
};
// ---------------------------------------------------------------------------
enum class Sign {
   SIGNED,
   UNSIGNED,
   UNSPECIFIED,
};
// ---------------------------------------------------------------------------
enum class Cast {
   // clang-format off
   TRUNC, ZEXT, SEXT, FPTOUI, FPTOSI, UITOFP, SITOFP, FPTRUNC, FPEXT,
   PTRTOINT, INTTOPTR, BITCAST, END
   // clang-format on
};
// ---------------------------------------------------------------------------
template <typename T>
struct Base;
// ---------------------------------------------------------------------------
template <typename T>
class TypeFactory;
// ---------------------------------------------------------------------------
template <typename T>
class Type {
   using BaseType = Base<T>;
   using Token = token::Token;
   using ty_t = typename T::ty_t;

   template <typename U>
   friend std::ostream &operator<<(std::ostream &os, const Type<U> &ty);

   friend TypeFactory<T>;
   // sfriend typename TypeFactory::DeclTypeBaseRef;
   // friend typename Base::const_member_iterator;

   explicit Type(std::pair<unsigned short, std::vector<BaseType> &> ini) : index_{ini.first}, types_{&ini.second} {}
   Type(std::vector<BaseType> &types, unsigned short index) : index_{index}, types_{&types} {}

   public:
   // todo: change to bit flags
   struct Qualifiers {
      bool operator==(const Qualifiers &other) const = default;
      bool operator!=(const Qualifiers &other) const = default;

      bool operator<(const Qualifiers &other) const {
         return std::tie(CONST, RESTRICT, VOLATILE) < std::tie(other.CONST, other.RESTRICT, other.VOLATILE);
      }
      bool operator>(const Qualifiers &other) const {
         return std::tie(CONST, RESTRICT, VOLATILE) > std::tie(other.CONST, other.RESTRICT, other.VOLATILE);
      }

      unsigned char CONST : 1 = false,
                            RESTRICT : 1 = false,
                            VOLATILE : 1 = false;
      // todo: bool ATOMIC = false;
   };

   Type() : index_{0}, types_(nullptr) {}

   Type(const Type &other, Qualifiers qualifiers) : qualifiers{qualifiers}, index_{other.index_}, types_{other.types_} {}

   Type(const Type &other) = default;
   Type(Type &&other) = default;
   Type &operator=(const Type &other) = default;
   Type &operator=(Type &&other) = default;

   static Type discardQualifiers(const Type &other) {
      Type ty{other};
      ty.qualifiers = {};
      return ty;
   }

   static Type qualified(const Type &other, token::Kind kind) {
      Type ty{other};
      ty.addQualifier(kind);
      return ty;
   }

   bool isCompatibleWith(const Type &other) const {
      if (!*this || !other) {
         return false;
      }
      if (qualifiers != other.qualifiers) {
         return false;
      }
      return (*this)->isCompatibleWith(*other);
   }

   Type &addQualifier(token::Kind kind) {
      switch (kind) {
         case token::Kind::CONST:
            qualifiers.CONST = true;
            break;
         case token::Kind::RESTRICT:
            qualifiers.RESTRICT = true;
            break;
         case token::Kind::VOLATILE:
            qualifiers.VOLATILE = true;
            break;
         // todo: case token::Kind::ATOMIC:
         // todo:    ty.qualifiers.ATOMIC = true;
         // todo:    break;
         default:
            assert(false && "Invalid token::Kind to qualify Type with");
            break;
      }
      return *this;
   }

   bool operator==(const Type &other) const {
      return *this && other && **this == *other && qualifiers == other.qualifiers;
   }

   // todo: change to partial ordering
   std::strong_ordering operator<=>(const Type &other) const {
      return **this <=> *other;
   }

   operator bool() const {
      return index_ != 0 && types_ != nullptr;
   }

   operator ty_t *() const {
      return static_cast<ty_t *>(**this);
   }

   const BaseType *operator->() const {
      return &**this;
   }

   const BaseType &operator*() const {
      assert(types_ && "Type is not initialized");
      return (*types_)[index_];
   }

   // iterator over the type as a signle element, but can iterate over member, step into or leave struct and union types
   // members can be accessed by member access or dereference operator
   class const_iterator {
      using member_vec_t = std::vector<tagged<Type>>;
      using member_it_t = typename member_vec_t::const_iterator;

      friend Type;

      public:
      using value_type = const tagged<Type>;
      using reference = const tagged<Type> &;
      using pointer = const tagged<Type> *;
      using iterator_category = std::input_iterator_tag;
      using difference_type = std::ptrdiff_t;

      const_iterator() : root{}, indices_{}, memberIt_{} {}
      const_iterator(Type root) : root{root}, indices_{0, 0}, memberIt_{} {}
      const_iterator(tagged<Type> root) : root{root}, indices_{0, 0}, memberIt_{} {}

      // copy and move operators
      const_iterator(const const_iterator &) = default;
      const_iterator(const_iterator &&) = default;
      const_iterator &operator=(const const_iterator &) = default;
      const_iterator &operator=(const_iterator &&) = default;

      const_iterator &enter() {
         if ((**this)->isStructTy() || (**this)->isUnionTy()) {
            indices_.push_back(0);
            memberIt_.push_back((**this)->getMembers().begin());
         } else if ((**this)->isArrayTy()) {
            getNthElement(0);
         } else {
            assert(false && "Cannot enter non-struct, non-union or non-array type");
         }
         return *this;
      }

      const_iterator &leave() {
         assert(indices_.size() > 2 && "Cannot leave root type");
         if (parent() && parent()->isArrayTy()) {
            root.pop_back();
         }
         indices_.pop_back();
         memberIt_.pop_back();
         return *this;
      }

      // this does not change the type, because arrays have no tagged elements.
      // however, as we already build the indices, we can use them to get the nth element
      const_iterator &getNthElement(std::uint32_t n) {
         assert((**this)->isArrayTy() && "Cannot get nth element from non-struct or non-union type");
         root.push_back((**this)->getElementTy());
         indices_.push_back(n);
         memberIt_.push_back(root.end() - 1);
         return *this;
      }

      const_iterator &operator++() {
         if (canTransparentlyEnter()) {
            do {
               enter();
            } while (canTransparentlyEnter());
         } else {
            step();
            // '>' can happen on array access with getNthElement
            while (canTransparentlyLeave()) {
               indices_.pop_back();
               memberIt_.pop_back();
               if (!memberIt_.empty()) {
                  step();
               }
            }
         }
         return *this;
      }

      bool operator==(const const_iterator &other) const {
         return (!*this && !other) || (root == other.root && (root.empty() || indices_ == other.indices_));
      }

      operator bool() const {
         return !memberIt_.empty() && (!parent() || indices_.back() < numParentMembers());
      }

      const_iterator operator++(int) {
         const_iterator tmp = *this;
         ++(*this);
         return tmp;
      }

      reference operator*() const {
         if (memberIt_.empty()) {
            return root.front();
         }
         return *memberIt_.back();
      }

      pointer operator->() const {
         return &*(*this);
      }

      const std::vector<std::uint64_t> &GEPDerefValues() const {
         return indices_;
      }

      ~const_iterator() {
      }

      Type parent() const {
         if (memberIt_.empty()) {
            return {};
         } else if (memberIt_.size() == 1) {
            return root.front();
         }
         return *memberIt_[memberIt_.size() - 2];
      }

      private:
      void step() {
         ++indices_.back();
         if (!parent() || !parent()->isArrayTy()) {
            ++memberIt_.back();
         }
      }

      std::size_t numParentMembers() const {
         if (indices_.size() <= 2) {
            return 0;
         }
         if (parent() && (parent()->isStructTy() || parent()->isUnionTy())) {
            return parent()->getMembers().size();
         } else if (parent() && parent()->isArrayTy()) {
            if (parent()->isFixedSizeArrayTy()) {
               return parent()->getArraySize();
            } else if (parent()->arrayTy().unspecifiedSize) {
               return std::numeric_limits<std::size_t>::max();
            } else {
               assert(false && "Cannot get number of members from non-fixed size array type");
            }
         }
         return 0;
      }
      
      bool canTransparentlyLeave() const {
         return  indices_.size() > 2 && parent() && indices_.back() >= numParentMembers() && (parent()->isStructTy() || parent()->isUnionTy()) && !parent()->getTag();
      }

      bool canTransparentlyEnter() const {
         return **this && ((**this)->isStructTy() || (**this)->isUnionTy()) && !(**this)->structOrUnionTy().tag && (**this)->getMembers().size() > 0;
      }

      // single elementlist to be compatible with pointer aand reference type
      member_vec_t root;
      std::vector<std::uint64_t> indices_;
      // these is a denormalization for performance reasons
      std::vector<member_it_t> memberIt_;
   };

   const_iterator begin() const {
      return {*this};
   }

   const_iterator end() const {
      return {};
   }

   const_iterator membersBegin() const {
      const_iterator it{*this};
      it.enter();
      if (it.canTransparentlyEnter()) {
         ++it;
      }
      return it;
   }

   const_iterator membersEnd() const {
      return {};
   }

   Qualifiers qualifiers;

   private:
   // todo: maybe short is not enough
   unsigned short index_;
   std::vector<BaseType> *types_;
};
// ---------------------------------------------------------------------------
// Type
// ---------------------------------------------------------------------------
template <typename T>
std::ostream &operator<<(std::ostream &os, const Type<T> &ty) {
   if (ty.index_ == 0) {
      os << "<error-type>";
      return os;
   }
   if (ty.qualifiers.CONST) {
      os << "const ";
   }
   if (ty.qualifiers.RESTRICT) {
      os << "restrict ";
   }
   if (ty.qualifiers.VOLATILE) {
      os << "volatile ";
   }
   // todo: if (ty.qualifiers.ATOMIC) {
   // todo:    os << "_Atomic ";
   // todo: }
   os << *ty;
   return os;
}
// ---------------------------------------------------------------------------
} // namespace type
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TYPE_H