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
D Grant Starkweather - dstarkweather@phash.org

*/

#include "internal.h"
#include "phash.h"

#include <cstdint>
#include <sys/types.h>
#include <string.h>
#include <vector>
#include <array>
#include <math.h>
#include <dirent.h>
#if !defined(__GLIBC__) && !defined(_WIN32)
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#endif
#include "callbacks.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifdef HAVE_VIDEO_HASH
#include "VideoProcessor.h"
#endif

/* variables for textual hash */
const int KgramLength = 50;
const int WindowLength = 100;
const int delta = 1;
static const int MAX_THREADS = 4;

typedef struct ph_slice {
  DP** hash_p;
  int n;
  void* hash_params;
} slice;

const char* ph_about() {
  static const char phash_project[] = PACKAGE_STRING " " PACKAGE_VERSION ". Copyright 2008-2010 Aetilius, Inc.";
  return phash_project;
}

#ifdef HAVE_IMAGE_HASH

CImageF* GetMHKernel(float alpha, float level) {
  //int sigma = static_cast<int>(4) * pow(alpha, level);
  auto sigma = static_cast<int>(4 * pow(alpha, level));
  static CImageF *pkernel = nullptr;
  float xpos, ypos, A;
  if(!pkernel) {
    pkernel = new CImageF(2 * sigma + 1, 2 * sigma + 1, 1, 1, 0);
    cimg_forXY(*pkernel, X, Y) {
      xpos = pow(alpha, -level)*(X - sigma);
      ypos = pow(alpha, -level)*(Y - sigma);
      A = xpos*xpos + ypos*ypos;
      pkernel->atXY(X, Y) = (2 - A)*exp(-A / 2);
    }
  }
  return pkernel;
}

uint8_t* ph_mh_imagehash(const char *filename, int &N, float alpha, float lvl) {
  if(filename == nullptr) {
    return nullptr;
  }
  auto hash = new uint8_t[72];// static_cast<uint8_t*>(malloc(72 * sizeof(uint8_t)));
  N = 72;

  CImageI src(filename);
  CImageI img;

  if(src.spectrum() == 3) {
    img = src.get_RGBtoYCbCr().channel(0).blur(1.0).resize(512, 512, 1, 1, 5).get_equalize(256);
  } else {
    img = src.channel(0).get_blur(1.0).resize(512, 512, 1, 1, 5).get_equalize(256);
  }
  src.clear();

  auto pkernel = GetMHKernel(alpha, lvl);
  auto fresp = img.get_correlate(*pkernel);
  img.clear();
  fresp.normalize(0, 1.0);
  CImageF blocks(31, 31, 1, 1, 0);
  for(auto rindex = 0; rindex < 31; rindex++) {
    for(auto cindex = 0; cindex < 31; cindex++) {
      blocks(rindex, cindex) = static_cast<float>(fresp.get_crop(rindex * 16, cindex * 16, rindex * 16 + 16 - 1, cindex * 16 + 16 - 1).sum());
    }
  }
  int hash_index;
  auto nb_ones = 0, nb_zeros = 0;
  auto bit_index = 0;
  unsigned char hashbyte = 0;
  for(auto rindex = 0; rindex < 31 - 2; rindex += 4) {
    CImageF subsec;
    for(auto cindex = 0; cindex < 31 - 2; cindex += 4) {
      subsec = blocks.get_crop(cindex, rindex, cindex + 2, rindex + 2).unroll('x');
      float ave = float(subsec.mean());
      cimg_forX(subsec, I) {
        hashbyte <<= 1;
        if(subsec(I) > ave) {
          hashbyte |= 0x01;
          nb_ones++;
        } else {
          nb_zeros++;
        }
        bit_index++;
        if((bit_index % 8) == 0) {
          hash_index = static_cast<int>(bit_index / 8) - 1;
          hash[hash_index] = hashbyte;
          hashbyte = 0x00;
        }
      }
    }
  }

  return hash;
}

