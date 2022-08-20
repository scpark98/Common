////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// PIDL 처리 클래스. 많은 부분을 http://www.codeproject.com/KB/shell/citemidlist.aspx 에서 따옴
/// 
/// @author   parkkh
/// @date     Friday, October 14, 2011  2:31:57 PM
/// 
/// Copyright(C) 2011 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <shlobj.h>


#ifndef SAFE_RELEASE
#	define SAFE_RELEASE(p) if(p){p->Release();p=NULL;}
#endif


class XPidl
{
public :
	XPidl();
	~XPidl();

public :			// 전역 함수 - Malloc 을 alloc/free 한다.
	static void		InitMalloc();
	static void		FreeMalloc();

public :			// 다른곳에서도 사용 가능.
	static LPMALLOC	g_malloc;

public :
	void			Clear();
	BOOL			GetSpecialFolderLocation(int csidl);
	CString			GetDispName();
	BOOL			IsEmpty() const {return m_pidl ? FALSE : TRUE;}
	void			Attach(LPITEMIDLIST pidl);
	LPITEMIDLIST	Detach();
	LPCITEMIDLIST	GetPIDL() { return m_pidl;}
	BOOL			GetPath(CString& path);
	BOOL			CreateFromFromPath(LPCTSTR lpszPath);
	UINT			GetItemCount();
	LPITEMIDLIST	Left(int count);

public :			// 연산자 오버로딩 
	const XPidl&	operator=(const XPidl& rhs);
	operator		LPITEMIDLIST(void) const{return m_pidl;}
	operator		LPCITEMIDLIST(void) const{return (LPCITEMIDLIST)m_pidl;}
	operator		LPITEMIDLIST*(void){return &m_pidl;}

public :			// static 함수들.
	static LPITEMIDLIST ConcatPidls(LPITEMIDLIST pidl1,LPITEMIDLIST pidl2);//Concat two pidl.
	static UINT			GetPidlSize(LPITEMIDLIST pidl);
	static UINT			GetPidlSize(LPITEMIDLIST pidl, int count);
	static LPITEMIDLIST	Next(LPITEMIDLIST pidl);
	static LPITEMIDLIST	CopyItemIDList(LPITEMIDLIST pidl);
	static LPITEMIDLIST CreatePidl(UINT nSize);
	static void			FreePidl(LPITEMIDLIST pidl);

private :
	LPITEMIDLIST	m_pidl;

};

