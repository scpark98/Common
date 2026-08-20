#pragma once

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>

/*
* ui_language  (헤더 온리 — 프로젝트에 추가할 .cpp 가 없다)
*
*  - 이 프로세스의 UI 를 "어느 언어로 표시할지" 확정하고, 그 언어로 스트링 테이블을 읽는다.
*  - MFC / Common 의존이 없어 순수 Win32 프로젝트(LMMHost 등)에도 이 헤더 하나만 include 하면 된다.
*  - MFC 프로젝트는 Functions.h 가 이 헤더를 include 하므로 아무것도 할 것이 없다.
*
*  왜 LoadString 을 쓰지 않는가:
*    LoadString / FindResource 는 언어를 지정하지 않으므로 OS 가 고르는데, 그 결과가 OS 버전·설치
*    형태마다 달라 통제할 수 없다. load_string() 은 FindResourceEx 에 LANGID 를 명시해 그 판단을
*    통째로 건너뛴다. 스레드 로캘을 건드리지 않으므로 ANSI 코드페이지 부작용도 없다.
*
*  정책: 표시 언어를 따른다. 지역/로캘은 원칙적으로 날짜·숫자·통화 형식용이지만,
*        표시 언어를 알 수 없는 환경에서는 로캘이 유일한 차선 신호다(아래 3번).
*
*  결정 우선순위:
*    1. 앱이 set_ui_language(lang) 로 명시한 값 (ini 설정, 사용자 선택 메뉴 등)
*    2. Vista+ : GetUserPreferredUILanguages() 의 첫 항목 (= 표시 언어)
*    3. XP     : MUI 팩이 설치되어 사용자가 표시 언어를 고른 경우에만 GetUserDefaultUILanguage().
*                고른 적이 없으면 이 API 는 "설치된 Windows 의 언어" 를 돌려줄 뿐 사용자의 의사가
*                아니므로(문서화된 동작), 사용자 로캘(GetUserDefaultLCID)의 언어를 쓴다.
*    4. 위 모두 실패하면 영어
*
*  3번의 근거 (2026-08-20 실측, Windows XP Professional SP3):
*    셸은 한국어로 보이지만 MUI 팩이 없어 HKCU\Control Panel\Desktop\MultiUILanguageId 가 없었다.
*    그래서 GetUserDefaultUILanguage() 가 설치 언어(0409)로 떨어졌고, 사용자 로캘 0412 / ANSI 949 로
*    다른 신호가 전부 한국어인데도 앱 문자열이 모두 영어로 나왔다.
*
*  사용법:
*    - 기본은 자동이다. 이 헤더를 include 한 것만으로 WinMain 보다 먼저 언어가 확정된다.
*    - 앱이 직접 정한다면 InitInstance 초반에 set_ui_language(원하는 LANGID) 를 호출한다.
*    - 자동 확정을 끄려면 프로젝트 전처리기에 SC_NO_AUTO_UI_LANGUAGE 를 정의한다.
*/

//SDK 7.1A(_USING_V110_SDK71_) 에는 없을 수 있어 로컬에서 직접 정의한다.
#ifndef MUI_LANGUAGE_ID
#define MUI_LANGUAGE_ID		0x4
#endif

namespace ui_language_detail
{
	const LANGID	language_english = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
	const LANGID	language_neutral = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

	typedef BOOL (WINAPI *pfn_GetUserPreferredUILanguages)(DWORD, PULONG, WCHAR*, PULONG);
	typedef BOOL (WINAPI *pfn_SetThreadPreferredUILanguages)(DWORD, const WCHAR*, PULONG);
	typedef BOOL (WINAPI *pfn_SetProcessPreferredUILanguages)(DWORD, const WCHAR*, PULONG);

	//전역 생성자는 DLL 문맥에서 loader lock 을 쥔 채 실행된다. 거기서 LoadLibrary 를 부르면
	//교착 위험이 있으므로, 항상 로드되어 있는 kernel32 를 GetModuleHandle 로만 잡는다.
	inline HMODULE kernel32()
	{
		static HMODULE module = ::GetModuleHandleW(L"kernel32.dll");
		return module;
	}

	inline pfn_GetUserPreferredUILanguages get_preferred()
	{
		static pfn_GetUserPreferredUILanguages pfn =
			(pfn_GetUserPreferredUILanguages)::GetProcAddress(kernel32(), "GetUserPreferredUILanguages");
		return pfn;
	}

