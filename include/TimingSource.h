#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/GlobalVariable.h>

namespace llvm{
enum InstGroups {
   Integer = 0, I64 = 1, Float = 2, Double = 3,
   Add = 0<<2, Mul = 1<<2, Div = 2<<2, Mod = 3<<2,
   Last = Double|Mod,
   NumGroups
};
class TimingSource{
   llvm::GlobalVariable* cpu_times;
   llvm::SmallVector<double, NumGroups> unit_times;
   llvm::Type* EleTy;
   public:
   static llvm::StringRef getName(InstGroups IG);
   static void load_lmbench(const char* File, double* Data);

   TimingSource():cpu_times(NULL), EleTy(NULL) { unit_times.resize(NumGroups); }
   void initArray(llvm::Module* M, llvm::Type* EleTy, bool force = false);
   void init(std::function<void(double* Input)> func){
      func(unit_times.data());
   }
   InstGroups instGroup(llvm::Instruction* I) const throw(std::out_of_range);
   llvm::Value* createLoad(llvm::IRBuilder<>& Builder, InstGroups IG);
   void insert_load_source(llvm::Function& F);
   double count(llvm::BasicBlock& BB);
};
}
