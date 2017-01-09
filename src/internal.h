#ifndef PHASH_INTERNAL_H
#define PHASH_INTERNAL_H

#include "../config.h"
#include <memory>
#include <cstdint>
#if defined(HAVE_CRTDBG_H)
#  include <crtdbg.h>
#endif

#define STRINGIZE_(x)      #x
#define STRINGIZE(x)       STRINGIZE_(x)

#define WIDE_(s)           L ## s
#define WIDE(s)            WIDE_(s)

#define CONCATENATE_(a, b) a ## b
#define CONCATENATE(a, b)  CONCATENATE_(a, b)

#if defined(_MSC_VER)
#  define WARNING(msg)      __pragma(message(__FILE__ "("STRINGIZE(__LINE__)") : Warning Msg: "STRINGIZE(msg)))
#elif defined(__GNUC__) || defined(__clang__)
#  define WARNING(msg)     (void)(msg)
#else
#  define WARNING(msg)     (void)(msg)
#endif

#ifdef _DEBUG
#  if defined(_MSC_VER)
#    define ASSERT(e)      ( _ASSERT(e) )
#  elif defined(__clang__)
#    define ASSERT(e)      ( assert(e) )
#  else
#    define ASSERT(e)      (void)(e)
#  endif
#else
#  define ASSERT(e)        (void)(e)
#endif

#define NOEXCEPT           noexcept
#define NOEXCEPT_OP(x)     noexcept(x)

#ifndef M_PI
#define M_PI               3.1415926535897932
#endif
#define SQRT_TWO           1.4142135623730950488016887242097

#define ROUNDING_FACTOR(x) (((x) >= 0) ? 0.5 : -0.5) 

#if defined(HAVE_IMAGE_HASH) || defined(HAVE_VIDEO_HASH)

#if defined(HAVE_LIBJPEG)
#define cimg_use_jpeg              1
#endif
#if defined(HAVE_LIBPNG)
#define cimg_use_png               1
#endif
#if defined(HAVE_LIBZ)
#define cimg_use_zlib               1
#endif
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

#include "CImg.h"

using CImageIList    = cimg_library::CImgList<uint8_t>;
using CImageI        = cimg_library::CImg<uint8_t>;
using CImageF        = cimg_library::CImg<float>;
using CImageIListPtr = std::shared_ptr<CImageIList>;
using CImageIPtr     = std::shared_ptr<CImageI>;
using CImageFPtr     = std::shared_ptr<CImageF>;

#endif /* HAVE_IMAGE_HASH || HAVE_VIDEO_HASH */

enum ph_action : short;
enum ph_error : short;

/**
 * @brief Notify callback manager of an error which occurred.
 *
 * @param action The ph_action being executed when the error occurred.
 * @param error  The ph_error which occurred.
 */
void ph_notify_error(const ph_action action, const ph_error error);

/**
 * @brief Notify callback manager of a status update.
 *
 * @param action  The ph_action being executed.
 * @param percent The percent complete for the specified action.
 */
void ph_notify_status(const ph_action action, int percent);

#endif /* PHASH_INTERNAL_H */
