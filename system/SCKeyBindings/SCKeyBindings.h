#pragma once

#include <afxwin.h>
#include <vector>

class CSCMenu;		//20260711 by claude. seed_from_scmenu 전방 선언 — 상세 include 는 SCKeyBindings.cpp.

//앱 단위 단축키(accelerator) 바인딩 매니저.
//- .rc ACCELERATORS 를 기본값으로 seed → 사용자 override(레지스트리) 적용 → 런타임 HACCEL 생성.
//- 액션은 커맨드 ID 로 식별. 메뉴/accel 과 동일한 WM_COMMAND 경로로 dispatch 되므로 포커스 무관.
//- remap 설정 UI 는 set_binding / find_conflict / build_haccel 로 구성 (엔진은 UI 비의존).
//XP 호환: LoadAccelerators / CopyAcceleratorTable / CreateAcceleratorTable 만 사용.
class CSCKeyBindings
{
public:
	struct Binding
	{
		UINT	cmd;		//WM_COMMAND ID
		BYTE	fVirt;		//FVIRTKEY | FCONTROL | FSHIFT | FALT (현재값)
		WORD	key;		//VK_* (현재값)
		BYTE	def_fVirt;	//기본값 (reset_to_default 용)
		WORD	def_key;
		CString	name;		//레지스트리/UI 안정 키. 비어있으면 .rc seed 된 익명 항목 (v1 에서 remap 비대상)
		CString	display;	//UI 표시 이름
	};

	CSCKeyBindings();
	~CSCKeyBindings();

	//.rc 의 ACCELERATORS 테이블을 기본값으로 적재. 기존 항목은 모두 지우고 재설정.
	void			seed_from_resource(UINT accel_res_id);

	//메뉴 캡션의 '\t' 뒤 단축키 표기(예: "현재 화면 복사\tCtrl+C")를 파싱해 바인딩으로 적재.
	//서브메뉴까지 재귀. 같은 cmd 가 이미 있으면 키만 갱신, 없으면 추가(name 비움 → remap 비대상).
	//.rc ACCELERATORS 를 수동 관리하지 않고 메뉴 캡션만으로 단축키를 정의하기 위한 경로.
	//owner-draw 메뉴는 캡션 문자열이 비어있을 수 있으므로, LoadMenu 직후의 표준 HMENU 를 넘길 것.
	void			seed_from_menu(HMENU hMenu);

	//20260711 by claude. CSCMenu 를 이미 load 한 프로젝트용 — 리소스를 CMenu 로 재로드하지 않고
	//m_items 의 (m_id, m_hot_key) 를 직접 시드. m_sub_menu 재귀. hot_key 파싱·dedup·등록 규칙은
	//seed_from_menu 와 동일 (내부 add_or_update_binding 공유).
	void			seed_from_scmenu(const CSCMenu* menu);

	//"Ctrl+Alt+E", "F2", "Left", "Space" 등 단축키 문자열 → (fVirt, key). 성공 시 true.
	//fVirt 는 항상 FVIRTKEY 포함. 인식 못 하는 키 이름이면 false.
	static bool		string_to_key(LPCTSTR text, BYTE& fVirt, WORD& key);

	//.rc 에 없는 신규 단축키를 named 액션으로 추가. 같은 cmd 의 named 액션이 있으면 갱신.
	//name 은 레지스트리/UI 용 안정 키 (cmd 숫자값이 바뀌어도 유지되도록).
	void			register_action(UINT cmd, LPCTSTR name, LPCTSTR display, BYTE fVirt, WORD key);

	void			set_registry_section(LPCTSTR section);	//미지정 시 _T("keybindings")
	void			load_overrides();	//named 액션의 사용자 override 를 레지스트리에서 읽어 적용
	void			save_overrides();	//현재 named 액션 바인딩을 레지스트리에 기록

	//remap (설정 UI 용). named 액션만 대상. 성공 시 true, 해당 cmd 의 named 액션이 없으면 false.
	bool			set_binding(UINT cmd, BYTE fVirt, WORD key);
	void			reset_to_default(UINT cmd);

	//(fVirt,key) 를 이미 쓰는 cmd 반환 (except_cmd 제외). 충돌 없으면 0.
	UINT			find_conflict(BYTE fVirt, WORD key, UINT except_cmd = 0) const;

	//현재 바인딩으로 HACCEL 생성. 반환 핸들의 DestroyAcceleratorTable 은 호출자 책임.
	HACCEL			build_haccel() const;

	//"Ctrl+[", "Alt+Left" 등 표시 문자열.
	static CString	key_to_string(BYTE fVirt, WORD key);

	const std::vector<Binding>&	bindings() const { return m_bindings; }

private:
	Binding*		find_named(UINT cmd);
	//20260711 by claude. seed_from_menu / seed_from_scmenu 공용 — (cmd, fVirt, key) 를 dedup 후 추가/갱신.
	void			add_or_update_binding(UINT cmd, BYTE fVirt, WORD key);

	std::vector<Binding>	m_bindings;
	CString			m_section;
};
