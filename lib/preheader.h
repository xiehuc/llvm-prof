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
