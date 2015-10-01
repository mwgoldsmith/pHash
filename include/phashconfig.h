#ifndef PHASHCONFIG_H
#define PHASHCONFIG_H

/* configure with audio hash */
#define HAVE_AUDIO_HASH            0

#if HAVE_AUDIO_HASH
#define HAVE_LIBMPG123             0
#endif

/* configure with video hash */
#define HAVE_VIDEO_HASH            1

/* configure with image hash */
#define HAVE_IMAGE_HASH            1

/* configure with pthread support */
#define HAVE_PTHREAD               1

#if HAVE_IMAGE_HASH || HAVE_VIDEO_HASH

#define cimg_use_jpeg              1
#define cimg_use_png               1
#define cimg_use_zlib              1
//#define cimg_use_ffmpeg
//#define cimg_use_opencv
//#define cimg_use_tiff
//#define cimg_use_magick
//#define cimg_use_fftw3
//#define cimg_use_lapack

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

#endif

#endif /* PHASHCONFIG_H */