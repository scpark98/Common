// GdiButton.cpp : ���� �����Դϴ�.
//

//#include "stdafx.h"
#include "GdiButton.h"

#include "../Functions.h"
#include "../MemoryDC.h"
#include "../GdiPlusBitmap.h"

#pragma warning(disable: 4305)	//'argument': truncation from 'double' to 'Gdiplus::REAL'

#define	TIMER_BLINK		0

ColorMatrix GrayMat = {	0.30f, 0.30f, 0.30f, 0.00f, 0.00f,
						0.59f, 0.59f, 0.59f, 0.00f, 0.00f,
						0.11f, 0.11f, 0.11f, 0.00f, 0.00f,
						0.00f, 0.00f, 0.00f, 1.00f, 0.00f,
						0.00f, 0.00f, 0.00f, 0.00f, 1.00f	};

ColorMatrix HotMat = {	1.10f, 0.00f, 0.00f, 0.00f, 0.00f,		// red scaling factor
						0.00f, 1.10f, 0.00f, 0.00f, 0.00f,		// green scaling factor 
						0.00f, 0.00f, 1.10f, 0.00f, 0.00f,		// blue scaling factor 
						0.00f, 0.00f, 0.00f, 0.80f, 0.00f,		// alpha scaling factor 
						0.00f, 0.00f, 0.00f, 0.00f, 1.00f	};	// three translations 

// CGdiButton

IMPLEMENT_DYNAMIC(CGdiButton, CButton)

CGdiButton::CGdiButton()
{
	int i;

	m_button_style		= BS_PUSHBUTTON;

	m_pBack				= NULL;
	m_pBackOrigin		= NULL;
	m_bAsStatic			= false;

	m_nAnchor			= ANCHOR_NONE;
	m_nAnchorMarginX	= 0;
	m_nAnchorMarginY	= 0;

	m_bPushed			= false;
	m_use_hover			= true;
	m_bHover			= false;
	m_bIsTracking		= false;
	m_down_offset		= CPoint(1, 1);

	for (i = 0; i < 4; i++)
	{
		m_cr_text[i] = ::GetSysColor(COLOR_BTNTEXT);
		m_cr_back[i] = ::GetSysColor(COLOR_3DFACE);
	}

	m_bHasFocus			= false;
	m_bShowFocusRect	= false;
	m_crFocusRect		= RGB(6, 205, 255);
	m_nFocusRectWidth	= 2;

	m_b3DRect			= true;

	m_bBlink			= FALSE;
	m_bBlinkStatus		= FALSE;
	m_nBlinkTime0		= 400;
	m_nBlinkTime1		= 1200;

	memset(&m_lf, 0, sizeof(LOGFONT));
}

CGdiButton::~CGdiButton()
{
	release_all();
}

void CGdiButton::safe_release(Bitmap** pBitmap)
{
	if (*pBitmap == NULL)
		return;

	delete *pBitmap;
	*pBitmap = NULL;
}

void CGdiButton::release_all()
{
	safe_release(&m_pBack);
	safe_release(&m_pBackOrigin);

	for (int i = 0; i < m_image.size(); i++)
	{
		safe_release(&m_image[i].normal);
		safe_release(&m_image[i].over);
		safe_release(&m_image[i].down);
		safe_release(&m_image[i].disabled);
	}
}

BEGIN_MESSAGE_MAP(CGdiButton, CButton)
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()



// CGdiButton �޽��� ó�����Դϴ�.

//���� CButton::SetButtonStyle �Լ��� overriding�Ͽ� OWNER_DRAW�� �߰�������� �Ѵ�.
void CGdiButton::SetButtonStyle(UINT nStyle, BOOL bRedraw)
{
	DWORD dwStyle = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);

	CButton::SetButtonStyle(nStyle, bRedraw);

	// BS_OWNERDRAW �Ӽ��� �����Ѵ�.
	//���� : dwStyle�� �ݵ�� CButton::SetButtonStyle() �Լ� ȣ�� ���� ���� ���� ����ؾ� �Ѵ�.
	::SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle | BS_OWNERDRAW);
	m_button_style = nStyle;
}



bool CGdiButton::add_image(HINSTANCE hInst, LPCTSTR lpType, UINT normal, UINT over, UINT down, UINT disabled)
{
	CButtonImage btn;

	//normal�� 0�̾�� �ȵȴ�.
	if (normal == 0)
		return false;

	btn.normal = get_bitmap(hInst, lpType, MAKEINTRESOURCE(normal));
	if (!btn.normal)
		return false;

	m_width = btn.normal->GetWidth();
	m_height = btn.normal->GetHeight();

	if (over == 0)
		btn.over = gen_over_image(btn.normal);
	else if (over > 0)
		btn.over = get_bitmap(hInst, lpType, MAKEINTRESOURCE(over));

	if (down == 0)
		btn.down = gen_down_image(btn.normal);
	else if (down > 0)
		btn.down = get_bitmap(hInst, lpType, MAKEINTRESOURCE(down));

	if (disabled == 0)
		btn.disabled = gen_disabled_image(btn.normal);
	else if (disabled > 0)
		btn.disabled = get_bitmap(hInst, lpType, MAKEINTRESOURCE(disabled));

	m_image.push_back(btn);

	fit_to_image(m_fit2image);

	return true;
}