	inline pfn_SetThreadPreferredUILanguages set_thread_preferred()
	{
		static pfn_SetThreadPreferredUILanguages pfn =
			(pfn_SetThreadPreferredUILanguages)::GetProcAddress(kernel32(), "SetThreadPreferredUILanguages");
		return pfn;
	}

	inline pfn_SetProcessPreferredUILanguages set_process_preferred()
	{
		static pfn_SetProcessPreferredUILanguages pfn =
			(pfn_SetProcessPreferredUILanguages)::GetProcAddress(kernel32(), "SetProcessPreferredUILanguages");
		return pfn;
	}

	//SetProcessPreferredUILanguages 는 프로세스 전역이다. 이 헤더가 DLL 에 링크되면 그 DLL 을 로드한
	//"남의 프로세스" 언어까지 바꾸게 되므로, 자기 모듈이 exe 가 아니면 강제하지 않는다(값 기록만).
	inline bool hosted_in_dll()
	{
		HMODULE self = NULL;

		if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				reinterpret_cast<LPCWSTR>(&kernel32), &self))
			return false;

		return self != ::GetModuleHandleW(NULL);
	}

	//XP 에서 사용자가 MUI 로 표시 언어를 고른 적이 있는지. 값이 없으면 GetUserDefaultUILanguage() 는
	//설치 언어를 돌려줄 뿐이라 사용자의 의사로 볼 수 없다.
	inline bool has_mui_selection()
	{
		HKEY key = NULL;
		if (::RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
			return false;

		LONG result = ::RegQueryValueExW(key, L"MultiUILanguageId", NULL, NULL, NULL, NULL);
		::RegCloseKey(key);

		return result == ERROR_SUCCESS;
	}

	inline LANGID& forced_language()
	{
		static LANGID lang = 0;
		return lang;
	}

	inline HINSTANCE& resource_module()
	{
		static HINSTANCE module = NULL;
		return module;
	}

	inline std::map<UINT, std::wstring>& cache()
	{
		static std::map<UINT, std::wstring> table;
		return table;
	}

	//소멸자를 두지 않는다. static 소멸 순서상 다른 스레드가 아직 쓰고 있을 수 있고,
	//프로세스 종료 시 OS 가 회수하므로 DeleteCriticalSection 을 할 이유가 없다.
	struct cache_lock_holder
	{
		CRITICAL_SECTION cs;
		cache_lock_holder() { ::InitializeCriticalSection(&cs); }
	};

	inline CRITICAL_SECTION& cache_lock()
	{
		static cache_lock_holder holder;
		return holder.cs;
	}

	//MUI_LANGUAGE_ID 형식의 목록은 "4자리 16진 LANGID" 문자열을 NULL 로 구분해 나열하고 마지막에 NULL 을 하나 더 둔다.
	inline void write_langid_hex(WCHAR* dest, LANGID lang)
	{
		static const WCHAR digits[] = L"0123456789ABCDEF";

		dest[0] = digits[(lang >> 12) & 0xF];
		dest[1] = digits[(lang >> 8) & 0xF];
		dest[2] = digits[(lang >> 4) & 0xF];
		dest[3] = digits[lang & 0xF];
	}

	//강제한 언어의 리소스가 없을 때 폴백이 .rc 의 첫 LANGUAGE 블록으로 떨어지는 것을 막으려고
	//영어를 두 번째 항목으로 붙인다. (LMMLoginManager.rc 는 일본어 블록이 먼저라 그대로 두면 일본어가 나온다.)
	//호출자는 WCHAR[16] 을 0 으로 초기화해서 넘긴다 — 구분자·종료 NULL 이 그 0 으로 채워진다.
	inline void build_language_list(LANGID lang, WCHAR* list)
	{
		write_langid_hex(&list[0], lang);

		if (lang != language_english)
			write_langid_hex(&list[5], language_english);
	}
}

