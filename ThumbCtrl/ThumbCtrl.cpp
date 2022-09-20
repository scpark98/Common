// ScparkThumbCtrl.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "ThumbCtrl.h"

#include "../../Common/Functions.h"
#include "../../Common/MemoryDC.h"
#include "../../Common/GifEx.h"
#include "../messagebox/Win32InputBox/Win32InputBox.h"

#define CTHUMBNAILCONTROL_CLASSNAME _T("CThumbCtrl")

#define TEXT_HEIGHT						2

// CThumbCtrl

IMPLEMENT_DYNAMIC(CThumbCtrl, CWnd)

CThumbCtrl::CThumbCtrl()
{
	if (!RegisterWindowClass())
		return;

	m_pEdit = NULL;
	m_in_editing = false;
	m_modified = false;
	m_last_clicked = 0;

	set_color_theme(color_theme_default, false);

	m_index = 0;
	m_loading_index = -1;
	m_max_thumbs = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("max thumbs limit"), 0);
	m_szTile = CSize(128, 128);
	m_szMargin = CSize(10, 10);
	m_szGap = CSize(10, 20);

	m_show_title = true;
	m_show_resolution = true;
	//20/*m_nTitleHeight*/	= 20;

	m_show_index = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("show index"), false);
	m_index_select_after_loading = -1;
	m_has_focus = false;

	m_scroll_pos = 0;
	m_scroll_drag = false;
	m_scroll_grip_size = 40;

	m_use_popup = true;

	for (int i = 0; i < 4; i++)
	{
		m_show_info_text[i] = true;
		m_crInfoText[i] = green;// g_default_color[i];
	}

	memset(&m_lf, 0, sizeof(LOGFONT));
}

CThumbCtrl::~CThumbCtrl()
{
	release(-1);

	if (m_pEdit)
	{
		m_pEdit->DestroyWindow();
		delete m_pEdit;
	}
}

void CThumbCtrl::release(int index)
{
	for (int i = 0; i < m_dqThumb.size(); i++)
	{
		delete m_dqThumb[i];
	}

	m_dqThumb.clear();
}

//index == -1이면 전체 삭제
void CThumbCtrl::remove(int index, bool bRepaint /*= false*/)
{
	if (index < 0)
	{
		m_dqThumb.clear();
	}
	else
	{
		if (m_dqThumb[index]->full_path.IsEmpty() == false)
			DeleteFile(m_dqThumb[index]->full_path);
		m_dqThumb.erase(m_dqThumb.begin() + index);
	}


	if (bRepaint)
	{
		recalculate_scroll_size();
		Invalidate();
	}

	m_modified = true;

	//실제적인 삭제는 여기서 이루어지지만 삭제를 묻는 질문은 메인에서 해야 한다.
	//따라서 메시지도 보낼 필요가 없다.
	//::SendMessage( GetParent()->GetSafeHwnd(),	MESSAGE_THUMBCTRL,
	//	(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_remove, index), 0 );
}

void CThumbCtrl::remove_selected(bool bRepaint)
{
	int i;
	std::deque<int> selected;
	get_selected_items(&selected);
	std::sort(selected.begin(), selected.end());
	for (i = selected.size() - 1; i >= 0; i--)
	{
		remove(selected[i], false);
	}

	recalculate_scroll_size();
	Invalidate();
}

BEGIN_MESSAGE_MAP(CThumbCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_HSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_MOUSEHOVER()
	ON_WM_SETFOCUS()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KILLFOCUS()
	ON_WM_CONTEXTMENU()
	ON_COMMAND_RANGE(idToggleIndex, idRemoveAll, OnPopupMenu)
	ON_WM_VSCROLL()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CThumbCtrl::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, CTHUMBNAILCONTROL_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		CBrush	brush;
		brush.CreateSolidBrush(m_crBack);

		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground = (HBRUSH)brush.GetSafeHandle();
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = CTHUMBNAILCONTROL_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}

	}

	return TRUE;
}


// CThumbCtrl 메시지 처리기입니다.

void CThumbCtrl::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CWnd::OnPaint()을(를) 호출하지 마십시오.
	CRect	rc;
	//TRACE(_T("scrollpos = %d\n"), GetScrollPos(SB_VERT));
	//CRect	rect(m_szMargin.cx, m_szMargin.cy - GetScrollPos(SB_VERT), m_szMargin.cx + m_szTile.cx, m_szMargin.cy + m_szTile.cy - GetScrollPos(SB_VERT));
	CRect	rect(m_szMargin.cx, m_szMargin.cy + m_scroll_pos, m_szMargin.cx + m_szTile.cx, m_szMargin.cy + m_szTile.cy + m_scroll_pos);
	CString str;

	if (this == NULL)
		return;

	GetClientRect(rc);

	CMemoryDC	dc(&dc1, &rc, true);
	dc.FillSolidRect(rc, m_crBack);

	if (!m_loading_completed)
	{
		CRect r = makeCenterRect(rc.CenterPoint().x, rc.CenterPoint().y, rc.Width() / 2, 16);
		DrawSunkenRect(&dc, r);
		r.DeflateRect(1, 1);
		double width = r.Width();
#if (ADD_USING_THREAD)
		r.right = r.left + width * ((double)CheckAllThreadEnding() / (double)m_loading_files.size());
#else
		r.right = r.left + width * ((double)m_loading_index / (double)m_loading_files.size());
#endif
		dc.FillSolidRect(r, green);
		//DrawTextShadow(&dc, i2S(m_loading_complete), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		return;
	}
	else if (m_dqThumb.size() == 0)
	{
		DrawTextShadow(&dc, _T("No images to display."), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		return;
	}

	draw_function(&dc, true);
}


BOOL CThumbCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return false;
	//return CWnd::OnEraseBkgnd(pDC);
}

void CThumbCtrl::ReconstructFont()
{
	m_lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;
	m_lf.lfHeight = -16;
	m_lf.lfWeight = 500;
	_tcscpy(m_lf.lfFaceName, _T("맑은 고딕"));

	m_font.DeleteObject();
	m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);
}

void CThumbCtrl::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy(m_lf.lfFaceName, sFontname);
	ReconstructFont();
}

void CThumbCtrl::set_font_size(int nSize)
{
	m_lf.lfHeight = nSize;
	ReconstructFont();
}

void CThumbCtrl::set_font_bold(bool bBold)
{
	m_lf.lfWeight = (bBold ? FW_BOLD : FW_NORMAL);
	ReconstructFont();
}

void CThumbCtrl::PreSubclassWindow()
{
	CWnd::PreSubclassWindow();

	//자기 자신에게 부여된 폰트가 없다면 null이 리턴된다.
	//dlg의 parent의 font를 얻어와야 한다.
	CFont* font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	//m_nDefaultHeight = m_lf.lfHeight;
	ReconstructFont();

	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//ModifyStyle ( NULL, WS_VSCROLL/*WS_BORDER | WS_EX_CLIENTEDGE *//*| WS_HSCROLL |*/  );

	m_szTile.cx = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("tile size cx"), 128);
	m_szTile.cy = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("tile size cy"), 128);
}

UINT ThreadStart(LPVOID pParam)
{
	CThumbCtrl*	pMain = (CThumbCtrl*)pParam;

	//m_nDataIndex를 메인에서 증가시키면서 쓰레드를 구동하려 했으나
	//sync가 맞지 않으면 인덱스를 제대로 전달시키지 못하는 현상이 발생한다.
	//따라서 loading_function을 호출할때마다 증가시켜서 호출시키도록 하니 정상 동작했다.
	pMain->loading_function(pMain->m_nDataIndex++);
	AfxEndThread(NULL, TRUE);

	return 0;
}

