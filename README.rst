llvm-prof
===========

this was copied form llvm release 3.3 include ``llvm-prof`` ``libLLVMProfiling.so``
``libprofile_rt.so``

because since llvm release 3.4 they remove profiling related code totaly, so
this project would help you get the function back.

this also provide a ``inst-timing`` a simple program report all llvm ir's
instruction's cpu timing and cycles.

build
------

::

        $ sudo apt install llvm-3.5-dev clang-3.5 libopenmpi-dev gfortran # maybe mpi dependency could make a option.
	$ mkdir build
	$ cd build
	$ cmake .. 
	$ # or cmake .. -DLLVM_RECOMMEND_VERSION=3.5 to look for llvm-config-3.5
	$ make 
	$ sudo make install

compile option
~~~~~~~~~~~~~~~

*  ``LLVM_RECOMMEND_VERSION`` : select which llvm version to build
*  ``OUTPUT_HASPID``          : does llvmprof.out contain a pid for mpi program
*  ``DYNAMIC_LINK``           : dynamic link LLVM single big shared object

argument
---------

* `-list-all`      : print out all outputs
* `-value-content` : print out traped value detail content instead of brief report
* `-unsort`        : print out outputs without sort
* `-diff`          : 
  compare two output file and report whether they are different

  | example: ``llvm-prof -diff a.out b.out``

* `-merge`         : merge a list of output file into one.

  | example: ``llvm-prof -merge=sum generate.out *input.out``
  | option: -merge=none -merge=sum -merge=avg

* `-to-block`      : convert edge profiling output to basicblock info format

  | example: ``llvm-prof -to-block bitcode input.out output.out``

* `-timing`        : 
  cacluating prog's execute timing from llvmprof.out and timing source
  support multi source, split by ':'

  | example: ``llvm-prof -timing=lmbench bitcode prof.out lmbench.log``
  | example: ``llvm-prof -timing=lmbench:mpi bitcode prof.out lmbench.log mpi.log``
  | option: -timing=none -timing=lmbench -timing=mpi

environment variable
---------------------

* `PROFILING_OUTDIR` : put all llvmprof.out.\* to the dir

extra profilings
-----------------

* *ValueProfiling*    : provide value profiling, could trap some value.
* *PredBlockProfiling* : similar to edge profiling, different is it increase
  counter with a value, not 1. it is used in prediction block frequence.
* *MPIProfiling* : profiling for mpi call's count parameter

note
-----

* you need add `$PKG_CONFIG_PATH` to let pkg-config recognise llvm-prof.pc
* If there is something wrong with mpi or gfortran when you compiling llvm-prof,
  you can try to install libopenmpi-dev and gfortran.