int ph_radon_projections(const CImageI &img, int N, Projections &projs) {
  auto width = img.width();
  auto height = img.height();
  auto D = (width > height) ? width : height;
  auto x_center = static_cast<float>(width) / 2;
  auto y_center = static_cast<float>(height) / 2;
  auto x_off = static_cast<int>(std::floor(x_center + ROUNDING_FACTOR(x_center)));
  auto y_off = static_cast<int>(std::floor(y_center + ROUNDING_FACTOR(y_center)));

  projs.R = new CImageI(N, D, 1, 1, 0);
  projs.nb_pix_perline = static_cast<int*>(calloc(N, sizeof(int)));

  if(!projs.R || !projs.nb_pix_perline)
    return EXIT_FAILURE;

  projs.size = N;

  auto ptr_radon_map = projs.R;
  auto nb_per_line = projs.nb_pix_perline;

  for(auto k = 0; k < N / 4 + 1; k++) {
    auto theta = k* cimg_library::cimg::PI / N;
    auto alpha = std::tan(theta);
    for(auto x = 0; x < D; x++) {
      auto y = alpha*(x - x_off);
      auto yd = static_cast<int>(std::floor(y + ROUNDING_FACTOR(y)));
      if((yd + y_off >= 0) && (yd + y_off < height) && (x < width)) {
        *ptr_radon_map->data(k, x) = img(x, yd + y_off);
        nb_per_line[k] += 1;
      }
      if((yd + x_off >= 0) && (yd + x_off < width) && (k != N / 4) && (x < height)) {
        *ptr_radon_map->data(N / 2 - k, x) = img(yd + x_off, x);
        nb_per_line[N / 2 - k] += 1;
      }
    }
  }
  auto j = 0;
  for(auto k = 3 * N / 4; k < N; k++) {
    auto theta = k* cimg_library::cimg::PI / N;
    auto alpha = std::tan(theta);
    for(auto x = 0; x < D; x++) {
      auto y = alpha*(x - x_off);
      auto yd = static_cast<int>(std::floor(y + ROUNDING_FACTOR(y)));
      if((yd + y_off >= 0) && (yd + y_off < height) && (x < width)) {
        *ptr_radon_map->data(k, x) = img(x, yd + y_off);
        nb_per_line[k] += 1;
      }
      if((y_off - yd >= 0) && (y_off - yd < width) && (2 * y_off - x >= 0) && (2 * y_off - x < height) && (k != 3 * N / 4)) {
        *ptr_radon_map->data(k - j, x) = img(-yd + y_off, -(x - y_off) + y_off);
        nb_per_line[k - j] += 1;
      }

    }
    j += 2;
  }

  return EXIT_SUCCESS;
}

int ph_feature_vector(const Projections &projs, Features &fv) {
  auto ptr_map = projs.R;
  auto projection_map = *ptr_map;
  auto nb_perline = projs.nb_pix_perline;
  auto N = projs.size;
  auto D = projection_map.height();

  fv.features = static_cast<double*>(malloc(N * sizeof(double)));
  fv.size = N;
  if(!fv.features)
    return EXIT_FAILURE;

  auto *feat_v = fv.features;
  auto sum = 0.0;
  auto sum_sqd = 0.0;
  for(auto k = 0; k < N; k++) {
    auto line_sum = 0.0;
    auto line_sum_sqd = 0.0;
    auto nb_pixels = nb_perline[k];
    for(auto i = 0; i < D; i++) {
      line_sum += projection_map(k, i);
      line_sum_sqd += projection_map(k, i)*projection_map(k, i);
    }
    feat_v[k] = (line_sum_sqd / nb_pixels) - (line_sum*line_sum) / (nb_pixels*nb_pixels);
    sum += feat_v[k];
    sum_sqd += feat_v[k] * feat_v[k];
  }
  auto mean = sum / N;
  auto var = sqrt((sum_sqd / N) - (sum*sum) / (N*N));

  for(auto i = 0; i < N; i++) {
    feat_v[i] = (feat_v[i] - mean) / var;
  }

  return EXIT_SUCCESS;
}

