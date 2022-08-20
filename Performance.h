// CPerformance.h : header file
//

/* �ð������� ���� Ŭ����
- ��ó : http://www.devpia.com/MAEUL/Contents/Detail.aspx?BoardID=51&MAEULNO=20&no=8616&page=5
- ���� :

CPerformance perf;

...���� �ڵ�...
TRACE(_T("ù ��° ���� �ð��� %.5f ���Դϴ�\n"), perf.Elapsed());

// �ٽ� ������ ���
perf.Restart()
...���� �ڵ�...
TRACE(_T("�� ��° ���� �ð��� %.5f ���Դϴ�\n"), perf.Elapsed());
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
		// ��Ƽ ���μ����� ����ϴ� PC ������ BIOS (basic input/output system) �Ǵ�
		// HAL (hardware abstraction layer) �� Bug �� ���ؼ� � ���μ����� ����
		// ������ Ȯ������ �ʽ��ϴ�.
		// ���� ��� ������ ������ ���� ù ��° ���μ����� ���ǰ� ������ ���� �� ��°
		// ���μ����� ���ȴٸ�, ������ ����� �Ǿ� ��Ȯ�� ���� ����� ���� �� �����ϴ�.
		// �̷� ��츦 �����ϱ� ���ؼ� SetThreadAffinityMask �Լ��� ����ؼ� �׻� ù ��°
		// ���μ����� �����մϴ�.
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