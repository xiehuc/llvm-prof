#include "preheader.h"
#include "ValueProfiling.h"
#include "ProfilingUtils.h"

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace std;

char ValueProfiler::ID = 0;
int ValueProfiler::numTrapedValues = 0;
bool ValueProfiler::avaliable = false;

static RegisterPass<ValueProfiler> X("insert-value-profiling", "Insert Value Profiling into Module", false, true);

ValueProfiler::ValueProfiler():ModulePass(ID)
{
	Counters = NULL;
   avaliable = true;
}


template<typename InsertTy>
static Instruction* insertValueTrap(Value* v,Module* M,int numTrapedValues,InsertTy InsertPos)
{
	LLVMContext& Context = v->getContext();
	Type* Int32Ty = Type::getInt32Ty(Context);

	Constant* FuncEntry = M->getOrInsertFunction( "llvm_profiling_trap_value",
			Type::getVoidTy(Context), Int32Ty, Int32Ty, Int32Ty, NULL);
	Value* Args[3];
	Args[0] = Constant::getIntegerValue(Int32Ty,APInt(32,numTrapedValues));
	Args[1] = Constant::getNullValue(Int32Ty);
	Args[2] = isa<Constant>(v)?Constant::getIntegerValue(Int32Ty, APInt(32,1)):Constant::getNullValue(Int32Ty);
	if(v->getType()!=Int32Ty){
		CastInst::CastOps opcode = CastInst::getCastOpcode(v, true, Int32Ty, true);
		Args[1] = CastInst::Create(opcode, v, Int32Ty, "cycle.cast", InsertPos);
	}else{
		Args[1] = v;
	}
	return CallInst::Create(FuncEntry, Args, "", InsertPos);
}

Value* ValueProfiler::insertValueTrap(Value* v, BasicBlock* InsertTail)
{
   if(!avaliable) return v;
	return ::insertValueTrap(v, InsertTail->getParent()->getParent(),
			numTrapedValues++,InsertTail);
}

Value* ValueProfiler::insertValueTrap(Value* v,Instruction* InsertBefore)
{
   if(!avaliable) return v;
	return ::insertValueTrap(v,
			InsertBefore->getParent()->getParent()->getParent(),
			numTrapedValues++, InsertBefore);
}

bool ValueProfiler::runOnModule(Module& M)
{
   // empty traps, this is mannual insertion, we search in module
   if(numTrapedValues==0){
      for(Function& F : M)
         for(auto I = inst_begin(F), E = inst_end(F); I!=E; ++I){
            CallInst* CI = dyn_cast<CallInst>(&*I);
            if(CI == NULL) continue;
            Function* Called = CI->getCalledFunction();
            if(Called == NULL) continue;
            if(Called->getName() != "llvm_profiling_trap_value") continue;
            int idx = cast<ConstantInt>(CI->getArgOperand(0))->getSExtValue();
            numTrapedValues = max(numTrapedValues, idx+1);
         }
   }
	Function* Main = M.getFunction("main");
	Type*ATy = ArrayType::get(Type::getInt32Ty(M.getContext()),numTrapedValues);
	Counters = new GlobalVariable(M, ATy, false,
			GlobalVariable::InternalLinkage, Constant::getNullValue(ATy),
			"ValueProfCounters");
	InsertProfilingInitCall(Main, "llvm_start_value_profiling",Counters);
	return true;
}
