#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "openacc.h"
#include <sys/time.h>

static const int MAX_ITERS = 255;

// All possible optimizations
// #define LOAD_BALANCE
//OR: 
// #define COARSENING

// // #define OPT_ALGEBRAIC//seems to have a negative impact (on runtime),
// //probably because (according to compiler output) the compiler does not
// //use FMA instructions anymore. 

void func(unsigned char *buf, unsigned int xsize, unsigned int ysize) {
    struct timeval tvalBefore, tvalAfter;
    gettimeofday (&tvalBefore, NULL);
  #pragma acc parallel copy(buf[0:xsize*ysize - 1])
  {
    const float xmin = -1.5;
    const float ymin = -1.25;
    const float dx = 2.0 / xsize;
    const float dy = 2.5 / ysize;

#ifdef LOAD_BALANCE
    #pragma acc loop independent
    for (unsigned int k = 0; k < xsize; k++) {
      for (unsigned int l = 0; l < ysize; l++) {
        unsigned int px = k;
        unsigned int py = l;

        float x0 = xmin + px*dx;
        float y0 = ymin + py*dy;
        float x = 0.0;
        float y = 0.0;
        unsigned int j = 0;

  #ifdef OPT_ALGEBRAIC
        float xsqr = x * x;
        float ysqr = y * y;

        for(j = 0; xsqr + ysqr < 4.0 && j < MAX_ITERS; j++) {
            y = x * y;
            y += y; // Multiply by two
            y += y0;
            x = xsqr - ysqr + x0;
            xsqr = x * x;
            ysqr = y * y;
        }
  #else
        for(j = 0; x*x + y*y < 4.0 && j < MAX_ITERS; j++)
        {
          float xtemp = x*x - y*y + x0;
          y = 2*x*y + y0;
          x = xtemp;
        }
  #endif
        buf[l*xsize+k] = j;
      }
    }
  }
#elif defined COARSENING
  // H: Note: assumption: (xsize*ysize) % 2 == 0!
  #pragma acc loop independent
  for (unsigned int i = 0; i < (xsize * ysize) / 2; i++)
  {
    #pragma acc loop seq
    for (unsigned int k = 0; k < 2; k++)
    {
        // printf("%d %d\n", __pgi_gangidx(), __pgi_workeridx());
        unsigned int l = i + k * ((xsize * ysize) / 2);
        unsigned int px = l % xsize;
        unsigned int py = l / xsize;

        float x0 = xmin + px*dx;
        float y0 = ymin + py*dy;
        float x = 0.0;
        float y = 0.0;
        unsigned int j = 0;

  #ifdef OPT_ALGEBRAIC
        float xsqr = x * x;
        float ysqr = y * y;

        for(j = 0; xsqr + ysqr < 4.0 && j < MAX_ITERS; j++) {
            y = x * y;
            y += y; // Multiply by two
            y += y0;
            x = xsqr - ysqr + x0;
            xsqr = x * x;
            ysqr = y * y;
        }
  #else
        for(j = 0; x*x + y*y < 4.0 && j < MAX_ITERS; j++)
        {
          float xtemp = x*x - y*y + x0;
          y = 2*x*y + y0;
          x = xtemp;
        }
  #endif
        buf[l] = j;
      }
    }
  }
#else
    #pragma acc loop independent
      for (unsigned int i = 0; i < xsize * ysize; i++)
      {
        // printf("%d %d\n", __pgi_gangidx(), __pgi_workeridx());
        unsigned int px = i % xsize;
        unsigned int py = i / xsize;

        float x0 = xmin + px*dx;
        float y0 = ymin + py*dy;
        float x = 0.0;
        float y = 0.0;
        unsigned int j = 0;

  #ifdef OPT_ALGEBRAIC
        float xsqr = x * x;
        float ysqr = y * y;

        for(j = 0; xsqr + ysqr < 4.0 && j < MAX_ITERS; j++) {
            y = x * y;
            y += y; // Multiply by two
            y += y0;
            x = xsqr - ysqr + x0;
            xsqr = x * x;
            ysqr = y * y;
        }
  #else
        for(j = 0; x*x + y*y < 4.0 && j < MAX_ITERS; j++)
        {
          float xtemp = x*x - y*y + x0;
          y = 2*x*y + y0;
          x = xtemp;
        }
  #endif
        buf[i] = j;
      }
  }
