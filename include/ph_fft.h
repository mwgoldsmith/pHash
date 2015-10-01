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


#ifndef FFT_H
#define FFT_H

#define M_PI 3.1415926535897932

// Definition of the _complex struct to be used by those who use the complex
// functions and want type checking.
#ifndef _COMPLEX_DEFINED
#define _COMPLEX_DEFINED

typedef struct {
  double x, y; // real and imaginary parts
} _complex;

#if !__STDC__ && !defined __cplusplus
// Non-ANSI name for compatibility
#define complex _complex
#endif
#endif

_complex polar_to_complex(const double r, const double theta);

_complex add_complex(const _complex a, const _complex b);

_complex sub_complex(const _complex a, const _complex b);

_complex mult_complex(const _complex a, const _complex b);

void fft_calc(const int N, const double* x, _complex* X, _complex* P, const int step, const _complex* twids);
int fft(const double* x, const int N, _complex* X);

#endif
