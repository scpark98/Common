#include "cpu_usage.h"

static ULONGLONG _ft_to_ms(const FILETIME& ft)
{
	ULARGE_INTEGER u;
	u.LowPart = ft.dwLowDateTime;
	u.HighPart = ft.dwHighDateTime;
	return u.QuadPart / 10000;	//100-ns ticks → ms
}

CCpuUsage::CCpuUsage()
{
	FILETIME idle, kernel, user;
	if (!::GetSystemTimes(&idle, &kernel, &user))
		return;
	m_last_idle   = _ft_to_ms(idle);
	m_last_kernel = _ft_to_ms(kernel);
	m_last_user   = _ft_to_ms(user);
	m_ready = true;
}

int CCpuUsage::get_usage()
{
	if (!m_ready)
		return -1;

	FILETIME idle, kernel, user;
	if (!::GetSystemTimes(&idle, &kernel, &user))
		return -1;

	ULONGLONG cur_idle   = _ft_to_ms(idle);
	ULONGLONG cur_kernel = _ft_to_ms(kernel);
	ULONGLONG cur_user   = _ft_to_ms(user);

	ULONGLONG d_idle   = cur_idle   - m_last_idle;
	ULONGLONG d_kernel = cur_kernel - m_last_kernel;
	ULONGLONG d_user   = cur_user   - m_last_user;

	m_last_idle   = cur_idle;
	m_last_kernel = cur_kernel;
	m_last_user   = cur_user;

	ULONGLONG total = d_kernel + d_user;	//kernel 에 idle 포함.
	if (total == 0)
		return 0;

	ULONGLONG busy = total - d_idle;
	int usage = (int)(busy * 100 / total);
	if (usage < 0) usage = 0;
	if (usage > 100) usage = 100;
	return usage;
}
