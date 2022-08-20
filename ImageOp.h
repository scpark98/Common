#ifndef IMAGE_OP_H
#define IMAGE_OP_H

int ARGB2Pixel(int alpha, int r, int g, int b);
void pixel2ARGB(int pixel, int &alpha, int &red, int &green, int &blue);
void pixel2ARGBMask(int pixel, int &alpha, int &red, int &green, int &blue, COLORREF rRGB, int nAlpha );

//class ImageOp {
//public :
void GrayScaleOP(int width, int height, int *argb);
void NegativeOP(int width, int height, int *argb);
void BrightnessOP(int width, int height, int *argb, int nValue);
void ThresholdOP(int width, int height, int *argb, int level);
void MaskImageOP( int width, int height, int *argb, COLORREF rRGB, int nAlpha );
//};

#endif IMAGE_OP_H