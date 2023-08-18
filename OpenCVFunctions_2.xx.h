//#pragma once

#ifndef _OpenCVFunctions_h_
#define _OpenCVFunctions_h_

#include <afxwin.h>
#include <math.h>
#include <vector>
#include <stack>
#include <atlimage.h>

#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
//#include "opencv2/features2d/features2d.hpp"
//#include "opencv2/objdetect/objdetect.hpp"
//#include "opencv2/photo/photo.hpp"

#pragma warning(disable:4819)

#ifdef _DEBUG
#define LIB_SUFFIX "d.lib"
#else
#define LIB_SUFFIX ".lib"
#endif

#ifndef CV_VERSION_EPOCH
#include "opencv2/videoio/videoio.hpp"
#define OPENCV_VERSION CVAUX_STR(CV_VERSION_MAJOR)"" CVAUX_STR(CV_VERSION_MINOR)"" CVAUX_STR(CV_VERSION_REVISION)
#pragma comment(lib, "opencv_world" OPENCV_VERSION LIB_SUFFIX)
#else
#define OPENCV_VERSION CVAUX_STR(CV_VERSION_EPOCH)"" CVAUX_STR(CV_VERSION_MAJOR)"" CVAUX_STR(CV_VERSION_MINOR)
#pragma comment(lib, "opencv_core" OPENCV_VERSION LIB_SUFFIX)   
#pragma comment(lib, "opencv_imgproc" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_highgui" OPENCV_VERSION LIB_SUFFIX) 
#pragma comment(lib, "opencv_video" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_features2d" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_objdetect" OPENCV_VERSION LIB_SUFFIX)
//#pragma comment(lib, "opencv_photo" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_calib3d" OPENCV_VERSION LIB_SUFFIX)
#endif

#ifndef uint8_t
typedef unsigned char      uint8_t;
#endif
#ifndef uint16_t
typedef unsigned short     uint16_t;
#endif
#ifndef uint32_t
typedef unsigned int       uint32_t;
#endif
#ifndef uint64_t
typedef unsigned long long uint64_t;
#endif


#define SCV_DT_TOP		0x00000000
#define SCV_DT_LEFT		0x00000000
#define SCV_DT_CENTER	0x00000001
#define SCV_DT_RIGHT	0x00000002
#define SCV_DT_VCENTER	0x00000004
#define SCV_DT_BOTTOM	0x00000008

//#define		PI	3.141592
#ifndef		COLOR_RANGE
#define		COLOR_RANGE(x)	( (x) < 0 ? (x=0) : ((x) > 255 ? (x=255) : (x)) )
#endif
#ifndef		CLAMP_RANGE
#define		CLAMP_RANGE(x,min,max) ( (x > max) ? (x=max) : (x < min) ? (x=min) : x )
#endif


#ifndef DIB_WIDTHBYTES
#define		WIDTHBYTES(bits)	( ((bits) + 31) / 32 * 4 )	//bits is not width, but (width * bitCount)
#endif
#define		MAKE4WIDTH(width)		( (width + 3) & ~3 )		//width�� 4�� ����� ������ش�.

enum YUV_FORMAT
{
	yuv422_uyvy = 0,
	yuv422_yuyv,
	yuv420_nv12,
	yuv420_yv12,
};

//Ŭ���� �Լ�
template<class T> void cvClamp( T &n, T min, T max )
{
	if ( n < min ) n = min;
	else if ( n > max ) n = max;
}

using namespace std;
using namespace cv;


int			matMake4Width( Mat &src );

IplImage*	scvCutRect( IplImage* pImage, CRect r );
IplImage*	scvCutRect( IplImage* pImage, int x, int y, int w, int h );
//
enum SCV_DRAW_IMAGE_MODE
{
	scv_draw_original_size = 0,	//���� ũ��� ǥ��
	scv_draw_stretch,			//rTarget�� ũ�⿡ ���� stretch
};

