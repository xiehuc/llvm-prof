/*
 * libfn-timing.c
 * Copyright (C) 2015 xiehuc <xiehuc@gmail.com>
 *
 * Distributed under terms of the GPL license.
 */

#include "libtiming.c"
#include <math.h>
#include <sys/time.h>
#include <float.h>

#define REPNUM 10000
#define INTREP 100
#define REPEAT(FUNC)                                                           \
   {                                                                           \
      unsigned i, j;                                                           \
      for (i = 0; i < REPNUM; ++i) {                                           \
         PARAASSIGN;                                                           \
         beg = timing();                                                       \
         for (j = 0; j < INTREP; ++j) {                                        \
            ref += FUNC(PARALIST);                                             \
         }                                                                     \
         end = timing();                                                       \
         sum[i] = (end - beg) / INTREP;                                        \
      }                                                                        \
      ref /= REPNUM;                                                           \
      uint64_t cycle = median(sum, REPNUM);                                    \
      printf(#FUNC ":\t%lf nanoseconds,\t%lu cycles\n", cycle* res, cycle);    \
   }

static double double_rand(){
   struct timeval t = {0};
   gettimeofday(&t, NULL);
   srand48(t.tv_usec);
   return drand48();
}
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
   uint64_t beg, end;
   uint64_t sum[REPNUM];
   double ref = 0;
   double res = timing_res();
#define PARAASSIGN double arg1=double_rand();
#define PARALIST arg1
   REPEAT(sqrt);
   REPEAT(fabs);
   REPEAT(log);
#undef PARAASSIGN
#undef PARALIST
   return 0;
}
