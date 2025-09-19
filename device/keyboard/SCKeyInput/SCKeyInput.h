/*https://blog.naver.com/myheredity/130160917248
- 2025.09.11 scpark �� ����Ʈ ������ �����Ͽ� �ڵ� Ű �Է� Ŭ������ ����.
- CSCKeyInput Ÿ������ ������ �����ϸ� �����ڿ��� �ڵ����� �����尡 �����Ǹ�
  add(_T("test input, �ѱ�")); �� ���� ȣ���ϸ�
  ���� ��Ŀ���� ���� �����쿡 �� �ؽ�Ʈ�� �״�� Ű����� �Է��ϴ� �Ͱ� �����ϰ� ó����.
- m_key��� queue�� ����ǹǷ� �ؽ�Ʈ�� �Է�ó���Ǵ� ���߿��� �ؽ�Ʈ �߰��� ������.

*/

#pragma once

#include <afxwin.h>
#include <deque>
#include <mutex>

class CSCKeyInput
{
public:
	CSCKeyInput(CString str = _T(""));
	~CSCKeyInput();

	void	input(CString str);
	void	input(TCHAR ch);
	int		get_key_count();

	//Ű�Է� thread�� ���� �����Ų��.
	void	stop();

protected:
	std::deque<TCHAR> m_key;

	std::mutex m_mutex;
	bool	m_thread_running = false;
	bool	m_thread_terminated = true;
	void	thread_function();

	void	PressKey(TCHAR chChar, int nDelay = 1000);
	CString	SplitHangle(TCHAR ch, bool bToEnglish);


};