/*
scvDrawImage(pDC, src, 10, 20)
=> 10, 20�� �̹����� ���� ũ��� ǥ���Ѵ�.

scvDrawImage(pDC, src, target_rect, COLORREF crBack)
=> target_rect�� ������ Ȯ��, ����Ͽ� �̹����� ǥ���ϰ� ������ crBack���� ä���ش�.
*/

CRect		scvDrawImage(CDC* pDC, Mat src, int dx = 0, int dy = 0, int dw = 0, int dh = 0,
						 CRect *target_rect = NULL, COLORREF crBack = 0, bool fit = false);
//dest_rect �׷��� ���� ��ǥ
CRect		scvDrawImage(CDC* pDC, Mat src, CRect target_rect, COLORREF crBack = 0, bool fit = false);
//CRect		scvDrawImage(CDC* pDC, Mat src, int x, int y);

void		scvDisplayWindow( const TCHAR* name, IplImage* pImage, int x = 0, int y = 0, int w = 0, int h = 0 );

void		scvDisplayHistogram(TCHAR* name, IplImage* pImage, int x = 0, int y = 0, int w = 0, int h = 0 );

BITMAPINFO	scvGetBitmapInfo( IplImage* pImage );
void		scvPutText( IplImage* pImage, CString sText, int x, int y, double dFontSize, COLORREF cText, int lineWidth = 1 );
IplImage*	scvResize( IplImage* pImage, int w, int h );
double		scvGetSharpness( IplImage* img_src, cv::Rect* roi = NULL );
double		getSharpness(Mat mat);
double		getSharpness(uint8_t *data, int w, int h);	//data = 1 ch image raw data
double		scvGetBrightness( IplImage* pSrc, cv::Rect* roi = NULL );
short		GetSharpness(TCHAR* data, unsigned int width, unsigned int height);
void		scvDottedLine( Mat& src, cv::Point p1, cv::Point p2 );
COLORREF	scvGetMajorColor( IplImage* pImage, cv::Rect* roi = NULL );
IplImage*	scvBlendImage( IplImage* pSrc1, IplImage* pSrc2, cv::Rect* roi = NULL, IplImage* pMask = NULL );
//IplImage*	scvBlendImage2( IplImage* pSrc1, IplImage* pSrc2, int dx, int dy, int w, int h, double dAlpha );
IplImage*	scvBlendImage2( IplImage* pSrc1, IplImage* pSrc2, int dx, int dy, int w, int h, double dAlpha );
Mat			scvBrightContrast( Mat src, int nBright, double dContrast );
void		scvSharpening( Mat& src, float sigma, float weight_src = 1.5, float weight_tmp = -0.5 );

cv::Scalar	fromRGB( COLORREF rgb );

cv::Scalar	cvGetDefaultColor( int idx );
cv::Scalar	cvGetDefaultColor( int idx, int alpha );

void		reduceColor( Mat&image, int div );
IplImage*	reduceTo64Colors( IplImage* img );
IplImage*	scvReduceColor( IplImage* pSrc, int nColor );
IplImage*	bhQuantizeImage(IplImage* srcImage, int colorCount);
Mat			bhQuantizeImage(Mat src, int colorCount);
IplImage*	scvQuantizeImage(IplImage* srcImage, int colorCount);

COLORREF	scvGetMajorColor( IplImage* img, int nColorUsed, COLORREF* crEachColor = NULL, int* nCountEachColor = NULL );
COLORREF	scvGetMajorColor( Mat src, int nColorUsed, COLORREF* crEachColor = NULL, int* nCountEachColor = NULL );
//IplImage*	scvMake4BytesWidth( IplImage* src );

//���� �̹��� ���ϵ鸸 �д´�.
Mat			loadMat(LPCTSTR sfile, int flags = CV_LOAD_IMAGE_UNCHANGED, bool bDisplayError = false);