void CThumbCtrl::add_files(std::deque<CString> files, bool reset)
{
	KillTimer(timer_load_files);

	if (reset)
	{
		release(-1);
	}

	m_scroll_pos = 0;

	if (files.size() == 0)
	{
		m_loading_completed = true;
		return;
	}

	m_dqThumb.resize(files.size());
	for (int i = 0; i < files.size(); i++)
	{
		m_dqThumb[i] = new CThumbImage;
		m_dqThumb[i]->title = GetFileNameFromFullPath(files[i]);
	}

	Invalidate();

	m_loading_files = files;
	m_clock_start = getClock();

	m_loading_complete.resize(m_loading_files.size());
	m_loading_complete.assign(m_loading_files.size(), false);
	m_loading_completed = false;

	//외부에서 m_thumb.add_files(...)를 호출하면
	//멀티쓰레드로 이미지들을 읽어서 썸네일 리스트를 만드는데
	//외부에서 이 로딩 쓰레드가 끝나기 전에 이미지를 접근하면 안된다.
	//아래와 끝나기를 기다려줘야 한다.
	//while (!m_thumb.is_loading_completed())
	//{
	//	Wait(10);
	//}
	//....
	//로딩이 끝나면 m_thumb[0]->mat과 같은 형식으로 이미지에 접근 가능하다.

	//timer method
#if (!ADD_USING_THREAD)
	m_loading_index = 0;
	SetTimer(timer_load_files, 10, NULL);
#else
	int i;
	/*
	[158 images]
	th=10, 10s
	th=20, 10s
	th=40, 9s
	th=100,
	* 모두 읽어들인 후 invalidate하는 방식으로 하면
	th=10, 3.0s
	th=16, 3.0s ~ 3.6s
	th=20, 2.5s
	th=32, 3.0s ~ 3.5s

	th=100, 2.5s ~ 3.5s
	*/

	if (m_loading_files.size() <= 10)
		m_nThread = 5;
	else if (m_loading_files.size() <= 100)
		m_nThread = 10;
	else
		m_nThread = m_loading_files.size() / 20;


	for (i = 0; i < MAX_THREAD; i++)
		m_bThreadConvert[i] = false;


	if (m_nThread > MAX_THREAD)
		m_nThread = MAX_THREAD;

	int each_num = m_loading_files.size() / m_nThread;

	//데이터의 개수가 MAX_THREAD 미만이면 한 쓰레드에서 하나의 데이터를 처리하고
	if (each_num == 0)
	{
		m_nThread = m_loading_files.size();

		for (i = 0; i < m_nThread; i++)
		{
			m_nStartIndex[i] = i;
			m_nEndIndex[i] = i + 1;
		}

		m_nStartIndex[i] = each_num * i;
		m_nEndIndex[i] = each_num * (i + 1);
	}
	//MAX_THREAD보다 많으면 data_num / MAX_THREAD하고 남은 개수를 각 쓰레드가 나눠가진 후 처리한다.
	else
	{
		int remain = m_loading_files.size() - each_num * m_nThread;

		for (i = 0; i < m_nThread; i++)
		{
			if (i == 0)
				m_nStartIndex[i] = 0;
			else
				m_nStartIndex[i] = m_nEndIndex[i - 1];

			if (remain > 0)
			{
				m_nEndIndex[i] = m_nStartIndex[i] + each_num + 1;
				remain--;
			}
			else
			{
				m_nEndIndex[i] = m_nStartIndex[i] + each_num;
			}
		}
	}

	CString str;
	TRACE(_T("%d images loading started with %d threads...\n\n"), m_loading_files.size(), m_nThread);
	//AfxMessageBox(str);

	m_nDataIndex = 0;
	//thread에서 호출하는 loading_function에서 CheckAllThreadEnding을 호출하여
	//모든 loading이 끝났는지 검사했으나
	//thread에서 호출하는 loading_function에서 CheckAllThreadEnding을 호출하는 것은 문제가 있다.
	//이러한 방식은 위험하므로 독립된 타이머에서 CheckAllThreadEnding을 호출하여 완료 여부를 검사하도록 하니 정상 동작한다.
	SetTimer(timer_check_thread_end, 500, NULL);
	for (i = 0; i < m_nThread; i++)
	{
		m_bThreadConvert[i] = true;
		m_pThreadConvert = AfxBeginThread(ThreadStart, this);
	}
#endif
}

#include <mutex>
std::mutex mtx;

void CThumbCtrl::loading_function(int idx)
{
	int ii;
	CString str;

	if ((m_nEndIndex[idx] - m_nStartIndex[idx]) == 1)
		str.Format(_T("%d job started..."), m_nStartIndex[idx]);
	else
		str.Format(_T("%d ~ %d job started..."), m_nStartIndex[idx], m_nEndIndex[idx] - 1);
	TRACE(_T("%s\n"), str);

	for (ii = m_nStartIndex[idx]; ii < m_nEndIndex[idx]; ii++)
	{
		if (m_bThreadConvert[idx] == false)
			break;

		//mtx.lock();

		if (m_loading_files[ii].Right(3).MakeLower() == _T("gif"))
		{
			//CGifEx gif;
			//gif.Load(m_loading_files[ii], m_hWnd, true);
			//mat = HBITMAP2Mat(gif.get_hBitmap());
		}
		else
		{
			//img.Load(m_loading_files[ii]);
		}

		insert(ii, m_loading_files[ii], GetFileTitle(m_loading_files[ii]), false, false);
		//mtx.unlock();

		m_loading_complete[ii] = true;
	}

	m_bThreadConvert[idx] = false;

	if ((m_nEndIndex[idx] - m_nStartIndex[idx]) == 1)
		str.Format(_T("job completed : %d"), m_nStartIndex[idx]);
	else
		str.Format(_T("job completed : %d ~ %d"), m_nStartIndex[idx], m_nEndIndex[idx] - 1);
	TRACE(_T("%s\n"), str);
}

int CThumbCtrl::CheckAllThreadEnding()
{
	int completed = 0;
	for (int i = 0; i < m_loading_files.size(); i++)
	{
		if (m_loading_complete[i])
			completed++;
	}

	if (completed < m_loading_files.size())
	{
		m_loading_completed = false;
		Invalidate();
		return completed;
	}

	m_loading_completed = true;


	if (false)
	{
		CString str;
		str.Format(_T("%d thread loading ellapsed = %s ms\n"), m_nThread, i2S(getClock() - m_clock_start));
		AfxMessageBox(str);
	}

	if (m_index_select_after_loading >= 0)
		m_selected.push_back(m_index_select_after_loading);

	recalculate_scroll_size();
	//Invalidate();


	return m_loading_files.size();
}

int CThumbCtrl::insert(int index, CString full_path, CString title, bool key_thumb, bool invalidate)
{
	CThumbImage *thumb = new CThumbImage;
	thumb->img.Load(full_path);
	thumb->width = thumb->img.width;
	thumb->height = thumb->img.height;
	thumb->channel = 3;
	thumb->title = title;
	thumb->full_path = full_path;

	if (index < 0)
	{
		m_dqThumb.push_back(thumb);
	}
	else
	{
		m_dqThumb[index] = thumb;
	}

	if (invalidate)
	{
		recalculate_scroll_size();
		Invalidate();
	}

	return 0;
}

#if 0
//raw data를 넣을 경우 mat으로 만든 후 위 함수를 호출해서 넣어주자.
int CThumbCtrl::insert(int index, uint8_t* src, int width, int height, int channel, CString sTitle /*= NULL*/, bool key_thumb /*= false*/, bool bModifiedFlag /*= true*/)
{
	//if ( index < 0 )
	//	index = m_dqThumb.size();
	/*
	//썸네일에 표시할 최대 개수가 넘으면 맨 마지막 썸네일을 지우고 추가해준다.
	if ( m_dqThumb.size() >= m_max_thumbs )
	{
		while ( m_dqThumb.size() >= m_max_thumbs )
			m_dqThumb.pop_front();
	}
	*/
	int x, y, c;
	CThumbImage thumb;

	//LLOG( L"before new and copy." );
	int width4 = MAKE4WIDTH(width);
	//printf( "width = %d, rwsize =%d\n", width, rwsize );
	TRACE("width = %d, width4 = %d\n", width, width4);

	//메모리 할당 크기는 4의 배수로 맞추지만
	//실제 복사하는 메모리 데이터의 크기는 원본 크기와 동일해야 한다.
	thumb.data = new uint8_t[width4 * height * channel];
	memset(thumb.data, 0, sizeof(uint8_t) * width4 * height * channel);

	//memcpy_block( src, width, height, 0, 0, width, height, channel, thumb.data );
	memcpy_block(src, 0, 0, width, height, thumb.data, 0, 0, width4, height, width, height, channel);
	//memcpy( thumb.data, pData, width * height * channel );

	/*
	uint8_t *p, *q;

	for ( y = 0; y < height; y++ )
	{
		p = (uint8_t*)(src + y * width * channel + channel);
		q = (uint8_t*)(thumb.data + y * width4 * channel + channel);

		for ( x = 0; x < width; x++ )
		{
			for ( c = 0; c < channel; c++ )
			{
				*(q + x * channel + c) = *(p + x * channel + c);
			}
		}
	}
	*/

	thumb.width = width4;
	thumb.height = height;
	thumb.channel = channel;
	thumb.key_thumb = key_thumb;

	//LLOG( L"sTitle length = %d", sTitle.GetLength() );
	_stprintf(thumb.title, _T("%s\0"), sTitle);

	if (index < 0)
	{
		m_dqThumb.push_back(thumb);
	}
	else
	{
		if (m_dqThumb.max_size() <= index)
			m_dqThumb.resize(m_dqThumb.max_size() + 8);
		m_dqThumb.insert(m_dqThumb.begin() + index, thumb);
	}

	Invalidate();
	recalculate_scroll_size();

	//맨 처음 데이터 파일을 로딩할때는 이 플래그는 false여야 하고
	//추가, 삭제일 때만 true로 처리해야 한다.
	if (bModifiedFlag)
		m_modified = true;

	return index;
}
#endif

