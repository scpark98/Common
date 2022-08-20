#include "stdafx.h"
#include "OpenCVFunctions.h"
#include "Functions.h"

IplImage*	scvCutRect( IplImage* pImage, CRect r )
{
	return scvCutRect( pImage, r.left, r.top, r.Width(), r.Height() );
}

IplImage*	scvCutRect( IplImage* pImage, int x, int y, int w, int h )
{
	CRgn	rgnImage;
	CRect	rImage( 0, 0, pImage->width, pImage->height );
	CRect	rCutRect( x, y, x + w, y + h );

	rgnImage.CreateRectRgnIndirect( &rImage );

	//자르고자 하는 사각형이 이미지 영역을 벗어난 경우
	if ( rgnImage.RectInRegion( rCutRect ) == false )
	{
		rgnImage.DeleteObject();
		return NULL;
	}

	rgnImage.DeleteObject();

	if ( w == 0 || h == 0 )
		return NULL;

	if ( rCutRect.left > rCutRect.right )
		return NULL;

	if ( rCutRect.top > rCutRect.bottom )
		return NULL;

	if ( rCutRect.right > pImage->width )
		return NULL;

	if ( rCutRect.bottom > pImage->height )
		return NULL;

	//ROI를 이용한 방식
	cvSetImageROI( pImage, cvRect( x, y, w, h ) );
	IplImage* pDestImage = cvCreateImage( cvSize( w, h ), pImage->depth, pImage->nChannels );
	cvCopy( pImage, pDestImage );
	cvResetImageROI( pImage );

	return pDestImage;

	/*
	//////////////////////////////////////////////////////////////////////////
	//직접 픽셀 처리 방식

	int			i, j, k;
	int			sw = pImage->width;
	int			sh = pImage->height;
	int			sws = pImage->widthStep;
	int			ch = pImage->nChannels;

	IplImage*	pDestImage = cvCreateImage( cvSize( w, h ), pImage->depth, ch );
	char*		pSrc = pImage->imageData;
	char*		pDst = pDestImage->imageData;
	int			dws = pDestImage->widthStep;

	BYTE*		pData;

	for ( i = y; i < y + h; i++ )
	{
	pData = (BYTE*)(pSrc + i * sws);

	for ( j = x; j < x + w; j++ )
	{
	for ( k = 0; k < ch; k++ )
	{
	pDst[ (i-y) * dws + (j-x) * ch + k ] = pData[ j * ch + k ] ;
	//pDst[ (i-y) * dws + (j-x) * ch + k ] = pSrc[ i * sws + j * ch + k ];
	}
	}
	}

	return pDestImage;
	*/
}

BITMAPINFO	scvGetBitmapInfo( IplImage* pImage )
{
	BITMAPINFO bitmapInfo;

	bitmapInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biPlanes=1;
	bitmapInfo.bmiHeader.biCompression=BI_RGB;
	bitmapInfo.bmiHeader.biXPelsPerMeter=96;
	bitmapInfo.bmiHeader.biYPelsPerMeter=96;
	bitmapInfo.bmiHeader.biClrUsed=0;
	bitmapInfo.bmiHeader.biClrImportant=0;
	bitmapInfo.bmiHeader.biSizeImage=0;
	bitmapInfo.bmiHeader.biWidth=pImage->width;
	bitmapInfo.bmiHeader.biHeight=-pImage->height;
	bitmapInfo.bmiHeader.biBitCount = pImage->depth * pImage->nChannels;
	/*
	IplImage* tempImage = NULL;

	if(pImage->nChannels == 3)
	{
	tempImage = (IplImage*)cvClone(pImage);
	bitmapInfo.bmiHeader.biBitCount=tempImage->depth * tempImage->nChannels;

	}
	else if(pImage->nChannels == 1)
	{
	tempImage =  cvCreateImage(cvGetSize(pImage), IPL_DEPTH_8U, 3);
	cvCvtColor(pImage, tempImage, CV_GRAY2BGR);
	bitmapInfo.bmiHeader.biBitCount = pImage->depth * tempImage->nChannels;
	}

	cvReleaseImage(&tempImage);
	*/
	return bitmapInfo;
}

CRect scvDrawImage(CDC* pDC, Mat src, CRect target_rect, COLORREF crBack, bool fit)
{
	return scvDrawImage(pDC, src, 0, 0, 0, 0, &target_rect, crBack, fit);
}

//StretchDIBits 방식으로 뿌릴때 이미지의 width가 4의 배수가 아니면
//밀려서 출력되거나 아예 출력되지 않는다.
//따라서 4의 배수가 아니면 복사본을 4의 배수로 만들어주고 출력시켜준다.
//단 소스의 width를 변경해서는 안된다.
//MatToCImage로 변환해서 뿌리면 이와 같은 처리는 안해도 되지만
//픽셀단위의 복사가 이루어지므로 속도가 약 3배 정도 늘어나는 단점이 있다.
//실제로 그려진 영역 좌표값을 리턴한다.
CRect scvDrawImage(CDC* pDC, Mat src, int dx, int dy, int dw, int dh, CRect *target_rect, COLORREF crBack, bool fit)
{
	int sx, sy, sw, sh;
	int padding = 0;
	Mat	result( src );

	if ( src.cols % 4 != 0 )
		padding = matMake4Width( result );

	sx = sy = 0;
	sw = result.cols;
	sh = result.rows;

	if (dw == 0)
		dw = sw;
	if (dh == 0)
		dh = sh;

	if ( result.channels() == 1 )
		cvtColor( result, result, CV_GRAY2BGR );

	int ch = result.channels();
	int dep= result.depth();

	//src가 4ch 이미지이면 투명 표시 배경이미지를 생성하고 그 위에 src를 blending해서 보여줘야 한다.
	if (result.channels() == 4)
	{
		Mat mat = Mat::zeros(result.rows, result.cols, CV_8UC3);
		int cell_size = 10;
		mat += cv::Scalar(199, 199, 199);
		for (int y = 0; y < result.rows; y+=cell_size)
		{
			for (int x = 0; x < result.cols; x+=cell_size)
			{
				if ((x + y) % (cell_size*2) == 0)
					cv::rectangle(mat, cv::Rect(x, y, cell_size, cell_size), cv::Scalar(242, 242, 242), -1);
			}
		}
		overlayImage(mat, src, result, cv::Point(0,0), 1.0);
		ch = 3;
	}

	BITMAPINFO bitmapInfo;
	memset( &bitmapInfo, 0, sizeof(bitmapInfo) );
	bitmapInfo.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biPlanes	= 1;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biWidth	= result.cols;
	bitmapInfo.bmiHeader.biHeight	= -result.rows;
	bitmapInfo.bmiHeader.biBitCount = IPL_DEPTH_8U * ch;

	pDC->SetStretchBltMode(COLORONCOLOR);

	//target_rect가 있다면 그 안에 이미지의 가로,세로 비율을 유지한 채 그려주고 공백 부분은 배경색으로 채우라는 의미다.
	//fit이 true이면 가로 또는 세로를 꽉차게, false이면 이미지 원래 크기대로 그려준다.
	if (target_rect)
	{
		cv::Rect r;
		
		if (fit)
		{
			r = getRatioRect(CRect2cvRect(target_rect), (double)result.cols / (double)result.rows);
		}
		else
		{
			r.x = (target_rect->Width() - result.cols) / 2;
			r.y = (target_rect->Height() - result.rows) / 2;
			r.width = result.cols;
			r.height = result.rows;
		}
		//left area
		pDC->FillSolidRect(target_rect->left, target_rect->top, r.x - target_rect->left, target_rect->Height(), crBack);
		//right area
		pDC->FillSolidRect(r.x + r.width, target_rect->top, target_rect->right - r.x - r.width, target_rect->Height(), crBack);
		//top area
		pDC->FillSolidRect(r.x, target_rect->top, r.width, r.y, crBack);
		//bottom area
		pDC->FillSolidRect(r.x, r.y + r.height, r.width, target_rect->Height() - r.y - r.height, crBack);
		dx = r.x;
		dy = r.y;
		dw = r.width;
		dh = r.height;
	}
	else
	{
		//scv_draw_stretch이면 여백을 배경색으로 채우고 디스플레이하지만
		//scv_draw_original_size라면 그냥 지정된 좌표에 디스플레이하는 것이 이 함수의 역할이다.
		//main dlg의 그림 이외의 클라이언트 영역을 채우는 것은 main에서 할 일이다.
	}

	::StretchDIBits(pDC->GetSafeHdc(), dx, dy, dw, dh, 
					sx, result.rows - sy - sh, sw - padding, sh,
					result.data, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY );

	return CRect(dx, dy, dx + dw, dy + dh);
}

void scvDisplayWindow( const char* name, IplImage* pImage, int x /*= 0*/, int y /*= 0*/, int w /*= 0*/, int h /*= 0*/ )
{
	if ( !pImage )
		return;

	if ( w == 0 )
		w = pImage->width;

	if ( h == 0 )
		h = pImage->height;

	cvNamedWindow( name, CV_WINDOW_NORMAL );
	cvResizeWindow( name, w, h );
	cvMoveWindow( name, x, y );
	cvShowImage( name, pImage );
}
/*
void scvPutText( IplImage* pImage, CString sText, int x, int y, double dFontSize, COLORREF cText, int lineWidth )
{
	CvFont font;
	double hScale=dFontSize;
	//<span class="pun" style="margin: 0px; padding: 0px; border: 0px; font-size: 14px; vertical-align: baseline; background-color: transparent; color: rgb(0, 0, 0); background-position: initial initial; background-repeat: initial initial;">;
	double vScale=dFontSize;
	//<span class="pun" style="margin: 0px; padding: 0px; border: 0px; font-size: 14px; vertical-align: baseline; background-color: transparent; color: rgb(0, 0, 0); background-position: initial initial; background-repeat: initial initial;">;

	cvInitFont( &font, CV_FONT_HERSHEY_SIMPLEX, dFontSize, dFontSize, 0, lineWidth, 16 );
	USES_CONVERSION;
	cvPutText ( pImage, T2A(sText), cvPoint(x,y), &font, cvScalar( GetBValue(cText), GetGValue(cText), GetRValue(cText) ) );
	//cvPutText ( pImage, sText, cvPoint(x,y), &font, cvColorToScalar( cText, CV_8UC ) );
}
*/
double scvGetSharpness( IplImage* img_src, cv::Rect* roi /*= NULL*/ )
{
	Mat img, img_gray;

	if(roi)
		cvarrToMat(img_src)(*roi).copyTo(img);
	else
		cvarrToMat(img_src).copyTo(img);

	//imwrite( "c:\\test.bmp", img );

	if(img_src->nChannels == 4)
		cvtColor(img, img_gray, CV_BGRA2GRAY);
	else if(img_src->nChannels == 3)
		cvtColor(img, img_gray, CV_BGR2GRAY);
	else
		img_gray = img;

	Mat img_blur, img_sobel;
	blur(img_gray, img_blur, cv::Size(7,7));
	Sobel(img_blur, img_sobel, CV_8U, 1, 1, 3, 1, 0, BORDER_DEFAULT);

	Mat sobel_one = img_sobel.reshape(1);
	int sum_sobel = sum(sobel_one)[0];

	return (double)(sum_sobel) / (double)(sobel_one.cols);
}

double		getSharpness(Mat mat)
{
	if (mat.channels() == 3)
		cvtColor(mat, mat, CV_BGR2GRAY);
	else if (mat.channels() == 4)
		cvtColor(mat, mat, CV_BGRA2GRAY);

	Mat img_blur, img_sobel;
	//blur(mat, img_blur, cv::Size(7, 7));
	cv::Sobel(mat, img_sobel, CV_8U, 1, 1, 3, 1, 0, BORDER_DEFAULT);
	imshow("sobel", img_sobel);

	Mat sobel_one = img_sobel.reshape(1);
	int sum_sobel = sum(sobel_one)[0];

	return (double)(sum_sobel) / (double)(sobel_one.cols);
}

double getSharpness(uint8_t *data, int w, int h)	//data = 1 ch image raw data
{
	uint8_t	*result = (uint8_t*)malloc(sizeof(uint8_t) * w * h);
	memset(result, 0, sizeof(uint8_t) * w * h);
	sobel(data, result, w, h);
	//sobel(data, result, w, h, 1, 1);

	int sum = 0;
	int x, y;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			sum += result[y * w + x];
		}
	}

	//Mat mat(h, w, CV_8UC1, result);
	//imshow("mat", mat);

	free(result);

	return ((double)sum / (double)w);
}

/*
short GetSharpness(char* data, unsigned int width, unsigned int height)
{
// assumes that your image is already in planner yuv or 8 bit greyscale
IplImage* in = cvCreateImage(cvSize(width,height),IPL_DEPTH_8U,1);
IplImage* out = cvCreateImage(cvSize(width,height),IPL_DEPTH_16S,1);
memcpy(in->imageData,data,width*height);

// aperture size of 1 corresponds to the correct matrix
cvLaplace(in, out, 1);

short maxLap = -32767;
short* imgData = (short*)out->imageData;
for(int i =0;i<(out->imageSize/2);i++)
{
if(imgData[i] > maxLap) maxLap = imgData[i];
}

cvReleaseImage(&in);
cvReleaseImage(&out);
return maxLap;
}
*/
// OpenCV port of 'LAPM' algorithm (Nayar89)
double modifiedLaplacian(const cv::Mat& src)
{
	cv::Mat M = (Mat_<double>(3, 1) << -1, 2, -1);
	cv::Mat G = cv::getGaussianKernel(3, -1, CV_64F);

	cv::Mat Lx;
	cv::sepFilter2D(src, Lx, CV_64F, M, G);

	cv::Mat Ly;
	cv::sepFilter2D(src, Ly, CV_64F, G, M);

	cv::Mat FM = cv::abs(Lx) + cv::abs(Ly);

	double focusMeasure = cv::mean(FM).val[0];
	return focusMeasure;
}

// OpenCV port of 'LAPV' algorithm (Pech2000)
double varianceOfLaplacian(const cv::Mat& src)
{
	cv::Mat lap;
	cv::Laplacian(src, lap, CV_64F);

	cv::Scalar mu, sigma;
	cv::meanStdDev(lap, mu, sigma);

	double focusMeasure = sigma.val[0]*sigma.val[0];
	return focusMeasure;
}

// OpenCV port of 'TENG' algorithm (Krotkov86)
double tenengrad(const cv::Mat& src, int ksize)
{
	cv::Mat Gx, Gy;
	cv::Sobel(src, Gx, CV_64F, 1, 0, ksize);
	cv::Sobel(src, Gy, CV_64F, 0, 1, ksize);

	cv::Mat FM = Gx.mul(Gx) + Gy.mul(Gy);

	double focusMeasure = cv::mean(FM).val[0];
	return focusMeasure;
}

// OpenCV port of 'GLVN' algorithm (Santos97)
double normalizedGraylevelVariance(const cv::Mat& src)
{
	cv::Scalar mu, sigma;
	cv::meanStdDev(src, mu, sigma);

	double focusMeasure = (sigma.val[0]*sigma.val[0]) / mu.val[0];
	return focusMeasure;
}

double scvGetBrightness( IplImage* pSrc, cv::Rect* roi /*= NULL*/ )
{
	Mat img, img_gray;

	if(roi)
		cvarrToMat(pSrc)(*roi).copyTo(img);
	else
		cvarrToMat(pSrc).copyTo(img);

	if(pSrc->nChannels == 4)
		cvtColor(img, img_gray, CV_BGRA2GRAY);
	else if(pSrc->nChannels == 3)
		cvtColor(img, img_gray, CV_BGR2GRAY);
	else
		img_gray = img;

	return mean( img_gray )[0];
}

void scvDottedLine( Mat& src, cv::Point p1, cv::Point p2 )
{
	LineIterator it(src, p1, p2, 8);            // get a line iterator

	for(int i = 0; i < it.count; i++,it++)
	{
		if ( i%5!=0 )
		{
			(*it)[0] = 200;
		}
	}
}

