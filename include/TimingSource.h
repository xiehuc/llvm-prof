#ifndef LLVM_TIMING_SOURCE_H_H
#define LLVM_TIMING_SOURCE_H_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/GlobalVariable.h>

class FreeExpression;

/* a timing source is used to count inst types in a basicblock */
namespace llvm{
struct TimingSourceInfoEntry;
class TimingSource{
   public:
   static TimingSource* Construct(const llvm::StringRef Name);
   template<class T>
   static const char* Register(const char* Name, const char* Desc){
      TimingSource::Register_(Name, Desc, [](){return new T;});
      return Name;
   }
   static const std::vector<TimingSourceInfoEntry>& Avail();

   enum class Kind {
      Base,
      BBlock,
      Lmbench,
      Irinst,
      IrinstMax,
      BBlockLast,
      MPI = BBlockLast,
      MPBench,
      MPBenchRe, // a mpbench source for new mpi format
      Latency,
      MPILast,
      LibCall = MPILast,
      LibFn,
      LibCallLast
   };

   virtual ~TimingSource(){};
   //在该模式中, 立即从文件中读取Timing数据并计算//
   /* init unit_times with nanoseconds unit.
    * @example:
    *    init(std::bind(TimingSource::load_lmbench, "lmbench.log", _1));
    */
   virtual void init(std::function<void(double*)> func){
      func(params.data());
   }
   virtual void init(std::initializer_list<double> list) {
      params.clear();
      params.assign(list);
   }
   virtual void init_with_file(const char* file) {
      init(std::bind(file_initializer, file, std::placeholders::_1));
   }
   Kind getKind() const { return kindof;}

   virtual void print(llvm::raw_ostream&) const;

   protected:
   Kind kindof;
   void (*file_initializer)(const char* file, double* data);
   std::vector<double> params;