Bitmap* CGdiButton::gen_over_image(Bitmap* img)
{
	Rect r(0, 0, img->GetWidth(), img->GetHeight());
	Bitmap* over = img->Clone(r, PixelFormatDontCare);

	Graphics g(over);
	ImageAttributes ia;

	ia.SetColorMatrices(&HotMat, &GrayMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
	g.DrawImage(over, r, 0, 0, r.Width, r.Height, UnitPixel, &ia);

	return over;
}

Bitmap* CGdiButton::gen_down_image(Bitmap* img)
{
	Rect r(0, 0, img->GetWidth(), img->GetHeight());
	Bitmap* down = img->Clone(r, PixelFormatDontCare);

	Graphics g(down);
	ImageAttributes ia;

	ia.SetColorMatrices(&HotMat, &GrayMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
	g.DrawImage(down, r, 0, 0, r.Width, r.Height, UnitPixel, &ia);

	return down;
}

Bitmap* CGdiButton::gen_disabled_image(Bitmap* img)
{
	Rect r(0, 0, img->GetWidth(), img->GetHeight());
	Bitmap* disabled = img->Clone(r, PixelFormatDontCare);

	Graphics g(disabled);
	ImageAttributes ia;

	ia.SetColorMatrix(&GrayMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
	g.DrawImage(disabled, r, 0, 0, r.Width, r.Height, UnitPixel, &ia);

	return disabled;
}

void CGdiButton::fit_to_image(bool fit)
{
	if (m_image.size() == 0)
		return;

	m_fit2image = fit;

	if (m_fit2image)
	{
		m_width = m_image[0].normal->GetWidth();
		m_height = m_image[0].normal->GetHeight();
	}
	else
	{
		m_width = m_rwOrigin.Width();
		m_height = m_rwOrigin.Height();
	}

	resize_control(m_width, m_height);

	UpdateSurface();
}

void CGdiButton::select(int index)
{
	if (index < 0 || index >= m_image.size())
		return;

	m_idx = index;
	UpdateSurface();
}

Bitmap* CGdiButton::get_bitmap(HINSTANCE hInst, LPCTSTR lpType, LPCTSTR lpName, bool show_error)
{
	Bitmap	*bitmap = NULL;
	CString type = lpType;

	type.MakeLower();

	if (type == "png" || type == "jpg")
	{
		bitmap = GetImageFromResource(hInst, lpType, lpName);
	}
	else
	{
		bitmap = Bitmap::FromResource(hInst, (WCHAR*)lpName);
	}

	if (bitmap == NULL && show_error)
	{
		CString str;
		str.Format(_T("Fail to read resource image.\nCheck the image resource(file path) or Gdiplus initialization."));
		AfxMessageBox(str);
	}

	return bitmap;
}

Bitmap* CGdiButton::GdiplusImageToBitmap(Image* img, Color bkgd)
{
	Bitmap* bmp = nullptr;
	try {
		int wd = img->GetWidth();
		int hgt = img->GetHeight();
		auto format = img->GetPixelFormat();
		bmp = new Bitmap(wd, hgt, format);
		auto g = std::unique_ptr<Graphics>(Graphics::FromImage(bmp));
		g->Clear(bkgd);
		g->DrawImage(img, 0, 0, wd, hgt);
	}
	catch (...) {
		// this might happen if img->GetPixelFormat() is something exotic
		// ... not sure
	}
	return bmp;
}

#if 0
//�� ��ư�� �ִ� 10���� �̹����� ���� �� ������ 0���� normal���� �̹���, �׸��� checked ���·� ���۵ȴ�.
bool CGdiButton::AddImage(CString sfile, Bitmap* pBack, COLORREF crBack, bool bFitToImage, int dx, int dy, UINT nAnchor, bool bAsStatic, bool bShowError)
{
	Bitmap	*bitmap = Load(sfile);

	if (bitmap == NULL)
	{
		if (bShowError)
			AfxMessageBox(sfile + "\n\n�� ������ �������� �ʰų� �� �� �����ϴ�.");
		return false;
	}

	return AddImage(bitmap, pBack, crBack, bFitToImage, dx, dy, nAnchor, bAsStatic, bShowError);
}
bool CGdiButton::AddImage( HINSTANCE hInst, LPCTSTR lpName, LPCTSTR lpType, Bitmap* pBack, COLORREF crBack, bool bFitToImage, int dx, int dy, UINT nAnchor, bool bAsStatic, bool bShowError)
{
	Bitmap	*bitmap = NULL;
	CString str = lpType;

	if (str == "PNG")
		bitmap = GetImageFromResource(hInst, lpType, lpName);
	else if (str == "JPG")
	{
		HRSRC hResource = FindResource(hInst, lpName, TEXT("JPG"));

		if (!hResource) 
			return false;

		DWORD imageSize = SizeofResource(hInst, hResource);
		HGLOBAL hGlobal = LoadResource(hInst, hResource);
		LPVOID pData = LockResource(hGlobal);
		HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE,imageSize);
		LPVOID pBuffer = GlobalLock(hBuffer);

		CopyMemory(pBuffer,pData,imageSize);
		GlobalUnlock(hBuffer);
		IStream *pStream;

		HRESULT hr=CreateStreamOnHGlobal(hBuffer,TRUE,&pStream);
		Gdiplus::Image I(pStream);
		pStream->Release();

		if (I.GetLastStatus() != Gdiplus::Ok)
			return false;
		bitmap = GdiplusImageToBitmap(&I);
	}
	else
	{
		bitmap = Bitmap::FromResource(hInst, (WCHAR*)lpName);
	}

	if (bitmap == NULL)
	{
		if (bShowError)
		{
			str.Format(_T("Fail to read resource image.\nCheck the image resource(file path) or Gdiplus initialization."));
			AfxMessageBox(str);
		}
		return false;
	}

	return AddImage(bitmap, pBack, crBack, bFitToImage, dx, dy, nAnchor, bAsStatic, bShowError);

}
bool CGdiButton::AddImage(Bitmap *bitmap, Bitmap* pBack, COLORREF crBack, bool bFitToImage, int dx, int dy, UINT nAnchor, bool bAsStatic, bool bShowError)
{
	//m_pImage[m_nImages] = new Bitmap(bitmap->GetWidth(), bitmap->GetHeight(), PixelFormat32bppARGB);
	Rect sourceRect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	m_pImage.push_back(bitmap->Clone(sourceRect, PixelFormatDontCare));
	delete bitmap;

	m_nAnchor = nAnchor;

	CRect	rc;
	GetWindowRect(rc);

	m_crBack = crBack;
	m_bFitToImage = bFitToImage;

	if (m_pImage.size() == 0)
	{
		if (m_bFitToImage)
		{
			m_width	= m_pImage[0]->GetWidth();
			m_height = m_pImage[0]->GetHeight();

			ResizeControl(dx, dy);
		}
		else
		{
			//m_pImage[m_nImages] = (Bitmap*)(m_pImage[m_nImages]->GetThumbnailImage(rc.Width(), rc.Height()));
			m_width = rc.Width();// m_pImage[0]->GetWidth();
			m_height = rc.Height();// m_pImage[0]->GetHeight();
		}

		SafeRelease(&m_pBack);
		SafeRelease(&m_pBackOrigin);

		if (pBack)
		{
			Rect rc(0, 0, pBack->GetWidth(), pBack->GetHeight());
			m_pBackOrigin = pBack->Clone(rc, PixelFormatDontCare);
			SetBackImage(pBack);
		}

		m_nIndex = 0;
		m_bAsStatic = bAsStatic;
	}
	else
	{
		//�߰��Ǵ� �̹������� ��� 0�� �̹����� ������ ũ�⿩�� �Ѵ�.
		//m_pImage[m_nImages] = (Bitmap*)(m_pImage[m_nImages]->GetThumbnailImage(m_width, m_height));
	}

	return true;
}
void CGdiButton::SelectImage(int nIndex)
{
	if (nIndex < 0 || nIndex >= m_pImage.size())
		return;

	m_nIndex = nIndex;
	UpdateSurface();
}
#endif

//true�̸� 1�� �̹����� ǥ�����ش�. false�� ���� 0���� ǥ���ϴµ� ���� ������ AddGrayImage�� ȸ�� �̹����� �ڵ� �߰��� �� �������ش�.
//�������� AddImage���� checkbox�� radiobutton�̸� �˾Ƽ� grayimage�� �߰��������� �Ʒ��� ���� ������ �־
//�ڵ� �߰��� �����Ǿ���.
//����1. AddImage�ܰ迡���� �߰����� AddImage�� ������ ������ �� �� �����Ƿ� �� �ܰ迡�� gray image�� �ִ� ���� ������ �ȴ�.
//����2. GetButtonStyle()�� ��ư Ÿ���� �� ���е��� �ʴ´�. ���� ��Ȯ�� ����� ã�ƾ� �Ѵ�.
//check�� radio�� on, off�� �ϳ��� �̹����θ� ������ ����Ѵٸ�
//on�̹����� �߰��� �� AddGrayImage()�� �߰��� ȣ���Ͽ� �������.
void CGdiButton::SetCheck(bool bCheck)
{
	m_idx = bCheck;

	//radio ��ư�� �������ų� SetCheck(true)�� ȣ��Ǹ�
	//���� group ���� �ٸ� ��ư���� unchecked�� ����� ����Ѵ�.
	//owner draw �Ӽ������� WindowProc�� ������ �⺻ �޽����� �޾Ƽ� ó���� �� ����.
	if ((m_button_style == BS_RADIOBUTTON) && bCheck)
	{
		CWnd *pWndParent = GetParent(); 
		CWnd *pWnd = pWndParent->GetNextDlgGroupItem(this); 
		CWnd* pWndFirstCheck = NULL;

		//���� ��ư�� visible�� false�̸� �Ʒ� while���� ���ѷ����� ����.
		//�̸� �����ϱ� ���� �ѹ� �˻��� �����츦 ����ߴٰ� �� �����츦 ������ �����Ų��.
		while (pWnd && (pWnd != this) && (pWnd != pWndFirstCheck)) 
		{ 
			if (pWnd->GetStyle() & WS_GROUP)
				TRACE(_T("%p : group property\n"), pWnd);
			else
				TRACE(_T("%p : not group property\n"), pWnd);

			if (pWnd->IsKindOf(RUNTIME_CLASS(CGdiButton))) 
			{ 
				if (((CGdiButton*)pWnd)->m_button_style == BS_RADIOBUTTON)
				{
					//((CGdiButton*)pWnd)->SetCheck(BST_UNCHECKED); 
					((CGdiButton*)pWnd)->m_idx = 0;
					((CGdiButton*)pWnd)->UpdateSurface();
				}
			} 

			if (pWndFirstCheck == NULL)
				pWndFirstCheck = pWnd;

			pWnd = pWndParent->GetNextDlgGroupItem(pWnd); 
		}
	}
}

Bitmap* CGdiButton::Load(CString sfile)
{
	Bitmap*	bitmap = NULL;

	BSTR bstrFilename = sfile.AllocSysString();;
	bitmap = Bitmap::FromFile(bstrFilename);
	::SysFreeString(bstrFilename);

	if (bitmap->GetLastStatus() == Gdiplus::Ok)
		return bitmap;

	return NULL;
}

Bitmap*	CGdiButton::GetImageFromResource(HINSTANCE hInst, LPCTSTR lpType, LPCTSTR lpName)
{
	HRSRC hResource = FindResource(hInst, lpName, lpType);

	if (!hResource)
		return NULL;

	DWORD imageSize = SizeofResource(AfxGetApp()->m_hInstance, hResource);
	HGLOBAL hGlobal = LoadResource(AfxGetApp()->m_hInstance, hResource);
	LPVOID pData = LockResource(hGlobal);
	HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
	LPVOID pBuffer = GlobalLock(hBuffer);

	CopyMemory(pBuffer,pData,imageSize);
	GlobalUnlock(hBuffer);

	IStream *pStream;
	HRESULT hr = CreateStreamOnHGlobal(hBuffer, TRUE, &pStream);
	Image image(pStream);
	pStream->Release();

	if (image.GetLastStatus() != Ok)
		return NULL;

	Bitmap* pBitmap = static_cast<Bitmap*>(image.Clone());

	return pBitmap;
}

void CGdiButton::SetBackImage(Bitmap* pBack)
{
	if (pBack == NULL)
		return;

	if (m_pBack)
		safe_release(&m_pBack);

	CRect	rc;
	CPoint	pt;

	GetWindowRect(rc);
	pt = rc.TopLeft();
	::ScreenToClient(GetParent()->m_hWnd, &pt);

	Rect cutRect(pt.x, pt.y, rc.Width(), rc.Height());
	m_pBack = pBack->Clone(cutRect, pBack->GetPixelFormat());

	UpdateSurface();
}

CGdiButton& CGdiButton::text(CString text)
{
	m_text = text;
	UpdateSurface();

	return *this;
}

CGdiButton& CGdiButton::text_color(COLORREF normal, COLORREF over, COLORREF down, COLORREF disabled)
{
	m_cr_text[0] = normal;
	m_cr_text[1] = over;
	m_cr_text[2] = down;
	m_cr_text[3] = disabled;

	UpdateSurface();

	return *this;
}

CGdiButton& CGdiButton::back_color(COLORREF normal, COLORREF over, COLORREF down, COLORREF disabled)
{
	if (m_pBack)
		safe_release(&m_pBack);

	m_cr_back[0] = normal;
	m_cr_back[1] = over;
	m_cr_back[2] = down;
	m_cr_back[3] = disabled;

	UpdateSurface();

	return *this;
}

void CGdiButton::SetBrightnessHoverEffect(float fScale)	//1.0f = no effect.
{
	HotMat.m[0][0] = fScale;
	HotMat.m[1][1] = fScale;
	HotMat.m[2][2] = fScale;
}

//�׸��� ũ�⿡ �°� ��Ʈ���� resize�ϰ� dx, dy, nAnchor�� ���� move���ش�.
void CGdiButton::resize_control(int cx, int cy)
{
	CRect	rc, rParentRect;

	GetParent()->GetClientRect(&rParentRect);

	//parent�� Ŭ���̾�Ʈ ���� ������ �� ��Ʈ���� ��ǥ�� ���Ѵ�.
	//ScreenToClient(rc);�� ::ScreenToClient(GetParent()->m_hWnd, &pt);�� rc.topleft�� pt�� �ٸ� ����� ���δ�.
	GetWindowRect(rc);
	//ScreenToClient(rc);
	CPoint	pt = rc.TopLeft();
	::ScreenToClient(GetParent()->m_hWnd, &pt);
	/*
	if (cx < 0)
		dx = pt.x;

	if (dy < 0)
		dy = pt.y;

	if (m_nAnchor > ANCHOR_NONE)
	{
		if (m_nAnchor & ANCHOR_LEFT)
			dx = m_nAnchorMarginX;

		if (m_nAnchor & ANCHOR_TOP)
			dy = m_nAnchorMarginY;

		if (m_nAnchor & ANCHOR_RIGHT)
			dx = rParentRect.right - m_width - m_nAnchorMarginX;

		if (m_nAnchor & ANCHOR_BOTTOM)
			dy = rParentRect.bottom - m_height - m_nAnchorMarginY;

		if (m_nAnchor & ANCHOR_HCENTER)
			dx = rParentRect.CenterPoint().x - m_width / 2;

		if (m_nAnchor & ANCHOR_VCENTER)
			dy = rParentRect.CenterPoint().y - m_height / 2;
	}
	*/
	//w, h�� +1�� �� �� ������ ��ư�� ũ�Ⱑ fit�ϸ� ������ �� (x+1), (y+1)�� �׷����Ƿ�
	//�����ʰ� �ϴ��� 1�ȼ��� �Ⱥ��̰� �ȴ�. ���� ��ư�� ũ��� ���� �̹����� w, h���� 1�� �� Ŀ�� �Ѵ�.
	SetWindowPos(NULL, 0, 0, m_width, m_height, SWP_NOMOVE | SWP_NOZORDER);
}
/*
bool CGdiButton::AddGrayImage(bool bSelectGray)
{
	if (m_pImage.size() == 0)
		return false;

	Rect rc(0, 0, m_width, m_height);

	//gray image�� �׻� 0���� ��ġ�Ѵ�. ���� 0���� �����ϸ� �����...
	if (m_pImage.size() == 2)
	{
		SafeRelease(&m_pImage[0]);

	}
	else
	{
		m_pImage[1] = m_pImage[0]->Clone(rc, PixelFormatDontCare);
	}

	Graphics		g(m_pImage[1]);
	ImageAttributes ia;

	ia.SetColorMatrix(&GrayMat,ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
	g.DrawImage(m_pImage[0], rc, 0, 0, m_width, m_height, UnitPixel, &ia);

	m_nImages++;

	if (bSelectGray)
		SelectImage((m_nIndex = 0));

	return true;
}
*/
void CGdiButton::PreSubclassWindow()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	// Set control to owner draw
	m_button_style = getButtonStyle(m_hWnd);
	
	CString text;
	GetWindowText(text);
	//TRACE(_T("%s = %d\n"), str, m_button_style);
	
	ModifyStyle(0, BS_OWNERDRAW, SWP_FRAMECHANGED);
	//ModifyStyle(0, WS_TABSTOP, WS_TABSTOP);

	GetWindowRect(m_rwOrigin);
	GetParent()->ScreenToClient(m_rwOrigin);
	TRACE(_T("%s : %s\n"), text, GetRectInfoString(m_rwOrigin, 0));

	CFont* font = GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf),&m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT),sizeof(m_lf),&m_lf);

	ReconstructFont();

	CButton::PreSubclassWindow();
}