//위 우선순위 2~4 로 언어를 결정해 반환한다. 상태를 바꾸지 않는다.
inline LANGID decide_ui_language()
{
	ui_language_detail::pfn_GetUserPreferredUILanguages preferred = ui_language_detail::get_preferred();

	if (preferred != NULL)
	{
		ULONG	count = 0;
		WCHAR	buffer[128] = { 0 };
		ULONG	chars = sizeof(buffer) / sizeof(buffer[0]);

		if (preferred(MUI_LANGUAGE_ID, &count, buffer, &chars) && count > 0)
			return (LANGID)wcstoul(buffer, NULL, 16);
	}

	//XP. MUI 로 고른 값이 있을 때만 표시 언어로 인정한다.
	if (ui_language_detail::has_mui_selection())
		return ::GetUserDefaultUILanguage();

	//표시 언어 신호가 없다. 사용자가 명시적으로 고른 유일한 언어 신호는 로캘뿐이다.
	LANGID from_locale = LANGIDFROMLCID(::GetUserDefaultLCID());
	if (from_locale != 0)
		return from_locale;

	return ui_language_detail::language_english;
}

//확정된 UI 언어. set_ui_language() 가 아직 불리지 않았으면 decide_ui_language() 결과를 돌려준다.
inline LANGID get_ui_language()
{
	LANGID forced = ui_language_detail::forced_language();
	if (forced != 0)
		return forced;

	return decide_ui_language();
}

//문자열 캐시를 비운다. set_ui_language() 로 언어를 바꾼 뒤 새 언어를 반영하려면 호출해야 한다.
inline void clear_string_cache()
{
	::EnterCriticalSection(&ui_language_detail::cache_lock());
	ui_language_detail::cache().clear();
	::LeaveCriticalSection(&ui_language_detail::cache_lock());
}

//UI 언어를 확정한다. lang = 0 이면 decide_ui_language() 결과를 쓴다.
//Vista+ 에서는 프로세스/스레드의 우선 UI 언어 목록에도 반영해 다이얼로그 템플릿·메뉴·MFC 기본
//리소스까지 같은 언어로 맞춘다. XP 에는 그 API 가 없어 문자열(load_string)에만 적용된다.
//반환: OS 쪽 강제까지 성공하면 true. false 여도 load_string 은 이 언어로 동작한다.
inline bool set_ui_language(LANGID lang = 0)
{
	if (lang == 0)
		lang = decide_ui_language();

	ui_language_detail::forced_language() = lang;
	clear_string_cache();

	if (ui_language_detail::hosted_in_dll())
		return false;

	WCHAR list[16] = { 0 };
	ui_language_detail::build_language_list(lang, list);

	bool applied = false;
	ULONG count = 0;

	ui_language_detail::pfn_SetProcessPreferredUILanguages process_preferred = ui_language_detail::set_process_preferred();
	if (process_preferred != NULL && process_preferred(MUI_LANGUAGE_ID, list, &count))
		applied = true;

	ui_language_detail::pfn_SetThreadPreferredUILanguages thread_preferred = ui_language_detail::set_thread_preferred();
	if (thread_preferred != NULL && thread_preferred(MUI_LANGUAGE_ID, list, &count))
		applied = true;

	return applied;
}

//리소스를 읽어올 모듈. 기본값은 GetModuleHandle(NULL)(= 실행 파일 자신).
//MFC 확장 DLL 의 리소스를 쓰는 앱은 AfxGetResourceHandle() 값을 넣어준다. 캐시도 함께 비워진다.
inline void set_resource_module(HINSTANCE module)
{
	ui_language_detail::resource_module() = module;
	clear_string_cache();
}

namespace ui_language_detail
{
	//스트링 테이블은 16개씩 묶인 블록으로 저장된다. 블록 리소스 id 는 1-based.
	//블록 내부는 [WORD 길이][길이 만큼의 WCHAR] 가 16번 반복되는 형태이고, 문자열은 널 종료가 아니다.
	inline HRSRC find_string_block(HINSTANCE module, UINT block, LANGID lang)
	{
		HRSRC found = ::FindResourceExW(module, RT_STRING, MAKEINTRESOURCEW(block), lang);
		if (found != NULL)
			return found;

		//요청한 언어에 그 블록이 없을 때. 영어 블록이 있으면 영어로, 없으면 OS 판단(중립)으로 물러선다.
		if (lang != language_english)
		{
			found = ::FindResourceExW(module, RT_STRING, MAKEINTRESOURCEW(block), language_english);
			if (found != NULL)
				return found;
		}

		return ::FindResourceExW(module, RT_STRING, MAKEINTRESOURCEW(block), language_neutral);
	}

