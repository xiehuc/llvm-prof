/*
 * inst-timing.c
 * Copyright (C) 2015 HaoMeng <835231875@qq.com>
 *
 * Distributed under terms of the GPL license.
 */

#include "libtiming.c"

#define REPNUM 50000
#define INSNUM 2000
#define ALLOCA_NUM 100

//ref+=inst_template(TEMPLATE,VAR);
#define REPEAT(TEMPLATE, REP, VAR...)                                          \
   {                                                                           \
      for (unsigned i = 0; i < REP; ++i) {                                     \
         beg = timing();                                                       \
         ref += inst_template(TEMPLATE, ##VAR);                                \
         end = timing();                                                       \
         sum[i] = end - beg - t_err;                                           \
      }                                                                        \
      ref /= REP;                                                              \
      double ins_cycles = (double)median(sum, REPNUM) / INSNUM;                \
      printf(TEMPLATE ":\t%lf nanoseconds,\t%lf cycles\n",                     \
             ins_cycles* cycle_time, ins_cycles);                              \
   }
#define REPEAT_INST(TEMPLATE, VAR...) REPEAT(TEMPLATE, REPNUM, ##VAR)
#define REPEAT_ALLOCA_INST(TEMPLATE, VAR...) REPEAT(TEMPLATE, ALLOCA_NUM, ##VAR)
#define REPEAT_GETELE_INST(TEMPLATE, VAR...) REPEAT(TEMPLATE, REPNUM, element, ##VAR)

static int element[1][INSNUM][2];
static double cycle_time;         

static int int_less(const void* pl, const void* pr)
{
   uint64_t l = *(const uint64_t*)pl, r = *(const uint64_t*)pr;
   return (l==r) ? 0 : l < r;
}

static uint64_t median(uint64_t* arr, size_t len)
{
   qsort(arr, len, sizeof(uint64_t), int_less);
   return arr[len/2];
}

int main()
{
   //Here we can also read system file to obtain the hightest CPU frequency  
   printf("Warnning: shouldn't use this program on laptop\n");
   cycle_time = timing_res();
   printf("CPU freq: %lf GHz\n",1/cycle_time);
   volatile int* var = malloc(sizeof(int));
   uint64_t beg, end, sum[REPNUM];
   int ref = 0;
   uint64_t t_err = timing_err();
   int integer = 333333;
   double real = 333.333;

   for(unsigned k = 0;k<1;k++){
      for(unsigned i = 0;i < INSNUM;i++){
         for(unsigned j = 0; j < 2;j++){
            element[k][i][j] = k+i+j;
         }
      }
   }

   REPEAT_INST("load",var);
   REPEAT_INST("store",var);
   REPEAT_INST("fix_add",var);
   REPEAT_INST("float_add",var);
   REPEAT_INST("fix_mul",var);
   REPEAT_INST("float_mul",var);
   REPEAT_INST("fix_sub",var);
   REPEAT_INST("float_sub",var);
   REPEAT_INST("u_div",var);
   REPEAT_INST("s_div",var);
   REPEAT_INST("float_div",var);
   REPEAT_INST("u_rem",var);
   REPEAT_INST("s_rem",var);
   REPEAT_INST("float_rem",var);
   REPEAT_INST("shl",var);
   REPEAT_INST("lshr",var);
   REPEAT_INST("ashr",var);
   REPEAT_INST("and",var);
   REPEAT_INST("or",var);
   REPEAT_INST("xor",var);
   REPEAT_ALLOCA_INST("alloca",var);
   REPEAT_GETELE_INST("getelementptr",var);
   REPEAT_INST("trunc_to",var);
   REPEAT_INST("zext_to",var);
   REPEAT_INST("sext_to",var);
   REPEAT_INST("fptrunc_to",var);
   REPEAT_INST("fpext_to",var);
   REPEAT_INST("fptoui_to",var);
   REPEAT_INST("fptosi_to",var);
   REPEAT_INST("uitofp_to",var);
   REPEAT_INST("sitofp_to",var);
   REPEAT_INST("ptrtoint_to",var);
   REPEAT_INST("inttoptr_to",var);
   REPEAT_INST("bitcast_to",var);
   REPEAT_INST("icmp",&integer);
   REPEAT_INST("fcmp",&real);
   REPEAT_INST("select",var);
   return 0;
}
