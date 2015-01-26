#include "TimingSource.h"
#include <llvm/Support/raw_ostream.h>
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

#include "ValueUtils.h"

using namespace llvm;

static 
std::unordered_map<std::string, unsigned> MpiSpec = 
{
   {"mpi_allreduce_" , 2} , // == 0 --- p2p
   {"mpi_bcast_"     , 1} , // == 1 --- collect
   {"mpi_reduce_"    , 1} , // == 2 --- full collect
   {"mpi_send_"      , 0} , 
   {"mpi_recv_"      , 0} , 
   {"mpi_isend_"     , 0} , 
   {"mpi_irecv_"     , 0}
};

static unsigned MpiType[128];

static int mpi_init_type(unsigned* DT)
{
   memset(DT, 0, sizeof(MpiType));
#include "datatype.h"
   return 0;
}

int mpi_type_initialize = mpi_init_type(MpiType);

static unsigned get_datatype_size(StringRef Name, const CallInst& I)
{
   GlobalVariable* datatype;
   if(MpiSpec[Name] == 0)
      datatype = dyn_cast<GlobalVariable>(I.getArgOperand(2)); // this is p2p communication
   else 
      datatype = dyn_cast<GlobalVariable>(I.getArgOperand(3)); // this is collect communication
   if(datatype == NULL) return 0;
   auto CI = dyn_cast<ConstantInt>(datatype->getInitializer());
   if(CI == NULL) return 0;
   return MpiType[CI->getZExtValue()]; // datatype -> sizeof
}

StringRef LmbenchTiming::getName(EnumTy IG)
{
   static SmallVector<std::string,NumGroups> InstGroupNames;
   if(InstGroupNames.size() == 0){
      InstGroupNames.resize(NumGroups);
      auto& n = InstGroupNames;
      std::vector<std::pair<EnumTy,StringRef>> bits = {
         {Integer, "I"}, {I64, "I64"}, {Float,"F"}, {Double,"D"}
      }, ops = {{Add,"Add"},{Mul, "Mul"}, {Div, "Div"}, {Mod, "Mod"}};
      for(auto bit : bits){
         for(auto op : ops)
            n[bit.first|op.first] = (bit.second+op.second).str();
      }
   }
   return InstGroupNames[IG];
}

LmbenchTiming::EnumTy LmbenchTiming::classify(Instruction* I)
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

   return static_cast<EnumTy>(bit|op);
}

LmbenchTiming::LmbenchTiming():
   TimingSource(Lmbench, NumGroups), 
   T(params) {
   file_initializer = load_lmbench;
   char* REnv = getenv("MPI_SIZE");
   if(REnv == NULL){
      errs()<<"please set environment MPI_SIZE same as when profiling\n";
      exit(-1);
   }
   this->R = atoi(REnv);
}
double LmbenchTiming::count(Instruction& I)
{
   return params[classify(&I)];
}

double LmbenchTiming::count(BasicBlock& BB)
{
   double counts = 0.0;

   for(auto& I : BB){
      try{
         counts += params[classify(&I)];
      }catch(std::out_of_range e){
         //ignore exception
      }
   }
   return counts;
}

double LmbenchTiming::count(const Instruction& I, double bfreq, double count)
{
   const CallInst* CI = dyn_cast<CallInst>(&I);
   if(CI == NULL) return 0.0;
   Function* F = dyn_cast<Function>(lle::castoff(const_cast<Value*>(CI->getCalledValue())));
   if(F == NULL) return 0.0;
   StringRef FName = F->getName();
   auto Spec = MpiSpec.find(FName);
   if(Spec == MpiSpec.end()) return 0.0;
   unsigned C = Spec->second;
   size_t D = count * get_datatype_size(FName, *CI);
   if(C == 0){
      return bfreq*get(SOCK_LATENCY) + D/get(SOCK_BANDWIDTH);
   }else
      return bfreq*get(SOCK_LATENCY) + C*D*log(R)/get(SOCK_BANDWIDTH);
}

#define eq(a,b) (strcmp(a,b)==0)
void LmbenchTiming::load_lmbench(const char* file, double* cpu_times)
{
   FILE* f = fopen(file,"r");
   if(f == NULL){
      fprintf(stderr, "Could not open %s file: %s", file, strerror(errno));
      exit(-1);
   }

   double nanosec, mbpsec, microsec;
   char bits[48], ops[48];
   char line[512];
   while(fgets(line,sizeof(line),f)){
      unsigned bit,op;
      if(sscanf(line, "AF_UNIX sock stream latency: %lf microseconds",
               &microsec)==1) {
         cpu_times[SOCK_LATENCY] = microsec * 1000; // ms -> ns
      }else if(sscanf(line, "AF_UNIX sock stream bandwidth: %lf MB/sec",
               &mbpsec)==1) {
         cpu_times[SOCK_BANDWIDTH] = mbpsec * 1024*1024/1000000000; // MB/sec -> B/ns
      } else if(sscanf(line, "%s %[^:]: %lf nanoseconds", bits, ops, &nanosec)==3){
         bit = eq(bits,"integer")?Integer:eq(bits,"int64")?I64:eq(bits,"float")?Float:eq(bits,"double")?Double:-1U;
         op = eq(ops,"add")?Add:eq(ops,"mul")?Mul:eq(ops,"div")?Div:eq(ops,"mod")?Mod:-1U;
         if(bit == -1U || op == -1U) continue;
         cpu_times[bit|op] = nanosec;
      }
   }
}