//raw, yuv�� �����Ͽ� �д´�.
Mat			loadImage( LPCTSTR sfile, int flags = CV_LOAD_IMAGE_UNCHANGED, bool bDisplayError = true, int width = 0, int height = 0, int yuv_format = yuv422_uyvy );

//4ä���� ���� foreground �̹����� ��濡 �ռ��Ѵ�.
void		overlayImage(const cv::Mat &background, const cv::Mat &foreground, cv::Mat &output, cv::Point location = cv::Point(0,0), double dAlpha = 1.0 );

//3ä���� foreground �̹����� 1ä���� ����ũ ���� ������ �̿��Ͽ� ��濡 �ռ��Ѵ�.
void		overlayImage( cv::Mat &background, cv::Mat &foreground, cv::Mat &mask, cv::Mat &output, cv::Point2i location, double dAlpha = 1.0 );

//Ư�� ������ �����ϰ� ��� ���Ͽ� �������� �ռ��Ѵ�.
void		overlayImage( cv::Mat &back, cv::Mat fore, cv::Scalar crExcept, cv::Point2i location, double dAlpha = 1.0, double dSimility = 0 );

//Ư�� ������ �����ϰ� ����ũ ������ ���� ������ �����ϰ� ��� ���Ͽ� �������� �ռ��Ѵ�.
void		overlayImage( cv::Mat &back, cv::Mat &fore, cv::Mat &mask, cv::Scalar crExcept, cv::Point2i location, double dSimility );

//���� ũ���̹����� fore�� bitwise�� ����ؼ� ������ �ռ��Ѵ�.
cv::Mat		overlay_bitwise(cv::Mat background, cv::Mat fore);

double		GetColorSimilityDistance( CvScalar c1, CvScalar c2 );
double		GetColorSimilityDistance( COLORREF c1, COLORREF c2 );


//����ũ����  ��� �󵵿� ���� src �̹����� ǥ�õǰ� ������ �κ��� white�� ä������.(����ũ�� ���:����, ȸ��:������, ����:bWhite�� true�̸� �������, false�̸� �������� ä��)
void		overlayMaskImage( Mat &src, Mat mask, bool bWhite );
void		overlayMaskImage(Mat src, Mat mask, Mat &output, cv::Scalar cr, cv::Point2i location, double dAlpha);
void		overlayMaskImage(Mat src, Mat mask, Mat &output, cv::Scalar cr, double dAlpha);

//4ä�� png ������ ���� ä�ΰ��� ������ ä gray �̹����� ��ȯ��Ų��.
void		grayWithoutAlpha( cv::Mat &img );

//Ư�� ����(crExcept)�� �ȼ��� ������ �ٸ� �ȼ��� ����(crValue)�� �����Ѵ�.
void		adjustColor( cv::Mat &img, cv::Scalar crValue, cv::Scalar crExcept, cv::Scalar crTolerance = cvScalar( 0, 0, 0, 0 ) );

//shift(move)
Mat			translateImg(Mat &img, int offsetx, int offsety);

//rotate, warp ����
//1=CW, 2=CCW, 3=180
void		rotate90(cv::Mat &matImage, int rotflag );
//90���� ȸ��
//void		rotate(uint8_t *src, uint32_t src_width, uint32_t src_height, uint8_t *dst, int rotflag);
//opencv�� t(). -90�� ȸ�� �� ���� flip.
void		transpose(uint8_t *src, uint32_t src_width, uint32_t src_height, uint8_t *dst);


//src�� angle��ŭ ȸ������ rotated�� �����ϰ� mask �̹����� �������ش�.
Mat			rotateMat( Mat src, Mat* rotated, double angle );
//fore�� angle��ŭ ȸ����Ų �̹����� back�� cx, cy�� center �ռ��Ѵ�.
//back�� NULL�� ���� fore�� ȸ���� �̹����� ���� 8bit mask�� �����ϹǷ� �� ������� ������ Ȱ���� �� �ִ�.
Mat			rotateOverlay( Mat* back, Mat* fore, double angle, int cx, int cy, double alpha = 1.0 );

