FAST feature detectors in C Version 2.0
---------------------------------------

The files are valid C and C++ code, and have no special requirements for
compiling, and they do not depend on any libraries. Just compile them along with
the rest of your project. 

To use the functions, #include "fast.h"

The corner detectors have the following prototype (where X is 9, 10, 11 or 12):

xy* fastX_detect_nonmax(const unsigned char * data, int xsize, int ysize, int stride, int threshold, int* numcorners)

Where xy is the following simple struct typedef:

typedef struct 
{
	int x, y;
} xy;

The image is passed in as a block of data and dimensions, and the list of
corners is returned as an array of xy structs, and an integer (numcorners)
with the number of corners returned.  The data can be deallocated with free().
Nonmaximal suppression is performed on the corners. Note that the stride 
is the number of bytes between rows. If your image has no padding, then this
is the same as xsize.

The detection, scoring and nonmaximal suppression are available as individual
functions.  To see how to use the individual functions, see fast.c

# 주의 : visual studio에서 실행할 경우 모두 c 또는 cpp로 통일해서 컴파일해야 한다.
