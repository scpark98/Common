//#pragma once

#ifndef _OpenCVFunctions_h_
#define _OpenCVFunctions_h_

/*
MFC의 어떤 클래스를 상속받아 만든 다른 cpp, h파일과는 달리
Common 폴더에 존재하는 일반 cpp, h파일을 만들었다면
MFC의 클래스 등을 사용하기 위해서는 Afxwin.h 등 몇 개의 h파일들을 include 시켜줘야 한다.
예전에는 그냥 cpp 파일에 #include "stdafx.h"라고만 해도 h, cpp파일에서
MFC 클래스들을 사용할 수 있었으나 Common 폴더에서 프로젝트 폴더에 있는 저 파일을 인식할 리 없다.
프로젝트 세팅에서 "Precompiled Header"가 "Use"로 되어 있어서 동작했다고 생각되지만
IntelliSense에 의해 #include "stdafx.h" 문구에 빨간색 에러 표시가 뜨고
다른 코드들에도 빨간색 에러로 표시되면서 툴팁 정보가 표시되지 않는 불편함이 생겼다.
이를 정석적으로 해결하는 방법은 MFC 클래스 및 확장 클래스들을 사용하기 위해서는
h파일에 아래와 같이 필요한 h파일들을 include하고
cpp파일은 프로젝트 세팅에서 "Precompiled Header"를 사용안함으로 설정하는 것이라고 생각된다.
[MFC 헤더와 라이브러리 설명 참조]
http://www.devpia.com/MAEUL/Contents/Detail.aspx?BoardID=51&MAEULNo=20&no=567
*/

/*
cv::resize interpolation method
확대 : AREA, NEAREST는 원본과 유사하고 LINEAR는 부드러운 엣지, CUBIC과 LANZCOS4는 선명한 엣지를 만듬.
축소 : AREA는 Anti-Aliasing 처리되고 나머지는 거친 엣지를 만듬.
*/

#include <Afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
#include <Afxcmn.h>

#include <math.h>
#include <vector>
#include <stack>
#include <atlimage.h>

#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
//#include "opencv2/video/tracking.hpp"
//#include "opencv2/features2d/features2d.hpp"
//#include "opencv2/objdetect/objdetect.hpp"
//#include "opencv2/photo/photo.hpp"

#pragma warning(disable:4819)

#ifdef _DEBUG
#define LIB_SUFFIX "d.lib"
#else
#define LIB_SUFFIX ".lib"
#endif

#ifdef CV_WORLD
#include "opencv2/videoio/videoio.hpp"
#define OPENCV_VERSION CVAUX_STR(CV_VERSION_MAJOR)"" CVAUX_STR(CV_VERSION_MINOR)"" CVAUX_STR(CV_VERSION_REVISION)
#pragma comment(lib, "opencv_world" OPENCV_VERSION LIB_SUFFIX)
#else
#define OPENCV_VERSION CVAUX_STR(CV_VERSION_MAJOR)"" CVAUX_STR(CV_VERSION_MINOR)"" CVAUX_STR(CV_VERSION_REVISION)
#pragma comment(lib, "opencv_core" OPENCV_VERSION LIB_SUFFIX)   
#pragma comment(lib, "opencv_imgproc" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_highgui" OPENCV_VERSION LIB_SUFFIX) 
#pragma comment(lib, "opencv_video" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_videoio" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_imgcodecs" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_features2d" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_objdetect" OPENCV_VERSION LIB_SUFFIX)
#pragma comment(lib, "opencv_flann" OPENCV_VERSION LIB_SUFFIX)
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
#define		MAKE4WIDTH(width)		( (width + 3) & ~3 )		//width를 4의 배수로 만들어준다.

enum YUV_FORMAT
{
	yuv422_uyvy = 0,
	yuv422_yuyv,
	yuv420_nv12,
	yuv420_yv12,
};

//클리핑 함수
template<class T> void cvClamp( T &n, T min, T max )
{
	if ( n < min ) n = min;
	else if ( n > max ) n = max;
}

using namespace std;
using namespace cv;


int			matMake4Width( Mat &src );

