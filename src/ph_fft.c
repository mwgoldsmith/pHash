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
#include <malloc.h>
#include <math.h>

_complex polar_to_complex(const double r, const double theta) {
  _complex result;
  result.x = r * cos(theta);
  result.y = r * sin(theta);
  return result;
}

_complex add_complex(const _complex a, const _complex b) {
  _complex result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

_complex sub_complex(const _complex a, const _complex b) {
  _complex result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

_complex mult_complex(const _complex a, const _complex b) {
  _complex result;
  result.x = (a.x * b.x) - (a.y * b.y);
  result.y = (a.x * b.y) + (a.y * b.x);
  return result;
}

void fft_calc(const int N, const double* x, _complex* X, _complex* P, const int step, const _complex* twids) {
  int k;
  _complex* S = P + N / 2;

  if (N == 1) {
    X[0].x = x[0];
    X[0].y = 0;
    return;
  }

  fft_calc(N / 2, x, S, X, 2 * step, twids);
  fft_calc(N / 2, x + step, P, X, 2 * step, twids);

  for (k = 0; k < N / 2; k++) {
    P[k] = mult_complex(P[k], twids[k * step]);
    X[k] = add_complex(S[k], P[k]);
    X[k + N / 2] = sub_complex(S[k], P[k]);
  }
}

int fft(const double* x, const int N, _complex* X) {
  _complex* twiddle_factors = (_complex*)malloc(sizeof(_complex) * (N / 2));
  _complex* Xt = (_complex*)malloc(sizeof(_complex) * N);
  int k;

  for (k = 0; k < N / 2; k++) {
    twiddle_factors[k] = polar_to_complex(1.0, 2.0 * M_PI * k / N);
  }

  fft_calc(N, x, X, Xt, 1, twiddle_factors);

  free(twiddle_factors);
  free(Xt);

  return 0;
}
