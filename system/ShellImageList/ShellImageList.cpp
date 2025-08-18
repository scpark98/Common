 // ShellList.cpp: implementation of the CShellImageList class.
//
//////////////////////////////////////////////////////////////////////

#include "ShellImageList.h"
#include <ShlObj.h>
#include "../../Functions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CShellVolumeList::CShellVolumeList()
{
	//local일 경우는 직접 얻어오지만 remote일 경우는 받아와서 채워줘야 한다.
	m_label.insert(std::pair<int, CString>(CSIDL_DRIVES, get_system_label(CSIDL_DRIVES)));
	m_label.insert(std::pair<int, CString>(CSIDL_DESKTOP, get_system_label(CSIDL_DESKTOP)));
	m_label.insert(std::pair<int, CString>(CSIDL_MYDOCUMENTS, get_system_label(CSIDL_MYDOCUMENTS)));

	//"내 PC"는 가상 폴더이므로 실제 경로는 없다. 하지만 ""로 처리할 경우 실제 공백인 경로와 구분되지 않으므로 경로 또한 레이블과 동일하게 "내 PC"로 설정한다.
	m_path.insert(std::pair<int, CString>(CSIDL_DRIVES, get_system_label(CSIDL_DRIVES)));
	m_path.insert(std::pair<int, CString>(CSIDL_DESKTOP, get_known_folder(CSIDL_DESKTOP)));
	m_path.insert(std::pair<int, CString>(CSIDL_MYDOCUMENTS, get_known_folder(CSIDL_MYDOCUMENTS)));

	//std::deque<CDiskDriveInfo> drive_list;
	//::get_drive_list(&drive_list);
	set_drive_list();
}

int CShellVolumeList::get_csidl(CString label)
{
	std::map<int, CString>::iterator it;

	for (it = m_label.begin(); it != m_label.end(); it++)
	{
		if (it->second == label)
			return it->first;
	}

	return -1;
}

CString CShellVolumeList::get_label(int csidl)
{
	if (m_label.find(csidl) != m_label.end())
		return m_label[csidl];

	return _T("");
}

//csidl에 해당하는 실제 경로 리턴
CString CShellVolumeList::get_path(int csidl)
{
	if (m_path.find(csidl) != m_path.end())
		return m_path[csidl];

	return _T("");
}

//label에 해당하는 실제 경로 리턴
CString CShellVolumeList::get_path(CString label)
{
	int csidl = get_csidl(label);
	return get_path(csidl);
}

CString CShellVolumeList::get_drive_volume(CString path)
{
	int pos = path.Find(':');
	if (pos < 0)
		return _T("");

	CString drive_letter = path.Mid(pos - 1, 2);

	for (int i = 0; i < m_drives.size(); i++)
	{
		if (_tcsstr(m_drives[i].path, drive_letter) != NULL)
			return m_drives[i].label;
	}

	return _T("");
}

void CShellVolumeList::get_drive_space(CString path, ULARGE_INTEGER* total_space, ULARGE_INTEGER* free_space)
{
	for (auto drive : m_drives)
	{
		if (CString(drive.path).Find(path) >= 0)
		{
			total_space->QuadPart = drive.total_space.QuadPart;
			free_space->QuadPart = drive.free_space.QuadPart;
		}
	}
}

void CShellVolumeList::set_system_label(std::map<int, CString>* map)
{
	m_label.clear();
	m_label.insert(map->begin(), map->end());
}

void CShellVolumeList::set_system_path(std::map<int, CString>* map)
{
	m_path.clear();
	m_path.insert(map->begin(), map->end());
}

void CShellVolumeList::set_drive_list(std::deque<CDiskDriveInfo>* drive_list)
{
	m_drives.clear();

	//remote인 경우는 drive_list를 받아서 채워줘야 하고
	if (drive_list)
	{
		m_drives.assign(drive_list->begin(), drive_list->end());
	}
	//local인 경우는 직접 얻어와서 채워줘야 한다.
	else
	{
		DWORD dwError = 0;
		TCHAR tzDriveString[MAX_PATH] = { 0, };
		CString strDrive;

		DWORD logicalDrives = GetLogicalDrives();
		unsigned int i = 64;

		do
		{
			i++;
			if ((logicalDrives & 1) != 0)
			{
				strDrive.Format(_T("%c:\\"), i);
				UINT driveType = GetDriveType(strDrive);

				// CD-ROM이나 floppy disk는 하위 폴더를 표시하지 않는다.
				if ((driveType == DRIVE_REMOVABLE || driveType == DRIVE_CDROM))
					continue;

				ULARGE_INTEGER total_space, free_space;
				total_space.QuadPart = get_disk_total_size(strDrive);
				free_space.QuadPart = get_disk_free_size(strDrive);
				m_drives.push_back(CDiskDriveInfo(driveType, ::get_drive_volume(strDrive[0]), strDrive, total_space, free_space));
			}
		} while ((logicalDrives >>= 1) != 0);
	}
}


