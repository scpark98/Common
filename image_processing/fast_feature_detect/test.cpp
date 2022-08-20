#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "fast.h"


using namespace cv;

void main()
{
	Mat src = imread( "d:\\night000_1920x620.bmp" );
	Mat gray;
	Mat result;
	int	i, j;
	int xsize = src.cols;
	int ysize = src.rows;
	int stride = xsize;
	int threshold;			//mean의 1/2이 적당.
	int numcorners;
	int sum = 0;

	src.copyTo( result );

	if ( src.channels() == 4 )
		cvtColor( src, gray, CV_BGRA2GRAY );
	if ( src.channels() == 3 )
		cvtColor( src, gray, CV_BGR2GRAY );

	for ( i = 0; i < ysize; i++ )
	for ( j = 0; j < xsize; j++ )
		sum += gray.data[i * xsize + j];

	threshold = sum / (xsize * ysize) / 2.0;


	xy* corners = fast09_detect_nonmax( (unsigned char*)gray.data, xsize, ysize, stride, threshold, &numcorners);

	for ( int i = 0; i < numcorners; i++ )
		cv::circle( result, cv::Point( corners[i].x, corners[i].y ), 1, cv::Scalar(0,0,255), -1, CV_AA );

	printf( "numcorners = %d\n", numcorners );
	imshow( "result", result );

	char str[256];
	sprintf( str, "d:\\night000_1920x620_09_%d.bmp", numcorners );
	imwrite( str, result );
	cv::waitKey(0);

}
