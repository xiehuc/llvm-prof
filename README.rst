llvm-prof
===========

this was copied form llvm release 3.3 include llvm-prof libLLVMProfiling
libprofile\_rt.so 

because since llvm release 3.4 they remove profiling related code totaly, so
this project would help you get the function back.

build
------

::

	$ mkdir build
	$ cd build
	$ cmake .. 
	$ make 
	$ sudo make install

compile option
~~~~~~~~~~~~~~~

*  ``LLVM_RECOMMAND_VERSION`` : select which llvm version to build
*  ``OUTPUT_HASPID``          : does llvmprof.out contain a pid for mpi program
*  ``DYNAMIC_LINK``           : dynamic link LLVM single big shared object

argument
---------

* `-list-all`      : print out all outputs
* `-value-content` : print out traped value detail content instead of brief report
* `-unsort`        : print out outputs without sort
* `-diff`          : compare two output file and report whether they are different
                     example: ``llvm-prof -diff a.out b.out``
* `-merge`         : merge a list of output file into one.
                     example: ``llvm-prof -merge=sum generate.out *input.out``
                     option: -merge=none -merge=sum -merge=avg
* `-to-block`      : convert edge profiling output to basicblock info format
                     example: ``llvm-prof -to-block bitcode input.out output.out``
* `-timing`        : cacluating prog's execute timing from llvmprof.out and timing source
                     example: ``llvm-prof -timing=lmbench bitcode prof.out lmbench.log``
                     option: -timing=none -timing=lmbench

environment variable
---------------------

* `PROFILING_OUTDIR` : put all llvmprof.out.\* to the dir

extra profilings
-----------------

* *ValueProfiling*    : provide value profiling, could trap some value.
* *SLGlobalProfiling* : provide store load profiling on global variables, could
  find dynamic data denpendencies.
* *PredBlockProfiling* : similar to edge profiling, different is it increase
  counter with a value, not 1. it is used in prediction block frequence.

note
-----

you need add `$PKG_CONFIG_PATH` to let pkg-config recognise llvm-prof.pc
