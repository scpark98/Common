/*
- 2013. 07. 23 �ڻ��� å�� �ۼ�
- ī�޶� �̹��� ������ ����
- RexMapper�� �ø���� ������ Ʈ���� ��ȣ�� �ұ�Ģ������ ���� ���� �� ������ ���� �߻��Ͽ� ��� ����
*/
#pragma  once

#include <sys/timeb.h>
#include <list>
#include <vector>
#include <queue>
#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;
using namespace std;

#pragma once

#define MAX_MAPDATA 7

class MapGroupData
{
public:
	int			m_total;
	int			m_nSpeed[MAX_MAPDATA];
	bool		m_bForward;
	IplImage*	m_images[MAX_MAPDATA];
	__timeb32	m_times[MAX_MAPDATA];
};

class MapData
{
public:
	IplImage*	m_img;
	int			m_nSpeed;
	bool		m_bForward;
	DWORD		m_tc;
	__timeb32	m_time;

	MapData( DWORD tc, __timeb32 time, IplImage* img, int nSpeed, bool bForward )
	{
		m_tc		= tc;
		m_time		= time;
		m_img		= img;
		m_nSpeed	= nSpeed;
		m_bForward	= bForward;
	}

	~MapData()
	{		
	}

	void DestroyImage()
	{
		if ( m_img )
			cvReleaseImage( &m_img );
	}
};

class RexImageMapper
{
public:
	RexImageMapper();
	~RexImageMapper();

	void	Init( void (*p_func)(MapGroupData*), int max_timeout_ms, int nMaxImage );
	void	SetTimeIntervalAndMaxImage( int max_timeout_ms, int m_nMaxImage );
	void	PushImg(IplImage *img, DWORD tc, int nSpeed, bool bForward );

protected:
	CWinThread*			m_pMapperThread;
	bool				m_bRunthread;
	CCriticalSection	m_cs_img;
	list<MapData*>		m_img_list;

	MapData*			m_last_map_data;
	vector<MapData*>	m_mapgroup_list;

	static UINT MapperThread(LPVOID dParameter);

	void (*m_pCall_func)(MapGroupData* data);


	//max_timeout	: ����ť�� �����Ͱ� ������ ��� ���� �����Ͱ� ���� ��� ����ť�� �ѱ�� ���� �ִ� ��� �ð�
	int					m_max_timeout_ms;
	int					m_nTriggerGap;		//T1~T2 or T2~T3 ���� �������� ���� Ÿ�Ӿƿ��� ��������.
	int					m_nDynanicTimeout;	//���� Ÿ�Ӿƿ���

	int					m_nMaxImage;	//�ִ� �Կ� �̹��� �� (T, K�� 3, O�� �ִ� 7)

	DWORD				m_last_tc;			//���������� push�� ������
	DWORD				m_last_group_tc;	//group_list�� ���������� push�� ������

	void	Map();
	void	SubmitMapGroupList();
	void	MapImgTime();
};

