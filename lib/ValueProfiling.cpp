#include "ValueProfiling.h"
#include <llvm/IR/Module.h>

using namespace llvm;

bool ValueProfiler::runOnModule(Module& M)
{
	Function* Main = M.getFunction("main");
	Type*ATy = ArrayType::get(Type::getInt32Ty(M.getContext()),numTrapedValues);
	GlobalVariable* Counters = new GlobalVariable(M, ATy, false,
			GlobalVariable::InternalLinkage, Constant::getNullValue(ATy),
			"ValueProfCounters");
}
