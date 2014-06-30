#include "ValueUtils.h"
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
   else return dyn_cast<GlobalVariable>(U);
}
