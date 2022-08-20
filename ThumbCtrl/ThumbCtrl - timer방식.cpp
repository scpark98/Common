// ScparkThumbCtrl.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "ThumbCtrl.h"

#include "../../Common/Functions.h"
#include "../../Common/MemoryDC.h"
#include "../messagebox/Win32InputBox/Win32InputBox.h"

#define CTHUMBNAILCONTROL_CLASSNAME _T("CThumbCtrl")

#define TEXT_HEIGHT						2

// CThumbCtrl

IMPLEMENT_DYNAMIC(CThumbCtrl, CWnd)

CThumbCtrl::CThumbCtrl()
{
	if( !RegisterWindowClass() )
		return;

	m_modified		= false;

	set_color_theme(color_theme_default, false);

	m_index			= 0;
	m_loading_index	= -1;
	m_limit_count	= 0;
	m_szTile		= CSize( 128, 128 );
	m_min_width		= m_szMargin.cx * 2 + m_szTile.cx + GetSystemMetrics(SM_CXVSCROLL);
	m_szMargin		= CSize(15, 15);
	m_szGap			= CSize(20, 20);

	m_show_title	= true;
	m_show_resolution = true;
	//20/*m_nTitleHeight*/	= 20;

	m_show_index	= AfxGetApp()->GetProfileInt( _T("setting\\thumbctrl"), _T("show index"), false );
	m_has_focus		= false;

	m_bUseMultiline = true;


	for (int i = 0; i < 4; i++)
	{
		m_show_info_text[i] = true;
		m_crInfoText[i] = GetDefaultColor(i);
	}

	memset( &m_lf, 0, sizeof( LOGFONT ) );
}

CThumbCtrl::~CThumbCtrl()
{
	//KillTimer(timer_load_files);
	remove_all();
}

void CThumbCtrl::remove_all()
{
	if ( m_dqThumb.size() == 0 )
		return;

	m_selected.clear();
	m_dqThumb.clear();

	//���α׷��� ����� �� destructor()���� remove_all()�� ȣ��Ǵµ�
	//�� �� �̹� m_hWnd�� NULL�� �ȴ�. �� ���� ó���� ������� �ʾƾ� �Ѵ�.
	if ( m_hWnd == NULL )
		return;

	RecalculateScrollSize();
	Invalidate();

	m_modified = true;
	::SendMessage( GetParent()->GetSafeHwnd(),	MESSAGE_THUMBCTRL,
					(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_remove_all, 0), 0 );
}

void CThumbCtrl::remove( int nIndex, bool bRepaint /*= false*/ )
{
	if ( nIndex < 0 || nIndex >= m_dqThumb.size() )
		return;

	m_dqThumb.erase( m_dqThumb.begin() + nIndex );

	RecalculateScrollSize();

	if ( bRepaint )
		Invalidate();

	m_modified = true;
	::SendMessage( GetParent()->GetSafeHwnd(),	MESSAGE_THUMBCTRL,
		(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_remove, nIndex), 0 );
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
	ON_COMMAND_RANGE( idToggleIndex, idRemoveAll, OnPopupMenu )
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
		brush.CreateSolidBrush( m_crBack );

		wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW ;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) brush.GetSafeHandle();
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = CTHUMBNAILCONTROL_CLASSNAME;

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
	int		i, j;
	CRect	rc;
	TRACE(_T("scrollpos = %d\n"), GetScrollPos(SB_VERT));
	CRect	rect(m_szMargin.cx, m_szMargin.cy - GetScrollPos(SB_VERT), m_szMargin.cx + m_szTile.cx, m_szMargin.cy + m_szTile.cy - GetScrollPos(SB_VERT));
	CString str;

	if ( this == NULL )
		return;

	GetClientRect ( rc );

	CMemoryDC	dc(&dc1, &rc, true);

	draw_function(&dc, true);
}


BOOL CThumbCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	return false;
	//return CWnd::OnEraseBkgnd(pDC);
}

void CThumbCtrl::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont( &m_font, true );

	ASSERT(bCreated);
}

void CThumbCtrl::set_font_name(LPCTSTR sFontname, BYTE byCharSet )
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy( m_lf.lfFaceName, sFontname);
	ReconstructFont();
}

void CThumbCtrl::set_font_size( int nSize )
{
	m_lf.lfHeight = nSize;
	ReconstructFont();
}

void CThumbCtrl::set_font_bold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	ReconstructFont();
}