int ph_dct(const Features &fv, Digest &digest) {
  auto N = fv.size;
  const auto nb_coeffs = 40;

  digest.coeffs = static_cast<uint8_t*>(malloc(nb_coeffs * sizeof(uint8_t)));
  if(!digest.coeffs)
    return EXIT_FAILURE;

  digest.size = nb_coeffs;
  auto R = fv.features;
  auto D = digest.coeffs;

  double D_temp[nb_coeffs];
  auto max = 0.0;
  auto min = 0.0;
  for(auto k = 0; k < nb_coeffs; k++) {
    auto sum = 0.0;
    for(auto n = 0; n < N; n++) {
      auto temp = R[n] * cos((cimg_library::cimg::PI*(2 * n + 1)*k) / (2 * N));
      sum += temp;
    }
    if(k == 0)
      D_temp[k] = sum / sqrt(static_cast<double>(N));
    else
      D_temp[k] = sum*SQRT_TWO / sqrt(static_cast<double>(N));
    if(D_temp[k] > max)
      max = D_temp[k];
    if(D_temp[k] < min)
      min = D_temp[k];
  }

  for(auto i = 0; i < nb_coeffs; i++) {
    D[i] = static_cast<uint8_t>(UCHAR_MAX*(D_temp[i] - min) / (max - min));
  }

  return EXIT_SUCCESS;
}

int ph_crosscorr(const Digest &x, const Digest &y, double &pcc, double threshold) {
  int N = y.size;
  int result = 0;

  uint8_t *x_coeffs = x.coeffs;
  uint8_t *y_coeffs = y.coeffs;

  double *r = new double[N];
  double sumx = 0.0;
  double sumy = 0.0;
  for(int i = 0; i < N; i++) {
    sumx += x_coeffs[i];
    sumy += y_coeffs[i];
  }
  double meanx = sumx / N;
  double meany = sumy / N;
  double max = 0;
  for(int d = 0; d < N; d++) {
    double num = 0.0;
    double denx = 0.0;
    double deny = 0.0;
    for(int i = 0; i < N; i++) {
      num += (x_coeffs[i] - meanx)*(y_coeffs[(N + i - d) % N] - meany);
      denx += pow((x_coeffs[i] - meanx), 2);
      deny += pow((y_coeffs[(N + i - d) % N] - meany), 2);
    }
    r[d] = num / sqrt(denx*deny);
    if(r[d] > max)
      max = r[d];
  }
  delete[] r;
  pcc = max;
  if(max > threshold)
    result = 1;

  return result;
}

#ifdef max
#undef max
#endif

int _ph_image_digest(const CImageI &img, double sigma, double gamma, Digest &digest, int N) {
  auto result = EXIT_FAILURE;
  CImageI graysc;
  if(img.spectrum() >= 3) {
    graysc = img.get_RGBtoYCbCr().channel(0);
  } else if(img.spectrum() == 1) {
    graysc = img;
  } else {
    return result;
  }

  graysc.blur(static_cast<float>(sigma));

  (graysc / graysc.max()).pow(gamma);

  Projections projs;
  if(ph_radon_projections(graysc, N, projs) < 0)
    goto cleanup;

  Features features;
  if(ph_feature_vector(projs, features) < 0)
    goto cleanup;

  if(ph_dct(features, digest) < 0)
    goto cleanup;

  result = EXIT_SUCCESS;

cleanup:
  free(projs.nb_pix_perline);
  free(features.features);

  delete projs.R;
  return result;
}

#define max(a,b) (((a)>(b))?(a):(b))

int ph_image_digest(const char *file, double sigma, double gamma, Digest &digest, int N) {
  CImageI src(file);
  auto result = _ph_image_digest(src, sigma, gamma, digest, N);

  return result;
}

int _ph_compare_images(const CImageI &imA, const CImageI &imB, double &pcc, double sigma, double gamma, int N, double threshold) {
  int result = 0;
  Digest digestA;
  if(_ph_image_digest(imA, sigma, gamma, digestA, N) < 0)
    goto cleanup;

  Digest digestB;
  if(_ph_image_digest(imB, sigma, gamma, digestB, N) < 0)
    goto cleanup;

  if(ph_crosscorr(digestA, digestB, pcc, threshold) < 0)
    goto cleanup;

  if(pcc > threshold)
    result = 1;

cleanup:
  free(digestA.coeffs);
  free(digestB.coeffs);

  return result;
}

int ph_compare_images(const char *file1, const char *file2, double &pcc, double sigma, double gamma, int N, double threshold) {
  CImageI *imA = new CImageI(file1);
  CImageI *imB = new CImageI(file2);

  int res = _ph_compare_images(*imA, *imB, pcc, sigma, gamma, N, threshold);

  delete imA;
  delete imB;
  return res;
}

