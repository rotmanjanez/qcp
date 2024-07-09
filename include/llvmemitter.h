#ifndef QCP_LLVM_EMITTER_H
#define QCP_LLVM_EMITTER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "stringpool.h"
#include "emittertraits.h"
// ---------------------------------------------------------------------------
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/raw_ostream.h"
// ---------------------------------------------------------------------------
#include <array>
#include <iostream>
#include <span>
#include <variant>
#include <getopt.h>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
namespace op {
enum class Kind; // forward declaration
} // namespace op
// ---------------------------------------------------------------------------
namespace type {
// ---------------------------------------------------------------------------
enum class Cast; // forward declaration
// ---------------------------------------------------------------------------
template <typename T>
class Type; // forward declaration
} // namespace type
// ---------------------------------------------------------------------------
namespace emitter {
// ---------------------------------------------------------------------------
class LLVMEmitter {
   public:
   using ssa_t = llvm::Value;
   using const_t = llvm::Constant;
   using iconst_t = llvm::ConstantInt;
   using phi_t = llvm::PHINode;
   using bb_t = llvm::BasicBlock;
   using ty_t = llvm::Type;
   using fn_t = llvm::Function;
   using sw_t = llvm::SwitchInst;

   // containes a backref to the type with emitterType() method
   // todo: make const
   using Type = typename emitter_traits<LLVMEmitter>::Type;
   using value_t = typename emitter_traits<LLVMEmitter>::value_t;
   using const_or_iconst_t = typename emitter_traits<LLVMEmitter>::const_or_iconst_t;

   // todo: check if int <= long
   static constexpr bool CHAR_HAS_16_BIT = false;
   static constexpr bool CHAR_IS_SIGNED = true;
   static constexpr bool INT_HAS_64_BIT = false;
   static constexpr bool LONG_HAS_64_BIT = true;

   static constexpr struct option LONG_OPTIONS[] = {
      {"-emit-llvm-bc", no_argument, 0, 0},
      {"-emit-llvm", no_argument, 0, 0},
   };
   static constexpr char SHORT_OPTIONS[] = "";

   static constexpr const char* const HELP_MESSAGE = "  -emit-llvm-bc\n"
                                                     "  -emit-llvm\n";

   LLVMEmitter();

   void dumpToFile(const std::string& filename) {
      std::error_code EC;
      llvm::raw_fd_ostream OS(filename, EC);
      Mod->print(OS, nullptr);
   }

   void writeToObjFile(int fd);

   void writeToObjFile(const std::string& filename);

   void writeToBitcodeFile(int fd) {
      llvm::raw_fd_ostream OS(fd, false);
      Mod->print(OS, nullptr);
   }


   void dumpToStdout() {
      Mod->print(llvm::outs(), nullptr);
   }

   void writeLLVMToFile(int fd) {
      llvm::raw_fd_ostream OS(fd, false);
      Mod->print(OS, nullptr);
   }

   void finalizeFn(fn_t* fn);

   unsigned long long getUIntegerValue(iconst_t* c) {
      return c->getZExtValue();
   }

   long long getIntegerValue(iconst_t* c) {
      return c->getSExtValue();
   }

   iconst_t* sizeOf(Type ty);

   ty_t* emitVoidTy();
   ty_t* emitIntTy(unsigned bits);
   ty_t* emitFloatTy();
   ty_t* emitDoubleTy();
   ty_t* emitLongDoubleTy();
   ty_t* emitPtrTo(Type ty);
   ty_t* emitArrayTy(Type ty, iconst_t* size);
   ty_t* emitStructTy(std::span<const Type> tys, bool incomplete, Ident name = Ident());

   ssa_t* emitUndef();
   ssa_t* emitPoison();

   // todo: change vector to iterator
   ty_t* emitFnTy(Type retTy, std::vector<Type> argTys, bool isVarArgFnTy);

   ssa_t* emitGlobalVar(Type ty, Ident name = Ident());
   const_t* emitFnPtr(Type ty, fn_t* fn);

   void setInitValueGlobalVar(ssa_t* val, const_or_iconst_t init);
   void zeroInitGlobalVar(Type ty, ssa_t* val);


   fn_t* emitFnProto(Type fnTy, bool alwaysInline, bool noReturn, Ident name = Ident());
   bb_t* emitFn(fn_t* fnProto);
   bool isFnProto(fn_t* fn);
   ssa_t* getParam(fn_t* fn, unsigned idx);

   bb_t* emitBB(fn_t* fn, bb_t* insertBefore = nullptr, Ident name = Ident());

   iconst_t* emitIConst(Type ty, unsigned long value);
   const_t* emitFPConst(Type ty, double value);
   const_t* emitNullPtr(Type ty);
   const_or_iconst_t emitZeroConst(Type ty);


   const_t* emitArrayConst(Type ty, std::span<const const_or_iconst_t> values);
   const_t* emitArrayConst(Type ty, const_or_iconst_t value);

   const_t* emitStructConst(Type ty, std::span<const const_or_iconst_t> values);


   const_t* emitStringLiteral(const std::string_view str);

   ssa_t* emitLocalVar(fn_t* fn, bb_t* entry, Type ty, Ident name = Ident(), bool insertAtBegin = false /* TODO: this is only here so that the output is identical to clang but not necessary */);
   void zeroInitLocalVar(bb_t* entry, Type ty, ssa_t* val);