/*
scvDrawImage(pDC, src, 10, 20)
=> 10, 20에 이미지를 원본 크기로 표시한다.

scvDrawImage(pDC, src, target_rect, COLORREF crBack)
=> target_rect에 비율을 확대, 축소하여 이미지를 표시하고 여백은 crBack으로 채워준다.
*/

//이 함수가 실제 그리기 함수이므로 다른 오버로드 함수는 이 함수의 파라미터에 맞게 채워서 호출해야 한다.
//또한 src가 call by reference로 선언된 이유는 다음과 같다.
//4의 배수가 아닌 이미지를 그릴 경우 이 함수의 바디에서 matMake4Width 함수를 호출하게 되는데
//여기서 꽤 시간이 많이 소요된다. 만약 이미지를 그릴때마다 매번 불려지면 딜레이가 엄청 발생하므로
//처음 그릴때 4의 배수로 맞춰준 이미지를 메모리상에 유지하면 확대, 축소, 스크롤 등에 의해
//다시 그려질때는 매우 빠르게 그려진다.
//단, src의 cols가 달라지므로 cols를 UI상에 표시해야 하는 경우 이 값을 로딩시에 기억하고 있다가 표시해줘야 한다.
CRect		cv_draw(CDC* pDC, Mat &src, int dx = 0, int dy = 0, int dw = 0, int dh = 0,
						 CRect *target_rect = NULL, COLORREF crBack = 0, double zoom = 1.0);

CRect		cv_draw(CDC* pDC, Mat &src, CRect *target_rect, COLORREF crBack = 0, double zoom = 1.0);
//zoom_rect 크기로 늘리거나 줄인 후 target_rect에 그린다. 이때는 fit이 의미가 없다.
CRect		cv_draw(CDC* pDC, Mat &src, CRect zoom_rect, CRect *target_rect, COLORREF crBack = 0, double zoom = 1.0);
//src_rect도 있으면 좋지만 이 함수가 많아지게 된다. 만약 소스의 부분 이미지만 뿌리는 거라면 src에 이미지의 일부분만 넣어주자.

double		getSharpness(Mat mat);
double		getSharpness(uint8_t *data, int w, int h);	//data = 1 ch image raw data
short		GetSharpness(TCHAR* data, unsigned int width, unsigned int height);
void		scvDottedLine( Mat& src, cv::Point p1, cv::Point p2 );
Mat			scvBrightContrast( Mat src, int nBright, double dContrast );
void		scvSharpening( Mat& src, float sigma, float weight_src = 1.5, float weight_tmp = -0.5 );

cv::Scalar	fromRGB( COLORREF rgb );

cv::Scalar	cvGetDefaultColor( int idx );
cv::Scalar	cvGetDefaultColor( int idx, int alpha );

void		reduceColor( Mat&image, int div );

COLORREF	scvGetMajorColor( Mat src, int nColorUsed, COLORREF* crEachColor = NULL, int* nCountEachColor = NULL );

//범용 이미지 파일들만 읽는다.
Mat			loadMat(LPCTSTR sfile, int flags = IMREAD_UNCHANGED, bool bDisplayError = false);

//raw, yuv도 구분하여 읽는다.
Mat			loadImage( LPCTSTR sfile, int flags = IMREAD_UNCHANGED, bool bDisplayError = true, int width = 0, int height = 0, int yuv_format = yuv422_uyvy );

//4채널의 투명 foreground 이미지를 배경에 합성한다.
void		overlayImage(const cv::Mat &background, const cv::Mat &foreground, cv::Mat &output, cv::Point location = cv::Point(0,0), double dAlpha = 1.0 );

//3채널의 foreground 이미지와 1채널의 마스크 파일 정보를 이용하여 배경에 합성한다.
void		overlayImage( cv::Mat &background, cv::Mat &foreground, cv::Mat &mask, cv::Mat &output, cv::Point2i location, double dAlpha = 1.0 );

//특정 색상을 제외하고 배경 파일에 오버레이 합성한다.
void		overlayImage( cv::Mat &back, cv::Mat fore, cv::Scalar crExcept, cv::Point2i location, double dAlpha = 1.0, double dSimility = 0 );

