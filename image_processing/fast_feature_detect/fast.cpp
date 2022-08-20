#include <stdio.h>
#include <stdlib.h>
#include "fast.h"

//출처: http://onecellboy.tistory.com/188 [신불사 - 신현호라 불리는 사나이]
#ifdef DEBUG
#define TRACE(fmt,...) printf(fmt,##__VA_ARGS__) 
#else
#define TRACE(fmt,...)
#endif


//stride값은 한 row 당 byte수를 나타내는데
//xsize가 4의 배수이면 xsize와 동일하게 하고
//4의 배수가 아니면? xsize의 앞,뒤 4의 배수를 넣어봤으나
//몇 가지 테스트 영상에서는 큰 차이가 없었다. 그냥 xsize를 사용한다.
//7보다 작은 xsize는 FAST에서 특징점이 구해지지 않음에 주의.
//image는 gray image이어야 한다.
xy* fast09_detect_nonmax(const byte* image, int xsize, int ysize, int stride, int* b, int* ret_num_corners, int nMax, bool bUseNonmax)
{
	int i;
	xy* corners;
	int num_corners = 0;
	int* scores;
	xy* nonmax;
	int* nonmax_score;
	int nMinCorners = (double)nMax * 0.9;
	int nMaxCorners = (double)nMax * 1.1;

	bool bPrintData = false;

	corners = fast09_detect(image, xsize, ysize, stride, *b, &num_corners);
	scores = fast09_score(image, stride, corners, num_corners, *b);

	if ( bPrintData )
	{
		TRACE( "\noriginal corners===========================\n" );
		for ( i = 0; i < num_corners; i++ )
			TRACE( "%3d, %3d, %3d, %5d\n", i, corners[i].x, corners[i].y, scores[i] );
	}

	if (bUseNonmax)
	{
		nonmax_score = (int*)malloc(num_corners * sizeof(int));
		nonmax = nonmax_suppression(corners, scores, num_corners, ret_num_corners, nonmax_score);

		if ( bPrintData )
		{
			TRACE( "after nonmax------------------------------\n" );
			for ( i = 0; i < *ret_num_corners; i++ )
				TRACE( "%3d, %3d, %3d, %5d\n", i, nonmax[i].x, nonmax[i].y, nonmax_score[i] );
		}

		//nonmax_score값에 따라 정렬한 후 상위 nMax를 리턴한다.
		if ( *ret_num_corners > nMax )
		{
			sort( nonmax, nonmax_score, *ret_num_corners );
			*ret_num_corners = nMax;

			if ( bPrintData )
			{
				TRACE( "after sorted------------------------------\n" );
				for ( i = 0; i < nMax; i++ )
					TRACE( "%3d, %3d, %3d, %5d\n", i, nonmax[i].x, nonmax[i].y, nonmax_score[i] );
			}
		}

		if ( bPrintData )
			TRACE( "===========================================\n" );

		free(corners);
		free(scores);
		free(nonmax_score);

		return nonmax;
	}

	//non maximum을 이용하면 중첩된 특징점을 제거해주는 장점은 있으나
	//스코어가 높은 특징점을 버리고 낮은 스코어의 특징점이 구해지는 단점도 있다.
	*ret_num_corners = num_corners;
	if ( *ret_num_corners > nMax )
	{
		sort( corners, scores, num_corners );
		*ret_num_corners = nMax;

		if ( bPrintData )
		{
			TRACE( "after sorted------------------------------\n\n" );
			for ( i = 0; i < nMax; i++ )
				TRACE( "%3d, %3d, %3d, %5d\n", i, corners[i].x, corners[i].y, scores[i] );
		}
	}

	free(scores);

	return corners;
}
/*
xy* fast10_detect_nonmax(const byte* im, int xsize, int ysize, int stride, int* b, int* ret_num_corners, int nMax)
{
	xy* corners;
	int num_corners;
	int* scores;
	xy* nonmax;
	int* nonmax_score;

	corners = fast10_detect(im, xsize, ysize, stride, *b, &num_corners);
	scores = fast10_score(im, stride, corners, num_corners, *b);
	nonmax_score = (int*)malloc(num_corners * sizeof(int));
	nonmax = nonmax_suppression(corners, scores, num_corners, ret_num_corners, nonmax_score);

	free(corners);
	free(scores);
	free(nonmax_score);

	return nonmax;
}

xy* fast11_detect_nonmax(const byte* im, int xsize, int ysize, int stride, int* b, int* ret_num_corners, int nMax)
{
	xy* corners;
	int num_corners;
	int* scores;
	xy* nonmax;
	int* nonmax_score;

	corners = fast11_detect(im, xsize, ysize, stride, *b, &num_corners);
	scores = fast11_score(im, stride, corners, num_corners, *b);
	nonmax_score = (int*)malloc(num_corners * sizeof(int));
	nonmax = nonmax_suppression(corners, scores, num_corners, ret_num_corners, nonmax_score);

	free(corners);
	free(scores);
	free(nonmax_score);

	return nonmax;
}

xy* fast12_detect_nonmax(const byte* im, int xsize, int ysize, int stride, int* b, int* ret_num_corners, int nMax)
{
	xy* corners;
	int num_corners;
	int* scores;
	xy* nonmax;
	int* nonmax_score;

	corners = fast12_detect(im, xsize, ysize, stride, *b, &num_corners);
	scores = fast12_score(im, stride, corners, num_corners, *b);
	nonmax_score = (int*)malloc(num_corners * sizeof(int));
	nonmax = nonmax_suppression(corners, scores, num_corners, ret_num_corners, nonmax_score);

	free(corners);
	free(scores);
	free(nonmax_score);

	return nonmax;
}
*/
void swap( int& a, int& b )
{
	int temp = a;
	a = b;
	b = temp;
}

void swapxy( xy& a, xy& b )
{
	xy temp;

	temp.x = a.x;
	temp.y = a.y;

	a.x = b.x;
	a.y = b.y;

	b.x = temp.x;
	b.y = temp.y;
}

void sort(xy* data, int* scores, int num)
{
	int i, j;

	for ( i = 0; i < num - 1; i++ )
	{
		for ( j = 0; j < num - i - 1; j++ )
		{
			if ( scores[j] < scores[j+1] )
			{
				swap( scores[j], scores[j+1] );
				swapxy( data[j], data[j+1] );
			}
		}
	}
}