COLORREF scvGetMajorColor( IplImage* pImage, cv::Rect* roi/* = NULL*/ )
{
	//IplImage*	p256;

	//cvCvtColor( pImage, p256, )
	return 0;
}
/*
IplImage* scvBlendImage( IplImage* pSrc1, IplImage* pSrc2, cv::Rect* roi, IplImage* pMask )
{
IplImage* src1		= cvCreateImage(cvGetSize(pSrc1), IPL_DEPTH_8U, 4);
IplImage* src2		= cvCreateImage(cvGetSize(pSrc1), IPL_DEPTH_8U, 4);
IplImage* result	= cvCreateImage(cvGetSize(pSrc1), IPL_DEPTH_8U, 4);


if ( pSrc1->nChannels != 4 )
cvCvtColor( pSrc1, src1, CV_BGR2BGRA );

if ( pSrc2->nChannels != 4 )
cvCvtColor( pSrc2, src2, CV_BGR2BGRA );

cvCopy( src1, result );

if ( roi != NULL )
{
cvSetImageROI( src1, *roi );
cvSetImageROI( src2, *roi );
cvSetImageROI( result, *roi );
}

cvAddWeighted( src1, 0.5, src2, 0.5, 0.0, result );

cvResetImageROI( result );

cvReleaseImage( &src1 );
cvReleaseImage( &src2 );

return result;
}
*/
IplImage*	scvBlendImage2( IplImage* pSrc1, IplImage* pSrc2, int dx, int dy, int dw, int dh, double dAlpha )
{
	int		i, j, k;
	int		ch = pSrc1->nChannels;
	int		ws1 = pSrc1->widthStep;
	int		ws2 = pSrc2->widthStep;
	byte*	pSrc1Data;
	byte*	pSrc2Data;
	double	dBeta = 1.0 - dAlpha;

	for ( i = dy; i < dy + dh; i++ )
	{
		if ( i >= pSrc1->height )
			break;

		pSrc1Data = (byte*)(pSrc1->imageData + i * ws1);
		pSrc2Data = (byte*)(pSrc2->imageData + i * ws2);

		for ( j = dx; j < dx + dw; j++ )
		{
			if ( j >= pSrc1->width )
				continue;

			for ( k = 0; k < ch; k++ ) 
			{
				pSrc1Data[ j * ch + k ] = ( (double)(pSrc1Data[j*ch + k]) * dBeta + (double)(pSrc2Data[j*ch + k]) * dAlpha );
			}
		}
	}

	return NULL;//pRes;
}

//이 함수는 실제 명암/대비와는 효과가 좀 다르다. PaintShopPro7에서 curve 기능과 유사하다.
Mat scvBrightContrast( Mat src, int nBright, double dContrast )
{
	Mat result = Mat::zeros( src.size(), src.type() );

	for( int y = 0; y < src.rows; y++ )
	{
		for( int x = 0; x < src.cols; x++ )
		{
			for( int c = 0; c < 3; c++ )
			{
				result.at<Vec3b>(y,x)[c] = saturate_cast<uchar>( dContrast*( src.at<Vec3b>(y,x)[c] ) + nBright );
			}
		}
	}

	return result;
}

//출처: http://pgmaru.tistory.com/39 [풀그림마루]
//선명한 이미지에 안개 등과 같은 흐린 레이어가 덧붙여졌다고 가정하고
//덧붙여진 레이어를 제거하는 방법이다.
//addWeighted 함수의 weight_src, weight_tmp 인자의 합은 1.0이어야 원본 밝기 평균과 동일하다.(default = 1.5, -0.5. 이미지에 맞게 조정)
void scvSharpening( Mat& src, float sigma, float weight_src /*= 1.5*/, float weight_tmp /*= -0.5*/ )
{
	cv::Mat tmp;

	cv::GaussianBlur(src, tmp, cv::Size(0,0), sigma );
	cv::addWeighted(src, weight_src, tmp, weight_tmp, 0, src);
}

void reduceColor( Mat&image, int div )
{
	int nl= image.rows; // 행 개수
	int nc= image.cols * image.channels(); // 각 행에 있는 원소의 총 개수

	for (int j=0; j<nl; j++)
	{
		uchar* data= image.ptr<uchar>(j);

		for (int i=0; i<nc; i++)
			data[i]= (data[i] / div) * div + div/2;
	}
}

IplImage* reduceTo64Colors( IplImage *img )
{
	int i,j;
	int height   = img->height;   
	int width    = img->width;    
	int step     = img->widthStep;

	IplImage*	img_result = cvCreateImage( cvSize( width, height ), img->depth, img->nChannels );

	uchar *data = (uchar *)img->imageData;
	int step2 = img_result->widthStep;
	uchar *data2 = (uchar *)img_result->imageData;

	for (i = 0; i < height ; i++)
	{
		for (j = 0; j < width; j++)
		{

			// operator XXXXXXXX & 11000000 equivalent to  XXXXXXXX AND 11000000 (=192)
			// operator 01000000 >> 2 is a 2-bit shift to the right = 00010000 
			uchar C1 = (data[i*step+j*3+0] & 192)>>2;
			uchar C2 = (data[i*step+j*3+1] & 192)>>4;
			uchar C3 = (data[i*step+j*3+2] & 192)>>6;

			data2[i*step2+j*3+0] = C1;// | C2 | C3; // merges the 2 MSB of each channel
			data2[i*step2+j*3+1] = C2;// | C2 | C3; // merges the 2 MSB of each channel
			data2[i*step2+j*3+2] = C3;//1 | C2 | C3; // merges the 2 MSB of each channel
		}     
	}
	return img_result;
}
/*
IplImage* scvReduceColor( IplImage* src, int nColor )
{
IplImage * gray = cvCreateImage( cvGetSize(src), src->depth, 1 ); 

cvCvtColor( src, gray, CV_BGR2GRAY );

int		i, j;
int		w = src->width;
int		h = src->height;
int		step = gray->widthStep;
int		v;

uchar*	data = (uchar *)gray->imageData;

for ( i = 0; i < h; i++ )
for ( j = 0; j < w; j++ )
{
v = data[i * step + j];
// 		if ( v < 127 )
// 			data[i * step + j] = 0;
// 		else
// 			data[i * step + j] = 255;
v = data[i * step + j] / (255 / nColor) * (255 / nColor);
data[i * step + j] = v;
}

return gray;
}
*/
//src의 width가 4의 배수가 아닐 경우는 완전히 정확한 결과가 나오지 않는다.
IplImage* bhQuantizeImage(IplImage* src, int colorCount)
{
	int i, size;

	CvMat *labels;
	CvMat *points;
	CvMat *color = cvCreateMat (colorCount, 1, CV_32FC3);
	CvMat *count = cvCreateMat (colorCount, 1, CV_32SC1);

	IplImage* dstImage = cvCreateImage( cvGetSize( src ), src->depth, src->nChannels );

	size = src->width * src->height;
	labels = cvCreateMat (size, 1, CV_32SC1);
	points = cvCreateMat (size, 1, CV_32FC3);

	//이미지 데이터를 1차원 픽셀 데이터 행렬로 변환
	for (i = 0; i < size; i++)
	{
		points->data.fl[i * 3 + 0] = (uchar) src->imageData[i * 3 + 0];
		points->data.fl[i * 3 + 1] = (uchar) src->imageData[i * 3 + 1];
		points->data.fl[i * 3 + 2] = (uchar) src->imageData[i * 3 + 2];
	}

	cvKMeans2 (points, colorCount, labels, cvTermCriteria (CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0) );

	cvSetZero (color);
	cvSetZero (count);

	for (i = 0; i < size; i++)
	{
		int idx = labels->data.i[i];
		int j = ++count->data.i[idx];;
		color->data.fl[idx * 3 + 0] = color->data.fl[idx * 3 + 0] * (j - 1) / j + points->data.fl[i * 3 + 0] / j;
		color->data.fl[idx * 3 + 1] = color->data.fl[idx * 3 + 1] * (j - 1) / j + points->data.fl[i * 3 + 1] / j;
		color->data.fl[idx * 3 + 2] = color->data.fl[idx * 3 + 2] * (j - 1) / j + points->data.fl[i * 3 + 2] / j;
	}

	for (i = 0; i < size; i++)
	{
		int idx = labels->data.i[i];
		dstImage->imageData[i * 3 + 0] = (char) color->data.fl[idx * 3 + 0];
		dstImage->imageData[i * 3 + 1] = (char) color->data.fl[idx * 3 + 1];
		dstImage->imageData[i * 3 + 2] = (char) color->data.fl[idx * 3 + 2];
	}


	cvReleaseMat (&labels);
	cvReleaseMat (&points);
	cvReleaseMat (&color);
	cvReleaseMat (&count);

	return dstImage;
}

Mat	bhQuantizeImage(Mat src, int colorCount)
{
	int i, size;

	CvMat *labels;
	CvMat *points;
	CvMat *color = cvCreateMat (colorCount, 1, CV_32FC3);
	CvMat *count = cvCreateMat (colorCount, 1, CV_32SC1);

	Mat dst = src.clone();

	size = src.cols * src.rows;
	labels = cvCreateMat (size, 1, CV_32SC1);
	points = cvCreateMat (size, 1, CV_32FC3);

	for (i = 0; i < size; i++) {
		points->data.fl[i * 3 + 0] = (uchar) src.data[i * 3 + 0];
		points->data.fl[i * 3 + 1] = (uchar) src.data[i * 3 + 1];
		points->data.fl[i * 3 + 2] = (uchar) src.data[i * 3 + 2];
	}

	cvKMeans2 (points, colorCount, labels, cvTermCriteria (CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0) );

	cvSetZero (color);
	cvSetZero (count);

	for (i = 0; i < size; i++)
	{
		int idx = labels->data.i[i];
		int j = ++count->data.i[idx];;
		color->data.fl[idx * 3 + 0] = color->data.fl[idx * 3 + 0] * (j - 1) / j + points->data.fl[i * 3 + 0] / j;
		color->data.fl[idx * 3 + 1] = color->data.fl[idx * 3 + 1] * (j - 1) / j + points->data.fl[i * 3 + 1] / j;
		color->data.fl[idx * 3 + 2] = color->data.fl[idx * 3 + 2] * (j - 1) / j + points->data.fl[i * 3 + 2] / j;
	}

	for (i = 0; i < size; i++)
	{
		int idx = labels->data.i[i];
		dst.data[i * 3 + 0] = (char) color->data.fl[idx * 3 + 0];
		dst.data[i * 3 + 1] = (char) color->data.fl[idx * 3 + 1];
		dst.data[i * 3 + 2] = (char) color->data.fl[idx * 3 + 2];
	}


	cvReleaseMat (&labels);
	cvReleaseMat (&points);
	cvReleaseMat (&color);
	cvReleaseMat (&count);

	return dst;
	/*
	int		i, j, index, index2;
	int		w = src.cols;
	int		h = src.rows;

	Mat dst = src.clone();

	CvMat* points = cvCreateMat(w*h,3, CV_32FC1);
	CvMat* labels = cvCreateMat(w*h,1, CV_32SC1);

	CvScalar colors[10];


	colors[0] = CV_RGB(0,0,0);			//검정
	colors[1] = CV_RGB(24,35,41);		//남색
	colors[2] = CV_RGB(0,100,100);
	colors[3] = CV_RGB(0,200,100);
	colors[4] = CV_RGB(100,200,100);
	colors[5] = CV_RGB(100,0,0);
	colors[6] = CV_RGB(200,0,0);
	colors[7] = CV_RGB(170,80,62);		//자주
	colors[8] = CV_RGB(255,255,128);	//노랑
	colors[9] = CV_RGB(255,255,255);	//흰색

	for(i = 0; i < h; i++)
	{
	for(j = 0; j < w; j++)
	{
	index = i*w*3 + j*3;
	index2 = (i*w + j)*3;
	((float*)points->data.fl)[index2] = ((uint8_t)src.data[index]);
	((float*)points->data.fl)[index2+1] = ((uint8_t)src.data[index+1]);
	((float*)points->data.fl)[index2+2] = ((uint8_t)src.data[index+2]);
	}
	}

	cvKMeans2(points, colorCount, labels, cvTermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 10,1.0));

	for(i = 0; i < h; i++)
	{
	for(j = 0; j < w; j++)
	{
	index = i*w*3 + j*3;
	int label = ((int*)labels->data.i)[i*w+j];
	dst.data[index] = colors[label].val[2];
	dst.data[index+1] = colors[label].val[1];
	dst.data[index+2] = colors[label].val[0];
	}
	}

	return dst;
	*/
}

IplImage* scvQuantizeImage(IplImage* src, int colorCount)
{
	int		i, j, index, index2;
	int		w = src->width;
	int		h = src->height;

	IplImage* dstImage = cvCreateImage( cvGetSize( src ), src->depth, src->nChannels );

	CvMat* points = cvCreateMat(w*h,3, CV_32FC1);
	CvMat* labels = cvCreateMat(w*h,1, CV_32SC1);

	CvScalar colors[10];

	colors[0] = CV_RGB(0,0,0);
	colors[1] = CV_RGB(0,0,100);
	colors[2] = CV_RGB(0,100,100);
	colors[3] = CV_RGB(0,200,100);
	colors[4] = CV_RGB(100,200,100);
	colors[5] = CV_RGB(100,0,0);
	colors[6] = CV_RGB(200,0,0);
	colors[7] = CV_RGB(0,200,0);
	colors[8] = CV_RGB(200,0,200);
	colors[9] = CV_RGB(255,100,200);



	for(i = 0; i < h; i++)
	{
		for(j = 0; j < w; j++)
		{
			index = i*w*3 + j*3;
			index2 = (i*w + j)*3;
			((float*)points->data.fl)[index2] = ((uint8_t)src->imageData[index]);
			((float*)points->data.fl)[index2+1] = ((uint8_t)src->imageData[index+1]);
			((float*)points->data.fl)[index2+2] = ((uint8_t)src->imageData[index+2]);
		}
	}

	cvKMeans2(points, colorCount, labels, cvTermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 10,1.0));

	for(i = 0; i < h; i++)
	{
		for(j = 0; j < w; j++)
		{
			index = i*w*3 + j*3;
			int label = ((int*)labels->data.i)[i*w+j];
			dstImage->imageData[index] = colors[label].val[0];
			dstImage->imageData[index+1] = colors[label].val[1];
			dstImage->imageData[index+2] = colors[label].val[2];
		}
	}

	return dstImage;
}

COLORREF scvGetMajorColor( IplImage* img, int nColorUsed, COLORREF* crEachColor /*= NULL*/, int* nCountEachColor /*= NULL*/ )
{
	ASSERT( nColorUsed <= 256 );
	ASSERT( img->nChannels == 3 );

	int			i, j, size = img->width * img->height;
	int			nBrAvg = -1;
	COLORREF	crPixel;
	COLORREF	crMajor;
	int			nTotalColor = 0;
	COLORREF	crColors[256];
	int			ncrCount[256];
	bool		bFound;

	for ( i = 0; i < nColorUsed; i++ )
	{
		ncrCount[i] = 0;
	}

	for ( i = 0; i < size; i++ )
	{
		crPixel = RGB(  (uchar) img->imageData[i * 3 + 2],
			(uchar) img->imageData[i * 3 + 1],
			(uchar) img->imageData[i * 3 + 0] );

		bFound = false;

		for ( j = 0; j < nTotalColor; j++ )
		{
			if ( crPixel == crColors[j] )
			{
				ncrCount[j]++;
				bFound = true;
				break;
			}
		}

		if ( !bFound )
		{
			crColors[nTotalColor] = crPixel;
			ncrCount[nTotalColor]++;

			nTotalColor++;
		}
	}

	int nMax = 0;

	for ( i = 0; i < nColorUsed; i++ )
	{
		TRACE( "%d, %d, (%3d,%3d,%3d)\n", i, ncrCount[i], GetRValue( crColors[i] ), GetGValue( crColors[i] ), GetBValue( crColors[i] ) );

		if ( crEachColor != NULL )
			crEachColor[i] = crColors[i];

		if ( nCountEachColor != NULL )
			nCountEachColor[i] = ncrCount[i];

		if ( ncrCount[i] > nMax )
		{
			nMax = ncrCount[i];
			crMajor = crColors[i];
		}
	}

	return crMajor;
}

COLORREF scvGetMajorColor( Mat src, int nColorUsed, COLORREF* crEachColor /*= NULL*/, int* nCountEachColor /*= NULL*/ )
{
	ASSERT( nColorUsed <= 256 );
	ASSERT( src.channels() == 3 );

	int			i, j, size = src.cols * src.rows;
	int			nBrAvg = -1;
	COLORREF	crPixel;
	COLORREF	crMajor;
	int			nTotalColor = 0;
	COLORREF	crColors[256];
	int			ncrCount[256];
	bool		bFound;

	for ( i = 0; i < nColorUsed; i++ )
		ncrCount[i] = 0;

	for ( i = 0; i < size; i++ )
	{
		crPixel = RGB(  (uchar)src.data[i * 3 + 2],
			(uchar)src.data[i * 3 + 1],
			(uchar)src.data[i * 3 + 0] );

		bFound = false;

		for ( j = 0; j < nTotalColor; j++ )
		{
			if ( crPixel == crColors[j] )
			{
				ncrCount[j]++;
				bFound = true;
				break;
			}
		}

		if ( !bFound )
		{
			crColors[nTotalColor] = crPixel;
			ncrCount[nTotalColor]++;

			nTotalColor++;
		}
	}

	int nMax = 0;

	for ( i = 0; i < nColorUsed; i++ )
	{
		TRACE( "%d, %d, (%3d,%3d,%3d)\n", i, ncrCount[i], GetRValue( crColors[i] ), GetGValue( crColors[i] ), GetBValue( crColors[i] ) );

		if ( crEachColor != NULL )
			crEachColor[i] = crColors[i];

		if ( nCountEachColor != NULL )
			nCountEachColor[i] = ncrCount[i];

		if ( ncrCount[i] > nMax )
		{
			nMax = ncrCount[i];
			crMajor = crColors[i];
		}
	}

	return crMajor;
}
/*
IplImage* scvMake4BytesWidth( IplImage* src )
{
if ( src->width % 4 == 0 )
return src;


}
*/