//src�̹����� ptSrc��ǥ�� ptDst�� warping ��Ų��. mask �̹����� �����Ѵ�.
//pt�� lt���� �ð���� ���ʷ� �ش�.
Mat			warpMat( Mat *src, int ptSrc[8], int ptDst[8] );
//back�� fore �̹����� ptDst ��ǥ�� warp �� �ռ������ش�. mask �̹����� �����Ѵ�.
//pt�� lt���� �ð���� ���ʷ� �ش�.
Mat			warpOverlay( Mat *back, Mat *fore, int ptDst[8] );

double		getDistance( cv::Point a, cv::Point b );

// Helper function that computes the longest distance from the edge to the center point.
double		getMaxDisFromCorners(const cv::Size& imgSize, const cv::Point& center);

// Helper function that creates a gradient image.   
// firstPt, radius and power, are variables that control the artistic effect of the filter.
void		generateGradient(cv::Mat& mask);


Mat			GetHistogram( Mat src );

//filter ȿ��
Mat			sketch( Mat src, int blurSize = 51 );
Mat			sketch_Sobel( Mat src, bool bPencel ); //bPencel = false -> charcoal (more dark and bold)
Mat			waterColor( Mat &src );
//void		InvertColor( Mat& src, Mat& dst );	=> use cv::bitwise_not


//p->q�� ȭ��ǥ�� �׸��µ� �� ������ bOnlyForCheck�� true�̸�
//ȭ��ǥ�� �׸��� ������ ��ȿ�� ������ ������ ȭ��ǥ������ �Ǻ��Ѵ�.
//return value : 
int			drawArrowLine(  Mat& img, cv::Point2f p, cv::Point2f q, cv::Scalar line_color,
							double scale = 1.0, int line_thickness = 1, int line_type = 8,
							int shift = 0, double tipLength = 0.1, double minLength = 0.0, double maxLength = 0.0, bool bDrawArrowLine = true );


//pt�� NULL�� �ָ� �̹��� ��ü ������ �߽����� align�� ����ǰ�
//Ư�� ��ǥ�� �����ָ� �ش� ��ǥ�� �������� align�� �����.
//cv::Rect	drawText( Mat& img, std::string sText, cv::Point pt = cv::Point(-1, -1), int nFormat = SCV_DT_LEFT | SCV_DT_TOP, int nMargin = 10,
//					  int nFontFace = FONT_HERSHEY_DUPLEX, double fontScale = 1.0, cv::Scalar color = cv::Scalar(0,0,0),
//					  int thickness = 1, int lineType = 8);
cv::Rect	drawText(	cv::Mat& img,
						const std::string &text,
						cv::Point org = cv::Point(-1, -1),
						int format = SCV_DT_LEFT | SCV_DT_TOP,
						int margin = 10,
						int fontFace = FONT_HERSHEY_DUPLEX,
						double fontScale = 1.0,
						cv::Scalar color = cv::Scalar(0,0,0),
						int thickness = 1,
						int lineType = 8,
						bool bottomLeftOrigin = false );

//�Է� ������ �޾� ������ ũ���� ����
//w x h ������ �ش� ���� ��հ����� ä�� 
//������ �����Ѵ�.
void		matMosaic( Mat src, Mat& dst, int w, int h );

//�Է� ������ �޾� w x h ũ�Ⱑ
//1 �ȼ��� �ǵ��� �پ�� ������ �����Ѵ�.
void		matSamplingResize( Mat src, Mat& dst, int w, int g ); // rowSize = ����, colSize = �ʺ�

//
Mat			data2Mat( uint8_t *data, int w, int h, int ch );


//�� �簢���� ��ġ�� ������ �����Ѵ�.
cv::Rect	getIntersectionRect( cv::Rect r1, cv::Rect r2 );

