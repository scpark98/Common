#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#define M_PI   3.14159265358979323846264338327950288

//각 점은 원래 double 타입으로 처리되나 MOD 과제용으로 포팅하기 위해
//vsdouble을 define하여 테스트를 진행함.
//일부 함수의 double은 절대 int로 변경해서는 안되는 부분에 주의할 것.
#define vsdouble	double

typedef struct
{
	vsdouble x, y;
	int group;
} kmpoint_t, *kmpoint;

kmpoint			gen_xy( int count, vsdouble dMaxX, vsdouble dMaxY );
inline vsdouble dist2(kmpoint a, kmpoint b);
inline int		nearest(kmpoint pt, kmpoint cent, int n_cluster, vsdouble *d2);
void			kpp(kmpoint pts, int len, kmpoint cent, int n_cent);
kmpoint			lloyd(kmpoint pts, int len, int n_cluster);
void			print_eps(kmpoint pts, int len, kmpoint cent, int n_cluster);