void scvDisplayHistogram( char* name, IplImage* pImage, int x /*= 0*/, int y /*= 0*/, int w /*= 0*/, int h /*= 0*/ )
{
	IplImage* canvas;
	int       hist[256];
	double    scale = 1;
	int       i, j, max = 0;

	//////////////////////////////////////////////////////////////////////////
	// histogram을 표현할 이미지 canvas 생성
	if ( w == 0 )
		w = 256;

	if ( h == 0 )
		h = 256;

	canvas = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);
	cvSet(canvas, CV_RGB(255, 255, 255), 0);

	//////////////////////////////////////////////////////////////////////////
	// hist 초기화 및 각 밝기의 빈도수 계산
	for (i = 0; i < 256; i++)
	{
		hist[i] = 0;
	}

	for (i = 0; i < pImage->height; i++)
	{
		for (j = 0; j < pImage->width; j++)
		{
			hist[(uchar)pImage->imageData[(i * pImage->widthStep) + j]]++;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// max : 가장 많이 나온 밝기의 개수
	// 동일한 scale로 여러 이미지의 histogram을 보고싶을 경우
	// max 값을 임의로 고정시키면 된다
	// 여기서는 최대로 나온 개수를 256*60개라고 임의로 지정
	//max = 256 * 50;		

	//////////////////////////////////////////////////////////////////////////
	// 화면을 꽉 채워서 보고싶으면 아래의 for문을 이용해 hist 중 최대 값을 찾는다

	for (i = 0; i < 256; i++)
	{
		max = hist[i] > max ? hist[i] : max;
	}


	//////////////////////////////////////////////////////////////////////////
	// canvas 창에 맞도록 scale 값 설정
	scale = max > canvas->height ? (double)canvas->height/max : 1.;

	//////////////////////////////////////////////////////////////////////////
	// canvas에 세로 줄을 그림
	// 줄을 canvas의 밑에서부터 그려야하기 때문에 맨 밑의 포인트(pt2)부터 그림

	for (i = 0; i < 256; i++)
	{
		CvPoint pt1 = cvPoint(i, canvas->height - (hist[i] * scale));
		CvPoint pt2 = cvPoint(i, canvas->height);
		cvLine(canvas, pt1, pt2, CV_RGB(0,0,0), 1, 8, 0);
	}

	//////////////////////////////////////////////////////////////////////////
	// name이라는 창에 canvas 이미지를 출력
	cvNamedWindow( name, CV_WINDOW_NORMAL );
	cvResizeWindow( name, w, h );
	cvMoveWindow( name, x, y );
	cvShowImage( name, canvas );

	cvReleaseImage(&canvas);
}

/*
//출처:http://jepsonsblog.blogspot.kr/2012/10/overlay-transparent-image-in-opencv.html
//This code only works if:
//•The background is in BGR colour space.
//•The foreground is in BGRA colour space.

//usage:
// add the second parameter "CV_LOAD_IMAGE_UNCHANGED" as flag, to make sure the transparancy channel is read!
cv::Mat foreground = imread("D:/images/foreground.png", CV_LOAD_IMAGE_UNCHANGED );	//CV_LOAD_IMAGE_UNCHANGED = -1
cv::Mat background = imread("D:/images/background.jpg");
cv::Mat result;

overlayImage(background, foreground, result, cv::Point(0,0));
cv::imshow("result", result);

foreground 이미지가 3채널이면 dAlpha값을 투명도로 계산해서 합성시키고
4채널이면 투명 채널정보와 dAlpha를 모두 이용해서 합성한다.
따라서 4채널 이미지의 투명 정보 그대로 오버레이 할 때는 dAlpha값을 1.0으로 설정한다.
*/

void overlayImage(const cv::Mat &background, const cv::Mat &foreground, cv::Mat &output, cv::Point2i location, double dAlpha)
{
	background.copyTo(output);

	// start at the row indicated by location, or at row 0 if location.y is negative.
	for ( int y = std::max(location.y , 0); y < background.rows; ++y )
	{
		int fY = y - location.y; // because of the translation

		// we are done of we have processed all rows of the foreground image.
		if(fY >= foreground.rows)
			break;

		// start at the column indicated by location, 


		// or at column 0 if location.x is negative.
		for(int x = std::max(location.x, 0); x < background.cols; ++x)
		{
			int fX = x - location.x; // because of the translation.

			// we are done with this row if the column is outside of the foreground image.
			if(fX >= foreground.cols)
				break;

			// determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
			double opacity = dAlpha;

			if ( foreground.channels() == 4 )
				opacity = ((double)foreground.data[fY * foreground.step + fX * foreground.channels() + 3]) / 255. * dAlpha;

			//TRACE( "%f\n", opacity );

			// and now combine the background and foreground pixel, using the opacity, 


			// but only if opacity > 0.
			for(int c = 0; opacity > 0 && c < output.channels(); ++c)
			{
				uint8_t foregroundPx =
					foreground.data[fY * foreground.step + fX * foreground.channels() + c];

				uint8_t backgroundPx =
					background.data[y * background.step + x * background.channels() + c];

				output.data[y*output.step + output.channels()*x + c] =
					backgroundPx * (1.-opacity) + foregroundPx * opacity;

			}
		}
	}
}

void overlayImage( cv::Mat &background, cv::Mat &foreground, cv::Mat &mask, cv::Mat &output, cv::Point2i location, double dAlpha )
{
	background.copyTo(output);

	//만약 background, foreground, mask의 크기가 같고 location이 0,0이라면
	//아래와 같이 copyTo로도 가능하다.
	if ( (background.size() == foreground.size()) && (background.size() == mask.size()) && (location == cv::Point2i(0,0)) )
	{
		foreground.copyTo( output, mask );
		return;
	}

	if ( mask.channels() == 3 )
		cvtColor( mask, mask, CV_BGR2GRAY );

	// start at the row indicated by location, or at row 0 if location.y is negative.
	for ( int y = std::max(location.y , 0); y < background.rows; ++y )
	{
		int fY = y - location.y; // because of the translation

		// we are done of we have processed all rows of the foreground image.
		if(fY >= foreground.rows || fY >= mask.rows )
			break;

		// start at the column indicated by location, 


		// or at column 0 if location.x is negative.
		for(int x = std::max(location.x, 0); x < background.cols; ++x)
		{
			int fX = x - location.x; // because of the translation.

			// we are done with this row if the column is outside of the foreground image.
			if(fX >= foreground.cols || fX >= mask.cols )
				break;

			// determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
			double opacity =
				((double)mask.data[fY * mask.step + fX * mask.channels()]) / 255. * dAlpha;

			//TRACE( "%f\n", opacity );

			// and now combine the background and foreground pixel, using the opacity, 


			// but only if opacity > 0.
			for(int c = 0; opacity > 0 && c < output.channels(); ++c)
			{
				uint8_t foregroundPx =
					foreground.data[fY * foreground.step + fX * foreground.channels() + c];

				uint8_t backgroundPx =
					background.data[y * background.step + x * background.channels() + c];

				output.data[y*output.step + output.channels()*x + c] =
					backgroundPx * (1.-opacity) + foregroundPx * opacity;

			}
		}
	}
}

//특정 색상을 제외하고 배경 파일에 오버레이 합성한다.
void overlayImage( cv::Mat &background, cv::Mat foreground, cv::Scalar crExcept, cv::Point2i location, double dAlpha, double dSimility )
{
	Mat output;

	background.copyTo( output );

	// start at the row indicated by location, or at row 0 if location.y is negative.
	for ( int y = std::max(location.y , 0); y < background.rows; ++y )
	{
		int fY = y - location.y; // because of the translation

		// we are done of we have processed all rows of the foreground image.
		if(fY >= foreground.rows)
			break;

		// start at the column indicated by location, 


		// or at column 0 if location.x is negative.
		for(int x = std::max(location.x, 0); x < background.cols; ++x)
		{
			int fX = x - location.x; // because of the translation.

			// we are done with this row if the column is outside of the foreground image.
			if(fX >= foreground.cols)
				break;

			// determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
			double opacity = dAlpha;

			if ( foreground.channels() == 4 )
				opacity = ((double)foreground.data[fY * foreground.step + fX * foreground.channels() + 3]) / 255. * dAlpha;

			//TRACE( "%f\n", opacity );

			// and now combine the background and foreground pixel, using the opacity, 


			// but only if opacity > 0.
			//for(int c = 0; opacity > 0 && c < output.channels(); ++c)
			{
				uint8_t b = foreground.data[fY * foreground.step + fX * foreground.channels() + 0];
				uint8_t g = foreground.data[fY * foreground.step + fX * foreground.channels() + 1];
				uint8_t r = foreground.data[fY * foreground.step + fX * foreground.channels() + 2];

				if ( cv::Scalar( b, g, r ) == crExcept ||
					GetColorSimilityDistance( cv::Scalar( b, g, r ), crExcept ) < dSimility )
					continue;

				output.data[y*output.step + output.channels()*x + 0] = background.data[y * background.step + x * background.channels() + 0] * (1.-opacity) + b * opacity;
				output.data[y*output.step + output.channels()*x + 1] = background.data[y * background.step + x * background.channels() + 1] * (1.-opacity) + g * opacity;
				output.data[y*output.step + output.channels()*x + 2] = background.data[y * background.step + x * background.channels() + 2] * (1.-opacity) + r * opacity;
				/*
				uint8_t foregroundPx =
				foreground.data[fY * foreground.step + fX * foreground.channels() + c];


				output.data[y*output.step + output.channels()*x + c] =
				backgroundPx * (1.-opacity) + foregroundPx * opacity;
				*/
			}
		}
	}

	output.copyTo( background );
}

//특정 색상을 제외하고 마스크 영역의 검은 영역만 제외하고 배경 파일에 오버레이 합성한다.
void overlayImage( cv::Mat &back, cv::Mat &fore, cv::Mat &mask, cv::Scalar crExcept, cv::Point2i location, double dSimility )
{
	cv::Vec3b	value;

	for (int row = 0; row < fore.size().height; row++)
	{
		for (int col = 0; col < fore.size().width; col++)
		{
			//  			if ( mask.at<uchar>(row, col) == 0 )
			//  				continue;

			value = fore.at<cv::Vec3b>(row, col);

			if ( GetColorSimilityDistance( value, crExcept ) < dSimility )
				continue;

			back.at<cv::Vec3b>(row + location.y, col + location.x) = value;
		}
	}
}

//동일 크기이미지의 fore를 bitwise를 사용해서 간단히 합성한다.
cv::Mat overlay_bitwise(cv::Mat background, cv::Mat fore)
{
	Mat gray, gray_inv, src1final, src2final;

	cvtColor(fore, gray, CV_BGR2GRAY);
	threshold(gray, gray, 0, 255, CV_THRESH_BINARY);
	//adaptiveThreshold(gray,gray,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY,5,4); 
	bitwise_not(gray, gray_inv);
	background.copyTo(src1final, gray_inv);
	fore.copyTo(src2final, gray);
	return (src1final + src2final);
}

double GetColorSimilityDistance( CvScalar c1, CvScalar c2 )
{
	return sqrt(
		(c1.val[0]-c2.val[0])*(c1.val[0]-c2.val[0]) +
		(c1.val[1]-c2.val[1])*(c1.val[1]-c2.val[1]) +
		(c1.val[2]-c2.val[2])*(c1.val[2]-c2.val[2])
		);
}

double GetColorSimilityDistance( int r1, int g1, int b1, int r2, int g2, int b2 )
{
	return GetColorSimilityDistance( Scalar( r1, g1, b1 ), Scalar( r2, g2, b2 ) );
}


//마스크 이미지의 흰색 농도에 따라 src 이미지가 표시되고 검정인 부분은 white로 채워진다.(마스크의 흰색:원본, 회색:반투명, 검정:흰색으로 대체)
//2016.09.23 => 마스크의 흰색=>원본, 회색=>반투명, 검정=>흰색 또는 검정
void overlayMaskImage( Mat &src, Mat mask, bool bWhite )
{
	bool bConvertedColor = false;

	if ( src.channels() == 1 )
	{
		cvtColor( src, src, CV_GRAY2BGR );
		bConvertedColor = true;
	}

	if ( mask.channels() != 1 )
		cvtColor( mask, mask, CV_RGB2GRAY );

	resize( mask, mask, src.size() );

	int			n, m, diff;
	cv::Vec3b	value;

	for (int row = 0; row < src.size().height; row++)
	{
		for (int col = 0; col < src.size().width; col++)
		{
			value = src.at<cv::Vec3b>(row, col);
			m = mask.at<uchar>(row, col);
			diff = 255 - m;

			n = value.val[0] + (bWhite ? diff : -diff);
			COLOR_RANGE( n );
			value.val[0] = n;

			n = value.val[1] + (bWhite ? diff : -diff);
			COLOR_RANGE( n );
			value.val[1] = n;

			n = value.val[2] + (bWhite ? diff : -diff);
			COLOR_RANGE( n );
			value.val[2] = n;

			src.at<cv::Vec3b>(row, col) =  value;
		}
	}

	if ( bConvertedColor )
		cvtColor( src, src, CV_BGR2GRAY );
}

//src 영상에 mask intensity 정보를 alpha값으로 하여 특정색으로 overlay 시킨다.
void overlayMaskImage(Mat src, Mat mask, Mat &output, cv::Scalar cr, cv::Point2i location, double dAlpha)
{
	if (dAlpha > 1.0)
		dAlpha = 1.0;

	//만약 background, foreground, mask의 크기가 같고 location이 0,0이라면
	//아래와 같이 copyTo로도 가능하다.
	if ((src.size() == mask.size()) && (location == cv::Point2i(0, 0)) && dAlpha == 1.0f)
	{
		src.copyTo(output, mask);
		return;
	}

	src.copyTo(output);

	if (mask.channels() == 3)
		cvtColor(mask, mask, CV_BGR2GRAY);

	// start at the row indicated by location, or at row 0 if location.y is negative.
	for (int y = std::max(location.y, 0); y < src.rows; ++y)
	{
		int fY = y - location.y; // because of the translation

		//we are done of we have processed all rows of the foreground image.
		if (fY >= mask.rows)
			break;

		// start at the column indicated by location, 


		// or at column 0 if location.x is negative.
		for (int x = std::max(location.x, 0); x < src.cols; ++x)
		{
			int fX = x - location.x; // because of the translation.

			// we are done with this row if the column is outside of the foreground image.
			if (fX >= mask.cols)
				break;

			// determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
			double opacity =
				((double)mask.data[fY * mask.step + fX * mask.channels()]) / 255. * dAlpha;

			//TRACE( "%f\n", opacity );

			// and now combine the background and foreground pixel, using the opacity, 


			// but only if opacity > 0.
			for (int c = 0; opacity > 0 && c < output.channels(); ++c)
			{
				uint8_t foregroundPx = mask.data[fY * mask.step + fX];
				uint8_t backgroundPx = src.data[y * src.step + x * src.channels() + c];

				output.data[y*output.step + output.channels()*x + c] =
					backgroundPx * (1. - opacity) + cr.val[c] * opacity;
			}
		}
	}

}

//마스크 연산으로만 오버레이 처리되므로 src, mask의 크기는 동일해야 한다.
void overlayMaskImage(Mat src, Mat mask, Mat &output, cv::Scalar cr, double dAlpha)
{
	Mat colormat(src.rows, src.cols, src.type(), cr);
	Mat colormask;// = Mat::zeros(src.size(), src.type());
	colormat.copyTo(colormask, mask);
	addWeighted(src, 1.0, colormask, dAlpha, 0, output);
}

//4채널 png 파일의 알파 채널값을 유지한 채 gray 이미지로 변환시킨다.
void grayWithoutAlpha( cv::Mat &img )
{
	ASSERT( img.channels() == 4 );

	std::vector<Mat> channels;
	Mat alpha;

	split( img, channels );
	channels[3].copyTo( alpha );

	cvtColor( img, img, CV_BGRA2GRAY );
	cvtColor( img, img, CV_GRAY2BGRA );
	split( img, channels );

	alpha.copyTo( channels[3] );
	merge( channels, img );
}


//특정 색상의 픽셀을 제외한 다른 픽셀의 컬러를 조정한다.
void adjustColor( cv::Mat &img, cv::Scalar crValue, cv::Scalar crExcept, cv::Scalar crTolerance )
{
	int			col, row, n;
	cv::Vec3b	value;

	for ( row = 0; row < img.rows; row++)
	{
		for ( col = 0; col < img.cols; col++)
		{
			value = img.at<cv::Vec3b>(row, col);

			// 			if ( value.val[0] == 254 && value.val[1] == 254 && value.val[2] == 254 )
			// 				value.val[0] = 254;

			if ( abs( value.val[0] - crExcept[0] ) <= crTolerance[0] &&
				abs( value.val[1] - crExcept[1] ) <= crTolerance[1] &&
				abs( value.val[2] - crExcept[2] ) <= crTolerance[2] )
				continue;

			n = value.val[0];

			if ( n < 100 )
				n = n;

			n += crValue[0];
			COLOR_RANGE( n );
			value.val[0] = n;

			n = value.val[1];
			n += crValue[1];
			COLOR_RANGE( n );
			value.val[1] = n;

			n = value.val[2];
			n += crValue[2];
			COLOR_RANGE( n );
			value.val[2] = n;

			img.at<cv::Vec3b>(row, col) =  value;
		}
	}
}

cv::Scalar	fromRGB( COLORREF rgb )
{
	return cv::Scalar( GetBValue( rgb ), GetGValue( rgb ), GetRValue( rgb ) );
}

#include <fstream>
Mat	loadMat(LPCTSTR sfile, int flag /*= IMREAD_UNCHANGED*/, bool bDisplayError /*= false*/ )
{
	/*
	CString str;

	if ( PathFileExists( sfile ) == false && bDisplayError )
	{
		str.Format(_T("file not found : \n\n%s"), sfile );
		AfxMessageBox( str );
	}

	CT2CA psz(sfile);
	std::string ststr(psz);

	return imread(CString2string(sfile), flags);
	*/
	// Open the file with Unicode name
	ifstream f(sfile, iostream::binary);

	// Get its size
	filebuf* pbuf = f.rdbuf();
	size_t size = pbuf->pubseekoff(0, f.end, f.in);
	pbuf->pubseekpos(0, f.in);

	// Put it in a vector
	vector<uchar> buffer(size);
	pbuf->sgetn((char*)buffer.data(), size);

	// Decode the vector
	return imdecode(buffer, flag);
}

//raw, yuv도 구분하여 읽는다.
Mat	loadImage( LPCTSTR sfile, int flags /*= CV_LOAD_IMAGE_UNCHANGED*/, bool bDisplayError /*= true*/, int width, int height, int yuv_format )
{
	CString str;

	if ( PathFileExists( sfile ) == false && bDisplayError )
	{
		str.Format(_T("file not found : \n\n%s"), sfile );
		AfxMessageBox( str );
	}

	CString sExt = GetFileExtensionFromFilename(sfile).MakeLower();

	if ( sExt != _T("raw") && sExt != _T("yuv") )
		return loadMat( sfile, flags, bDisplayError );

	cv::Mat mat;
	uint64_t file_size = GetFileSize( sfile );

	//1ch raw image
	if ( file_size == width * height )
	{
		mat.create( height, width, CV_8UC1 );
		read_raw( sfile, mat.data, file_size );
	}
	//3ch raw image
	else if ( file_size == width * height * 3 )
	{
		mat.create( height, width, CV_8UC3 );
		read_raw( sfile, mat.data, file_size );
	}
	//yuv422 image
	else if ( file_size == width * height * 2 )
	{
		mat.create( height, width, CV_8UC3 );

		uint8_t *src = new uint8_t[file_size];
		read_raw( sfile, src, file_size );
		if ( yuv_format == yuv422_uyvy )
			yuv422_uyvy_to_bgr( src, mat.data, width, height );
		else
			;//yuv422_yuyv_to_bgr( src, mat.data, width, height );
		delete [] src;
	}
	//yuv420 image
	else if ( file_size >= width * height * 1.5 )
	{
		mat.create( height, width, CV_8UC3 );

		uint8_t *src = new uint8_t[file_size];
		read_raw( sfile, src, file_size );

		if ( yuv_format == yuv420_nv12 )
			yuv420_nv12_to_bgr( src, mat.data, width, height );
		else
			yuv420_yv12_to_bgr( src, mat.data, width, height );
		delete [] src;
	}
	else
	{
	}

	return mat;
}


Mat	translateImg(Mat &img, int offsetx, int offsety)
{
	Mat trans_mat = (Mat_<double>(2,3) << 1, 0, offsetx, 0, 1, offsety);
	warpAffine(img,img,trans_mat,img.size());
	return img;
}

//1=CW, 2=CCW, 3=180
void rotate90(cv::Mat &matImage, int rotflag )
{
	
	if (rotflag == 1)
	{
		transpose(matImage, matImage);  
		flip(matImage, matImage,1); //transpose+flip(1)=CW
	}
	else if (rotflag == 2)
	{
		transpose(matImage, matImage);  
		flip(matImage, matImage,0); //transpose+flip(0)=CCW     
	}
	else if (rotflag ==3)
	{
		flip(matImage, matImage,-1);    //flip(-1)=180          
	}
	else if (rotflag != 0)
	{ //if not 0,1,2,3:
		TRACE( "Unknown rotation flag(%d)", rotflag );
	}
}

//src를 angle만큼 회전시켜 rotated에 저장하고 mask 이미지를 리턴해준다.
Mat	rotateMat( Mat src, Mat* rotated, double angle )
{
	Mat mask;

	if ( (int)angle % 90 == 0 )
	{
		src.copyTo( *rotated );
		if ( (int)angle == 90 )
			rotate90( *rotated, 2 );
		else if ( (int)angle == 180 )
			rotate90( *rotated, 3 );
		else if ( (int)angle == 270 )
			rotate90( *rotated, 1 );
		mask = ~Mat::zeros( rotated->size(), CV_8UC1 );
		return mask;
	}

	// get rotation matrix for rotating the image around its center
    cv::Point2f center(src.cols/2.0, src.rows/2.0);
    cv::Mat rot = cv::getRotationMatrix2D(center, angle, 1.0);

    // determine bounding rectangle
    cv::Rect bbox = cv::RotatedRect(center,src.size(), angle).boundingRect();

    // adjust transformation matrix
    rot.at<double>(0,2) += bbox.width/2.0 - center.x;
    rot.at<double>(1,2) += bbox.height/2.0 - center.y;

	cv::warpAffine(src, *rotated, rot, bbox.size(), INTER_CUBIC );//, BORDER_TRANSPARENT, cv::Scalar(255,0,255));

	//GaussianBlur( *rotated, *rotated, cv::Size(3,3), 1.0 );
	//addWeighted(*rotated, 1.5, *rotated, -0.5, 0, *rotated);

	if ( true )
	{
		// find external contour and create mask
		std::vector<std::vector<cv::Point> > contours;
		cv::Mat imageWarpedCloned = rotated->clone(); // clone the image because findContours will modify it
		cv::cvtColor(imageWarpedCloned, imageWarpedCloned, CV_BGR2GRAY); //only if the image is BGR
		cv::findContours (imageWarpedCloned, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

		// create mask
		mask = cv::Mat::zeros(rotated->size(), CV_8U); 
		cv::drawContours(mask, contours, 0, cv::Scalar(255), -1);
	}
	//binary를 이용한 마스크 생성 방식. 코드는 간단하나 검정이 있으면 구멍이 뚫린다.
	else
	{
		Mat gray; 
		cvtColor( *rotated, gray, CV_BGR2GRAY );
		threshold( gray, mask, 0, 255, CV_THRESH_BINARY );
	}

	//GaussianBlur( mask, mask, cv::Size(3,3), 1.0 );
	//addWeighted( mask, 1.5, mask, -0.5, 0, mask );
	cv::erode( mask, mask, cv::Mat()); // for avoid artefacts

	return mask;
}

//fore를 angle만큼 회전시킨 이미지를 back의 cx, cy에 center 합성한다.
//back이 NULL인 경우는 fore는 회전된 이미지가 들어가고 8bit mask를 리턴하므로 그 결과값을 별도로 활용할 수 있다.
Mat	rotateOverlay( Mat* back, Mat* fore, double angle, int cx, int cy, double alpha )
{
	Mat mask;
	Mat rotated;
	
	mask = rotateMat( *fore, &rotated, angle );

	if ( back != NULL )
	{
		if ( alpha == 1.0 )
			rotated.copyTo( (*back)(cv::Rect(cx - rotated.cols/2, cy - rotated.rows/2, rotated.cols, rotated.rows)), mask );
		else
			overlayImage( *back, rotated, mask, *back, cv::Point2i(cx - rotated.cols/2, cy - rotated.rows/2), alpha );
	}
	rotated.copyTo( *fore );
	return mask;
}
/*
//90도씩 회전
void rotate(uint8_t *src, uint32_t src_width, uint32_t src_height, uint8_t *dst, int rotflag)
{
	int x, y;

	for (y = 0; y < src_height; y++)
	{
		for (x = 0; x < src_width; x++)
			;//*(dst + x
	}
}
*/

//opencv의 t(). -90도 회전 후 상하 flip.
void transpose(uint8_t *src, uint32_t src_width, uint32_t src_height, uint8_t *dst)
{
	int x, y;

	for (y = 0; y < src_height; y++)
	{
		for (x = 0; x < src_width; x++)
		{
			*(dst + x * src_height + y) = *(src + y * src_width + x);
		}
	}
}

//src이미지의 ptSrc좌표를 ptDst로 warping 시킨다. mask 이미지를 리턴한다.
//pt는 lt부터 시계방향 차례로 준다.
Mat	warpMat( Mat *src, int ptSrc[8], int ptDst[8] )
{
	bool bptSrcIsNull = false;
	bool bptDstIsNull = false;

	if ( ptSrc == NULL )
	{
		bptSrcIsNull = true;
		ptSrc = new int[8];
		ptSrc[0] = 0;
		ptSrc[1] = 0;
		ptSrc[2] = src->cols;
		ptSrc[3] = 0;
		ptSrc[4] = src->cols;
		ptSrc[5] = src->rows;
		ptSrc[6] = 0;
		ptSrc[7] = src->rows;
	}

	if ( ptDst == NULL )
	{
		bptDstIsNull = true;
		ptDst = new int[8];
		ptDst[0] = 0;
		ptDst[1] = 0;
		ptDst[2] = src->cols;
		ptDst[3] = 0;
		ptDst[4] = src->cols;
		ptDst[5] = src->rows;
		ptDst[6] = 0;
		ptDst[7] = src->rows;
	}

	// perform warp perspective
	std::vector<cv::Point2f> prev;
	prev.push_back(cv::Point2f(ptSrc[0], ptSrc[1]));
	prev.push_back(cv::Point2f(ptSrc[2], ptSrc[3]));
	prev.push_back(cv::Point2f(ptSrc[4], ptSrc[5]));
	prev.push_back(cv::Point2f(ptSrc[6], ptSrc[7]));

	std::vector<cv::Point2f> post;
	post.push_back(cv::Point2f(ptDst[0], ptDst[1]));
	post.push_back(cv::Point2f(ptDst[2], ptDst[3]));
	post.push_back(cv::Point2f(ptDst[4], ptDst[5]));
	post.push_back(cv::Point2f(ptDst[6], ptDst[7]));

	Mat H = findHomography( prev, post );//, RHO ); 
	Mat warped, mask;
	warpPerspective( *src, warped, H, cv::Size(src->cols+200, src->rows) ); 
	if ( bptSrcIsNull )
		delete [] ptSrc;
	if ( bptDstIsNull )
		delete [] ptDst;

	std::vector<std::vector<cv::Point> > contours;
	cv::Mat imageWarpedCloned = warped.clone(); // clone the image because findContours will modify it
	cv::cvtColor(imageWarpedCloned, imageWarpedCloned, CV_BGR2GRAY); //only if the image is BGR
	cv::findContours (imageWarpedCloned, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

	// create mask
	mask = cv::Mat::zeros(warped.size(), CV_8U); 
	cv::drawContours(mask, contours, 0, cv::Scalar(255), -1);

	warped.copyTo( *src );
	return mask;
}

Mat	warpOverlay( Mat *back, Mat *fore, int ptDst[8] )
{
	// perform warp perspective
	std::vector<cv::Point2f> prev;
	prev.push_back(cv::Point2f(0, 0) );
	prev.push_back(cv::Point2f(fore->cols, 0) );
	prev.push_back(cv::Point2f(fore->cols, fore->rows) );
	prev.push_back(cv::Point2f(0, fore->rows) );

	std::vector<cv::Point2f> post;
	post.push_back(cv::Point2f(ptDst[0], ptDst[1]));
	post.push_back(cv::Point2f(ptDst[2], ptDst[3]));
	post.push_back(cv::Point2f(ptDst[4], ptDst[5]));
	post.push_back(cv::Point2f(ptDst[6], ptDst[7]));

	Mat H = findHomography( prev, post, 0 ); 
	Mat warped, mask;

	// Warp the logo image to change its perspective
	warpPerspective( *fore, warped, H, back->size() ); 

	//contour를 이용한 마스크 생성 방식
	// find external contour and create mask
	if ( true )
	{
		std::vector<std::vector<cv::Point> > contours;
		cv::Mat imageWarpedCloned = warped.clone(); // clone the image because findContours will modify it
		cv::cvtColor(imageWarpedCloned, imageWarpedCloned, CV_BGR2GRAY); //only if the image is BGR
		cv::findContours (imageWarpedCloned, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

		// create mask
		mask = cv::Mat::zeros(warped.size(), CV_8U); 
		cv::drawContours(mask, contours, 0, cv::Scalar(255), -1);
	}
	//binary를 이용한 마스크 생성 방식
	else
	{
		Mat gray; 
		cvtColor( warped, gray, CV_BGR2GRAY );
		threshold( gray, mask, 0, 255, CV_THRESH_BINARY );
	}

	// copy warped image into image2 using the mask
	cv::erode(mask, mask, cv::Mat()); // for avoid artefacts

	warped.copyTo( *back, mask );
	warped.copyTo( *fore );
	return mask;
}

double getDistance( cv::Point a, cv::Point b )
{
	return sqrt(pow((double) (a.x - b.x), 2) + pow((double) (a.y - b.y), 2));
}

// Helper function that computes the longest distance from the edge to the center point.
double getMaxDisFromCorners(const cv::Size& imgSize, const cv::Point& center)
{
	// given a rect and a line
	// get which corner of rect is farthest from the line

	std::vector<cv::Point> corners(4);
	corners[0] = cv::Point(0, 0);
	corners[1] = cv::Point(imgSize.width, 0);
	corners[2] = cv::Point(0, imgSize.height);
	corners[3] = cv::Point(imgSize.width, imgSize.height);

	double maxDis = 0;
	for (int i = 0; i < 4; ++i)
	{
		double dis = getDistance(corners[i], center);
		if (maxDis < dis)
			maxDis = dis;
	}

	return maxDis;
}

// Helper function that creates a gradient image.   
// firstPt, radius and power, are variables that control the artistic effect of the filter.
void generateGradient(cv::Mat& mask)
{
	cv::Point firstPt = cv::Point(mask.size().width/2, mask.size().height/2);
	double radius = 0.8;
	double power = 0.9;

	double maxImageRad = radius * getMaxDisFromCorners(mask.size(), firstPt);

	double min = 100.0;
	double max = 0.0;

	mask.setTo(cv::Scalar(1));
	for (int i = 0; i < mask.rows; i++)
	{
		for (int j = 0; j < mask.cols; j++)
		{
			double temp = getDistance(firstPt, cv::Point(j, i)) / maxImageRad;
			temp = temp * power;
			double temp_s = pow(cos(temp), 4);
			mask.at<double>(i, j) = temp_s;


			if ( temp_s < min )
				min = temp_s;
			if ( temp_s > max )
				max = temp_s;
		}
	}

	TRACE( "%f\n%f\n", min, max );
}

//http://www.askaswiss.com/2016/01/how-to-create-pencil-sketch-opencv-python.html
Mat	sketch( Mat src, int blurSize )
{
	Mat gray, neg, blend = Mat::zeros( src.size(), CV_8UC1 );
	Mat dst;

	cvtColor( src, gray, CV_BGR2GRAY );
	//src.copyTo( gray );
	neg = ~gray;
	GaussianBlur( neg, neg, cv::Size(blurSize,blurSize), 0.0 );

	int w = src.cols;
	int h = src.rows;
	
	divide(gray, 255-neg, blend, 256);
	return blend;
}

Mat	milky_white( Mat src, int blurSize, int level )
{
	Mat gray, neg, blend = Mat::zeros( src.size(), CV_8UC1 );
	Mat dst;

	src.copyTo( gray );
	neg = ~gray;
	GaussianBlur( neg, neg, cv::Size(blurSize,blurSize), 0.0 );

	int w = src.cols;
	int h = src.rows;
	
	if ( gray.channels() == 1 )
	{
		divide(255-gray, 255-neg, blend, 32);
		blend = 255 - blend;
	}
	else if ( gray.channels() == 3 )
	{
		divide(cv::Scalar(255,255,255)-gray, cv::Scalar(255,255,255)-neg, blend, 64);
		blend = cv::Scalar(255,255,255) - blend;
	}

	return blend;
}

Mat sketch_Sobel( Mat src, bool bPencel )
{
	Mat sobelX, sobelY;
	cv::Sobel(src,sobelX,CV_16S,1,0);
	cv::Sobel(src,sobelY,CV_16S,0,1);

	cv::Mat sobel;
	//compute the L1 norm
	sobel= abs(sobelX)+abs(sobelY);


	// Find Sobel max value
	double sobmin, sobmax;
	cv::minMaxLoc(sobel,&sobmin,&sobmax);

	// Conversion to 8-bit image
	// sobelImage = -alpha*sobel + 255
	cv::Mat sobelImage;
	sobel.convertTo(sobelImage,CV_8U,-255./sobmax,255);

	Mat sobelThresholded;
	cv::threshold(sobelImage, sobelThresholded, 230, 255, cv::THRESH_BINARY);

	if ( bPencel )
		return sobelImage;

	return sobelThresholded;
}

Mat waterColor( Mat &src )
{
	double dsize = 7, sigma = 32;
	int iterate = 20;

	Mat t1 = src.clone(), t2;

	for ( int i = 0; i < iterate; i++ )
	{
		if ( i % 2 == 0 )
			bilateralFilter( t1, t2, dsize, sigma, sigma );
		else
			bilateralFilter( t2, t1, dsize, sigma, sigma );
	}

	if ( iterate % 2 == 0 )
		return t1;

	return t2;
}

Mat	GetHistogram( Mat src )
{
	Mat			gray;
	Mat			histogram;
	const int*	channel_numbers = { 0 };
	float		channel_range[] = { 0.0, 256.0 };
	const float* channel_ranges = channel_range;
	int			histSize = 256;						//총 단계수. 기본 256 단계

	if ( src.channels() ==  4 )
		cvtColor( src, gray, CV_BGRA2GRAY );
	else if ( src.channels() ==  3 )
		cvtColor( src, gray, CV_BGR2GRAY );
	else if ( src.channels() ==  1 )
		src.copyTo( gray );

	calcHist( &gray, 1, channel_numbers, Mat(), histogram, 1, &histSize, &channel_ranges );

	return histogram;
}

//width가 4의 배수가 아니면 scvDrawImage함수에서 제대로 그려지지 않는다.
//read, cut, resize 등으로 width의 변경이 있을 경우는 이 함수로 보정해주자.
int matMake4Width( Mat &src )
{
	if ( src.cols % 4 == 0 )
		return 0;

	int padding = 0;
	int	bpp = 8 * src.channels();

	//32 bit image is always DWORD aligned because each pixel requires 4 bytes
	//위와 같이 써있어서 아래 if문을 사용했었으나 가로크기가 4의 배수가 아니면 밀린다.
	//if ( bpp < 32 )
		padding = 4 - (src.cols % 4);

	if ( padding == 4 )
		padding = 0;

	cv::Mat tmpImg;

	if ( padding > 0 || src.isContinuous() == false )
	{
		// Adding needed columns on the right (max 3 px)
		cv::copyMakeBorder( src, tmpImg, 0, 0, 0, padding, cv::BORDER_CONSTANT, BORDER_CONSTANT );
		tmpImg.copyTo( src );
	}

	return padding;
}

int drawArrowLine( Mat& img, cv::Point2f p, cv::Point2f q, cv::Scalar line_color,
					double scale, int line_thickness, int line_type, int shift, double tipLength,
					double minLength, double maxLength, bool bDrawArrowLine )
{
	int		width	= img.cols;
	int		height	= img.rows;
	int		dir		= -1;		//4방향값
	double	vector_length; 

	double	x_diff = abs(p.x - q.x);
	double	y_diff = abs(p.y - q.y);

	bool	bOneColor = false;

	vector_length = sqrt( y_diff*y_diff + x_diff*x_diff );

	//left direction. cyan
	if ((x_diff > y_diff) && (p.x > q.x))
	{
		line_color = (bOneColor ? line_color : cv::Scalar(255, 255, 128));
		dir = 3;
	}
	//right direction. yellow
	else if ((x_diff > y_diff) && (p.x < q.x))
	{
		line_color = (bOneColor ? line_color : cv::Scalar(64, 255, 255));
		dir = 1;
	}
	//up direction. blue
	else if ((x_diff < y_diff) && (p.y > q.y))
	{
		line_color = (bOneColor ? line_color : cv::Scalar(255, 0, 0));
		dir = 0;
	}
	//down direction. violet
	else
	{
		line_color = (bOneColor ? line_color : cv::Scalar(255, 0, 255));
		dir = 2;
	}


	//일정 길이 이하 화살표는 그리지 않는다.
	if (minLength > 0.0 && vector_length < minLength)
		return -1;
	if (maxLength > 0.0 && vector_length > maxLength)
		return -1;

	if ( !bDrawArrowLine )
		return dir;

	//draw arrow
	double	angle = atan2((double)p.y - q.y, (double)p.x - q.x);
	double	dArrowSize = 4.0;

	q.x = (int)(p.x - scale * vector_length * cos(angle));  //scale 확대 길이로 화살표 그려줌
	q.y = (int)(p.y - scale * vector_length * sin(angle));

	cv::line( img, p, q, line_color, line_thickness, line_type, shift );
	p.x = (int) (q.x + dArrowSize * cos(angle + CV_PI / 4));
	if(p.x>=width)
		p.x=width-1;
	else if(p.x<0)
		p.x=0;
	p.y = (int) (q.y + dArrowSize * sin(angle + CV_PI / 4));
	if(p.y>=height)
		p.y=height-1;
	else if(p.y<0)
		p.y=0;
	cv::line( img, p, q, line_color, line_thickness, line_type, shift );
	p.x = (int) (q.x + dArrowSize * cos(angle - CV_PI / 4));
	if(p.x>=width)
		p.x=width-1;
	else if(p.x<0)
		p.x=0;
	p.y = (int) (q.y + dArrowSize * sin(angle - CV_PI / 4));
	if(p.y>height)
		p.y=height-1;
	else if(p.y<0)
		p.y=0;
	cv::line( img, p, q, line_color, line_thickness, line_type, shift );

	return dir;
}

//cv::Rect drawText( cv::Mat& img, const std::string &text, cv::Point org, int nFormat, int nMargin,
//					int fontFace, double fontScale, cv::Scalar color, int thickness=1, int lineType=8, bool bottomLeftOrigin=false )
cv::Rect	drawText(	cv::Mat& img,
						const std::string &text,
						cv::Point org,// = cv::Point(-1, -1),
						int format,// = SCV_DT_LEFT | SCV_DT_TOP,
						int margin,// = 10,
						int fontFace,// = FONT_HERSHEY_DUPLEX,
						double fontScale,// = 1.0,
						cv::Scalar color,// = cv::Scalar(0,0,0),
						int thickness,// = 1,
						int lineType,// = 8,
						bool bottomLeftOrigin// = false
					)
{
	int width = 0;
	int height = 0;

	//pt가 (-1, -1)로 넘어오면 이미지 전체 영역을 기준으로 align이 결정되고
	//그 이외의 값이면 해당 점을 기준으로 align이 결정된다.
	if ( org == cv::Point(-1,-1) )
	{
		org.x = org.y = 0;
		width = img.cols - 1;
		height = img.rows - 1;
	}

	int baseline = 0;
	cv::Size textSize = cv::getTextSize( text, fontFace, fontScale, thickness, &baseline );
	double x = org.x + margin;
	double y = org.y + textSize.height + margin;

	baseline += thickness;
	//TRACE( "baseline = %d\n", baseline );

	if ( format & SCV_DT_CENTER )
		x = org.x + width / 2.0 - textSize.width / 2.0;
	else if ( format & SCV_DT_RIGHT )
		x = org.x + width - textSize.width - margin;

	if ( format & SCV_DT_VCENTER )
		y = org.y + height / 2.0 + textSize.height / 2.0;
	else if ( format & SCV_DT_BOTTOM )
		y = org.y + height - margin;

	putText( img, text, cv::Point(x, y), fontFace,
		fontScale, color, thickness, lineType );

	return cv::Rect( x, y, textSize.width, textSize.height );
}


void  matMosaic( Mat src, Mat& dst, int w, int h )
{
	if ( dst.data )
		dst.release();

	dst = Mat( (src.rows / h) * h, (src.cols / w) * w, src.type() );

	uchar* target = (uchar*)dst.data;
	uchar* source = (uchar*)src.data;

	cv::Rect roi;
	roi.width = w;
	roi.height = h;

	for(int idy = 0; idy < (src.rows/h) ; idy++)
	{
		for(int idx = 0; idx < (src.cols/w) ; idx++)
		{
			roi.x = idx*w;
			roi.y = idy*h;

			if (dst.channels() == 1) // Gray 영상일 경우
			{
				Mat sub_img_src = src(roi);//메모리공유
				Mat sub_img_dest = dst(roi);

				double sumOfSubset = (int) (cv::sum(sub_img_src).val[0] / (w * h));

				sub_img_dest = Mat::ones(h, w, src.type()) * sumOfSubset;
			}
			else if (dst.channels()==3) // RGB 영상일 경우
			{

				Mat sub_img_src = src(roi);//메모리공유
				Mat sub_img_dest = dst(roi);
				Mat sub_img_mask(w, h, src.type());

				uchar* data = (uchar*)sub_img_mask.data;
				int length = sub_img_mask.rows  * sub_img_mask.cols;

				int sumOfSubsetB = (int) (cv::sum(sub_img_src).val[0] / (w * h));
				int sumOfSubsetG = (int) (cv::sum(sub_img_src).val[1] / (w * h));
				int sumOfSubsetR = (int) (cv::sum(sub_img_src).val[2] / (w * h));

				sub_img_dest = Mat::ones(h, w, src.type()) * sumOfSubsetR;

				for(int i=0;i<length;i++)
				{
					data[i*3+0] = cv::saturate_cast<uchar>( sumOfSubsetB );
					data[i*3+1] = cv::saturate_cast<uchar>( sumOfSubsetG );
					data[i*3+2] = cv::saturate_cast<uchar>( sumOfSubsetR );
				}
				sub_img_mask.copyTo(sub_img_dest);
			}
		}
	}
}

void matSamplingResize( Mat src, Mat&dst, int w, int h )
{
	if ( dst.data )
		dst.release();

	dst = Mat(src.rows / h, src.cols / w, src.type());

	uchar* target = (uchar*)dst.data;
	uchar* source = (uchar*)src.data;

	cv::Rect roi;
	roi.width = w;
	roi.height = h;

	for(int idy = 0; idy < (src.rows/h) ; idy++)
	{
		for(int idx = 0; idx < (src.cols/w) ; idx++)
		{
			roi.x = idx * w;
			roi.y = idy * h;

			if (dst.channels() == 1) // Gray 영상일 경우
			{
				Mat sub_img_src = src(roi);//메모리공유

				double sumOfSubset = (int) (cv::sum(sub_img_src).val[0] / (w * h));

				target[idy*dst.cols + idx] = cv::saturate_cast<uchar>( sumOfSubset );
			}
			else if (dst.channels()==3) // RGB 영상일 경우
			{

				Mat sub_img_src = src(roi);//메모리공유

				int sumOfSubsetB = (int) (cv::sum(sub_img_src).val[0] / (w * h));
				int sumOfSubsetG = (int) (cv::sum(sub_img_src).val[1] / (w * h));
				int sumOfSubsetR = (int) (cv::sum(sub_img_src).val[2] / (w * h));

				target[(idy*dst.cols + idx) * 3 + 0] = cv::saturate_cast<uchar>( sumOfSubsetB );
				target[(idy*dst.cols + idx) * 3 + 1] = cv::saturate_cast<uchar>( sumOfSubsetG );
				target[(idy*dst.cols + idx) * 3 + 2] = cv::saturate_cast<uchar>( sumOfSubsetR );
			}
		}
	}
}

//두 사각형의 겹치는 영역을 리턴한다.
cv::Rect	getIntersectionRect( cv::Rect r1, cv::Rect r2 )
{
	cv::Rect	r(0, 0, 0, 0);

	if (r1.x > r2.x + r2.width ) return r;
	if (r1.x + r1.width < r2.x ) return r;
	if (r1.y > r2.y + r2.height ) return r;
	if (r1.y + r1.height < r2.y ) return r;

	r.x = MAX( r1.x, r2.x );
	r.y = MAX( r1.y, r2.y );
	r.width = MIN( r1.x + r1.width, r2.x + r2.width ) - r.x;
	r.height = MIN( r1.y + r1.height, r2.y + r2.height ) - r.y;

	return r;
}

//rTarget에 접하는 dRatio인 최대 사각형을 구한다.
cv::Rect	getRatioRect(cv::Rect rTarget, double dRatio)
{
	int		w = rTarget.width;
	int		h = rTarget.height;
	int		nNewW;
	int		nNewH;
	double	dTargetRatio = (double)rTarget.width / (double)rTarget.height;

	cv::Rect	rResult;

	if (rTarget.area() <= 0)
		return cv::Rect();

	bool bResizeWidth;

	if (dRatio > 1.0)
	{
		if (dTargetRatio < dRatio)
			bResizeWidth = false;
		else
			bResizeWidth = true;
	}
	else
	{
		if (dTargetRatio > dRatio)
			bResizeWidth = true;
		else
			bResizeWidth = false;
	}


	if (bResizeWidth)
	{
		rResult.y = rTarget.y;
		rResult.height = rTarget.height;

		nNewW = (double)(rTarget.height) * dRatio;
		rResult.x = rTarget.x + (rTarget.width - nNewW) / 2.0;
		rResult.width = nNewW;
	}
	else
	{
		rResult.x = rTarget.x;
		rResult.width = rTarget.width;

		nNewH = (double)(rTarget.width) / dRatio;
		rResult.y = rTarget.y + (rTarget.height - nNewH) / 2.0;
		rResult.height = nNewH;
	}

	return rResult;
}

double		getOverlappedRatio( cv::Rect r1, cv::Rect r2 )
{
	cv::Rect r = getIntersectionRect( r1, r2 );

	if ( r1.width == 0 || r1.height == 0 )
		return 0.0;

	return ( (double)(r.width * r.height) / (double)(r1.width * r1.height) );
}

bool		isRectEmpty( cv::Rect r )
{
	return ( r.width == 0 || r.height == 0 );
}

bool		isRectNull( cv::Rect r )
{
	return ( r.width == 0 && r.height == 0 );
}

CString		getRectInfo( cv::Rect r )
{
	CString	str;
	str.Format( _T("%d, %d, %d, %d"), r.x, r.y, r.width, r.height );
	return str;
}

cv::Rect	inflateRect( cv::Rect r, int offsetX, int offsetY )
{
	cv::Rect result = r;

	return inflateRect( r.x, r.y, r.width, r.height, offsetX, offsetY );
}

cv::Rect	inflateRect( int x, int y, int w, int h, int offsetX, int offsetY )
{
	cv::Rect result( x, y, w, h );

	result.x -= offsetX;
	result.y -= offsetY;
	result.width += offsetX * 2;
	result.height += offsetY * 2;

	return result;
}

void adjustRectRange( cv::Rect &rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize )
{
	int32_t l = rect.x;
	int32_t t = rect.y;
	int32_t r = rect.x + rect.width;
	int32_t b = rect.y + rect.height;
	adjustRectRange( &l, &t, &r, &b, minx, miny, maxx, maxy, bRetainSize );
	rect.x = l;
	rect.y = t;
	rect.width = r - l;
	rect.height = b - t;
}

Mat	data2Mat( uint8_t *data, int w, int h, int ch )
{
	Mat mat( h, w, CV_MAKETYPE(CV_8U,ch) );
	memcpy( mat.data, data, w * h * ch );
	return mat;
}

void showMat( CString name, Mat mat, double dZoom, int x, int y )
{
#ifdef _UNICODE
	CT2CA pszConvertedAnsiString(name);
	std::string ststr(pszConvertedAnsiString);
#else
	std::string ststr((LPCTSTR)name);
#endif

	if ( dZoom == 1.0 )
		namedWindow(ststr, WINDOW_AUTOSIZE );
	else 
	{
		namedWindow(ststr, WINDOW_NORMAL );
		setWindowProperty(ststr, CV_WND_PROP_ASPECTRATIO , CV_WINDOW_KEEPRATIO );
	}

	if ( x >= 0 && y >= 0 )
		moveWindow(ststr, x, y );
	resizeWindow(ststr, (int)((double)mat.cols * dZoom), (int)((double)mat.rows * dZoom) );
	imshow(ststr, mat );
}

void showRawImage( CString name, uint8_t *data, int width, int height, int ch, double dZoom, int x, int y )
{
#ifdef _UNICODE
	CT2CA pszConvertedAnsiString(name);
	std::string ststr(pszConvertedAnsiString);
#else
	std::string ststr((LPCTSTR)name);
#endif

	if ( dZoom == 1.0 )
		namedWindow(ststr, WINDOW_AUTOSIZE );
	else 
	{
		namedWindow(ststr, WINDOW_NORMAL  );
		setWindowProperty(ststr, CV_WND_PROP_ASPECTRATIO , CV_WINDOW_KEEPRATIO );
	}

	if ( x >= 0 && y >= 0 )
		moveWindow(ststr, x, y );
	resizeWindow(ststr, (int)((double)width * dZoom), (int)((double)height * dZoom) );

	imshow( ststr, data2Mat(data, width, height, ch) );
}

void saveRawImage( CString sfile, uint8_t *data, int width, int height, int ch )
{
#ifdef _UNICODE
	CT2CA pszConvertedAnsiString(sfile);
	std::string ststr(pszConvertedAnsiString);
#else
	std::string ststr((LPCTSTR)sfile);
#endif

	imwrite( ststr, data2Mat(data, width, height, ch) );
}

void save2Raw( cv::Mat mat, CString sRawFilename, int dst_ch )
{
	FILE	*fp = _tfopen( sRawFilename, _T("wb") );
	if ( fp == NULL )
		return;

	if ( mat.channels() != dst_ch )
	{
		if ( dst_ch == 1 )
		{
			if ( mat.channels() == 4 )
				cv::cvtColor( mat, mat, CV_BGRA2GRAY );
			else
				cv::cvtColor( mat, mat, CV_BGR2GRAY );
		}
		else if ( dst_ch == 3 )
		{
			if ( mat.channels() == 4 )
				cv::cvtColor( mat, mat, CV_BGRA2BGR );
			else
				cv::cvtColor( mat, mat, CV_GRAY2BGR );
		}
	}

	fwrite( mat.data, mat.cols * mat.rows * dst_ch, 1, fp );
	fclose( fp );
}

//너비가 4의 배수가 아닌 경우 밀림!
HBITMAP ConvertMatToBMP (cv::Mat frame)
{      
	auto convertOpenCVBitDepthToBits = [](const int value)
	{
		auto regular = 0u;

		switch (value)
		{
		case CV_8U:
		case CV_8S:
			regular = 8u;
			break;

		case CV_16U:
		case CV_16S:
			regular = 16u;
			break;

		case CV_32S:
		case CV_32F:
			regular = 32u;
			break;

		case CV_64F:
			regular = 64u;
			break;

		default:
			regular = 0u;
			break;
		}

		return regular;
	};

	auto imageSize = frame.size();
	assert(imageSize.width && "invalid size provided by frame");
	assert(imageSize.height && "invalid size provided by frame");

	if (imageSize.width && imageSize.height)
	{
		BITMAPINFOHEADER headerInfo;
		ZeroMemory(&headerInfo, sizeof(headerInfo));

		headerInfo.biSize     = sizeof(headerInfo);
		headerInfo.biWidth    = MAKE4WIDTH(imageSize.width);
		headerInfo.biHeight   = -(imageSize.height); // negative otherwise it will be upsidedown
		headerInfo.biPlanes   = 1;// must be set to 1 as per documentation frame.channels();

		const auto bits       = convertOpenCVBitDepthToBits( frame.depth() );
		headerInfo.biBitCount = frame.channels() * bits;
		//headerInfo.biSizeImage= sizeof(BYTE) * headerInfo.biWidth * headerInfo.biHeight * frame.channels();

		BITMAPINFO bitmapInfo;
		ZeroMemory(&bitmapInfo, sizeof(bitmapInfo));

		bitmapInfo.bmiHeader              = headerInfo;
		bitmapInfo.bmiColors->rgbBlue     = 0;
		bitmapInfo.bmiColors->rgbGreen    = 0;
		bitmapInfo.bmiColors->rgbRed      = 0;
		bitmapInfo.bmiColors->rgbReserved = 0;

		auto dc  = GetDC(nullptr);
		assert(dc != nullptr && "Failure to get DC");
		auto bmp = CreateDIBitmap(dc,
			&headerInfo,
			CBM_INIT,
			frame.data,
			&bitmapInfo,
			DIB_RGB_COLORS);
		assert(bmp != nullptr && "Failure creating bitmap from captured frame");

		return bmp;
	}
	else
	{
		return nullptr;
	}
}

bool PasteBMPToClipboard(void* bmp)
{
	assert(bmp != nullptr && "You need a bmp for this function to work");

	if (OpenClipboard(NULL) && bmp != nullptr)
	{
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, bmp);
		CloseClipboard();
		return true;
	}
	else
	{
		return false;
	}	
}

Mat HBITMAP2Mat( HWND hwnd, HBITMAP hbwindow )
{
	HDC hwindowDC,hwindowCompatibleDC;

	int height, width;
	Mat mat;
	BITMAPINFO bmi;
	BITMAPINFOHEADER* bmih = &(bmi.bmiHeader);

	ZeroMemory(bmih, sizeof(BITMAPINFOHEADER));
	bmih->biSize = sizeof(BITMAPINFOHEADER);

	hwindowDC=::GetDC(hwnd);
	hwindowCompatibleDC=CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC,COLORONCOLOR);  

	if ( GetDIBits(hwindowDC, hbwindow, 0, 0, NULL, &bmi, DIB_RGB_COLORS) )
	{
		height = bmih->biHeight;
		width = bmih->biWidth;

		mat.create( height, width, CV_8UC4 );

		// create a bitmap
		//hbwindow = CreateCompatibleBitmap( hwindowDC, width, height);
		bmih->biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
		bmih->biWidth = width;    
		bmih->biHeight = -height;  //this is the line that makes it draw upside down or not
		bmih->biPlanes = 1;    
		bmih->biBitCount = 32;    
		bmih->biCompression = BI_RGB;    
		bmih->biSizeImage = 0;  
		bmih->biXPelsPerMeter = 0;    
		bmih->biYPelsPerMeter = 0;    
		bmih->biClrUsed = 0;    
		bmih->biClrImportant = 0;

		// use the previously created device context with the bitmap
		HBITMAP bitmapOld = (HBITMAP)SelectObject(hwindowCompatibleDC, hbwindow);
		// copy from the window device context to the bitmap device context
		StretchBlt( hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, width, height, SRCCOPY ); //change SRCCOPY to NOTSRCCOPY for wacky colors !
		GetDIBits( hwindowCompatibleDC, hbwindow, 0, height, mat.data, (BITMAPINFO *)&bmi, DIB_RGB_COLORS );  //copy from hwindowCompatibleDC to hbwindow

		// avoid memory leak
		//DeleteObject( hbwindow );
		DeleteDC( hwindowCompatibleDC );
		ReleaseDC( hwnd, hwindowDC );
	}

	imshow( "mat", mat );
	return mat;
}

Mat hwnd2mat(HWND hwnd){

	HDC hwindowDC,hwindowCompatibleDC;

	int height,width,srcheight,srcwidth;
	HBITMAP hbwindow;
	Mat src;
	BITMAPINFOHEADER  bi;

	hwindowDC=GetDC(hwnd);
	hwindowCompatibleDC=CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC,COLORONCOLOR);  

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	srcheight = windowsize.bottom;
	srcwidth = windowsize.right;
	height = windowsize.bottom/2;  //change this to whatever size you want to resize to
	width = windowsize.right/2;

	src.create(height,width,CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap( hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;    
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;    
	bi.biBitCount = 32;    
	bi.biCompression = BI_RGB;    
	bi.biSizeImage = 0;  
	bi.biXPelsPerMeter = 0;    
	bi.biYPelsPerMeter = 0;    
	bi.biClrUsed = 0;    
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt( hwindowCompatibleDC, 0,0, width, height, hwindowDC, 0, 0,srcwidth,srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC,hbwindow,0,height,src.data,(BITMAPINFO *)&bi,DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

	// avoid memory leak
	DeleteObject (hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hwnd, hwindowDC);

	return src;
}

void FillGridPattern( Mat &src, int size )
{
	int x, y;
	bool toggle = true;

	for ( y = 0; y < src.rows; y += size )
	{
		toggle = !toggle;
		for ( x = 0; x < src.cols; x += size )
		{
			if ( toggle )
				rectangle( src, cv::Point(x,y), cv::Point(x+size,y+size), cv::Scalar(128,128,192), -1);
			else
				rectangle( src, cv::Point(x,y), cv::Point(x+size,y+size), cv::Scalar(192,140,140), -1);
			toggle = !toggle;
		}
	}
}

//paperWidth, paperHeight, dx, dy의 단위는 픽셀이 아닌 모두 mm 단위이다.
void printMat( Mat img, CString sPrinterName, CString sDocName, int x, int y, int dw, int dh, int copies )
{
	CDC   PrinterDC;
	DOCINFO  DocInfo = { sizeof(DOCINFO) };
	CString  str;

	DocInfo.lpszDocName = sDocName;

	if ( !PrinterDC.CreateDC( NULL, sPrinterName, NULL, NULL ) )
	{
		str.Format(_T("\"%s\" 라는 이름의 프린터를 찾을 수 없습니다.\n프린터 이름을 확인하시기 바랍니다."), sPrinterName );
		AfxMessageBox( str );
		return;
	}

	int		nDPIX = GetDeviceCaps( PrinterDC, LOGPIXELSX ); //프린터의 dpi를 구해서
	int		nDPIY = GetDeviceCaps( PrinterDC, LOGPIXELSY );
	double	dx = (double)nDPIX / 25.4;      //mm 단위로 변환한다.
	double	dy = (double)nDPIY / 25.4;      //그래야 이미지를 원하는 mm 크기로 인쇄할 수 있다.

	CPrintDialog *pPrintDlg = new CPrintDialog( FALSE );
	pPrintDlg->GetDefaults();
	//DEVMODE *pDevMode;

	/*
	lpDevMode = (DEVMODE*)::GlobalLock( pPrintDlg->GetDevMode() );
	//lpDevMode->dmPrintQuality = DMRES_HIGH;
	lpDevMode->dmOrientation = DMORIENT_PORTRAIT;
	lpDevMode->dmPaperWidth = paperWidth * 10;
	lpDevMode->dmPaperLength = paperHeight * 10;
	lpDevMode->dmCopies = copies;
	lpDevMode->dmFields |= DM_PAPERLENGTH | DM_PAPERWIDTH | DM_PAPERSIZE;
	lpDevMode->dmPaperSize = DMPAPER_USER;

	//lpDevMode->dmColor = DMCOLOR_COLOR;
	//	lpDevMode->dmColor = DMCOLOR_MONOCHROME;
	//	lpDevMode->dmICMMethod = DMICMMETHOD_SYSTEM;
	PrinterDC.ResetDC( lpDevMode );
	::GlobalUnlock( lpDevMode );
	*/

	/*
	pDevMode = (DEVMODE *)::GlobalLock( pPrintDlg->GetDevMode() );
	pDevMode->dmPaperSize = 291;
	pDevMode->dmPaperWidth  = paperWidth * 10;
	pDevMode->dmPaperLength = paperHeight * 10;
	pDevMode->dmFields &= ~DM_PAPERSIZE;
	pDevMode->dmFields &= ~DM_PAPERLENGTH;
	pDevMode->dmFields &= ~DM_PAPERWIDTH;
	pDevMode->dmFields |= (DM_PAPERSIZE | DM_PAPERLENGTH | DM_PAPERWIDTH);
	::GlobalUnlock( pPrintDlg->GetDevMode() );
	PrinterDC.ResetDC(pDevMode);
	*/
	if ( PrinterDC.StartDoc( &DocInfo ) < 0 )
	{
		PrinterDC.AbortDoc();
		PrinterDC.Detach();
		AfxMessageBox(_T("인쇄를 시작할 수 없습니다. (StartDoc fail)") );
		PrinterDC.DeleteDC();
		return;
	}

	if ( PrinterDC.StartPage() < 0 )
	{
		AfxMessageBox(_T("PrintDlg : StartPage fail.") );
		PrinterDC.EndDoc();
		PrinterDC.DeleteDC();
		return;
	}

	//용지 크기는 250 x 190
	//인쇄 크기는 210 x 150
	//imshow( "final", m_FinalBuild );

	//가로사진 또는 배경합성일 경우는 최종 합성 이미지를 90도 회전시켜 출력시켜야 한다.
	//주의 : img 이미지의 width가 4의 배수가 아니면 rotate90에서 강제 리턴됨.
	if ( img.cols > img.rows )
	{
		rotate90( img, 1 );
		SWAP( x, y );
		SWAP( dw, dh );
	}

	//Epson 프린터의 용지 크기 설정을 기본값인 A4로 그대로 사용한 상태에서
	//x=25, y=15에서 시작하면 위의 용지에 정확히 중앙 인쇄된다.
	//즉, 프린터의 공장출고 설정을 그대로 사용하면 되며 관리자는 프린터의 어떠한 추가 설정이 필요치 않다.

	scvDrawImage(&PrinterDC, img, (double)x * dx, (double)y * dy, (double)(x + dw) * dx, (double)(y + dh) * dy);

	PrinterDC.EndPage();
	PrinterDC.EndDoc();
	PrinterDC.DeleteDC();
}

CSize getCutSizeToFitPrinterPaper( Mat img, CString sPrinterName, int marginX, int marginY )
{
	return getCutSizeToFitPrinterPaper( img.cols, img.rows, sPrinterName, marginX, marginY );
}

CSize getCutSizeToFitPrinterPaper( int w, int h, CString sPrinterName, int marginX, int marginY )
{
	bool	bRotated = false;
	CDC		PrinterDC;
	CString str;
	CSize	szCut( 0, 0 );

	if ( !PrinterDC.CreateDC( NULL, sPrinterName, NULL, NULL ) )
	{
		str.Format(_T("\"%s\" 라는 이름의 프린터를 찾을 수 없습니다.\n프린터 이름을 확인하시기 바랍니다."), sPrinterName );
		AfxMessageBox( str );
		return szCut;
	}

	int		nDPIX = GetDeviceCaps( PrinterDC, LOGPIXELSX ); //프린터의 dpi를 구해서
	int		nDPIY = GetDeviceCaps( PrinterDC, LOGPIXELSY );
	int		nPaperWidth = GetDeviceCaps( PrinterDC, HORZSIZE );
	int		nPaperHeight = GetDeviceCaps( PrinterDC, VERTSIZE );

	//가로사진 또는 배경합성일 경우는 최종 합성 이미지를 90도 회전시켜 출력시켜야 한다.
	//주의 : img 이미지의 width가 4의 배수가 아니면 rotate90에서 강제 리턴됨.
	if ( w > h )
	{
		SWAP( w, h ); 
		bRotated = true;
	}

	//가로, 세로 인쇄 여백 수치를 적용해서 축소한 후 출력.
	double dPrinterRatio = (double)(nPaperWidth - marginX * 2) / (double)(nPaperHeight - marginY * 2);
	double dImageRatio = (double)w / (double)h;

	if ( dPrinterRatio != dImageRatio )
	{
		if ( dImageRatio > dPrinterRatio )
			szCut.cx = (w - (double)h * dPrinterRatio) / 2.0;
		else
			szCut.cy = (h - (double)w / dPrinterRatio) / 2.0;

		//img(cv::Rect( szCut.cx, szCut.cy, img.cols - szCut.cx * 2, img.rows - szCut.cy * 2)).copyTo(img);
	}

	//위에서 가로사진을 회전해서 잘릴 크기를 구했으므로 다시 뒤집어줘야 한다.
	if ( bRotated )
		SWAP( szCut.cx, szCut.cy );

	PrinterDC.DeleteDC();

	return szCut;
}

//프린터 설정 용지 크기대로 인쇄
void printMat( Mat &img, CString sPrinterName, CString sDocName, int marginX, int marginY, int copies )
{
	CDC   PrinterDC;
	DOCINFO  DocInfo = { sizeof(DOCINFO) };
	CString  str;

	DocInfo.lpszDocName = sDocName;

	if ( !PrinterDC.CreateDC( NULL, sPrinterName, NULL, NULL ) )
	{
		str.Format( _T("\"%s\" This printer was not found.\nCheck default printer in Control Panel."), sPrinterName );
		AfxMessageBox( str );
		return;
	}

	int		nDPIX = GetDeviceCaps( PrinterDC, LOGPIXELSX ); //프린터의 dpi를 구해서
	int		nDPIY = GetDeviceCaps( PrinterDC, LOGPIXELSY );
	int		nPaperWidth = GetDeviceCaps( PrinterDC, HORZSIZE );
	int		nPaperHeight = GetDeviceCaps( PrinterDC, VERTSIZE );
	double	dx = (double)nDPIX / 25.4;      //mm 단위로 변환한다.
	double	dy = (double)nDPIY / 25.4;      //그래야 이미지를 원하는 mm 크기로 인쇄할 수 있다.

	CPrintDialog *pPrintDlg = new CPrintDialog( FALSE );
	pPrintDlg->GetDefaults();
	DEVMODE *pDevMode = (DEVMODE*)::GlobalLock( pPrintDlg->GetDevMode() );
	pDevMode->dmOrientation = DMORIENT_PORTRAIT; //항상 세로방향을 기준으로 출력한다. 가로사진은 회전시켜 출력한다.
	pDevMode->dmCopies = copies;
	pDevMode->dmFields = pDevMode->dmFields | DM_ORIENTATION | DM_COPIES;
	PrinterDC.ResetDC( pDevMode );
	::GlobalUnlock( pDevMode );


	if ( PrinterDC.StartDoc( &DocInfo ) < 0 )
	{
		PrinterDC.AbortDoc();
		PrinterDC.Detach();
		AfxMessageBox(_T("인쇄를 시작할 수 없습니다. (StartDoc fail)") );
		PrinterDC.DeleteDC();
		return;
	}

	if ( PrinterDC.StartPage() < 0 )
	{
		AfxMessageBox(_T("PrintDlg : StartPage fail.") );
		PrinterDC.EndDoc();
		PrinterDC.DeleteDC();
		return;
	}

	//가로사진 또는 배경합성일 경우는 최종 합성 이미지를 90도 회전시켜 출력시켜야 한다.
	//주의 : img 이미지의 width가 4의 배수가 아니면 rotate90에서 강제 리턴됨.
	if ( img.cols > img.rows )
	{
		rotate90( img, 1 );
	}

	//가로, 세로 인쇄 여백 수치를 적용해서 축소한 후 출력.
	//double dPrinterRatio = (double)(nPaperWidth - marginX * 2) / (double)(nPaperHeight - marginY * 2);
	//double dImageRatio = (double)img.cols / (double)img.rows;
	//CSize szCut( 0, 0 );

	//if ( dPrinterRatio != dImageRatio )
	//{
	//	if ( dImageRatio > dPrinterRatio )
	//		szCut.cx = (img.cols - (double)img.rows * dPrinterRatio) / 2.0;
	//	else
	//		szCut.cy = (img.rows - (double)img.cols / dPrinterRatio) / 2.0;

	//	img(cv::Rect( szCut.cx, szCut.cy, img.cols - szCut.cx * 2, img.rows - szCut.cy * 2)).copyTo(img);
	//}

	scvDrawImage( &PrinterDC, img, (double)marginX * dx, (double)marginY * dy,
				(double)(nPaperWidth - marginX) * dx, (double)(nPaperHeight - marginY) * dy);

	PrinterDC.EndPage();
	PrinterDC.EndDoc();
	PrinterDC.DeleteDC();
}

//출처: http://trip2ee.tistory.com/75 [지적(知的) 탐험]
void CannyEdge(uint8_t *src, uint8_t *dst, int width, int height, int th_low, int th_high )
{
	int i, j;
	int dx, dy, mag, slope, direction;
	int index, index2;

	const int fbit = 10;
	const int tan225 =   424;       // tan25.5 << fbit, 0.4142
	const int tan675 =   2472;      // tan67.5 << fbit, 2.4142

	const int CERTAIN_EDGE = 255;
	const int PROBABLE_EDGE = 100;

	bool bMaxima;

	int *mag_tbl = new int[width*height];
	int *dx_tbl = new int[width*height];
	int *dy_tbl = new int[width*height];

	char **stack_top, **stack_bottom;

	stack_top = new char*[width*height];
	stack_bottom = stack_top;

#define CANNY_PUSH(p)    *(p) = CERTAIN_EDGE, *(stack_top++) = (p)
#define CANNY_POP()      *(--stack_top)

	for(i=0; i<width*height; i++)
	{
		mag_tbl[i] = 0;
		dst[i] = 0;
	}

	// Sobel Edge Detection
	for(i=1; i<height-1; i++)
	{
		index = i*width;

		for(j=1; j<width-1; j++)
		{
			index2 = index+j;
			// -1 0 1
			// -2 0 2
			// -1 0 1
			dx = src[index2-width+1] + (src[index2+1]<<1) + src[index2+width+1]
			-src[index2-width-1] - (src[index2-1]<<1) - src[index2+width-1];

			// -1 -2 -1
			//  0  0  0
			//  1  2  1
			dy = -src[index2-width-1] - (src[index2-width]<<1) - src[index2-width+1]
			+src[index2+width-1] + (src[index2+width]<<1) + src[index2+width+1];

			mag = abs(dx)+abs(dy);     // magnitude
			//mag = sqrtf(dx*dx + dy*dy);

			dx_tbl[index2] = dx;
			dy_tbl[index2] = dy;
			mag_tbl[index2] = mag;              // store the magnitude in the table
		}   // for(j)
	}   // for(i)

	for(i=1; i<height-1; i++)
	{
		index = i*width;

		for(j=1; j<width-1; j++)
		{
			index2 = index+j;
			mag = mag_tbl[index2];              // retrieve the magnitude from the table

			// if the magnitude is greater than the lower threshold
			if(mag > th_low)
			{
				// determine the orientation of the edge
				dx = dx_tbl[index2];
				dy = dy_tbl[index2];

				if(dx != 0)
				{
					slope = (dy<<fbit)/dx;

					if(slope > 0)
					{
						if(slope < tan225)
							direction = 0;
						else if(slope < tan675)
							direction = 1;
						else
							direction = 2;
					}
					else
					{
						if(-slope > tan675)
							direction = 2;
						else if(-slope > tan225)
							direction = 3;
						else
							direction = 0;
					}
				}
				else
				{
					direction = 2;
				}

				bMaxima = true;

				// perform non-maxima suppression
				if(direction == 0)
				{
					if(mag < mag_tbl[index2-1] || mag < mag_tbl[index2+1])
						bMaxima = false;
				}
				else if(direction == 1)
				{
					if(mag < mag_tbl[index2+width+1] || mag < mag_tbl[index2-width-1])
						bMaxima = false;
				}
				else if(direction == 2)
				{
					if(mag < mag_tbl[index2+width] || mag < mag_tbl[index2-width])
						bMaxima = false;
				}
				else // if(direction == 3)
				{
					if(mag < mag_tbl[index2+width-1] || mag < mag_tbl[index2-width+1])
						bMaxima = false;
				}

				if(bMaxima)
				{
					if(mag > th_high)
					{
						dst[index2] = CERTAIN_EDGE;           // the pixel does belong to an edge
						*(stack_top++) = (char*)(dst+index2);
					}
					else
					{
						dst[index2] = PROBABLE_EDGE;          // the pixel might belong to an edge
					}
				}
			}
		}   // for(j)
	}   // for(i)

	while(stack_top != stack_bottom)
	{
		char* p = CANNY_POP();

		if(*(p+1) == PROBABLE_EDGE)
			CANNY_PUSH(p+1);

		if(*(p-1) == PROBABLE_EDGE)
			CANNY_PUSH(p-1);

		if(*(p+width) == PROBABLE_EDGE)
			CANNY_PUSH(p+width);

		if(*(p-width) == PROBABLE_EDGE)
			CANNY_PUSH(p-width);

		if(*(p-width-1) == PROBABLE_EDGE)
			CANNY_PUSH(p-width-1);

		if(*(p-width+1) == PROBABLE_EDGE)
			CANNY_PUSH(p-width+1);

		if(*(p+width-1) == PROBABLE_EDGE)
			CANNY_PUSH(p+width-1);

		if(*(p+width+1) == PROBABLE_EDGE)
			CANNY_PUSH(p+width+1);
	}

	for(i=0; i<width*height; i++)
	{
		if(dst[i]!=CERTAIN_EDGE)
			dst[i] = 0;
	}

	delete [] mag_tbl;
	delete [] dx_tbl;
	delete [] dy_tbl;
	delete [] stack_bottom;
}

bool matIsEqual(const cv::Mat Mat1, const cv::Mat Mat2)
{
	if( Mat1.dims == Mat2.dims && 
		Mat1.size == Mat2.size && 
		Mat1.elemSize() == Mat2.elemSize())
	{
		if( Mat1.isContinuous() && Mat2.isContinuous())
		{
			bool b = (0==memcmp( Mat1.ptr(), Mat2.ptr(), Mat1.total()*Mat1.elemSize()));
			return b;
		}
		else
		{
			const cv::Mat* arrays[] = {&Mat1, &Mat2, 0};
			uchar* ptrs[2];
			cv::NAryMatIterator it( arrays, ptrs, 2);
			for(unsigned int p = 0; p < it.nplanes; p++, ++it)
				if( 0!=memcmp( it.ptrs[0], it.ptrs[1], it.size*Mat1.elemSize()) )
					return false;

			return true;
		}
	}

	return false;
}
inline uchar reduceValue(const uchar value, const uchar intervalSize = 4)
{
	return value / intervalSize * intervalSize;
}
 
/**
 * It reduces the image color palette from a total number 256*256*256 of unique colors to something smaller like 32*32*32.
 * @see http://stackoverflow.com/questions/5906693/how-to-reduce-the-number-of-colors-in-an-image-with-opencv
 */
void reduceColorPalette(const Mat& inputImage, Mat& outputImage, const uchar intervalSize)
{
	uchar* inputPixelPtr = inputImage.data;
	uchar* outputPixelPtr = outputImage.data;
 
	if(inputImage.channels() == 3) { // color image
		for (int i = 0; i < inputImage.rows; i++) {
			for (int j = 0; j < inputImage.cols; j++) {
				const int pi = i*inputImage.cols*3 + j*3;
				outputPixelPtr[pi + 0] = reduceValue(inputPixelPtr[pi + 0]); // B
				outputPixelPtr[pi + 1] = reduceValue(inputPixelPtr[pi + 1]); // G
				outputPixelPtr[pi + 2] = reduceValue(inputPixelPtr[pi + 2]); // R
			}
		}
 
	} else if(inputImage.channels() == 1) { // grayscale image
		for (int i = 0; i < inputImage.rows; i++) {
			for (int j = 0; j < inputImage.cols; j++) {
				const int pi = i*inputImage.cols*3 + j*3;
				outputPixelPtr[pi] = reduceValue(inputPixelPtr[pi]); // gray
			}
		}
 
	} else { // not supported image format
		printf("Image type not supported.\n");
		outputImage = inputImage;
	}
}
 
/**
 * This method makes histogram equalization of the pixel intensities.
 * For grayscale images the equalization is done directly using the values of the grayscale channel,
 * but color images the image is transformed to other color space than RGB which separates intensity values
 * from color components, color spaces such HSV/HLS, YUV or YCbCr.
 *
 * @see http://stackoverflow.com/questions/15007304/histogram-equalization-not-working-on-color-image-opencv
 */
void equalizeIntensity(const Mat& inputImage, Mat& outputImage)
{
	if(inputImage.channels() == 3) { // color image
		Mat ycrcb;
		cvtColor(inputImage, ycrcb, CV_BGR2HSV);
		std::vector<Mat> channels;
		split(ycrcb, channels);
		equalizeHist(channels[0], channels[0]);
		Mat result;
		merge(channels, ycrcb);
		cvtColor(ycrcb, outputImage, CV_HSV2BGR);
 
	} else if(inputImage.channels() == 1) { // grayscale image
		equalizeHist(inputImage, outputImage);
 
	} else { // not supported image format
		printf("Image type not supported.\n");
		outputImage = inputImage;
	}
}
 
/**
 * This method computes the gradients of a grayscale image.
 * @param inputImage A grayscale image of type CV_8U
 * @param outputImage A grayscale image of type CV_8U
 */
void computeGradients(const Mat& inputImage, Mat& outputImage)
{
	if(inputImage.channels() == 1) { // grayscale
		// compute the gradients on both directions x and y
		Mat grad_x, grad_y;
		Mat abs_grad_x, abs_grad_y;
		int scale = 1;
		int delta = 0;
		int ddepth = CV_16S; // use 16 bits unsigned to avoid overflow
 
		Scharr( inputImage, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
		//Sobel( input_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
		convertScaleAbs( grad_x, abs_grad_x ); // CV_16S -> CV_8U
 
		Scharr( inputImage, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
		//Sobel( input_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
		convertScaleAbs( grad_y, abs_grad_y ); // CV_16S -> // CV_16S -> CV_8U
 
		// create the output by adding the absolute gradient images of each x and y direction
		addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, outputImage );
 
	} else {
		printf("Image type not supported.\n");
		outputImage = inputImage;
	}
 
}

/**
 * This method extracts the gradient image of a color or grayscale image.
 * @param inputImage A matrix of a color or grayscale image
 * @param outputImage The gradient matrix
 */
void processImage(const Mat& inputImage, Mat& outputImage)
{
	if (inputImage.channels() == 3) { // color image
		/// Apply Histogram Equalization
		//equalizeIntensity(inputImage, inputImage);
 
		// Reduce the maximum number of colors from 256*256*256 to a smaller number such 32*32*32
		//reduceNumberOfColors(inputImage, inputImage);
 
		// Blur the input image to remove the noise
		GaussianBlur(inputImage, inputImage, cv::Size(9, 9), 0, 0, BORDER_DEFAULT);
 
		// Convert it to grayscale (CV_8UC3 -> CV_8UC1)
		Mat image_gray;
		cvtColor(inputImage, image_gray, COLOR_BGR2GRAY);
 
		// Compute the gradient image
		computeGradients(image_gray, image_gray);
		normalize(image_gray, outputImage, 0, 255, NORM_MINMAX, CV_8U);
		//threshold(outputImage, outputImage, 50, 255, THRESH_TOZERO);
 
		// invert the gradient image
		cv::subtract(cv::Scalar::all(255), outputImage, outputImage);
 
	} else if (inputImage.channels() == 1) { // grayscale image
		Mat image_gray = inputImage;
		computeGradients(image_gray, image_gray);
		normalize(image_gray, outputImage, 0, 255, NORM_MINMAX, CV_8U);
		//threshold(outputImage, outputImage, 50, 255, THRESH_TOZERO);
 
		// invert the gradient image
		cv::subtract(cv::Scalar::all(255), outputImage, outputImage);
 
	} else { // not supported image format
		printf("Image type not supported.\n");
		outputImage = inputImage;
	}
}
 
void Sepia( Mat& mat, int magnitude )
{
	int		x, y;
	double	r, g, b;
	TypedMat<uint8_t> tmMat = mat;

	for ( y = 0; y < mat.rows; y++ )
	{
		for ( x = 0; x < mat.cols; x++ )
		{
			r = tmMat(y,x,2);
			g = tmMat(y,x,1);
			b = tmMat(y,x,0);

			tmMat(y,x,2) = MIN( 255, r * 0.393 + g * 0.769 + b * 0.189 );
			tmMat(y,x,1) = MIN( 255, r * 0.349 + g * 0.686 + b * 0.168 );
			tmMat(y,x,0) = MIN( 255, r * 0.272 + g * 0.534 + b * 0.131 );
		}
	}
}

//mat이미지를 w x h 크기로 채운다.
void resizeGrid( Mat& mat, int width, int height )
{
	if ( width <= mat.cols && height <= mat.rows )
		return;

	int	x, y, w, h;
	Mat result( height, width, mat.type() );

	for ( y = 0; y < height; y += mat.rows )
	{
		h = mat.rows;
		if ( (y + h) >= result.rows )
			h = result.rows - y - 1;

		for ( x = 0; x < width; x += mat.cols )
		{
			w = mat.cols;
			if ( (x + w) >= result.cols )
				w = result.cols - x - 1;
			mat(cv::Rect(0, 0, w, h)).copyTo( result(cv::Rect(x, y, w, h)) );
		}
	}

	result.copyTo( mat );
}

void erode_filter( uint8_t* src, int width, int height, int nX, int nY )
{
	if ( src == NULL ) return;

	uint8_t *dst = new uint8_t[width * height];
	memset(dst, 0, width * height);

	erode_filter( src, dst, width, height, nX, nY );
	memcpy( src, dst, width * height );

	delete [] dst;
}

void erode_filter( uint8_t* src, uint8_t* dst, int width, int height, int nX, int nY )
{
	int x, y, m, n, nMin;

	memset( dst, 0, width * height );

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			nMin = (int)src[y * width + x];

			for ( n = y - nY; n < y + nY; n++ )
			{
				for ( m = x - nX; m < x + nX; m++ )
				{
					if ( m >= 0 && m < width && n >= 0 && n < height )
					{
						if ( nMin > src[n * width + m] )
							nMin = src[n * width + m];
					}
				}
			}

			dst[y * width + x] = (uint8_t)nMin;
		}
	}
}


bool dilate_filter( uint8_t* src, uint8_t* dst, int width, int height, int nX, int nY )
{
	int x, y, m, n, nMax;

	memset( dst, 0, width * height );

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			nMax = (int)src[y * width + x];

			for ( n = y - nY; n < y + nY; n++ )
			{
				for ( m = x - nX; m < x + nX; m++ )
				{
					if ( m >= 0 && m < width && n >= 0 && n < height )
					{
						if ( nMax < src[n * width + m] )
							nMax = src[n * width + m];
					}
				}
			}

			dst[y * width + x] = (uint8_t)nMax;
		}
	}

	return true;
}

//타깃 사각형의 비율과 크기에 맞게 src영상을 자르고 resize한다.
Mat getFittingMat( Mat src, int tWidth, int tHeight )
{
	int w = src.cols;
	int h = src.rows;
	int nw = w;
	int nh = h;
	double srcRatio = (double)w / (double)h;
	double tgtRatio = (double)tWidth / (double)tHeight;
	Mat result;

	if ( srcRatio == tgtRatio )
		return src;

	//가로 양쪽이 잘리는 경우
	if ( srcRatio > tgtRatio )
	{
		nw = (double)h * tgtRatio;
		src(cv::Rect( (w - nw)/2.0, 0, nw, nh )).copyTo( result );
	}
	else
	{
		nh = (double)w / tgtRatio;
		src(cv::Rect( 0, (h - nh)/2.0, nw, nh )).copyTo( result );
	}

	resize( result, result, cv::Size(tWidth, tHeight) );
	return result;
}

cv::Scalar	cvGetDefaultColor( int idx )
{
	idx %= 10;

	switch ( idx )
	{
		case 0 : return CV_RGB( 237, 125,  49 );
		case 1 : return CV_RGB(  91, 155, 213 );
		case 2 : return CV_RGB( 165, 255, 165 );
		case 3 : return CV_RGB( 255, 192,   0 );
		case 4 : return CV_RGB(  68, 114, 196 );
		case 5 : return CV_RGB( 112, 173,  71 );
		case 6 : return CV_RGB(  37,  94, 255 );
		case 7 : return CV_RGB( 158,  72,  14 );
		case 8 : return CV_RGB(  99,  99, 199 );
		case 9 : return CV_RGB( 153, 115,   0 );
		default: return CV_RGB( 128, 128, 128 );
	}
}

cv::Scalar	cvGetDefaultColor( int idx, int alpha )
{
	idx %= 10;

	switch ( idx )
	{
		case 0 : return cv::Scalar(  49, 125, 237, alpha );
		case 1 : return cv::Scalar( 213, 155,  91, alpha );
		case 2 : return cv::Scalar( 165, 255, 165, alpha );
		case 3 : return cv::Scalar(   0, 192, 255, alpha );
		case 4 : return cv::Scalar( 196, 114,  68, alpha );
		case 5 : return cv::Scalar(  71, 173, 112, alpha );
		case 6 : return cv::Scalar( 255,  94,  37, alpha );
		case 7 : return cv::Scalar(  14,  72, 158, alpha );
		case 8 : return cv::Scalar( 199,  99,  99, alpha );
		case 9 : return cv::Scalar(   0, 115, 153, alpha );
	}
}

//출처: http://devmonster.tistory.com/22 [Dev.Monster]
//win_size_x = 3, win_size_y = 3이면 3x3격자 크기로 8방향을 검사하게 된다.
//차선 tophat의 경우는 세로 연결에 좀 더 가중치를 두기 위해 3x5로 사용할 수 있다.
//3x5로 사용할 경우 중간의 일부 픽셀은 중복되므로 이를 스킵해야 처리 속도가 줄지만 그 스킵은 미구현.
void pixelLabeling( uint8_t* src, int width, int height, std::vector<std::vector<cv::Point>> &contours, int win_size_x, int win_size_y )
{
	int		labelNumber = 0;
	int		xsize = win_size_x / 2;
	int		ysize = win_size_y / 2;
	stack <cv::Point> st;

	//labeling index가 저장되고 이미 labeling이 된 픽셀인지 기억하기 위한 변수.
	//index는 256개를 넘을 수 있으므로 uint8_t로 하면 오류가 발생된다. 넉넉히 uint32_t로 크게 잡아준다.
	uint32_t* labelled = new uint32_t[width * height];

	memset(labelled, 0, sizeof(uint32_t) * width * height);

	contours.clear();

	for( int y = ysize; y < height - ysize; y++ )
	{
		for( int x = xsize; x < width - xsize; x++ )
		{
			// source image가 0(배경)이고 + Labeling이 수행된 픽셀이면 스킵한다.
			if( src[width * y + x] == 0 || labelled[width * y + x] != 0 ) continue;

			labelNumber++;
			std::vector<cv::Point> vt;
			contours.push_back(vt);

			// 새로운 label seed를 stack에 push
			st.push(cv::Point(x, y));

			// 해당 label seed가 labeling될 때(stack이 빌 때) 까지 수행
			while( !st.empty() )
			{
				// stack top의 label point를 받고 pop
				int ky = st.top().y;
				int kx = st.top().x;
				st.pop();

				// labeling index를 해당 픽셀값으로 할당.
				labelled[width * ky + kx] = labelNumber;
				//배경이 0인 검정색이므로 실제 labelNumber는 1부터 시작된다. contour는 0부터 시작되도록 -1한다.			
				contours[labelNumber-1].push_back(cv::Point(kx,ky));

				// search 8-neighbor
				for( int ny = ky - ysize; ny <= ky + ysize; ny++ )
				{
					// y축 범위를 벗어나는 점 제외
					if( ny < 0 || ny >= height ) continue;

					//for( int nx = kx /*- xsize*/; nx <= kx /*+ xsize*/; nx++ )
					for( int nx = kx - xsize; nx <= kx + xsize; nx++ )
					{
						// x축 범위를 벗어나는 점 제외
						if( nx < 0 || nx >= width ) continue;

						//원래 3x3 또는 3x5일때는 총 8방향 또는 14방향을 모두 검색해야 하지만
						//tophat을 이용한 차선 검출의 경우는 수직성이 강한 점들이 차선일 가능성이 높으므로
						//최상단과 최하단의 양쪽 끝점은 스킵시킨다.
						//if ( (ny == ky - ysize || ny == ky + ysize) &&
						//	 (nx == kx - xsize || nx == kx + xsize) )
						//	 continue;

						// source image가 0(배경)이고 + Labeling이 수행된 픽셀이면 스킵한다.
						if( src[width * ny + nx] == 0 || labelled[width * ny + nx] != 0 ) continue;

						st.push(cv::Point(nx, ny));

						// 탐색한 픽셀이니 labeling
						labelled[width * ny + nx] = labelNumber;
						contours[labelNumber-1].push_back(cv::Point(nx,ny));
					}
				}
			}        
		}
	}

	delete [] labelled;
}

//각 픽셀값이 label된 인덱스 번호를 가지고 있다면(1~n)
//각 label마다 다른 색상으로 표시한 mat을 리턴한다.
//미완성 버전.
Mat getLabelledColorImage( uint8_t* src, int width, int height )
{
	Mat srcMat( height, width, CV_8UC1, src );
	Mat dstMat = Mat::zeros( height, width, CV_8UC3 );
	Mat mask;

	TypedMat<uint8_t> tm_src = srcMat;
	TypedMat<uint8_t> tm_dst = dstMat;

	for ( int i = 1; i < 255; i++ )
	{
		Mat colorMat( srcMat.size(), CV_8UC3, cvGetDefaultColor(i) );
		inRange( srcMat, Scalar(i), Scalar(i), mask );
		colorMat.copyTo( dstMat, mask );
	}
	
	return dstMat;
}

CRect getCRect(cv::Rect r)
{
	return CRect(r.x, r.y, r.x + r.width, r.y + r.height);
}

cv::Point	CenterPoint(cv::Rect r)
{
	return cv::Point((r.x + r.width) / 2.0, (r.y + r.height) / 2.0);
}

void sobel(const unsigned char* src, unsigned char* dst, int w, int h)
{
	int maskX[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
	int maskY[9] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };

	int x, y, xx, yy;
	int sumX, sumY, value;

	for (y = 1; y < h - 1; y++)
	{
		for (x = 1; x < w - 1; x++)
		{
			sumX = sumY = 0;
			for (yy = 0; yy < 3; yy++)
			{
				for (xx = 0; xx < 3; xx++)
				{
					sumX = sumX + src[(y - 1 + yy)*w + (x - 1 + xx)] * maskX[yy * 3 + xx];
					sumY = sumY + src[(y - 1 + yy)*w + (x - 1 + xx)] * maskY[yy * 3 + xx];
				}
			}

			value = abs(sumX) + abs(sumY);

			//0.1을 곱하여 명암을 낮추는 이유는 일반 영상에서는 잘 나타나지 않지만
			//간혹 엣지가 뚜렷한 이미지의 경우(극단적으로 이진영상처럼 엣지가 뚜렷히 나타나는 경우)
			//포커스가 맞지 않아서 영상이 흐릿해지면서 엣지가 두껍게 나타나고 그로 인해
			//오히려 포커스가 맞았을때보다 그 합이 더 커지는 경우가 발생한다.
			value = (double)value * 0.1;

			dst[y * w + x] = (value > 255 ? 255 : value);
		}
	}

	/*
	int i, j, index, index2;
	int dx, dy;

	for (i = 1; i < h - 1; i++)
	{
		index = i*w;

		for (j = 1; j < w - 1; j++)
		{
			index2 = index + j;
			// -1 0 1
			// -2 0 2
			// -1 0 1
			dx = src[index2 - w + 1] + (src[index2 + 1] << 1) + src[index2 + w + 1]
				- src[index2 - w - 1] - (src[index2 - 1] << 1) - src[index2 + w - 1];

			// -1 -2 -1
			//  0  0  0
			//  1  2  1
			dy = -src[index2 - w - 1] - (src[index2 - w] << 1) - src[index2 - w + 1]
				+ src[index2 + w - 1] + (src[index2 + w] << 1) + src[index2 + w + 1];

			//int mag = abs(dx) + abs(dy);     // magnitude
			int mag = sqrtf(dx*dx + dy*dy);

			dst[index2] = (mag > 255 ? 255 : mag);

			//dx_tbl[index2] = dx;
			//dy_tbl[index2] = dy;
			//mag_tbl[index2] = mag;              // store the magnitude in the table
		}   // for(j)

	}   // for(i)
	*/
}


void sobel(const unsigned char* inbuffer, unsigned char* outbuffer, int width, int height, int h, int v)
{
	int i, j, s32GradientH, s32GradientV;
	int s32GradientH_sum, s32GradientV_sum;

	for (i = 2; i < height - 2; i++)
	{
		for (j = 2; j < width - 2; j++)
		{
			s32GradientH = 0;
			s32GradientV = 0;
			s32GradientH_sum = 0;
			s32GradientV_sum = 0;

			//if(h == 1)
			{
				s32GradientH = s32GradientH + (inbuffer[(j - 1) + ((i - 1)* width)]);
				s32GradientH = s32GradientH + (inbuffer[(j)+((i - 1)* width)] << 1);
				s32GradientH = s32GradientH + (inbuffer[(j + 1) + ((i - 1)* width)]);
				s32GradientH = s32GradientH - (inbuffer[(j - 1) + ((i + 1)* width)]);
				s32GradientH = s32GradientH - (inbuffer[(j)+((i + 1)* width)] << 1);
				s32GradientH = s32GradientH - (inbuffer[(j + 1) + ((i + 1)* width)]);
				s32GradientH_sum += abs(s32GradientH);

				s32GradientH = s32GradientH - inbuffer[(j - 1) + ((i - 1)* width)];
				s32GradientH = s32GradientH - (inbuffer[(j)+((i - 1)* width)] << 1);
				s32GradientH = s32GradientH - inbuffer[(j + 1) + ((i - 1)* width)];
				s32GradientH = s32GradientH + inbuffer[(j - 1) + ((i + 1)* width)];
				s32GradientH = s32GradientH + (inbuffer[(j)+((i + 1)* width)] << 1);
				s32GradientH = s32GradientH + inbuffer[(j + 1) + ((i + 1)* width)];
				s32GradientH_sum += abs(s32GradientH);

				s32GradientH = s32GradientH_sum / 2;

				if (s32GradientH < 0) s32GradientH = 0;
				if (s32GradientH > 255) s32GradientH = 255;
			}

			//if(v == 1)
			{
				s32GradientV = s32GradientV + (inbuffer[(j - 1) + ((i - 1)* width)]);
				s32GradientV = s32GradientV + (inbuffer[(j - 1) + ((i)* width)] << 1);
				s32GradientV = s32GradientV + (inbuffer[(j - 1) + ((i + 1)* width)]);
				s32GradientV = s32GradientV - (inbuffer[(j + 1) + ((i - 1)* width)]);
				s32GradientV = s32GradientV - (inbuffer[(j + 1) + ((i)* width)] << 1);
				s32GradientV = s32GradientV - (inbuffer[(j + 1) + ((i + 1)* width)]);
				s32GradientV_sum += abs(s32GradientV);

				s32GradientV = s32GradientV - (inbuffer[(j - 1) + ((i - 1)* width)]);
				s32GradientV = s32GradientV - (inbuffer[(j - 1) + ((i)* width)] << 1);
				s32GradientV = s32GradientV - (inbuffer[(j - 1) + ((i + 1)* width)]);
				s32GradientV = s32GradientV + (inbuffer[(j + 1) + ((i - 1)* width)]);
				s32GradientV = s32GradientV + (inbuffer[(j + 1) + ((i)* width)] << 1);
				s32GradientV = s32GradientV + (inbuffer[(j + 1) + ((i + 1)* width)]);
				s32GradientV_sum += abs(s32GradientV);

				s32GradientV = s32GradientV_sum / 2;

				if (s32GradientV < 0) s32GradientV = 0;
				if (s32GradientV > 255) s32GradientV = 255;
			}


			if (h == 1 && v == 1)
			{
				if (s32GradientV > 0 || s32GradientH > 0)
				{
					outbuffer[i*width + j] = MAX(s32GradientV - s32GradientH, 0);
				}
			}
			else if (h == 1)
			{
				if (s32GradientH > 0)
				{
					outbuffer[i*width + j] = s32GradientH;
				}
			}
			else if (v == 1)
			{
				if (s32GradientV > 0)
				{
					outbuffer[i*width + j] = s32GradientV;
				}
			}

		}
	}
}

cv::Point	find_matchTemplate( Mat img, Mat templ, int match_method /*= CV_TM_SQDIFF*/ )
{
	Mat result( img.rows - templ.rows + 1, img.cols - templ.cols + 1, CV_32F );

	if ( img.channels() == 4 )
		cvtColor( img, img, CV_BGRA2GRAY );
	else if ( img.channels() == 3 )
		cvtColor( img, img, CV_BGR2GRAY );

	if ( templ.channels() == 4 )
		cvtColor( templ, templ, CV_BGRA2GRAY );
	else if ( templ.channels() == 3 )
		cvtColor( templ, templ, CV_BGR2GRAY );

	//m_src(cv::Rect( 242, 157, size, size )).copyTo( img1 );
	matchTemplate( img, templ, result, match_method );
	normalize( result, result, 0, 1, NORM_MINMAX, -1, Mat() );

	//SetWindowText( d2S( ((float *)(result.data))[0] ) );
	
	//result.convertTo( result, CV_8UC1, 255.0 );
	imshow( "img", img );
	imshow( "result", result );
	//bool bok = imwrite( "f:\\0.CV_TM_CCOEFF_NORMED.bmp", result );
	//imwrite( "f:\\template.bmp", img0 );
	
	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	cv::Point matchLoc;

	minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc );
	int countNonZeroValue = countNonZero( result );
	if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED )
	{
		matchLoc = minLoc;
	}
	else
	{
		matchLoc = maxLoc;
	}

	return cv::Point( matchLoc.x, matchLoc.y );
}