void CGdiButton::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	ASSERT(bCreated);
}


BOOL CGdiButton::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	/*
	switch (pMsg->message)
	{
		case WM_LBUTTONDBLCLK	:	return true;
									pMsg->message = WM_LBUTTONDOWN;
									break;

		//LButtonDown�� Up�� �⺻ �޽��� �ڵ鷯�� ���� ó���ϸ� ���ο��� ��ư Ŭ�� �̺�Ʈ�� �߻����� �ʴ´�.
		//���⼭ return false�� �ϸ� ���ο��� ��ư Ŭ�� �̺�Ʈ�� ó���ȴ�.
		//��, ���⼭�� SetCapture�� ReleaseCapture�� ����ϸ� ���� ���ο��� �̺�Ʈ�� �߻����� �ʴ´�.
		case WM_LBUTTONDOWN		:	if (m_bAsStatic)
										return true;

									m_bPushed = true;

									UpdateSurface();
									return false;

		case WM_LBUTTONUP		:	if (m_bAsStatic)
										return true;

									if (m_bPushed)
									{
										m_bPushed = false;

										CRect	rc;
										GetWindowRect(rc);

										//��ư ���� ������ ���߸� �ǹ��ִ�.
										//���� ��ư ��Ÿ���� check box �̸� ��۵ǰ�
										//radio button �̸� ���� �׷쳻�� �ٸ� radio button���� unchecked ��Ų��.
										if (rc.PtInRect(pMsg->pt))
										{
											if (m_button_style == BS_CHECKBOX)
												Toggle();
											else if (m_button_style == BS_RADIOBUTTON)
												SetCheck(true);
										}

										UpdateSurface();
									}

									return false;
	}
	*/
	return CButton::PreTranslateMessage(pMsg);
}

