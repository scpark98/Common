#include <stdio.h>
#include <string.h>
#include <math.h>
#include "kalman_filter.h"

CKalmanFilter::CKalmanFilter()
{
}

void CKalmanFilter::reset()
{
	m_init = false;
}

void CKalmanFilter::calcKalmanFilter(const double z[2], double y[2], double x_est[4], double p_est[16])
{
	signed char Q[16];
	int r2;
	double a[16];
	int k;
	double p_prd[16];
	double x_prd[4];
	int i0;
	double b_a[8];

	double b_z[2];
	double klm_gain[8];
	/*
	   --    Copyright 2010 The MathWorks, Inc. --
	   --  Initialize state transition matrix --
	   --      % [x  ] --
	   --      % [y  ] --
	   --      % [Vx] --
	   --      % [Vy] --
	   --  Initialize measurement matrix --
	*/
	for (r2 = 0; r2 < 16; r2++)
	{
		Q[r2] = 0;
	}

	//   --  Initial state conditions --
	//   --  Predicted state and covariance --

	for (k = 0; k < 4; k++)
	{
		Q[k + 4 * k] = 1;
		x_prd[k] = 0.0;
		for (r2 = 0; r2 < 4; r2++)
		{
			x_prd[k] += (double)A_T[k + 4 * r2] * x_est[r2];
			a[k + 4 * r2] = 0.0;
			for (i0 = 0; i0 < 4; i0++)
			{
				a[k + 4 * r2] += (double)A_T[k + 4 * i0] * p_est[i0 + 4 * r2];
			}
		}
	}

	for (r2 = 0; r2 < 4; r2++)
	{
		for (i0 = 0; i0 < 4; i0++)
		{
			a21 = 0.0;
			for (r1 = 0; r1 < 4; r1++)
			{
				a21 += a[r2 + 4 * r1] * (double)A[r1 + 4 * i0];
			}

			p_prd[r2 + 4 * i0] = a21 + (double)Q[r2 + 4 * i0];
		}
	}

	//   --  Estimation --
	for (r2 = 0; r2 < 2; r2++)
	{
		for (i0 = 0; i0 < 4; i0++)
		{
			b_a[r2 + (i0 << 1)] = 0.0;
			for (r1 = 0; r1 < 4; r1++)
			{
				b_a[r2 + (i0 << 1)] += (double)H_T[r2 + (r1 << 1)] * p_prd[i0 + 4 * r1];
			}
		}

		for (i0 = 0; i0 < 2; i0++)
		{
			a21 = 0.0;
			for (r1 = 0; r1 < 4; r1++) {
				a21 += b_a[r2 + (r1 << 1)] * (double)H[r1 + 4 * i0];
			}

			S[r2 + (i0 << 1)] = a21 + (double)R[r2 + (i0 << 1)];
		}

		for (i0 = 0; i0 < 4; i0++)
		{
			B[r2 + (i0 << 1)] = 0.0;
			for (r1 = 0; r1 < 4; r1++)
			{
				B[r2 + (i0 << 1)] += (double)H_T[r2 + (r1 << 1)] * p_prd[i0 + 4 * r1];
			}
		}
	}

	if (fabs(S[1]) > fabs(S[0]))
	{
		r1 = 1;
		r2 = 0;
	}
	else
	{
		r1 = 0;
		r2 = 1;
	}

	a21 = S[r2] / S[r1];
	a22 = S[2 + r2] - a21 * S[2 + r1];
	for (k = 0; k < 4; k++)
	{
		b_a[1 + (k << 1)] = (B[r2 + (k << 1)] - B[r1 + (k << 1)] * a21) / a22;
		b_a[k << 1] = (B[r1 + (k << 1)] - b_a[1 + (k << 1)] * S[2 + r1]) / S[r1];
	}

	//   --  Estimated state and covariance --
	for (r2 = 0; r2 < 2; r2++)
	{
		a21 = 0.0;
		for (i0 = 0; i0 < 4; i0++)
		{
			klm_gain[i0 + 4 * r2] = b_a[r2 + (i0 << 1)];
			a21 += (double)H_T[r2 + (i0 << 1)] * x_prd[i0];
		}

		b_z[r2] = z[r2] - a21;
	}

	for (r2 = 0; r2 < 4; r2++)
	{
		a21 = 0.0;
		for (i0 = 0; i0 < 2; i0++)
		{
			a21 += klm_gain[r2 + 4 * i0] * b_z[i0];
		}

		x_est[r2] = x_prd[r2] + a21;
		for (i0 = 0; i0 < 4; i0++)
		{
			a[r2 + 4 * i0] = 0.0;
			for (r1 = 0; r1 < 2; r1++)
			{
				a[r2 + 4 * i0] += klm_gain[r2 + 4 * r1] * (double)H_T[r1 + (i0 << 1)];
			}
		}

		for (i0 = 0; i0 < 4; i0++)
		{
			a21 = 0.0;
			for (r1 = 0; r1 < 4; r1++)
			{
				a21 += a[r2 + 4 * r1] * p_prd[r1 + 4 * i0];
			}

			p_est[r2 + 4 * i0] = p_prd[r2 + 4 * i0] - a21;
		}
	}

	//   --  Compute the estimated measurements --
	for (r2 = 0; r2 < 2; r2++)
	{
		y[r2] = 0.0;
		for (i0 = 0; i0 < 4; i0++)
		{
			y[r2] += (double)H_T[r2 + (i0 << 1)] * x_est[i0];
		}
	}
	//   -- end of the function --
}
/*
//점들에 대한 칼만을 구할때는 x1, y1만 의미있으나
//result는 int result[4]와 같이 4개 정수배열을 넘겨줘야 한다.
void CKalmanFilter::KalmanFilter( int* result, int x1, int y1, int x2, int y2 )
{
	double y11 = (y1 == INT_MAX ? DBL_MAX : (double)y1);
	double x21 = (x2 == INT_MAX ? DBL_MAX : (double)x2);
	double y21 = (y2 == INT_MAX ? DBL_MAX : (double)y2);

	KalmanFilter( (double*)result, (double)x1, y11, x21, y21 );
}
*/
void CKalmanFilter::KalmanFilter( double* result, double x1, double y1, double x2, double y2 )
{
	double result_temp[4];

	int		i;
	double	xy1_data[2] = { 0, }, xy1_dest[2] = { 0, };
	double	xy2_data[2] = { 0, }, xy2_dest[2] = { 0, };

	if (m_init == false)
	{
		for (i = 0; i < 2; i++)
		{
			memset(x_est[i], 0, sizeof(double) * 4);
			memset(p_est[i], 0, sizeof(double) * 16);
		}
		x_est[0][0] = x1;
		x_est[0][1] = y1;
		x_est[1][0] = x2;
		x_est[1][1] = y2;

		result_temp[0] = 0;
		result_temp[1] = 0;
		result_temp[2] = 0;
		result_temp[3] = 0;

		m_init = true;
	}

	xy1_data[0] = x1;
	xy1_data[1] = y1;
	calcKalmanFilter(xy1_data, xy1_dest, x_est[0], p_est[0]);

	xy2_data[0] = x2;
	xy2_data[1] = y2;
	calcKalmanFilter(xy2_data, xy2_dest, x_est[1], p_est[1]);

	result_temp[0] = xy1_dest[0];// + x_est[0][2];
	result_temp[1] = xy1_dest[1];// + x_est[0][3];
	result_temp[2] = xy2_dest[0];
	result_temp[3] = xy2_dest[1];

	result[0] = result_temp[0];
	result[1] = result_temp[1];

	if ( y1 != DBL_MAX )
	{
		result[1] = result_temp[1];
		
		if ( x2 != DBL_MAX )
		{
			result[2] = result_temp[2];

			if ( y2 != DBL_MAX )
				result[3] = result_temp[3];
		}
	}
}
