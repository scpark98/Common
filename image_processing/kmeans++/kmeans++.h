#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#define M_PI   3.14159265358979323846264338327950288

//�� ���� ���� double Ÿ������ ó���ǳ� MOD ���������� �����ϱ� ����
//vsdouble�� define�Ͽ� �׽�Ʈ�� ������.
//�Ϻ� �Լ��� double�� ���� int�� �����ؼ��� �ȵǴ� �κп� ������ ��.
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