CImageF* ph_dct_matrix(const int N) {
  auto ptr_matrix = new CImageF(N, N, 1, 1, 1 / sqrt(static_cast<float>(N)));
  const double c1 = sqrt(2.0 / N);
  for(auto x = 0; x<N; x++) {
    for(auto y = 1; y<N; y++) {
      *ptr_matrix->data(x, y) = static_cast<float>(c1*cos((cimg_library::cimg::PI / 2 / N)*y*(2 * x + 1)));
    }
  }

  return ptr_matrix;
}

int ph_dct_imagehash(const char* file, uint64_t &hash) {
  if(!file) {
    return -1;
  }
  CImageI src;
  try {
    src.load(file);
  } catch(cimg_library::CImgIOException ex) {
    return -1;
  }
  CImageF meanfilter(7, 7, 1, 1, 1);
  CImageF img;
  if(src.spectrum() == 3) {
    img = src.RGBtoYCbCr().channel(0).get_convolve(meanfilter);
  } else if(src.spectrum() == 4) {
    auto width = img.width();
    auto height = img.height();
    auto depth = img.depth();
    img = src.crop(0, 0, 0, 0, width - 1, height - 1, depth - 1, 2).RGBtoYCbCr().channel(0).get_convolve(meanfilter);
  } else {
    img = src.channel(0).get_convolve(meanfilter);
  }

  img.resize(32, 32);
  auto C = ph_dct_matrix(32);
  auto Ctransp = C->get_transpose();
  auto dctImage = (*C)*img*Ctransp;
  auto subsec = dctImage.crop(1, 1, 8, 8).unroll('x');;

  auto median = subsec.median();
  uint64_t one = 0x0000000000000001;
  hash = 0x0000000000000000;
  for(auto i = 0; i < 64; i++) {
    auto current = subsec(i);
    if(current > median)
      hash |= one;
    one = one << 1;
  }

  delete C;

  return 0;
}

#ifdef HAVE_PTHREAD

int ph_num_threads() {
  int numCPU;
#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);

  numCPU = sysinfo.dwNumberOfProcessors;
#elif __GLIBC__
  numCPU = sysconf(_SC_NPROCESSORS_ONLN);
#else
  int mib[2];
  size_t len;

  mib[0] = CTL_HW;
  mib[1] = HW_AVAILCPU;

  sysctl(mib, 2, &numCPU, &len, nullptr, 0);

  if(numCPU < 1) {
    mib[1] = HW_NCPU;
    sysctl(mib, 2, &numCPU, &len, nullptr, 0);

    if(numCPU < 1) {
      numCPU = 1;
    }
  }

#endif
  return numCPU;
}

void *ph_image_thread(void *p) {
  auto s = static_cast<slice *>(p);
  for(auto i = 0; i < s->n; ++i) {
    auto dp = static_cast<DP *>(s->hash_p[i]);
    uint64_t hash;
    if (ph_dct_imagehash(dp->id, hash) < 0) {

      continue;
    }

    dp->hash = static_cast<uint64_t*>(malloc(sizeof(hash)));
    memcpy(dp->hash, &hash, sizeof(hash));
    dp->hash_length = 1;
  }

  return nullptr;
}

DP** ph_dct_image_hashes(char *files[], int count, int threads) {
  if(!files || count <= 0)
    return nullptr;

  int num_threads;
  if(threads > count) {
    num_threads = count;
  } else if(threads > 0) {
    num_threads = threads;
  } else {
    num_threads = ph_num_threads();
  }

  auto hashes = static_cast<DP**>(malloc(count * sizeof(DP*)));

  for(auto i = 0; i < count; ++i) {
    hashes[i] = static_cast<DP *>(malloc(sizeof(DP)));
    hashes[i]->id = strdup(files[i]);
  }

  ASSERT(num_threads <= MAX_THREADS);
  auto thds = std::array<pthread_t, MAX_THREADS>();

  auto rem = count % num_threads;
  auto start = 0;
  auto s = new slice[num_threads];

  for(auto n = 0; n < num_threads; ++n) {
    auto off = static_cast<int>(floor((count / static_cast<float>(num_threads)) + (rem > 0 ? num_threads - (count % num_threads) : 0)));

    s[n].hash_p = &hashes[start];
    s[n].n = off;
    s[n].hash_params = nullptr;
    start += off;
    --rem;
    pthread_create(&thds[n], nullptr, ph_image_thread, &s[n]);
  }

  for(auto i = 0; i < num_threads; ++i) {
    pthread_join(thds[i], nullptr);
  }

  delete[] s;

  return hashes;
}

