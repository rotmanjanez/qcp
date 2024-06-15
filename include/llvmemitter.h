#ifndef QCP_LLVM_EMITTER_H
#define QCP_LLVM_EMITTER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "stringpool.h"
// ---------------------------------------------------------------------------
#include <iostream>
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
template <typename _EmitterT>
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
   using TY = type::Type<LLVMEmitter>;
   using value_t = std::variant<ssa_t*, const_t*, iconst_t*>;
   using const_or_iconst_t = std::variant<const_t*, iconst_t*>;

   static constexpr bool CHAR_HAS_16_BIT = false;
   static constexpr bool CHAR_IS_SIGNED = true;
   static constexpr bool INT_HAS_64_BIT = false;
   static constexpr bool LONG_HAS_64_BIT = true;

   LLVMEmitter() : Mod{new llvm::Module("qcp", Ctx)}, Builder{Ctx} {}
   ~LLVMEmitter() { delete Mod; }

   void dumpToFile(const std::string& filename) {
      std::error_code EC;
      llvm::raw_fd_ostream OS(filename, EC);
      Mod->print(OS, nullptr);
   }

   void dumpToStdout() {
      Mod->print(llvm::outs(), nullptr);
   }

   unsigned long long getIntegerValue(const_t* c) {
      return static_cast<unsigned long long>(static_cast<llvm::ConstantInt*>(c)->getZExtValue());
   }

   iconst_t* sizeOf(TY ty);

   ty_t* emitVoidTy();

   ty_t* emitIntTy(unsigned bits);

   ty_t* emitFloatTy();

   ty_t* emitDoubleTy();

   ty_t* emitPtrTo(TY ty);

   ty_t* emitArrayTy(TY ty, iconst_t* size);

   ssa_t* emitUndef();

   ssa_t* emitPoison();

   // todo: change vector to iterator
   ty_t* emitFnTy(TY retTy, std::vector<TY> argTys, bool isVarArg);

   ssa_t* emitGlobalVar(TY ty, Ident name = Ident());

   void setInitValueGlobalVar(ssa_t* val, const_or_iconst_t init);

   fn_t* emitFnProto(TY fnTy, Ident name = Ident());

   bb_t* emitFn(fn_t* fnProto);

   bool isFnProto(fn_t* fn);

   ssa_t* getParam(fn_t* fn, unsigned idx);

   bb_t* emitBB(fn_t* fn, bb_t* insertBefore = nullptr, Ident name = Ident());

   const_or_iconst_t emitConst(TY ty, long value);

   ssa_t* emitLocalVar(fn_t* fn, bb_t* entry, TY ty, Ident name = Ident(), bool insertAtBegin = false /* TODO: this is only here so that the output is identical to clang but not necessary */);

   void setInitValueLocalVar(fn_t* fn, TY ty, ssa_t* val, value_t value);

   ssa_t* emitAlloca(bb_t* bb, TY ty, ssa_t* size, Ident name = Ident());

   ssa_t* emitLoad(bb_t* bb, TY ty, ssa_t* ptr, Ident name = Ident());

   void emitStore(bb_t* bb, TY ty, value_t value, ssa_t* ptr);

   ssa_t* emitJump(bb_t* bb, bb_t* target);

   ssa_t* emitBranch(bb_t* bb, bb_t* trueBB, bb_t* falseBB, value_t cond);

   ssa_t* emitRet(bb_t* bb, value_t value);

   phi_t* emitPhi(bb_t* bb, TY ty, Ident name = Ident());

   void addIncoming(phi_t* phi, ssa_t* value, bb_t* bb, Ident name = Ident());

   ssa_t* emitBinOp(bb_t* bb, TY ty, op::Kind kind, value_t lhs, value_t rhs, ssa_t* dest = nullptr, Ident name = Ident());

   const_or_iconst_t emitConstBinOp(bb_t* bb, TY ty, op::Kind kind, const_or_iconst_t lhs, const_or_iconst_t rhs, Ident name = Ident());

   ssa_t* emitIncDecOp(bb_t* bb, TY ty, op::Kind kind, ssa_t* operand, Ident name = Ident());

   ssa_t* emitNeg(bb_t* bb, TY ty, ssa_t* operand, Ident name = Ident());

   const_or_iconst_t emitConstNeg(bb_t* bb, TY ty, const_or_iconst_t operand, Ident name = Ident());

   ssa_t* emitBWNeg(bb_t* bb, TY ty, ssa_t* operand, Ident name = Ident());

   const_or_iconst_t emitConstBWNeg(bb_t* bb, TY ty, const_or_iconst_t operand, Ident name = Ident());

   const_or_iconst_t emitConstCast(bb_t* bb, TY fromTy, const_or_iconst_t val, TY toTy, qcp::type::Cast cast);

   ssa_t* emitCast(bb_t* bb, TY fromTy, ssa_t* val, TY toTy, qcp::type::Cast cast);

   ssa_t* emitCall(bb_t* bb, fn_t* fn, const std::vector<value_t>& args, Ident name = Ident());

   ssa_t* emitCall(bb_t* bb, TY fnTy, ssa_t* fnPtr, const std::vector<value_t>& args, Ident name = Ident());

   ssa_t* emitGEP(bb_t* bb, TY ty, value_t ptr, value_t idx, Ident name = Ident());

   sw_t* emitSwitch(bb_t* bb, value_t value);

   void addSwitchCase(sw_t* sw, iconst_t* value, bb_t* target);

   void addSwitchDefault(sw_t* sw, bb_t* target);

   private:
   ssa_t* emitCall(bb_t* bb, llvm::FunctionCallee fn, const std::vector<value_t>& args, Ident name = Ident());
   ssa_t* emitAllocaImpl(bb_t* bb, TY ty, ssa_t* size, Ident name, bool insertAtBegin);

   llvm::LLVMContext Ctx;
   llvm::Module* Mod;
   llvm::IRBuilder<> Builder;
};
// ---------------------------------------------------------------------------
} // namespace emitter
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_LLVM_EMITTER_H