#define MAP_STYLE(src, dest) if(dwStyle & (src)) dwText |= (dest)

void CGdiButton::DrawItem(LPDRAWITEMSTRUCT lpDIS/*lpDrawItemStruct*/)
{
	if (IsWindowVisible() == false)
		return;

	CDC*		pDC1 = CDC::FromHandle(lpDIS->hDC);
	CRect		rc;
	CRect		rText;
	CPoint		pt(0, 0);
	Bitmap*		pImage = NULL;
	CString		text;
	COLORREF	cr_text = m_cr_text[0];
	COLORREF	cr_back = m_cr_back[0];
	DWORD		dwStyle = GetStyle();
	DWORD		dwText = 0;

	GetClientRect(rc);
	GetWindowText(text);

	rText = rc;

	//����� �׷��ְ� �̹����� �׸��Ƿ� ���⼭�� CMemoryDC�� �̿��Ͽ� ������۸��� ����� �ȱ����δ�.
	CMemoryDC	dc(pDC1, &rc);//, true);
	Graphics	g(dc.m_hDC, rc);

	bool is_down = (m_is_down || lpDIS->itemState & ODS_SELECTED);
	bool is_disabled = (lpDIS->itemState & ODS_DISABLED);

	int idx = MIN(m_idx, m_image.size()-1);
	if (idx < 0)
		return;

	//check or radio�ε� m_image.size()�� 1���̶��
	//checked = normal��, unchecked = disabled�� ǥ���Ѵ�.
	bool use_disabled_image = false;
	if (m_button_style != BS_PUSHBUTTON && m_image.size() == 1 && m_idx == 0)
	{
		use_disabled_image = true;
	}

	if (is_disabled)
	{
		cr_text = m_cr_text[3];
		cr_back = m_cr_back[3];
	}
	//�ٿ� �̹���. �ݵ�� hovering���� ���� üũ�Ǿ�� �Ѵ�.
	else if (is_down)
	{
		cr_text = m_cr_text[2];
		cr_back = m_cr_back[2];
	}
	//
	else if (m_use_hover && m_bHover)
	{
		cr_text = m_cr_text[1];
		cr_back = m_cr_back[1];
	}
	else
	{
		cr_text = m_cr_text[0];
		cr_back = m_cr_back[0];
	}

	//���� parent�� �����̳� ��� �׸��� �ְ�
	//�׷����� �̹����� ����� ������ PNG��� ������ �����ϰ�
	CRect Rect;
	GetWindowRect(&Rect);
	CWnd* pParent = GetParent();
	ASSERT(pParent);
	pParent->ScreenToClient(&Rect);  //convert our corrdinates to our parents
	//copy what's on the parents at this point
	CDC* pDC = pParent->GetDC();
	CDC MemDC;
	CBitmap bmp;
	MemDC.CreateCompatibleDC(pDC);
	bmp.CreateCompatibleBitmap(pDC, Rect.Width(), Rect.Height());
	CBitmap* pOldBmp = MemDC.SelectObject(&bmp);
	MemDC.BitBlt(0, 0, Rect.Width(), Rect.Height(), pDC, Rect.left, Rect.top, SRCCOPY);
	dc.BitBlt(0, 0, Rect.Width(), Rect.Height(), &MemDC, 0, 0, SRCCOPY);
	MemDC.SelectObject(pOldBmp);
	pParent->ReleaseDC(pDC);
	MemDC.DeleteDC();

	//�̹����� �ִٸ� �̹����� ���� �׷��ְ�
	if (m_image.size() > 0 && m_image[idx].normal != NULL)
	{
		RectF			grect;
		ImageAttributes ia;

		//���� ȭ�Ҹ� ���� ���������� �Ϻ� ȭ�Ҵ� �����. �׷��� �� ��ģ ����
		//g.SetInterpolationMode(InterpolationModeNearestNeighbor);

		//�ε巴�� resize������ �ణ �ѿ��� ����
		g.SetInterpolationMode(InterpolationModeHighQualityBilinear);

		//�߰� ����
		//g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

		if (m_bAsStatic)
		{
			pImage = m_image[idx].normal;
		}
		else
		{
			if (is_disabled)
			{
				pImage = m_image[idx].disabled;
			}
			//�ٿ� �̹���. �ݵ�� hovering���� ���� üũ�Ǿ�� �Ѵ�.
			else if (is_down)
			{
				pImage = (use_disabled_image ? m_image[idx].disabled : m_image[idx].down);
				pt = m_down_offset;
			}
			//
			else if (m_use_hover && m_bHover)
			{
				pImage = (use_disabled_image ? m_image[idx].disabled : m_image[idx].over);
			}
			else
			{
				pImage = (use_disabled_image ? m_image[idx].disabled : m_image[idx].normal);
			}
		}

		if (pImage == NULL)
			pImage = m_image[idx].normal;

		//����� �׸���
		if (m_pBack)
			g.DrawImage(m_pBack, 0, 0);
		//else
			//dc.FillSolidRect(rc, cr_back);

		if (m_use_hover && m_bHover && m_hover_rect)
		{
			Color color;
			color.SetFromCOLORREF(m_hover_rect_color);
			Pen pen(color, m_hover_rect_thick);
			g.DrawRectangle(&pen, Gdiplus::Rect(0, 0, m_width, m_height));
		}

		g.DrawImage(pImage, pt.x, pt.y, m_width - pt.x * 2, m_height - pt.y * 2);

		if (m_bShowFocusRect)//&& m_bHasFocus)
		{
			//TRACE(_T("draw focus rect\n"));
			//pDC->DrawFocusRect(rc);
			Color	color;

			color.SetFromCOLORREF(m_crFocusRect);
			Pen	pen(color, m_nFocusRectWidth);
			pen.SetDashStyle(DashStyleDot);
			g.DrawRectangle(&pen, rc.left, rc.top, rc.Width(), rc.Height());
		}
	}
	//������ �̹����� ���� ��� ��ư�� �̹����� �׷��ش�.
	else
	{
		if (is_disabled)
		{
			dc.FillSolidRect(rc, m_cr_back[3]);
		}
		else if (m_use_hover && m_bHover)
		{
			dc.FillSolidRect(rc, m_cr_back[2]);
		}
		else
		{
			dc.FillSolidRect(rc, m_cr_back[0]);
		}

		int		size = 6;
		CRect	r = rc;

		if (m_button_style == BS_CHECKBOX)
		{
			r.left += 3;
			r.right = r.left + size * 2 + 1;
			r.top = r.CenterPoint().y - size;
			r.bottom = r.top + size * 2 + 1;
			DrawRectangle(&dc, r, cr_text, RGB(255, 255, 255));

			Pen pen(Color(255, 32, 32, 32), 1.51);

			if (GetCheck())
			{
				g.DrawLine(&pen, r.left + 1, r.CenterPoint().y - 1, r.left + 4, r.CenterPoint().y + 3);
				g.DrawLine(&pen, r.left + 4, r.CenterPoint().y + 3, r.right - 3, r.top + 3);
			}

			rText = r;

			rText.left = rText.right + 4;
			rText.right = rc.right;
		}
		else if (m_button_style == BS_RADIOBUTTON)
		{
			r.left += 3;
			r.right = r.left + size * 2 + 1;
			r.top = r.CenterPoint().y - size;
			r.bottom = r.top + size * 2 + 1;

			Color color;
			color.SetFromCOLORREF(cr_text);
			Pen pen(color, 0.8);
			SolidBrush br(color);
			g.DrawEllipse(&pen, r.left, r.top, r.Width(), r.Height());

			if (GetCheck())
			{
				g.FillEllipse(&br, r.left + r.Width() / 4, r.top + r.Height() / 4, r.Width() / 1.7, r.Height() / 1.7);
			}

			rText = r;
			rText.left = rText.right + 4;
			rText.right = rc.right;
		}
		else
		{
			if (m_b3DRect)
			{
				dc.Draw3dRect(rc,
					is_down ? GRAY128 : white,
					is_down ? white : GRAY128
					//down_state ? GetSysColor(COLOR_3DSHADOW) : GetSysColor(COLOR_3DLIGHT),
					//down_state ? GetSysColor(COLOR_3DLIGHT) : GetSysColor(COLOR_3DSHADOW)
				);
				if (is_down)
					r.OffsetRect(-1, -1);
			}

			rText = r;
		}
	}

	if (m_text.IsEmpty())
		return;

	//���� ����� �˻���� �ʴ´�. �켱 Ǫ�ù�ư�� ������� �Ѵ�.
#if 1
	if (m_button_style == BS_PUSHBUTTON)
	{
		if ((dwStyle & BS_RIGHT) == BS_RIGHT)
			dwText |= DT_RIGHT;
		else if ((dwStyle & BS_LEFT) == BS_LEFT)
			dwText |= DT_LEFT;
		else//if (dwStyle & BS_CENTER)
			dwText |= DT_CENTER;
	}
#else		
	MAP_STYLE(BS_LEFT,	 DT_LEFT);
	MAP_STYLE(BS_RIGHT,	 DT_RIGHT);
	MAP_STYLE(BS_CENTER, DT_CENTER);
#endif
	dwText |= (DT_SINGLELINE | DT_VCENTER);

	g.SetSmoothingMode(SmoothingModeAntiAlias);
	CFont *pOldFont = dc.SelectObject(&m_font);

	dc.SetBkMode(TRANSPARENT);

	dc.SetTextColor(cr_text);

	dc.DrawText(m_text, rText, dwText);

	dc.SelectObject(pOldFont);
}