void CThumbCtrl::set_scroll_pos(int pos)
{
	CRect rc;
	GetClientRect(rc);

	if (m_scroll_total > rc.Height())
		Clamp(pos, -m_scroll_total + rc.Height(), 0);
	else
		pos = 0;

	m_scroll_pos = pos;
	//TRACE(_T("current = %d / %d\n"), m_scroll_pos, m_scroll_total);
	m_scroll_trans = 0.0;
	m_crScroll = m_crTitle;
	Invalidate(false);

	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	//커서가 스크롤바 영역 밖이라면 스크롤바를 숨겨준다.
	if (/*(rc.PtInRect(pt) == false) ||*/ !m_rScroll.PtInRect(pt))
		SetTimer(timer_scroll_bar_disappear, 500, NULL);
}

void CThumbCtrl::recalculate_scroll_size()
{
	draw_function(NULL, false);

	if (m_dqThumb.size() == 0)
		return;

	CRect	rc;
	GetClientRect(rc);
	//::GetSystemMetrics(SM_CXVSCROLL);
	m_rScroll = CRect(rc.right - ::GetSystemMetrics(SM_CXVSCROLL), rc.top, rc.right + 1, rc.bottom);

	//맨 아래로 스크롤한 후 썸네일 크기를 줄이면 스크롤바가 잘못 표시되는 버그 있음!!
	Clamp(m_scroll_pos, -m_scroll_total + rc.Height(), 0);


	/*
	//Gap의 너비는 균등하게 자동 조절하자.
	if (m_per_line > 1)
	{
		while (true)
		{
			int rest = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx - m_szGap.cx * (m_per_line - 1));
			TRACE(_T("rest = %d, m_szGap.cx = %d\n"), rest, m_szGap.cx);
			if ((m_szGap.cx * (m_per_line - 1) > m_szTile.cx) || (rest > m_szTile.cx))
			{
				m_per_line++;
				m_szGap.cx = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx) / (m_per_line - 1);
				if (m_szGap.cx <= 20)
				{
					m_per_line--;
					break;
				}
			}
			else if (m_szGap.cx <= 20)
			{
				m_per_line--;
				if (m_per_line == 1)
					break;
			}
			else
			{
				break;
			}
		}
	}

	if (m_per_line > 1)
		m_szGap.cx = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx) / (m_per_line - 1);
	TRACE( _T("tile.cx = %d, perLine = %d, maxLine = %d\n"), m_szTile.cx, m_per_line, m_max_line );
	*/
	//최근 윈도우의 스크롤바는 그 범위 크기에 따라 thumb의 크기가 자동 변경되는데
	//그래야만 thumb이 크게 표시되어 드래그하기 편해지기 떄문이다.
	//이렇게 구현하기 위해서는 nPage의 값을 적정값으로 고정하고
	//nMax = realMax + si.nPage - 1;로 주면 원하는 범위로 동작된다.
	//물론 정밀한 이동이 필요한 경우는 그 값을 작게 줘야 정밀하게 스크롤할 수 있다.
	/*
	SCROLLINFO	si;
	si.cbSize = sizeof( SCROLLINFO );

	memset( &si, 0, sizeof(SCROLLINFO) );
	GetScrollInfo( SB_VERT, &si );
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nPage = 1080 + 1;
	*/
	int i;
	m_scroll_total = m_szMargin.cy;

	for (i = 0; i < m_line_height.size(); i++)
		m_scroll_total = m_scroll_total + (m_szTile.cy + m_line_height[i] + m_szGap.cy);
	/*
	if ( totalScrollHeight > rc.Height() )
		//SetScrollRange( SB_VERT, 0, totalScrollHeight - rc.Height() );
		si.nMax = totalScrollHeight - rc.Height() + si.nPage - 1;
	else
		//SetScrollRange( SB_VERT, 0, 0 );
		si.nMax = 0;

	SetScrollInfo( SB_VERT, &si );
	*/
}

void CThumbCtrl::scroll_up(bool up)
{
	CRect rc;
	GetClientRect(rc);

	if (rc.Height() >= m_scroll_total)
		return;

	int pos = m_scroll_pos;//GetScrollPos(SB_VERT);

	if (up)
		pos += (m_szTile.cy + m_szGap.cy);
	else
		pos -= (m_szTile.cy + m_szGap.cy);

	/*
	if (up)
		pos += ((m_szTile.cy + 20 + m_szGap.cy) >> 2) * 3;
	else
		pos -= ((m_szTile.cy + 20 + m_szGap.cy) >> 2) * 3;
	*/

	set_scroll_pos(pos);
}

void CThumbCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	SCROLLINFO	si;
	si.cbSize = sizeof(SCROLLINFO);

	//memset( &si, 0, sizeof(SCROLLINFO) );

	if (GetScrollInfo(SB_HORZ, &si))
	{
		TRACE("nIndex = %d, nPos = %d\n", m_index, si.nPos);
		switch (nSBCode)
		{
		case SB_PAGELEFT:
		case SB_LINELEFT:	//스크롤 바의 왼쪽 화살표를 클릭한 경우
			if (m_index > 0)
			{
				m_index--;
				si.nPos = m_index;
			}
			break;
		case SB_PAGERIGHT:
		case SB_LINERIGHT:	//스크롤 바의 오른쪽 화살표를 클릭한 경우
			if (m_index <= m_dqThumb.size() - 4)
			{
				m_index++;
				si.nPos = m_index;
			}
			break;
		case SB_THUMBPOSITION:	//트랙을 잡고 이동한 후
		case SB_THUMBTRACK:	//트랙을 잡고 움직이는 동안
			si.nPos = m_index = si.nTrackPos;
			break;
		}

		SetScrollPos(SB_HORZ, si.nPos);
		Invalidate();
	}

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CThumbCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	SCROLLINFO	si;
	si.cbSize = sizeof(SCROLLINFO);

	memset(&si, 0, sizeof(SCROLLINFO));

	if (GetScrollInfo(SB_VERT, &si))
	{
		//TRACE( "nIndex = %d, nPos = %d\n", m_index, si.nPos );
		switch (nSBCode)
		{
		case SB_PAGEUP:
		case SB_LINEUP:	//스크롤 바의 왼쪽(위쪽) 화살표를 클릭한 경우
			si.nPos -= ((m_szTile.cy + 20/*m_nTitleHeight*/ + m_szGap.cy) >> 2);
			break;
		case SB_PAGEDOWN:
		case SB_LINEDOWN:	//스크롤 바의 오른쪽(아래) 화살표를 클릭한 경우
			si.nPos += ((m_szTile.cy + 20/*m_nTitleHeight*/ + m_szGap.cy) >> 2);
			break;
		case SB_THUMBPOSITION:	//트랙을 잡고 이동한 후
		case SB_THUMBTRACK:	//트랙을 잡고 움직이는 동안
			si.nPos = si.nTrackPos;
			break;
		}

		SetScrollPos(SB_VERT, si.nPos);
		Invalidate(false);
		TRACE(_T("thumb rect 0 cy = %d\n"), m_dqThumb[0]->rect.CenterPoint().y);// GetRectInfoString(m_dqThumb[0]->rect, 1));

	}

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL CThumbCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (is_editing())
		edit_end();

	if (IsCtrlPressed())
	{
		if ((zDelta > 0) && (m_szTile.cx >= MAX_TILE_SIZE))
			return true;
		if ((zDelta < 0) && (m_szTile.cx <= MIN_TILE_SIZE))
			return true;

		m_szTile.cx = m_szTile.cx + (zDelta > 0 ? 4 : -4);
		m_szTile.cy = m_szTile.cy + (zDelta > 0 ? 4 : -4);

		AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("tile size cx"), m_szTile.cx);
		AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("tile size cy"), m_szTile.cy);
		recalculate_scroll_size();
		Invalidate();
	}
	else
	{
		scroll_up(zDelta > 0);
	}
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

