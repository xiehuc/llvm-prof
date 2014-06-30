#include "ValueUtils.h"

#include <llvm/IR/Argument.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

using namespace lle;
using namespace llvm;

Value* lle::castoff(Value* v)
{
   if(ConstantExpr* CE = dyn_cast<ConstantExpr>(v))
      v = CE->getAsInstruction();

	if(CastInst* CI = dyn_cast<CastInst>(v)){
		return castoff(CI->getOperand(0));
	}else
		return v;
}

static GlobalVariable* parameter_access_global_variable(Argument* Arg)
{
   Function* F = Arg->getParent();
   for(Function::use_iterator U = F->use_begin(), E = F->use_end(); U!=E;++U){
      CallInst* CI = dyn_cast<CallInst>(*U);
      if(!CI) continue;
      Value* T = CI->getArgOperand(Arg->getArgNo());
      GlobalVariable* G = NULL;
      if((G = dyn_cast<GlobalVariable>(T)))
         return G;

      if(Argument* A = dyn_cast<Argument>(T))
         G = parameter_access_global_variable(A);
      else if(Instruction* I = dyn_cast<Instruction>(T))
         G = access_global_variable(I);

      if(G) return G;
   }
   return NULL;
}
/**
 * access whether a Instruction is reference global variable. 
 * if is, return the global variable
 * else return NULL
 */
GlobalVariable* lle::access_global_variable(Instruction *I)
{
   Value* U = NULL;
   if(StoreInst* SI = dyn_cast<StoreInst>(I))
      U = SI->getPointerOperand();
   else if(LoadInst* LI = dyn_cast<LoadInst>(I))
      U = LI->getPointerOperand();
   else if(isa<CastInst>(I))
      U = castoff(I);
   else if(GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(I))
      U = GEP->getPointerOperand();
   else return NULL;

   if(isa<ConstantExpr>(U) )
      U = castoff(U);
   if(Instruction* T = dyn_cast<Instruction>(U))
      return access_global_variable(T);
   else if(Argument* Arg = dyn_cast<Argument>(U))
      return parameter_access_global_variable(Arg);
   else 
      return dyn_cast<GlobalVariable>(U);
}