BOOL CGdiButton::OnEraseBkgnd(CDC* pDC)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (m_pBack)
		return FALSE;
	else
		return TRUE;

	return CButton::OnEraseBkgnd(pDC);
}


void CGdiButton::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (!m_bIsTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE|TME_HOVER;
		tme.dwHoverTime = 1;
		m_bIsTracking = _TrackMouseEvent(&tme);
	}

	CButton::OnMouseMove(nFlags, point);
}


void CGdiButton::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (!m_use_hover)
		return;

	m_bHover = true;
	Invalidate();

	CButton::OnMouseHover(nFlags, point);
}


void CGdiButton::OnMouseLeave()
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (!m_use_hover)
		return;

	m_bIsTracking = false;
	m_bHover = false;
	UpdateSurface();

	CButton::OnMouseLeave();
}

void CGdiButton::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (nIDEvent == TIMER_BLINK)
	{
		m_bBlinkStatus = !m_bBlinkStatus;
		ShowWindow(m_bBlinkStatus ? SW_SHOW : SW_HIDE);

		KillTimer(TIMER_BLINK);

		if (m_bBlinkStatus)
			SetTimer(TIMER_BLINK, m_nBlinkTime1, NULL);
		else
			SetTimer(TIMER_BLINK, m_nBlinkTime0, NULL);
	}

	CButton::OnTimer(nIDEvent);
}


