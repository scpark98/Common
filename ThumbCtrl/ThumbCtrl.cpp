// ScparkThumbCtrl.cpp : ���� �����Դϴ�.
//

//#include "stdafx.h"
#include "ThumbCtrl.h"

#include <thread>
#include "../Functions.h"
#include "../MemoryDC.h"
#include "../GifEx.h"
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
	m_szMargin = CSize(20, 20);
	m_szGap = CSize(50, 20);

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
	for (int i = 0; i < m_thumb.size(); i++)
	{
		if (m_thumb[i].img->is_valid())
		{
			m_thumb[i].img->release();
			delete m_thumb[i].img;
		}

		SAFE_DELETE_ARRAY(m_thumb[i].feature);
	}

	m_thumb.clear();
}

//index == -1�̸� ��ü ����
//�� �Լ��� ���ϻ����ε��ѵ� ���� �ڵ� �̿ϼ��ȵ��ϹǷ� �켱 ��ŵ�Ѵ�.
void CThumbCtrl::remove(int index, bool bRepaint /*= false*/)
{
	return;

	if (index < 0)
	{
		m_thumb.clear();
	}
	else
	{
		if (m_thumb[index].full_path.IsEmpty() == false)
			DeleteFile(m_thumb[index].full_path);
		m_thumb.erase(m_thumb.begin() + index);
	}


	if (bRepaint)
	{
		recalculate_scroll_size();
		Invalidate();
	}

	m_modified = true;

	//�������� ������ ���⼭ �̷�������� ������ ���� ������ ���ο��� �ؾ� �Ѵ�.
	//���� �޽����� ���� �ʿ䰡 ����.
	//::SendMessage( GetParent()->GetSafeHwnd(),	Message_CThumbCtrl,
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
	ON_COMMAND_RANGE(idTotalCount, idRemoveAll, OnPopupMenu)
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
		brush.CreateSolidBrush(m_cr_back.ToCOLORREF());

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


// CThumbCtrl �޽��� ó�����Դϴ�.

void CThumbCtrl::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	// �׸��� �޽����� ���ؼ��� CWnd::OnPaint()��(��) ȣ������ ���ʽÿ�.
	CRect	rc;
	//TRACE(_T("scrollpos = %d\n"), GetScrollPos(SB_VERT));
	//CRect	rect(m_szMargin.cx, m_szMargin.cy - GetScrollPos(SB_VERT), m_szMargin.cx + m_szTile.cx, m_szMargin.cy + m_szTile.cy - GetScrollPos(SB_VERT));
	CRect	rect(m_szMargin.cx, m_szMargin.cy + m_scroll_pos, m_szMargin.cx + m_szTile.cx, m_szMargin.cy + m_szTile.cy + m_scroll_pos);
	CString str;

	if (this == NULL)
		return;

	GetClientRect(rc);

	CMemoryDC	dc(&dc1, &rc);
	dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());

	if (m_loading_files.size() > 0 && !m_loading_completed)
	{
		CRect r = make_center_rect(rc.CenterPoint().x, rc.CenterPoint().y, rc.Width() / 2, 16);
		draw_sunken_rect(&dc, r, true, Gdiplus::Color::DimGray, Gdiplus::Color::DarkGray);
		r.DeflateRect(1, 1);
		double width = r.Width();
		/*
#if (ADD_USING_THREAD)
		r.right = r.left + width * ((double)CheckAllThreadEnding() / (double)m_loading_files.size());
#else
		r.right = r.left + width * ((double)m_loading_index / (double)m_loading_files.size());
#endif
*/
		dc.FillSolidRect(r, green);
		return;
	}
	else if (m_thumb.size() == 0)
	{
		str = _T("ǥ���� �̹����� �����ϴ�.");
		dc.DrawText(str, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		//DrawTextShadow(&dc, str, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE, GRAY64, red);
		//DrawShadowText(dc.GetSafeHdc(), str, str.GetLength(), rc,
		//	DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, 0, red, 2, 4);
		draw_rectangle(&dc, rc, GRAY192);

		return;
	}

	long t0 = clock();
	draw_function(&dc, true);
	TRACE(_T("	draw_function(&dc, true) = %ld\n"), clock() - t0);
}


BOOL CThumbCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	return FALSE;
	//return CWnd::OnEraseBkgnd(pDC);
}

void CThumbCtrl::ReconstructFont()
{
	m_lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;

	m_font.DeleteObject();
	m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);
}

void CThumbCtrl::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	ReconstructFont();
}

void CThumbCtrl::set_font_size(int nSize)
{
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = -MulDiv(nSize, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
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

	//�ڱ� �ڽſ��� �ο��� ��Ʈ�� ���ٸ� null�� ���ϵȴ�.
	//dlg�� parent�� font�� ���;� �Ѵ�.
	CFont* font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	//m_nDefaultHeight = m_lf.lfHeight;
	ReconstructFont();

	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	//ModifyStyle ( NULL, WS_VSCROLL/*WS_BORDER | WS_EX_CLIENTEDGE *//*| WS_HSCROLL |*/  );

	m_szTile.cx = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("tile size cx"), 128);
	m_szTile.cy = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("tile size cy"), 128);
}

CThumbCtrl* pWnd;
void CThumbCtrl::add_files(std::deque<CString> files, bool reset)
{
	pWnd = this;
	KillTimer(timer_load_files);

	if (reset)
	{
		m_selected.clear();
		release(-1);
	}

	m_scroll_pos = 0;

	if (files.size() == 0)
	{
		m_loading_completed = true;
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
			(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_loading_completed, 0), 0);
		Invalidate();
		return;
	}

	m_thumb.resize(files.size());
	for (int i = 0; i < files.size(); i++)
	{
		//m_thumb[i] = new CThumbImage;
		m_thumb[i].title = get_part(files[i], fn_name);
	}

	Invalidate();

	m_loading_files = files;
	m_loading_completed = false;

	m_clock_start = getClock();

	//timer method
#if (!ADD_USING_THREAD)
	m_loading_index = 0;
	SetTimer(timer_load_files, 10, NULL);
#else
	m_thread.job(files.size(), loading_function, loading_completed_callback);
#endif
}

#include <mutex>
std::mutex mtx;

void CThumbCtrl::loading_function(int idx, int start, int end)
{
	int i;
	CString str;

	if (end - start == 1)
		str.Format(_T("%d job started..."), start);
	else
		str.Format(_T("%d ~ %d job started..."), start, end - 1);
	TRACE(_T("%s\n"), str);

	for (i = start; i < end; i++)
	{
		if (pWnd->m_loading_files[i].Right(3).MakeLower() == _T("gif"))
		{
			//CGifEx gif;
			//gif.Load(m_loading_files[i], m_hWnd, true);
			//mat = HBITMAP2Mat(gif.get_hBitmap());
		}
		else
		{
			//img->Load(m_loading_files[i]);
		}

		pWnd->insert(i, pWnd->m_loading_files[i], get_part(pWnd->m_loading_files[i], fn_title), false, false);
	}

	if ((end - start) == 1)
		str.Format(_T("job completed : %d"), start);
	else
		str.Format(_T("job completed : %d ~ %d"), start, end - 1);
	TRACE(_T("%s\n"), str);

	mtx.lock();
	pWnd->m_thread.thread_ended.push_back(idx);
	mtx.unlock();
}

