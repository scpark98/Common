#include "StdAfx.h"
#include "RexImageMapper.h"


RexImageMapper::RexImageMapper()
{
	m_last_map_data = NULL;
	m_last_group_tc	= 0;
}

RexImageMapper::~RexImageMapper()
{
	m_bRunthread = false;

	for( int i = 0; i < (int)m_mapgroup_list.size(); i++)
	{
		m_mapgroup_list[i]->DestroyImage();
		delete m_mapgroup_list[i];
	}
	Sleep(10);
}

UINT RexImageMapper::MapperThread(LPVOID dParameter)
{
	RexImageMapper* pMine = (RexImageMapper*)dParameter;

	while(pMine->m_bRunthread)
	{
		pMine->Map();
		Sleep(50);
	}
	return 0;
}

void RexImageMapper::Init(void (*p_func)(MapGroupData*), int max_timeout_ms, int nMaxImage )
{
	m_pCall_func		= p_func;
	m_max_timeout_ms	= m_nDynanicTimeout = max_timeout_ms;
	m_nMaxImage			= nMaxImage;
	m_nTriggerGap		= 0;

	m_bRunthread = true;
	m_pMapperThread = ::AfxBeginThread(RexImageMapper::MapperThread, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
}

void RexImageMapper::SetTimeIntervalAndMaxImage(int max_timeout_ms, int nMaxImage )
{
	m_cs_img.Lock();

	m_max_timeout_ms	= max_timeout_ms;
	m_nMaxImage			= nMaxImage;

	m_cs_img.Unlock();
}

void RexImageMapper::PushImg(IplImage *img, DWORD tc, int nSpeed, bool bForward )
{
	__timeb32	t;
	_ftime32( &t );

	if ( tc == 0 )
		tc = GetTickCount();

	m_cs_img.Lock();
	m_last_tc = GetTickCount();

	LLOG(LegitLog::Info, L"img [ %ld ] push", tc);	
	m_img_list.push_back( new MapData( tc, t, img, nSpeed, bForward ) );
	m_cs_img.Unlock();
}

void RexImageMapper::Map()
{
	m_cs_img.Lock();

	MapData* map_data = NULL;

	if ( m_mapgroup_list.size() >= m_nMaxImage )
	{
		m_last_group_tc = 0;
		SubmitMapGroupList();
	}

	if ( m_img_list.empty() == false )
	{
		MapData* img_data = m_img_list.front();
		m_img_list.pop_front();

		map_data = new MapData( img_data->m_tc, img_data->m_time, img_data->m_img, img_data->m_nSpeed, img_data->m_bForward );
		delete img_data;
	}

	if ( map_data )
	{
		//현재 팝업된 데이터의 시간값이 바로 이전 push된 데이터의 시간값과 일정 시간 이상 차이나면
		//새로운 차량으로 판단하고 submit을 해준 후 push해야 한다.
		if ( m_mapgroup_list.size() && ( m_last_group_tc > 0 ) )
		{
			if ( m_mapgroup_list.size() == 1)
			{
				if ( (int)( map_data->m_tc - m_last_group_tc ) > m_max_timeout_ms )
				{
					LLOG( LegitLog::Info, L"3.size = 1, m_tc-lastgtc = %d, map_data->tc = %d, m_last_group_tc = %d", map_data->m_tc - m_last_group_tc, map_data->m_tc, m_last_group_tc );
					m_last_group_tc = 0;
					SubmitMapGroupList();
				}
			}
			else if ( m_mapgroup_list.size() > 1)
			{
				if ( (int)( map_data->m_tc - m_last_group_tc ) > m_nTriggerGap * 3 )
				{
					LLOG( LegitLog::Info, L"4.size = %d, gap timeout = %d (data_tc=%d - lastgtc=%d), gap = %d", m_mapgroup_list.size(), map_data->m_tc - m_last_group_tc, map_data->m_tc, m_last_group_tc, m_nTriggerGap );
					m_last_group_tc = 0;
					SubmitMapGroupList();
				}
			}
		}

		m_mapgroup_list.push_back(map_data);

		if ( m_mapgroup_list.size() == 1 )
			m_nTriggerGap = 0;
		else if ( m_mapgroup_list.size() > 1 )
			m_nTriggerGap = map_data->m_tc - m_last_group_tc;

		if ( m_mapgroup_list.size() > 0 )
			m_last_group_tc = map_data->m_tc;
	}
	else
	{
		DWORD tc = GetTickCount();

		if ( m_mapgroup_list.size() == 1 )
		{
			if ( ( (int)( tc - m_last_group_tc ) > m_max_timeout_ms ) )
			{
				LLOG( LegitLog::Info, L"1.size = 1, max timeout = %d (cur=%d - lastgtc=%d)", tc - m_last_group_tc, tc, m_last_group_tc );
				m_last_group_tc = 0;
				SubmitMapGroupList();
			}
		}
		else
		if ( m_mapgroup_list.size() > 1 )
		{
			if ( ( (int)( tc - m_last_group_tc ) > m_nTriggerGap * 3 ) )
			{
				LLOG( LegitLog::Info, L"2.size = %d, gap timeout = %d (cur=%d - lastgtc=%d), gap = %d", m_mapgroup_list.size(), tc - m_last_group_tc, tc, m_last_group_tc, m_nTriggerGap );
				m_last_group_tc = 0;
				SubmitMapGroupList();
			}
		}
	}

	m_cs_img.Unlock();
}

void RexImageMapper::SubmitMapGroupList()
{
	if(m_mapgroup_list.empty())
		return;

	MapGroupData data;
	data.m_total = (int)m_mapgroup_list.size();

	//UpdateLogFile("----------start map group-------------");
	for(int i=0; i<data.m_total; i++)
	{
		if ( i < MAX_MAPDATA )
		{
			data.m_times[i]		= m_mapgroup_list[i]->m_time;
			data.m_images[i]	= m_mapgroup_list[i]->m_img;
			data.m_nSpeed[i]	= m_mapgroup_list[i]->m_nSpeed;
			data.m_bForward		= m_mapgroup_list[i]->m_bForward;
		}

	}
	if(data.m_total > MAX_MAPDATA)
		data.m_total = MAX_MAPDATA;

	//UpdateLogFile("----------end map group-------------");
	
	m_pCall_func(&data);

	//delete
	for(int i=0; i<(int)m_mapgroup_list.size(); i++)
	{
		if(m_mapgroup_list[i]->m_img)
			cvReleaseImage(&m_mapgroup_list[i]->m_img);
		delete m_mapgroup_list[i];
	}

	m_mapgroup_list.clear();
}
