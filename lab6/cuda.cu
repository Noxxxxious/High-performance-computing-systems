#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <sys/time.h>
#include "numgen.c"

__host__
void errorexit(const char *s) {
    printf("\n%s",s);	
    exit(EXIT_FAILURE);	 	
}

__device__
bool is_prime(int n) {
    if (n <= 1) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (int i = 3; i <= sqrtf((float)n); i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

__global__ 
void check_primes(int *results, int size) {
    int my_index = blockIdx.x * blockDim.x + threadIdx.x;
    if (my_index < size) {
        results[my_index] = is_prime(my_index);
    }
}

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

int main(int argc,char **argv) {

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);
  
  //program input argument
  long inputArgument = ins__args.arg; 
  unsigned long int *numbers = (unsigned long int*)malloc(inputArgument * sizeof(unsigned long int));
  numgen(inputArgument, numbers);

  struct timeval ins__tstart, ins__tstop;
  gettimeofday(&ins__tstart, NULL);
  
  int threadsinblock=1024;
  int blocksingrid = (inputArgument + threadsinblock - 1) / threadsinblock;	

  int size = threadsinblock*blocksingrid;
  int *hresults=(int*)malloc(size*sizeof(int));
  if (!hresults) errorexit("Error allocating memory on the host");	

  int *dresults=NULL;
  if (cudaSuccess!=cudaMalloc((void **)&dresults,size*sizeof(int)))
    errorexit("Error allocating memory on the GPU");

  check_primes<<<blocksingrid, threadsinblock>>>(dresults, inputArgument);
  if (cudaSuccess != cudaGetLastError())
      errorexit("Error during kernel launch");
  
  if (cudaSuccess!=cudaMemcpy(hresults,dresults,size*sizeof(int),cudaMemcpyDeviceToHost))
    errorexit("Error copying results");
  
  int prime_count = 0;
  for(int i = 0; i <= inputArgument; i++) {
    if (hresults[i]) {
      prime_count++;
    }
  }

  printf("\nFound %d primes in total\n", prime_count);

  free(hresults);
  if (cudaSuccess!=cudaFree(dresults))
    errorexit("Error when deallocating space on the GPU");

  gettimeofday(&ins__tstop, NULL);
  ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);

}
