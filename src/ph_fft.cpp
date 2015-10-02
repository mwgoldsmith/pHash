/*

pHash, the open source perceptual hash library
Copyright (C) 2009 Aetilius, Inc.
All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Evan Klinger - eklinger@phash.org
David Starkweather - dstarkweather@phash.org

*/

#include "ph_fft.h"
#include <math.h>

struct complex {
  complex() = default;

  complex(double r, double i)
    : x{ r }
    , y{ i } {}

  inline complex operator+(const complex b) const {
    return complex(x + b.x, y + b.y);
  }

  inline complex operator-(const complex b) const {
    return complex(x - b.x, y - b.y);
  }

  inline complex operator*(const complex b) const {
    return complex((x * b.x) - (y * b.y), (x * b.y) + (y * b.x));
  }

  double x, y; // real and imaginary parts
};

namespace {

inline complex polar_to_complex(const double r, const double theta) {
  return complex(r * cos(theta), r * sin(theta));
}

inline void fft_calc(const int N, const double* x, complex* X, complex* P, const int step, const  complex* twids) {
  int k;
  complex* S = P + N / 2;

  if (N == 1) {
    X[0].x = x[0];
    X[0].y = 0;
    return;
  }

  fft_calc(N / 2, x, S, X, 2 * step, twids);
  fft_calc(N / 2, x + step, P, X, 2 * step, twids);

  for (k = 0; k < N / 2; k++) {
    P[k] = P[k] * twids[k * step];
    X[k] = S[k] + P[k];
    X[k + N / 2] = S[k] - P[k];
  }
}

}

int fft(const double* x, const int N, complex* X) {
  complex* twiddle_factors = new complex[(N / 2)];
  complex* Xt = new complex[N];
  int k;

  for (k = 0; k < N / 2; k++) {
    twiddle_factors[k] = polar_to_complex(1.0, 2.0 * M_PI * k / N);
  }

  fft_calc(N, x, X, Xt, 1, twiddle_factors);

  delete[] twiddle_factors;
  delete[] Xt;

  return 0;
}