//특정 색상을 제외하고 마스크 영역의 검은 영역만 제외하고 배경 파일에 오버레이 합성한다.
void		overlayImage( cv::Mat &back, cv::Mat &fore, cv::Mat &mask, cv::Scalar crExcept, cv::Point2i location, double dSimility );

//동일 크기이미지의 fore를 bitwise를 사용해서 간단히 합성한다.
cv::Mat		overlay_bitwise(cv::Mat background, cv::Mat fore);

double		GetColorSimilityDistance( cv::Scalar c1, cv::Scalar c2 );
double		GetColorSimilityDistance( COLORREF c1, COLORREF c2 );


//마스크에서  흰색 농도에 따라 src 이미지가 표시되고 검정인 부분은 white로 채워진다.(마스크의 흰색:원본, 회색:반투명, 검정:bWhite가 true이면 흰색으로, false이면 검정으로 채움)
void		overlayMaskImage( Mat &src, Mat mask, bool bWhite );
void		overlayMaskImage(Mat src, Mat mask, Mat &output, cv::Scalar cr, cv::Point2i location, double dAlpha);
void		overlayMaskImage(Mat src, Mat mask, Mat &output, cv::Scalar cr, double dAlpha);

//4채널 png 파일의 알파 채널값을 유지한 채 gray 이미지로 변환시킨다.
void		grayWithoutAlpha( cv::Mat &img );

//특정 색상(crExcept)의 픽셀을 제외한 다른 픽셀의 색상(crValue)을 조정한다.
void		adjustColor( cv::Mat &img, cv::Scalar crValue, cv::Scalar crExcept, cv::Scalar crTolerance = cv::Scalar( 0, 0, 0, 0 ) );

//shift(move)
Mat			translateImg(Mat &img, int offsetx, int offsety);

//rotate, warp 관련
//1=CW, 2=CCW, 3=180
void		rotate90(cv::Mat &matImage, int rotflag );
//90도씩 회전
//void		rotate(uint8_t *src, uint32_t src_width, uint32_t src_height, uint8_t *dst, int rotflag);
//opencv의 t(). -90도 회전 후 상하 flip.
void		transpose(uint8_t *src, uint32_t src_width, uint32_t src_height, uint8_t *dst);


//src를 angle만큼 회전시켜 rotated에 저장하고 mask 이미지를 리턴해준다.
Mat			rotateMat( Mat src, Mat* rotated, double angle );
//fore를 angle만큼 회전시킨 이미지를 back의 cx, cy에 center 합성한다.
//back이 NULL인 경우는 fore는 회전된 이미지가 들어가고 8bit mask를 리턴하므로 그 결과값을 별도로 활용할 수 있다.
Mat			rotateOverlay( Mat* back, Mat* fore, double angle, int cx, int cy, double alpha = 1.0 );

//src이미지의 ptSrc좌표를 ptDst로 warping 시킨다. mask 이미지를 리턴한다.
//pt는 lt부터 시계방향 차례로 준다.
Mat			warpMat( Mat *src, int ptSrc[8], int ptDst[8] );
//back에 fore 이미지를 ptDst 좌표에 warp 및 합성시켜준다. mask 이미지를 리턴한다.
//pt는 lt부터 시계방향 차례로 준다.
Mat			warpOverlay( Mat *back, Mat *fore, int ptDst[8] );

double		getDistance( cv::Point a, cv::Point b );

// Helper function that computes the longest distance from the edge to the center point.
double		getMaxDisFromCorners(const cv::Size& imgSize, const cv::Point& center);

// Helper function that creates a gradient image.   
// firstPt, radius and power, are variables that control the artistic effect of the filter.
void		generateGradient(cv::Mat& mask);


Mat			GetHistogram( Mat src );

//filter 효과
Mat			sketch( Mat src, int blurSize = 51 );
Mat			sketch_Sobel( Mat src, bool bPencel ); //bPencel = false -> charcoal (more dark and bold)
Mat			waterColor( Mat &src );
//void		InvertColor( Mat& src, Mat& dst );	=> use cv::bitwise_not


