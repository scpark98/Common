// RandomText.h: interface for the RandomText class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RANDOMTEXT_H__999A1FA0_DEF8_11D8_B14D_002018574596__INCLUDED_)
#define AFX_RANDOMTEXT_H__999A1FA0_DEF8_11D8_B14D_002018574596__INCLUDED_

#include <afxwin.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class RandomText  
{
public:
	static CString GetSlogan();
	static CString GetName();

	//parts를 2로 주면 "1.103"처럼 끝 두 필드만 생성한다.
	static CString get_ip(int parts = 4);
	static CString get_mac();
	static CString extension(bool dot = false);

private:
	static int GetRand(const int top, int& last);
	static CString Name();
	static CString Size();
	static CString RandomText::Cute();
	static CString SurName();
	static CString Person(const bool itome=false);
	static CString LikeHate();
	static CString Goodie(bool plural=false);
	static CString GoodBad();
	static CString DoDont();
	static CString Happy();
	static CString Animal(bool plural);
	static void AddWord(CString& sent, const CString& word);
	static CString CommonRare();

};

#endif // !defined(AFX_RANDOMTEXT_H__999A1FA0_DEF8_11D8_B14D_002018574596__INCLUDED_)
