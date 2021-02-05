#include <stdio.h>
#include <sys/time.h>

#define N 10000

int main(int argc, char** argv) {

float x[N*N];
float y[N];
float z[N];
float a=1.5; 

  for (int i=0; i<N; i++)
  {
   for (int j=0; j<N; j++) { 
    x[i*N+j] = 1.0;
   } 
   y[i]=2.0;
  }


    struct timeval tvalBefore, tvalAfter;
    gettimeofday (&tvalBefore, NULL);

//#pragma acc parallel loop
  for (int i=0; i<N; i++)
  {
    float sum = 0;
//#pragma acc loop reduction(+:sum)
    for (int j=0; j<N; j++) 
       sum += x[i*N+j] * y[j];
    z[i]=sum;   
  }
 
    gettimeofday (&tvalAfter, NULL);

    float kernel_time = (((tvalAfter.tv_sec - tvalBefore.tv_sec) * 1000000L
      + tvalAfter.tv_usec) - tvalBefore.tv_usec);
    printf("Kernel time: %f us\n", kernel_time);
 

  
  return 0;

}
