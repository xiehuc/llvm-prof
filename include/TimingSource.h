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

   enum Kind {
      Base,
      Lmbench,
      Irinst,
      IrinstMax,
      MPBench
   };

   TimingSource(Kind K, size_t NumParam):kindof(K){
      // prevent return NumGroup, which out of range
      params.resize(NumParam+1); 
   }
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

   virtual double count(llvm::BasicBlock &BB) const {return 0.;} // caculation part
   virtual double count(const llvm::Instruction &I, double bfreq,
                        double count) const{return 0.;} // io part
   virtual void print(llvm::raw_ostream&) const;

   protected:
   Kind kindof;
   void (*file_initializer)(const char* file, double* data);
   std::vector<double> params;
   private:
   static void Register_(const char* Name, const char* Desc, std::function<TimingSource*()> &&);
};

struct TimingSourceInfoEntry{
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
   SOCK_BANDWIDTH,                                 // unit: MB/sec
   SOCK_LATENCY,                                   // unit: microsecond
   NumGroups
};

class LmbenchTiming: 
   public TimingSource, public _timing_source::T<LmbenchInstGroups>
{
   unsigned R;
   public:
   static const char* Name;
   typedef LmbenchInstGroups EnumTy;
   static llvm::StringRef getName(EnumTy);
   static EnumTy classify(llvm::Instruction* I);
   static void load_lmbench(const char* file, double* cpu_times);
   static bool classof(const TimingSource* S) {
      return S->getKind() == Lmbench;
   }

   LmbenchTiming();

   double count(llvm::Instruction& I) const; // caculation part
   double count(llvm::BasicBlock& BB) const override; // caculation part

   double count(const llvm::Instruction &I, double bfreq,
                double count) const override;
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
class IrinstTiming: 
   public TimingSource, public _timing_source::T<IrinstGroups>
{
   public:
   static const char* Name;
   typedef IrinstGroups EnumTy;
   static EnumTy classify(llvm::Instruction* I);
   static void load_irinst(const char* file, double* cpu_times);
   static bool classof(const TimingSource* S) {
      return S->getKind() == Irinst;
   }

   IrinstTiming();

   using TimingSource::count;
   double count(llvm::Instruction& I) const; // caculation part
   double count(llvm::BasicBlock& BB) const override; // caculation part
   
};

class IrinstMaxTiming: public IrinstTiming
{
   public:
   static const char* Name;
   static bool classof(const TimingSource* S) {
      return S->getKind() == IrinstMax;
   }
   IrinstMaxTiming();
   double count(llvm::BasicBlock& BB) const override;
};

/** if need init with file, should first cast TimingSource to
 * MPBenchTiming, then use it's init_with_file.
 * because it's init_with_file differs from TimingSource's
 */
class MPBenchTiming : public TimingSource {
   public:
   static const char* Name;
   static bool classof(const TimingSource* S) {
      return S->getKind() == MPBench;
   }

   MPBenchTiming();
   ~MPBenchTiming();
   void init_with_file(const char* file);

   using TimingSource::count;
   double count(const llvm::Instruction &I, double bfreq,
                double count) const override;
   void print(llvm::raw_ostream&) const override;
   private: 
   FreeExpression* bandwidth;
   FreeExpression* latency;
   unsigned R;
};

}

#endif
