#include "StdAfx.h"
#include "RexMapper.h"

MapGroupData::MapGroupData()
{
	m_total		= 0;
	m_iChannel	= 0;

	for ( int i = 0; i < MAX_MAPDATA; i++ )
		m_images[i] = NULL;
}

MapGroupData::~MapGroupData()
{
	for(int i=0; i<m_total; i++)
	{
		if ( m_images[i] != NULL )
			cvReleaseImage(&m_images[i]);
	}	
}

RexMapper::RexMapper(void)
{
	m_last_map_data = NULL;
	m_temp_max_time_out_ms = 0;
}

RexMapper::~RexMapper(void)
{
	m_bRunthread = false;
	for(int i=0; i<(int)m_mapgroup_list.size(); i++)
	{
		m_mapgroup_list[i]->DestroyImage();
		delete m_mapgroup_list[i];
	}
	Sleep(10);
}

UINT RexMapper::MapperThread(LPVOID dParameter)
{
	RexMapper* pMine = (RexMapper*)dParameter;

	while(pMine->m_bRunthread)
	{
		pMine->Map();
		Sleep(1);
	}
	return 0;
}

void RexMapper::Init(void (*p_func)(char*), int max_cross_interval_ms, int max_timeout_ms, int max_image, int oneloop_shotnum)
{
	m_pCall_func = p_func;
	m_max_cross_interval_ms = max_cross_interval_ms;
	m_max_timeout_ms = max_timeout_ms;
	m_max_image = max_image;
	m_oneloop_shotnum = oneloop_shotnum;

	m_bRunthread = true;
	m_pMapperThread = ::AfxBeginThread(RexMapper::MapperThread, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
}

void RexMapper::SetOneLoopShotNum(int oneloop_shotnum)
{
	m_oneloop_shotnum = oneloop_shotnum;
}

void RexMapper::SetTimeInterval(int max_cross_interval_ms, int max_timeout_ms)
{
	m_cs_tag.Lock();
	m_cs_img.Lock();

	m_max_cross_interval_ms = max_cross_interval_ms;
	m_max_timeout_ms = max_timeout_ms;

	m_cs_img.Unlock();
	m_cs_tag.Unlock();
}

void RexMapper::SetTempTimeout(int temp_max_timeout_ms)
{
	m_temp_max_time_out_ms = temp_max_timeout_ms;
}

void RexMapper::PushTag(CString tag, CString etc, DWORD tc )
{
 	__timeb32	t;
 	_ftime32( &t );
	
	if(tc == 0)
		tc = GetTickCount();

	//CString str;
	//str.Format("%s    [ %ld ]", tag, tc);
	//UpdateLogFile(str);	
	
	m_cs_tag.Lock();
	m_last_tc = GetTickCount();
	m_tag_list.push_back(new MapData(tc, t, tag, NULL, etc, 0, 0, 0));
	m_cs_tag.Unlock();
	//LLOG(LegitLog::Info, L"tag %S [ %ld ] push", tag, tc);	
}

void RexMapper::PushImg(IplImage *img, DWORD tc, bool flash, unsigned int gain, unsigned int sspeed)
{
	__timeb32	t;
	_ftime32( &t );
	if(tc == 0)
		tc = GetTickCount();

	//CString str;
	//str.Format("image [ %ld ]", tc);
	//UpdateLogFile(str);	

	m_cs_img.Lock();
	m_last_tc = GetTickCount();
	if((int)m_img_list.size() <= m_max_image)
	{
		m_img_list.push_back(new MapData(tc, t, "", img, "", flash, gain, sspeed));
	}
	else
	{
		cvReleaseImage(&img);
	}	
	m_cs_img.Unlock();
	//LLOG(LegitLog::Info, L"img [ %ld ] push", tc);	
}


void RexMapper::Map()
{
	m_cs_tag.Lock();
	m_cs_img.Lock();

	MapTagTime();
	MapImgTime();

	char str[100] = {0};
	MapData* map_data = NULL;

	if(m_tag_list.empty() == false && m_img_list.empty() == false)
	{
		MapData* tag_data = m_tag_list.front();
		MapData* img_data = m_img_list.front();
		m_tag_list.pop_front();
		m_img_list.pop_front();

		//map_data = new MapData(tag_data->m_tc, tag_data->m_time, tag_data->m_tag, img_data->m_img, img_data->m_etc);
		map_data = new MapData(img_data->m_tc, img_data->m_time, tag_data->m_tag, img_data->m_img, tag_data->m_etc, img_data->m_flash, img_data->m_gain, img_data->m_sspeed);
		//cout << "new map data : " << tag_data->m_tag << " - " << img_data->m_etc << endl;
		delete tag_data;
		delete img_data;
	}
			
	if(map_data)
	{			
		//cout << map_data->GetOrder() << endl;
		if(map_data->GetOrder() == 1)
		{
			//new group throw
			//if(m_mapgroup_list.empty() == false)
			//	LLOG(LegitLog::Info, L"submit mode 0");
			SubmitMapGroupList();

			//for test
			//sprintf( str, "submit mode 0" );
			//m_pCall_func( str );
		}

		m_mapgroup_list.push_back(map_data);
		m_last_map_data = map_data;

		if(m_last_map_data->m_tag == "T3" || m_last_map_data->m_tag == "K3" || (m_last_map_data->m_tag.Left(1) == "O" && atoi(m_last_map_data->m_tag.Mid(1,1)) == m_oneloop_shotnum ))
		{
			//if(m_mapgroup_list.empty() == false)
			//	LLOG(LegitLog::Info, L"submit mode 1");

			//for test
			//sprintf( str, "submit mode 1" );
			//m_pCall_func( str );


			SubmitMapGroupList();
			m_last_map_data = NULL;
		}		
	}	
	else if(m_last_map_data)
	{
		//Timeout throw
		if(abs((int(GetTickCount() - m_last_tc))) > m_max_timeout_ms)
		{
			//if(m_mapgroup_list.empty() == false)
			//	LLOG(LegitLog::Info, L"submit mode 2 : %d", (int)(GetTickCount() - m_last_tc));
			//UpdateLogFile("submit3");

			//for test
			//sprintf( str, "submit mode 2 : %d", (int)(GetTickCount() - m_last_tc) );
			//m_pCall_func( str );

			
			SubmitMapGroupList();
			m_last_map_data = NULL;
		}
		if(m_temp_max_time_out_ms > 0)
		{
			if(abs((int(GetTickCount() - m_last_tc))) > m_temp_max_time_out_ms)
			{
				m_temp_max_time_out_ms = 0;
				//if(m_mapgroup_list.empty() == false)
				//	LLOG(LegitLog::Info, L"submit temp mode 3 : %d", (int)(GetTickCount() - m_last_tc));
				//UpdateLogFile("submit3");

				//for test
				//sprintf( str, "submit temp mode 3 : %d", (int)(GetTickCount() - m_last_tc) );
				//m_pCall_func( str );

				SubmitMapGroupList();		
				m_last_map_data = NULL;
			}
		}
	}

	m_cs_img.Unlock();
	m_cs_tag.Unlock();
}

void RexMapper::PushMapGroupData(MapGroupData* data)
{
	m_cs_mapgroupdata.Lock();
	if(m_mapgroupdata_list.size() <= m_max_image/3)
	{
		m_mapgroupdata_list.push_back(data);
	}
	else
	{
		delete data;
	}
	m_cs_mapgroupdata.Unlock();
}

void RexMapper::SubmitMapGroupList()
{
	if(m_mapgroup_list.empty())
		return;
	MapGroupData *data = new MapGroupData();
	data->m_total = (int)m_mapgroup_list.size();

	//UpdateLogFile("----------start map group-------------");
	for(int i=0; i < data->m_total; i++)
	{
		if(i < MAX_MAPDATA)
		{
			data->m_times[i] = m_mapgroup_list[i]->m_time;
			data->m_tags[i] = m_mapgroup_list[i]->m_tag;
			data->m_images[i] = m_mapgroup_list[i]->m_img;
			data->m_etcs[i] = m_mapgroup_list[i]->m_etc;
			data->m_flash[i] = m_mapgroup_list[i]->m_flash;
			data->m_gain[i] = m_mapgroup_list[i]->m_gain;
			data->m_sspeed[i] = m_mapgroup_list[i]->m_sspeed;

			CString str;
			str.Format("TAG : %s", m_mapgroup_list[i]->m_tag);
			//UpdateLogFile(str);
		}
	}
	if(data->m_total > MAX_MAPDATA)
		data->m_total = MAX_MAPDATA;

	m_cs_mapgroupdata.Lock();
	if(m_mapgroupdata_list.size() <= m_max_image/3)
	{
		m_mapgroupdata_list.push_back(data);
	}
	else
	{
		//printf( "mapgroupdata size = %d\n", m_mapgroupdata_list.size() );
		//LLOG( LegitLog::Info, L"mapgroupdata push cancelled because max_image." );
		delete data;
	}
	
	m_cs_mapgroupdata.Unlock();

	//UpdateLogFile("----------end map group-------------");
	
	//m_pCall_func(data);
	for(int i=0; i<(int)m_mapgroup_list.size(); i++)
	{
		//		if(m_mapgroup_list[i]->m_img)
		//			cvReleaseImage(&m_mapgroup_list[i]->m_img);
		delete m_mapgroup_list[i];
	}
	m_mapgroup_list.clear();
	//delete
}

MapGroupData* RexMapper::PopMapGroupData()
{
	MapGroupData* ret_data = NULL;
	m_cs_mapgroupdata.Lock();
	if(m_mapgroupdata_list.empty() == false)
	{
		ret_data = m_mapgroupdata_list.front();
		m_mapgroupdata_list.pop_front();
	}
	m_cs_mapgroupdata.Unlock();
	return ret_data;
}

void RexMapper::MapTagTime()
{
	if(m_tag_list.empty())
		return;

	char	 str[256] = {0};
	MapData* head_tag = m_tag_list.front();

	while(m_img_list.empty() == false)
	{
		MapData* img_data = m_img_list.front();
		if(img_data->m_tc > head_tag->m_tc)
			break;

		if(abs((int)img_data->m_tc - (int)head_tag->m_tc) > m_max_cross_interval_ms)
		{
			//CString str;
			//str.Format("remove img [%ld] [I%d - T%d]", ((int)img_data->m_tc - (int)head_tag->m_tc), (int)img_data->m_tc, (int)head_tag->m_tc);
			//LLOG(LegitLog::Warning, L"%S", str);
			//cout << "remove img\a : " << head_tag->m_etc << " : " << ((int)img_data->m_tc - (int)head_tag->m_tc) << endl;

			//for test
			//sprintf( str, "remove img [%d] [I%u - T%u]", (img_data->m_tc - head_tag->m_tc), img_data->m_tc, head_tag->m_tc );
			//m_pCall_func( str );

			m_img_list.pop_front();
			img_data->DestroyImage();
			delete img_data;
		}
		else
			break;
	}
}

void RexMapper::MapImgTime()
{
	if(m_img_list.empty())
		return;

	char	str[256] = {0};
	MapData* head_img = m_img_list.front();

	while(m_tag_list.empty() == false)
	{		
		MapData* tag_data = m_tag_list.front();
		if(tag_data->m_tc > head_img->m_tc)
			break;

		if(abs((int)tag_data->m_tc - (int)head_img->m_tc) > m_max_cross_interval_ms)
		{
			//CString str;
			//str.Format("remove tag %s [%ld] [T%d - I%d]", tag_data->m_tag, ((int)tag_data->m_tc - (int)head_img->m_tc), (int)tag_data->m_tc, (int)head_img->m_tc);
			//LLOG( LegitLog::Warning, L"%S", str);
			//cout << "remove tag\a : " << tag_data->m_tag  << " : " << head_img->m_etc << " : "<< ((int)tag_data->m_tc - (int)head_img->m_tc) << endl;

			//for test
			//sprintf( str, "remove tag %s [%ld] [T%u - I%u]", tag_data->m_tag, (tag_data->m_tc - head_img->m_tc), tag_data->m_tc, head_img->m_tc );
			//m_pCall_func( str );

			m_tag_list.pop_front();
			delete tag_data;
		}
		else
			break;
	}
}
