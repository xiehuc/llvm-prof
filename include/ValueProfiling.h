#ifndef LLVM_VALUE_PROFILER_H_H
#define LLVM_VALUE_PROFILER_H_H
#include <llvm/Pass.h>
#include <llvm/IR/Value.h>
#include <vector>

namespace llvm{
	class ValueProfiler: public ModulePass
	{
		GlobalVariable* Counters;
		int numTrapedValues;
		typedef std::vector<std::pair<int,BasicBlock*> > ConstantTrapTy;
		ConstantTrapTy ConstantTraps;
		public:
		static char ID;
		ValueProfiler();
		//insert a value profiler, return inserted call instruction
		Instruction* insertValueTrap(Value* value,BasicBlock* InsertTail);
		Instruction* insertValueTrap(Value* value,Instruction* InsertBefore);
		//insert initial profiling for module
		bool runOnModule(Module& M);
		int getNumTrapedValues() const { return numTrapedValues;}
		virtual const char* getPassName() const{
			return "ValueProfiler";
		}
	};
}
#endif