	inline bool read_string(UINT id, LANGID lang, std::wstring& out)
	{
		HINSTANCE module = resource_module();
		if (module == NULL)
			module = (HINSTANCE)::GetModuleHandleW(NULL);

		HRSRC found = find_string_block(module, id / 16 + 1, lang);
		if (found == NULL)
			return false;

		HGLOBAL loaded = ::LoadResource(module, found);
		if (loaded == NULL)
			return false;

		const WCHAR* cursor = (const WCHAR*)::LockResource(loaded);
		if (cursor == NULL)
			return false;

		const WCHAR* end = cursor + ::SizeofResource(module, found) / sizeof(WCHAR);

		for (UINT i = 0; i < id % 16; i++)
		{
			if (cursor >= end)
				return false;

			cursor += 1 + *cursor;
		}

		if (cursor >= end)
			return false;

		const WORD length = *cursor++;
		if (cursor + length > end)
			return false;

		out.assign(cursor, length);
		return true;
	}
}

//id 에 해당하는 스트링 테이블 문자열. get_ui_language() 로 정해진 언어로 읽는다. 스레드 안전.
//요청 언어에 그 문자열이 없으면 영어 -> 중립 순으로 물러선다. 없는 ID 는 빈 문자열(널 아님).
//반환 포인터는 프로세스 수명 캐시를 가리킨다 — clear_string_cache() 를 부르면 무효화된다.
inline const TCHAR* load_string(UINT id)
{
	::EnterCriticalSection(&ui_language_detail::cache_lock());

	std::map<UINT, std::wstring>& table = ui_language_detail::cache();
	std::map<UINT, std::wstring>::iterator it = table.find(id);

	if (it == table.end())
	{
		std::wstring value;
		ui_language_detail::read_string(id, get_ui_language(), value);

		//읽기에 실패해도 빈 문자열을 캐시에 넣는다. 없는 ID 를 반복 요청할 때 매번 리소스를 뒤지지 않는다.
		it = table.insert(std::make_pair(id, value)).first;
	}

	const TCHAR* result = it->second.c_str();

	::LeaveCriticalSection(&ui_language_detail::cache_lock());

	return result;
}

//진단용. 언어·로캘·코드페이지의 현재 상태를 한 줄로 채운다. out 은 256자 이상이어야 한다.
//OS·머신마다 어떤 값이 무엇을 가리키는지 실측할 때 쓴다 — 특히 XP 는 추측이 자주 빗나간다.
inline void format_ui_language_state(TCHAR* out, int count)
{
	//스레드 로캘이 가리키는 ANSI 코드페이지. GetACP()(시스템 값) 와 어긋나면
	//narrow <-> wide 변환을 서로 다른 코드페이지로 하는 코드에서 문자가 깨진다.
	DWORD thread_acp = 0;
	::GetLocaleInfo(::GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
		(LPTSTR)&thread_acp, sizeof(thread_acp) / sizeof(TCHAR));

	_sntprintf_s(out, count, _TRUNCATE,
		_T("user_ui=%04X system_ui=%04X user_lcid=%08X thread_lcid=%08X acp=%u thread_acp=%u ")
		_T("mui_reg=%d mui_api=%d decided=%04X used=%04X"),
		::GetUserDefaultUILanguage(),
		::GetSystemDefaultUILanguage(),
		::GetUserDefaultLCID(),
		::GetThreadLocale(),
		::GetACP(),
		thread_acp,
		ui_language_detail::has_mui_selection() ? 1 : 0,
		ui_language_detail::get_preferred() != NULL ? 1 : 0,
		decide_ui_language(),
		get_ui_language());
}

//20260820 by claude. GDI+ 의 CGdiplusDummyForInitialization 과 같은 방식이되, 헤더 온리라서
//인스턴스가 TU 마다 하나씩 생긴다. 실제 확정은 카운터로 한 번만 수행한다(std::ios_base::Init 패턴).
//전역 생성자는 WinMain 보다 먼저 돌므로 어떤 문자열 조회보다도 앞선다.
//소멸자는 두지 않는다 — 되돌릴 이유가 없고 static 소멸 순서 문제만 만든다.
class CUILanguageDummyForInitialization
{
public:
	CUILanguageDummyForInitialization()
	{
		static int init_count = 0;

		if (init_count++ == 0)
			set_ui_language();
	}
};

#ifndef SC_NO_AUTO_UI_LANGUAGE
static CUILanguageDummyForInitialization ui_language_dummy_for_initialization;
#endif