   ssa_t* emitAlloca(bb_t* bb, Type ty, ssa_t* size, Ident name = Ident());
   ssa_t* emitLoad(bb_t* bb, Type ty, ssa_t* ptr, Ident name = Ident());
   void emitStore(bb_t* bb, Type ty, value_t value, ssa_t* ptr);

   ssa_t* emitJump(bb_t* bb, bb_t* target);
   ssa_t* emitBranch(bb_t* bb, bb_t* trueBB, bb_t* falseBB, value_t cond);
   ssa_t* emitRet(bb_t* bb, value_t value);

   ssa_t* emitPhi(bb_t* bb, Type ty, std::span<std::pair<value_t, bb_t*>> incoming, Ident name = Ident());

   ssa_t* emitBinOp(bb_t* bb, Type ty, op::Kind kind, value_t lhs, value_t rhs, ssa_t* dest = nullptr, Ident name = Ident());
   const_or_iconst_t emitConstBinOp(bb_t* bb, Type ty, op::Kind kind, const_or_iconst_t lhs, const_or_iconst_t rhs, Ident name = Ident());
   ssa_t* emitIncDecOp(bb_t* bb, Type ty, op::Kind kind, ssa_t* operand, Ident name = Ident());
   ssa_t* emitNeg(bb_t* bb, Type ty, ssa_t* operand, Ident name = Ident());
   const_or_iconst_t emitConstNeg(bb_t* bb, Type ty, const_or_iconst_t operand, Ident name = Ident());
   ssa_t* emitBWNeg(bb_t* bb, Type ty, ssa_t* operand, Ident name = Ident());
   const_or_iconst_t emitConstBWNeg(bb_t* bb, Type ty, const_or_iconst_t operand, Ident name = Ident());
   const_or_iconst_t emitConstCast(bb_t* bb, Type fromTy, const_or_iconst_t val, Type toTy, qcp::type::Cast cast);

   ssa_t* emitCast(bb_t* bb, Type fromTy, ssa_t* val, Type toTy, qcp::type::Cast cast);

   ssa_t* emitCall(bb_t* bb, fn_t* fn, std::span<const value_t> args, Ident name = Ident());
   ssa_t* emitCall(bb_t* bb, Type fnTy, value_t fnPtr, std::span<const value_t> args, Ident name = Ident());

   ssa_t* emitGEP(bb_t* bb, Type ty, value_t ptr, std::span<const uint64_t> idx, Ident name = Ident());
   ssa_t* emitGEP(bb_t* bb, Type ty, value_t ptr, std::span<const std::uint32_t> idx, Ident name = Ident());
   ssa_t* emitGEP(bb_t* bb, Type ty, value_t ptr, value_t idx, Ident name = Ident());

   sw_t* emitSwitch(bb_t* bb, value_t value);
   void addSwitchCase(sw_t* sw, iconst_t* value, bb_t* target);
   void addSwitchDefault(sw_t* sw, bb_t* target);

   private:
   template <typename T, typename Fn>
   ssa_t* emitGEPImpl(bb_t* bb, Type ty, value_t ptr, std::span<T> indices, Ident name, Fn fn);

   ssa_t* emitCall(bb_t* bb, llvm::FunctionCallee fn, std::span<const value_t> args, Ident name = Ident());
   ssa_t* emitAllocaImpl(bb_t* bb, Type ty, ssa_t* size, Ident name, bool insertAtBegin);

   const_t* zeroConst(Type ty);
   void writeToObjFileImpl(llvm::raw_fd_ostream& OS);

   llvm::LLVMContext Ctx;
   std::unique_ptr<llvm::Module> mod_;
   llvm::Module* Mod;
   llvm::TargetMachine* TM;
   llvm::IRBuilder<> Builder;
};
// ---------------------------------------------------------------------------
inline typename LLVMEmitter::ssa_t* asLLVMValue(const typename LLVMEmitter::value_t& val) {
   if (std::holds_alternative<std::monostate>(val)) {
      return nullptr;
   } else if (typename LLVMEmitter::ssa_t* const* ssa = std::get_if<typename LLVMEmitter::ssa_t*>(&val)) {
      return *ssa;
   } else if (typename LLVMEmitter::iconst_t* const* iconst = std::get_if<typename LLVMEmitter::iconst_t*>(&val)) {
      return *iconst;
   }
   return std::get<typename LLVMEmitter::const_t*>(val);
}
// ---------------------------------------------------------------------------
template <typename T, typename Fn>
typename LLVMEmitter::ssa_t* LLVMEmitter::emitGEPImpl(bb_t* bb, Type ty, value_t ptr, std::span<T> indices, Ident name, Fn fn) {
   Builder.SetInsertPoint(bb);
   std::vector<llvm::Value*> llvmindices;
   llvmindices.reserve(indices.size());
   for (auto& i : indices) {
      llvmindices.push_back(fn(i));
   }
   return Builder.CreateGEP(static_cast<ty_t*>(ty), asLLVMValue(ptr), llvmindices, static_cast<std::string>(name).c_str(), true); // ask alexis: are all gep inbounds?
}
// ---------------------------------------------------------------------------
} // namespace emitter
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_LLVM_EMITTER_H