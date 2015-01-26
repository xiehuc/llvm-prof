      PROGRAM CMPLXD
         use mpi
         character(len=*), PARAMETER :: FMT=
     &   "('DT[',I0,']=',I0,';//',A)"
         real r
         integer i
         logical l
         complex c
         complex*16 dc
         double precision dp
         integer*1 i1
         integer*2 i2
         integer*4 i4
         real*4 r4
         real*8 r8
         write(*,FMT) MPI_REAL,    sizeof(r), "MPI_REAL" 
         write(*,FMT) MPI_INTEGER, sizeof(i), "MPI_INTEGER" 
         write(*,FMT) MPI_LOGICAL, sizeof(l), "MPI_LOGICAL"
         write(*,FMT) MPI_DOUBLE_PRECISION, sizeof(dp),
     &   "MPI_DOUBLE_PRECISION"
         write(*,FMT) MPI_COMPLEX, sizeof(c), "MPI_COMPLEX"
         write(*,FMT) MPI_DOUBLE_COMPLEX, sizeof(dc),
     &   "MPI_DOUBLE_COMPLEX"

         write(*,FMT) MPI_INTEGER1, sizeof(i1), "MPI_INTEGER1"
         write(*,FMT) MPI_INTEGER2, sizeof(i2), "MPI_INTEGER2"
         write(*,FMT) MPI_INTEGER4, sizeof(i4), "MPI_INTEGER4"
         write(*,FMT) MPI_REAL4,    sizeof(r4), "MPI_REAL4"
         write(*,FMT) MPI_REAL8,    sizeof(r8), "MPI_REAL8"

         write(*,FMT) MPI_2INTEGER, 2*sizeof(i), "MPI_2INTEGER"
         write(*,FMT) MPI_2REAL,    2*sizeof(r), "MPI_2REAL"
         write(*,FMT) MPI_2DOUBLE_PRECISION,2*sizeof(dp),
     &   "MPI_2DOUBLE_PRECISION"
         write(*,FMT) MPI_2COMPLEX, 2*sizeof(c), "MPI_2COMPLEX"
         write(*,FMT) MPI_2DOUBLE_COMPLEX, 2*sizeof(dc),
     &   "MPI_2DOUBLE_COMPLEX"
      END
