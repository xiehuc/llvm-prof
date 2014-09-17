//***********************************************************
//  Must Include this File Before All #include llvm headers
//***********************************************************

/**
 * XXX You must include this file before all included llvm header files.
 * because some datastructure use NDEBUG, so you need define NDEBUG before
 * them, or datastructure broken, leading undefined behavior
 */

#ifdef LLVM_FLAGS_NDEBUG // a special macro shows llvm build with NDEBUG
#undef NDEBUG
#define NDEBUG
#else
#undef NDEBUG
#endif

#if LLVM_VERSION_MAJOR==3 && LLVM_VERSION_MINOR==4
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/CFG.h>
#include <llvm/Support/system_error.h>
#include <llvm/Support/CallSite.h>
#else
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/CallSite.h>
#include <system_error>
#endif
