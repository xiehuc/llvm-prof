#ifndef LLVM_VALUE_UTILS_H_H
#define LLVM_VALUE_UTILS_H_H
/* 
 * the file provides some useful value helper functions
 * author: xiehuc@gmail.com 
 */

namespace llvm{
   class Value;
   class GlobalVariable;
   class Instruction;
   class CallInst;
}

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

   /**
    * return 0 means failed (because no mpi inst's count idx is 0)
    * return >0 means the index of count param 
    */
   unsigned get_mpi_count_idx(const llvm::CallInst*);

   enum MPICategoryType {
      MPI_CT_P2P      = 0, 
      MPI_CT_REDUCE   = 1, // collect
      MPI_CT_REDUCE2  = 2, // double collect
      MPI_CT_NSIDES   = 3  // n times communicate
   };
   /**
    * return mpi collection category
    * if unknow --- throw std::out_of_range
    */
   MPICategoryType get_mpi_collection(const llvm::CallInst*) noexcept(false);
}
#endif