//rTarget�� ���ϴ� dRatio�� �ִ� �簢���� ���Ѵ�.
cv::Rect	getRatioRect(cv::Rect rTarget, double dRatio);

//�� �簢�� ��ġ�� ������ r1 �������� ����ؼ� �����Ѵ�.
double		getOverlappedRatio( cv::Rect r1, cv::Rect r2 );
bool		isRectEmpty( cv::Rect r );
bool		isRectNull( cv::Rect r );
CString		getRectInfo( cv::Rect r );

cv::Rect	inflateRect( cv::Rect r, int offsetX, int offsetY );
cv::Rect	inflateRect( int x, int y, int w, int h, int offsetX, int offsetY );

void		adjustRectRange( cv::Rect &rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize = true );

void		showMat( CString name, Mat mat, double dZoom = 1.0, int x = -1, int y = -1 );
void		showRawImage( CString name, uint8_t *data, int width, int height, int ch = 1, double dZoom = 1.0, int x = -1, int y = -1 );
void		saveRawImage( CString sfile, uint8_t *data, int width, int height, int ch );
void		save2Raw( cv::Mat mat, CString sRawFilename, int dst_ch = 1 );

HBITMAP		ConvertMatToBMP (cv::Mat frame);
bool		PasteBMPToClipboard(void* bmp);
Mat			HBITMAP2Mat( HWND hwnd, HBITMAP hbwindow );
cv::Mat		HBITMAP2Mat(HBITMAP hBitmap);

void		FillGridPattern( Mat &src, int size = 16 );

//������ġ�� ũ�� ����
void		printMat( Mat img, CString sPrinterName, CString sDocName, int x, int dy, int dw, int dh, int copies = 1 );

//������ ���� ���� ũ���� �μ�. �μ��ϱ� ���� �̹����� ���� ũ�� ������ �°� �߶���� �Ѵ�.
//marginX, marginY�� �ȼ� ������ �ƴ� mm ������.
CSize		getCutSizeToFitPrinterPaper( Mat img, CString sPrinterName, int marginX = 0, int marginY = 0 );
CSize		getCutSizeToFitPrinterPaper( int w, int h, CString sPrinterName, int marginX = 0, int marginY = 0 );
void		printMat( Mat &img, CString sPrinterName, CString sDocName, int marginX = 0, int marginY = 0, int copies = 1 );



void		CannyEdge(uint8_t *src, uint8_t *dst, int width, int height, int th_low, int th_high );

bool		matIsEqual(const cv::Mat Mat1, const cv::Mat Mat2);

CRect		getCRect(cv::Rect r);
cv::Point	CenterPoint(cv::Rect r);

void		sobel(const unsigned char* src, unsigned char* dst, int width, int height);
void		sobel(const unsigned char* src, unsigned char* dst, int width, int height, int h, int v);

//img���� templ �̹����� ���� ������ ��ǥ�� �����Ѵ�.
cv::Point	find_matchTemplate( Mat img, Mat templ, int match_method = CV_TM_SQDIFF );
//img���� templ_origin�� r �簢�� ������ ���� ������ �簢�� ������ �����Ѵ�.
cv::Rect	find_matchTemplate( Mat img, Mat templ_origin, cv::Rect r, int match_method = CV_TM_SQDIFF );

CPoint		cvPoint2CPoint( cv::Point src );
cv::Point	CPoint2cvPoint( CPoint src );
CRect		cvRect2CRect( cv::Rect src );
cv::Rect	CRect2cvRect( CRect src );
bool		ptInRect( cv::Rect src, cv::Point pt );
bool		ptInRect( cv::Rect src, CPoint pt );

//ġȯ �Լ�
//#ifndef SWAP
//template<class T> void SWAP( T& x, T& y )
//{
//	T temp	= x;
//	x		= y;
//	y		= temp;
//}
//#endif

class HarrisDetector {

private:
	// 32-bit float image of corner strength
	cv::Mat cornerStrength;

	// 32-bit float image of threshold corners
	cv::Mat cornerTh;

