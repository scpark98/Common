/*
- 2013. 06. 박세용 전임 작성
- 카메라 이미지와 트리거 신호를 매핑
- 시리얼로 들어오는 트리거 신호의 불규칙성으로 인해 매핑 시 누락이 많이 발생하여 사용 보류
*/

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
	MapGroupData();
	~MapGroupData();
	int			m_total;
	int			m_iChannel;				//동영상 검지모드에서 가상 채널 인덱스
	IplImage*	m_images[MAX_MAPDATA];
	CString		m_tags[MAX_MAPDATA];
	CString		m_etcs[MAX_MAPDATA];
	__timeb32	m_times[MAX_MAPDATA];
	bool		m_flash[MAX_MAPDATA];
	unsigned int m_gain[MAX_MAPDATA];
	unsigned int m_sspeed[MAX_MAPDATA];
};

class MapData
{
public:
	CString m_tag;
	IplImage* m_img;
	CString m_etc;
	DWORD m_tc;
	__timeb32 m_time;
	bool m_flash;
	unsigned int m_gain;
	unsigned int m_sspeed;

	MapData(DWORD tc, __timeb32 time, CString tag, IplImage* img, CString etc, bool flash, unsigned int gain, unsigned int sspeed)
	{
		m_tag = tag;
		m_img = img;
		m_etc = etc;
		m_tc = tc;
		m_time = time;
		m_flash = flash;
		m_gain = gain;
		m_sspeed = sspeed;
	}

	~MapData()
	{		
	}

	void DestroyImage()
	{
		if ( m_img )
		{
			cvReleaseImage(&m_img);
			m_img = NULL;
		}
	}

	int GetOrder()
	{
		int ret = 0;
		if(m_tag.GetLength() < 2)
			return 0;
		try
		{
			ret = atoi(m_tag.Mid(1,1));
		}catch(...)
		{
			ret = 0;
		}
		return ret;
	}
};

class RexMapper
{
public:
	RexMapper(void);
	~RexMapper(void);

	bool m_bRunthread;
	CCriticalSection m_cs_tag, m_cs_img, m_cs_mapgroupdata;
	list<MapData*> m_tag_list;
	list<MapData*> m_img_list;

	list<MapGroupData*> m_mapgroupdata_list;
	MapGroupData* PopMapGroupData();
	void PushMapGroupData(MapGroupData* data);

	MapData* m_last_map_data;
	vector<MapData*> m_mapgroup_list;

	static UINT MapperThread(LPVOID dParameter);

	void (*m_pCall_func)(char* str);

	CWinThread *m_pMapperThread;

	//max_out		: 차량 간격으로 판단하는 최소 시간
	//max_cross		: 트리거 타임와 이미지 그랩 타임과의 매핑을 위한 최대 허용 시간 차이
	//max_timeout	: 매핑큐에 데이터가 존재할 경우 다음 데이터가 없을 경우 파일큐로 넘기기 위한 최대 대기 시간

	int m_max_cross_interval_ms;
	int m_max_timeout_ms;
	int m_max_image;
	int m_oneloop_shotnum;

	DWORD m_last_tc;

	void Init(void (*p_func)(char*), int max_cross_interval_ms, int max_timeout_ms, int max_image, int oneloop_shotnum);
	void SetTimeInterval(int max_cross_interval_ms, int max_timeout_ms);
	void SetOneLoopShotNum(int oneloop_shotnum);
	
	int m_temp_max_time_out_ms;
	void SetTempTimeout(int temp_max_timeout_ms);

	void PushTag(CString tag, CString etc, DWORD tc = 0 );
	void PushImg(IplImage *img, DWORD tc, bool flash, unsigned int gain, unsigned int sspeed);

	void Map();
	void SubmitMapGroupList();
	void MapTagTime();
	void MapImgTime();
};

