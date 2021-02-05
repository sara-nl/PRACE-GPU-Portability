#include <stdio.h>

int main(int argc, char** argv) {

int N=10000;
float x[10000];
float y[10000];
float a=1.5; 

  {
  for (int i=0; i<N; i++)
  {
    x[i] = 1.0;
    y[i] = 2.0;
  }

#pragma acc parallel loop
  for (int i=0; i<N; i++)
  {
    y[i] = a*x[i] + y[i];
  }

  }

  for (int i=0; i<N; i+=1000) printf("%f \n", x[i]);
  
  return 0;

}