//p->q로 화살표를 그리는데 맨 마지막 bOnlyForCheck가 true이면
//화살표는 그리지 않지만 유효한 범위와 방향의 화살표인지만 판별한다.
//return value : 
int			drawArrowLine(  Mat& img, cv::Point2f p, cv::Point2f q, cv::Scalar line_color,
							double scale = 1.0, int line_thickness = 1, int line_type = 8,
							int shift = 0, double tipLength = 0.1, double minLength = 0.0, double maxLength = 0.0, bool bDrawArrowLine = true );


//pt를 NULL로 주면 이미지 전체 영역을 중심으로 align이 적용되고
//특정 좌표를 정해주면 해당 좌표를 기준으로 align이 적용됨.
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

//입력 영상을 받아 동일한 크기의 영상에
//w x h 영역을 해당 셀의 평균값으로 채운 
//영상을 리턴한다.
void		matMosaic( Mat src, Mat& dst, int w, int h );

//입력 영상을 받아 w x h 크기가
//1 픽셀이 되도록 줄어든 영상을 리턴한다.
void		matSamplingResize( Mat src, Mat& dst, int w, int g ); // rowSize = 높이, colSize = 너비

//
Mat			data2Mat( uint8_t *data, int w, int h, int ch );


//사각형 관련
	//두 사각형의 겹치는 영역을 리턴한다.
	cv::Rect	getIntersectionRect( cv::Rect r1, cv::Rect r2 );

	//rTarget에 접하는 dRatio인 최대 사각형을 구한다.
	cv::Rect	getRatioRect(cv::Rect rTarget, double dRatio);

	//두 사각형 겹치는 정도를 r1 기준으로 계산해서 리턴한다.
	double		getOverlappedRatio( cv::Rect r1, cv::Rect r2 );
	bool		isRectEmpty( cv::Rect r );
	bool		isRectNull( cv::Rect r );
	CString		getRectInfo( cv::Rect r );

	cv::Rect	inflateRect( cv::Rect r, int offsetX, int offsetY );
	cv::Rect	inflateRect( int x, int y, int w, int h, int offsetX, int offsetY );

	void		cvAdjustRectRange(cv::Rect &rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize = true);
	void		cvRectSetNull(cv::Rect &r);
	bool		cvRectIsNull(cv::Rect r);

//imshow 관련
	void		showMat( CString name, Mat mat, double dZoom = 1.0, int x = -1, int y = -1 );
	void		showRawImage( CString name, uint8_t *data, int width, int height, int ch = 1, double dZoom = 1.0, int x = -1, int y = -1 );

//저장 관련
	//jpg : 0(low quality) ~ 100(best quality)
	//png : 0(no compression, fast) ~ 100(smallest, slow)
	//imwrite의 quality옵션은 위와 같지만 혼동될 수 있으므로
	//save_mat함수의 quality는 100이면 best, no compresstion이고 0이면 low quality로 저장한다.
	bool		save_mat(CString sfile, cv::Mat mat, int quality);
	void		saveRawImage( CString sfile, uint8_t *data, int width, int height, int ch );
	void		save2Raw( cv::Mat mat, CString sRawFilename, int dst_ch = 1 );

void		matToCImage(cv::Mat &mat, CImage &cImage);
HBITMAP		ConvertMatToBMP (cv::Mat frame);
void		copy_to_clipboard(cv::Mat mat);
bool		copy_to_clipboard(void* bmp);
Mat			HBITMAP2Mat( HWND hwnd, HBITMAP hbwindow );
cv::Mat		HBITMAP2Mat(HBITMAP hBitmap);
void		FillGridPattern( Mat &src, int size = 16 );

//시작위치와 크기 지정
void		printMat( Mat img, CString sPrinterName, CString sDocName, int x, int dy, int dw, int dh, int copies = 1 );

//프린터 설정 용지 크기대로 인쇄. 인쇄하기 전에 이미지를 용지 크기 비율에 맞게 잘라줘야 한다.
//marginX, marginY는 픽셀 단위가 아닌 mm 단위임.
CSize		getCutSizeToFitPrinterPaper( Mat img, CString sPrinterName, int marginX = 0, int marginY = 0 );
CSize		getCutSizeToFitPrinterPaper( int w, int h, CString sPrinterName, int marginX = 0, int marginY = 0 );
void		printMat( Mat &img, CString sPrinterName, CString sDocName, int marginX = 0, int marginY = 0, int copies = 1 );



