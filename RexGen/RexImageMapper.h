/*
- 2013. 07. 23 박상춘 책임 작성
- 카메라 이미지 만으로 매핑
- RexMapper는 시리얼로 들어오는 트리거 신호의 불규칙성으로 인해 매핑 시 누락이 많이 발생하여 사용 보류
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


	//max_timeout	: 매핑큐에 데이터가 존재할 경우 다음 데이터가 없을 경우 파일큐로 넘기기 위한 최대 대기 시간
	int					m_max_timeout_ms;
	int					m_nTriggerGap;		//T1~T2 or T2~T3 사이 간격으로 동적 타임아웃을 적용하자.
	int					m_nDynanicTimeout;	//동적 타임아웃값

	int					m_nMaxImage;	//최대 촬영 이미지 수 (T, K는 3, O는 최대 7)

	DWORD				m_last_tc;			//마지막으로 push된 데이터
	DWORD				m_last_group_tc;	//group_list에 마지막으로 push된 데이터

	void	Map();
	void	SubmitMapGroupList();
	void	MapImgTime();
};

