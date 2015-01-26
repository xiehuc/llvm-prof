      PROGRAM CMPLXD
         use mpi
         character(len=*), PARAMETER :: FMT = "('DT[',I0,']=',I0,';')"
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
         write(*,FMT) MPI_REAL,    sizeof(r) 
         write(*,FMT) MPI_INTEGER, sizeof(i) 
         write(*,FMT) MPI_LOGICAL, sizeof(l) 
         write(*,FMT) MPI_DOUBLE_PRECISION, sizeof(dp)
         write(*,FMT) MPI_COMPLEX, sizeof(c) 
         write(*,FMT) MPI_DOUBLE_COMPLEX, sizeof(dc)

         write(*,FMT) MPI_INTEGER1, sizeof(i1)
         write(*,FMT) MPI_INTEGER2, sizeof(i2)
         write(*,FMT) MPI_INTEGER4, sizeof(i4)
         write(*,FMT) MPI_REAL4,    sizeof(r4) 
         write(*,FMT) MPI_REAL8,    sizeof(r8) 

         write(*,FMT) MPI_2INTEGER, 2*sizeof(i)
         write(*,FMT) MPI_2REAL,    2*sizeof(r) 
         write(*,FMT) MPI_2DOUBLE_PRECISION,2*sizeof(dp) 
         write(*,FMT) MPI_2COMPLEX, 2*sizeof(c)
         write(*,FMT) MPI_2DOUBLE_COMPLEX, 2*sizeof(dc)
      END
