#ifndef LLVM_FREE_EXPRESSION_H_H
#define LLVM_FREE_EXPRESSION_H_H

namespace llvm {
class raw_ostream;
}

struct Param{
   const char* Name;
   double Val;
   Param(const char* N):Name(N),Val(0.) {};
   double operator*() const { return Val; }
};
class FreeExpression{
   public:
   static FreeExpression* Construct(const std::string& Name);
   template<class T>
   static const char* Register(const char* Name){
      FreeExpression::Register_(Name, [](){return new T;});
      return Name;
   }

   virtual ~FreeExpression(){};
   unsigned init_param(const std::string&);
   virtual double operator()(double X) const = 0;
   virtual void print(llvm::raw_ostream&) const = 0;
   private:
   static void Register_(const char* Name, std::function<FreeExpression*()>&&);
};

class LogisticLog: public FreeExpression {
   public:
   static const char* Name;
   LogisticLog():L("L"), k("k"), u("u"), End(0) {}
   double operator()(double X) const override;
   void print(llvm::raw_ostream&) const override;
   private:
   Param L;
   Param k;
   Param u;
   Param End;
};

class Linear: public FreeExpression {
   public:
   static const char* Name;
   Linear():k("k"), b("b"), End(0) {}
   double operator()(double X) const override {
      return k.Val * X + b.Val;
   }
   void print(llvm::raw_ostream&) const override;
   private:
   Param k;
   Param b;
   Param End;
};
#endif
