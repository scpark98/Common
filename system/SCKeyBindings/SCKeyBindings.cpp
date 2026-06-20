// SCKeyBindings.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "SCKeyBindings.h"

CSCKeyBindings::CSCKeyBindings()
	: m_section(_T("keybindings"))
{
}

CSCKeyBindings::~CSCKeyBindings()
{
}

void CSCKeyBindings::seed_from_resource(UINT accel_res_id)
{
	m_bindings.clear();

	HACCEL h = ::LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(accel_res_id));
	if (h == NULL)
		return;

	int count = ::CopyAcceleratorTable(h, NULL, 0);
	if (count <= 0)
		return;

	std::vector<ACCEL> table(count);
	::CopyAcceleratorTable(h, table.data(), count);

	for (int i = 0; i < count; i++)
	{
		Binding b;
		b.cmd = table[i].cmd;
		b.fVirt = table[i].fVirt;
		b.key = table[i].key;
		b.def_fVirt = table[i].fVirt;
		b.def_key = table[i].key;
		m_bindings.push_back(b);
	}
}

void CSCKeyBindings::register_action(UINT cmd, LPCTSTR name, LPCTSTR display, BYTE fVirt, WORD key)
{
	Binding* existing = find_named(cmd);
	if (existing != NULL)
	{
		existing->name = name;
		existing->display = display;
		existing->def_fVirt = fVirt;
		existing->def_key = key;
		existing->fVirt = fVirt;
		existing->key = key;
		return;
	}

	Binding b;
	b.cmd = cmd;
	b.fVirt = fVirt;
	b.key = key;
	b.def_fVirt = fVirt;
	b.def_key = key;
	b.name = name;
	b.display = display;
	m_bindings.push_back(b);
}

void CSCKeyBindings::set_registry_section(LPCTSTR section)
{
	m_section = section;
}

void CSCKeyBindings::load_overrides()
{
	CWinApp* app = AfxGetApp();
	if (app == NULL)
		return;

	for (size_t i = 0; i < m_bindings.size(); i++)
	{
		Binding& b = m_bindings[i];
		if (b.name.IsEmpty())
			continue;

		int v = app->GetProfileInt(m_section, b.name, -1);
		if (v < 0)
			continue;

		b.fVirt = (BYTE)((v >> 16) & 0xFF);
		b.key = (WORD)(v & 0xFFFF);
	}
}

void CSCKeyBindings::save_overrides()
{
	CWinApp* app = AfxGetApp();
	if (app == NULL)
		return;

	for (size_t i = 0; i < m_bindings.size(); i++)
	{
		const Binding& b = m_bindings[i];
		if (b.name.IsEmpty())
			continue;

		int v = ((int)b.fVirt << 16) | (int)b.key;
		app->WriteProfileInt(m_section, b.name, v);
	}
}

bool CSCKeyBindings::set_binding(UINT cmd, BYTE fVirt, WORD key)
{
	Binding* b = find_named(cmd);
	if (b == NULL)
		return false;

	b->fVirt = fVirt;
	b->key = key;
	return true;
}

void CSCKeyBindings::reset_to_default(UINT cmd)
{
	Binding* b = find_named(cmd);
	if (b == NULL)
		return;

	b->fVirt = b->def_fVirt;
	b->key = b->def_key;
}

UINT CSCKeyBindings::find_conflict(BYTE fVirt, WORD key, UINT except_cmd) const
{
	for (size_t i = 0; i < m_bindings.size(); i++)
	{
		const Binding& b = m_bindings[i];
		if (b.cmd == except_cmd)
			continue;
		if (b.fVirt == fVirt && b.key == key)
			return b.cmd;
	}
	return 0;
}

HACCEL CSCKeyBindings::build_haccel() const
{
	if (m_bindings.empty())
		return NULL;

	std::vector<ACCEL> table(m_bindings.size());
	for (size_t i = 0; i < m_bindings.size(); i++)
	{
		table[i].fVirt = m_bindings[i].fVirt;
		table[i].key = m_bindings[i].key;
		table[i].cmd = (WORD)m_bindings[i].cmd;
	}

	return ::CreateAcceleratorTable(table.data(), (int)table.size());
}

