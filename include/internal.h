#ifndef PHASH_INTERNAL_H
#define PHASH_INTERNAL_H

#include "config.h"
#include <memory>
#include <cstdint>

#if defined(HAVE_IMAGE_HASH) || defined(HAVE_VIDEO_HASH)

#if defined(HAVE_LIBJPEG)
#define cimg_use_jpeg              1
#endif
#if defined(HAVE_LIBPNG)
#define cimg_use_png               1
#endif
//#define cimg_use_zlib              1
/* #undef cimg_use_ffmpeg */
/* #undef cimg_use_opencv */
/* #undef cimg_use_tiff */
/* #undef cimg_use_magick */
/* #undef cimg_use_fftw3 */
/* #undef cimg_use_lapack */

/** Hide library messages(quiet mode). */
#define CIMG_MSG_LEVEL_QUIET       0
/** Print library messages on the console. */
#define CIMG_MSG_LEVEL_CONSOLE     1
/** Display library messages on a dialog window(default behavior). */
#define CIMG_MSG_LEVEL_DIALOG      2
/** Do as CIMG_MSG_LEVEL_CONSOLE + add extra debug warnings(slow down the code!). */
#define CIMG_MSG_LEVEL_CONSOLE_DBG 3
/** Do as CIMG_MSG_LEVEL_DIALOG + add extra debug warnings(slow down the code!). */
#define CIMG_MSG_LEVEL_DIALOG_DBG  4

#ifdef _DEBUG
#  define cimg_verbosity           CIMG_MSG_LEVEL_CONSOLE_DBG
#else
#  define cimg_verbosity           CIMG_MSG_LEVEL_QUIET
#endif

/** Disable display capabilities. */
#define CIMG_DISPLAY_DISABLE       0 
/** Use the X-Window framework (X11). */
#define CIMG_DISPLAY_X11           1 
/** Use the Microsoft GDI32 framework. */
#define CIMG_DISPLAY_WIN32         2

#define cimg_display               CIMG_DISPLAY_DISABLE
#define cimg_debug                 0

#include <CImg.h>

using CImageIList    = cimg_library::CImgList<uint8_t>;
using CImageI        = cimg_library::CImg<uint8_t>;
using CImageF        = cimg_library::CImg<float>;
using CImageIListPtr = std::shared_ptr<CImageIList>;
using CImageIPtr     = std::shared_ptr<CImageI>;
using CImageFPtr     = std::shared_ptr<CImageF>;
#endif /* HAVE_IMAGE_HASH || HAVE_VIDEO_HASH */

/* 
* @brief Radon Projection info
*/
#ifdef HAVE_IMAGE_HASH
typedef struct ph_projections {
  CImageI   *R;           //contains projections of image of angled lines through center
  int *nb_pix_perline;        //the head of int array denoting the number of pixels of each line
  int size;                   //the size of nb_pix_perline
} Projections;
#endif

#endif
