#pragma once

/*
- ScintillaCtrl.cpp, ScintillaCtrl.h, Scintilla_init.h 3개의 파일을 프로젝트에 추가
- 프로젝트 속성 -> VC++ 디렉터리 -> 포함 디렉터리에 scintilla, lexilla의 include 폴더 전체경로를 추가
  (ex. D:\1.Projects_C++;D:\1.Projects_C++\Common\CEdit\Scintilla\scintilla\include;D:\1.Projects_C++\Common\CEdit\Scintilla\lexilla\include;)
- mainDlg.h에 #include "Common/CEdit/Scintilla/ScintillaCtrl.h"
- using namespace Scintilla;
- mainDlg.cpp의 OnInitDialog()에서 초기화 함수 호출. init_lexilla_module(CString exe_directory);

- Scintilla API : https://www.scintilla.org/PaneAPI.html

*/

#include <ILexer.h>
#include <Lexilla.h>
#include <SciLexer.h>
#include <Scintilla.h>
#include <ILoader.h>
#include <afxwin.h>

Scintilla::ILexer5* m_pCLexer;
//HINSTANCE			m_hSciDll;

Lexilla::CreateLexerFn m_pCreateLexer{ nullptr };
HINSTANCE m_hScintilla{ nullptr };
HINSTANCE m_hLexilla{ nullptr };

void init_lexilla_module(CString exe_directory)
{
	CString sMsg;

	m_hLexilla = ::LoadLibrary(exe_directory + _T("Lexilla.dll"));
	if (m_hLexilla == nullptr)
	{
		sMsg.Format(_T("Fail to load Lexilla.dll"));
		AfxMessageBox(sMsg);
		return;
	}

	m_hScintilla = ::LoadLibrary(exe_directory + _T("Scintilla.dll"));
	if (m_hScintilla == nullptr)
	{
		sMsg.Format(_T("Fail to load Scintilla.dll"));
		return;
	}

	//Create the C++ Lexer from Lexilla
#pragma warning(suppress: 26490)
	m_pCreateLexer = reinterpret_cast<Lexilla::CreateLexerFn>(GetProcAddress(m_hLexilla, LEXILLA_CREATELEXER)); //NOLINT(clang-diagnostic-cast-function-type-mismatch)
	if (m_pCreateLexer == nullptr)
	{
		AfxMessageBox(_T("Could not find the Lexilla CreateLexer function"));
		return;
	};

	m_pCLexer = m_pCreateLexer("cpp");
}
