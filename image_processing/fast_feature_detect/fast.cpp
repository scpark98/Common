#include <stdio.h>
#include <stdlib.h>
#include "fast.h"

//��ó: http://onecellboy.tistory.com/188 [�źһ� - ����ȣ�� �Ҹ��� �糪��]
#ifdef DEBUG
#define TRACE(fmt,...) printf(fmt,##__VA_ARGS__) 
#else
#define TRACE(fmt,...)
#endif


//stride���� �� row �� byte���� ��Ÿ���µ�
//xsize�� 4�� ����̸� xsize�� �����ϰ� �ϰ�
//4�� ����� �ƴϸ�? xsize�� ��,�� 4�� ����� �־������
//�� ���� �׽�Ʈ ���󿡼��� ū ���̰� ������. �׳� xsize�� ����Ѵ�.
//7���� ���� xsize�� FAST���� Ư¡���� �������� ������ ����.
//image�� gray image�̾�� �Ѵ�.
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

		//nonmax_score���� ���� ������ �� ���� nMax�� �����Ѵ�.
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

	//non maximum�� �̿��ϸ� ��ø�� Ư¡���� �������ִ� ������ ������
	//���ھ ���� Ư¡���� ������ ���� ���ھ��� Ư¡���� �������� ������ �ִ�.
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