void CGdiButton::OnSetFocus(CWnd* pOldWnd)
{
	CButton::OnSetFocus(pOldWnd);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	//TRACE("set focus\n");
	m_bHasFocus = true;
	UpdateSurface();
}


void CGdiButton::OnKillFocus(CWnd* pNewWnd)
{
	CButton::OnKillFocus(pNewWnd);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	//TRACE("kill focus\n");
	m_bHasFocus = false;
	UpdateSurface();
}


BOOL CGdiButton::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	return CButton::OnSetCursor(pWnd, nHitTest, message);
}

void CGdiButton::ReAlign()
{
	int dx, dy;
	CRect	rc, rParentRect;

	GetParent()->GetClientRect(&rParentRect);

	if (m_nAnchor > ANCHOR_NONE)
	{
		if (m_nAnchor & ANCHOR_LEFT)
			dx = m_nAnchorMarginX;

		if (m_nAnchor & ANCHOR_TOP)
			dy = m_nAnchorMarginY;

		if (m_nAnchor & ANCHOR_RIGHT)
			dx = rParentRect.right - m_width - m_nAnchorMarginX;

		if (m_nAnchor & ANCHOR_BOTTOM)
			dy = rParentRect.bottom - m_height - m_nAnchorMarginY;

		if (m_nAnchor & ANCHOR_HCENTER)
			dx = rParentRect.CenterPoint().x - m_width / 2;

		if (m_nAnchor & ANCHOR_VCENTER)
			dy = rParentRect.CenterPoint().y - m_height / 2;
	
		SetWindowPos(NULL, dx, dy, m_width, m_height, SWP_NOZORDER | SWP_NOSIZE);
		SetBackImage(m_pBackOrigin);
	}
}

