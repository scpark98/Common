// CPerformance.h : header file
//

/* 시간측정을 위한 클래스
- 출처 : http://www.devpia.com/MAEUL/Contents/Detail.aspx?BoardID=51&MAEULNO=20&no=8616&page=5
- 사용법 :

CPerformance perf;

...수행 코드...
TRACE(_T("첫 번째 측정 시간은 %.5f 초입니다\n"), perf.Elapsed());

// 다시 측정할 경우
perf.Restart()
...수행 코드...
TRACE(_T("두 번째 측정 시간은 %.5f 초입니다\n"), perf.Elapsed());
*/
	
#pragma once


class CPerformance
{
public:
	CPerformance()
	{
		ZeroMemory(&m_liStartTime, sizeof(m_liStartTime));
		ZeroMemory(&m_liEndTime, sizeof(m_liEndTime));
		ZeroMemory(&m_liFrequency, sizeof(m_liFrequency));
		VERIFY(QueryPerformanceFrequency(&m_liFrequency));
		Restart();
	}

	virtual ~CPerformance()
	{
	}

	void Restart()
	{
		// 멀티 프로세서를 사용하는 PC 에서는 BIOS (basic input/output system) 또는
		// HAL (hardware abstraction layer) 의 Bug 로 인해서 어떤 프로세서가 사용될
		// 것인지 확실하지 않습니다.
		// 예를 들어 측정을 시작할 때는 첫 번째 프로세서가 사용되고 종료할 때는 세 번째
		// 프로세서가 사용된다면, 오차가 생기게 되어 정확한 측정 결과는 얻을 수 없습니다.
		// 이런 경우를 방지하기 위해서 SetThreadAffinityMask 함수를 사용해서 항상 첫 번째
		// 프로세서를 지정합니다.
		DWORD_PTR dOldMask = SetThreadAffinityMask(GetCurrentThread(), 0);
		VERIFY(QueryPerformanceCounter(&m_liStartTime));
		SetThreadAffinityMask(GetCurrentThread(), dOldMask);
	}

	double Elapsed()
	{
		DWORD_PTR dOldMask = SetThreadAffinityMask(GetCurrentThread(), 0);
		VERIFY(QueryPerformanceCounter(&m_liEndTime));
		SetThreadAffinityMask(GetCurrentThread(), dOldMask);
		return (double(m_liEndTime.QuadPart - m_liStartTime.QuadPart) / double(m_liFrequency.QuadPart));
	}

private:
	LARGE_INTEGER m_liStartTime;
	LARGE_INTEGER m_liEndTime;
	LARGE_INTEGER m_liFrequency;
};