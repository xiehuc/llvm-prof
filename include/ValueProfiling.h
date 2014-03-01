#include <llvm/Pass.h>

namespace llvm{
	class ValueProfiler: public ModulePass
	{
		bool runOnModule(Module& M);
		int numTrapedValues;
		public:
		static char ID;
		ValueProfiler();
		void insertValueTrap();
		virtual const char* getPassName() const{
			return "ValueProfiler";
		}
	};
}
