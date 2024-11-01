// ShellList.h: interface for the CShellImageList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHELLLIST_H__E0B73BFC_2F7C_44D4_BC38_1FA6A74CBD31__INCLUDED_)
#define AFX_SHELLLIST_H__E0B73BFC_2F7C_44D4_BC38_1FA6A74CBD31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxwin.h>
#include <map>
#include <deque>
/*
enum ENUM_SHELL_LIST
{
	system_label_thisPC = 0,		//�� PC
	system_label_desktop,			//���� ȭ��
	system_label_mydocuments,		//�� ����
};
*/
//�� PC, ���� ȭ��, �� ���� ���� label�� (���� Downloads ���� � �߰� ����)
//���� ��� �� C����̺�~���� volume label ����
//ex. 
class CShellVolumeList
{
public:
	CShellVolumeList();

	std::map<int, CString>* get_label_map() { return &m_label; }
	std::map<int, CString>* get_path_map() { return &m_path; }

	//"�� PC"�̸� CSIDL_DRIVES�� ����
	int			get_csidl(CString label);

	//CSIDL_DRIVES�̸� "�� PC"�� ����
	CString		get_label(int csidl);

	//csidl�� �ش��ϴ� ���� ��� ����
	CString		get_path(int csidl);

	//label�� �ش��ϴ� ���� ��� ����
	CString		get_path(CString label);

	void		set_system_label(std::map<int, CString>* map);
	void		set_system_path(std::map<int, CString>* map);
	void		set_drive_list(std::deque<CString>* drive_list);

	//disk drive list�� ����
	std::deque<CString>* get_drive_list() { return &m_drives; }

	//path�� �ָ� �ش� ����̺��� label�� ����
	CString		get_drive_volume(CString path);

protected:
	std::map<int, CString> m_label;	//�� PC, ���� ȭ��, �� ���� �� CSIDL�� system label
	std::map<int, CString> m_path;		//�� PC, ���� ȭ��, �� ���� �� ���� ���
	std::deque<CString> m_drives;	//volume['C'] = "���� ��ũ",...
};

//drive_list�� local, remote�� ���� �ٸ��Ƿ� �� ���� ����� �߰��Ͽ� ����ϵ��� �����߾�����
//CShellImageList		m_shell_imagelist_local;
//CShellImageList		m_shell_imagelist_remote;
//���� ���� �� �ν��Ͻ��� �����ϸ� CImageList�� Attach()���� ASSERT FAIL�� �߻��Ѵ�.
//�� ���μ��������� �ý��� �̹��� ����Ʈ �ڵ��� �ѹ����� Attach()�ؾ� �ϴ� ��Ģ�� �ִ�.

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

	int  GetSystemImageListIcon(CString szFile, BOOL bDrive = TRUE);
	int  GetSystemImageListIcon(int csidl, BOOL bDrive = TRUE);

	/*
	//�� PC, ����, ���� ȭ�� ���� ������ ���ڿ��� ����(�ٱ��� �����ÿ� Ư�� �ʿ�)
	//Shlobj.h�� ���ǵ� CSIDL_�� ���۵Ǵ� ���� �����ϰ� �̿�
	void set_shell_known_string(int index, int csidl, CString str);

	//OS�� ����, ������ ������ ���� "�� ��ǻ��"�� "�� PC", "My Computer" �� �پ��ϹǷ� csidl�� �����Ѵ�.
	int get_csidl_by_shell_known_string(int index, CString label);
	CString get_shell_known_string_by_csidl(int index, int csidl);
	std::map<int, CString>* get_csidl_map(int index);


	//remote�� ���� drive������ ���ͼ� ���� �־���� �Ѵ�.
	//�� PC�� label, ����ȭ�� fullpath, ���� fullpath ���� ��� ���ǹǷ� ����Ʈ�� 0, 1, 2���� �־��ش�.
	std::deque<CString> *get_drive_list(int index) { return &m_drive_lists[index]; }
	void	set_drive_list(int index, std::deque<CString> *drive_list);
	void	add_drive_list(std::deque<CString> *drive_list);

	//path�� �ݵ�� "C:\Windows"�� ���� fullpath �Ǵ� ����̺� ���� + ':'�� ��� �Ѵ�.
	CString	get_drive_volume(int index, CString path);
	*/

	//m_volume[0]�� local pc�� ���� ������,
	//m_volume[1]���ʹ� remote pc�鿡 ���� ������ ����ȴ�.
	std::deque<CShellVolumeList>	m_volume;
	void		set_system_label(int index, std::map<int, CString>* map);
	void		set_system_path(int index, std::map<int, CString>* map);
	void		set_drive_list(int index, std::deque<CString>* drive_list);
private:
	CStringArray m_ExtArray;
	CUIntArray   m_IDArray;


	/*
	//OS�� ����, ������ ������ ���� "�� ��ǻ��"�� "�� PC", "This PC", "My Computer" ��
	//���� �ٸ��� ǥ�õǹǷ� csidl�� ���� label�� ���´�.
	//local�� 0��, remote0�� 1��...�� �����ؾ� ���� remote n������ ǥ���Ҷ����� �� Ŭ������ �̿��� �� �ִ�.
	std::deque<std::map<int, CString>> m_csidl_map;

	//0:�� PC�� label, 1:����ȭ�� ���, 2:���� ���, 3:C����̺� volume, 4:D����̺� volume...
	//CShellImageList�� �ϳ��� �ν��Ͻ��� ����ؾ� �ϴµ� local, remote�� drive_list�� �ٸ���
	//local, remote�� �ƴ� ���� remote ��ġ�� ���� �� �����Ƿ� ���� ���ͷ� ó���Ѵ�.
	m_drive_lists[0]
	std::deque<std::deque<CString>> m_drive_lists;
	*/
	int m_osType;
};

#endif // !defined(AFX_SHELLLIST_H__E0B73BFC_2F7C_44D4_BC38_1FA6A74CBD31__INCLUDED_)