#endif /* HAVE_PTHREAD */

#ifdef HAVE_VIDEO_HASH

uint64_t* ph_dct_videohash(const char *filename, int &Length) {
  auto keyframes = VideoProcessor::GetSceneChangeFrames(filename);
  if(keyframes == nullptr) {
    return nullptr;
  }

  Length = keyframes->size();
  
  auto hash = static_cast<uint64_t*>(malloc(sizeof(uint64_t)*Length));
  auto C = ph_dct_matrix(32);
  auto Ctransp = C->get_transpose();
  CImageF dctImage;
  CImageF subsec;
  CImageI currentframe;


  for(unsigned int i = 0; i < keyframes->size(); i++) {
    currentframe = keyframes->at(i);
    currentframe.blur(1.0);
    dctImage = (*C)*(currentframe)*Ctransp;
    subsec = dctImage.crop(1, 1, 8, 8).unroll('x');
    auto med = subsec.median();
    hash[i] = 0x0000000000000000;
    uint64_t one = 0x0000000000000001;
    for(auto j = 0; j < 64; j++) {
      if(subsec(j) > med)
        hash[i] |= one;
      one = one << 1;
    }
  }

  keyframes->clear();
  delete C;

  return hash;
}

#ifdef HAVE_PTHREAD

void *ph_video_thread(void *p) {
  auto s = static_cast<slice *>(p);
  for(auto i = 0; i < s->n; ++i) {
    auto dp = static_cast<DP *>(s->hash_p[i]);
    int N;
    auto hash = ph_dct_videohash(dp->id, N);
    if(hash) {
      dp->hash = hash;
      dp->hash_length = N;
    } else {
      dp->hash = nullptr;
      dp->hash_length = 0;
    }
  }

  return nullptr;
}

DP** ph_dct_video_hashes(char *files[], int count, int threads) {
  if(!files || count <= 0)
    return nullptr;

  int num_threads;
  if(threads > count) {
    num_threads = count;
  } else if(threads > 0) {
    num_threads = threads;
  } else {
    num_threads = ph_num_threads();
  }

  auto hashes = static_cast<DP**>(malloc(count * sizeof(DP*)));

  for(auto i = 0; i < count; ++i) {
    hashes[i] = static_cast<DP *>(malloc(sizeof(DP)));
    hashes[i]->id = strdup(files[i]);
  }

  ASSERT(num_threads <= MAX_THREADS);
  auto thds = new pthread_t[num_threads];

  auto rem = count % num_threads;
  auto start = 0;
  auto s = new slice[num_threads];
  for(auto n = 0; n < num_threads; ++n) {
    auto off = static_cast<int>(floor((count / static_cast<float>(num_threads)) + (rem>0 ? num_threads - (count % num_threads) : 0)));

    s[n].hash_p = &hashes[start];
    s[n].n = off;
    s[n].hash_params = nullptr;
    start += off;
    --rem;
    pthread_create(&thds[n], nullptr, ph_video_thread, &s[n]);
  }

  for(auto i = 0; i < num_threads; ++i) {
    pthread_join(thds[i], nullptr);
  }

  delete[] s;
  delete[] thds;

  return hashes;
}

#endif /* HAVE_PTHREAD */

inline int matrix_size(int zx, int zy) {
  return (((zx)+1) * ((zy)+1));
}

inline int matrix_pos(int zx, int x, int y) {
  return ((x) * (((zx)+1) + (y)));
}

