#include "SCTime.h"
#include <regex>
#include "../../Functions.h"

CSCTime::CSCTime(int _year, int _month, int _day, int _hour, int _minute, int _second, int _millisecond)
{
	year = _year;
	month = _month;
	day = _day;
	hour = _hour;
	minute = _minute;
	second = _second;
	milliseconds = _millisecond;
}

CSCTime::CSCTime(const CSCTime& t)
{
	year = t.year;
	month = t.month;
	day = t.day;
	hour = t.hour;
	minute = t.minute;
	second = t.second;
	milliseconds = t.milliseconds;
}

CSCTime::CSCTime(const SYSTEMTIME& t)
{
	year = t.wYear;
	month = t.wMonth;
	day = t.wDay;
	hour = t.wHour;
	minute = t.wMinute;
	second = t.wSecond;
	milliseconds = t.wMilliseconds;
}

//"yyyy-MM-dd HH:mm:ss.SSS" 형태의 패턴을 찾아 파싱한다.
CSCTime::CSCTime(CString str)
{
	//str이 최소 "yyyyMMddHHmmss" 14글자보다 작으면 파싱 불가
	if (str.GetLength() < 14)
		return;

	bool has_millisecond = true;
	std::regex datetime_pattern("(\\d{4})[-/](\\d{2})[-/](\\d{2}) (\\d{2}):(\\d{2}):(\\d{2}).(\\d{3})");

	/*
	if (str.GetLength() > 4 && str[str.GetLength() - 4] == '.')
	{
		has_millisecond = true;
		datetime_pattern = ("(\\d{4})[-/](\\d{2})[-/](\\d{2}) (\\d{2}):(\\d{2}):(\\d{2}).(\\d{3})");
	}
	else
	{
		datetime_pattern = ("(\\d{4})[-/](\\d{2})[-/](\\d{2}) (\\d{2}):(\\d{2}):(\\d{2})");
	}
	*/

	std::smatch matches;
	std::string ssline = CString2string(str);

	if (!std::regex_search(ssline, matches, datetime_pattern))
	{
		datetime_pattern = ("(\\d{4})[-/](\\d{2})[-/](\\d{2}) (\\d{2}):(\\d{2}):(\\d{2})");
		if (!std::regex_search(ssline, matches, datetime_pattern))
		{
			//timestamp pattern not found
			return;
		}

		has_millisecond = false;
	}

	CString matched_str = CString(matches[0].str().c_str());

	matched_str.Remove('-');
	matched_str.Remove('/');
	matched_str.Remove(' ');
	matched_str.Remove(':');
	matched_str.Remove('.');

	if (has_millisecond)
		ASSERT(matched_str.GetLength() == 17);
	else
		ASSERT(matched_str.GetLength() == 14);

	//yyyyMMddHHmmss
	year	= _tstoi(matched_str.Left(4));
	month	= _tstoi(matched_str.Mid(4, 2));
	day		= _tstoi(matched_str.Mid(6, 2));
	hour	= _tstoi(matched_str.Mid(8, 2));
	minute	= _tstoi(matched_str.Mid(10, 2));
	second	= _tstoi(matched_str.Mid(12, 2));

	if (has_millisecond)
		milliseconds = _tstoi(matched_str.Right(3));
	else
		milliseconds = 0;
}

SYSTEMTIME CSCTime::to_SYSTEMTIME() const
{
	SYSTEMTIME t;
	ZeroMemory(&t, sizeof(SYSTEMTIME));

	t.wYear = (WORD)year;
	t.wMonth = (WORD)month;
	t.wDay = (WORD)day;
	t.wHour = (WORD)hour;
	t.wMinute = (WORD)minute;
	t.wSecond = (WORD)second;
	t.wMilliseconds = (WORD)milliseconds;
	return t;
}

void CSCTime::get_time(CDateTimeCtrl& datetime_ctrl)
{
	SYSTEMTIME st;
	datetime_ctrl.GetTime(&st);

	year = st.wYear;
	month = st.wMonth;
	day = st.wDay;
	hour = st.wHour;
	minute = st.wMinute;
	second = st.wSecond;
	milliseconds = 0;
}

void CSCTime::load_profile_value(CString section, CString entry, CSCTime default_value)
{
	CWinApp* pApp = AfxGetApp();
	CSCTime* t;
	UINT size = sizeof(CSCTime);

	pApp->GetProfileBinary(section, entry, reinterpret_cast<LPBYTE*>(&t), &size);

	if (t && (size == sizeof(CSCTime)))
	{
		year = t->year;
		month = t->month;
		day = t->day;
		hour = t->hour;
		minute = t->minute;
		second = t->second;
		milliseconds = t->milliseconds;
	}

	delete t;
}

void CSCTime::save_profile_value(CString section, CString entry)
{
	CWinApp* pApp = AfxGetApp();
	pApp->WriteProfileBinary(section, entry, (LPBYTE)this, sizeof(CSCTime));
}

