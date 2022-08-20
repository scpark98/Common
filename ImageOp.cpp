#include "StdAfx.h"
#include "ImageOp.h"
#include "math.h"


#define AVERAGE(a,b,c) ( ((a)+(b)+(c))/3 )
#define GRAYSCALE(a,b,c) ( ((a)*0.3+(b)*0.59+(c)*0.11) )
#define BLACK 0
#define WHITE 255


int ARGB2Pixel(int alpha, int r, int g, int b)
{
	if ( r > 255 ) r = 255; if ( r < 0 ) r = 0; r = (r << 16); 
	if ( g > 255 ) g = 255; if ( g < 0 ) g = 0; g = (g << 8 ); 
	if ( b > 255 ) b = 255; if ( b < 0 ) b = 0;
	return ( ((alpha << 24 )&0xff000000)  | r | g| b );
}

inline void pixel2ARGB(int pixel, int &alpha, int &red, int &green, int &blue)
{
	alpha = (pixel & 0xff000000) >> 24;
	red = (pixel   & 0x00ff0000) >> 16;
	green = (pixel & 0x0000ff00) >> 8;
	blue = (pixel  & 0x000000ff) >> 0;
}

inline void pixel2ARGBMask(int pixel, int &alpha, int &red, int &green, int &blue, COLORREF rRGB, int nAlpha )
{
	red = (pixel   & 0x00ff0000) >> 16;
	green = (pixel & 0x0000ff00) >> 8;
	blue = (pixel  & 0x000000ff) >> 0;

	// red, green, blue를 우선 추출한 후 그 값이 마스크와 같으면 알파처리한다.
	if ( RGB(red,green,blue) == rRGB )
		alpha = 0;//(pixel & 0xff000000) >> 24;
	else
		alpha = (pixel & 0xff000000) >> 24;
}

void MaskImageOP( int width, int height, int *argb, COLORREF rRGB, int nAlpha )
{
	int a, r, g, b;
	for ( int i=0; i < width*height; i++)
	{
		pixel2ARGBMask( argb[i], a,r,g,b, rRGB, nAlpha );
		argb[i] = ARGB2Pixel(a,r,g,b);
	}
}

void GrayScaleOP(int width, int height, int *argb) 
{
	int a, r, g, b;
	for ( int i=0; i < width*height; i++)
	{
		pixel2ARGB(argb[i], a,r,g,b);
		r = GRAYSCALE(r,g,b);
		argb[i] = ARGB2Pixel(a,r,r,r);
	}
}

void NegativeOP(int width, int height, int *argb) 
{
	int a, r, g, b;
	for ( int i=0; i < width*height; i++)
	{
		pixel2ARGB(argb[i], a,r,g,b);
		r = 255-r;
		g = 255-g;
		b = 255-b;
		argb[i] = ARGB2Pixel(a,r,g,b);
	}
}

void BrightnessOP(int width, int height, int *argb, int nValue) 
{
	int a, r, g, b;
	for ( int i=0; i < width*height; i++)
	{
		pixel2ARGB(argb[i], a,r,g,b);
		r = r + nValue;
		g = g + nValue;
		b = b + nValue;
		argb[i] = ARGB2Pixel(a,r,g,b);
	}
}

void ThresholdOP(int width, int height, int *argb, int level)
{
	int a, r, g, b;
	for ( int i=0; i < width*height; i++)
	{
		pixel2ARGB(argb[i], a,r,g,b);
		r = AVERAGE(r,g,b);
		r = (r<level)?0:255;
		argb[i] = ARGB2Pixel(a,r,r,r);
	}
}