CString CSCKeyBindings::key_to_string(BYTE fVirt, WORD key)
{
	CString s;

	if (fVirt & FCONTROL)	s += _T("Ctrl+");
	if (fVirt & FALT)		s += _T("Alt+");
	if (fVirt & FSHIFT)		s += _T("Shift+");

	UINT scan = ::MapVirtualKey(key, MAPVK_VK_TO_VSC);
	LONG lparam = (LONG)(scan << 16);

	//확장 키 (네비게이션/편집/넘버패드 일부) 는 extended bit 가 없으면 GetKeyNameText 가 엉뚱한 이름을 반환.
	switch (key)
	{
		case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
		case VK_PRIOR: case VK_NEXT: case VK_HOME: case VK_END:
		case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
			lparam |= (1 << 24);
			break;
		default:
			break;
	}

	TCHAR name[64] = { 0 };
	if (::GetKeyNameText(lparam, name, _countof(name)) > 0)
		s += name;
	else
		s.AppendFormat(_T("VK_%02X"), key);

	return s;
}

//단일 키 이름 토큰(모디파이어 제외) → VK. 인식 못 하면 0.
static WORD key_name_to_vk(const CString& token)
{
	CString t = token;
	t.Trim();
	if (t.IsEmpty())
		return 0;

	//단일 문자: 영문/숫자는 VK = 대문자 ASCII. 그 외 기호는 VkKeyScan 으로.
	if (t.GetLength() == 1)
	{
		TCHAR c = t[0];
		if (c >= _T('a') && c <= _T('z'))
			c = (TCHAR)(c - _T('a') + _T('A'));
		if ((c >= _T('A') && c <= _T('Z')) || (c >= _T('0') && c <= _T('9')))
			return (WORD)c;

		SHORT sk = ::VkKeyScan(c);
		if (sk != -1)
			return (WORD)(sk & 0xFF);
		return 0;
	}

	CString u = t;
	u.MakeUpper();

	//F1 ~ F24
	if (u[0] == _T('F') && u.GetLength() >= 2 && u.GetLength() <= 3)
	{
		bool all_digit = true;
		for (int i = 1; i < u.GetLength(); i++)
		{
			if (!_istdigit(u[i]))
			{
				all_digit = false;
				break;
			}
		}
		if (all_digit)
		{
			int n = _ttoi(u.Mid(1));
			if (n >= 1 && n <= 24)
				return (WORD)(VK_F1 + n - 1);
		}
	}

	//넘버패드 — "Numpad +", "Numpad -", "Numpad *", "Numpad /", "Numpad .", "Numpad 0"~"Numpad 9".
	//숫자열/기호열의 같은 글자와는 다른 VK (VK_ADD/VK_SUBTRACT/VK_NUMPAD0.. 등) 라 별도 매핑이 필요하다.
	if (u.Find(_T("NUMPAD")) == 0)
	{
		CString pad = u.Mid(6);
		pad.Trim();
		if (pad.GetLength() == 1)
		{
			TCHAR c = pad[0];
			if (c >= _T('0') && c <= _T('9'))
				return (WORD)(VK_NUMPAD0 + (c - _T('0')));

			switch (c)
			{
				case _T('+'):
					return VK_ADD;
				case _T('-'):
					return VK_SUBTRACT;
				case _T('*'):
					return VK_MULTIPLY;
				case _T('/'):
					return VK_DIVIDE;
				case _T('.'):
					return VK_DECIMAL;
				default:
					break;
			}
		}
	}

	static const struct { LPCTSTR name; WORD vk; } map[] =
	{
		{ _T("LEFT"),		VK_LEFT },		{ _T("RIGHT"),		VK_RIGHT },
		{ _T("UP"),			VK_UP },		{ _T("DOWN"),		VK_DOWN },
		{ _T("SPACE"),		VK_SPACE },
		{ _T("ENTER"),		VK_RETURN },	{ _T("RETURN"),		VK_RETURN },
		{ _T("ESC"),		VK_ESCAPE },	{ _T("ESCAPE"),		VK_ESCAPE },
		{ _T("TAB"),		VK_TAB },
		{ _T("BACKSPACE"),	VK_BACK },		{ _T("BACK"),		VK_BACK },
		{ _T("DEL"),		VK_DELETE },	{ _T("DELETE"),		VK_DELETE },
		{ _T("INS"),		VK_INSERT },	{ _T("INSERT"),		VK_INSERT },
		{ _T("HOME"),		VK_HOME },		{ _T("END"),		VK_END },
		{ _T("PGUP"),		VK_PRIOR },		{ _T("PAGEUP"),		VK_PRIOR },
		{ _T("PRIOR"),		VK_PRIOR },
		{ _T("PGDN"),		VK_NEXT },		{ _T("PAGEDOWN"),	VK_NEXT },
		{ _T("NEXT"),		VK_NEXT },
		{ _T("PLUS"),		VK_OEM_PLUS },	{ _T("MINUS"),		VK_OEM_MINUS },
		{ _T("COMMA"),		VK_OEM_COMMA },	{ _T("PERIOD"),		VK_OEM_PERIOD },
	};

	for (int i = 0; i < _countof(map); i++)
	{
		if (u == map[i].name)
			return map[i].vk;
	}

	return 0;
}