CShellImageList::CShellImageList()
{
	Initialize();
}

CShellImageList::~CShellImageList()
{
	m_imagelist_small.Detach();
	m_imagelist_large.Detach();

	CoUninitialize();
}

int CShellImageList::GetSystemImageListIcon(int index, int csidl, BOOL bDrive)
{
	return GetSystemImageListIcon(index, m_volume[index].get_label(csidl), bDrive);
}

int CShellImageList::GetSystemImageListIcon(int index, CString szFile, BOOL bDrive)
{   
	SHFILEINFO shFileInfo;

	if(szFile.IsEmpty() || szFile == m_volume[index].get_label(CSIDL_DRIVES))
	{
		LPITEMIDLIST pidl_Computer = NULL;
		SHGetFolderLocation( NULL, CSIDL_DRIVES, NULL, 0, &pidl_Computer ); // 컴퓨터
		SHGetFileInfo((TCHAR*)pidl_Computer, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if(szFile == m_volume[0].get_label(CSIDL_MYDOCUMENTS))
	{
		LPITEMIDLIST pidl_Document = NULL;
		SHGetFolderLocation( NULL, CSIDL_MYDOCUMENTS, NULL, 0, &pidl_Document ); // 내문서
		SHGetFileInfo((TCHAR*)pidl_Document, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if(szFile == m_volume[0].get_label(CSIDL_DESKTOP))
	{
		LPITEMIDLIST pidl_Desktop = NULL;
		SHGetFolderLocation( NULL, CSIDL_DESKTOP, NULL, 0, &pidl_Desktop ); // 바탕화면
		SHGetFileInfo((TCHAR*)pidl_Desktop, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else
	{
		//SHGetStockIconInfo()
		//bDrive일 경우 remote의 F:\ 드라이브 아이콘을 구하려고 실제 "F:\"를 줘서는 안된다.
		//아이콘 정보는 local에서 추출하는데 local에 F드라이브가 없을 수 있다.
		//그렇다고 D라고 가정할수도 없다. D는 local drive일수도, 네트워크 드라이브일수도, 구글 드라이브일수도 있다.
		//우선 현재는 C:\\라고 가정하고 처리함.
		//확인 필요!
		//우선 bDrive일 경우는 로컬에 해당 드라이브가 없다면 DRIVE_FIXED 드라이브중의 마지막 드라이브라고 가정하고 표시한다.
		//이러한 이유로 이전 버전의 nFTD에서도 리모트의 모든 드라이브가 시스템 드라이브로 표시된다.
		if (bDrive)
		{
			if (szFile.GetLength() == 3)
			{
				//LPITEMIDLIST pidl = NULL;
				//HRESULT hr = SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl);
				//hr = SHGetFileInfo((LPCTSTR)pidl,
				//	-1,
				//	&shFileInfo,
				//	sizeof(shFileInfo),
				//	SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);

				//만약 local에 해당 드라이브가 존재하지 않는다면 C:\\라고 가정하고 표시한다.
				UINT drive_type = GetDriveType(szFile);
				if (GetDriveType(szFile) <= DRIVE_NO_ROOT_DIR)
					szFile = _T("C:\\");

				SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
			}
			else
			{
				//만약 remote라서 아이콘 정보를 얻을 수 없는 경우 기본 폴더 이미지로 표시한다.
				if (!PathFileExists(szFile))
					szFile = _T("C:\\Windows");
				SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
			}
		}
		else
		{
			SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		}
	}
	
	return shFileInfo.iIcon;
}

UINT CShellImageList::get_drive_type(int index, CString path)
{
	if (index >= (int)m_volume.size() || path.IsEmpty())
		return DRIVE_UNKNOWN;

	if (path.GetLength() == 1)
		path += _T(":\\");
	else if (path.GetLength() == 2 && path.Right(1) == ';')
		path += _T("\\");
	else
		path = path.Left(3);

	std::deque<CDiskDriveInfo>* drives = m_volume[index].get_drive_list();
	for (int i = 0; i < drives->size(); i++)
	{
		if (CString(drives->at(i).path).CompareNoCase(path) == 0)
			return drives->at(i).type;
	}

	return DRIVE_UNKNOWN;
}

int CShellImageList::get_drive_index(int index, CString path)
{
	path = normalize_drive_path(path);

	if (index >= (int)m_volume.size() || path.IsEmpty())
		return 0;

	std::deque<CDiskDriveInfo>* drives = m_volume[index].get_drive_list();
	for (int i = 0; i < drives->size(); i++)
	{
		if (CString(drives->at(i).path).CompareNoCase(path) == 0)
			return i;
	}

	return 0;
}

//shell32.dll에 있는 아이콘의 인덱스 리턴
int CShellImageList::get_drive_icon(UINT drive_type)
{
	switch (drive_type)
	{
	case DRIVE_FIXED :
		return 7;
	case DRIVE_REMOTE :
		return 9;
	case DRIVE_CDROM :
		return 11;
	case DRIVE_NO_ROOT_DIR :
		return 12;
	case DRIVE_REMOVABLE :
		return 26;
	case DRIVE_RAMDISK :
		return 12;
	}

	return 7;
}

int CShellImageList::get_drive_icon(int index, CString path)
{
	path = normalize_drive_path(path);
	if (path.IsEmpty())
		return 0;

	//C drive는 local이든 remote이든 동일하다.
	if (path.Left(3) == _T("C:\\"))
		return GetSystemImageListIcon(index, _T("C:\\"), TRUE);

	return get_drive_icon(get_drive_type(index, path));
}

void CShellImageList::GetSystemDisplayName(CString szFile, CString &szDisplayName)
{   
	SHFILEINFO shFileInfo;
	SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	//sprintf(szDisplayName, _T("%s"),shFileInfo.szDisplayName);
	szDisplayName = shFileInfo.szDisplayName;
}

void CShellImageList::Initialize()
{
	CoInitialize(NULL);

	CShellVolumeList volume;
	m_volume.push_back(volume);

	SHFILEINFO shInfo;

	//SHGetFileInfo()로 얻은 himagelist를 CImageList.Attach(himagelist)할 경우
	//CShellImageList의 다른 인스턴스에서 다시 CImageList.Attach(himagelist)하게되면 ASSERT FAIL이 발생한다.
	//하나의 프로세스에서 얻어온 himagelist는 반드시 단 한번만 Attach()해야한다.
	m_imagelist_small.Attach((HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 0, &shInfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	m_imagelist_large.Attach((HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 0, &shInfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_LARGEICON));

	m_ExtArray.RemoveAll();
	m_IDArray.RemoveAll();

	CreateDirectory(_T("c:\\nFTDTemp_qrehadfkjn\\"),NULL);
	int cache = GetSystemImageListIcon(0, "c:\\nFTDTemp_qrehadfkjn\\");
	RemoveDirectory(_T("c:\\nFTDTemp_qrehadfkjn\\"));
	InsertCache("\\", cache);
}

int CShellImageList::FindCache(CString szFileExt)
{
	int nExtSize = m_ExtArray.GetSize();
	BOOL bFound  = FALSE;
	szFileExt.MakeLower();
	int x = 0;
	for (; x<nExtSize; x++)
		if(m_ExtArray.GetAt(x) == szFileExt) break;

	if(x == nExtSize) return -1;
	return x;
}

void CShellImageList::InsertCache(CString szFileExt, int ID)
{
	szFileExt.MakeLower();
	m_ExtArray.Add(szFileExt);
	m_IDArray.Add(ID);
}

CImageList* CShellImageList::get_imagelist(bool small_icon)
{
	return (small_icon ? &m_imagelist_small : &m_imagelist_large);
}

int CShellImageList::GetImageListIcon(CString szPath, CString szFile)
{
	int cache;
	CString szExt;

	// 확장자구하기
	int pExt = szFile.ReverseFind('.');
	if(pExt == -1) szExt.Format(_T(".")); 
	else szExt = szFile.Mid(pExt + 1, szFile.GetLength()); 

	// 캐쉬에 있다면
	if((cache = FindCache(szExt)) != -1)
	{
		return m_IDArray.GetAt(cache);
	}
	
	// 캐쉬에 없다면
	cache = GetSystemImageListIcon(0, szPath);
	InsertCache(szExt, cache);
	return cache;
}

int CShellImageList::GetVirtualImageListIcon(CString szExt)
{
	int cache;

	// 캐쉬에 있다면
	if((cache = FindCache(szExt)) != -1)
	{
		return m_IDArray.GetAt(cache);
	}
	
	// 캐쉬에 없다면
	CString temp;
	temp.Format(_T("c:\\nFTDtemp.%s"),szExt);
	
	cache = GetSystemImageListIcon(0, temp, FALSE);
	InsertCache(szExt,cache);
	
	return cache;
}

void CShellImageList::set_system_label(int index, std::map<int, CString>* map)
{
	if (index >= (int)m_volume.size())
		m_volume.resize(index + 1);

	m_volume[index].set_system_label(map);
}

void CShellImageList::set_system_path(int index, std::map<int, CString>* map)
{
	if (index >= (int)m_volume.size())
		m_volume.resize(index + 1);

	m_volume[index].set_system_path(map);
}

void CShellImageList::set_drive_list(int index, std::deque<CDiskDriveInfo>* drive_list)
{
	if (index >= (int)m_volume.size())
		m_volume.resize(index + 1);

	m_volume[index].set_drive_list(drive_list);
}

CString CShellImageList::get_system_label(int index, int csidl)
{
	if (index >= (int)m_volume.size())
		return _T("");

	return m_volume[index].get_label(csidl);
}

CString CShellImageList::get_system_path(int index, int csidl)
{
	if (index >= (int)m_volume.size())
		return _T("");

	return m_volume[index].get_path(csidl);
}

//"내 PC\\로컬 디스크 (C:)" -> "C:\\"
//"로컬 디스크 (C:)" -> "C:\\"
//"문서" -> "C:\\Documents"
//"문서\\AnySupport" -> "C:\\Documents\\AnySupport"
//"Seagate(\\192.168.0.52) (X:)" -> "X:"	(네트워크 드라이브)
//하위 폴더 포함 유무에 관계없이 변환.
CString CShellImageList::convert_special_folder_to_real_path(int index, CString special_folder)
{
	if (index >= (int)m_volume.size())
		return _T("");

	//실제 존재한다고 판별되면 이는 real_path이므로 그대로 리턴.
	if (special_folder.IsEmpty() || PathFileExists(special_folder))
	{
		//"c:\\program files"로 넘어와도 실제 폴더명인 "C:\\Program Files"로 변환한다.
		if (PathFileExists(special_folder))
			special_folder = get_original_path(special_folder);
		return special_folder;
	}

	CString myPC_label;

	myPC_label = m_volume[index].get_label(CSIDL_DRIVES);

	if (special_folder == myPC_label)
		return myPC_label;

	//"내 PC" 라는 문구가 존재하면 제거한다.
	//"내 PC\\연구소문서(\\\\192.168.1.103) (Y:)" -> "\\연구소문서(\\\\192.168.1.103) (Y:)"
	if (special_folder.Find(myPC_label) >= 0)
		special_folder.Replace(myPC_label, _T(""));

	//"\\연구소문서(\\\\192.168.1.103) (Y:)"와 같이 맨 앞에 '\\'가 붙어있다면 제거.
	if (special_folder.GetLength() > 1 && special_folder.Left(1) == '\\')
		special_folder = special_folder.Mid(1);

	CString real_path = special_folder;
	CString drive_prefix;

	//"바탕 화면\\", "문서", "문서\\abc"와 같이 넘어오므로 맨 처음 항목의 실제 경로를 구해야 한다.
	//"Seagate(\\192.168.0.52) (X:)"과 같은 네트워크 드라이브도 존재하므로 '\\'로만 판별해서는 안된다.
	bool is_network_drive = false;

	int pos1 = special_folder.Find(_T(":)"));
	int pos2 = special_folder.Find(_T("\\\\"));
	if (pos1 > 0 && pos2 > 0 && pos1 > pos2)
	{
		is_network_drive = true;
		drive_prefix = special_folder.Left(special_folder.Find(_T(":)")) + 2);
	}
	else
	{
		std::deque<CString> token;
		get_token_string(special_folder, token, _T("\\"), false);
		drive_prefix = token[0];
	}

	CString rest_path = special_folder;
	rest_path.Replace(drive_prefix, _T(""));

	if (drive_prefix == m_volume[index].get_label(CSIDL_DRIVES))
		real_path.Format(_T("%s%s"), m_volume[index].get_label(CSIDL_DRIVES), rest_path);
	else if (drive_prefix == ::get_system_label(CSIDL_DESKTOP))
		real_path.Format(_T("%s%s"), m_volume[index].get_path(CSIDL_DESKTOP), rest_path);
	else if (drive_prefix == ::get_system_label(CSIDL_MYDOCUMENTS))
		real_path.Format(_T("%s%s"), m_volume[index].get_path(CSIDL_MYDOCUMENTS), rest_path);
	else
	{
		auto drive_list = m_volume[index].get_drive_list();

		for (int i = 0; i < drive_list->size(); i++)
		{
			//"로컬 디스크 (C:)"
			if (_tcsicmp(drive_list->at(i).label, drive_prefix) == 0)
			{
				int pos = real_path.Find(_T(":)"));
				if (pos < 0)
					return real_path;

				CString rest = real_path.Mid(pos + 2);
				CString drive_letter = real_path.Mid(pos - 1, 1);

				if (rest.Left(1) == _T("\\"))
					rest = rest.Mid(1);

				real_path.Format(_T("%s:\\%s"), drive_letter, rest);
				break;
			}
		}
	}

	return real_path;
}

//"c:\\abc\\def"				=> "로컬 디스크 (C:)\\abc\\def"
//"C:\Users\scpark\Desktop"		=> "바탕 화면"
//"C:\Users\scpark\Documents"	=> "문서"
CString	CShellImageList::convert_real_path_to_special_folder(int index, CString real_path)
{
	if (index >= (int)m_volume.size())
		return _T("");

	//real_path는 local에서는(index = 0일 경우) 실제로 존재해야 하는 경로이므로
	//"c:\\program files"라고 넘어와도 "C:\\Program Files"로 변환해서 비교해줘야 한다.
	if (index == 0 && PathFileExists(real_path))
	{
		//로컬이 아닌 경우는 존재여부를 검사할 수 없으므로 변환하지 않는다.
		real_path = get_original_path(real_path);
	}

	if (real_path == m_volume[index].get_path(CSIDL_DESKTOP))
		return m_volume[index].get_label(CSIDL_DESKTOP);
	else if (real_path == m_volume[index].get_path(CSIDL_MYDOCUMENTS))
		return m_volume[index].get_label(CSIDL_MYDOCUMENTS);

	CString volume_path = real_path;

	if (real_path.Mid(1, 2) != _T(":\\"))
		return real_path;

	CString volume = m_volume[index].get_drive_volume(real_path);
	volume_path.Replace(CString(real_path[0]) + _T(":"), volume);

	return volume_path;
}

//C:\\, C:\\Program Files, C:\\Windows 등과 같은 주요 폴더는 rename, delete등의 액션을 허용하지 않아야 한다.
//내 PC, 다운로드, 바탕 화면, 문서 등의 폴더도 허용하지 않아야 한다.
bool CShellImageList::is_protected(int index, CString folder)
{
	folder = convert_special_folder_to_real_path(index, folder);
	//folder.MakeLower();

	//바탕 화면, 문서 등 특수 폴더는 보호. "내 PC"의 패스는 ""으로 세팅되어 있으므로 비교 제외.
	//std::map<int, CString> path_map = *plist->m_volume[index].get_path_map();
	//if (folder.Find(path_map[CSIDL_DESKTOP]) >= 0)
	//	return true;
	//if (folder.Find(path_map[CSIDL_MYDOCUMENTS]) >= 0)
	//	return true;

	//드라이브 루트는 모두 보호.
	for (auto drive_list : *m_volume[index].get_drive_list())
	{
		//CString drive_root = convert_special_folder_to_real_path(drive_list.path, plist, index);
		if (folder.CompareNoCase(drive_list.path) == 0)
			return true;
	}

	//windows 폴더 및 하위 폴더는 모두 보호.
	if (folder.Find(_T("C:\\Windows")) >= 0)
		return true;


	std::deque<CString> protected_folder;
	protected_folder.push_back(_T("C:\\Documents and Settings"));
	protected_folder.push_back(_T("C:\\Program Files"));
	protected_folder.push_back(_T("C:\\Program Files (x86)"));
	protected_folder.push_back(_T("C:\\ProgramData"));
	protected_folder.push_back(_T("C:\\Recovery"));
	protected_folder.push_back(_T("C:\\System Volume Information"));
	protected_folder.push_back(_T("C:\\Windows"));
	protected_folder.push_back(_T("C:\\Users"));

	int found = find_dqstring(protected_folder, folder, true, false);
	if (found >= 0)
		return true;

	return false;
}