//초기 버전은 1행으로만 구현되었고 ctrl의 height에 따라 썸네일의 크기를 자동 변경했으나
//썸네일 크기는 사용자가 변경할 수 있고 멀티라인으로 변경하자.
void CThumbCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	recalculate_scroll_size();
	set_scroll_pos(m_scroll_pos);

	if (!IsWindowVisible() || IsIconic())
		return;
}

void CThumbCtrl::set_show_title(int show)
{
	if (show == -1)
		m_show_title = !m_show_title;
	else
		m_show_title = show;

	Invalidate();
}

void CThumbCtrl::set_show_resolution(int show)
{
	if (show == -1)
		m_show_resolution = !m_show_resolution;
	else
		m_show_resolution = show;

	Invalidate();
}

void CThumbCtrl::set_show_index(int show)
{
	if (show == -1)
		m_show_index = !m_show_index;
	else
		m_show_index = show;

	Invalidate();
}

void CThumbCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_in_editing)
	{
		edit_end();
		//return;
	}

	SetFocus();

	int i, j;
	CRect rc;
	GetClientRect(rc);

	if (m_rScroll.PtInRect(point))
	{
		m_scroll_drag = true;
		SetCapture();
		CRect rc;
		GetClientRect(rc);
		set_scroll_pos(-(double)(point.y - m_scroll_grip_size / 2) / (double)(rc.Height() - m_scroll_grip_size) * (double)(m_scroll_total - rc.Height()));
		return;
	}

	for (i = 0; i < m_dqThumb.size(); i++)
	{
		if (m_dqThumb[i]->rect.PtInRect(point))
		{
			int selected_index = find_index(&m_selected, i);

			//컨트롤키를 눌렀으면
			if (IsCtrlPressed())
			{
				int last_selected = 0;
				if (m_selected.size() > 0)
					last_selected = m_selected[m_selected.size() - 1];

				//해당 항목이 이미 선택된 상태라면 선택 해제시켜주고
				if (selected_index >= 0)
				{
					//shift까지 눌렸다면 마지막 선택 ~ 현재 항목까지 모두 해제.
					if (IsShiftPressed())
					{
						//마지막 선택 ~ 현재 항목중에서 선택된 항목이 있다면 삭제시킨다.
						if (m_selected.size())
						{
							for (j = m_selected.size() - 1; j >= 0; j--)
							{
								if ((m_selected[j] >= last_selected && m_selected[j] <= i) ||
									(m_selected[j] <= last_selected && m_selected[j] >= i))
									m_selected.erase(m_selected.begin() + j);
							}
						}
					}
					else
					{
						m_selected.erase(m_selected.begin() + selected_index);
					}
				}
				//선택되지 않은 항목이라면 선택리스트에 추가한다.
				else
				{
					//shift까지 눌렸다면 마지막 선택 ~ 현재 항목까지 모두 선택
					if (IsShiftPressed())
					{
						//마지막 선택 ~ 현재 항목중에서 선택된 항목이 있다면 삭제하고 추가해야 한다.
						if (m_selected.size())
						{
							for (j = m_selected.size() - 1; j >= 0; j--)
							{
								if ((m_selected[j] >= last_selected && m_selected[j] <= i) ||
									(m_selected[j] <= last_selected && m_selected[j] >= i))
									m_selected.erase(m_selected.begin() + j);
							}
						}

						for (j = MIN(i, last_selected); j <= MAX(i, last_selected); j++)
							m_selected.push_back(j);
					}
					else
					{
						m_selected.push_back(i);
					}
				}
			}
			else
			{
				//전에 선택항 항목과 새로 선택한 항목이 다르면 m_last_clicked를 초기화시켜줘야
				//원클릭에 의한 타이틀 변경이 올바르게 동작한다.
				if (m_selected.size() && (i != m_selected[0]))
					m_last_clicked = 0;

				//단일 선택이면 기존 선택리스트를 초기화하고 선택리스트에 새로 넣어준다.
				m_selected.clear();
				m_selected.push_back(i);
			}

			//선택된 항목이 rc안에 다 안들어오고 일부만 보이면 다 보이도록 스크롤 시켜준다.
			if (RectInRect(rc, m_dqThumb[i]->rect) == false)
			{
				//int max_height = m_line_height[m_dqThumb[i]->line_index];
				//SetScrollPos(SB_VERT, m_dqThumb[i]->rect.CenterPoint().y - m_dqThumb[0]->rect.CenterPoint().y - rc.Height() / 2);
				ensure_visible(i);
			}

			Invalidate();
			return;
		}
	}

	//클릭된 곳이 썸네일 영역이 아니면 선택을 해제시켜준다.
	m_selected.clear();
	Invalidate();

	CWnd::OnLButtonDown(nFlags, point);
}


void CThumbCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_selected.size())
	{
		::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_THUMBCTRL,
			(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_lbutton_dbclicked, m_selected[0]), 0);
	}

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CThumbCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_scroll_drag)
	{
		ReleaseCapture();
		m_scroll_drag = false;
		CRect rc;
		GetClientRect(rc);
		set_scroll_pos(-(double)(point.y - m_scroll_grip_size / 2) / (double)(rc.Height() - m_scroll_grip_size) * (double)(m_scroll_total - rc.Height()));
		return;
	}

	int index = get_selected_item();
	if (index < 0)
		return;

	//name영역이 눌려지고
	CRect r = m_dqThumb[index]->rect;
	r.top = m_dqThumb[index]->thumb_bottom;
	if (r.PtInRect(point) == false)
		return;

	long t1 = getClock();
	//TRACE(_T("clicked = %d\n"), t1 - m_last_clicked);

	//name영역을 클릭한 후 1~2초 사이에 다시 클릭하면 name 편집모드로 전환된다.
	if ((abs(t1 - m_last_clicked) >= 1000) && (abs(t1 - m_last_clicked) < 2000))
		edit_begin(-1);

	m_last_clicked = t1;

	CWnd::OnLButtonUp(nFlags, point);
}


void CThumbCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CRect rc;
	GetClientRect(rc);

	if (m_scroll_drag)
	{
		set_scroll_pos(-(double)(point.y - m_scroll_grip_size / 2) / (double)(rc.Height() - m_scroll_grip_size) * (double)(m_scroll_total - rc.Height()));
		//m_scroll_pos = (-(double)(point.y) / (double)(rc.Height()-20) * (double)(m_scroll_total - rc.Height()));
		//Invalidate();
	}
	//커서가 스크롤바 영역 근처에 가면 스크롤바를 나타나게 한다.
	else if (point.x >= m_rScroll.left - m_szMargin.cx)
	{
		m_scroll_trans = 0.0;
		m_crScroll = m_crTitle;
		InvalidateRect(m_rScroll);
	}
	//나타날때는 바로, 사라질때는 fade out된다.
	else if (m_scroll_trans == 0.0)
	{
		SetTimer(timer_scroll_bar_disappear, 10, NULL);
	}

	CWnd::OnMouseMove(nFlags, point);
}


void CThumbCtrl::OnMouseLeave()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//printf( "leave" );
	CWnd::OnMouseLeave();
}


void CThumbCtrl::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//printf( "hover" );
	CWnd::OnMouseHover(nFlags, point);
}


void CThumbCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	m_has_focus = true;
	//printf( "set focus" );
	Invalidate();
}


void CThumbCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	m_has_focus = false;
	//printf( "kill focus" );
	Invalidate();
}


void CThumbCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CWnd::OnRButtonDown(nFlags, point);
}


void CThumbCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CMenu	menu;

	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, idToggleIndex, _T("Display thumbnail index(&I)"));
	menu.AppendMenu(MF_STRING, idToggleResolution, _T("Display image resolution"));
	menu.AppendMenu(MF_STRING, idFind, _T("Find...(&F)"));
	menu.AppendMenu(MF_STRING, idPromptMaxThumb, _T("Set max thumbnails(&A)(0 : no limits)..."));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, idDeleteThumb, _T("Delete selected thumbnails(&D)"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, idRemoveAll, _T("Remove all thumbnails...(&R)"));

	menu.CheckMenuItem(idToggleIndex, m_show_index ? MF_CHECKED : MF_UNCHECKED);

	ClientToScreen(&point);
	menu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);

	menu.DestroyMenu();

	CWnd::OnRButtonUp(nFlags, point);
}