//��ư�� ��ġ ����
void CGdiButton::Offset(int x, int y)
{
	CRect	rc;

	//parent�� Ŭ���̾�Ʈ ���� ������ �� ��Ʈ���� ��ǥ�� ���Ѵ�.
	GetWindowRect(rc);
	CPoint	pt = rc.TopLeft();
	::ScreenToClient(GetParent()->m_hWnd, &pt);

	pt.x += x;
	pt.y += y;

	if (pt.x < 0)
		pt.x = 0;

	if (pt.y < 0)
		pt.y = 0;

	SetWindowPos(&wndNoTopMost, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);	

	SetBackImage(m_pBackOrigin);
}

void CGdiButton::Inflate(int cx, int cy)
{
	Inflate(cx, cy, cx, cy);
}

//��ư ũ�⸦ �ø��ų� ���δ�.
//��ư �̹����� �׻� ��ư ������ �߾ӿ� �׷��ش�.
void CGdiButton::Inflate(int l, int t, int r, int b)
{
	CRect	rc;

	//parent�� Ŭ���̾�Ʈ ���� ������ �� ��Ʈ���� ��ǥ�� ���Ѵ�.
	GetWindowRect(rc);
	CPoint	pt = rc.TopLeft();
	::ScreenToClient(GetParent()->m_hWnd, &pt);

	rc.MoveToXY(pt.x, pt.y);
	rc.InflateRect(l, t, r, b);

	//��� �׸��� �����ߴٸ� ��� ���� ���� ���;� �Ѵ�.
	if (m_pBack && m_pBackOrigin)
	{
		safe_release(&m_pBack);

		Rect cutRect(rc.left, rc.top, rc.Width(), rc.Height());
		m_pBack = m_pBackOrigin->Clone(cutRect, m_pBackOrigin->GetPixelFormat());
	}


	SetWindowPos(&wndNoTopMost, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER);	
	UpdateSurface();
}

