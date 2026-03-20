/*
[제작 목적]
- CSCThemeDlg에서는 타이틀바 영역을 잡고 CSCSystemButtons을 추가한 후 아이콘, 타이틀까지 명시하여 타이틀바를 표현했다.
  이럴 경우 많은 부가 클래스까지 모두 포함되어야 한다.
  그렇다고 CSCThemeDlg를 안쓰고 CTitleDlg라는 클래스를 만들고 child로 추가하면 윈도우 메시지 처리가 매우 번거롭고 복잡해졌다.
  (CASeeDlg)
- 이 클래스는 CTitleDlg와 같은 목적이 아니라 CDC 또는 Gdiplus::Graphics 객체위에 타이틀바를 그리는 역할만 한다.
  즉, 별도의 컨트롤, 윈도우 컴포넌트가 아니라 그냥 그리는 역할만 한다.
  (CSCThemeDlg에서 타이틀바 영역을 잡고 타이틀바를 그렸고 버튼은 CSCSystemButtons을 사용했다)
- 아이콘, 타이틀, 시스템 버튼까지 모두 그려준다.
- 타이틀바를 잡고 이동시키거나, 창 크기를 변경시키거나, 시스템 버튼을 누르는 등의 액션은 모두 parent에서 처리한다.
  즉, 종료 버튼은 CButton 클래스가 아닌 DC에 그려진 이미지이며 이 버튼을 누름 처리는 parent에서 처리한다.
- CASeeDlg 등과 같은 메인 다이얼로그의 타이트바를 이와 같이 처리하기에는 용도에 맞지 않으며
  툴바나 메시지 팝업창 등의 타이틀바를 심플하게 표시할 수 있다.
*/

#pragma once

#include <afxwin.h>
#include "Common/SCGdiplusBitmap.h"

class CSCTitleBar
{
public:
	void			draw(CDC* pDC, CRect rc);
	void			draw(Gdiplus::Graphics* g, CRect rc);

	CString			get_title() { return m_title; }
	void			set_title(CString title) { m_title = title; }
	int				get_titlebar_height() { return m_titlebar_height; }
	void			set_titlebar_height(int height) { m_titlebar_height = height; }

protected:
	CString			m_title;
	int				m_titlebar_height = 24;
};
