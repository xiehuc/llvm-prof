      PROGRAM CMPLXD
         use mpi
         character(len=*), PARAMETER :: FMT = "('DT[',I0,']=',I0,';')"
         write(*,FMT) MPI_REAL,    sizeof(real) 
         write(*,FMT) MPI_INTEGER, sizeof(integer) 
         write(*,FMT) MPI_LOGICAL, sizeof(logical) 
         write(*,FMT) MPI_DOUBLE_PRECISION, sizeof(double precesion)
         write(*,FMT) MPI_COMPLEX, sizeof(complex) 
         write(*,FMT) MPI_DOUBLE_COMPLEX, sizeof(complex*16)

         write(*,FMT) MPI_INTEGER1, sizeof(integer*1)
         write(*,FMT) MPI_INTEGER2, sizeof(integer*2)
         write(*,FMT) MPI_INTEGER4, sizeof(integer*4)
         write(*,FMT) MPI_REAL4,    sizeof(real*4) 
         write(*,FMT) MPI_REAL8,    sizeof(real*8) 

         write(*,FMT) MPI_2INTEGER, 2*sizeof(integer)
         write(*,FMT) MPI_2REAL,    2*sizeof(real) 
         write(*,FMT) MPI_2DOUBLE_PRECISION,2*sizeof(double precision) 
         write(*,FMT) MPI_2COMPLEX, 2*sizeof(complex)
         write(*,FMT) MPI_2DOUBLE_COMPLEX, 2*sizeof(complex*16)
      END