double ph_dct_videohash_dist(uint64_t* hashA, int N1, uint64_t* hashB, int N2, int threshold) {
  int den = (N1 <= N2) ? N1 : N2;
  auto* m = new int[matrix_size(N1, N2)];

  for(auto i = 0; i < N1 + 1; i++) {
    m[matrix_pos(N1, i, 0)] = 0;
  }

  for(auto j = 0; j < N2 + 1; j++) {
    m[matrix_pos(N1, 0, j)] = 0;
  }

  for(auto i = 1; i < N1 + 1; i++) {
    for(auto j = 1; j < N2 + 1; j++) {
      auto d = ph_hamming_distance(hashA[i - 1], hashB[j - 1]);

      if(d <= threshold) {
        m[matrix_pos(N1, i, j)] = m[matrix_pos(N1, i - 1, j - 1)] + 1;
      } else {
        m[matrix_pos(N1, i, j)] =
          ((m[matrix_pos(N1, i - 1, j)] >= m[matrix_pos(N1, i, j - 1)]))
          ? m[matrix_pos(N1, i - 1, j)]
          : m[matrix_pos(N1, i, j - 1)];
      }
    }
  }

  auto result = static_cast<double>(m[matrix_pos(N1, N1, N2)]) / static_cast<double>(den);
  delete[] m;

  return result;
}

#endif /* HAVE_VIDEO_HASH */

#endif /* HAVE_IMAGE_HASH */

int ph_hamming_distance(const uint64_t hash1, const uint64_t hash2) {
  uint64_t x = hash1^hash2;
  const uint64_t m1 = 0x5555555555555555ULL;
  const uint64_t m2 = 0x3333333333333333ULL;
  const uint64_t h01 = 0x0101010101010101ULL;
  const uint64_t m4 = 0x0f0f0f0f0f0f0f0fULL;
  x -= (x >> 1) & m1;
  x = (x & m2) + ((x >> 2) & m2);
  x = (x + (x >> 4)) & m4;
  return (x * h01) >> 56;
}

DP* ph_malloc_datapoint(int hashtype) {
  auto dp = static_cast<DP*>(malloc(sizeof(DP)));
  dp->hash = nullptr;
  dp->id = nullptr;
  dp->hash_type = hashtype;
  return dp;
}

void ph_free_datapoint(DP *dp) {
  if(!dp) {
    return;
  }
  free(dp);

  return;
}

char** ph_readfilenames(const char *dirname, int &count) {
  count = 0;
  struct dirent *dir_entry;
  auto dir = opendir(dirname);
  if(!dir)
    return nullptr;

  /*count files */
  while((dir_entry = readdir(dir)) != nullptr) {
    if(strcmp(dir_entry->d_name, ".") && strcmp(dir_entry->d_name, ".."))
      count++;
  }

  /* alloc list of files */
  char** files = static_cast<char**>(malloc(count * sizeof(*files)));
  if(!files)
    return nullptr;

  errno = 0;
  auto index = 0;
  char path[1024];
  path[0] = '\0';
  rewinddir(dir);
  while((dir_entry = readdir(dir)) != nullptr) {
    if(strcmp(dir_entry->d_name, ".") && strcmp(dir_entry->d_name, "..")) {
      strcat(path, dirname);
      strcat(path, "/");
      strcat(path, dir_entry->d_name);
      files[index++] = strdup(path);
    }
    path[0] = '\0';
  }
  if(errno)
    return nullptr;
  closedir(dir);
  return files;
}

int ph_bitcount8(uint8_t val) {
  auto num = 0;
  while(val) {
    ++num;
    val &= val - 1;
  }

  return num;
}

double ph_hammingdistance2(uint8_t *hashA, int lenA, uint8_t *hashB, int lenB) {
  if(lenA != lenB) {
    return -1.0;
  }
  if((hashA == nullptr) || (hashB == nullptr) || (lenA <= 0)) {
    return -1.0;
  }
  double dist = 0;
  for(auto i = 0; i < lenA; i++) {
    uint8_t D = hashA[i] ^ hashB[i];
    dist = dist + double(ph_bitcount8(D));
  }

  auto bits = static_cast<double>(lenA) * 8;
  return dist / bits;
}

