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

InstGroups TimingSource::instGroup(Instruction* I)
{
   Type* T = I->getType();
   unsigned Op = I->getOpcode();
   unsigned bit,op;
   auto e = std::out_of_range("no group for this instruction");

   if(T->isIntegerTy(32)) bit = Integer;
   else if(T->isIntegerTy(64)) bit = I64;
   else if(T->isFloatTy()) bit = Float;
   else if(T->isDoubleTy()) bit = Double;
   else return Last;

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
      default: return Last;
   }

   return static_cast<InstGroups>(bit|op);
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