	// image of local maxima (internal)
	cv::Mat localMax;

	// size of neighborhood for derivatives smoothing
	int neighborhood;

	// aperture for gradient computation
	int aperture;

	// Harris parameter
	double k;

	// maximum strength for threshold computation
	double maxStrength;

	// calculated threshold (internal)
	double threshold;

	// size of neighborhood for non-max supression
	int nonMaxSize;

	// kernel for non-max supression
	cv::Mat kernel;

public:

	HarrisDetector() :	neighborhood(3), aperture(3), 
						k(0.01), maxStrength(0.0),
						threshold(0.01), nonMaxSize(3)
	{
		// create kernel used in non-max supression
		setLocalMaxWindowSize(nonMaxSize);
	}

	void setLocalMaxWindowSize(int nonMaxSize)
	{
		this->nonMaxSize = nonMaxSize;
	}

	// Compute Harris corners
	void detect(const cv::Mat &image)
	{
		// Harris computation
		cv::cornerHarris(image, cornerStrength,
						neighborhood,   // neighborhood size
						aperture,       // aperture size
						k               // Harris parameter
						);

		// internal threshold computation
		double minStrength; // not used

		cv::minMaxLoc(cornerStrength, &minStrength, &maxStrength);

		// local maxima detection
		cv::Mat dilated;    //temporary image
		cv::dilate(cornerStrength, dilated, cv::Mat());
		cv::compare(cornerStrength, dilated, localMax, cv::CMP_EQ);
	}

	// Get the corner map from the comuted Harris values
	cv::Mat getCornerMap(double qualityLevel)
	{
		cv::Mat cornerMap;

		// thresholding the corner strength
		threshold = qualityLevel * maxStrength;

		cv::threshold(cornerStrength, cornerTh, threshold, 255, cv::THRESH_BINARY);

		// convert to 8-bit image
		cornerTh.convertTo(cornerMap, CV_8U);

		// non-maxima suppression
		cv::bitwise_and(cornerMap, localMax, cornerMap);

		return cornerMap;
	}

	// Get the feature points from the computed Harris value
	void getCorners(std::vector<cv::Point> &points, double qualityLevel)
	{
		// Get the corner map
		cv::Mat cornerMap = getCornerMap(qualityLevel);

		// Get the corners
		getCorners(points, cornerMap);
	}

	// Get the features points from the computed corner map
	void getCorners(std::vector<cv::Point> &points, const cv::Mat &cornerMap)
	{
		// Iterate over the pixels to obtain all features
		for (int y = 0; y < cornerMap.rows; y++)
		{
			const uchar *rowPtr = cornerMap.ptr<uchar>(y);

			for (int x = 0; x < cornerMap.cols; x++)
			{
				// if it is a feature point
				if (rowPtr[x])
				{
					points.push_back(cv::Point(x, y));
				}
			}
		}
	}

	// Draw circles at feature point locations on an image
	void drawOnImage(cv::Mat &image, const std::vector<cv::Point> &points,
		cv::Scalar color = cv::Scalar(255, 255, 255),
		int radius = 3, int thickness = 1)
	{
		std::vector<cv::Point>::const_iterator it = points.begin();

		// for all corners
		while (it != points.end())
		{
			// draw a circle at each corner location
			cv::circle(image, *it, radius, color, thickness);
			++ it;
		}
	}
};