TxtHashPoint* ph_texthash(const char *filename, int *nbpoints) {
  int count;
  TxtHashPoint *TxtHash;
  TxtHashPoint WinHash[WindowLength];
  char kgram[KgramLength];

  auto pfile = fopen(filename, "r");
  if(!pfile) {
    return nullptr;
  }
  struct stat fileinfo;
  fstat(fileno(pfile), &fileinfo);
  count = fileinfo.st_size - WindowLength + 1;
  count = static_cast<int>(0.01*count);
  int d;
  uint64_t hashword = 0ULL;

  TxtHash = static_cast<TxtHashPoint*>(malloc(count * sizeof(struct ph_hash_point)));
  if(!TxtHash) {
    return nullptr;
  }
  *nbpoints = 0;
  int i, first = 0, last = KgramLength - 1;
  int text_index = 0;
  int win_index = 0;
  for(i = 0; i < KgramLength; i++) {    /* calc first kgram */
    d = fgetc(pfile);
    if(d == EOF) {
      free(TxtHash);
      return nullptr;
    }
    if(d <= 47)         /*skip cntrl chars*/
      continue;
    if(((d >= 58) && (d <= 64)) || ((d >= 91) && (d <= 96)) || (d >= 123)) /*skip punct*/
      continue;
    if((d >= 65) && (d <= 90))       /*convert upper to lower case */
      d = d + 32;

    kgram[i] = static_cast<char>(d);
    hashword = hashword << delta;   /* rotate left or shift left ??? */
    hashword = hashword^textkeys[d];/* right now, rotate breaks it */
  }

  WinHash[win_index].hash = hashword;
  WinHash[win_index++].index = text_index;
  struct ph_hash_point minhash;
  minhash.hash = ULLONG_MAX;
  minhash.index = 0;
  struct ph_hash_point prev_minhash;
  prev_minhash.hash = ULLONG_MAX;
  prev_minhash.index = 0;

  while((d = fgetc(pfile)) != EOF) {    /*remaining kgrams */
    text_index++;
    if(d == EOF) {
      free(TxtHash);
      return nullptr;
    }
    if(d <= 47)         /*skip cntrl chars*/
      continue;
    if(((d >= 58) && (d <= 64)) || ((d >= 91) && (d <= 96)) || (d >= 123)) /*skip punct*/
      continue;
    if((d >= 65) && (d <= 90))       /*convert upper to lower case */
      d = d + 32;

    uint64_t oldsym = textkeys[kgram[first%KgramLength]];

    /* rotate or left shift ??? */
    /* right now, rotate breaks it */
    oldsym = oldsym << delta*KgramLength;
    hashword = hashword << delta;
    hashword = hashword^textkeys[d];
    hashword = hashword^oldsym;
    kgram[last%KgramLength] = static_cast<char>(d);
    first++;
    last++;

    WinHash[win_index%WindowLength].hash = hashword;
    WinHash[win_index%WindowLength].index = text_index;
    win_index++;

    if(win_index >= WindowLength) {
      minhash.hash = ULLONG_MAX;
      for(i = win_index; i<win_index + WindowLength; i++) {
        if(WinHash[i%WindowLength].hash <= minhash.hash) {
          minhash.hash = WinHash[i%WindowLength].hash;
          minhash.index = WinHash[i%WindowLength].index;
        }
      }
      if(minhash.hash != prev_minhash.hash) {
        TxtHash[(*nbpoints)].hash = minhash.hash;
        TxtHash[(*nbpoints)++].index = minhash.index;
        prev_minhash.hash = minhash.hash;
        prev_minhash.index = minhash.index;

      } else {
        TxtHash[*nbpoints].hash = prev_minhash.hash;
        TxtHash[(*nbpoints)++].index = prev_minhash.index;
      }
      win_index = 0;
    }
  }

  fclose(pfile);
  return TxtHash;
}

TxtMatch* ph_compare_text_hashes(TxtHashPoint *hash1, int N1, TxtHashPoint *hash2, int N2, int *nbmatches) {
  auto max_matches = (N1 >= N2) ? N1 : N2;
  auto found_matches = static_cast<TxtMatch*>(malloc(max_matches * sizeof(TxtMatch)));
  if(!found_matches) {
    return nullptr;
  }

  *nbmatches = 0;
  int i, j;
  for(i = 0; i<N1; i++) {
    for(j = 0; j<N2; j++) {
      if(hash1[i].hash == hash2[j].hash) {
        auto m = i + 1;
        auto n = j + 1;
        auto cnt = 1;
        while((m < N1) && (n < N2) && (hash1[m++].hash == hash2[n++].hash)) {
          cnt++;
        }
        found_matches[*nbmatches].first_index = i;
        found_matches[*nbmatches].second_index = j;
        found_matches[*nbmatches].length = cnt;
        (*nbmatches)++;
      }
    }
  }
  return found_matches;
}

