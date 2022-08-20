#include "stdafx.h"
#include "XShellPidl.h"


LPMALLOC	XPidl::g_malloc = NULL;

void XPidl::InitMalloc()
{
	if(FAILED(SHGetMalloc(&g_malloc)))
		ASSERT(0);
}
void XPidl::FreeMalloc()
{
	SAFE_RELEASE(g_malloc);
}



XPidl::XPidl()
{
	m_pidl = NULL;
}

XPidl::~XPidl()
{
	Clear();
}

BOOL XPidl::GetSpecialFolderLocation(int csidl)
{
	Clear();

	if(FAILED(SHGetSpecialFolderLocation(NULL, csidl, &m_pidl)))
	{ASSERT(0); return FALSE;}

	return TRUE;
}

void XPidl::Clear()
{
	if(m_pidl)
	{
		// 만약 g_malloc 이 NULL 이라면 XPidl::InitMalloc() 호출해 줘야함
		g_malloc->Free(m_pidl);
		m_pidl = NULL;
	}
}

CString	XPidl::GetDispName()
{
	SHFILEINFO sfi;
	if (SHGetFileInfo((LPCTSTR) m_pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_DISPLAYNAME))
		return sfi.szDisplayName;

	return _T("???");
}

const XPidl& XPidl::operator=(const XPidl& rhs)
{
	Clear();

	if(rhs.IsEmpty()==FALSE)
		m_pidl = CopyItemIDList(rhs.m_pidl);

	return *this;
}


LPITEMIDLIST XPidl::CopyItemIDList(LPITEMIDLIST pidl)
{
	if(pidl==NULL) {ASSERT(0); return NULL;}

	UINT nSize=GetPidlSize(pidl);

	LPITEMIDLIST pidlNew=(LPITEMIDLIST)g_malloc->Alloc(nSize);
	if(pidlNew)
		memcpy(pidlNew,pidl,nSize);
	
	return pidlNew;
}

void XPidl::FreePidl(LPITEMIDLIST pidl)
{
	if(pidl==NULL) {ASSERT(0); return;}
	g_malloc->Free(pidl);
}


UINT XPidl::GetPidlSize(LPITEMIDLIST pidl)
{
	if(pidl==NULL)return 0;

	UINT nSize = sizeof(pidl->mkid.cb);
	
	while(pidl->mkid.cb)
	{
		nSize+=pidl->mkid.cb;
		pidl=Next(pidl);
	}

	return nSize;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
///         n 개의 유효한 pidl 의 크기만을 가져온다. - char* 로 치면 NULL 이 포함 안된다!
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  8:00:04 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
UINT	XPidl::GetPidlSize(LPITEMIDLIST pidl, int count)
{
	if(pidl==NULL || count<=0 )return 0;

	UINT nSize = 0;
	
	while(count && pidl->mkid.cb)
	{
		nSize+=pidl->mkid.cb;
		pidl=Next(pidl);
		count--;
	}

	return nSize;
}

LPITEMIDLIST XPidl::Next(LPITEMIDLIST pidl)
{
	LPBYTE lpMem=(LPBYTE)pidl;
	lpMem+=pidl->mkid.cb;
	return (LPITEMIDLIST)lpMem;
}


void XPidl::Attach(LPITEMIDLIST pidl)
{
	Clear();
	m_pidl=pidl;
}

LPITEMIDLIST XPidl::Detach()
{
	LPITEMIDLIST pidl=m_pidl;
	m_pidl=NULL;
	return pidl;
}


LPITEMIDLIST XPidl::ConcatPidls(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2)
{
	LPITEMIDLIST pidlNew;
	UINT cb1;
	UINT cb2;
	
	if(pidl1)
		cb1=GetPidlSize(pidl1)-sizeof(pidl1->mkid.cb);
	else
		cb1=0;
	
    cb2=GetPidlSize(pidl2);
	pidlNew=CreatePidl(cb1+cb2);
	if(pidlNew){
        if(pidl1)
			memcpy(pidlNew, pidl1, cb1);
		memcpy(((LPSTR)pidlNew)+cb1,pidl2,cb2);
    }
    return pidlNew;
}

LPITEMIDLIST XPidl::CreatePidl(UINT nSize)
{
	LPITEMIDLIST pidl=NULL;
	
    pidl=(LPITEMIDLIST)g_malloc->Alloc(nSize);
	if(pidl)
        memset(pidl,0,nSize);
	
    return pidl;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         경로 구하기.
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  11:58:31 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XPidl::GetPath(CString& path)
{
	BOOL ret = SHGetPathFromIDList(m_pidl,path.GetBuffer(MAX_PATH));
	path.ReleaseBuffer();
	return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         path 를 가지고 생성
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  5:19:03 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XPidl::CreateFromFromPath(LPCTSTR lpszPath)
{
	Clear();

	LPSHELLFOLDER pDesktopFolder;
	HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);

	if (FAILED(hr))
	{ASSERT(0); return FALSE;}

	OLECHAR olePath [MAX_PATH];

	// IShellFolder::ParseDisplayName requires the file name be in
	// Unicode.
#ifdef _UNICODE
	lstrcpy(olePath, lpszPath);
#else
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpszPath, -1, olePath, MAX_PATH);
#endif

	// Convert the path to an ITEMIDLIST.
	ULONG chEaten;
	ULONG dwAttributes;
	hr = pDesktopFolder->ParseDisplayName(NULL, NULL, olePath, &chEaten, &m_pidl, &dwAttributes);

	pDesktopFolder->Release();

//	UINT nSize=GetPidlSize(m_pidl);

	return SUCCEEDED(hr) ? TRUE : FALSE;
}

UINT XPidl::GetItemCount()
{
	if(m_pidl==NULL) return 0;

	UINT nCount = 0;
	LPITEMIDLIST pidl = m_pidl;

	for (UINT nSizeCurr = m_pidl->mkid.cb; nSizeCurr != 0; nCount++)
	{
		pidl = Next(pidl);
		nSizeCurr = pidl->mkid.cb;
	}

	return nCount;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
///         PIDL 에서 왼쪽에서 n 개만큼 떼어오기
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  7:48:22 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
LPITEMIDLIST XPidl::Left(int count)
{
	UINT nSize=GetPidlSize(m_pidl, count);

	LPITEMIDLIST pidlNew=(LPITEMIDLIST)g_malloc->Alloc(nSize + sizeof(USHORT));
	if(pidlNew)
		memcpy(pidlNew,m_pidl,nSize);

	// 마지막 cbSize 를 0 으로 막아주기.
	USHORT cb= 0;
	memcpy( ((BYTE*)pidlNew ) + nSize, &cb, sizeof(cb));

	return pidlNew;
}


