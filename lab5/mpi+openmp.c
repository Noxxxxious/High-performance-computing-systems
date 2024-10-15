#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <mpi.h>
#include <math.h>
#include "numgen.c"

int isPrime(long unsigned int n) {
  if (n == 0 || n == 1) {
    return false;
  }
  if (n == 2) {
    return true;
  }
  for (unsigned long int i = 2; i * i <= n;i++) {
    if (n % i == 0) {
      return false;
    }
  }
  return true;
}

long unsigned int countPrimes(unsigned long int* numbers, int rangeSize) {
  long unsigned int total_primes = 0;
  #pragma omp parallel for
  for(int i = 0; i < rangeSize; i++){
    if (isPrime(numbers[i])){
      #pragma omp critical
      {
        total_primes++;
      }
    }
  }
  return total_primes;
}

int main(int argc, char** argv) {

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  long inputArgument = ins__args.arg;
  struct timeval ins__tstart, ins__tstop;
  int myrank, nproc;
  MPI_Status status;
  unsigned long int* numbers;
  int result = 0, resultTemp;
  

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  int rangeSize = inputArgument / nproc;
  int masterRangeSize = rangeSize + inputArgument % nproc;

  if (!myrank) {
    gettimeofday(&ins__tstart, NULL);
    numbers = (unsigned long int*)malloc(inputArgument * sizeof(unsigned long int));
    numgen(inputArgument, numbers);
  }

  if (myrank == 0) {
    for (int i = 1; i < nproc; i++) {
      MPI_Request request;
      MPI_Isend(&numbers[rangeSize * (i - 1)], rangeSize, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, &request);
    }

    for (int i = 1; i < nproc; i++) {
      MPI_Request request;
      MPI_Isend(NULL, 0, MPI_INT, i, 2, MPI_COMM_WORLD, &request);
    }

    MPI_Request *requests;
    requests = (MPI_Request*)malloc((nproc - 1) * sizeof(MPI_Request));
    unsigned long int* slaveResults = (unsigned long int*)malloc((nproc - 1) * sizeof(unsigned long int));
    for (int i = 1; i < nproc; i++) {
      MPI_Irecv(&slaveResults[i - 1], 1, MPI_INT, i, 1, MPI_COMM_WORLD, &(requests[i - 1]));
    }

    result += countPrimes(&numbers[rangeSize * (nproc - 1)], masterRangeSize);
    printf("[Master] Found %d primes.\n", result);

    MPI_Status *statuses;
    statuses = (MPI_Status*)malloc((nproc - 1) * sizeof(MPI_Status));
    MPI_Waitall(nproc - 1, requests, statuses);
    
    for(int i = 1; i < nproc; i++){
      result += slaveResults[i - 1];
    }

    printf("%d primes were found", result);
  }
  else {
    do {
      MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (status.MPI_TAG == 0) {
        numbers = (unsigned long int*)malloc((rangeSize) * sizeof(unsigned long int));
        MPI_Recv(numbers, rangeSize, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &status);
        resultTemp = countPrimes(numbers, rangeSize);
        printf("[Slave %d] Found %d primes, sending to master.\n", myrank, resultTemp);
        MPI_Request request;
        MPI_Isend(&resultTemp, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &request);
        free(numbers);
      }
    } while (status.MPI_TAG != 2);
  }

  if (!myrank) {
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }

  MPI_Finalize();

}