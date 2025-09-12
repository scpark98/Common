/*https://blog.naver.com/myheredity/130160917248
- 2025.09.11 scpark 위 사이트 정보를 참조하여 자동 키 입력 클래스로 제작.
- CSCKeyInput 타입으로 변수를 선언하면 생성자에서 자동으로 쓰레드가 구동되며
  add(_T("test input, 한글")); 과 같이 호출하면
  현재 포커스를 가진 윈도우에 위 텍스트를 그대로 키보드로 입력하는 것과 동일하게 처리됨.
- m_key라는 queue에 저장되므로 텍스트가 입력처리되는 도중에도 텍스트 추가가 가능함.

*/

#pragma once

#include <afxwin.h>
#include <deque>

class CSCKeyInput
{
public:
	CSCKeyInput(CString str = _T(""));
	~CSCKeyInput();

	void	add(CString str);
	void	add(TCHAR ch);

	//키입력 thread를 정상 종료시킨다.
	void	stop();

protected:
	std::deque<TCHAR> m_key;

	bool	m_thread_running = false;
	bool	m_thread_terminated = true;
	void	thread_function();

	void	PressKey(TCHAR chChar, int nDelay = 1000);
	CString	SplitHangle(TCHAR ch, bool bToEnglish);


};