CSCTime CSCTime::operator+(const CSCTime& t0)
{
	CSCTime t = *this;

	t.year += t0.year;
	t.month += t0.month;
	t.day += t0.day;
	t.hour += t0.hour;
	t.minute += t0.minute;
	t.second += t0.second;
	t.milliseconds += t0.milliseconds;

	normalize(t);

	return t;
}

CSCTime CSCTime::operator-(const CSCTime& t0)
{
	CSCTime t = *this;

	t.year -= t0.year;
	t.month -= t0.month;
	t.day -= t0.day;
	t.hour -= t0.hour;
	t.minute -= t0.minute;
	t.second -= t0.second;
	t.milliseconds -= t0.milliseconds;

	normalize(t);

	return t;
}

bool CSCTime::operator<(const CSCTime& t) const
{
	if (year != t.year)
		return year < t.year;
	if (month != t.month)
		return month < t.month;
	if (day != t.day)
		return day < t.day;
	if (hour != t.hour)
		return hour < t.hour;
	if (minute != t.minute)
		return minute < t.minute;
	if (second != t.second)
		return second < t.second;
	return milliseconds < t.milliseconds;
}

bool CSCTime::operator<=(const CSCTime& t) const
{
	if (year != t.year)
		return year <= t.year;
	if (month != t.month)
		return month <= t.month;
	if (day != t.day)
		return day <= t.day;
	if (hour != t.hour)
		return hour <= t.hour;
	if (minute != t.minute)
		return minute <= t.minute;
	if (second != t.second)
		return second <= t.second;
	return milliseconds <= t.milliseconds;
}

bool CSCTime::operator>(const CSCTime& t) const
{
	if (year != t.year)
		return year > t.year;
	if (month != t.month)
		return month > t.month;
	if (day != t.day)
		return day > t.day;
	if (hour != t.hour)
		return hour > t.hour;
	if (minute != t.minute)
		return minute > t.minute;
	if (second != t.second)
		return second > t.second;
	return milliseconds > t.milliseconds;
}

bool CSCTime::operator>=(const CSCTime& t) const
{
	if (year != t.year)
		return year >= t.year;
	if (month != t.month)
		return month >= t.month;
	if (day != t.day)
		return day >= t.day;
	if (hour != t.hour)
		return hour >= t.hour;
	if (minute != t.minute)
		return minute >= t.minute;
	if (second != t.second)
		return second >= t.second;
	return milliseconds >= t.milliseconds;
}

bool CSCTime::operator==(const CSCTime& t)
{
	CSCTime subtract = *this - t;
	if (subtract.is_empty())
		return true;
	return false;
}

bool CSCTime::operator!=(const CSCTime& t)
{
	CSCTime subtract = *this - t;
	if (subtract.is_empty())
		return false;
	return true;
}

bool CSCTime::is_empty()
{
	return (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0 && milliseconds == 0);
}

void CSCTime::normalize(CSCTime& t)
{
	if (t.milliseconds >= 1000)
	{
		t.second += (t.milliseconds / 1000);
		t.milliseconds %= 1000;
	}
	else if (t.milliseconds < 0)
	{
		t.second--;;
		t.milliseconds += 1000;
	}

	if (t.second >= 60)
	{
		t.minute += (t.second / 60);
		t.second %= 60;
	}
	else if (t.second < 0)
	{
		t.minute--;
		t.second += 60;
	}

	if (t.minute >= 60)
	{
		t.hour += (t.minute / 60);
		t.minute %= 60;
	}
	else if (t.minute < 0)
	{
		t.hour--;
		t.minute += 60;
	}

	if (t.hour >= 24)
	{
		t.day += (t.hour / 24);
		t.hour %= 24;
	}
	else if (t.hour < 0)
	{
		t.day--;
		t.hour += 60;
	}

	if (t.day < 0)
	{
		t.month--;
		if (t.month > 0)
			t.day = GetDaysOfMonth(t.year, t.month);
	}

	int max_days = GetDaysOfMonth(t.year, t.month);
	while (t.day > max_days)
	{
		t.month += (day / max_days);
		t.day %= max_days;
		max_days = GetDaysOfMonth(t.year, t.month);
	}

	if (t.month > 12)
	{
		t.year += (t.year / 12);
		t.hour %= 12;

		int max_days = GetDaysOfMonth(t.year, t.month);
		while (t.day > max_days)
		{
			t.month += (t.day / max_days);
			t.day %= max_days;
			max_days = GetDaysOfMonth(t.year, t.month);
		}
	}
}

CString CSCTime::get_date_str(CString separator)
{
	CString str;
	str.Format(_T("%04d%s%02d%s%02d"), year, separator, month, separator, day);
	return str;
}

CString CSCTime::get_time_str(CString separator, bool include_milliseconds)
{
	CString str;
	str.Format(_T("%02d%s%02d%s%02d"), hour, separator, minute, separator, second);

	if (include_milliseconds)
		str = str + _T(".") + i2S(milliseconds, false, true, 3);

	return str;
}

void CSCTime::from_string(CString str)
{
	*this = CSCTime(str);
}

CString CSCTime::to_string(CString date_separator, CString time_separator, bool include_milliseconds)
{
	return get_date_str(date_separator) + _T(" ") + get_time_str(time_separator, include_milliseconds);
}
