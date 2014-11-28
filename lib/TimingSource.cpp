#include "TimingSource.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unordered_map>

using namespace llvm;

static SmallVector<std::string,NumGroups> InstGroupNames;

StringRef TimingSource::getName(InstGroups IG)
{
   if(InstGroupNames.size() == 0){
      InstGroupNames.resize(NumGroups);
      auto& n = InstGroupNames;
      std::vector<std::pair<InstGroups,StringRef>> bits = {
         {Integer, "I"}, {I64, "I64"}, {Float,"F"}, {Double,"D"}
      }, ops = {{Add,"Add"},{Mul, "Mul"}, {Div, "Div"}, {Mod, "Mod"}};
      for(auto bit : bits){
         for(auto op : ops)
            n[bit.first|op.first] = (bit.second+op.second).str();
      }
   }
   return InstGroupNames[IG];
}

void TimingSource::initArray(Module* M, Type *EleTy, bool force)
{
   if(cpu_times == NULL || force){
      this->EleTy = EleTy;
      Type* ATy = ArrayType::get(EleTy, NumGroups);
      cpu_times = new GlobalVariable(*M, ATy, false, GlobalValue::InternalLinkage, Constant::getNullValue(ATy), "cpuTime");
   }
}

InstGroups TimingSource::instGroup(Instruction* I) const throw(std::out_of_range)
{
   Type* T = I->getType();
   unsigned Op = I->getOpcode();
   unsigned bit,op;
   auto e = std::out_of_range("no group for this instruction");

   if(T->isIntegerTy(32)) bit = Integer;
   else if(T->isIntegerTy(64)) bit = I64;
   else if(T->isFloatTy()) bit = Float;
   else if(T->isDoubleTy()) bit = Double;
   else throw e;

   switch(Op){
      case Instruction::FAdd:
      case Instruction::FSub:
      case Instruction::Sub:
      case Instruction::Add: op = Add;break;
      case Instruction::FMul:
      case Instruction::Mul: op = Mul;break;
      case Instruction::FDiv:
      case Instruction::UDiv:
      case Instruction::SDiv: op = Div;break;
      case Instruction::FRem:
      case Instruction::SRem:
      case Instruction::URem: op = Mod;break;
      default: throw e;
   }

   return static_cast<InstGroups>(bit|op);
}

llvm::Value* TimingSource::createLoad(IRBuilder<> &Builder, InstGroups IG)
{
   LLVMContext& C = EleTy->getContext();
   Type* I32Ty = Type::getInt32Ty(C);
   Value* Arg[2] = {ConstantInt::get(I32Ty,0), ConstantInt::get(I32Ty, IG)};
   Value* P = Builder.CreateGEP(cpu_times, Arg);
   return Builder.CreateLoad(P, getName(IG));
}

void TimingSource::insert_load_source(Function& F)
{
   LLVMContext& C = F.getContext();
   Module* M = F.getParent();
   Type* I32Ty = Type::getInt32Ty(C);
   Constant* LoadF = M->getOrInsertFunction("load_timing_source", Type::getVoidTy(C), Type::getInt32PtrTy(C), NULL);
   
   llvm::Constant* idxs[] = {ConstantInt::get(I32Ty, 0), ConstantInt::get(I32Ty, 0)};
   llvm::Value* args[] = {ConstantExpr::getGetElementPtr(cpu_times, idxs)};
   CallInst::Create(LoadF, args, "", F.getEntryBlock().getFirstInsertionPt());
}

double TimingSource::count(BasicBlock& BB)
{
   double counts = 0.0;

   for(auto& I : BB){
      try{
         counts += unit_times[instGroup(&I)];
      }catch(std::out_of_range e){
         //ignore exception
      }
   }
   return counts;
}

#define eq(a,b) (strcmp(a,b)==0)
void TimingSource::load_lmbench(const char* file, double* cpu_times)
{
   FILE* f = fopen(file,"r");
   if(f == NULL){
      fprintf(stderr, "Could not open %s file: %s", file, strerror(errno));
      exit(-1);
   }

   double nanosec;
   char bits[48], ops[48];
   char line[512];
   while(fgets(line,sizeof(line),f)){
      unsigned bit,op;
      if(sscanf(line, "%s %[^:]: %lf", bits, ops, &nanosec)<3) continue;
      bit = eq(bits,"integer")?Integer:eq(bits,"int64")?I64:eq(bits,"float")?Float:eq(bits,"double")?Double:-1U;
      op = eq(ops,"add")?Add:eq(ops,"mul")?Mul:eq(ops,"div")?Div:eq(ops,"mod")?Mod:-1U;
      if(bit == -1U || op == -1U) continue;
      cpu_times[bit|op] = nanosec;
   }
}
