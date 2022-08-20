#ifndef __CANNY_H__
#define __CANNY_H__



//2017-03-02 Jwc

//#ifdef __cplusplus
//extern "C"{
//#endif

extern void gaussian_bit(unsigned char* dst , const unsigned char *src,const short sw, const short sh);

extern void function_canny_main(
	unsigned char *dst,
	const unsigned char *src,
	const unsigned short width,
	const unsigned short height,
	const unsigned char lowthres, 
	const unsigned char highthres
	);

extern void CannyEdge(unsigned char *src, unsigned char *dst, int width, int height, int th_low, int th_high );

//이미지선언

//#ifdef __cplusplus
//}
//#endif

#endif
