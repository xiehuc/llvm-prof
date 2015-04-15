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
#include <stdio.h>
#include <functional>

#include "FreeExpression.h"
#include "ValueUtils.h"

using namespace llvm;
#define IGN 3

static 
const std::map<StringRef, unsigned> MpiSpec = 
{
   {"mpi_allreduce_" , 2} , // == 0 --- p2p
   {"mpi_bcast_"     , 1} , // == 1 --- collect
   {"mpi_reduce_"    , 1} , // == 2 --- full collect
   {"mpi_send_"      , 0} , // == Ignore --- ignore
   {"mpi_recv_"      , 0} , 
   {"mpi_isend_"     , 0} , 
   {"mpi_irecv_"     , 0}
};

static 
std::vector<TimingSourceInfoEntry> TSIEntries;

static unsigned MpiType[128];

static int mpi_init_type(unsigned* DT)
{
   memset(DT, 0, sizeof(MpiType));
#include "datatype.h"
   return 0;
}
static int mpi_type_initialize = mpi_init_type(MpiType);

void TimingSource::Register_(const char* name, const char* desc, std::function<TimingSource*()>&& func)
{
   TimingSourceInfoEntry entry;
   entry.Name = name;
   entry.Desc = desc;
   entry.Creator = func;
   TSIEntries.push_back(std::move(entry));
}

static unsigned get_datatype_index(StringRef Name, const CallInst& I)
{
   GlobalVariable* datatype;
   try{
      if(MpiSpec.at(Name) == 0)
         datatype = dyn_cast<GlobalVariable>(I.getArgOperand(2)); // this is p2p communication
      else 
         datatype = dyn_cast<GlobalVariable>(I.getArgOperand(3)); // this is collect communication
      if(datatype == NULL) return 0;
   }catch(std::out_of_range e){
      return 0;
   }
   auto CI = dyn_cast<ConstantInt>(datatype->getInitializer());
   if(CI == NULL) return 0;
   return CI->getZExtValue(); // datatype -> sizeof
}

TimingSource* TimingSource::Construct(const StringRef Name)
{
  auto Found = std::find_if(
      TSIEntries.begin(), TSIEntries.end(),
      [Name](const TimingSourceInfoEntry &E) { return E.Name == Name; });
   if(Found == TSIEntries.end()) return NULL;
   else return Found->Creator();
}

const std::vector<TimingSourceInfoEntry>& TimingSource::Avail()
{
   return TSIEntries;
}

void TimingSource::print(raw_ostream& OS) const
{
   unsigned col = 1;
   for(auto i = params.begin(), e = params.end()-1 ; i!=e; ++i, ++col){
      OS<<*i<<",\t";
      if(col==5){
         OS<<"\n";
         col = 0;
      }
   }
   OS<<"\n\n";
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
      default: return NumGroups;
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
double LmbenchTiming::count(Instruction& I) const
{
   return params[classify(&I)];
}

double LmbenchTiming::count(BasicBlock& BB) const
{
   double counts = 0.0;

   for(auto& I : BB)
      counts += params[classify(&I)];
   return counts;
}

double LmbenchTiming::count(const Instruction& I, double bfreq, double count) const
{
   const CallInst* CI = dyn_cast<CallInst>(&I);
   if(CI == NULL) return 0.0;
   Function* F = dyn_cast<Function>(lle::castoff(const_cast<Value*>(CI->getCalledValue())));
   if(F == NULL) return 0.0;
   StringRef FName = F->getName();
   if(!FName.startswith("mpi_")) return 0.0;
   auto Spec = MpiSpec.find(FName);
   if(Spec == MpiSpec.end()){
      errs()<<"WARNNING: doesn't consider "<<FName<<" mpi call\n";
      return 0.0;
   }
   unsigned C = Spec->second;
   if(C == IGN) return 0.0;
   size_t D = get_datatype_index(FName, *CI);
   if(D == 0) errs()<<"WARNNING: doesn't consider MPI_datatype "<<D<<"\n";
   D = count * MpiType[D];
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
      } else if(sscanf(line, "%47s %47[^:]: %lf nanoseconds", bits, ops, &nanosec)==3){
         bit = eq(bits,"integer")?Integer:eq(bits,"int64")?I64:eq(bits,"float")?Float:eq(bits,"double")?Double:-1U;
         op = eq(ops,"add")?Add:eq(ops,"mul")?Mul:eq(ops,"div")?Div:eq(ops,"mod")?Mod:-1U;
         if(bit == -1U || op == -1U) continue;
         cpu_times[bit|op] = nanosec;
      }
   }
   fclose(f);
}