void CThumbCtrl::PreSubclassWindow()
{
	CWnd::PreSubclassWindow();

	//�ڱ� �ڽſ��� �ο��� ��Ʈ�� ���ٸ� null�� ���ϵȴ�.
	//dlg�� parent�� font�� ���;� �Ѵ�.
	CFont* font = GetParent()->GetFont();

	if ( font != NULL )
		font->GetObject(sizeof(m_lf),&m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT),sizeof(m_lf),&m_lf);

	//m_nDefaultHeight = m_lf.lfHeight;
	ReconstructFont();

	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	ModifyStyle ( NULL, WS_VSCROLL/*WS_BORDER | WS_EX_CLIENTEDGE *//*| WS_HSCROLL |*/  );

	m_szTile.cx = AfxGetApp()->GetProfileInt( _T("setting\\thumbctrl"), _T("tile size cx"), 128 );
	m_szTile.cy = AfxGetApp()->GetProfileInt( _T("setting\\thumbctrl"), _T("tile size cy"), 128 );

	m_min_width = m_szMargin.cx * 2 + m_szTile.cx + GetSystemMetrics(SM_CXVSCROLL);
}
/*
void CThumbCtrl::Add( CString sFile )
{
	Insert( m_dqThumb.size(), sFile );
}

void CThumbCtrl::Add( CPicture* pImage )
{
	Insert( m_dqThumb.size(), pImage );
}

void CThumbCtrl::Insert( int nIndex, CString str )
{
	CPicture*	pPicture = NULL;
	
	pPicture = new CPicture();
	thumb.Load( str );
	Insert( nIndex, pPicture );
}

void CThumbCtrl::Insert( int nIndex, CPicture* pImage )
{
	CPicture*	pPicture = NULL;

	if ( m_dqThumb.size() >= m_limit_count )
	{
		pPicture = (CPicture*)m_dqThumb.GetAt( m_dqThumb.size() - 1 );
		thumb.FreePictureData();
		delete pPicture;
		pPicture = NULL;
		m_dqThumb.RemoveAt( m_dqThumb.size() - 1 );
	}

	m_dqThumb.InsertAt( nIndex, pImage );

	SetScrollRange( SB_HORZ, 0, m_dqThumb.size() );

	SCROLLINFO si;
	memset( &si, 0, sizeof(SCROLLINFO) );

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_RANGE;// | SIF_POS;
	si.nPage = 1;
	si.nMin = 0;
	//si.nPos = m_arArray.GetSize() - 1;
	si.nMax = m_dqThumb.size() - 1;

	SetScrollInfo( SB_HORZ, &si, TRUE );

	EnableScrollBarCtrl( SB_HORZ );
	EnableScrollBar( SB_HORZ );

	Invalidate();
}
*/


void CThumbCtrl::add(std::deque<CString> files, bool reset)
{
	if (reset)
		m_dqThumb.clear();

	m_loading_index = 0;
	loading_files = files;
	SetTimer(timer_load_files, 1, NULL);
}

int CThumbCtrl::insert(int index, cv::Mat mat, CString full_path, CString title, bool key_thumb, bool modified)
{
	CThumbImage thumb;

	thumb.title = title;
	thumb.full_path = full_path;
	thumb.width = mat.cols;
	thumb.height = mat.rows;
	thumb.channel = mat.channels();

	int nw, nh;
	CRect r;
	
	if (mat.rows != 0)
		r = GetRatioRect(CRect(0, 0, m_szTile.cx, m_szTile.cy), (double)mat.cols / (double)mat.rows);
	else
		r = CRect();
	nw = r.Width();
	nh = r.Height();

	//���� r�� width�� 4�� ����� �ƴ϶�� ���� �׸��� ��ƾ���� �ε尡 ���� �ɸ���.
	//�װ� �ذ��ϱ� �������� ���θ� 4�� ����� �����ְ� ���θ� �ణ �ٿ��� �׷�����.
	if (r.Width() % 4 != 0)
	{
		nw = (r.Width() / 4) * 4;
		nh = (double)nw / ((double)mat.cols / (double)mat.rows);
	}

	if (nw || nh)
	{
		//����Ҷ��� INTER_AREA�� ����.
		cv::resize(mat, mat, cv::Size(nw, nh), 0, 0, INTER_AREA);
		mat.copyTo(thumb.mat);
	}

	if (index < 0)
	{
		m_dqThumb.push_back(thumb);
	}
	else
	{
		if (m_dqThumb.max_size() <= index)
			m_dqThumb.resize(m_dqThumb.max_size() + 8);
		m_dqThumb.insert( m_dqThumb.begin() + index, thumb );
	}

	RecalculateScrollSize();
	Invalidate();

	//�� ó�� ������ ������ �ε��Ҷ��� �� �÷��״� false���� �ϰ�
	//�߰�, ������ ���� true�� ó���ؾ� �Ѵ�.
	if (modified)
		m_modified = true;

	return 0;
}

