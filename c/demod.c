#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <complex.h>
#include <assert.h>
#include "rtl-sdr.h"

static int samp_rate, carrier, bw, sps, decim;

FILE *gnuplot;

void demod_init(int s, int c, int b, int sp) {
  samp_rate = s;
  carrier = c;
  bw = b;
  sps = sp;
  assert((s % (b * sp)) == 0);
  decim = s / (b*sp);

  gnuplot = popen("gnuplot", "w");
  fprintf(gnuplot, "set yrange [-10:10]\n");
}

void demod_callback(unsigned char *buf, uint32_t len, void *ctx) {
  assert ((len % (2*decim)) == 0);
  int i = 0;


  double complex bb = 0;
  double mag = 0;
  double phase = 0;
  static double lastphase = 0;
  double dp = 0;

  double complex all_bb[len/2/decim];
  double all_mag[len/2/decim];
  double all_phase[len/2/decim];
  double all_dp[len/2/decim];


  if (ctx) {
    double complex val;

    for (i = 0; i < len/2; i++) {
      if ((i % decim) == 0) 
        bb = 0;

      val = buf[2*i] + buf[2*i+1]*I;
      val /= 255.0/2.0;
      val -= 1+I;
      val *= cexp(-2.0*M_PI*carrier*I*i/samp_rate);
      bb += val;

      if (((i+1) % decim) == 0) {
        mag = cabs(bb);
        phase = carg(bb);

        dp = phase - lastphase;
        if (dp>0)
            dp = fmod(dp+M_PI, 2.0*M_PI)-M_PI;
        else
            dp = fmod(dp-M_PI, 2.0*M_PI)+M_PI;

        lastphase = phase;

        int index = i/decim;
        all_bb[index] = bb;
        all_mag[index] = mag;
        all_phase[index] = phase;
        all_dp[index] = dp;
      }
    }

    fprintf(gnuplot, "plot '-'\n");
    for (i = 0; i < len/2/decim; i++)
        fprintf(gnuplot, "%g\n", all_dp[i]/M_PI*4*sps);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);

  }
}

void demod_close(void) {
  pclose(gnuplot);
}
