// ShellList.h: interface for the CShellImageList class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <afxwin.h>
#include <map>
#include <deque>

class CDiskDriveInfo
{
public:
	CDiskDriveInfo()
	{
		total_space.QuadPart = 0;
		free_space.QuadPart = 0;
	}

	CDiskDriveInfo(UINT _type, CString _label, CString _path, ULARGE_INTEGER _total_space, ULARGE_INTEGER _free_space)
	{
		type = _type;
		_stprintf_s(label, sizeof(label) / sizeof(TCHAR), _T("%s"), _label);
		_stprintf_s(path, sizeof(path) / sizeof(TCHAR), _T("%s"), _path);
		total_space = _total_space;
		free_space = _free_space;
	}

	UINT			type = DRIVE_UNKNOWN;	//DRIVE_FIXED, DRIVE_REMOTE, ...
	TCHAR			label[MAX_PATH] = { 0, };	//"로컬 디스크 (C:)"
	TCHAR			path[MAX_PATH] = { 0, };	//"C:\\"
	ULARGE_INTEGER	total_space;
	ULARGE_INTEGER	free_space;
};

//내 PC, 바탕 화면, 내 문서 등의 label과 (추후 Downloads 폴더 등도 추가 가능)
//실제 경로 및 C드라이브~등의 volume label 저장
//ex. 
class CShellVolumeList
{
public:
	CShellVolumeList();

	std::map<int, CString>* get_label_map() { return &m_label; }
	std::map<int, CString>* get_path_map() { return &m_path; }

	//"내 PC"이면 CSIDL_DRIVES를 리턴
	int			get_csidl(CString label);

	//CSIDL_DRIVES이면 "내 PC"를 리턴
	CString		get_label(int csidl);

	//csidl에 해당하는 실제 경로 리턴
	CString		get_path(int csidl);

	//label에 해당하는 실제 경로 리턴
	CString		get_path(CString label);

	void		set_system_label(std::map<int, CString>* map);
	void		set_system_path(std::map<int, CString>* map);
	//remote인 경우는 drive_list를 받아서 채워주고 NULL일 경우는 로컬의 drive list를 직접 구해서 m_drives에 저장한다.
	void		set_drive_list(std::deque<CDiskDriveInfo>* drive_list = NULL);

	//disk drive list를 리턴
	std::deque<CDiskDriveInfo>* get_drive_list() { return &m_drives; }

	//path를 주면 해당 드라이브의 label을 리턴
	CString		get_drive_volume(CString path);

	void		get_drive_space(CString path, ULARGE_INTEGER* total_space, ULARGE_INTEGER* free_space);
protected:
	std::map<int, CString> m_label;	//내 PC, 바탕 화면, 내 문서 의 CSIDL과 system label
	std::map<int, CString> m_path;	//내 PC, 바탕 화면, 내 문서 의 실제 경로
	std::deque<CDiskDriveInfo> m_drives;	//volume['C'] = "로컬 디스크",...
};

//drive_list가 local, remote에 따라 다르므로 이 또한 멤버로 추가하여 기억하도록 구현했었으나
//CShellImageList		m_shell_imagelist_local;
//CShellImageList		m_shell_imagelist_remote;
//위와 같이 두 인스턴스를 실행하면 CImageList의 Attach()에서 ASSERT FAIL이 발생한다.
//MSDN에는 한 프로세스에서는 시스템 이미지 리스트 핸들을 한번씩만 Attach()해야 하는 규칙이 있다.

class CShellImageList
{
public:
	CImageList m_imagelist_small;
	CImageList m_imagelist_large;

	CShellImageList();
	~CShellImageList();

	void Initialize();
	int  FindCache(CString szFileExt);
	void InsertCache(CString szFileExt, int ID);

	CImageList* get_imagelist(bool small_icon = true);
	
	int  GetImageListIcon(CString szPath, CString szFile);
	void GetSystemDisplayName(CString szFile, CString &szDisplayName);
	int  GetVirtualImageListIcon(CString szExt);

	//index = 0(local), 1(remote)
	int  GetSystemImageListIcon(int index, CString szFile, BOOL bDrive = TRUE);
	int  GetSystemImageListIcon(int index, int csidl, BOOL bDrive = TRUE);

	UINT	get_drive_type(int index, CString path);
	int		get_drive_index(int index, CString path);
	int		get_drive_icon(UINT drive_type);
	int		get_drive_icon(int index, CString path);