//////////////Irinst////////////
IrinstTiming::EnumTy IrinstTiming::classify(Instruction* I)
{
   unsigned Op = I->getOpcode();
   unsigned op;
   auto e = std::out_of_range("no group for this instruction");

   switch(Op){
      case Instruction::FAdd:          op = FLOAT_ADD;      break;
      case Instruction::FSub:          op = FLOAT_SUB;      break;
      case Instruction::Sub :          op = FIX_SUB;        break;
      case Instruction::Add :          op = FIX_ADD;        break;
      case Instruction::FMul:          op = FLOAT_MUL;      break;
      case Instruction::Mul :          op = FIX_MUL;        break;
      case Instruction::FDiv:          op = FLOAT_DIV;      break;
      case Instruction::UDiv:          op = U_DIV;          break;
      case Instruction::SDiv:          op = S_DIV;          break;
      case Instruction::FRem:          op = FLOAT_REM;      break;
      case Instruction::SRem:          op = S_REM;          break;
      case Instruction::URem:          op = U_REM;          break;
      case Instruction::And :          op = AND;            break;
      case Instruction::Or  :          op = OR;             break;
      case Instruction::Xor :          op = XOR;            break;
      case Instruction::Alloca:        op = ALLOCA;         break;
      case Instruction::Load:          op = LOAD;           break;
      case Instruction::Store:         op = STORE;          break;
      case Instruction::GetElementPtr: op = GETELEMENTPTR;  break;
      case Instruction::Trunc:         op = TRUNC;          break;
      case Instruction::FPTrunc:       op = FPTRUNC;        break;
      case Instruction::ZExt:          op = ZEXT;           break;
      case Instruction::SExt:          op = SEXT;           break;
      case Instruction::FPToUI:        op = FPTOUI;         break;
      case Instruction::UIToFP:        op = UITOFP;         break;
      case Instruction::SIToFP:        op = SITOFP;         break;
      case Instruction::FPToSI:        op = FPTOSI;         break;
      case Instruction::IntToPtr:      op = INTTOPTR;       break;
      case Instruction::PtrToInt:      op = PTRTOINT;       break;
      case Instruction::BitCast:       op = BITCAST;        break;
      case Instruction::ICmp:          op = ICMP;           break;
      case Instruction::FCmp:          op = FCMP;           break;
      case Instruction::Select:        op = SELECT;         break;
      case Instruction::Shl:           op = SHL;            break;
      case Instruction::LShr:          op = LSHR;           break;
      case Instruction::AShr:          op = ASHR;           break;
      default: op = IrinstNumGroups;
   }
   return static_cast<EnumTy>(op);
}
IrinstTiming::IrinstTiming():
   TimingSource(Irinst,IrinstNumGroups), 
   T(params) {
   file_initializer = load_irinst;
}
double IrinstTiming::count(Instruction& I) const
{
   return params[classify(&I)];
}

double IrinstTiming::count(BasicBlock& BB) const
{
   double counts = 0.0;

   for(auto& I : BB)
      counts += params[classify(&I)];
   return counts;
}
static 
std::map<StringRef, IrinstTiming::EnumTy> InstMap = 
{
   {"load",          LOAD},
   {"store",         STORE},
   {"alloca",        ALLOCA},

   {"fix_add",       FIX_ADD}, 
   {"fix_sub",       FIX_SUB},
   {"fix_mul",       FIX_MUL},
   {"u_div",         U_DIV},
   {"s_div",         S_DIV},
   {"u_rem",         U_REM},
   {"s_rem",         S_REM},

   {"float_add",     FLOAT_ADD},
   {"float_sub",     FLOAT_SUB},
   {"float_mul",     FLOAT_MUL},
   {"float_div",     FLOAT_DIV},
   {"float_rem",     FLOAT_REM},

   {"shl",           SHL},
   {"lshr",          LSHR},
   {"ashr",          ASHR},
   {"and",           AND},
   {"or",            OR},
   {"xor",           XOR},

   //disabled part, we consider these ir couldn't translate to asm precisely
   {"icmp",          ICMP},
   {"fcmp",          FCMP},
   {"getelementptr", GETELEMENTPTR},
   {"trunc_to",      TRUNC},
   {"zext_to",       ZEXT},
   {"sext_to",       SEXT},
   {"fptrunc_to",    FPTRUNC},
   {"fpext_to",      FPEXT},
   {"fptoui_to",     FPTOUI},
   {"fptosi_to",     FPTOSI},
   {"uitofp_to",     UITOFP},
   {"sitofp_to",     SITOFP},
   {"ptrtoint_to",   PTRTOINT},
   {"inttoptr_to",   INTTOPTR},
   {"bitcast_to",    BITCAST},
   {"select",        SELECT},

   //lmbench part, lmbench is more precise than ours
   {"integer bit",   AND},
   {"integer add",   FIX_ADD},
   {"integer mul",   FIX_MUL},
   {"integer div",   S_DIV},
   {"integer mod",   S_REM},
   {"double add",    FLOAT_ADD},
   {"double mul",    FLOAT_MUL},
   {"double div",    FLOAT_DIV}
};
void IrinstTiming::load_irinst(const char* file, double* cpu_times)
{
   FILE* f = fopen(file,"r");
   if(f == NULL){
      fprintf(stderr, "Could not open %s file: %s", file, strerror(errno));
      exit(-1);
   }

   double nanosec;
   char ops[48];
   char line[512];
   std::string tmp = "";
   while(fgets(line,sizeof(line),f)){
      unsigned op;
      if (sscanf(line, "%47[^:]:\t%lf nanoseconds", ops, &nanosec) == 2) {
        tmp = ops;
        auto ite = InstMap.find(ops);
        if (ite != InstMap.end()) {
          op = ite->second;
          cpu_times[op] = nanosec;
        } else
          continue;
      }
   }
   fclose(f);
}

