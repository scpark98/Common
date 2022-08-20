#ifndef __KALMANFILTER__H__
#define __KALMANFILTER__H__

#include <limits.h>
#include <float.h>

/* usage
CKalmanFilter	m_kalman;	//declare instance variable

int pt[2];					//for point type
int rect[4];				//for rectangle type
...

//for kalman point 
m_kalman.KalmanFilter( pt, x, y );

//for kalman rect
m_kalman.KalmanFilter( rect, l, t, r, b );
*/

class CKalmanFilter
{
	double	S[4];
	double	a21;
	int		r1;
	double	a22;
	double	B[12];

	signed char A_T[16] = { 1, 0, 0, 0,
							0, 1, 0, 0,
							1, 0, 1, 0,
							0, 1, 0, 1 };

	signed char A[16] = { 1, 0, 1, 0,
						  0, 1, 0, 1,
						  0, 0, 1, 0,
						  0, 0, 0, 1 };

	signed char H_T[8] = { 1, 0, 0, 1,
						   0, 0, 0, 0 };

	short R[4] = { 50, 0, 0, 50 };

	signed char H[8] = { 5, 0, 0, 0,
						 0, 5, 0, 0 };

	double x_est[3][4] = { 0, };  //[3] = (x,y) , (w,0), (h,0)
	double p_est[3][16] = { 0, };

public:
	CKalmanFilter();

	void	reset();
	//void	KalmanFilter( int* result, int x1, int y1 = INT_MAX, int x2 = INT_MAX, int y2 = INT_MAX );
	void	KalmanFilter( double* result, double x1, double y1 = DBL_MAX, double x2 = DBL_MAX, double y2 = DBL_MAX );

protected:
	bool	m_init = false;
	void	calcKalmanFilter(const double z[2], double y[2], double x_est[4], double p_est[16]);
};

#endif