void CThumbCtrl::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CWnd::OnRButtonDblClk(nFlags, point);
}


void CThumbCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/)
{
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

void CThumbCtrl::on_menu_delete()
{
	int count = get_selected_items();
	CString str;

	if (count == 1)
		str.Format(_T("선택된 썸네일을 삭제합니다."));
	else if (count > 1)
		str.Format(_T("선택된 %d개의 썸네일을 삭제합니다."), count);

	if (AfxMessageBox(str, MB_OKCANCEL) == IDCANCEL)
		return;

	remove_selected(true);
}

void CThumbCtrl::OnPopupMenu(UINT nMenuID)
{
	switch (nMenuID)
	{
	case idToggleIndex:
		m_show_index = !m_show_index;
		AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("show index"), m_show_index);
		Invalidate();
		break;

	case idToggleResolution:
		m_show_resolution = !m_show_resolution;
		AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("show resolution"), m_show_resolution);
		Invalidate();
		break;

	case idFind :
	{
		find_text(true, true);

		break;
	}

	case idDeleteThumb :
		on_menu_delete();
		break;

	case idPromptMaxThumb:
	{
		TCHAR buf[200] = { 0 };
		TCHAR msg[200] = { 0 };

		_stprintf(msg, _T("표시할 썸네일의 최대 개수를 입력하세요(0:제한없음).\n현재 설정값 = %d"), m_max_thumbs);
		int res = CWin32InputBox::InputBox(_T("썸네일 최대 개수"), msg, buf, 200, false);

		if (res == IDCANCEL)
			return;

		int n = _ttoi(buf);

		if (n <= 0)
			return;

		m_max_thumbs = n;

		//썸네일에 표시할 최대 개수가 넘으면 맨 마지막 썸네일을 지우고 추가해준다.
		if (m_max_thumbs && (m_dqThumb.size() >= m_max_thumbs))
		{
			while (m_dqThumb.size() > m_max_thumbs)
				m_dqThumb.pop_front();

			Invalidate();
		}

		AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("max thumbs limit"), m_max_thumbs);
	}
	break;
	}
}

void CThumbCtrl::set_info_text(int thumb_index, int idx, CString info, bool refresh)
{
	if (thumb_index < 0 || thumb_index >= m_dqThumb.size())
		return;

	if (idx < 0 || idx >= 4)
		return;

	m_dqThumb[thumb_index]->info[idx] = info;

	if (refresh)
		Invalidate();
}



void CThumbCtrl::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanging(lpwndpos);

	// TODO: Add your message handler code here
	//TRACE( _T("lpwndpos->cx = %d\n"), lpwndpos->cx );

	//크기를 제한하려 한 것이나 안먹히는것같다.
	if (lpwndpos->cx < (m_szMargin.cx * 2 + m_szTile.cx))
	{
		lpwndpos->cx = m_szMargin.cx * 2 + m_szTile.cx;
		//return;
	}
}


void CThumbCtrl::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	if (m_in_editing)
		edit_end();
}

void CThumbCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == timer_scroll_bar_disappear)
	{
		KillTimer(timer_scroll_bar_disappear);

		m_crScroll = get_color(m_crTitle, m_crBack, m_scroll_trans);

		m_scroll_trans += 0.01;
		if (m_scroll_trans >= 1.0)
		{
			KillTimer(timer_scroll_bar_disappear);
			m_scroll_trans = 1.0;
			m_crScroll = m_crBack;
		}
		else
		{
			SetTimer(timer_scroll_bar_disappear, 10, NULL);
		}

		InvalidateRect(m_rScroll, false);
	}
	else if (nIDEvent == timer_check_thread_end)
	{
		KillTimer(timer_check_thread_end);
		CheckAllThreadEnding();
		if (m_loading_completed)
		{
			recalculate_scroll_size();
			Invalidate();
		}
		else
		{
			SetTimer(timer_check_thread_end, 500, NULL);
		}
	}
	else if (nIDEvent == timer_load_files)
	{
		KillTimer(timer_load_files);

		//if (!mat.empty())
		{
			insert(m_loading_index, m_loading_files[m_loading_index], GetFileNameFromFullPath(m_loading_files[m_loading_index]), false, false);
			//Invalidate();
		}

		m_loading_index++;
		TRACE(_T("m_loading_index = %d\n"), m_loading_index);

		if (m_loading_index < m_loading_files.size())
		{
			Invalidate();
			SetTimer(timer_load_files, 10, NULL);
		}
		else
		{
			m_loading_completed = true;
			recalculate_scroll_size();
			Invalidate();
		}
	}
	else if (nIDEvent == timer_select_after_loading)
	{
		KillTimer(timer_select_after_loading);

#if (!ADD_USING_THREAD)
		if (m_index_select_after_loading > m_loading_index)
#else
		if (!m_loading_completed)
#endif
			SetTimer(timer_select_after_loading, 500, NULL);
		else
			select_item(m_index_select_after_loading);
	}

	CWnd::OnTimer(nIDEvent);
}

void CThumbCtrl::set_color_theme(int theme, bool invalidate)
{
	switch (theme)
	{
	case color_theme_default:
		m_crTitle = ::GetSysColor(COLOR_BTNTEXT);
		m_crResolution = ::GetSysColor(COLOR_BTNTEXT);
		m_crSelected = ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBack = ::GetSysColor(COLOR_WINDOW);
		m_crBackThumb = ::GetSysColor(COLOR_WINDOW);
		m_crScroll = m_crTitle;
		break;
	case color_theme_dark_gray:
		m_crTitle = GRAY(164);
		m_crResolution = GRAY(164);
		m_crSelected = GRAY(64);
		m_crBack = GRAY(32);
		m_crBackThumb = RGB(255, 0, 0);
		m_crScroll = m_crTitle;
		break;
	}

	if (invalidate)
		Invalidate();
}