IrinstMaxTiming::IrinstMaxTiming() { this->kindof = IrinstMax; }
double IrinstMaxTiming::count(BasicBlock &BB) const
{
   double float_count = 0.0;
   double fix_count = 0.0;
   double non_of_them = 0.0;

   for(auto& I : BB){
      EnumTy E = classify(&I);
      switch (E) {
         case FIX_ADD:
         case FIX_SUB:
         case FIX_MUL:
         case U_DIV:
         case S_DIV:
         case U_REM:
         case S_REM:
         case ICMP:
            fix_count += params[E];
            break;

         case FLOAT_ADD:
         case FLOAT_SUB:
         case FLOAT_MUL:
         case FLOAT_DIV:
         case FLOAT_REM:
         case FCMP:
            float_count += params[E];
            break;

         default:
            non_of_them += params[E];
            break;
      }
   }
   return non_of_them + std::max(float_count, fix_count);
}

MPBenchTiming::MPBenchTiming():TimingSource(MPBench, 0)
{
   file_initializer = NULL;
   bandwidth = latency = NULL;
   char* REnv = getenv("MPI_SIZE");
   if(REnv == NULL){
      errs()<<"please set environment MPI_SIZE same as when profiling\n";
      exit(-1);
   }
   this->R = atoi(REnv);
}

MPBenchTiming::~MPBenchTiming()
{
   if(bandwidth) delete bandwidth;
   if(latency) delete latency;
}

void MPBenchTiming::init_with_file(const char* file)
{
   FILE* f = fopen(file,"r");
   if(f == NULL){
      fprintf(stderr, "Could not open %s file: %s", file, strerror(errno));
      exit(-1);
   }
   char line[512], field[128], group[128];
   unsigned off;
   while (fgets(line, sizeof(line), f)) {
      if (sscanf(line, "%127[^:]: %127[^:]: %n", field, group, &off) != 2)
         continue;
      FreeExpression** expr;
      if (strcmp(field, "mpi_bandwidth") == 0)
         expr = &bandwidth;
      else if (strcmp(field, "mpi_latency") == 0)
         expr = &latency;
      else continue;
      *expr = FreeExpression::Construct(group);
      if(*expr == NULL){
         fprintf(stderr, "Couldn't construct free expression: %s", group);
         exit(-1);
      }
      (*expr)->init_param(line+off);
   }
}

double MPBenchTiming::count(const llvm::Instruction& I, double bfreq,
                 double count) const
{
   const CallInst* CI = dyn_cast<CallInst>(&I);
   if(CI == NULL) return 0.;
   Function* F = dyn_cast<Function>(lle::castoff(const_cast<Value*>(CI->getCalledValue())));
   if(F == NULL) return 0.;
   StringRef FName = F->getName();
   if(!FName.startswith("mpi_")) return 0.0;
   auto Spec = MpiSpec.find(FName);
   if(Spec == MpiSpec.end()){
      errs()<<"WARNNING: doesn't consider "<<FName<<" mpi call\n";
      return 0.;
   }
   unsigned C = Spec->second;
   if(C == IGN) return 0.0;
   size_t D = get_datatype_index(FName, *CI);
   if(D == 0){
      errs()<<"WARNNING: doesn't consider MPI_datatype "<<D<<"\n";
      return 0.; // 避免传入0到自由表达式，因为有些会用于分母(除0异常)
   }
   //D = count * MpiType[D];
   D = MpiType[D];
   if (C == 0) {
      return bfreq * (*latency)(D) + count * D / (*bandwidth)(D);
   } else
      return bfreq * (*latency)(D) + C * count * D * log(R) / (*bandwidth)(D);
}

void MPBenchTiming::print(llvm::raw_ostream &OS) const
{
   OS<<"mpi_bandwidth: ";
   if (bandwidth)
      bandwidth->print(OS);
   else
      OS << "(NULL)";
   OS << "\nmpi_latency: ";
   if (latency)
      latency->print(OS);
   else
      OS << "(NULL)";
   OS << "\n";
}

const char* LmbenchTiming::Name = TimingSource::Register<LmbenchTiming>(
    "lmbench", "loading lmbench timing source");
const char* IrinstTiming::Name = TimingSource::Register<IrinstTiming>(
    "irinst", "loading llvm ir inst timing source");
const char* IrinstMaxTiming::Name = TimingSource::Register<IrinstMaxTiming>(
    "irinst-max", "loading llvm ir inst timing source");
const char* MPBenchTiming::Name = TimingSource::Register<MPBenchTiming>(
    "mpbench", "loading mpbench timing source");
