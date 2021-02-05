#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "openacc.h"
#include <sys/time.h>

static const int MAX_ITERS = 255;


void func(unsigned char *buf, unsigned int xsize, unsigned int ysize) {
    struct timeval tvalBefore, tvalAfter;
    gettimeofday (&tvalBefore, NULL);
  #pragma acc parallel copy(buf[0:xsize*ysize - 1])
  {
    const float xmin = -1.5;
    const float ymin = -1.25;
    const float dx = 2.0 / xsize;
    const float dy = 2.5 / ysize;

    #pragma acc loop independent
    for (unsigned int k = 0; k < xsize; k++) {
      for (unsigned int l = 0; l < ysize; l++) {
//	printf("%d %d\n", __pgi_gangidx(), __pgi_workeridx());
        unsigned int px = k;
        unsigned int py = l;

        float x0 = xmin + px*dx;
        float y0 = ymin + py*dy;
        float x = 0.0;
        float y = 0.0;
        unsigned int j = 0;

        for(j = 0; x*x + y*y < 4.0 && j < MAX_ITERS; j++)
        {
          float xtemp = x*x - y*y + x0;
          y = 2*x*y + y0;
          x = xtemp;
        }
        buf[l*xsize+k] = j;
      }
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

  func(buf, xsize, ysize);

  /* Write out the picture as portable gray map */
  FILE *out = fopen("test.pgm","w");
  fprintf(out,"P2\n%d %d\n%d\n",xsize,ysize,MAX_ITERS);
  for (int y = 0; y < ysize; y++)
  {
    for (int x = 0; x< xsize; x++)
    {
      fprintf(out,"%d ",buf[y*xsize + x]);
    }
    fprintf(out,"\n");
  }
  // free(buf);


  return 0;
}