// TypedMat : written  by darkpgmr (http://darkpgmr.tistory.com/36), 2013
// TypedMat<unsigned char> tm = image;    // ������ 1
// image�� 1ä�� grayscale �̹����� ���
//tm[y][x] = 100;    // (x,y)�� �ȼ����� 100���� ����
// image�� 3ä�� color �̹����� ���
//tm(y,x,0) = 100;    // (x,y)�� �ȼ��� blue���� 100���� ����
template<class T> class TypedMat
{
	T** m_pData;
	int m_nChannels;
	int m_nRows, m_nCols;

public:
	TypedMat():m_pData(NULL),m_nChannels(1),m_nRows(0),m_nCols(0){}
	~TypedMat(){if(m_pData) delete [] m_pData;}

	// OpenCV Mat ���� (�޸� ����)
	void Attach(const cv::Mat& m);
	void Attach(const IplImage& m);
	TypedMat(const cv::Mat& m):m_pData(NULL),m_nChannels(1),m_nRows(0),m_nCols(0) { Attach(m);}
	TypedMat(const IplImage& m):m_pData(NULL),m_nChannels(1),m_nRows(0),m_nCols(0) { Attach(m);}
	const TypedMat & operator =(const cv::Mat& m){ Attach(m); return *this;}
	const TypedMat & operator =(const IplImage& m){ Attach(m); return *this;}

	// ��(row) ��ȯ
	T* GetPtr(int r)
	{ assert(r>=0 && r<m_nRows); return m_pData[r];}

	// ������ ��ø (��������) -- 2D
	T * operator [](int r)
	{ assert(r>=0 && r<m_nRows); return m_pData[r];}

	const T * operator [](int r) const
	{ assert(r>=0 && r<m_nRows); return m_pData[r];}

	// ������ ��ø (��������) -- 3D
	T & operator ()(int r, int c, int k)
	{ assert(r>=0 && r<m_nRows && c>=0 && c<m_nCols && k>=0 && k<m_nChannels); return m_pData[r][c*m_nChannels+k];}

	const T operator ()(int r, int c, int k) const
	{ assert(r>=0 && r<m_nRows && c>=0 && c<m_nCols && k>=0 && k<m_nChannels); return m_pData[r][c*m_nChannels+k];}
};

template<class T> void TypedMat<T>::Attach(const cv::Mat& m)
{
	assert(sizeof(T)==m.elemSize1());
	m_nChannels = m.channels();
	m_nRows = m.rows;
	m_nCols = m.cols;

	if(m_pData) delete [] m_pData;

	m_pData = new T * [m_nRows];

	for(int r=0; r<m_nRows; r++)
	{
		m_pData[r] = (T *)(m.data + r*m.step);
	}
}

template<class T> void TypedMat<T>::Attach(const IplImage& m)
{
	assert(sizeof(T) == m.widthStep/(m.width*m.nChannels));

	m_nChannels = m.nChannels;
	m_nRows = m.height;
	m_nCols = m.width;

	if(m_pData) delete [] m_pData;

	m_pData = new T * [m_nRows];

	for(int r=0; r<m_nRows; r++)
	{
		m_pData[r] = (T *)(m.imageData + r*m.widthStep);
	}
}

void reduceColorPalette(const Mat& inputImage, Mat& outputImage, const uchar intervalSize = 4);
void equalizeIntensity(const Mat& inputImage, Mat& outputImage);
void computeGradients(const Mat& inputImage, Mat& outputImage);
void processImage(const Mat& inputImage, Mat& outputImage);
void Sepia( Mat& mat, int magnitude = 75 );
Mat	 milky_white( Mat src, int blurSize, int level );

//mat�̹����� w x h ũ��� ä���.
void resizeGrid( Mat& mat, int width, int height );

void erode_filter( uint8_t* src, uint8_t* dst, int width, int height, int nX, int nY );
void erode_filter( uint8_t* src, int width, int height, int nX, int nY );
bool dilate_filter( uint8_t* src, uint8_t* dst, int width, int height, int nX, int nY );

//Ÿ�� ������ ������ ũ�⿡ �°� src������ �ڸ��� resize�Ѵ�.
Mat getFittingMat( Mat src, int tWidth, int tHeight );

void pixelLabeling( uint8_t* src, int width, int height, std::vector<std::vector<cv::Point>> &contours, int win_size_x = 3, int win_size_y = 3 );
Mat getLabelledColorImage( uint8_t* src, int width, int height );

#endif

