#ifndef FAST_H
#define FAST_H

#ifndef xy
typedef struct
{
	int x, y;
} xy; 
#endif

#ifndef byte
typedef unsigned char byte;
#endif


int fast09_corner_score(const byte* p, const int pixel[], int bstart);
//int fast10_corner_score(const byte* p, const int pixel[], int bstart);
//int fast11_corner_score(const byte* p, const int pixel[], int bstart);
//int fast12_corner_score(const byte* p, const int pixel[], int bstart);

xy* fast09_detect(const byte* im, int xsize, int ysize, int stride, int b, int* ret_num_corners);
//xy* fast10_detect(const byte* im, int xsize, int ysize, int stride, int b, int* ret_num_corners);
//xy* fast11_detect(const byte* im, int xsize, int ysize, int stride, int b, int* ret_num_corners);
//xy* fast12_detect(const byte* im, int xsize, int ysize, int stride, int b, int* ret_num_corners);

int* fast09_score(const byte* i, int stride, xy* corners, int num_corners, int b);
//int* fast10_score(const byte* i, int stride, xy* corners, int num_corners, int b);
//int* fast11_score(const byte* i, int stride, xy* corners, int num_corners, int b);
//int* fast12_score(const byte* i, int stride, xy* corners, int num_corners, int b);


xy* fast09_detect_nonmax(const byte* im, int xsize, int ysize, int stride, int* b, int* ret_num_corners, int nMax, bool bUseNonmax = true);
//xy* fast10_detect_nonmax(const byte* im, int xsize, int ysize, int stride, int* b, int* ret_num_corners, int nMax);
//xy* fast11_detect_nonmax(const byte* im, int xsize, int ysize, int stride, int* b, int* ret_num_corners, int nMax);
//xy* fast12_detect_nonmax(const byte* im, int xsize, int ysize, int stride, int* b, int* ret_num_corners, int nMax);

xy* nonmax_suppression(const xy* corners, const int* scores, int num_corners, int* ret_num_nonmax, int* ret_score);

void swap(int &a, int &b);
void swapxy(xy &a, xy &b);
void sort(xy* corners, int* scores, int num);

#endif
