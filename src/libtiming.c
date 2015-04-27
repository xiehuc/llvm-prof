/*
 * libtiming.c
 * Copyright (C) 2015 xiehuc <xiehuc@gmail.com>
 *
 * Distributed under terms of the GPL license.
 *
 * function: timing_res 
 * return once timing's resolution, nanosecond unit
 *
 * function: timing_err
 * return an error between two timing's, should sub this value
 *
 * function: timing
 * return a timing, mul timing_res to calc real time
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

/* the define for inst_template.
 * use a template to generate instruction 
 * Impletion in InstTemplate.cpp, a LLVM pass
 */
int inst_template(const char* templ, ...);

/** @return Hz **/
static unsigned long get_cpu_freq_by_sys()
{
   const char* file = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";
   FILE* f = fopen(file,"r");
   if(f==NULL){
      perror("Unable Find CPU freq sys file:");
      fprintf(stderr, "%s\n", file);
      return 0;
   }
   unsigned long freq = 0;
   fscanf(f, "%lu", &freq);
   if(freq==0)
      perror("Unable Read CPU freq:");
   fclose(f);
   return freq * 1E3; //convert KHz to Hz
}

static unsigned long get_cpu_freq_by_proc()
{
   const char* file = "/proc/cpuinfo";
   FILE* f = fopen(file, "r");
   if(f==NULL){
      perror("Unable Find /proc/cpuinfo File:");
      return 0;
   }
   char line[256] = {0};
   unsigned long freq = 0L;
   while(fgets(line, sizeof(line), f)){
      double mhz = 0.0L;
      if (sscanf(line, "cpu MHz\t: %lf", &mhz) == 1){
         freq = mhz * 1E6; // convert MHz to Hz
         break;
      }
   }
   if(freq==0)
      perror("Unable Read CPU freq:");
   fclose(f);
   return freq;
}


#if (defined TIMING_tsc) || (defined TIMING_tscp)


/* We use 64bit values for the times.  */
typedef unsigned long long int hp_timing_t;

#ifdef TIMING_tsc
/** copy code from simple-pmu:cycles.h (http://halobates.de/simple-pmu) **/
static inline hp_timing_t _timing(void)
{
#ifdef __i386__
	unsigned long long s;
	asm volatile("rdtsc" : "=A" (s) :: "memory");
	return s;
#else
	unsigned low, high;
	asm volatile("rdtsc" : "=a" (low), "=d" (high) :: "memory");
	return ((unsigned long long)high << 32) | low;
#endif
}
#else /*TIMING_TSC*/
static inline hp_timing_t _timing(void)
{
#ifdef __i386__
	unsigned long long s;
	asm volatile("rdtscp" : "=A" (s) :: "ecx", "memory");
	return s;
#else
	unsigned low, high;
	asm volatile("rdtscp" : "=a" (low), "=d" (high) :: "ecx", "memory");
	return ((unsigned long long)high << 32) | low;
#endif
}
#endif /*TIMING_TSC*/


double timing_res() 
{
   unsigned long freq = get_cpu_freq_by_sys();
   if(freq == 0) freq = get_cpu_freq_by_proc();
   if(freq == 0) exit(-1);
   return 1.0E9 /*nanosecond*/ / freq;
}
uint64_t timing_err()
{
   hp_timing_t a = 0,b = 0;
   a=_timing();
   b=_timing();
   return b-a;
}
uint64_t timing()
{
   return _timing();
}


#endif

#ifdef TIMING_clock_gettime

#define CLK_ID CLOCK_PROCESS_CPUTIME_ID

double timing_res() 
{
   return 1.0;
}
uint64_t timing_err()
{
   struct timespec a = {0}, b = {0};
   long sum = 0;
   for(int i=0;i<100;++i){
      clock_gettime(CLK_ID, &a);
      clock_gettime(CLK_ID, &b);
      sum += (b.tv_sec - a.tv_sec)*1E9;
      sum += b.tv_nsec - a.tv_nsec;
   }
   return sum /=100;
}
uint64_t timing()
{
   struct timespec t = {0};
   clock_gettime(CLK_ID, &t);
   return t.tv_sec*1E9+t.tv_nsec;
}
#endif

// return Hz
static unsigned long get_cpu_freq_by_sleep(){
   uint64_t t_err = timing_err();
   unsigned long beg = 0, end = 0, sum = 0;
   beg = timing();
   sleep(1);
   end = timing();
   sum += (end-beg-t_err);
   beg = timing();
   sleep(0);
   end = timing();
   sum -= end - beg - t_err; // sub sleep_err
   return sum;
}
