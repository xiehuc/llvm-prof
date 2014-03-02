#include "ValueProfiling.h"
#include "ProfilingUtils.h"

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

char ValueProfiler::ID = 0;

ValueProfiler::ValueProfiler():ModulePass(ID)
{
	numTrapedValues = 0;
}

Value* ValueProfiler::insertValueTrap(BasicBlock* Header,Value* v)
{
	Module* M = Header->getParent()->getParent();
	LLVMContext& Context = v->getContext();
	Type* Int64Ty = Type::getInt64Ty(Context);
	BasicBlock::iterator InsertPos = Header->getTerminator();
	while (isa<AllocaInst>(InsertPos)) ++InsertPos;


	Constant* FuncEntry = M->getOrInsertFunction("llvm_profiling_trap_value",
			Type::getVoidTy(Context),
			Type::getInt32Ty(Context), 
			Int64Ty,NULL);
	std::vector<Value* > Args(2);
	Args[0] = Constant::getIntegerValue(Type::getInt32Ty(Context),APInt(32,numTrapedValues));
	Args[1] = Constant::getNullValue(Int64Ty);
	++numTrapedValues;
	if(v->getType()!=Type::getInt32Ty(Context)){
		CastInst::CastOps opcode = CastInst::getCastOpcode(v, true, Type::getInt32Ty(Context), true);
		Args[1] = CastInst::Create(opcode, v, Int64Ty, "cycle.cast", InsertPos);
		outs()<<*Header<<"\n";
	}else{
		Args[1] = v;
	}
	CallInst* RET = CallInst::Create(FuncEntry, Args, "", InsertPos);
	outs()<<*RET<<"\n";
	return RET;
}

bool ValueProfiler::runOnModule(Module& M)
{
	Function* Main = M.getFunction("main");
	Type*ATy = ArrayType::get(Type::getInt32Ty(M.getContext()),numTrapedValues);
	GlobalVariable* Counters = new GlobalVariable(M, ATy, false,
			GlobalVariable::InternalLinkage, Constant::getNullValue(ATy),
			"ValueProfCounters");
	InsertProfilingInitCall(Main, "llvm_start_value_profiling",Counters);
	return true;
}
