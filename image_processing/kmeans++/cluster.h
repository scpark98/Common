#pragma once

#include "../../Common/DoubleLinkedListTemplete.h"

class ClusterPoint
{
public:
	double	x;
	double	y;
	int		group;							//Ŭ������ �׷� �ε���
	ClusterPoint() : x(0.0), y(0.0) {}
	ClusterPoint(double x1, double y1) { x = x1; y = y1; }
};

class Cluster
{
public:
	CDoublyLinkedList<ClusterPoint> pt;	//�� Ŭ������ ���� ����. ��ȿ���� ���� ������ �����ؾ� �ϹǷ� �迭���ٴ� DLL�� ����Ͽ� ������.
	ClusterPoint min, max;				//���� �� �ּ�, �ִ�
	double		cx, cy;					//Ŭ�������� ����. Ŭ������ ������ ���Ͱ� �ƴ� kmeans�� ���ؼ� ���� ������ ������.
	double		area;					//�ִ� �簢�� ����
	double		density;				//������ �е�
	double		distance;				//Ŭ�����Ϳ� ī�޶��� ���� �Ÿ�
	int			group;
	bool		bValid;					//�־��� ���ǿ� �����ϴ� Ŭ����������
	double		dOldcx;					//Optical flow�� Ŭ�����ͳ��� ������ dx, dy �̵��Ÿ��� ����صд�. �� �Ÿ���ŭ Ŭ������ �簢���� �������ش�.
	double		dOldcy;

	//Ŭ������ ���� ������ ������ ���� ũ�� ����ġ ��հ�
	double		dVectorQuantityX;
	double		dVectorQuantityY;

	int			noVectorCount;			//n������ ���� �������� ���ٸ� �ش� Ŭ�����ʹ� invalid��Ų��.
	double		width() { return max.x - min.x; }
	double		height() { return max.y - min.y; }

	double		centerx() { return (min.x + width() / 2.0); }
	double		centery() { return (min.y + height() / 2.0); }

	void recalcClusterSize()			//�ִ� �簢���� ���ϸ� ����, �е��� ���ŵȴ�.
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

	//���� Ŭ�������� ������ �ٸ� Ŭ�����ͷ� �̵� �Ǵ� �����Ѵ�.
	void MoveTo(Cluster& clusterDest, bool bCopy)
	{
		Node<ClusterPoint> *t = pt.begin();
		while (t)
		{
			clusterDest.pt.addNode(t->data);
			if (bCopy)
				t = t->next;					//����
			else
				t = pt.deleteNode(t, true);	//���� �� ���� = �̵�
		}

		//src, dst Ŭ�������� ���� ������ �޶����ٸ�
		//ũ��, �е� � �ٽ� ���Ž������ �Ѵ�.
		clusterDest.recalcClusterSize();
		if (!bCopy)
			recalcClusterSize();
	}
};