bool CGdiButton::GetCheck()
{
	return m_idx;
}

void CGdiButton::UpdateSurface()
{
	CRect rc;

	GetWindowRect(&rc);
	//RedrawWindow();

	GetParent()->ScreenToClient(&rc);
	GetParent()->InvalidateRect(rc, false);
	GetParent()->UpdateWindow();
}

void CGdiButton::Toggle()
{
	m_idx = !m_idx;
}

LRESULT CGdiButton::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

// 	CString str;
// 	GetWindowText(str);

	//TRACE("%s\n", str);
	//if (str == "Radio3")

/*
	if (message == WM_GETDLGCODE)
		return (DLGC_BUTTON | DLGC_RADIOBUTTON);

	if (message == BM_SETCHECK)
	{
		if (wParam == BST_CHECKED)
		//TRACE("%d, %d, %d\n", message, wParam, lParam);
		TRACE("checked\n");

	 	else if (message == BST_UNCHECKED)
		{
			TRACE("unchecked\n");
		}
	}
*/


	return CButton::WindowProc(message, wParam, lParam);
}

void CGdiButton::SetBlinkTime(int nTime0 /*= 500*/, int nTime1 /*= 500*/)
{
	KillTimer(TIMER_BLINK);

	m_nBlinkTime0		= nTime0;
	m_nBlinkTime1		= nTime1;

	SetBlink(m_bBlink);
}

void CGdiButton::SetBlink(BOOL bBlink /*= TRUE*/)
{
	m_bBlink = bBlink;
	m_bBlinkStatus = FALSE;

	if (m_bBlink)
	{
		SetTimer(TIMER_BLINK, m_nBlinkTime0, NULL);
	}
	else
	{
		m_bBlink = false;
		KillTimer(TIMER_BLINK);
		ShowWindow(SW_SHOW);
		//UpdateSurface();
	}
}

CGdiButton& CGdiButton::SetFontName(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	ReconstructFont();

	return *this;
}

CGdiButton& CGdiButton::SetFontSize(int nSize)
{
	m_lf.lfHeight = -MulDiv(nSize, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	ReconstructFont();

	return *this;
}

CGdiButton& CGdiButton::SetFontBold(bool bBold)
{
	m_lf.lfWeight = (bBold ? FW_BOLD : FW_NORMAL);
	ReconstructFont();

	return *this;
}

void CGdiButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	m_is_down = IsWindowEnabled();
	Invalidate();
	CButton::OnLButtonDown(nFlags, point);
}


void CGdiButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CRect rc;

	GetClientRect(rc);

	if (!m_is_down || !rc.PtInRect(point))
	{
		CButton::OnLButtonUp(nFlags, point);
		return;
	}

	m_is_down = false;

	if (m_button_style == BS_CHECKBOX)
	{
		Toggle();
	}
	else if (m_button_style == BS_RADIOBUTTON)
	{
		SetCheck((m_idx = 1));
	}

	UpdateSurface();

	CButton::OnLButtonUp(nFlags, point);
}

void CGdiButton::use_hover(bool use)
{
	m_use_hover = use;
}

void CGdiButton::set_hover_rect(int thick, COLORREF cr)
{
	m_hover_rect = true;
	m_hover_rect_thick = thick;
	m_hover_rect_color = cr;
	Invalidate();
}

void CGdiButton::set_hover_rect_thick(int thick)
{
	m_hover_rect_thick = thick;
	Invalidate();
}

void CGdiButton::set_hover_rect_color(COLORREF cr)
{
	m_hover_rect_color = cr;
	Invalidate();
}

void CGdiButton::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CButton::OnWindowPosChanged(lpwndpos);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	Invalidate();
}


void CGdiButton::OnSize(UINT nType, int cx, int cy)
{
	CButton::OnSize(nType, cx, cy);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	if (m_fit2image)
	{

	}
	else
	{
		m_width = cx;
		m_height = cy;
		resize_control(cx, cy);
	}
}

