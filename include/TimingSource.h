#ifndef LLVM_TIMING_SOURCE_H_H
#define LLVM_TIMING_SOURCE_H_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/GlobalVariable.h>

/* a timing source is used to count inst types in a basicblock */

namespace llvm{
enum InstGroups {
   Integer = 0, I64 = 1, Float = 2, Double = 3, // Ntype
   Add = 0<<2, Mul = 1<<2, Div = 2<<2, Mod = 3<<2, // Method
   Last = Double|Mod, // Method | Ntype
   NumGroups
};
class TimingSource{
   llvm::SmallVector<double, NumGroups> unit_times;
   public:
   /* return the IG's text representation */
   static llvm::StringRef getName(InstGroups IG);
   /* return what group of I belongs to.
    * if unknow , return InstGroups::Last
    */
   static InstGroups instGroup(llvm::Instruction* I);

   static void load_lmbench(const char* File, double* Data);

   TimingSource(){ unit_times.resize(NumGroups); }
   //在该模式中, 立即从文件中读取Timing数据并计算//
   /* init unit_times with nanoseconds unit.
    * @example:
    *    init(std::bind(TimingSource::load_lmbench, "lmbench.log", _1));
    */
   void init(std::function<void(double* Input)> func){
      func(unit_times.data());
   }
   void init(std::initializer_list<double> list) {
      unit_times.insert(unit_times.begin(), list.begin(), list.end());
   }
   double get(InstGroups G){
      return unit_times[G];
   }
   /* return BB's inst timing count using unit_times */
   double count(llvm::BasicBlock& BB);
};
}

#endif
