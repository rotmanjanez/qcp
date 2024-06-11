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
class Ident; // forward declaration
// ---------------------------------------------------------------------------
namespace emitter {
// ---------------------------------------------------------------------------
class LLVMEmitter {
   public:
   using ssa_t = llvm::Value;
   using const_t = llvm::Constant;
   using phi_t = llvm::PHINode;
   using bb_t = llvm::BasicBlock;
   using ty_t = llvm::Type;
   using fn_t = llvm::Function;

   // containes a backref to the type with emitterType() method
   // todo: make const
   using TY = type::Type<LLVMEmitter>;

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

   ty_t* emitVoidTy();

   ty_t* emitIntTy(unsigned bits);

   ty_t* emitFloatTy();

   ty_t* emitDoubleTy();

   ty_t* emitPtrTo(TY ty);

   ty_t* emitArrayTy(TY ty, std::size_t size);

   ssa_t* emitUndef();

   ssa_t* emitPoison();

   ty_t* emitFnTy(TY retTy, std::vector<TY> argTys, bool isVarArg);

   ssa_t* emitGlobalVar(TY ty, Ident name);

   void setInitValueGlobalVar(ssa_t* val, const_t* init);

   fn_t* emitFnProto(Ident name, TY fnTy);

   bb_t* emitFn(fn_t* fnProto);

   bool isFnProto(fn_t* fn);

   ssa_t* getParam(fn_t* fn, unsigned idx);

   bb_t* emitBB(Ident name, fn_t* fn, bb_t* insertBefore = nullptr);

   const_t* emitConst(TY ty, Ident name, long value);

   ssa_t* emitLocalVar(fn_t* fn, bb_t* bb, TY ty, Ident name, ssa_t* init = nullptr);

   void setInitValueLocalVar(fn_t* fn, ssa_t* val, ssa_t* value);

   ssa_t* emitAlloca(bb_t* bb, TY ty, Ident name);

   ssa_t* emitLoad(bb_t* bb, Ident name, TY ty, ssa_t* ptr);

   ssa_t* emitStore(bb_t* bb, ssa_t* value, ssa_t* ptr);

   ssa_t* emitJump(bb_t* bb, bb_t* target);

   ssa_t* emitBranch(bb_t* bb, bb_t* trueBB, bb_t* falseBB, ssa_t* cond);

   ssa_t* emitRet(bb_t* bb, ssa_t* value);

   phi_t* emitPhi(bb_t* bb, Ident name, TY ty);

   void addIncoming(Ident name, phi_t* phi, ssa_t* value, bb_t* bb);

   ssa_t* emitBinOp(bb_t* bb, TY ty, Ident name, op::Kind kind, ssa_t* lhs, ssa_t* rhs, ssa_t* dest = nullptr);

   ssa_t* emitUnOp(bb_t* bb, TY ty, Ident name, op::Kind kind, ssa_t* operand);

   ssa_t* emitCast(bb_t* bb, ssa_t* val, TY toTy, qcp::type::Cast cast);

   ssa_t* emitCall(bb_t* bb, Ident name, fn_t* fn, const std::vector<ssa_t*>& args);

   ssa_t* emitCall(bb_t* bb, Ident name, TY fnTy, ssa_t* fnPtr, const std::vector<ssa_t*>& args);

   private:
   llvm::LLVMContext Ctx;
   llvm::Module* Mod;
   llvm::IRBuilder<> Builder;
};
// ---------------------------------------------------------------------------
} // namespace emitter
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_LLVM_EMITTER_H