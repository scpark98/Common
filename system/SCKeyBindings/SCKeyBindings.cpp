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

CSCKeyBindings::Binding* CSCKeyBindings::find_named(UINT cmd)
{
	for (size_t i = 0; i < m_bindings.size(); i++)
	{
		if (!m_bindings[i].name.IsEmpty() && m_bindings[i].cmd == cmd)
			return &m_bindings[i];
	}
	return NULL;
}