#if 0
//raw data�� ���� ��� mat���� ���� �� �� �Լ��� ȣ���ؼ� �־�����.
int CThumbCtrl::insert( int index, uint8_t* src, int width, int height, int channel, CString sTitle /*= NULL*/, bool key_thumb /*= false*/, bool bModifiedFlag /*= true*/ )
{
	//if ( index < 0 )
	//	index = m_dqThumb.size();
	/*
	//����Ͽ� ǥ���� �ִ� ������ ������ �� ������ ������� ����� �߰����ش�.
	if ( m_dqThumb.size() >= m_limit_count )
	{
		while ( m_dqThumb.size() >= m_limit_count )
			m_dqThumb.pop_front();
	}
	*/
	int x, y, c;
	CThumbImage thumb;
	
	//LLOG( L"before new and copy." );
	int width4 = MAKE4WIDTH( width );
	//printf( "width = %d, rwsize =%d\n", width, rwsize );
	TRACE( "width = %d, width4 = %d\n", width, width4 );

	//�޸� �Ҵ� ũ��� 4�� ����� ��������
	//���� �����ϴ� �޸� �������� ũ��� ���� ũ��� �����ؾ� �Ѵ�.
	thumb.data = new uint8_t[ width4 * height * channel ];
	memset( thumb.data, 0, sizeof(uint8_t) * width4 * height * channel );

	//memcpy_block( src, width, height, 0, 0, width, height, channel, thumb.data );
	memcpy_block( src, 0, 0, width, height, thumb.data, 0, 0, width4, height, width, height, channel );
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

	thumb.width		= width4;
	thumb.height	= height;
	thumb.channel	= channel;
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
		m_dqThumb.insert( m_dqThumb.begin() + index, thumb );
	}

	Invalidate();
	RecalculateScrollSize();

	//�� ó�� ������ ������ �ε��Ҷ��� �� �÷��״� false���� �ϰ�
	//�߰�, ������ ���� true�� ó���ؾ� �Ѵ�.
	if ( bModifiedFlag )
		m_modified = true;

	return index;
}
#endif

void CThumbCtrl::RecalculateScrollSize()
{
	draw_function(NULL, false);

	if ( m_dqThumb.size() == 0 )
		return;

	int x, y;

	CRect	rc;
	GetClientRect( rc );

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
	SCROLLINFO	si;
	si.cbSize = sizeof( SCROLLINFO );

	memset( &si, 0, sizeof(SCROLLINFO) );
	GetScrollInfo( SB_VERT, &si );
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nPage = 1080 + 1;

	int i;
	int totalScrollHeight = m_szMargin.cy;
	
	for (i = 0; i < m_line_height.size(); i++)
		totalScrollHeight = totalScrollHeight + (m_szTile.cy + m_line_height[i] + m_szGap.cy);

	if ( totalScrollHeight > rc.Height() )
		//SetScrollRange( SB_VERT, 0, totalScrollHeight - rc.Height() );
		si.nMax = totalScrollHeight - rc.Height() + si.nPage - 1;
	else
		//SetScrollRange( SB_VERT, 0, 0 );
		si.nMax = 0;

	SetScrollInfo( SB_VERT, &si );
}

void CThumbCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	SCROLLINFO	si;
	si.cbSize = sizeof( SCROLLINFO );

	//memset( &si, 0, sizeof(SCROLLINFO) );

	if ( GetScrollInfo( SB_HORZ, &si ) )
	{
		TRACE( "nIndex = %d, nPos = %d\n", m_index, si.nPos );
		switch ( nSBCode )
		{
			case SB_PAGELEFT	:
			case SB_LINELEFT	:	//��ũ�� ���� ���� ȭ��ǥ�� Ŭ���� ���
									if ( m_index > 0 )
									{
										m_index--;
										si.nPos = m_index;
									}
									break;
			case SB_PAGERIGHT	:
			case SB_LINERIGHT	:	//��ũ�� ���� ������ ȭ��ǥ�� Ŭ���� ���
									if ( m_index <= m_dqThumb.size() - 4 )
									{
										m_index++;
										si.nPos = m_index;
									}
									break;
			case SB_THUMBPOSITION :	//Ʈ���� ��� �̵��� ��
			case SB_THUMBTRACK	:	//Ʈ���� ��� �����̴� ����
									si.nPos = m_index = si.nTrackPos;
									break;
		}

		SetScrollPos( SB_HORZ, si.nPos );
		Invalidate();
	}

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CThumbCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	SCROLLINFO	si;
	si.cbSize = sizeof( SCROLLINFO );

	memset( &si, 0, sizeof(SCROLLINFO) );

	if ( GetScrollInfo( SB_VERT, &si ) )
	{
		//TRACE( "nIndex = %d, nPos = %d\n", m_index, si.nPos );
		switch ( nSBCode )
		{
		case SB_PAGEUP	:
		case SB_LINEUP	:	//��ũ�� ���� ����(����) ȭ��ǥ�� Ŭ���� ���
			si.nPos -= ((m_szTile.cy + 20/*m_nTitleHeight*/ + m_szGap.cy) >> 2);
			break;
		case SB_PAGEDOWN	:
		case SB_LINEDOWN	:	//��ũ�� ���� ������(�Ʒ�) ȭ��ǥ�� Ŭ���� ���
			si.nPos += ((m_szTile.cy + 20/*m_nTitleHeight*/ + m_szGap.cy) >> 2);
			break;
		case SB_THUMBPOSITION :	//Ʈ���� ��� �̵��� ��
		case SB_THUMBTRACK	:	//Ʈ���� ��� �����̴� ����
			si.nPos = si.nTrackPos;
			break;
		}

		SetScrollPos( SB_VERT, si.nPos );
		Invalidate(false);
		TRACE(_T("thumb rect 0 cy = %d\n"), m_dqThumb[0].rect.CenterPoint().y);// GetRectInfoString(m_dqThumb[0].rect, 1));

	}

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL CThumbCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if ( IsCtrlPressed() )
	{
		if ( (zDelta > 0) && (m_szTile.cx >= 200) )
			return true;
		if ( (zDelta < 0) && (m_szTile.cx <= 100) )
			return true;

		m_szTile.cx = m_szTile.cx + (zDelta > 0 ? 4 : -4);
		m_szTile.cy = m_szTile.cy + (zDelta > 0 ? 4 : -4);
		m_min_width = m_szMargin.cx * 2 + m_szTile.cx + GetSystemMetrics(SM_CXVSCROLL);

		AfxGetApp()->WriteProfileInt( _T("setting\\thumbctrl"), _T("tile size cx"), m_szTile.cx );
		AfxGetApp()->WriteProfileInt( _T("setting\\thumbctrl"), _T("tile size cy"), m_szTile.cy );
		RecalculateScrollSize();
		Invalidate();
	}
	else
	{
		int pos = GetScrollPos(SB_VERT);
		
		if ( zDelta < 0 )
			pos += ((m_szTile.cy + 20/*m_nTitleHeight*/ + m_szGap.cy) >> 2) * 3;
		else
			pos -= ((m_szTile.cy + 20/*m_nTitleHeight*/ + m_szGap.cy) >> 2) * 3;

		int min, max;
		GetScrollRange(SB_VERT, &min, &max);
		Clamp(pos, min, max);
		SetScrollPos(SB_VERT, pos);
		Invalidate(false);
	}
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

//�ʱ� ������ 1�����θ� �����Ǿ��� ctrl�� height�� ���� ������� ũ�⸦ �ڵ� ����������
//����� ũ��� ����ڰ� ������ �� �ְ� ��Ƽ�������� ��������.
void CThumbCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	//Invalidate();

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	/*
	CRect	rect;
	GetClientRect( rect );

	//��ũ�ѹ��� ũ��� Ŭ���̾�Ʈ ������ ���Ե��� �ʴ´�.
	m_szTile.cy = rect.Height() - m_szMargin.cy * 2 - ( m_show_title ? TEXT_HEIGHT : 0 );
	m_szTile.cx = m_szTile.cy;

	m_nTitleHeight = rect.Height() - m_szTile.cy - m_szMargin.cy;

	Invalidate();
	*/

	//���� ����� ��Ŀ����� thumb size�� �ڵ����� ���ϴ°� �ƴ϶� Ư�������� �������ְ�
	//���� ��ũ�ѹ��� ������ �޶����� �ȴ�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	SetFocus();

	CRect rc;
	GetClientRect(rc);

	for (int i = 0; i < m_dqThumb.size(); i++)
	{
		if (m_dqThumb[i].rect.PtInRect(point))
		{
			if (!IsCtrlPressed())
				m_selected.clear();

			m_selected.push_back(i);

			//���õ� �׸��� rc�ȿ� �� �ȵ����� �Ϻθ� ���̸� �� ���̵��� ��ũ�� �����ش�.
			if (RectInRect(rc, m_dqThumb[i].rect) == false)
			{
				//int max_height = m_line_height[m_dqThumb[i].line_index];
				SetScrollPos(SB_VERT, m_dqThumb[i].rect.CenterPoint().y - m_dqThumb[0].rect.CenterPoint().y - rc.Height() / 2);
			}

			Invalidate();
			return;
		}
	}

	CWnd::OnLButtonDown(nFlags, point);
}


void CThumbCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_THUMBCTRL,
		(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_LButtonDblClk, m_selected[0]), 0 );

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CThumbCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	CWnd::OnLButtonUp(nFlags, point);
}


void CThumbCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	CWnd::OnMouseMove(nFlags, point);
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	CMenu	menu;

	menu.CreatePopupMenu();
	menu.AppendMenu( MF_STRING, idToggleIndex, _T("Display thumbnail index(&I)") );
	menu.AppendMenu( MF_STRING, idPromptMaxThumb, _T("Set max thumbnails(&A)...") );
	menu.AppendMenu( MF_SEPARATOR);
	menu.AppendMenu( MF_STRING, idDeleteThumb, _T("Delete selected thumbnail(&D)") );
	menu.AppendMenu( MF_SEPARATOR);
	menu.AppendMenu( MF_STRING, idRemoveAll, _T("Remove all thumbnails...(&R)") );

	menu.CheckMenuItem( idToggleIndex, m_show_index ? MF_CHECKED : MF_UNCHECKED );

	SetMenu( &menu );

	Invalidate();
	Wait(1);


	ClientToScreen( &point );
	menu.TrackPopupMenu( TPM_LEFTALIGN, point.x, point.y, this );

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

void CThumbCtrl::OnPopupMenu( UINT nMenuID )
{
	switch ( nMenuID )
	{
		case idToggleIndex :
								m_show_index = !m_show_index;
								AfxGetApp()->WriteProfileInt( _T("setting\\thumbctrl"), _T("show index"), m_show_index );
								Invalidate();
								break;

		case idPromptMaxThumb :
							{
								TCHAR buf[200]= {0};
								TCHAR msg[200]= {0};

								_stprintf( msg, _T("ǥ���� ������� �ִ� ������ �Է��ϼ���(0 ~ 999, 0:���Ѿ���).\n���� ������ = %d"), m_limit_count );
								int r = CWin32InputBox::InputBox( _T("����� �ִ� ����"), msg, buf, 200, false );

								if ( r == IDCANCEL )
									return;

								int n = _ttoi( buf );

								if ( n <= 0 )
									return;

								if ( n > 999 )
									n = 999;

								m_limit_count = n;

								//����Ͽ� ǥ���� �ִ� ������ ������ �� ������ ������� ����� �߰����ش�.
								if ( m_limit_count && (m_dqThumb.size() >= m_limit_count) )
								{
									while ( m_dqThumb.size() > m_limit_count )
										m_dqThumb.pop_front();

									Invalidate();
								}

								AfxGetApp()->WriteProfileInt( _T("setting\\thumbctrl"), _T("limit count"), m_limit_count );
							}

		case idDeleteThumb :	//remove(m_selected, true);
								break;
		case idRemoveAll :	
							{
								int ans = AfxMessageBox( _T("Remove all thumbnails?"), MB_ICONQUESTION | MB_YESNO );
								if ( ans == IDNO )
									return;
								remove_all();
							}	
								break;
	}
}

void CThumbCtrl::set_info_text(int thumb_index, int idx, CString info, bool refresh )
{
	if ( thumb_index < 0 || thumb_index >= m_dqThumb.size() )
		return;

	if ( idx < 0 || idx >= 4 )
		return;

	m_dqThumb[thumb_index].info[idx] = info;

	if ( refresh )
		Invalidate();
}



void CThumbCtrl::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanging(lpwndpos);

	// TODO: Add your message handler code here
	//TRACE( _T("lpwndpos->cx = %d\n"), lpwndpos->cx );

	//ũ�⸦ �����Ϸ� �� ���̳� �ȸ����°Ͱ���.
	if ( lpwndpos->cx < (m_szMargin.cx * 2 + m_szTile.cx) )
	{
		lpwndpos->cx = m_szMargin.cx * 2 + m_szTile.cx;
		//return;
	}

}


void CThumbCtrl::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	RecalculateScrollSize();
	if ( !IsWindowVisible() || IsIconic() )
		return;

	Invalidate();
}

//info text�� �������� ����Ʈ�� ���Ľ�Ų��.
void CThumbCtrl::sort_by_info( int idx )
{
	std::sort( m_dqThumb.begin(), m_dqThumb.end(),
				[idx](const CThumbImage a, const CThumbImage b)
				{
					//���ڿ����� ���������� ���� �����Ǿ� �Ѵ�.
					return (a.info[idx] > b.info[idx]);
				}
	);

	m_modified = true;
	Invalidate();
}


void CThumbCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == timer_load_files)
	{
		KillTimer(timer_load_files);

		cv::Mat mat = loadMat(loading_files[m_loading_index]);
		//if (!mat.empty())
		{
			insert(-1, mat, loading_files[m_loading_index], GetFileNameFromFullPath(loading_files[m_loading_index]));
			//Invalidate();
		}
		
		m_loading_index++;
		if (m_loading_index < loading_files.size())
			SetTimer(timer_load_files, 1, NULL);
		else
			RecalculateScrollSize();
	}
	else if (nIDEvent == timer_select_after_loading)
	{
		KillTimer(timer_select_after_loading);
		if (m_index_select_after_loading > m_loading_index)
			SetTimer(timer_select_after_loading, 1000, NULL);
		else
			select_item(m_index_select_after_loading);
	}

	CWnd::OnTimer(nIDEvent);
}