void CThumbCtrl::loading_completed_callback()
{
	pWnd->on_loading_completed();
	//pWnd->m_loading_completed = true;
	//if (pWnd->m_index_select_after_loading >= 0)
	//	pWnd->m_selected.push_back(pWnd->m_index_select_after_loading);

	//pWnd->recalculate_scroll_size();
}

void CThumbCtrl::on_loading_completed()
{
	m_loading_completed = true;
	if (m_index_select_after_loading >= 0)
		m_selected.push_back(m_index_select_after_loading);

	recalculate_scroll_size();

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
		(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_loading_completed, 0), 0);
}

int CThumbCtrl::insert(int index, CString full_path, CString title, bool key_thumb, bool invalidate)
{
	CThumbImage thumb;
#if USE_OPENCV
	thumb.img = loadMat(full_path);
#else
	thumb.img = new CSCGdiplusBitmap();

	if (!thumb.img)
	{
		TRACE(_T("index = %d, can't allocate memory.\n"), index);
		return -1;
	}

	thumb.img->load(full_path);
#endif
	thumb.width = thumb.img->width;
	thumb.height = thumb.img->height;
	thumb.channel = 3;
	thumb.title = title;
	thumb.full_path = full_path;
	thumb.score = 0.0;
	thumb.feature = NULL;

	if (index < 0)
	{
		m_thumb.push_back(thumb);
		index = m_thumb.size() - 1;
	}
	else
	{
		m_thumb[index] = thumb;
	}

	if (invalidate)
	{
		recalculate_scroll_size();
		Invalidate();
	}

	return index;
}

#if 0
//raw data�� ���� ��� mat���� ���� �� �� �Լ��� ȣ���ؼ� �־�����.
int CThumbCtrl::insert(int index, uint8_t* src, int width, int height, int channel, CString sTitle /*= NULL*/, bool key_thumb /*= false*/, bool bModifiedFlag /*= true*/)
{
	//if ( index < 0 )
	//	index = m_thumb.size();
	/*
	//����Ͽ� ǥ���� �ִ� ������ ������ �� ������ ������� ����� �߰����ش�.
	if ( m_thumb.size() >= m_max_thumbs )
	{
		while ( m_thumb.size() >= m_max_thumbs )
			m_thumb.pop_front();
	}
	*/
	int x, y, c;
	CThumbImage thumb;

	//LLOG( L"before new and copy." );
	int width4 = MAKE4WIDTH(width);
	//printf( "width = %d, rwsize =%d\n", width, rwsize );
	TRACE("width = %d, width4 = %d\n", width, width4);

	//�޸� �Ҵ� ũ��� 4�� ����� ��������
	//���� �����ϴ� �޸� �������� ũ��� ���� ũ��� �����ؾ� �Ѵ�.
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
		m_thumb.push_back(thumb);
	}
	else
	{
		if (m_thumb.max_size() <= index)
			m_thumb.resize(m_thumb.max_size() + 8);
		m_thumb.insert(m_thumb.begin() + index, thumb);
	}

	Invalidate();
	recalculate_scroll_size();

	//�� ó�� ������ ������ �ε��Ҷ��� �� �÷��״� false���� �ϰ�
	//�߰�, ������ ���� true�� ó���ؾ� �Ѵ�.
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
	m_cr_scroll = m_cr_title;
	Invalidate(false);

	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	//Ŀ���� ��ũ�ѹ� ���� ���̶�� ��ũ�ѹٸ� �����ش�.
	if ((rc.PtInRect(pt) == false) || !m_rScroll.PtInRect(pt))
		SetTimer(timer_scroll_bar_disappear, 2000, NULL);
}

void CThumbCtrl::recalculate_scroll_size()
{
	TRACE(_T("%s\n"), __function__);

	CRect	rc;
	GetClientRect(rc);

	long t0 = clock();
	draw_function(NULL, false);
	TRACE(_T("draw_function(NULL, false) = %ld\n"), clock() - t0);

	if (m_thumb.size() == 0)
		return;

	GetClientRect(rc);
	m_rScroll = CRect(rc.right - ::GetSystemMetrics(SM_CXVSCROLL), rc.top, rc.right + 1, rc.bottom);

	//�� �Ʒ��� ��ũ���� �� ����� ũ�⸦ ���̸� ��ũ�ѹٰ� �߸� ǥ�õǴ� ���� ����!!
	Clamp(m_scroll_pos, -m_scroll_total + rc.Height(), 0);


	/*
	//Gap�� �ʺ�� �յ��ϰ� �ڵ� ��������.
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
	//�ֱ� �������� ��ũ�ѹٴ� �� ���� ũ�⿡ ���� thumb�� ũ�Ⱑ �ڵ� ����Ǵµ�
	//�׷��߸� thumb�� ũ�� ǥ�õǾ� �巡���ϱ� �������� �����̴�.
	//�̷��� �����ϱ� ���ؼ��� nPage�� ���� ���������� �����ϰ�
	//nMax = realMax + si.nPage - 1;�� �ָ� ���ϴ� ������ ���۵ȴ�.
	//���� ������ �̵��� �ʿ��� ���� �� ���� �۰� ��� �����ϰ� ��ũ���� �� �ִ�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	SCROLLINFO	si;
	si.cbSize = sizeof(SCROLLINFO);

	//memset( &si, 0, sizeof(SCROLLINFO) );

	if (GetScrollInfo(SB_HORZ, &si))
	{
		TRACE("nIndex = %d, nPos = %d\n", m_index, si.nPos);
		switch (nSBCode)
		{
		case SB_PAGELEFT:
		case SB_LINELEFT:	//��ũ�� ���� ���� ȭ��ǥ�� Ŭ���� ���
			if (m_index > 0)
			{
				m_index--;
				si.nPos = m_index;
			}
			break;
		case SB_PAGERIGHT:
		case SB_LINERIGHT:	//��ũ�� ���� ������ ȭ��ǥ�� Ŭ���� ���
			if (m_index <= m_thumb.size() - 4)
			{
				m_index++;
				si.nPos = m_index;
			}
			break;
		case SB_THUMBPOSITION:	//Ʈ���� ��� �̵��� ��
		case SB_THUMBTRACK:	//Ʈ���� ��� �����̴� ����
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	SCROLLINFO	si;
	si.cbSize = sizeof(SCROLLINFO);

	memset(&si, 0, sizeof(SCROLLINFO));

	if (GetScrollInfo(SB_VERT, &si))
	{
		//TRACE( "nIndex = %d, nPos = %d\n", m_index, si.nPos );
		switch (nSBCode)
		{
		case SB_PAGEUP:
		case SB_LINEUP:	//��ũ�� ���� ����(����) ȭ��ǥ�� Ŭ���� ���
			si.nPos -= ((m_szTile.cy + 20/*m_nTitleHeight*/ + m_szGap.cy) >> 2);
			break;
		case SB_PAGEDOWN:
		case SB_LINEDOWN:	//��ũ�� ���� ������(�Ʒ�) ȭ��ǥ�� Ŭ���� ���
			si.nPos += ((m_szTile.cy + 20/*m_nTitleHeight*/ + m_szGap.cy) >> 2);
			break;
		case SB_THUMBPOSITION:	//Ʈ���� ��� �̵��� ��
		case SB_THUMBTRACK:	//Ʈ���� ��� �����̴� ����
			si.nPos = si.nTrackPos;
			break;
		}

		SetScrollPos(SB_VERT, si.nPos);
		Invalidate(false);
		TRACE(_T("thumb rect 0 cy = %d\n"), m_thumb[0].rect.CenterPoint().y);// GetRectInfoString(m_thumb[0].rect, 1));

	}

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CThumbCtrl::enlarge_size(bool enlarge)
{
	if (enlarge && (m_szTile.cx >= MAX_TILE_SIZE))
		return;
	if (!enlarge && (m_szTile.cx <= MIN_TILE_SIZE))
		return;

	m_szTile.cx = m_szTile.cx + (enlarge ? 4 : -4);
	m_szTile.cy = m_szTile.cy + (enlarge ? 4 : -4);

	AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("tile size cx"), m_szTile.cx);
	AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("tile size cy"), m_szTile.cy);
	recalculate_scroll_size();
	Invalidate();
}

