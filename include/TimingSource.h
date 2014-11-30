#ifndef LLVM_TIMING_SOURCE_H_H
#define LLVM_TIMING_SOURCE_H_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/GlobalVariable.h>

/* a timing source is used to count inst types in a basicblock */

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
   /* return the IG's text representation */
   static llvm::StringRef getName(InstGroups IG);
   /* return what group of I belongs to.
    * if unknow , return InstGroups::Last
    */
   static InstGroups instGroup(llvm::Instruction* I);

   static void load_lmbench(const char* File, double* Data);

   TimingSource():cpu_times(NULL), EleTy(NULL) { unit_times.resize(NumGroups); }
   //=================== 立即模式 ==========================//
   //在该模式中, 立即从文件中读取Timing数据并计算//
   /* init unit_times with nanoseconds unit.
    * @example:
    *    init(std::bind(TimingSource::load_lmbench, "lmbench.log", _1));
    */
   void init(std::function<void(double* Input)> func){
      func(unit_times.data());
   }
   /* return BB's inst timing count using unit_times */
   double count(llvm::BasicBlock& BB);

   //================== 延迟模式 ============================//
   //在该模式中, 通过插入全局数组和main函数中的load_timing_source函数, 编译成
   //exe文件的时候才读取Timing数据, 将被废弃
   /* initial cpu_times GlobalVariable, when cpu_times is NULL.
    * if force, always reinitial cpu_times. it would cause memory leak, but
    * llvm take over it. so don't worry.
    */
   void init(llvm::Module* M, llvm::Type* EleTy, bool force = false);
   llvm::Value* createLoad(llvm::IRBuilder<>& Builder, InstGroups IG);
   void insert_load_source(llvm::Function& F);
};
}

#endif
