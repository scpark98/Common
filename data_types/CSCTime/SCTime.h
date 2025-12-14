#pragma once

#include <afxwin.h>
#include <afxdtctl.h>

/*
- 기존 CTime은 ms가 없고
  SYSTEMTIME은 ms가 있으나 연산자 오버로딩이 되어있지 않아 불편하다.
  또한 멤버가 모두 WORD type이므로 음수의 시간값을 저장할 수 없다.
  두 시간 차이를 계산할 때 음수가 발생할 수 있으므로 음수값을 저장할 수 있어야 한다.
  즉, CTime, SYSTEMTIME과 같이 실제 시간을 저장할 수도 있고 CTimeSpan과 같이 차이값을 저장할 수도 있어야 한다.

- 연산자 오버로딩을 통해 더 편리하게 사용할 수 있도록 한다.
*/

class CSCTime
{
public:
	CSCTime(int _year = 0, int _month = 0, int _day = 0, int _hour = 0, int _minute = 0, int _second = 0, int _millisecond = 0);
	CSCTime(const CSCTime& t);
	CSCTime(const SYSTEMTIME& t);
	CSCTime(CString str);
	CSCTime operator+(const CSCTime& t);
	CSCTime operator-(const CSCTime& t);
	CSCTime& operator+=(const CSCTime& t);
	CSCTime& operator-=(const CSCTime& t);
	bool operator<(const CSCTime& t) const;
	bool operator<=(const CSCTime& t) const;
	bool operator>(const CSCTime& t) const;
	bool operator>=(const CSCTime& t) const;
	bool operator==(const CSCTime& t) const;
	bool operator!=(const CSCTime& t) const;
	SYSTEMTIME	to_SYSTEMTIME() const;
	void		get_time(CDateTimeCtrl& datetime_ctrl);

	CString		get_date_str(CString separator = _T("/"));
	CString		get_time_str(CString separator = _T(":"), bool include_milliseconds = true);

	void		from_string(CString str);
	CString		to_string(CString date_separator = _T("/"), CString time_separator = _T(":"), bool include_milliseconds = true);

	bool		is_empty();

	void		normalize(CSCTime& t);

	void		load_profile_value(CString section, CString entry, CSCTime default_value);
	void		save_profile_value(CString section, CString entry);

	int		year = 0;
	int		month = 0;
	int		day = 0;
	int		hour = 0;
	int		minute = 0;
	int		second = 0;
	int		milliseconds = 0;
};