BOOL CThumbCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (is_editing())
		edit_end();

	if (IsCtrlPressed())
	{
		enlarge_size(zDelta > 0);
		return TRUE;
	}
	else
	{
		scroll_up(zDelta > 0);
	}
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

//�ʱ� ������ 1�����θ� �����Ǿ��� ctrl�� height�� ���� ������� ũ�⸦ �ڵ� ����������
//����� ũ��� ����ڰ� ������ �� �ְ� ��Ƽ�������� ��������.
void CThumbCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	recalculate_scroll_size();
	set_scroll_pos(m_scroll_pos);

	if (!IsWindowVisible() || IsIconic())
		return;
}

void CThumbCtrl::show_title(int show)
{
	if (show == -1)
		m_show_title = !m_show_title;
	else
		m_show_title = show;

	Invalidate();
}

void CThumbCtrl::show_resolution(int show)
{
	if (show == -1)
		m_show_resolution = !m_show_resolution;
	else
		m_show_resolution = show;

	Invalidate();
}

void CThumbCtrl::show_index(int show)
{
	if (show == -1)
		m_show_index = !m_show_index;
	else
		m_show_index = show;

	Invalidate();
}

void CThumbCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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

	for (i = 0; i < m_thumb.size(); i++)
	{
		if (m_thumb[i].rect.PtInRect(point))
		{
			int selected_index = find_index(m_selected, i);

			//��Ʈ��Ű�� ������ m_use_multi_selection�� false��� �ص� ���߼����� �����ϴ�.
			if (IsCtrlPressed())
			{
				int last_selected = 0;
				if (m_selected.size() > 0)
					last_selected = m_selected[m_selected.size() - 1];

				//�ش� �׸��� �̹� ���õ� ���¶�� ���� ���������ְ�
				if (selected_index >= 0)
				{
					//shift���� ���ȴٸ� ������ ���� ~ ���� �׸���� ��� ����.
					if (IsShiftPressed())
					{
						//������ ���� ~ ���� �׸��߿��� ���õ� �׸��� �ִٸ� ������Ų��.
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
						::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
							(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_lbutton_unselected, selected_index), 0);
					}
				}
				//���õ��� ���� �׸��̶�� ���ø���Ʈ�� �߰��Ѵ�.
				else
				{
					//shift���� ���ȴٸ� ������ ���� ~ ���� �׸���� ��� ����
					if (IsShiftPressed())
					{
						//������ ���� ~ ���� �׸��߿��� ���õ� �׸��� �ִٸ� �����ϰ� �߰��ؾ� �Ѵ�.
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
						{
							m_selected.push_back(j);
							::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
								(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_lbutton_selected, i), 0);
						}
					}
					else
					{
						m_selected.push_back(i);
						::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
							(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_lbutton_selected, i), 0);
					}
				}
			}
			else
			{
				//���� ������ �׸�� ���� ������ �׸��� �ٸ��� m_last_clicked�� �ʱ�ȭ�������
				//��Ŭ���� ���� Ÿ��Ʋ ������ �ùٸ��� �����Ѵ�.
				if (m_selected.size() && (i != m_selected[0]))
					m_last_clicked = 0;

				//���� �����̸� ���� ���ø���Ʈ�� �ʱ�ȭ�ϰ� ���ø���Ʈ�� ���� �־��ش�.
				if (!m_use_multi_selection)
					m_selected.clear();

				if (selected_index >= 0 && m_selected.size() && find_index(m_selected, m_selected[selected_index]) >= 0)
				{
					m_selected.erase(m_selected.begin() + selected_index);
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
						(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_lbutton_unselected, selected_index), 0);
				}
				else
				{
					if (find_index(m_selected, i) < 0)
						m_selected.push_back(i);
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
						(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_lbutton_selected, i), 0);
				}
			}

			//���õ� �׸��� rc�ȿ� �� �ȵ����� �Ϻθ� ���̸� �� ���̵��� ��ũ�� �����ش�.
			if (rect_in_rect(rc, m_thumb[i].rect) == false)
			{
				//int max_height = m_line_height[m_thumb[i].line_index];
				//SetScrollPos(SB_VERT, m_thumb[i].rect.CenterPoint().y - m_thumb[0].rect.CenterPoint().y - rc.Height() / 2);
				ensure_visible(i);
			}

#ifdef _DEBUG
			for (int j = 0; j < m_selected.size(); j++)
				Trace(_T("%d "), m_selected[j]);
			Trace(_T("\n"));
#endif
			Invalidate();
			return;
		}
	}

	//Ŭ���� ���� ����� ������ �ƴϸ� ������ ���������ش�.
	if (!m_use_multi_selection)
		m_selected.clear();

	Invalidate();

	CWnd::OnLButtonDown(nFlags, point);
}


void CThumbCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (m_selected.size())
	{
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
			(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_lbutton_dbclicked, m_selected[0]), 0);
	}

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CThumbCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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

	//name������ ��������
	CRect r = m_thumb[index].rect;
	r.top = m_thumb[index].thumb_bottom;
	if (r.PtInRect(point) == false)
		return;

	long t1 = getClock();
	//TRACE(_T("clicked = %d\n"), t1 - m_last_clicked);

	//name������ Ŭ���� �� 1~2�� ���̿� �ٽ� Ŭ���ϸ� name �������� ��ȯ�ȴ�.
	if ((abs(t1 - m_last_clicked) >= 1000) && (abs(t1 - m_last_clicked) < 2000))
		edit_begin(-1);

	m_last_clicked = t1;

	CWnd::OnLButtonUp(nFlags, point);
}


void CThumbCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	CRect rc;
	GetClientRect(rc);

	if (m_scroll_drag)
	{
		set_scroll_pos(-(double)(point.y - m_scroll_grip_size / 2) / (double)(rc.Height() - m_scroll_grip_size) * (double)(m_scroll_total - rc.Height()));
		//m_scroll_pos = (-(double)(point.y) / (double)(rc.Height()-20) * (double)(m_scroll_total - rc.Height()));
		//Invalidate();
	}
	//Ŀ���� ��ũ�ѹ� ���� ��ó�� ���� ��ũ�ѹٸ� ��Ÿ���� �Ѵ�.
	else if (point.x >= m_rScroll.left - m_szMargin.cx)
	{
		if (m_scroll_trans == 1.0)
		{
			TRACE(_T("show scroll\n"));
			m_scroll_trans = 0.0;
			m_cr_scroll = m_cr_title;
			InvalidateRect(m_rScroll);
		}
	}
	//��Ÿ������ �ٷ�, ��������� fade out�ȴ�.
	else if (m_scroll_trans == 0.0)
	{
		TRACE(_T("hide scroll\n"));
		m_scroll_trans = 1.0;
		InvalidateRect(m_rScroll);
		//SetTimer(timer_scroll_bar_disappear, 1, NULL);
	}

	//CWnd::OnMouseMove(nFlags, point);
}


void CThumbCtrl::OnMouseLeave()
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	//printf( "leave" );
	CWnd::OnMouseLeave();
}


void CThumbCtrl::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	//printf( "hover" );
	CWnd::OnMouseHover(nFlags, point);
}


void CThumbCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	m_has_focus = true;
	//printf( "set focus" );
	Invalidate();
}


void CThumbCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	//edit_end(true);

	m_has_focus = false;
	//printf( "kill focus" );
	Invalidate();
}


void CThumbCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	CWnd::OnRButtonDown(nFlags, point);
}


void CThumbCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	//���õ� �׸��� 1�� ������ ���¿��� rbuttonup�̸�
	//���� �ڸ��� �׸� ���� �׼��� ���ɼ��� �����Ƿ� ���� ó���Ѵ�.
	//2�� �̻� ���õ� ���¿����� ���� ���¸� �״�� ���������ش�.
	if (get_selected_items() <= 1)
	{
		int index = get_index_from_point(point);
		if (index >= 0)
			select_item(index);
	}

	CMenu	menu;
	menu.CreatePopupMenu();

	menu.AppendMenu(MF_STRING, idTotalCount, _T("�� ") + i2S(m_thumb.size()) + _T("���� �����"));
	menu.EnableMenuItem(idTotalCount, MF_GRAYED);
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, idFind, _T("ã��...(&F)"));
	menu.AppendMenu(MF_STRING, idReload, _T("���� ��ħ(&R)"));
	menu.AppendMenu(MF_STRING, idReloadSelected, _T("���� �׸� ���� ��ħ"));
	menu.AppendMenu(MF_STRING, idSortByTitle, _T("Ÿ��Ʋ�� ����(&S)"));
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, idCopyToClipboard, _T("����(&C)"));
	menu.AppendMenu(MF_SEPARATOR);	

	menu.AppendMenu(MF_STRING, idToggleIndex, _T("�ε��� ǥ��"));
	menu.AppendMenu(MF_STRING, idToggleTitle, _T("Ÿ��Ʋ ǥ��"));
	menu.AppendMenu(MF_STRING, idToggleResolution, _T("�ػ� ���� ǥ��"));
	//menu.AppendMenu(MF_STRING, idPromptMaxThumb, _T("Set max thumbnails(&A)(0 : no limits)..."));
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, idDeleteThumb, _T("���� �׸��� ����Ʈ���� ����"));
	//menu.AppendMenu(MF_SEPARATOR);
	//menu.AppendMenu(MF_STRING, idRemoveAll, _T("��� ����� ����...(&R)"));

	menu.CheckMenuItem(idToggleIndex, m_show_index ? MF_CHECKED : MF_UNCHECKED);
	menu.CheckMenuItem(idToggleTitle, m_show_title ? MF_CHECKED : MF_UNCHECKED);
	menu.CheckMenuItem(idToggleResolution, m_show_resolution ? MF_CHECKED : MF_UNCHECKED);

	ClientToScreen(&point);
	menu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);

	menu.DestroyMenu();

	CWnd::OnRButtonUp(nFlags, point);
}


void CThumbCtrl::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	CWnd::OnRButtonDblClk(nFlags, point);
}


void CThumbCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
}

void CThumbCtrl::on_menu_delete()
{
	int count = get_selected_items();
	CString str;

	if (count == 1)
		str.Format(_T("���õ� ������� �����մϴ�."));
	else if (count > 1)
		str.Format(_T("���õ� %d���� ������� �����մϴ�."), count);

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

	case idToggleTitle:
		m_show_title = !m_show_title;
		AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("show title"), m_show_title);
		Invalidate();
		break;

	case idToggleResolution:
		m_show_resolution = !m_show_resolution;
		AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("show resolution"), m_show_resolution);
		Invalidate();
		break;

	case idFind:
	{
		find_text(true, true);
		break;
	}

	//loading�� main���� ȣ���ϹǷ� ���ο��� �޽����� �����Ѵ�.
	case idReload :
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
			(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_reload, 0), 0);
		break;
	case idReloadSelected:
		{
			int index = get_selected_item();
			if (index < 0 || index >= m_thumb.size())
				return;
			m_thumb[index].reload();
			InvalidateRect(m_thumb[index].rect);
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
				(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_reload_selected, 0), 0);
		}
		break;

	case idSortByTitle :
		sort_by_title();
		break;
	case idCopyToClipboard :
		if (m_thumb.size() == 0)
			break;
		{
			int index = get_selected_item();
			if (index >= 0)
			{
				m_thumb[index].img->copy_to_clipbard();
			}
		}
		break;
	case idDeleteThumb:
		on_menu_delete();
		break;

	case idPromptMaxThumb:
	{
		TCHAR buf[200] = { 0 };
		TCHAR msg[200] = { 0 };

		_stprintf_s(msg, _countof(msg), _T("ǥ���� ������� �ִ� ������ �Է��ϼ���(0:���Ѿ���).\n���� ������ = %d"), m_max_thumbs);
		int res = CWin32InputBox::InputBox(_T("����� �ִ� ����"), msg, buf, 200, false);

		if (res == IDCANCEL)
			return;

		int n = _ttoi(buf);

		if (n <= 0)
			return;

		m_max_thumbs = n;

		//����Ͽ� ǥ���� �ִ� ������ ������ �� ������ ������� ����� �߰����ش�.
		if (m_max_thumbs && (m_thumb.size() >= m_max_thumbs))
		{
			while (m_thumb.size() > m_max_thumbs)
				m_thumb.pop_front();

			Invalidate();
		}

		AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("max thumbs limit"), m_max_thumbs);
	}
	break;
	}
}