void CThumbCtrl::set_color_theme(int theme, bool invalidate)
{
	switch ( theme )
	{
	case color_theme_default :
		m_crTitle		= ::GetSysColor(COLOR_BTNTEXT);
		m_crResolution	= ::GetSysColor(COLOR_BTNTEXT);
		m_crSelected	= ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBack		= ::GetSysColor(COLOR_WINDOW);
		m_crBackThumb	= ::GetSysColor(COLOR_WINDOW);
		break;
	case color_theme_dark_gray :
		m_crTitle		= GRAY164;
		m_crResolution	= GRAY164;
		m_crSelected	= GRAY64;
		m_crBack		= GRAY32;
		m_crBackThumb	= RED;
		break;
	}

	if (invalidate)
		Invalidate();
}

void CThumbCtrl::draw_function(CDC* pDC, bool draw)
{
	int		i, j;
	CRect	rc;
	CRect	rect(m_szMargin.cx, m_szMargin.cy - GetScrollPos(SB_VERT), m_szMargin.cx + m_szTile.cx, m_szMargin.cy + m_szTile.cy - GetScrollPos(SB_VERT));
	CString str;
	CClientDC dc(this);

	GetClientRect(rc);

	if (pDC == NULL)
		pDC = &dc;

	if (draw)
		pDC->FillSolidRect ( rc, m_crBack );

	CFont		font;
	CFont*		pOldfont;

	m_lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;
	m_lf.lfHeight = -16;
	m_lf.lfWeight = 500;
	_tcscpy(m_lf.lfFaceName, _T("���� ���"));
	font.CreateFontIndirect( &m_lf );
	pOldfont = (CFont*)pDC->SelectObject( &font );

	if (draw)
		pDC->SetBkMode( TRANSPARENT );

	int nLineCount = 0;

	if (draw)
		::SetStretchBltMode( pDC->GetSafeHdc(), COLORONCOLOR );

	m_line_height.clear();
	std::deque<int> dqOneLineHeight;

	//���ο� ǥ���� ����� ������ ���Ѵ�.
	//�Ź� ����ϴ� ���� �ټ� �δ������ �ð��� ���� 0ms��.
	m_per_line = 2;
	//long t0 = getClock();
	while (true)
	{
		if (m_per_line > 1)
		{
			int rest = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx - m_szGap.cx * (m_per_line - 1));
			//TRACE(_T("rest = %d, m_szGap.cx = %d, tile = %d\n"), rest, m_szGap.cx, m_szTile.cx);
			if (((m_szGap.cx) * (m_per_line - 1) > m_szTile.cx) || (rest > m_szTile.cx))
			{
				m_per_line++;
				m_szGap.cx = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx) / (m_per_line - 1);
				if (m_szGap.cx <= 24)
				{
					m_per_line--;
					m_szGap.cx = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx) / (m_per_line - 1);
					break;
				}
			}
			else if (m_szGap.cx <= 24)
			{
				ASSERT(0);
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

	//TRACE(_T("time = %ld, %ld, m_szGap.cx = %d, tile = %d\n"), t0, getClock() - t0, m_szGap.cx, m_szTile.cx);


	for ( i = 0; i < m_dqThumb.size(); i++ )
	{
		CRect	rTile = rect;
		CRect	fit;
		//rTile.InflateRect( 1, 1 );
		//DrawRectangle( &dc, rTile, get_color(m_crBack, 48), NULL_BRUSH );

		if (m_dqThumb[i].mat.empty())
		{
			fit = rTile;
			if (draw)
			{
				pDC->SetTextColor(DARKPINK);
				pDC->DrawText( _T("��"), rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER );
			}
		}
		else
		{
			//scvDrawImage(&dc, m_dqThumb[i].mat, rect.left, rect.top, 0, 0, NULL, m_crBackThumb, -1.0);
			//scvDrawImage(&dc, m_dqThumb[i].mat, rect, m_crBackThumb, -1.0);
			fit = GetRatioRect(rect, (double)m_dqThumb[i].mat.cols/(double)m_dqThumb[i].mat.rows);
			/*
			if (find_index(m_selected, i) >= 0)
			{
			CRect rBack = fit;
			rBack.InflateRect(4, 4);
			//rBack.
			pDC->FillSolidRect(rBack, m_crSelected);
			}
			*/
			if (draw)
				scvDrawImage(pDC, m_dqThumb[i].mat, fit.left, fit.top + (rect.Height() - fit.Height())/2, fit.Width(), fit.Height(), NULL, m_crBackThumb, -1.0);
			//DrawSunkenRect(&dc, rect, true, get_color(m_crBack, -16), get_color(m_crBack, +16));
		}

		//����� �ε����� ǥ���Ѵ�.
		if ( m_show_index )
		{
			if (draw)
				pDC->SetTextColor(WHITE);

			str.Format( _T("%d"), i );

			CRect rIndex = rect;
			rIndex.top += 4;
			if (draw)
				DrawTextShadow(pDC, str, rIndex, DT_CENTER | DT_TOP | DT_NOCLIP );
		}

		//resolution�� title�� ǥ���Ѵ�.
		int height = 0;			//������� ǥ���ϰ� �� �Ʒ� �ػ󵵳� ���ϸ��� ǥ���ϴµ� �ʿ��� ����.
		CRect rTitle = rect;
		CRect resRect = rect;

		//resolution�� ǥ���Ѵ�.
		if (m_show_resolution)
		{
			if (draw)
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

			if (draw)
			{
				str.Format(_T("%dx%dx%d"), m_dqThumb[i].width, m_dqThumb[i].height, m_dqThumb[i].channel);
				pDC->DrawText(str, resRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
			rTitle = resRect;
			height = height + resRect.Height();
		}

		if ( m_show_title && m_dqThumb[i].title.GetLength() )
		{
			pDC->SetTextColor(m_crTitle);
			//DrawShadowText(pDC->GetSafeHdc(), m_dqThumb[i].title, _tcslen(m_dqThumb[i].title), rTitle,
			//				DT_CENTER | DT_VCENTER, RED, GetComplementaryColor(m_crBack), 2, 1);
			pDC->DrawText(m_dqThumb[i].title, rTitle, DT_CALCRECT | DT_CENTER | DT_WORDBREAK|DT_EDITCONTROL | DT_END_ELLIPSIS);//DT_CENTER | DT_TOP | DT_WORDBREAK | DT_NOCLIP);
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
			if (draw)
				pDC->DrawText(m_dqThumb[i].title, rTitle, DT_CENTER | DT_WORDBREAK|DT_EDITCONTROL | DT_END_ELLIPSIS);//DT_CENTER | DT_TOP | DT_WORDBREAK | DT_NOCLIP);
				//DrawTextShadow(&dc, thumb.title, rTitle, DT_CENTER | DT_WORDBREAK|DT_EDITCONTROL);//DT_CENTER | DT_TOP | DT_WORDBREAK | DT_NOCLIP);
				//DrawTextShadow( &dc, rTitle, CString(thumb.title), DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP );
				//pDC->FillSolidRect( rTitle, RGB(255,0,0) );

			height = height + text_max_height;
		}

		m_dqThumb[i].rect = rect;
		m_dqThumb[i].rect.bottom = rect.bottom + height + 2;
		m_dqThumb[i].rect.InflateRect(2, 2);
		m_dqThumb[i].line_index = nLineCount;

		//thumb���� Ȯ�ο�
		if (find_index(&m_selected, i) >= 0)
		{
			CRect rBack = m_dqThumb[i].rect;
			rBack.top = fit.top + (rect.Height() - fit.Height())/2;
			rBack.InflateRect(4, 4);
			//rBack.
			if (draw)
				DrawRectangle(pDC, rBack, RGB(0, 128, 255), NULL_BRUSH, 2);
		}

		//DrawRectangle(&dc, m_dqThumb[i].rect, RED);


		//Ư�� keythumbnail�� ��� �� ���ʿ� �������� ���м��� ǥ��������. T1���� ���.
		if (draw && (m_dqThumb[i].key_thumb && i > 0))
		{
			pDC->MoveTo(rect.left - (int)(m_szMargin.cx / 2.0), rc.top);
			pDC->LineTo(rect.left - (int)(m_szMargin.cx / 2.0), rc.bottom);
		}

		dqOneLineHeight.push_back(height);
		/*
		//���� ���ڿ��� ǥ���Ѵ�.
		int w = m_szTile.cx / 2;
		int h = 20;
		int margin = 3;
		UINT nFormat = 0;
		UINT nFormatOld = pDC->SetTextAlign( nFormat );
		CPoint cpText;

		for ( j = 0; j < 4; j++ )
		{
		if ( m_show_info_text[j] == false || m_dqThumb[i].info[j].GetLength() == 0 )
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
		TextOutShadow( &dc, cpText.x, cpText.y, CString(m_dqThumb[i].info[j]), m_crInfoText[j] );
		//DrawTextShadow( &dc, m_rInfoText[j], CString(m_dqThumb[i].info[j]), DT_CENTER | DT_TOP | DT_NOCLIP, (m_crInfoText[i] == -1 ? WHITE : m_crInfoText[i]) );
		//DrawShadowText(pDC->GetSafeHdc(), CString2PCWSTR(m_dqThumb[i].info[j]), _tcslen(m_dqThumb[i].info[j]),
		//				CRect(cpText.x, cpText.y, cpText.x + m_szTile.cx, cpText.y - m_lf.lfHeight),
		//				DT_CENTER | DT_VCENTER, m_crInfoText[j], RED, 2, 1);
		}
		*/
		//���� text align���� ����ߴٰ� ��������� �Ѵ�.
		//�׷��� ������ DrawText���� ������ �ش�.
		//pDC->SetTextAlign( nFormatOld );

		rect.left += (m_szGap.cx + m_szTile.cx);
		rect.right = rect.left + m_szTile.cx;

		if ( (rect.right + m_szMargin.cx > rc.right) /*|| (nLineCount >= m_per_line)*/ )
		{
			m_per_line = dqOneLineHeight.size();

			std::deque<int>::iterator result = std::max_element(dqOneLineHeight.begin(), dqOneLineHeight.end());
			rect.left = m_szMargin.cx;
			rect.top = rect.bottom + m_szGap.cy + result[0];
			rect.right	= rect.left + m_szTile.cx;
			rect.bottom	= rect.top + m_szTile.cy;

			m_line_height.push_back(result[0]);
			dqOneLineHeight.clear();
			nLineCount++;

			//������ ����ٰ� �ؼ� Ż���ϸ� ��ü ��ũ�� ũ�Ⱑ ������ �ʴ´�.
			//�켱 �������.
			//if ( rect.top > rc.bottom )
			//break;
		}
	}

	if (nLineCount * m_per_line < m_dqThumb.size())
	{
		std::deque<int>::iterator result = std::max_element(dqOneLineHeight.begin(), dqOneLineHeight.end());
		rect.left = m_szMargin.cx;
		rect.top = rect.bottom + m_szGap.cy + result[0];
		rect.right	= rect.left + m_szTile.cx;
		rect.bottom	= rect.top + m_szTile.cy;

		nLineCount++;
		m_line_height.push_back(result[0]);
	}

	m_max_line = nLineCount;

	if (m_max_line == 1 && (dqOneLineHeight.size() <= m_dqThumb.size()))
		m_per_line = dqOneLineHeight.size();

	// 	CRect	rFocus = rc;
	// 	rFocus.DeflateRect( 1, 2 );
	// 
	// 	if ( m_has_focus )
	// 	{
	// 		pDC->MoveTo( rFocus.left,  rFocus.top );
	// 		pDC->LineTo( rFocus.right, rFocus.top );
	// 		pDC->LineTo( rFocus.right, rFocus.bottom );
	// 		pDC->LineTo( rFocus.left, rFocus.bottom );
	// 		pDC->LineTo( rFocus.left,  rFocus.top );
	// 		//		pDC->FillRect( rFocus, &brTrans );
	// 	}

	//pDC->SelectObject( pOldPen );
	pDC->SelectObject( pOldfont );

	//Pen.DeleteObject();
	font.DeleteObject();
}

int CThumbCtrl::get_selected_item()
{
	if (m_selected.size())
		return m_selected[0];
	return -1;
}

int CThumbCtrl::get_selected_items(std::deque<int> *dqSelected)
{
	return m_selected.size();
}

//index = -1 : ��ü����
void CThumbCtrl::select_item(int index, bool select, bool make_ensure_visible)
{
	if (index >= (int)m_dqThumb.size())
	{
		//�ε����̶�� �ش� �ε����� ���� �ȵ����� �� �ִ�.
		//�ε��� ������ ���ý��Ѿ� �Ѵ�.
		if (index > m_loading_index)
		{
			m_index_select_after_loading = index;
			SetTimer(timer_select_after_loading, 1000, NULL);
		}

		return;
	}

	if (index == -1)
	{
		if (select)
		{
			if (m_selected.size() < m_dqThumb.size())
				m_selected.resize(m_dqThumb.size());
			for (int i = 0; i < m_dqThumb.size(); i++)
				m_selected.push_back(i);
		}
		else
		{
			m_selected.clear();
		}
	}
	else
	{
		m_selected.clear();
		m_selected.push_back(index);
	}

	if (index >= 0 && make_ensure_visible)
		ensure_visible(index);
	else
		Invalidate();
}

void CThumbCtrl::ensure_visible(int index)
{
	if (m_dqThumb.size() == 0 || index < 0 || index >= m_dqThumb.size())
		return;

	CRect rc;
	GetClientRect(rc);

	TRACE(_T("thumb rect = %s\n"), GetRectInfoString(m_dqThumb[index].rect, 1));
	//if (RectInRect(rc, m_dqThumb[index].rect) == false)
	{
		SetScrollPos(SB_VERT, m_dqThumb[index].rect.CenterPoint().y - m_dqThumb[0].rect.CenterPoint().y - rc.Height() / 2);
		Invalidate(false);
	}
}

BOOL CThumbCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case 'A' :
			if (IsCtrlPressed())
			{
				select_item(-1, true, false);
				return true;
			}
			break;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}
