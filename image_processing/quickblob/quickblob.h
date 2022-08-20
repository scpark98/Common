#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

// gcc -O2 -c quickblob.c -o quickblob.o
// ar rvs quickblob.a quickblob.o

// licensed LGPL
#ifndef uint8_t 
#define uint8_t unsigned char
#endif

/*
ABOUT
    image is streamed row by row
        stream_state keeps track of this
    blobs are assembled incrementally
    complete blobs are passed to log_blob_hook
    see more details in quickblob.c
*/

/* some structures you'll be working with */

struct blob
// you'll probably only need size, color, center_x and center_y
{
    int size;
    int color;
    // track current line segment
    int x1;
    int x2;
    int y;
    // basic linked list
    struct blob* prev;
    struct blob* next;
    // siblings are connected segments
    struct blob* sib_p;
    struct blob* sib_n;
    // incremental average
    double center_x;
    double center_y;
    // bounding box
    int bb_x1, bb_y1, bb_x2, bb_y2;
    // single linked list for tracking all old pixels
    // struct blob* old;
};

struct stream_state
// make a struct to hold an state required by the image loader
// and reference in the handle pointer
{
    int x, y, w, h;
    int wrap;  // don't touch this
	uint8_t threshold;
    uint8_t* row;
    uint8_t* data;
};

class CBlobs
{
public:
	int x1, y1, x2, y2;
	int cx;
	int cy;
	int size;
	int color;

	CBlobs( int x1_, int y1_, int x2_, int y2_, int cx1, int cy1, int size1, int color1 )
	{
		x1 = x1_;
		y1 = y1_;
		x2 = x2_;
		y2 = y2_;
		cx = cx1;
		cy = cy1;
		size = size1;
		color = color1;
	}
};

extern std::vector<CBlobs> vt_result;

/* these are the functions you need to define
 * you get void pointer for passing around useful data */

void log_blob_hook(struct blob* b);
// the blob struct will be for a completely finished blob
// you'll probably want to printf() important parts
// or write back to something in user_struct

bool quick_blobs( std::vector<CBlobs> &blobs, uint8_t *data, int w, int h, uint8_t threshold );
