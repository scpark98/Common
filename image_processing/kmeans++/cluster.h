#pragma once

#include "../../Common/DoubleLinkedListTemplete.h"

class ClusterPoint
{
public:
	double	x;
	double	y;
	int		group;							//클러스터 그룹 인덱스
	ClusterPoint() : x(0.0), y(0.0) {}
	ClusterPoint(double x1, double y1) { x = x1; y = y1; }
};

class Cluster
{
public:
	CDoublyLinkedList<ClusterPoint> pt;	//각 클러스터 내의 점들. 유효하지 않은 점들은 제거해야 하므로 배열보다는 DLL을 사용하여 구현함.
	ClusterPoint min, max;				//점들 중 최소, 최대
	double		cx, cy;					//클러스터의 센터. 클러스터 영역의 센터가 아닌 kmeans를 통해서 구한 점들의 센터임.
	double		area;					//최대 사각형 면적
	double		density;				//점들의 밀도
	double		distance;				//클러스터와 카메라의 추정 거리
	int			group;
	bool		bValid;					//주어진 조건에 만족하는 클러스터인지
	double		dOldcx;					//Optical flow로 클러스터내의 점들의 dx, dy 이동거리를 기억해둔다. 이 거리만큼 클러스터 사각형도 움직여준다.
	double		dOldcy;

	//클러스터 내의 점들의 움직임 벡터 크기 절대치 평균값
	double		dVectorQuantityX;
	double		dVectorQuantityY;

	int			noVectorCount;			//n프레임 연속 움직임이 없다면 해당 클러스터는 invalid시킨다.
	double		width() { return max.x - min.x; }
	double		height() { return max.y - min.y; }

	double		centerx() { return (min.x + width() / 2.0); }
	double		centery() { return (min.y + height() / 2.0); }

	void recalcClusterSize()			//최대 사각형이 변하면 면적, 밀도도 갱신된다.
	{
		min = ClusterPoint(10000.0, 10000.0);
		max = ClusterPoint(-10000.0, -10000.0);

		if (pt.getCount() == 0 || pt.begin() == NULL)
		{
			area = density = 0.0;
			return;
		}

		Node<ClusterPoint> *t = pt.begin();

		while (t)
		{
			min.x = MIN(min.x, t->data.x);
			min.y = MIN(min.y, t->data.y);
			max.x = MAX(max.x, t->data.x);
			max.y = MAX(max.y, t->data.y);
			t = t->next;
		}

		area = (max.x - min.x) * (max.y - min.y);

		if (area == 0)
			density = 0.0;
		else
			density = (double)pt.getCount() * 100.0 / area;
	}

	//현재 클러스터의 점들을 다른 클러스터로 이동 또는 복사한다.
	void MoveTo(Cluster& clusterDest, bool bCopy)
	{
		Node<ClusterPoint> *t = pt.begin();
		while (t)
		{
			clusterDest.pt.addNode(t->data);
			if (bCopy)
				t = t->next;					//복사
			else
				t = pt.deleteNode(t, true);	//복사 후 삭제 = 이동
		}

		//src, dst 클러스터의 점들 구성이 달라졌다면
		//크기, 밀도 등도 다시 갱신시켜줘야 한다.
		clusterDest.recalcClusterSize();
		if (!bCopy)
			recalcClusterSize();
	}
};