bool CSCKeyBindings::string_to_key(LPCTSTR text, BYTE& fVirt, WORD& key)
{
	fVirt = FVIRTKEY;
	key = 0;

	CString s(text);
	s.Trim();
	if (s.IsEmpty())
		return false;

	//선두의 모디파이어 토큰을 반복 제거 (Ctrl/Control/Alt/Shift, 대소문자 무관).
	for (;;)
	{
		CString up = s;
		up.MakeUpper();

		if (up.Find(_T("CTRL+")) == 0)			{ fVirt |= FCONTROL; s = s.Mid(5); }
		else if (up.Find(_T("CONTROL+")) == 0)	{ fVirt |= FCONTROL; s = s.Mid(8); }
		else if (up.Find(_T("ALT+")) == 0)		{ fVirt |= FALT;     s = s.Mid(4); }
		else if (up.Find(_T("SHIFT+")) == 0)	{ fVirt |= FSHIFT;   s = s.Mid(6); }
		else
			break;

		s.TrimLeft();
	}

	WORD vk = key_name_to_vk(s);
	if (vk == 0)
		return false;

	key = vk;
	return true;
}

void CSCKeyBindings::seed_from_menu(HMENU hMenu)
{
	if (hMenu == NULL)
		return;

	int count = ::GetMenuItemCount(hMenu);
	for (int i = 0; i < count; i++)
	{
		HMENU hSub = ::GetSubMenu(hMenu, i);
		if (hSub != NULL)
		{
			seed_from_menu(hSub);
			continue;
		}

		UINT id = ::GetMenuItemID(hMenu, i);
		if (id == 0 || id == (UINT)-1)		//separator(0) / 무효
			continue;

		TCHAR buf[256] = { 0 };
		MENUITEMINFO mii = { sizeof(mii) };
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = buf;
		mii.cch = _countof(buf);
		if (!::GetMenuItemInfo(hMenu, i, TRUE, &mii) || buf[0] == 0)
			continue;

		CString caption(buf);
		int tab = caption.Find(_T('\t'));
		if (tab < 0)
			continue;

		CString accel_text = caption.Mid(tab + 1);
		accel_text.Trim();
		if (accel_text.IsEmpty())
			continue;

		BYTE fVirt = 0;
		WORD key = 0;
		if (!string_to_key(accel_text, fVirt, key))
			continue;

		//메뉴 캡션이 이 chord(fVirt+key)를 명시했으면, 같은 chord 를 갖던 다른 cmd 의 (resource seed 단계의)
		//바인딩은 무효다. 제거하지 않으면 build_haccel 의 table 에 동일 chord 가 둘 생겨 TranslateAccelerator
		//가 먼저 등록된 cmd(대개 핸들러 없는 .rc 잔재)를 골라 정작 메뉴 명령이 호출되지 않는다.
		for (size_t k = 0; k < m_bindings.size(); )
		{
			if (m_bindings[k].cmd != id && m_bindings[k].fVirt == fVirt && m_bindings[k].key == key)
				m_bindings.erase(m_bindings.begin() + k);
			else
				k++;
		}

		//같은 cmd 가 이미 있으면 키만 갱신(name 보존 → register_action 으로 named 화된 항목도 유지), 없으면 추가.
		bool found = false;
		for (size_t k = 0; k < m_bindings.size(); k++)
		{
			if (m_bindings[k].cmd == id)
			{
				m_bindings[k].fVirt = fVirt;
				m_bindings[k].key = key;
				m_bindings[k].def_fVirt = fVirt;
				m_bindings[k].def_key = key;
				found = true;
				break;
			}
		}
		if (!found)
		{
			Binding b;
			b.cmd = id;
			b.fVirt = fVirt;
			b.key = key;
			b.def_fVirt = fVirt;
			b.def_key = key;
			m_bindings.push_back(b);
		}
	}
}

CSCKeyBindings::Binding* CSCKeyBindings::find_named(UINT cmd)
{
	for (size_t i = 0; i < m_bindings.size(); i++)
	{
		if (!m_bindings[i].name.IsEmpty() && m_bindings[i].cmd == cmd)
			return &m_bindings[i];
	}
	return NULL;
}