//img에서 templ_origin의 r 사각형 영역과 가장 유사한 사각형 영역을 리턴한다.
cv::Rect	find_matchTemplate( Mat img, Mat templ_origin, cv::Rect r, int match_method /*= CV_TM_SQDIFF*/ )
{
	cv::Point pt = find_matchTemplate( img, templ_origin(r), match_method );
	return cv::Rect( pt.x, pt.y, r.width, r.height );
}


CPoint		cvPoint2CPoint( cv::Point src )
{
	return CPoint( src.x, src.y );
}

cv::Point	CPoint2cvPoint( CPoint src )
{
	return cv::Point( src.x, src.y );
}

CRect		cvRect2CRect( cv::Rect src )
{
	return CRect( src.x, src.y, src.x + src.width, src.y + src.height );
}

cv::Rect	CRect2cvRect( CRect src )
{
	return cv::Rect( src.left, src.top, src.Width(), src.Height() );
}

bool		ptInRect( cv::Rect src, cv::Point pt )
{
	if ( pt.x >= src.x && pt.y >= src.y && pt.x <= (src.x + src.width) && pt.y <= (src.y + src.height) )
		return true;

	return false;
}

bool		ptInRect( cv::Rect src, CPoint pt )
{
	return ptInRect( src, cv::Point(pt.x, pt.y) );
}
