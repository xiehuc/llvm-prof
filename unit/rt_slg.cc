#include <gtest/gtest.h>

#define N 512
static int pos;
static unsigned Counters[N];

extern "C" {
int llvm_start_slg_profiling(int argc, const char **argv, unsigned* arrayStart,
      unsigned numElements);
void llvm_profiling_trap_load(int32_t idx, void* ptr);
void llvm_profiling_trap_store(int32_t idx, void* ptr);
}

TEST(libprofile_rt, stg_store)
{
   llvm_profiling_trap_store(0, (char*)&pos);
   llvm_profiling_trap_store(1, (char*)&pos+1);
}

TEST(libprofile_rt, stg_load)
{
   llvm_profiling_trap_load(0, (char*)&pos+1);
   llvm_profiling_trap_load(1, (char*)&pos);
   ASSERT_EQ(Counters[0], 1);
   ASSERT_EQ(Counters[1], 0);
}
TEST(libprofile_rt, stg_override)
{
	llvm_profiling_trap_store(2, (char*)&pos+2);
	llvm_profiling_trap_store(3, (char*)&pos+2);
   llvm_profiling_trap_load(2, (char*)&pos+2);
   ASSERT_EQ(Counters[2], 3);
}

TEST(libprofile_rt, stg_init)
{
	ASSERT_EQ(Counters[N-1], -1);
}

int main(int argc, char** argv)
{
   llvm_start_slg_profiling(argc, (const char**)argv, Counters, N);

   testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
