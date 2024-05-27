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

   static constexpr unsigned CHAR_BITS = 8;
   static constexpr unsigned SHORT_BITS = 16;
   static constexpr unsigned INT_BITS = 32;
   static constexpr unsigned LONG_BITS = 64;
   static constexpr unsigned LONG_LONG_BITS = 64;

   LLVMEmitter() : Mod{new llvm::Module("qcp", Ctx)}, Builder{Ctx} {}
   ~LLVMEmitter() {
      delete Mod;
   }

   void dumpToFile(const std::string& filename) {
      std::error_code EC;
      llvm::raw_fd_ostream OS(filename, EC);
      Mod->print(OS, nullptr);
   }

   void dumpToStdout() {
      Mod->print(llvm::outs(), nullptr);
   }

   ty_t* emitPtrTo(ty_t* ty);

   ty_t* emitIntTy(unsigned bits);

   ty_t* emitFnTy(ty_t* retTy, std::vector<ty_t*> argTys);

   ssa_t* emitGlobalVar(ty_t* ty, Ident name, const_t* init);

   void setInitValueGlobalVar(ssa_t* val, const_t* init);

   fn_t* emitFnProto(Ident name, ty_t* fnTy);

   bb_t* emitFn(fn_t* fnProto);

   bool isFnProto(fn_t* fn);

   ssa_t* getParam(fn_t* fn, unsigned idx);

   bb_t* emitBB(Ident name, fn_t* fn);

   ssa_t* emitConst(bb_t* bb, ty_t* ty, Ident name, long value);

   ssa_t* emitLocalVar(fn_t* fn, bb_t* bb, ty_t* ty, Ident name, ssa_t* init);

   ssa_t* emitAlloca(bb_t* bb, ty_t* ty, Ident name);

   ssa_t* emitLoad(bb_t* bb, Ident name, ty_t* ty, ssa_t* ptr);

   ssa_t* emitStore(bb_t* bb, ssa_t* value, ssa_t* ptr);

   ssa_t* emitJump(bb_t* bb, bb_t* target);

   ssa_t* emitBranch(bb_t* bb, bb_t* trueBB, bb_t* falseBB, ssa_t* cond);

   ssa_t* emitRet(bb_t* bb, ssa_t* value);

   phi_t* emitPhi(bb_t* bb, Ident name, ty_t* ty);

   void addIncoming(Ident name, phi_t* phi, ssa_t* value, bb_t* bb);

   ssa_t* emitBinOp(bb_t* bb, Ident name, op::Kind kind, ssa_t* lhs, ssa_t* rhs);

   ssa_t* emitUnOp(bb_t* bb, Ident name, op::Kind kind, ssa_t* operand);

   ssa_t* emitCall(bb_t* bb, Ident name, fn_t* fn, const std::vector<ssa_t*>& args);

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