void CThumbCtrl::draw_function(CDC* pDC, bool draw)
{
	int		i;
	CRect	rc;
	//CRect	rect(m_szMargin.cx, m_szMargin.cy - GetScrollPos(SB_VERT), m_szMargin.cx + m_szTile.cx, m_szMargin.cy + m_szTile.cy - GetScrollPos(SB_VERT));
	CRect	rect(m_szMargin.cx, m_szMargin.cy + m_scroll_pos, m_szMargin.cx + m_szTile.cx, m_szMargin.cy + m_szTile.cy + m_scroll_pos);

	CString str;
	CClientDC dc(this);

	GetClientRect(rc);

	if (pDC == NULL)
		pDC = &dc;

	if (draw)
		pDC->FillSolidRect(rc, m_crBack);

	//DrawRectangle(pDC, rc, red, 5UL, 2);

	CFont*		pOldfont;

	pOldfont = (CFont*)pDC->SelectObject(&m_font);

	if (draw)
		pDC->SetBkMode(TRANSPARENT);

	int nLineCount = 0;

	if (draw)
		::SetStretchBltMode(pDC->GetSafeHdc(), COLORONCOLOR);

	m_line_height.clear();
	std::deque<int> dqOneLineHeight;

	//라인에 표시할 썸네일 갯수를 디스플레이 하면서 계산한다.
	//매번 계산하는 것이 다소 부담되지만 시간은 거의 0ms다.
	//계산된 값을 토대로 스크롤 사이즈가 결정된다.
	m_per_line = 1;
	m_szGap.cx = MIN_GAP;

	//long t0 = getClock();
	while (true)
	{
		int rest = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx - m_szGap.cx * (m_per_line - 1));

		//TRACE(_T("tile = %d, %d, gap = %d, %d, margin = %d, %d, per_line = %d, rest = %d\n"),
		//	m_szTile.cx, m_szTile.cy, m_szGap.cx, m_szGap.cy, m_szMargin.cx, m_szMargin.cy, m_per_line, rest);


		//하나를 표시하기에도 너비가 적다면 잘리더라도 그냥 표시해야 하므로 중지.
		if (m_per_line == 1 && rest < 0)
			break;

		if (rest <= (m_szTile.cx + m_szGap.cx))
		{
			if (m_per_line == 1)
				m_szGap.cx = 0;
			else if (m_per_line == 2)
				m_szGap.cx = rest / (m_per_line);
			else
				m_szGap.cx = rest / (m_per_line - 1);
			break;
		}

		//TRACE(_T("rest = %d, m_szGap.cx = %d, tile = %d\n"), rest, m_szGap.cx, m_szTile.cx);
		if (((m_szGap.cx) * (m_per_line - 1) > m_szTile.cx) || (rest > m_szTile.cx))
		{
			m_per_line++;

			if (m_per_line < 2)
				m_szGap.cx = MIN_GAP;
			else
				m_szGap.cx = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx) / (m_per_line - 1);

			if (m_szGap.cx < MIN_GAP)
			{
				m_per_line--;

				if (m_per_line < 2)
					m_szGap.cx = MIN_GAP;
				else
					m_szGap.cx = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx) / (m_per_line - 1);

				break;
			}
			else
			{
				m_szGap.cx = MIN_GAP;
			}
		}
		else if (m_szGap.cx < MIN_GAP)
		{
			//ASSERT(0);
			m_per_line--;
			if (m_per_line == 1)
				break;
		}
		else
		{
			break;
		}
	}

	//TRACE(_T("time = %ld, %ld, m_szGap.cx = %d, tile = %d\n"), t0, getClock() - t0, m_szGap.cx, m_szTile.cx);
	CExtWndShadow _shadow;
	CRect rShadow;
	bool skip;

	//스크롤바
	if ((m_scroll_total > rc.Height()) && m_scroll_trans < 1.0)
	{
		int pos = (double)(rc.Height() - m_scroll_grip_size) * ((double)(-m_scroll_pos) / (double)(m_scroll_total - rc.Height()));
		//TRACE(_T("%d / %d, pos = %d\n"), m_scroll_pos, m_scroll_total, pos);
		int round = 2;
		CBrush brush(m_crScroll);
		CBrush* pOldBrush = (CBrush*)pDC->SelectObject(&brush);
		CPen* pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
		//pDC->FillSolidRect(m_rScroll, m_crScroll);
		//pDC->RoundRect(m_rScroll, CPoint(round, round));
		pDC->Rectangle(m_rScroll);
		pDC->SelectObject(pOldBrush);
		pDC->SelectObject(pOldPen);
		//pDC->FillSolidRect(m_rScroll.left + 1, pos, m_rScroll.Width() - 2, m_scroll_grip_size, GRAY(64));

		if (m_scroll_trans <= 0.5)
		{
			CBrush brush(RGB(64, 64, 64));
			CBrush* pOldBrush = (CBrush*)pDC->SelectObject(&brush);
			CPen* pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
			//pDC->RoundRect(m_rScroll.left, pos, m_rScroll.right, pos + m_scroll_grip_size, round, round);
			pDC->Rectangle(m_rScroll.left + 1, pos + 1, m_rScroll.right - 1, pos + m_scroll_grip_size - 2);
			pDC->SelectObject(pOldBrush);
			pDC->SelectObject(pOldPen);
		}
	}

	for (i = 0; i < m_dqThumb.size(); i++)
	{
		if (draw && ((rect.top > rc.bottom) || (rect.bottom + 80) < rc.top))
			skip = true;
		else
			skip = false;

		//TRACE(_T("%3d = %d\n"), i, skip);

		CRect	rTile = rect;
		CRect	fit;

		//타일 크기의 사각형 표시
		//rTile.InflateRect( 1, 1 );
		//DrawRectangle( &dc, rTile, get_color(m_crBack, 48), NULL_BRUSH );

		if (m_dqThumb[i]->img.empty())
		{
			fit = rTile;
			if (draw && !skip)
			{
				pDC->SetTextColor(deeppink);
				pDC->DrawText(_T("Ｘ"), rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
			}
		}
		else
		{
			//scvDrawImage(&dc, m_dqThumb[i]->mat, rect.left, rect.top, 0, 0, NULL, m_crBackThumb, -1.0);
			//scvDrawImage(&dc, m_dqThumb[i]->mat, rect, m_crBackThumb, -1.0);
			fit = GetRatioRect(rect, (double)m_dqThumb[i]->img.width / (double)m_dqThumb[i]->img.height);
			if (draw && !skip)
			{
				//입체감있는 프레임을 그려주는 코드인데 배경이 짙은 회색 계열이면 잘 표시가 안나서 일단 스킵.
				/*
				rShadow = fit;
				rShadow.top = rect.bottom - fit.Height();
				rShadow.bottom = rShadow.top + fit.Height();
				rShadow.InflateRect(13, 13, 11, 12);
				_shadow.Paint(
					*pDC, rShadow,
					CExtWndShadow::DEF_SHADOW_SIZE,
					0,//CExtWndShadow::DEF_BRIGHTNESS_MIN,
					100,//CExtWndShadow::DEF_BRIGHTNESS_MAX,
					false
				);
				*/

				m_dqThumb[i]->img.draw(pDC, fit.left, rect.bottom - fit.Height(), fit.Width(), fit.Height());
			}
			//DrawSunkenRect(&dc, rect, true, get_color(m_crBack, -16), get_color(m_crBack, +16));
		}

		//썸네일 인덱스를 표시한다.
		if (draw && m_show_index)
		{
			if (draw && !skip)
				pDC->SetTextColor(RGB(255, 255, 255));

			str.Format(_T("%d"), i + 1);

			CRect rIndex = rect;
			rIndex.top += 2;
			if (draw && !skip)
				//DrawTextShadow(pDC, str, rIndex, DT_CENTER | DT_TOP | DT_NOCLIP );
				DrawShadowText(pDC->GetSafeHdc(), str, str.GetLength(), rIndex,
					DT_CENTER | DT_TOP | DT_NOCLIP, RGB(255, 255, 255), 0, 2, 1);
		}

		//resolution과 title을 표시한다.
		int height = 0;			//썸네일을 표시하고 그 아래 해상도나 파일명을 표기하는데 필요한 높이.
		CRect rTitle = rect;
		CRect resRect = rect;

		//resolution을 표시한다.
		if (m_show_resolution)
		{
			if (draw && !skip)
				pDC->SetTextColor(m_crResolution);
			resRect.top = rect.bottom;
			resRect.bottom = resRect.top + 20;
			/*
			if (find_index(m_selected, i) >= 0)
			{
			CRect rBack = resRect;
			rBack.InflateRect(4, 4);
			pDC->FillSolidRect(rBack, m_crSelected);
			}
			*/

			if (draw && !skip)
			{
				str.Format(_T("%dx%dx%d"), m_dqThumb[i]->width, m_dqThumb[i]->height, m_dqThumb[i]->channel);
				//str.Format(_T("%d/%d"), m_scroll_pos, m_scroll_total);
				pDC->DrawText(str, resRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
			rTitle = resRect;
			height = height + resRect.Height();
		}

		if (m_show_title && m_dqThumb[i]->title.GetLength())
		{
			pDC->SetTextColor(m_crTitle);
			//DrawShadowText(pDC->GetSafeHdc(), m_dqThumb[i]->title, _tcslen(m_dqThumb[i]->title), rTitle,
			//				DT_CENTER | DT_VCENTER, RGB(255, 0, 0), GetComplementaryColor(m_crBack), 2, 1);
			pDC->DrawText(m_dqThumb[i]->title, rTitle, DT_CALCRECT | DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL | DT_END_ELLIPSIS);//DT_CENTER | DT_TOP | DT_WORDBREAK | DT_NOCLIP);
			int text_max_height = MIN(48, rTitle.Height());

			rTitle.left = rect.left;
			rTitle.right = rect.right;
			rTitle.top = resRect.bottom;
			rTitle.bottom = rTitle.top + text_max_height;
			/*
			if (find_index(m_selected, i) >= 0)
			{
			CRect rBack = rTitle;
			rBack.InflateRect(4, 4);
			pDC->FillSolidRect(rBack, m_crSelected);
			}
			*/
			if (draw && !skip)
				pDC->DrawText(m_dqThumb[i]->title, rTitle, DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL | DT_END_ELLIPSIS);//DT_CENTER | DT_TOP | DT_WORDBREAK | DT_NOCLIP);
				//DrawTextShadow(&dc, thumb.title, rTitle, DT_CENTER | DT_WORDBREAK|DT_EDITCONTROL);//DT_CENTER | DT_TOP | DT_WORDBREAK | DT_NOCLIP);
				//DrawTextShadow( &dc, rTitle, CString(thumb.title), DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP );
				//pDC->FillSolidRect( rTitle, RGB(255,0,0) );

			height = height + text_max_height;
		}

		m_dqThumb[i]->rect = rect;
		m_dqThumb[i]->rect.bottom = rect.bottom + height + 2;
		m_dqThumb[i]->rect.InflateRect(2, 2);
		m_dqThumb[i]->thumb_bottom = rect.bottom;
		m_dqThumb[i]->line_index = nLineCount;


		//특정 keythumbnail의 경우 그 왼쪽에 점선으로 구분선을 표시해주자. T1같은 경우.
		if (draw && !skip && (m_dqThumb[i]->key_thumb && (i > 0)))
		{
			pDC->MoveTo(rect.left - (int)(m_szMargin.cx / 2.0), rc.top);
			pDC->LineTo(rect.left - (int)(m_szMargin.cx / 2.0), rc.bottom);
		}

		dqOneLineHeight.push_back(height);
		/*
		//정보 문자열을 표시한다.
		int w = m_szTile.cx / 2;
		int h = 20;
		int margin = 3;
		UINT nFormat = 0;
		UINT nFormatOld = pDC->SetTextAlign( nFormat );
		CPoint cpText;

		for ( j = 0; j < 4; j++ )
		{
		if ( m_show_info_text[j] == false || m_dqThumb[i]->info[j]->GetLength() == 0 )
		continue;

		nFormat = 0;
		if ( j % 2 == 0 )
		{
		nFormat |= TA_LEFT;
		cpText.x = rect.left + margin;
		}
		else
		{
		nFormat |= TA_RIGHT;
		cpText.x = rect.left + m_szTile.cx - margin;
		}

		if ( j / 2 < 1 )
		{
		nFormat |= TA_TOP;
		cpText.y = rect.top + margin;
		}
		else
		{
		nFormat |= TA_BOTTOM;
		cpText.y = rect.top + m_szTile.cy - margin;
		}

		pDC->SetTextAlign( nFormat );
		TextOutShadow( &dc, cpText.x, cpText.y, CString(m_dqThumb[i]->info[j]), m_crInfoText[j] );
		//DrawTextShadow( &dc, m_rInfoText[j], CString(m_dqThumb[i]->info[j]), DT_CENTER | DT_TOP | DT_NOCLIP, (m_crInfoText[i] == -1 ? RGB(255, 255, 255) : m_crInfoText[i]) );
		//DrawShadowText(pDC->GetSafeHdc(), CString2PCWSTR(m_dqThumb[i]->info[j]), _tcslen(m_dqThumb[i]->info[j]),
		//				CRect(cpText.x, cpText.y, cpText.x + m_szTile.cx, cpText.y - m_lf.lfHeight),
		//				DT_CENTER | DT_VCENTER, m_crInfoText[j], RGB(255, 0, 0), 2, 1);
		}
		*/
		//이전 text align값을 기억했다가 복원해줘야 한다.
		//그렇지 않으면 DrawText에도 영향을 준다.
		//pDC->SetTextAlign( nFormatOld );

		rect.left += (m_szGap.cx + m_szTile.cx);
		rect.right = rect.left + m_szTile.cx;

		if ((rect.right + m_szMargin.cx > rc.right + 2) /* || (nLineCount >= m_per_line)*/)
		{
			m_per_line = dqOneLineHeight.size();

			std::deque<int>::iterator result = std::max_element(dqOneLineHeight.begin(), dqOneLineHeight.end());
			rect.left = m_szMargin.cx;
			rect.top = rect.bottom + m_szGap.cy + result[0];
			rect.right = rect.left + m_szTile.cx;
			rect.bottom = rect.top + m_szTile.cy;

			m_line_height.push_back(result[0]);
			dqOneLineHeight.clear();
			nLineCount++;
		}
	}

	if (dqOneLineHeight.size() && (nLineCount * m_per_line < m_dqThumb.size()))
	{
		std::deque<int>::iterator result = std::max_element(dqOneLineHeight.begin(), dqOneLineHeight.end());
		rect.left = m_szMargin.cx;
		rect.top = rect.bottom + m_szGap.cy + result[0];
		rect.right = rect.left + m_szTile.cx;
		rect.bottom = rect.top + m_szTile.cy;

		nLineCount++;
		m_line_height.push_back(result[0]);
	}

	m_max_line = nLineCount;

	if (m_max_line == 1 && (dqOneLineHeight.size() <= m_dqThumb.size()))
		m_per_line = dqOneLineHeight.size();

	if (draw)
	{
		for (i = 0; i < m_selected.size(); i++)
		{
			CRect r = m_dqThumb[m_selected[i]]->rect;
			r.top -= 4;
			DrawRectangle(pDC, r, RGB(0, 128, 255), NULL_BRUSH, 3);
		}
	}

	pDC->SelectObject(pOldfont);
}

int CThumbCtrl::get_selected_item()
{
	if (m_selected.size())
		return m_selected[0];
	return -1;
}

//dqSelected를 NULL로 주고 선택 개수만 리턴받아 쓰기도 한다.
int CThumbCtrl::get_selected_items(std::deque<int> *dqSelected)
{
	if (dqSelected)
	{
		dqSelected->clear();
		dqSelected->assign(m_selected.begin(), m_selected.end());
	}
	return m_selected.size();
}

//index = -1 : 전체선택
void CThumbCtrl::select_item(int index, bool select, bool make_ensure_visible)
{
#if (ADD_USING_THREAD)
	if (!m_loading_completed)
#else
	//로딩중이라면 해당 인덱스는 아직 안들어왔을 수 있다.
	//로딩이 끝나면 선택시켜야 한다.
	if ((index >= (int)m_dqThumb.size()) && (index > m_loading_index))
#endif
	{
		m_index_select_after_loading = index;
		//SetTimer(timer_select_after_loading, 1000, NULL);
		return;
	}

	int i;

	if (index == -1)
	{
		if (select)
		{
			if (m_selected.size() < m_dqThumb.size())
				m_selected.resize(m_dqThumb.size());
			for (i = 0; i < m_dqThumb.size(); i++)
				m_selected.push_back(i);
		}
		else
		{
			m_selected.clear();
			make_ensure_visible = false;
		}
	}
	else
	{
		if (IsShiftPressed())
		{
			if (m_selected.size() > 0)
			{
				int last = m_selected[m_selected.size() - 1];
				int curr = index;

				while (true)
				{
					if (find_index(&m_selected, curr) < 0)
						m_selected.push_front(curr);

					if (curr < last)
						curr++;
					else if (curr > last)
						curr--;
					else
						break;
				}
			}
		}
		else
		{
			m_selected.clear();
			m_selected.push_back(index);
		}
	}

	if (index >= 0 && make_ensure_visible)
		ensure_visible(index);

	Invalidate();
}

std::deque<int> CThumbCtrl::find_text(bool show_inputbox, bool select)
{
	return find_text(true, _T(""), select);
}

std::deque<int> CThumbCtrl::find_text(bool show_inputbox, CString text, bool select)
{
	TCHAR buf[200] = { 0 };
	TCHAR msg[200] = { 0 };
	int i;
	std::deque<int> dqFound;

	if (show_inputbox)
	{
		_stprintf(msg, _T("검색어를 입력하세요."));
		int res = CWin32InputBox::InputBox(_T("검색어 입력"), msg, buf, 200, false);

		if (res == IDCANCEL)
			return dqFound;

		text = buf;
	}


	for (i = 0; i < m_dqThumb.size(); i++)
	{
		if (m_dqThumb[i]->full_path.Find(text) >= 0)
			dqFound.push_back(i);
	}

	if (select)
	{
		m_selected.clear();
		m_selected.assign(dqFound.begin(), dqFound.end());

		if (dqFound.size() > 0)
			ensure_visible(dqFound[0]);
	}

	Invalidate();
	MessageBeep(0);

	return dqFound;
}

void CThumbCtrl::ensure_visible(int index)
{
	if (m_dqThumb.size() == 0 || index < 0 || index >= m_dqThumb.size())
		return;

	CRect rc;
	GetClientRect(rc);

	//TRACE(_T("thumb rect = %s\n"), GetRectInfoString(m_dqThumb[index]->rect, 1));
	//if (RectInRect(rc, m_dqThumb[index]->rect) == false)
	{
		set_scroll_pos(-(m_dqThumb[index]->thumb_bottom - m_dqThumb[0]->thumb_bottom - rc.Height() / 2 + m_dqThumb[index]->rect.Height()/2));
	}
}

void CThumbCtrl::on_key_down(int key)
{
	if (m_dqThumb.size() == 0)
		return;

	int selected = get_selected_item();
	if (selected < 0)
	{
		select_item(0);
		return;
	}

	if (key == VK_LEFT && selected > 0)
	{
		selected--;
	}
	else if (key == VK_RIGHT && selected < m_dqThumb.size() - 1)
	{
		selected++;
	}
	else if (key == VK_UP && selected >= m_per_line)
	{
		selected -= m_per_line;
	}
	else if (key == VK_DOWN && selected <= m_dqThumb.size() - 1 - m_per_line)
	{
		selected += m_per_line;
		//if (selected > m_dqThumb.size() - 1)
		//	selected = m_dqThumb.size() - 1;
	}
	else if (key == VK_HOME && selected > 0)
	{
		selected = 0;
	}
	else if (key == VK_END && selected < m_dqThumb.size() - 1)
	{
		selected = m_dqThumb.size() - 1;
	}
	else if (key == VK_PRIOR && selected >= m_per_line * 3)
	{
		selected -= m_per_line * 3;
		//if (selected < 0)
		//	selected = 0;
	}
	else if (key == VK_NEXT && selected <= m_dqThumb.size() - 1 - m_per_line * 3)
	{
		selected += m_per_line * 3;
		//if (selected > m_dqThumb.size() - 1)
		//	selected = m_dqThumb.size() - 1;
	}

	select_item(selected);
}

BOOL CThumbCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		/*
		if (m_in_editing)
		{
			m_pEdit->PreTranslateMessage(pMsg);
			return true;
		}
		*/

		if (IsWindowVisible() == false)
			return false;

		switch (pMsg->wParam)
		{
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		case VK_HOME:
		case VK_END:
		case VK_PRIOR:
		case VK_NEXT:
			if (m_in_editing)
				break;
			else
				on_key_down(pMsg->wParam);
			return true;
		case VK_F2:
			if (m_in_editing)
				return true;
			edit_begin(-1);
			return true;
		case VK_SPACE:
			if (m_in_editing)
			{
				break;
			}
			else if (m_selected.size() > 0)
			{
				::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_THUMBCTRL,
					(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_keydown, m_selected[0]), pMsg->wParam);
				return true;
			}
			break;
		case VK_RETURN:
			if (m_in_editing)
			{
				edit_end();
				return true;
			}
			else
			{
				if (m_selected.size() > 0)
				{
					::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_THUMBCTRL,
						(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_keydown, m_selected[0]), pMsg->wParam);
				}
				else
				{
					break;
				}
			}
			return true;
		case VK_ESCAPE:
			if (m_in_editing)
			{
				edit_end(false);
				return true;
			}
			break;
		case VK_DELETE :
			if (!m_in_editing)
				on_menu_delete();
			break;
		case 'A':
			if (IsCtrlPressed())
			{
				if (m_in_editing)
				{
					m_pEdit->SetSel(0, m_dqThumb[m_editing_index]->title.GetLength() - 4);
				}
				else
				{
					select_item(-1, true, false);
				}
				return true;
			}
			break;
		case 'F':
			if (IsCtrlPressed())
			{
				find_text(true, true);
			}
			break;
		}
	}
	else if (pMsg->message == WM_RBUTTONUP)
	{
		if (!m_use_popup)
		{
			::SendMessage(GetParent()->m_hWnd, WM_RBUTTONUP, pMsg->wParam, pMsg->lParam);
			return true;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

#define IDC_EDIT_CELL 29832
void CThumbCtrl::edit_begin(int index)
{
	if (!m_show_title)
		return;

	if (index < 0)
		index = get_selected_item();

	if (index == -1)
		index = 1;

	DWORD dwStyle = ES_CENTER | WS_BORDER | WS_CHILD | WS_VISIBLE /*| ES_AUTOHSCROLL */ | ES_AUTOVSCROLL | ES_MULTILINE;
	CRect r = m_dqThumb[index]->rect;
	//r.right--;
	//r.InflateRect(5, 5);
	r.top = m_dqThumb[index]->thumb_bottom + 10;
	r.bottom = r.top + 800;

	if (m_pEdit == NULL)
	{
		m_pEdit = new CEdit();
		m_pEdit->Create(dwStyle, r, this, IDC_EDIT_CELL);
		m_pEdit->SetFont(&m_font);
		//DWORD margin = m_pEdit->GetMargins();
		m_pEdit->SetMargins(0, 0);
	}

	CDC *pDC = m_pEdit->GetDC();
	CRect textRect = r;

	pDC->DrawText(m_dqThumb[index]->title, textRect, DT_CALCRECT/*| DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL*/);

	m_in_editing = true;
	m_editing_index = index;
	m_old_title = m_dqThumb[index]->title;

	m_pEdit->SetWindowText(m_dqThumb[index]->title);

	//텍스트 출력 높이를 위에서 계산해서 해봤으나 DrawText와 CEdit의 텍스트가 그려지는 모양이 다르다. 그냥 이방법이 정확하다.
	//다만 폰트의 높이에 따라 다를 수 있으나 아직 확인하진 않았다.
	int lc = m_pEdit->GetLineCount();
	r.bottom = r.top + (textRect.Height() + (lc == 1 ? 8 : 8)) * lc;

	//if (textRect.Width() > r.Width())
	//	r.InflateRect((textRect.Width() - r.Width()) / 2, 0);

	if (m_show_resolution)
		r.OffsetRect(0, textRect.Height() - 6);
	else
		r.OffsetRect(0, -9);
	m_pEdit->MoveWindow(r);
	m_pEdit->ShowWindow(SW_SHOW);
	m_pEdit->SetFocus();
	//m_pEdit->GetRect(r);
	//r.InflateRect(6, 0);
	//m_pEdit->SetRect(r);
	m_pEdit->SetSel(0, m_dqThumb[index]->title.ReverseFind('.'));
}

void CThumbCtrl::edit_end(bool valid)
{
	m_in_editing = false;
	m_pEdit->ShowWindow(SW_HIDE);

	CString sText;

	if (valid)
	{
		m_pEdit->GetWindowText(sText);

		if (sText == m_old_title)
			return;

		TRACE(_T("editted text = %s\n"), sText);
		sText.Trim();

		if (sText.IsEmpty())
			return;

		m_dqThumb[m_editing_index]->title = sText;
		Invalidate();

		//썸네일의 이름을 변경하는 것은 파일명인지 무엇인지를 이 컨트롤에서 처리하면 안된다.
		//용도에 맞게 메인에서 어떤 변경인지에 따라 처리한다.
		::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_THUMBCTRL,
			(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_rename, m_editing_index), 0);
	}
}

void CThumbCtrl::set_title(int index, CString title)
{
	if (index < 0 || index > m_dqThumb.size() - 1)
		return;
	m_dqThumb[index]->title = title;
	Invalidate();
}

//info text를 기준으로 리스트를 정렬시킨다.
void CThumbCtrl::sort_by_info(int idx)
{
	std::sort(m_dqThumb.begin(), m_dqThumb.end(),
		[idx](const CThumbImage *a, const CThumbImage *b)
		{
			//문자열인지 숫자인지에 따라 수정되야 한다.
			return (a->info[idx] > b->info[idx]);
		}
	);

	m_modified = true;
	Invalidate();
}

void CThumbCtrl::sort_by_title()
{
	std::sort(m_dqThumb.begin(), m_dqThumb.end(),
		[](CThumbImage *a, CThumbImage *b)
	{
		CString str0 = a->title;
		CString str1 = b->title;
		return !is_greater_with_numeric(str0, str1);
	}
	);

	//순서가 변경되면 해당 썸네일의 rect정보도 변경된다.
	//Invalidate에서도 그 정보를 갱신하지만
	//OnPaint가 완료되기 전에 다른 코드가 수행된다면
	//문제가 될 수 있다. 수동으로 갱신한 뒤 Invalidate을 호출해주자.
	recalculate_scroll_size();
	Invalidate();
}

int	 CThumbCtrl::find_by_title(CString title, bool bWholeWord)
{
	if (title.IsEmpty())
		return -1;

	for (int i = 0; i < m_dqThumb.size(); i++)
	{
		if (bWholeWord)
		{
			if (m_dqThumb[i]->title == title)
				return i;
		}
		else
		{
			if (m_dqThumb[i]->title.Find(title) >= 0)
				return i;
		}
	}

	return -1;
}

CGdiPlusBitmap CThumbCtrl::get_img(int index)
{
	if (index < 0 || index >= m_dqThumb.size())
		return CGdiPlusBitmap();

	return m_dqThumb[index]->img;
}
