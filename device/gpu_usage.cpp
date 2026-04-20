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

	// 영문 카운터명으로 추가 → 한글 등 로케일 영향 없음
	if (PdhAddEnglishCounterW(m_query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &m_counter) != ERROR_SUCCESS)
	{
		PdhCloseQuery(m_query);
		m_query = NULL;
		return;
	}

	// 초기 수집 — 일부 카운터는 2회 수집 후부터 값이 나옴
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

	// 인스턴스명 예: "pid_1234_luid_0x00000000_0x0000ABCD_phys_0_eng_0_engtype_3D"
	// 엔진 타입별로 프로세스 사용률을 합산 → 타입 중 최댓값이 Task Manager의 GPU 수치
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