	/*
	//내 PC, 문서, 바탕 화면 등의 정해진 문자열을 저장(다국어 지원시에 특히 필요)
	//Shlobj.h에 정의된 CSIDL_로 시작되는 값을 동일하게 이용
	void set_shell_known_string(int index, int csidl, CString str);

	//OS언어에 따라, 윈도우 버전에 따라 "내 컴퓨터"는 "내 PC", "My Computer" 등 다양하므로 csidl로 구분한다.
	int get_csidl_by_shell_known_string(int index, CString label);
	CString get_shell_known_string_by_csidl(int index, int csidl);
	std::map<int, CString>* get_csidl_map(int index);


	//remote인 경우는 drive정보를 얻어와서 직접 넣어줘야 한다.
	//내 PC의 label, 바탕화면 fullpath, 문서 fullpath 또한 계속 사용되므로 리스트의 0, 1, 2번에 넣어준다.
	std::deque<CString> *get_drive_list(int index) { return &m_drive_lists[index]; }
	void	set_drive_list(int index, std::deque<CString> *drive_list);
	void	add_drive_list(std::deque<CString> *drive_list);

	//path는 반드시 "C:\Windows"와 같이 fullpath 또는 드라이브 문자 + ':'를 줘야 한다.
	CString	get_drive_volume(int index, CString path);
	*/

	//하나의 프로젝트에서 CShellImageList의 객체는 단 1개뿐이어야 한다. 2개를 선언할 경우 Attach()에서 에러가 나며 이는 MSDN이 정한 규칙이다.
	//따라서 하나의 CShellImageList의 객체를 선언해놓고 CVtListCtrl, CPathCtrl, CTreeCtrl 등에서 이 인스턴스의 포인터를 참조하여 동작하도록 되어 있다.
	//m_volume[0]은 local pc에 대한 정보를,
	//m_volume[1]부터는 remote pc들에 대한 정보가 저장된다.
	std::deque<CShellVolumeList>	m_volume;
	void		set_system_label(int index, std::map<int, CString>* map);
	void		set_system_path(int index, std::map<int, CString>* map);

	std::deque<CDiskDriveInfo>* get_drive_list(int index) { return m_volume[index].get_drive_list(); }
	void		set_drive_list(int index, std::deque<CDiskDriveInfo>* drive_list);

	CString		get_system_label(int index, int csidl);
	CString		get_system_path(int index, int csidl);

	//"내 PC\\로컬 디스크 (C:)" -> "C:\\"
	//"로컬 디스크 (C:)" -> "C:\\"
	//"문서" -> "C:\\Documents"
	//"문서\\AnySupport" -> "C:\\Documents\\AnySupport"
	//"Seagate(\\192.168.0.52) (X:)" -> "X:"	(네트워크 드라이브)
	//하위 폴더 포함 유무에 관계없이 변환.
	CString		convert_special_folder_to_real_path(int index, CString special_folder);
	//"c:\\abc\\def"				=> "로컬 디스크 (C:)\\abc\\def"
	//"C:\Users\scpark\Desktop"		=> "바탕 화면"
	//"C:\Users\scpark\Documents"	=> "문서"
	CString		convert_real_path_to_special_folder(int index, CString real_path);


	//C:\\, C:\\Program Files, C:\\Windows 등과 같은 주요 폴더는 rename, delete등의 액션을 허용하지 않아야 한다.
	//내 PC, 다운로드, 바탕 화면, 문서 등의 폴더도 허용하지 않아야 한다.
	bool		is_protected(int index, CString folder);


private:
	CStringArray m_ExtArray;
	CUIntArray   m_IDArray;


	/*
	//OS언어에 따라, 윈도우 버전에 따라 "내 컴퓨터"는 "내 PC", "This PC", "My Computer" 등
	//각각 다르게 표시되므로 csidl로 실제 label을 얻어온다.
	//local은 0번, remote0은 1번...에 저장해야 추후 remote n번까지 표현할때에도 이 클래스를 이용할 수 있다.
	std::deque<std::map<int, CString>> m_csidl_map;

	//0:내 PC의 label, 1:바탕화면 경로, 2:문서 경로, 3:C드라이브 volume, 4:D드라이브 volume...
	//CShellImageList는 하나의 인스턴스만 사용해야 하는데 local, remote의 drive_list는 다르고
	//local, remote가 아닌 여러 remote 장치가 있을 수 있으므로 이중 백터로 처리한다.
	m_drive_lists[0]
	std::deque<std::deque<CString>> m_drive_lists;
	*/
	int m_osType;
};
