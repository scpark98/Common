#pragma once

#include <windows.h>
#include <pdh.h>

// PDH 湲곕컲 GPU ?ъ슜瑜? ?묒뾽愿由ъ옄? ?숈씪???뚯뒪(\GPU Engine(*)\Utilization Percentage).
// ?붿쭊 ??낅퀎(3D/Compute/Copy/VideoDecode/VideoEncode)濡??꾨줈?몄뒪 媛믪쓣 ?⑹궛????
// 洹?以?理쒕뙎媛믪쓣 諛섑솚 ???묒뾽愿由ъ옄 洹몃옒?꾩? ?쇱튂?섎뒗 吏묎퀎 諛⑹떇.
class CGpuUsage
{
public:
	CGpuUsage();
	~CGpuUsage();

	int get_usage();	// 0~100, ?ㅽ뙣 ??-1

private:
	PDH_HQUERY   m_query = NULL;
	PDH_HCOUNTER m_counter = NULL;
	bool         m_ready = false;
};