void		CannyEdge(uint8_t *src, uint8_t *dst, int width, int height, int th_low, int th_high );

bool		matIsEqual(const cv::Mat Mat1, const cv::Mat Mat2);

CRect		getCRect(cv::Rect r);
cv::Point	CenterPoint(cv::Rect r);

void		sobel(const unsigned char* src, unsigned char* dst, int width, int height);
void		sobel(const unsigned char* src, unsigned char* dst, int width, int height, int h, int v);

//img에서 templ 이미지와 가장 유사한 좌표를 리턴한다.
//cv::Point	find_matchTemplate( Mat img, Mat templ, int match_method = CV_TM_SQDIFF );
//img에서 templ_origin의 r 사각형 영역과 가장 유사한 사각형 영역을 리턴한다.
//cv::Rect	find_matchTemplate( Mat img, Mat templ_origin, cv::Rect r, int match_method = CV_TM_SQDIFF );

CPoint		cvPoint2CPoint( cv::Point src );
cv::Point	CPoint2cvPoint( CPoint src );
CRect		cvRect2CRect( cv::Rect src );
cv::Rect	CRect2cvRect( CRect src );
bool		ptInRect( cv::Rect src, cv::Point pt );
bool		ptInRect( cv::Rect src, CPoint pt );


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
// TypedMat<unsigned char> tm = image;    // 연결방법 1
// image가 1채널 grayscale 이미지일 경우
//tm[y][x] = 100;    // (x,y)의 픽셀값을 100으로 설정
// image가 3채널 color 이미지일 경우
//tm(y,x,0) = 100;    // (x,y)의 픽셀의 blue값을 100으로 설정
template<class T> class TypedMat
{
	T** m_pData;
	int m_nChannels;
	int m_nRows, m_nCols;

public:
	TypedMat():m_pData(NULL),m_nChannels(1),m_nRows(0),m_nCols(0){}
	~TypedMat(){if(m_pData) delete [] m_pData;}

	// OpenCV Mat 연동 (메모리 공유)
	void Attach(const cv::Mat& m);
	TypedMat(const cv::Mat& m):m_pData(NULL),m_nChannels(1),m_nRows(0),m_nCols(0) { Attach(m);}
	const TypedMat & operator =(const cv::Mat& m){ Attach(m); return *this;}

	// 행(row) 반환
	T* GetPtr(int r)
	{ assert(r>=0 && r<m_nRows); return m_pData[r];}

	// 연산자 중첩 (원소접근) -- 2D
	T * operator [](int r)
	{ assert(r>=0 && r<m_nRows); return m_pData[r];}

	const T * operator [](int r) const
	{ assert(r>=0 && r<m_nRows); return m_pData[r];}

	// 연산자 중첩 (원소접근) -- 3D
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

void reduceColorPalette(const Mat& inputImage, Mat& outputImage, const uchar intervalSize = 4);
void equalizeIntensity(const Mat& inputImage, Mat& outputImage);
void computeGradients(const Mat& inputImage, Mat& outputImage);
void processImage(const Mat& inputImage, Mat& outputImage);
void Sepia( Mat& mat, int magnitude = 75 );
Mat	 milky_white( Mat src, int blurSize, int level );

//mat이미지를 w x h 크기로 채운다.
void resizeGrid( Mat& mat, int width, int height );

void erode_filter( uint8_t* src, uint8_t* dst, int width, int height, int nX, int nY );
void erode_filter( uint8_t* src, int width, int height, int nX, int nY );
bool dilate_filter( uint8_t* src, uint8_t* dst, int width, int height, int nX, int nY );

//타깃 영역의 비율과 크기에 맞게 src영상을 자르고 resize한다.
Mat getFittingMat( Mat src, int tWidth, int tHeight );

void pixelLabeling( uint8_t* src, int width, int height, std::vector<std::vector<cv::Point>> &contours, int win_size_x = 3, int win_size_y = 3 );
Mat getLabelledColorImage( uint8_t* src, int width, int height );

#endif