#endif

    gettimeofday (&tvalAfter, NULL);

    float kernel_time = (((tvalAfter.tv_sec - tvalBefore.tv_sec) * 1000000L
      + tvalAfter.tv_usec) - tvalBefore.tv_usec) / 1000000.0;
    printf("Kernel time: %f seconds\n", kernel_time);
}

void empty_kernel(unsigned char *buf, unsigned int xsize, unsigned int ysize) {
    struct timeval tvalBefore, tvalAfter;
    gettimeofday (&tvalBefore, NULL);
  #pragma acc parallel copy(buf[0:xsize*ysize - 1])
  {
    // const float xmin = -1.5;
    // const float ymin = -1.25;
    // const float dx = 2.0 / xsize;
    // const float dy = 2.5 / ysize;

    #pragma acc loop independent
    for (unsigned int k = 0; k < ysize*xsize; k++) {
      // unsigned int a = k;
      // k = ysize*xsize + 1;
    }
  }
    gettimeofday (&tvalAfter, NULL);

    float kernel_time = (((tvalAfter.tv_sec - tvalBefore.tv_sec) * 1000000L
      + tvalAfter.tv_usec) - tvalBefore.tv_usec) / 1000000.0;
    printf("Kernel time: %f seconds\n", kernel_time);
}

int main(int argc, char *argv[])
{
  unsigned char *buf;

  unsigned int xsize = 8192;
  unsigned int ysize = 8192;

  if (argc > 1)
  {
    xsize = ysize = atoi(argv[1]);
    if (argc > 2)
    {
      ysize = atoi(argv[2]);
    }
  }

  if (!(buf = malloc(xsize * ysize)))
  {
    fprintf(stderr,"Not enough memory");
    return 1;
  }

  struct timeval tvalBefore, tvalAfter;
  gettimeofday (&tvalBefore, NULL);

  func(buf, xsize, ysize);

  gettimeofday (&tvalAfter, NULL);

  float kernel_time = (((tvalAfter.tv_sec - tvalBefore.tv_sec) * 1000000L
    + tvalAfter.tv_usec) - tvalBefore.tv_usec) / 1000000.0;

#ifdef OPT_ALGEBRAIC
  long n_fl_ops_per_iter = 8;
#else
  long n_fl_ops_per_iter = 10;
#endif
  long n_fl_ops = 0;
  float n_mem_ops = 0;
  /* Write out the picture as portable gray map */
  FILE *out = fopen("test.pgm","w");
  fprintf(out,"P2\n%d %d\n%d\n",xsize,ysize,MAX_ITERS);
  for (int y = 0; y < ysize; y++)
  {
    for (int x = 0; x< xsize; x++)
    {
      fprintf(out,"%d ",buf[y*xsize + x]);
      n_fl_ops += buf[y*xsize + x];
    }
    fprintf(out,"\n");
  }
  // free(buf);

  n_fl_ops *= n_fl_ops_per_iter;
#ifdef CPU_TIMERS
  n_mem_ops = (xsize * ysize) / 16;//write + caching (16).
#else
  n_mem_ops = xsize * ysize;//GPU: write counted as 1 ops + no caching.
#endif
  printf("Kernel time: %f seconds\n", kernel_time);
  printf("Floating point OPS performed: %ld\n", n_fl_ops);
  printf("GFLOP/S achieved: %f\n", n_fl_ops / kernel_time / 1000000000.0);
  printf("Memory OPS performed: %f\n", n_mem_ops);
  printf("AI: %f\n", n_fl_ops / (n_mem_ops * 4));




  gettimeofday (&tvalBefore, NULL);

  empty_kernel(buf, xsize, ysize);

  gettimeofday (&tvalAfter, NULL);

  kernel_time = (((tvalAfter.tv_sec - tvalBefore.tv_sec) * 1000000L
    + tvalAfter.tv_usec) - tvalBefore.tv_usec) / 1000000.0;
  printf("Kernel time: %f seconds\n", kernel_time);


  return 0;
}
// TODO: time calculation is not right: can I use profiling info?