void CThumbCtrl::info_text(int thumb_index, int idx, CString info, bool refresh)
{
	if (thumb_index < 0 || thumb_index >= m_thumb.size())
		return;

	if (idx < 0 || idx >= 4)
		return;

	m_show_info_text[idx] = true;
	m_thumb[thumb_index].info[idx] = info;

	if (refresh)
		Invalidate();
}



void CThumbCtrl::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanging(lpwndpos);

	// TODO: Add your message handler code here
	//TRACE( _T("lpwndpos->cx = %d\n"), lpwndpos->cx );

	//ũ�⸦ �����Ϸ� �� ���̳� �ȸ����°Ͱ���.
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
	//��ũ�ѹٸ� ���⶧ fadeout���� ������ InvalidateRect�� �Ϻ� ������ �ƴ�
	//��ü ������ ����ǰ� �־ �ϴ� �� Ÿ�̸Ӵ� ������� �ʴ´�.
	if (nIDEvent == timer_scroll_bar_disappear)
	{
		KillTimer(timer_scroll_bar_disappear);

		//m_cr_scroll = get_color(m_cr_title, m_cr_back, m_scroll_trans);

		m_scroll_trans = 1.0;
		if (true)//m_scroll_trans >= 1.0)
		{
			KillTimer(timer_scroll_bar_disappear);
			m_scroll_trans = 1.0;
			m_cr_scroll = m_cr_back;
		}
		else
		{
			SetTimer(timer_scroll_bar_disappear, 1, NULL);
		}

		InvalidateRect(m_rScroll, false);
	}
	else if (nIDEvent == timer_load_files)
	{
		KillTimer(timer_load_files);

		//if (!mat.empty())
		{
			insert(m_loading_index, m_loading_files[m_loading_index], get_part(m_loading_files[m_loading_index], fn_name), false, false);
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

			::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
				(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_loading_completed, 0), 0);
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
		m_cr_title.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
		m_cr_resolution.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
		m_cr_selected.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHT));
		m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_WINDOW));
		m_cr_back_thumb.SetFromCOLORREF(::GetSysColor(COLOR_WINDOW));
		m_cr_scroll = m_cr_title;
		break;
	case color_theme_dark_gray:
		m_cr_title = gGRAY(164);
		m_cr_resolution = gGRAY(164);
		m_cr_selected = gGRAY(64);
		m_cr_back = gGRAY(32);
		m_cr_back_thumb = gRGB(255, 0, 0);
		m_cr_scroll = m_cr_title;
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
	Gdiplus::Graphics g(pDC->GetSafeHdc());

	CString str;
	CClientDC dc(this);

	GetClientRect(rc);

	if (pDC == NULL)
		pDC = &dc;

	if (draw)
		pDC->FillSolidRect(rc, m_cr_back.ToCOLORREF());

	g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	//DrawRectangle(pDC, rc, red, 5UL, 2);

	CFont* pOldfont;

	pOldfont = (CFont*)pDC->SelectObject(&m_font);

	if (draw)
		pDC->SetBkMode(TRANSPARENT);

	int nLineCount = 0;

	if (draw)
		::SetStretchBltMode(pDC->GetSafeHdc(), COLORONCOLOR);

	m_line_height.clear();
	std::deque<int> dqOneLineHeight;

	//���ο� ǥ���� ����� ������ ���÷��� �ϸ鼭 ����Ѵ�.
	//�Ź� ����ϴ� ���� �ټ� �δ������ �ð��� ���� 0ms��.
	//���� ���� ���� ��ũ�� ����� �����ȴ�.
	m_per_line = 1;
	//m_szGap.cx = MIN_GAP;

	//long t0 = getClock();
	while (true)
	{
		int rest = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx - m_szGap.cx * (m_per_line - 1));

		//TRACE(_T("tile = %d, %d, gap = %d, %d, margin = %d, %d, per_line = %d, rest = %d\n"),
		//	m_szTile.cx, m_szTile.cy, m_szGap.cx, m_szGap.cy, m_szMargin.cx, m_szMargin.cy, m_per_line, rest);


		//�ϳ��� ǥ���ϱ⿡�� �ʺ� ���ٸ� �߸����� �׳� ǥ���ؾ� �ϹǷ� ����.
		if (m_per_line == 1 && rest < 0)
			break;

		if (rest <= (m_szTile.cx + m_szGap.cx))
		{
			if (m_per_line == 1)
				m_szGap.cx = 0;
			else if (m_per_line == 2)
				m_szGap.cx = rest / (m_per_line);
			else
				m_szGap.cx += ((double)rest / (double)(m_per_line - 1));
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

	//��ũ�ѹ�
	if ((m_scroll_total > rc.Height()) && m_scroll_trans < 1.0)
	{
		int pos = (double)(rc.Height() - m_scroll_grip_size) * ((double)(-m_scroll_pos) / (double)(m_scroll_total - rc.Height()));
		//TRACE(_T("%d / %d, pos = %d\n"), m_scroll_pos, m_scroll_total, pos);
		int round = 2;
		CBrush brush(m_cr_scroll.ToCOLORREF());
		CBrush* pOldBrush = (CBrush*)pDC->SelectObject(&brush);
		CPen* pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
		//pDC->FillSolidRect(m_rScroll, m_cr_scroll);
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

	for (i = 0; i < m_thumb.size(); i++)
	{
		//trace(_T("draw? = %d (%3d)"), draw, i);

		if (draw && ((rect.top > rc.bottom) || (rect.bottom + 80) < rc.top))
			skip = true;
		else
			skip = false;

		//trace(_T(", skip = %d\n"), skip);

		CRect	rTile = rect;
		CRect	fit;

		//Ÿ�� ũ���� �簢�� ǥ��
		//rTile.InflateRect( 1, 1 );
		//DrawRectangle( &dc, rTile, get_color(m_cr_back, 48), NULL_BRUSH );

		if (m_thumb[i].img == NULL || m_thumb[i].img->is_empty())
		{
			fit = rTile;
			if (draw && !skip)
			{
				pDC->SetTextColor(deeppink);
				pDC->DrawText(_T("��"), rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
			}
		}
		else
		{
			//scvDrawImage(&dc, m_thumb[i].mat, rect.left, rect.top, 0, 0, NULL, m_cr_back_thumb, -1.0);
			//scvDrawImage(&dc, m_thumb[i].mat, rect, m_cr_back_thumb, -1.0);
			fit = get_ratio_rect(rect, (double)m_thumb[i].img->width / (double)m_thumb[i].img->height);
			if (draw && !skip)
			{
				//��ü���ִ� �������� �׷��ִ� �ڵ��ε� ����� £�� ȸ�� �迭�̸� �� ǥ�ð� �ȳ��� �ϴ� ��ŵ.
				rShadow = rTile;// fit;
				//rShadow.top = rect.bottom - fit.Height();
				//rShadow.bottom = rShadow.top + fit.Height();
				//rShadow.InflateRect(13, 13, 0, 0);
				_shadow.Paint(
					*pDC, rShadow,
					5,//CExtWndShadow::DEF_SHADOW_SIZE,
					85,//CExtWndShadow::DEF_BRIGHTNESS_MIN,
					CExtWndShadow::DEF_BRIGHTNESS_MAX,
					false
				);
#if USE_OPENCV
				cv_draw(pDC, m_thumb[i].img, fit.left, rect.bottom - fit.Height(), fit.Width(), fit.Height());
#else
				m_thumb[i].img->draw(g, fit.left, rect.bottom - fit.Height(), fit.Width(), fit.Height());
#endif
			}
			//DrawSunkenRect(&dc, rect, true, get_color(m_cr_back, -16), get_color(m_cr_back, +16));
		}

		//����� �ε����� ǥ���Ѵ�.
		if (draw && m_show_index)
		{
			str.Format(_T("%d"), i + 1);

			CRect rIndex = rect;
			//rIndex.top += 2;
			if (draw && !skip)
			{
				//pDC->SetTextColor(m_cr_index.ToCOLORREF());

				//��Ƽ����Ʈ ȯ�濡���� DrawShadowText�� �������ؼ��� manifest�� �߰��ؾ� �ϴ� ��
				//���ŷο� �ϴ� DrawTextShadow�� ��ü�Ѵ�.
#ifdef _UNICODE
				DrawShadowText(pDC->GetSafeHdc(), str, str.GetLength(), rIndex,
					DT_CENTER | DT_TOP | DT_NOCLIP, RGB(255, 255, 255), 0, 2, 1);
#else
				draw_text(g, rIndex, str, 50.0f, Gdiplus::FontStyleRegular, 14, 2.0F, _T("���� ���"),
					m_cr_index,
					gRGB(255, 0, 0),
					gRGB(0, 0, 255),// m_cr_index_shadow,
					Gdiplus::Color(0, 255, 255, 255),
					DT_CENTER | DT_TOP);
#endif
			}
		}

		//resolution�� title�� ǥ���Ѵ�.
		int height = 0;			//������� ǥ���ϰ� �� �Ʒ� �ػ󵵳� ���ϸ��� ǥ���ϴµ� �ʿ��� ����.
		CRect rTitle = rect;
		CRect resRect = rect;

		//resolution�� ǥ���Ѵ�.
		if (m_show_resolution)
		{
			if (draw && !skip)
				pDC->SetTextColor(m_cr_resolution.ToCOLORREF());
			resRect.top = rect.bottom;
			resRect.bottom = resRect.top + 20;
			/*
			if (find_index(m_selected, i) >= 0)
			{
			CRect rBack = resRect;
			rBack.InflateRect(4, 4);
			pDC->FillSolidRect(rBack, m_cr_selected);
			}
			*/

			if (draw && !skip)
			{
				str.Format(_T("%d x %d x %d"), m_thumb[i].width, m_thumb[i].height, m_thumb[i].channel * 8);
				//str.Format(_T("%d/%d"), m_scroll_pos, m_scroll_total);
				pDC->DrawText(str, resRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
			rTitle = resRect;
			height = height + resRect.Height();
		}

		if (m_show_title && m_thumb[i].title.GetLength())
		{
			pDC->SetTextColor(m_cr_title.ToCOLORREF());
			//DrawShadowText(pDC->GetSafeHdc(), m_thumb[i].title, _tcslen(m_thumb[i].title), rTitle,
			//				DT_CENTER | DT_VCENTER, RGB(255, 0, 0), GetComplementaryColor(m_cr_back), 2, 1);
			pDC->DrawText(m_thumb[i].title, rTitle, DT_CALCRECT | DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL | DT_END_ELLIPSIS);//DT_CENTER | DT_TOP | DT_WORDBREAK | DT_NOCLIP);
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
			pDC->FillSolidRect(rBack, m_cr_selected);
			}
			*/
			if (draw && !skip)
				pDC->DrawText(m_thumb[i].title, rTitle, DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL | DT_END_ELLIPSIS);//DT_CENTER | DT_TOP | DT_WORDBREAK | DT_NOCLIP);
			//DrawTextShadow(&dc, thumb.title, rTitle, DT_CENTER | DT_WORDBREAK|DT_EDITCONTROL);//DT_CENTER | DT_TOP | DT_WORDBREAK | DT_NOCLIP);
			//DrawTextShadow( &dc, rTitle, CString(thumb.title), DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP );
			//pDC->FillSolidRect( rTitle, RGB(255,0,0) );

			height = height + text_max_height;
		}

		m_thumb[i].rect = rect;
		m_thumb[i].rect.bottom = rect.bottom + height + 2;
		m_thumb[i].rect.InflateRect(2, 2);
		m_thumb[i].thumb_bottom = rect.bottom;
		m_thumb[i].line_index = nLineCount;


		//Ư�� keythumbnail�� ��� �� ���ʿ� �������� ���м��� ǥ��������. T1���� ���.
		if (draw && !skip && (m_thumb[i].key_thumb && (i > 0)))
		{
			pDC->MoveTo(rect.left - (int)(m_szMargin.cx / 2.0), rc.top);
			pDC->LineTo(rect.left - (int)(m_szMargin.cx / 2.0), rc.bottom);
		}

		dqOneLineHeight.push_back(height);

		//���� ���ڿ��� ǥ���Ѵ�.
		int w = m_szTile.cx / 2;
		int h = 20;
		int margin = 3;
		UINT nFormat = 0;
		UINT nFormatOld = pDC->SetTextAlign(nFormat);
		CPoint cpText;

		for (int j = 0; j < 4; j++)
		{
			if (m_show_info_text[j] == false || m_thumb[i].info[j].GetLength() == 0)
				continue;

			nFormat = 0;
			if (j % 2 == 0)
			{
				nFormat |= TA_LEFT;
				cpText.x = rect.left + margin;
			}
			else
			{
				nFormat |= TA_RIGHT;
				cpText.x = rect.left + m_szTile.cx - margin;
			}

			if (j / 2 < 1)
			{
				nFormat |= TA_TOP;
				cpText.y = rect.top + margin;
			}
			else
			{
				nFormat |= TA_BOTTOM;
				cpText.y = rect.top + m_szTile.cy - margin;
			}

			pDC->SetTextAlign(nFormat);
			//draw_text(&g, cpText.x, cpText.y, CString(m_thumb[i].info[j]), m_crInfoText[j]);
		}

		//���� text align���� ����ߴٰ� ��������� �Ѵ�.
		//�׷��� ������ DrawText���� ������ �ش�.
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

	if (dqOneLineHeight.size() && (nLineCount * m_per_line < m_thumb.size()))
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

	if (m_max_line == 1 && (dqOneLineHeight.size() <= m_thumb.size()))
		m_per_line = dqOneLineHeight.size();

	//���� �׸� ǥ��
	if (draw)
	{
		Gdiplus::Color cr_text(128, 255, 24, 16);
		Gdiplus::Font font(L"Spoqa Han Sans Neo", 32, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		Gdiplus::FontFamily fontFamily(L"���� ���");
		Gdiplus::SolidBrush br_text(cr_text);
		Gdiplus::Pen pen(Gdiplus::Color(64, 0, 0, 255), 8);

		Gdiplus::StringFormat format;
		format.SetAlignment(Gdiplus::StringAlignmentCenter);
		format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

		Gdiplus::GraphicsPath path;

		for (i = 0; i < m_selected.size(); i++)
		{
			CRect r = m_thumb[m_selected[i]].rect;
			//r.top -= 4;
			r.InflateRect(8, 4, 8, 0);

			if (m_img_selection_mark.is_valid())
			{
				r = make_center_rect(r.CenterPoint().x, r.CenterPoint().y, m_img_selection_mark.width, m_img_selection_mark.height);
				m_img_selection_mark.draw(g, r.left, r.top);
			}
			else if (m_use_circle_number)
			{
				//g.DrawString(CStringW(i2S(i)), -1, &font, CRect2GpRectF(r), &format, &br_text);

				r = make_center_rect(r.CenterPoint().x, r.CenterPoint().y, 60, 60);

				g.DrawEllipse(&pen, CRect2GpRectF(r));
				path.AddString(CStringW(i2S(i+1)), -1, &fontFamily, Gdiplus::FontStyleBold, 32, CRect2GpRectF(r), &format);
				for (int j = 0; j < 4; ++j)
				{
					Gdiplus::Pen pen(Gdiplus::Color(128, 64, 64, 64), j);		//������
					pen.SetLineJoin(Gdiplus::LineJoinRound);
					g.DrawPath(&pen, &path);
				}
				Gdiplus::SolidBrush brush(Gdiplus::Color(128, 255, 128, 255));	//���ڻ�
				g.FillPath(&brush, &path);
				path.Reset();
			}
			else
			{
				draw_rectangle(g, r, Gdiplus::Color(153, 209, 255), Gdiplus::Color(64, 204, 232, 255));
			}
		}
	}

	draw_rectangle(pDC, rc, gGRAY(192));
	pDC->SelectObject(pOldfont);
}

int CThumbCtrl::get_selected_item()
{
	if (m_selected.size())
		return m_selected[0];
	return -1;
}

int	CThumbCtrl::get_index_from_point(CPoint pt)
{
	for (int i = 0; i < m_thumb.size(); i++)
	{
		if (m_thumb[i].rect.PtInRect(pt))
			return i;
	}
	return -1;
}

//dqSelected�� NULL�� �ְ� ���� ������ ���Ϲ޾� ���⵵ �Ѵ�.
int CThumbCtrl::get_selected_items(std::deque<int>* dqSelected)
{
	if (dqSelected)
	{
		dqSelected->clear();
		dqSelected->assign(m_selected.begin(), m_selected.end());
	}
	return m_selected.size();
}

//index = -1 : ��ü����
void CThumbCtrl::select_item(int index, bool select, bool make_ensure_visible)
{
#if (ADD_USING_THREAD)
	if (!m_loading_completed)
#else
	//�ε����̶�� �ش� �ε����� ���� �ȵ����� �� �ִ�.
	//�ε��� ������ ���ý��Ѿ� �Ѵ�.
	if ((index >= (int)m_thumb.size()) && (index > m_loading_index))
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
			if (m_selected.size() < m_thumb.size())
				m_selected.resize(m_thumb.size());

			m_selected.clear();

			for (i = 0; i < m_thumb.size(); i++)
				m_selected.push_back(i);
		}
		else
		{
			m_selected.clear();
			make_ensure_visible = false;
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
					if (find_index(m_selected, curr) < 0)
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
		_stprintf_s(msg, _countof(msg), _T("�˻�� �Է��ϼ���."));
		int res = CWin32InputBox::InputBox(_T("�˻��� �Է�"), msg, buf, 200, false);

		if (res == IDCANCEL)
			return dqFound;

		text = buf;
	}


	for (i = 0; i < m_thumb.size(); i++)
	{
		if (m_thumb[i].full_path.Find(text) >= 0)
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
	if (m_thumb.size() == 0 || index < 0 || index >= m_thumb.size())
		return;

	CRect rc;
	GetClientRect(rc);

	//TRACE(_T("thumb rect = %s\n"), GetRectInfoString(m_thumb[index].rect, 1));
	//������� rect�� rc ������ �������� ������ ��ũ�ѽ�Ų��.
	if (rect_in_rect(rc, m_thumb[index].rect) == false)
	{
		set_scroll_pos(-(m_thumb[index].thumb_bottom - m_thumb[0].thumb_bottom - rc.Height() / 2 + m_thumb[index].rect.Height() / 2));
	}
}

void CThumbCtrl::on_key_down(int key)
{
	if (m_thumb.size() == 0)
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
	else if (key == VK_RIGHT && selected < m_thumb.size() - 1)
	{
		selected++;
	}
	else if (key == VK_UP && selected >= m_per_line)
	{
		selected -= m_per_line;
	}
	else if (key == VK_DOWN && selected <= m_thumb.size() - 1 - m_per_line)
	{
		selected += m_per_line;
		//if (selected > m_thumb.size() - 1)
		//	selected = m_thumb.size() - 1;
	}
	else if (key == VK_HOME && selected > 0)
	{
		selected = 0;
	}
	else if (key == VK_END && selected < m_thumb.size() - 1)
	{
		selected = m_thumb.size() - 1;
	}
	else if (key == VK_PRIOR && selected >= m_per_line * 3)
	{
		selected -= m_per_line * 3;
		//if (selected < 0)
		//	selected = 0;
	}
	else if (key == VK_NEXT && selected <= m_thumb.size() - 1 - m_per_line * 3)
	{
		selected += m_per_line * 3;
		//if (selected > m_thumb.size() - 1)
		//	selected = m_thumb.size() - 1;
	}

	select_item(selected);
}

BOOL CThumbCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)// || pMsg->message == WM_KEYUP)
	{
		if (false)//m_in_editing)
		{
			::SendMessage(m_pEdit->m_hWnd, pMsg->message, pMsg->wParam, pMsg->lParam);
			return true;
		}

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
			{
				::SendMessage(m_pEdit->m_hWnd, pMsg->message, pMsg->wParam, pMsg->lParam);
				return true;
			}
			else
				on_key_down(pMsg->wParam);
			return true;

		case VK_ADD:
			if (m_in_editing)
				break;
			enlarge_size(true);
			return true;
		case VK_SUBTRACT:
			if (m_in_editing)
				break;
			enlarge_size(false);
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
				//::SendMessage(m_pEdit->m_hWnd, pMsg->message, pMsg->wParam, pMsg->lParam);
				//return true;
			}
			else if (m_selected.size() > 0)
			{
				::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
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
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
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
		case VK_DELETE:
			if (!m_in_editing)
				on_menu_delete();
			break;
		case 'A':
			if (IsCtrlPressed())
			{
				if (m_in_editing)
				{
					m_pEdit->SetSel(0, m_thumb[m_editing_index].title.GetLength() - 4);
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
	{
		if (m_selected.size() > 0)
		{
			//��Ƽ���û���� ���� �� ������ ���õ� �׸���
			if (m_use_multi_selection)
				index = m_selected[m_selected.size() - 1];
			//�׷��� ������ ���õ� �׸��� �������� �Ѵ�.
			else
				index = get_selected_item();
		}
		else
		{
			return;
		}
	}

	DWORD dwStyle = ES_CENTER | WS_BORDER | WS_CHILD | WS_VISIBLE /*| ES_AUTOHSCROLL */ | ES_AUTOVSCROLL | ES_MULTILINE;
	CRect r = m_thumb[index].rect;
	//r.right--;
	//r.InflateRect(5, 5);
	r.top = m_thumb[index].thumb_bottom + 10;
	r.bottom = r.top + 800;

	if (m_pEdit == NULL)
	{
		m_pEdit = new CEdit();
		m_pEdit->Create(dwStyle, r, this, IDC_EDIT_CELL);
		m_pEdit->SetFont(&m_font);
		//DWORD margin = m_pEdit->GetMargins();
		m_pEdit->SetMargins(0, 0);
	}

	CDC* pDC = m_pEdit->GetDC();
	CRect textRect = r;

	pDC->DrawText(m_thumb[index].title, textRect, DT_CALCRECT/*| DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL*/);

	m_in_editing = true;
	m_editing_index = index;
	m_old_title = m_thumb[index].title;

	m_pEdit->SetWindowText(m_thumb[index].title);

	//�ؽ�Ʈ ��� ���̸� ������ ����ؼ� �غ����� DrawText�� CEdit�� �ؽ�Ʈ�� �׷����� ����� �ٸ���. �׳� �̹���� ��Ȯ�ϴ�.
	//�ٸ� ��Ʈ�� ���̿� ���� �ٸ� �� ������ ���� Ȯ������ �ʾҴ�.
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
	m_pEdit->SetSel(0, m_thumb[index].title.ReverseFind('.'));
}

void CThumbCtrl::edit_end(bool valid)
{
	if (!m_in_editing)
		return;

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

		m_thumb[m_editing_index].title = sText;
		Invalidate();

		//������� �̸��� �����ϴ� ���� ���ϸ����� ���������� �� ��Ʈ�ѿ��� ó���ϸ� �ȵȴ�.
		//�뵵�� �°� ���ο��� � ���������� ���� ó���Ѵ�.
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
			(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_rename, m_editing_index), 0);
	}
}

void CThumbCtrl::set_title(int index, CString title)
{
	if (index < 0 || index > m_thumb.size() - 1)
		return;
	m_thumb[index].title = title;
	Invalidate();
}

//info text�� �������� ����Ʈ�� ���Ľ�Ų��.
void CThumbCtrl::sort_by_info(int idx)
{
	std::sort(m_thumb.begin(), m_thumb.end(),
		[idx](const CThumbImage a, const CThumbImage b)
		{
			//���ڿ����� ���������� ���� �����Ǿ� �Ѵ�.
			return (a.info[idx] > b.info[idx]);
		}
	);

	m_modified = true;
	Invalidate();
}

void CThumbCtrl::sort_by_title()
{
	std::sort(m_thumb.begin(), m_thumb.end(),
		[](CThumbImage a, CThumbImage b)
		{
			CString str0 = a.title;
			CString str1 = b.title;
			return !is_greater_with_numeric(str0, str1);
		}
	);

	//������ ����Ǹ� �ش� ������� rect������ ����ȴ�.
	//Invalidate������ �� ������ ����������
	//OnPaint�� �Ϸ�Ǳ� ���� �ٸ� �ڵ尡 ����ȴٸ�
	//������ �� �� �ִ�. �������� ������ �� Invalidate�� ȣ��������.
	recalculate_scroll_size();
	Invalidate();
}

void CThumbCtrl::sort_by_score()
{
	if (m_thumb.size() == 0)
		return;

	std::sort(m_thumb.begin(), m_thumb.end(),
		[](const CThumbImage a, const CThumbImage b)
		{
			//���ڿ����� ���������� ���� �����Ǿ� �Ѵ�.
			return (a.score > b.score);
		}
	);

	m_modified = true;
	Invalidate();
}

int	 CThumbCtrl::find_by_title(CString title, bool bWholeWord)
{
	if (title.IsEmpty())
		return -1;

	for (int i = 0; i < m_thumb.size(); i++)
	{
		if (bWholeWord)
		{
			if (m_thumb[i].title == title)
				return i;
		}
		else
		{
			if (m_thumb[i].title.Find(title) >= 0)
				return i;
		}
	}

	return -1;
}

#if USE_OPENCV
cv::Mat CThumbCtrl::get_img(int index)
{
	if (index < 0 || index >= m_thumb.size())
		return cv::Mat();

	return m_thumb[index].img;
}
#else
CSCGdiplusBitmap CThumbCtrl::get_img(int index)
{
	if (index < 0 || index >= m_thumb.size())
		return CSCGdiplusBitmap();

	return m_thumb[index].img;
}
#endif
void CThumbCtrl::set_selection_mark_image(CString image_path, int w, int h)
{
	m_img_selection_mark.load(image_path);
	if (w > 0 && h > 0)
	{
		m_img_selection_mark.resize(w, h);
		m_img_selection_mark.set_alpha(0.7);
	}
}

void CThumbCtrl::set_selection_mark_image(CString sType, UINT id, int w, int h)
{
	m_img_selection_mark.load(sType, id);
	if (w > 0 && h > 0)
	{
		m_img_selection_mark.resize(w, h);
		m_img_selection_mark.set_alpha(0.7);
	}
}
