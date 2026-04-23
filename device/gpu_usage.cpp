#include "gpu_usage.h"

#include <pdhmsg.h>
#include <vector>
#include <unordered_map>
#include <string>

#pragma comment(lib, "pdh.lib")

CGpuUsage::CGpuUsage()
{
	if (PdhOpenQueryW(NULL, 0, &m_query) != ERROR_SUCCESS)
		return;

	// ?곷Ц 移댁슫?곕챸?쇰줈 異붽? ???쒓? ??濡쒖????곹뼢 ?놁쓬
	if (PdhAddEnglishCounterW(m_query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &m_counter) != ERROR_SUCCESS)
	{
		PdhCloseQuery(m_query);
		m_query = NULL;
		return;
	}

	// 珥덇린 ?섏쭛 ???쇰? 移댁슫?곕뒗 2???섏쭛 ?꾨???媛믪씠 ?섏샂
	PdhCollectQueryData(m_query);
	m_ready = true;
}

CGpuUsage::~CGpuUsage()
{
	if (m_query)
		PdhCloseQuery(m_query);
}

int CGpuUsage::get_usage()
{
	if (!m_ready)
		return -1;

	if (PdhCollectQueryData(m_query) != ERROR_SUCCESS)
		return -1;

	DWORD buf_size = 0, item_count = 0;
	PDH_STATUS status = PdhGetFormattedCounterArrayW(m_counter, PDH_FMT_DOUBLE, &buf_size, &item_count, NULL);
	if (status != PDH_MORE_DATA)
		return -1;

	std::vector<BYTE> buffer(buf_size);
	PDH_FMT_COUNTERVALUE_ITEM_W* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());

	if (PdhGetFormattedCounterArrayW(m_counter, PDH_FMT_DOUBLE, &buf_size, &item_count, items) != ERROR_SUCCESS)
		return -1;

	// ?몄뒪?댁뒪紐??? "pid_1234_luid_0x00000000_0x0000ABCD_phys_0_eng_0_engtype_3D"
	// ?붿쭊 ??낅퀎濡??꾨줈?몄뒪 ?ъ슜瑜좎쓣 ?⑹궛 ?????以?理쒕뙎媛믪씠 Task Manager??GPU ?섏튂
	std::unordered_map<std::wstring, double> engine_sums;
	for (DWORD i = 0; i < item_count; ++i)
	{
		const wchar_t* name = items[i].szName;
		if (name == nullptr)
			continue;

		std::wstring s(name);
		size_t pos = s.find(L"engtype_");
		if (pos == std::wstring::npos)
			continue;

		std::wstring engtype = s.substr(pos);
		double v = items[i].FmtValue.doubleValue;
		if (v < 0.0)
			v = 0.0;
		engine_sums[engtype] += v;
	}

	double max_usage = 0.0;
	for (auto& p : engine_sums)
		if (p.second > max_usage)
			max_usage = p.second;

	if (max_usage > 100.0)
		max_usage = 100.0;

	return static_cast<int>(max_usage + 0.5);
}
