/* 
 * the file provides some useful value helper functions
 * author: xiehuc@gmail.com 
 */

#include <llvm/IR/Value.h>
#include <llvm/IR/GlobalVariable.h>

namespace lle{
   llvm::Value* castoff(llvm::Value*);
   llvm::GlobalVariable* access_global_variable(llvm::Instruction *I);
}
