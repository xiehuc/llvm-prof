/* 
 * the file provides some useful value helper functions
 * author: xiehuc@gmail.com 
 */

#include <llvm/IR/Value.h>
#include <llvm/IR/GlobalVariable.h>

namespace lle{
	/**
    * remove cast instruction for a value
	 * because cast means the original value and the returned value is
	 * semanticly equal */
   llvm::Value* castoff(llvm::Value*);
   /**
    * access whether a Instruction is reference global variable. 
    * if is, return the global variable
    * else return NULL 
    * it would deep into getelementptr, cast, load instruction
    * if it found finally depends Argument, it would search all call statement,
    * to check Argument is a global variable */
   llvm::GlobalVariable* access_global_variable(llvm::Instruction *I);
}
