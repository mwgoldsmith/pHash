AM_CXXFLAGS = -std=c++11
AM_CPPFLAGS = -I$(top_builddir)/include

lib_LTLIBRARIES = libphash.la
libphash_la_SOURCES = phash.cpp callbacks.cpp MediaContext.cpp
libphash_la_LDFLAGS = -no-undefined
include_HEADERS = phash.h callbacks.h

if HAVE_AUDIO_HASH
libphash_la_SOURCES += audiophash.cpp ph_fft.c
endif

if HAVE_VIDEO_HASH
libphash_la_SOURCES += VideoProcessor.cpp
endif

