#
# 2010/12/16 by D. Grant Starkweather
#

MVPVERSION = 0.0.0

HFLS	= mediahash.h MediaFile.h VideoFile.h Hash.h
OBJS	= mediahash.o MediaFile.o VideoFile.o Hash.o

CXX	= g++

CFLAGS	= -g -O3 -std=c++11 -I. $(DEFINES)

#edit these lines to reflect the location of the pHash header files (pHash.h)
CPPFLAGS = -pthread -I /storage/include/phash 

#location of phash-config.h
#CPPFLAGS += -I /usr/local/include

LDFLAGS	=
RANLIB	= ranlib

DESTDIR	= /storage
TEST	= mediahash

DEPS_LIBS = -lm
PHASH_LIBS = -L/storage/lib -lpHash


#uncomment if you are using prng library for random number generation
#DEPS_LIBS += /usr/local/lib/libprng.a


all : $(TEST)

clean :
	rm -f a.out core *.o *.t
	rm -f $(UTIL) $(TEST) 

$(TEST) : $(OBJS)
	rm -f $@
	g++ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DEPS_LIBS) $(PHASH_LIBS) $(OBJS)
	mv a.out $@

.cpp.o :
	rm -f $@
	g++ $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