   TimingSource(Kind K, size_t NumParam):kindof(K){
      // prevent return NumGroup, which out of range
      params.resize(NumParam+1); 
   }
   private:
   static void Register_(const char* Name, const char* Desc, std::function<TimingSource*()> &&);
};

class BBlockTiming: public TimingSource
{
   public:
   static bool classof(const TimingSource* S)
   {
      return S->getKind() < Kind::BBlockLast
             && S->getKind() > Kind::BBlock;
   }
   virtual double count(llvm::BasicBlock& BB) const = 0;
   protected:
   BBlockTiming(Kind K, size_t N):TimingSource(K,N) {}
};

class MPITiming: public TimingSource
{
   public:
   static bool classof(const TimingSource* S)
   {
      return S->getKind() < Kind::MPILast && S->getKind() > Kind::MPI;
   }
   virtual double count(const llvm::Instruction& I, double bfreq,
                        double count) const = 0; // io part
   protected:
   MPITiming(Kind K, size_t N);
   unsigned R;
};

class LibCallTiming: public TimingSource
{
   public:
   static bool classof(const TimingSource* S)
   {
      return S->getKind() < Kind::LibCallLast && S->getKind() > Kind::LibCall;
   }
   virtual double count(const llvm::CallInst& CI, double bfreq) const = 0;
   protected:
   LibCallTiming(Kind K, size_t N):TimingSource(K, N) {}
};

struct TimingSourceInfoEntry {
   StringRef Name;
   StringRef Desc;
   std::function<TimingSource*()> Creator;
};

namespace _timing_source{
template<class EnumType>
class T
{
   std::vector<double>& address;
   public:
   T(std::vector<double>& P):address(P) {}
   double get(EnumType E) const{
      return address[E];
   }
};
}

enum LmbenchInstGroups {
   Integer = 0, I64 = 1, Float = 2, Double = 3,    // Ntype
   Add = 0<<2, Mul = 1<<2, Div = 2<<2, Mod = 3<<2, // Method
   Last = Double|Mod,                              // Method | Ntype unit: nanosecond
   NumGroups
};

class LmbenchTiming : public BBlockTiming,
                      public _timing_source::T<LmbenchInstGroups> 
{
   public:
   static const char* Name;
   typedef LmbenchInstGroups EnumTy;
   static llvm::StringRef getName(EnumTy);
   static EnumTy classify(llvm::Instruction* I);
   static void load_lmbench(const char* file, double* cpu_times);
   static bool classof(const TimingSource* S)
   {
      return S->getKind() == Kind::Lmbench;
   }

   LmbenchTiming();

   double count(llvm::Instruction& I) const; // caculation part
   double count(llvm::BasicBlock& BB) const override; // caculation part
};

enum IrinstGroups {
   LOAD      , STORE     , ALLOCA   , GETELEMENTPTR , FIX_ADD , FLOAT_ADD ,
   FIX_MUL   , FLOAT_MUL , FIX_SUB  , FLOAT_SUB     , U_DIV   , S_DIV     ,
   FLOAT_DIV , U_REM     , S_REM    , FLOAT_REM     , SHL     , LSHR      ,
   ASHR      , AND       , OR       , XOR           , TRUNC   , ZEXT      ,
   SEXT      , FPTRUNC   , FPEXT    , FPTOUI        , FPTOSI  , UITOFP    ,
   SITOFP    , PTRTOINT  , INTTOPTR , BITCAST       , ICMP    , FCMP      ,
   SELECT    ,
   IrinstNumGroups
};
class IrinstTiming : public BBlockTiming,
                     public _timing_source::T<IrinstGroups> 
{
   public:
   static const char* Name;
   typedef IrinstGroups EnumTy;
   static EnumTy classify(llvm::Instruction* I);
   static void load_irinst(const char* file, double* cpu_times);
   static bool classof(const TimingSource* S) {
      return S->getKind() == Kind::Irinst;
   }

   IrinstTiming();

   double count(llvm::Instruction& I) const; // caculation part
   double count(llvm::BasicBlock& BB) const override; // caculation part
};

class IrinstMaxTiming: public IrinstTiming
{
   public:
   static const char* Name;
   static bool classof(const TimingSource* S) {
      return S->getKind() == Kind::IrinstMax;
   }
   IrinstMaxTiming();
   double count(llvm::BasicBlock& BB) const override;
};

class MPBenchReTiming : public MPITiming 
{
   public:
   static const char* Name;
   static bool classof(const TimingSource* S) {
      return S->getKind() == Kind::MPBenchRe;
   }

   MPBenchReTiming();
   ~MPBenchReTiming();
   void init_with_file(const char* file);

   double count(const llvm::Instruction &I, double bfreq,
                double count) const override;
   void print(llvm::raw_ostream&) const override;
   protected: 
   FreeExpression* bandwidth;
   FreeExpression* latency;
};

class MPBenchTiming : public MPBenchReTiming
{
   public:
   static const char* Name;
   static bool classof(const TimingSource* S) {
      return S->getKind() == Kind::MPBench;
   }

   MPBenchTiming();

   double count(const llvm::Instruction& I, double bfreq,
                double count) const override;
};

enum MPISpec { MPI_LATENCY, MPI_BANDWIDTH, MPINumSpec };

class LatencyTiming : public MPITiming, public _timing_source::T<MPISpec>
{
   public:
   typedef MPISpec EnumTy;
   static const char* Name;
   static bool classof(const TimingSource* S) {
      return S->getKind() == Kind::Latency;
   }

   LatencyTiming();

   double count(const llvm::Instruction& I, double bfreq,
                double count) const override;
};

enum LibFnSpec { SQRT, LOG, FABS, LibFnNumSpec };

class LibFnTiming : public LibCallTiming, public _timing_source::T<LibFnSpec> 
{
   public:
   typedef LibFnSpec EnumTy;
   static const char* Name;
   static bool classof(const TimingSource* S) {
      return S->getKind() == Kind::LibFn;
   }
   static void load_libfn(const char* file, double* cpu_times);

   LibFnTiming();

   double count(const llvm::CallInst& CI, double bfreq) const override;
};

}

#endif
