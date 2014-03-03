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

ValueProfiler::ValueProfiler():ModulePass(ID)
{
	numTrapedValues = 0;
	Counters = NULL;
}

template<typename InsertTy>
static Instruction* insertValueTrap(Value* v,Module* M,int numTrapedValues,InsertTy InsertPos)
{
	LLVMContext& Context = v->getContext();
	Type* Int32Ty = Type::getInt32Ty(Context);

	Constant* FuncEntry = M->getOrInsertFunction( "llvm_profiling_trap_value",
			Type::getVoidTy(Context), Int32Ty, Int32Ty, Int32Ty, NULL);
	std::vector<Value* > Args(3);
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

Instruction* ValueProfiler::insertValueTrap(Value* v, BasicBlock* InsertTail)
{
	/*if(isa<Constant>(v)){
		pair<int,BasicBlock*> Store(numTrapedValues++,InsertTail);
		ConstantTraps.push_back(Store);
		return NULL;
	}else*/
		return ::insertValueTrap(v, InsertTail->getParent()->getParent(),
				numTrapedValues++,InsertTail);
}

Instruction* ValueProfiler::insertValueTrap(Value* v,Instruction* InsertBefore)
{
	//ignore constant value to reduce memory pressure
	/*if(isa<Constant>(v)){
		pair<int,BasicBlock*> Store(numTrapedValues++,InsertBefore->getParent());
		ConstantTraps.push_back(Store);
		return NULL;
	}else*/
		return ::insertValueTrap(v,
				InsertBefore->getParent()->getParent()->getParent(),
				numTrapedValues++, InsertBefore);
}

bool ValueProfiler::runOnModule(Module& M)
{
	Function* Main = M.getFunction("main");
	Type*ATy = ArrayType::get(Type::getInt32Ty(M.getContext()),numTrapedValues);
	Counters = new GlobalVariable(M, ATy, false,
			GlobalVariable::InternalLinkage, Constant::getNullValue(ATy),
			"ValueProfCounters");
	InsertProfilingInitCall(Main, "llvm_start_value_profiling",Counters);
	//for(ConstantTrapTy::iterator I = ConstantTraps.begin(),E = ConstantTraps.end();I!=E;I++)
	//	IncrementCounterInBlock(I->second, I->first, Counters);
	return true;
}
