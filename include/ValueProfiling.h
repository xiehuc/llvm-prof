#ifndef LLVM_VALUE_PROFILER_H_H
#define LLVM_VALUE_PROFILER_H_H
#include <llvm/Pass.h>
#include <llvm/IR/Value.h>

namespace llvm{
	class ValueProfiler: public ModulePass
	{
		int numTrapedValues;
		public:
		static char ID;
		ValueProfiler();
		Value* insertValueTrap(BasicBlock* Header,Value* value);
		bool runOnModule(Module& M);
		virtual const char* getPassName() const{
			return "ValueProfiler";
		}
	};
}
#endif
