#include "ImageEx.h"
#include <math.h>
#include "../Functions.h"

//////////////////////////////////////////////////////////////////////////////////

//뭐芚변鑒
CImageEx::CImageEx()
{
	::SetRect(&m_rcNinePart,0,0,0,0);
	m_cbAlpha = 255;

	return;
}

//驕뭐변鑒
CImageEx::~CImageEx()
{
	//DestroyImage(); 
}

//饋쁑暠튬
bool CImageEx::DestroyImage()
{
	if (CImage::IsNull() == false)
	{
		CImage::ReleaseDC();
		CImage::Destroy();
	}
	return true;
}

//속潼暠튬
bool CImageEx::LoadImage(LPCTSTR pszFileName)
{
	HRESULT hr = CImage::Load(pszFileName);
	if (hr == S_OK)
	{
		return SetAlphaBit();
	}
	else
	{
		return FALSE;
	}
}

//속潼暠튬
bool CImageEx::LoadImage(HINSTANCE hInstance, LPCTSTR pszResourceName,LPCTSTR pszResourceType/*=TEXT("IMAGE")*/)
{
	//꿴冷栗都
	HRSRC hResource=FindResource(hInstance,pszResourceName,pszResourceType);
	if (hResource==NULL) return false;

	//뗍혤栗都
	DWORD dwImageSize=SizeofResource(hInstance,hResource);
	LPVOID pImageBuffer=LoadResource(hInstance,hResource);

	//눼쉔鑒앴
	IStream * pIStream=NULL;
	if (CreateStreamOnHGlobal(NULL,TRUE,&pIStream)!=S_OK)
	{
		ASSERT(FALSE);
		return false;
	}

	//畇흙鑒앴
	pIStream->Write(pImageBuffer,dwImageSize,NULL);

	HRESULT hr = CImage::Load(pIStream);

 	//姦렴栗都
 	SAFE_RELEASE(pIStream);

	if (hr == S_OK)
	{
		return SetAlphaBit();
	}
	else
	{
		return FALSE;
	}
}

//삥뺌暠獗
BOOL CImageEx::DrawImage(CDC * pDC, INT nXPos, INT nYPos)
{
	pDC->SetStretchBltMode(HALFTONE);

	if ( m_ImageClone.IsNull() )
		return CImage::Draw(pDC->GetSafeHdc(),nXPos,nYPos,GetWidth(),GetHeight());
	else
	{
		if ( m_cbAlpha != 255 )
			m_ImageClone.AlphaBlend(pDC->GetSafeHdc(),nXPos,nYPos,GetWidth(),GetHeight(),0,0,GetWidth(),GetHeight(),m_cbAlpha);
		else m_ImageClone.Draw(pDC->GetSafeHdc(),nXPos,nYPos,GetWidth(),GetHeight());
	}

	return CImage::Draw(pDC->GetSafeHdc(),nXPos,nYPos);
}

//삥뺌暠獗
BOOL CImageEx::DrawImage( CDC * pDC, INT nXPos, INT nYPos, INT nDestWidth, INT nDestHeight )
{
	pDC->SetStretchBltMode(HALFTONE);
	if( nDestWidth == 0 || nDestHeight == 0 ) return FALSE;

	if ( m_ImageClone.IsNull() )
		return CImage::Draw(pDC->GetSafeHdc(),nXPos,nYPos,nDestWidth,nDestHeight);
	else
	{
		if ( m_cbAlpha != 255 )
			m_ImageClone.AlphaBlend(pDC->GetSafeHdc(),nXPos,nYPos,nDestWidth,nDestHeight,0,0,GetWidth(),GetHeight(),m_cbAlpha);
		else m_ImageClone.Draw(pDC->GetSafeHdc(),nXPos,nYPos,nDestWidth,nDestHeight);
	}

	return FALSE;
}

//삥뺌暠獗
BOOL CImageEx::DrawImage( CDC * pDC, RECT &rc )
{
	pDC->SetStretchBltMode(HALFTONE);
	if( rc.right-rc.left == 0 || rc.bottom-rc.top == 0 ) return FALSE;

	if ( m_ImageClone.IsNull() )
		return CImage::Draw(pDC->GetSafeHdc(),rc);
	else 
	{
		if ( m_cbAlpha != 255 )
			m_ImageClone.AlphaBlend(pDC->GetSafeHdc(),rc,CRect(0,0,m_ImageClone.GetWidth(),m_ImageClone.GetHeight()),m_cbAlpha);
		else
		{
			if ( m_cbAlpha != 255 )
				m_ImageClone.AlphaBlend(pDC->GetSafeHdc(),rc.left,rc.top,(rc.right-rc.left),(rc.bottom-rc.top),0,0,GetWidth(),GetHeight(),m_cbAlpha);
			else m_ImageClone.Draw(pDC->GetSafeHdc(),rc);
		}
	}

	return FALSE;
}

//삥뺌暠獗
BOOL CImageEx::DrawImage(CDC * pDC, INT nXDest, INT nYDest, INT nDestWidth, INT nDestHeight, INT nXScr, INT nYSrc)
{
	pDC->SetStretchBltMode(HALFTONE);
	if( nDestWidth == 0 || nDestHeight == 0 ) return FALSE;

	if ( m_ImageClone.IsNull() )
		return CImage::Draw(pDC->GetSafeHdc(),nXDest,nYDest,nDestWidth,nDestHeight,nXScr,nYSrc,GetWidth(),GetHeight());
	else 
	{
		if ( m_cbAlpha != 255 )
			m_ImageClone.AlphaBlend(pDC->GetSafeHdc(),nXDest,nYDest,nDestWidth,nDestHeight,nXScr,nYSrc,GetWidth(),GetHeight(),m_cbAlpha);
		else m_ImageClone.Draw(pDC->GetSafeHdc(),nXDest,nYDest,nDestWidth,nDestHeight,nXScr,nYSrc,GetWidth(),GetHeight());
	}

	return FALSE;
}

//삥뺌暠獗
BOOL CImageEx::DrawImage(CDC * pDC, INT nXDest, INT nYDest, INT nDestWidth, INT nDestHeight, INT nXScr, INT nYSrc, INT nSrcWidth, INT nSrcHeight)
{
	pDC->SetStretchBltMode(HALFTONE);
	if( nSrcWidth == 0 || nSrcHeight == 0 ) return FALSE;

	if ( m_ImageClone.IsNull() )
		return CImage::Draw(pDC->GetSafeHdc(),nXDest,nYDest,nDestWidth,nDestHeight,nXScr,nYSrc,nSrcWidth,nSrcHeight);
	else 
	{
		if ( m_cbAlpha != 255 )
			m_ImageClone.AlphaBlend(pDC->GetSafeHdc(),nXDest,nYDest,nDestWidth,nDestHeight,nXScr,nYSrc,nDestWidth,nDestHeight,m_cbAlpha);
		else m_ImageClone.Draw(pDC->GetSafeHdc(),nXDest,nYDest,nDestWidth,nDestHeight,nXScr,nYSrc,nSrcWidth,nSrcHeight);
	}

	return FALSE;
}

bool CImageEx::Draw( CDC * pDC, INT x, INT y, INT cx, INT cy,INT nLeft,INT nTop,INT nRight,INT nBottom )
{
	int cxImage = GetWidth();
	int cyImage = GetHeight();

	// 璘
	{
		RECT rcDest = {x, y, x+nLeft, y+nTop};
		RECT rcSrc = {0, 0, nLeft, nTop};
		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 璘긋
	{
		RECT rcDest = {x, y+nTop, x+nLeft, (y+nTop)+(cy-nTop-nBottom)};
		RECT rcSrc = {0, nTop, nLeft, nTop+(cyImage-nTop-nBottom)};
		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 긋
	{
		RECT rcDest = {x+nLeft, y,x+(cx-nRight), y+nTop};
		RECT rcSrc = {nLeft, 0, (cxImage-nLeft-nRight), nTop};
 		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
 			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
 			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 塘
	{
		RECT rcDest = {x+(cx-nRight), y, (x+(cx-nRight))+nRight, y+nTop};
		RECT rcSrc = {cxImage-nRight, 0, (cxImage-nRight)+nRight, nTop};
  		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
  			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
  			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 塘긋
	{
		RECT rcDest = {x+(cx-nRight), y+nTop, (x+(cx-nRight))+nRight, (y+nTop)+(cy-nTop-nBottom)};
		RECT rcSrc = {cxImage-nRight, nTop, (cxImage-nRight)+nRight, nTop+(cyImage-nTop-nBottom)};
  		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
  			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
  			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 苟긋
	{
		RECT rcDest = {x+nLeft, y+(cy-nBottom), (x+nLeft)+(cx-nLeft-nRight), (y+(cy-nBottom))+nBottom};
		RECT rcSrc = {nLeft, cyImage-nBottom, nLeft+(cxImage-nLeft-nRight), (cyImage-nBottom)+nBottom};
 		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
 			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
 			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 塘苟
	{
		RECT rcDest = {x+(cx-nRight), y+(cy-nBottom), (x+(cx-nRight))+nRight, (y+(cy-nBottom))+nBottom};
		RECT rcSrc = {cxImage-nRight, cyImage-nBottom, (cxImage-nRight)+nRight, (cyImage-nBottom)+nBottom};
 		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
 			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
 			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 璘苟
	{
		RECT rcDest = {x, y+(cy-nBottom), x+nLeft, (y+(cy-nBottom))+nBottom};
		RECT rcSrc = {0, cyImage-nBottom, nLeft, (cyImage-nBottom)+nBottom};
		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 櫓쇌
	{
		RECT rcDest = {x+nLeft, y+nTop, x+(cx-nRight), y+(cy-nBottom)};
		RECT rcSrc = {nLeft, nTop, cxImage-nRight, cyImage-nBottom};

   		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
   			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
   			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	return true;
}

bool CImageEx::Draw( CDC * pDC, const RECT& rectDest, const RECT& rectSrc )
{
	return Draw(pDC,rectDest.left, rectDest.top, rectDest.right-rectDest.left, rectDest.bottom-rectDest.top, 
		rectSrc.left, rectSrc.top,rectSrc.right, rectSrc.bottom);
}

bool CImageEx::Draw( CDC * pDC, const RECT& rectDest )
{
	return Draw(pDC,rectDest,m_rcNinePart);
}

bool CImageEx::DrawFrame( CDC * pDC, const RECT& rectDest )
{
	INT x = rectDest.left;
	INT y = rectDest.top;
	
	INT cx = rectDest.right-rectDest.left;
	INT cy = rectDest.bottom - rectDest.top;

	INT nLeft = m_rcNinePart.left;
	INT nTop = m_rcNinePart.top;
	INT nRight = m_rcNinePart.right;
	INT nBottom = m_rcNinePart.bottom;
 
	// 璘긋
	{
		RECT rcDest = {x, y, x+nLeft, (y+nTop)+(cy)};
		RECT rcSrc = {0, nTop, nLeft, nTop+(GetHeight()-nTop-nBottom)};
		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 긋
	{
		RECT rcDest = {x, y,x+(cx), y+nTop};
		RECT rcSrc = {nLeft, 0, (GetWidth()-nLeft-nRight), nTop};
		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 塘긋
	{
		RECT rcDest = {x+(cx-nRight), y+nTop, (x+(cx-nRight))+nRight, (y+nTop)+(cy-nTop-nBottom)};
		RECT rcSrc = {GetWidth()-nRight, nTop, (GetWidth()-nRight)+nRight, nTop+(GetHeight()-nTop-nBottom)};
		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}

	// 苟긋
	{
		RECT rcDest = {x, y+(cy-nBottom), (x)+(cx), (y+(cy-nBottom))+nBottom};
		RECT rcSrc = {nLeft, GetHeight()-nBottom, nLeft+(GetWidth()-nLeft-nRight), (GetHeight()-nBottom)+nBottom};
		if (!::IsRectEmpty(&rcDest) && !::IsRectEmpty(&rcSrc))
			DrawImage(pDC,rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, 
			rcSrc.left, rcSrc.top,rcSrc.right-rcSrc.left, rcSrc.bottom-rcSrc.top);
	}
// 	DrawImage(pDC,rectDest.left,rectDest.top,nWidth,m_rcNinePart.top,0,0,GetWidth(),m_rcNinePart.top);
// 	DrawImage(pDC,rectDest.right-m_rcNinePart.right,rectDest.top-m_rcNinePart.top,m_rcNinePart.right,nHeight,GetWidth()-m_rcNinePart.right,0,m_rcNinePart.right,GetHeight());
// 	DrawImage(pDC,rectDest.left,rectDest.bottom-m_rcNinePart.bottom,nWidth,m_rcNinePart.bottom,0,GetHeight()-m_rcNinePart.bottom,GetWidth(),m_rcNinePart.bottom);
// 	DrawImage(pDC,rectDest.left,rectDest.top-m_rcNinePart.top,m_rcNinePart.left,nHeight,0,0,m_rcNinePart.left,GetHeight());

	return true;
}

void CImageEx::SetNinePart( CONST LPRECT lprcNinePart )
{
	if( lprcNinePart == NULL ) return;

	::CopyRect(&m_rcNinePart,lprcNinePart);
}

HBITMAP CImageEx::ImageToBitmap() 
{  
	return (HBITMAP)*this;
}

bool CImageEx::DrawExtrude( CDC*pDC,const RECT& rectDest,bool bLeft,int nPixel )
{
	//눼쉔팁캥
	ASSERT(pDC!=NULL);

	int nWidth = rectDest.right - rectDest.left;
	int nHeight = rectDest.bottom - rectDest.top;

	if ( bLeft )
	{
		DrawImage(pDC,rectDest.left,rectDest.top);

		if(nWidth>GetWidth())
			DrawImage(pDC,GetWidth(),rectDest.top,(nWidth-GetWidth()),GetHeight(),GetWidth()-nPixel,0,nPixel,GetHeight());
	}
	else
	{
		if ( nWidth <=GetWidth() )
		{
			DrawImage(pDC,rectDest.left,rectDest.top,nWidth,GetHeight(),GetWidth()-nWidth,0,nWidth,GetHeight());
		}
		else
		{
			DrawImage(pDC,rectDest.left,rectDest.top,nWidth-GetWidth(),GetHeight(),nPixel,0,nPixel,GetHeight());
			DrawImage(pDC,nWidth-GetWidth(),rectDest.top);
		}
	}

	return true;
}

bool CImageEx::SetGray()
{
	CImage *pImage = &m_ImageClone;
	if ( pImage == NULL && pImage->IsNull() ) 
	{
		pImage = this;
	}

	int nWidth = pImage->GetWidth();
	int nHeight = pImage->GetHeight();

	BYTE* pArray = (BYTE*)pImage->GetBits();
	int nPitch = pImage->GetPitch();
	int nBitCount = pImage->GetBPP() / 8;

	for (int i = 0; i < nHeight; i++) 
	{
		for (int j = 0; j < nWidth; j++) 
		{
			int grayVal = (BYTE)(((*(pArray + nPitch * i + j * nBitCount) * 306)
				+ (*(pArray + nPitch * i + j * nBitCount + 1) * 601)
				+ (*(pArray + nPitch * i + j * nBitCount + 2) * 117) + 512 ) >> 10);	// 셕炬뿍똑令

			*(pArray + nPitch * i + j * nBitCount) = grayVal;							// 립뿍똑令
			*(pArray + nPitch * i + j * nBitCount + 1) = grayVal;
			*(pArray + nPitch * i + j * nBitCount + 2) = grayVal;
		}
	}

	return true;
}

bool CImageEx::SetAlphaBit()
{
	ASSERT(IsNull() == false);
	if(IsNull())return FALSE;

	if ( GetBPP() == 32 )//png暠獗
	{
		LPVOID pBitsSrc = NULL;
		BYTE * psrc = NULL;
		BITMAP stBmpInfo;

		HBITMAP hBmp = (HBITMAP)*this;

		::GetObject(hBmp, sizeof(BITMAP), &stBmpInfo);

		if (32 != stBmpInfo.bmBitsPixel || NULL == stBmpInfo.bmBits)
			return FALSE;

		psrc = (BYTE *) stBmpInfo.bmBits;

		for (int nPosY = 0; nPosY < abs(stBmpInfo.bmHeight); nPosY++)
		{
			for (int nPosX = stBmpInfo.bmWidth; nPosX > 0; nPosX--)
			{
				BYTE alpha  = psrc[3];
				psrc[0] = (BYTE)((psrc[0] * alpha) / 255);
				psrc[1] = (BYTE)((psrc[1] * alpha) / 255);
				psrc[2] = (BYTE)((psrc[2] * alpha) / 255);
				psrc += 4;
			}
		}
	}

	return TRUE;
}

void CImageEx::GetImageParament( CImage *pImg,IMAGEPARAMENT *ppImgParam )
{
	if (pImg->IsNull()) return;  

	ppImgParam->nWidth   = pImg->GetWidth();      //暠獗욱똑  
	ppImgParam->nHeight   = pImg->GetHeight();        //暠獗멕똑  
	ppImgParam->nBitCount  = pImg->GetBPP();      //첼獗羹貫鑒  
	ppImgParam->nBytesPerLine   = (pImg->GetWidth()*pImg->GetBPP()+31)/32*4;   //첼契俚쌘鑒  
	ppImgParam->nBytesPerPixel   = pImg->GetBPP()/8;      //첼獗羹俚쌘鑒  
	if (pImg->GetBPP()<=8)   
		ppImgParam->nNumColors= 1 << pImg->GetBPP();        //딧겼데禱鑒  
	else   
		ppImgParam->nNumColors= 0;  
	ppImgParam->nSize  = ppImgParam->nBytesPerLine*ppImgParam->nHeight;        //獗羹悧俚쌘鑒    
}

int CImageEx::InImage( CImage *pImg,int x,int y )
{
	IMAGEPARAMENT  P;  

	GetImageParament(pImg,&P);  
	if ((x<0)||(y<0)||(x>=P.nWidth)||(y>=P.nHeight))  return 0;  
	else  return 1;  
}

void CImageEx::SetRectValue(CImage *pImg,int x,int y,int Dx,int Dy,BYTE *buf )
{
	IMAGEPARAMENT  P;  
	BYTE    *lp;  
	int     i,dw,dh,x1,y1,alpha,delta,Dxb,dwb;  

	GetImageParament(pImg,&P);  
	if (P.nBitCount<8) return;  
	x1=x;  
	y1=y;  
	alpha=delta=0;  
	
	if (x<0) 
	{   
		alpha=-x;    x1=0;  
	}  

	if (y<0) 
	{   
		delta=-y;    y1=0;  
	}  

	if (!InImage(pImg,x1,y1)) return;  
	dw=min(Dx,(int) P.nWidth-x1);      
	dh=min(Dy,(int) P.nHeight-y1);  
	dw -= alpha;  
	dh -= delta;  

	Dxb = Dx*P.nBytesPerPixel;  
	dwb = dw*P.nBytesPerPixel;  
	lp = (BYTE*) pImg->GetPixelAddress(x1,y1);  
	buf += (delta*Dx+alpha)*P.nBytesPerPixel;  
	
	for (i=0;i<dh;i++) 
	{  
		memcpy(lp,buf,dwb);    
		buf += Dxb;   
		lp -= P.nBytesPerLine;  
	}
}

void CImageEx::GetAllPalette( CImage *pImg,RGBQUAD *ColorTab )
{
	IMAGEPARAMENT P;  
	GetImageParament(pImg,&P);  
	pImg->GetColorTable(0, P.nNumColors, ColorTab);  
}

void CImageEx::SetAllPalette( CImage *pImg,RGBQUAD *ColorTab )
{
	IMAGEPARAMENT P;  

	GetImageParament(pImg,&P);  
	pImg->SetColorTable(0, P.nNumColors, ColorTab);  
}

int CImageEx::PaletteType( RGBQUAD *ColorTab )
{
	int i,k,m,n,r,g,b;  

	m=n=0;  
	for (i=0; i<256; i++)  
	{  
		r = ColorTab[i].rgbRed;  
		g = ColorTab[i].rgbGreen;  
		b = ColorTab[i].rgbBlue;  
		if ((r != g)||(r != b)) m=0;  
		if ((i>0)&&(r>ColorTab[i-1].rgbRed)) m++;  
		if (r+g+b==0) n++;  
	}  

	k=3;  
	if (m == 255) k=2;      
	else  if (256-n==1) k=0;                       
	else  if (256-n==15) k=1;      

	return(k);  
}

int CImageEx::ImageType()
{
	RGBQUAD ColorTab[256];  
	int     k;  

	if (IsNull()) return(0);  

	switch(GetBPP())  
	{  
	case 1:  k=0;   break;   
	case 4:  k=1;   break;   
	case 8:  k=3;   break;   
	default: k=4;   break;   
	}  

	if (k==3)                    
	{  
		GetColorTable(0,256,ColorTab);  
		k=PaletteType(ColorTab);  
	}  
	return(k);  
}

void CImageEx::RotateCimage( CImage *Imgn, int nAngle )
{
	double alpha = nAngle*3.141592654/180;

	IMAGEPARAMENT P;   
	RGBQUAD ColorTab[256];   
	int i, j, ww, Xd, Yd, Dx, Dy,nSize;   
	double centerx, centery, sintheta, costheta;   
	double X1, Y1, X2, Y2, theta, xx, yy, rr;   
	BYTE **list, *sc;   
	int x1, y1, x2, y2, flag;   
	double p, q, a, b, c, d, t1, t2, t3;   

	if (ImageType() == 2)  flag = 1; //flag槨깃羚貫，뎠혤令槨1珂，깊刻崗窟昑코꿨랬 ,늪珂暠獗잚謹槨뿍쌓暠獗  
	else flag = 0; //0깊刻離쐤줅듐랬   
	GetImageParament(this,&P);   

	Dx = P.nWidth;   
	Dy = P.nHeight;   
	nSize = 0;  
	if (Dx < Dy)  
	{  
		nSize = Dy;  
	}  
	else  
	{  
		nSize = Dx;  
	}  
	int nLineBytes = (nSize * P.nBitCount + 31) / 32 * 4;  

	//뻘唐寧듐狼錦맣，꼇횔뎠暠獗멕똑陶댕黨욱똑珂삔굼웰  
	sc = (BYTE*) malloc(2 * nLineBytes);    // * P.nBytesPerLine); //헝묏鱗데禱   
	//  
	list = (BYTE**) malloc(Dy * sizeof(BYTE*)); //뚤覩貫暠쉔접랗郭鑒莉   
	for (i = 0; i < Dy; i++)   
		list[i] = (BYTE*) GetPixelAddress(0, i);   

	centerx = Dx / 2; //셕炬貫暠櫓懃貫零   
	centery = Dy / 2;   
	rr = sqrt(centerx * centerx + centery *centery); //셕炬뚤실窟낀똑   

	theta = atan((double) centery / (double) centerx);   

	X1 = fabs(rr * cos(alpha + theta)) + 0.5;   
	Y1 = fabs(rr * sin(alpha + theta)) + 0.5;   
	X2 = fabs(rr * cos(alpha - theta)) + 0.5;   
	Y2 = fabs(rr * sin(alpha - theta)) + 0.5;   

	if (X2 > X1) X1 = X2; //돤棍쌈앤近욱똑   
	if (Y2 > Y1) Y1 = Y2; //棍쌈앤近멕똑   
	ww = (int) (2 * X1);   

	Imgn ->Destroy();   

	//쉔접써벎貫暠  
	Imgn ->Create(ww, (int) (2 * Y1), P.nBitCount,CImage::createAlphaChannel );  
	if (P.nBitCount == 8)   
	{   
		GetAllPalette(this,ColorTab);   
		//錦맣寧，零커깃딧겼  
		SetAllPalette(Imgn, ColorTab); //릿齡딧겼   
	}   

	sintheta = sin(alpha);   
	costheta = cos(alpha);   

	for (j = (int) (centery - Y1), Yd = 0; j <= (centery + Y1); j++, Yd++)   
	{   
		if (P.nBitCount == 8)   
			memset (sc, 0, ww); //256貫暠獗羹契零교쒼令   
		else memset(sc, 0, ww * P.nBytesPerPixel); //廬꽈貫暠獗羹契零교쒼令   

		for (i = (int) (centerx - X1), Xd = 0; i <= centerx + X1; i++, Xd += P.nBytesPerPixel)   
		{   
			xx = centerx + costheta * (i - centerx) + sintheta * (j - centery);   
			yy = centery - sintheta * (i - centerx) + costheta * (j - centery);   
			x1 = (int) xx;   
			x2 = x1 + 1;   
			p = xx - x1;   
			y1 = (int) yy;   
			y2 = y1 + 1;   
			q = yy - y1;   
			if (((x1 < 0)||(x2 >= P.nWidth )||(y1 < 0)||(y2 >= P.nHeight )))   
				continue;   
			if (flag == 0)   
			{   
				if (q > 0.5)  y1 = y2;   
				if (p > 0.5)  x1 = x2;   
				//錦맣랗, sc[Xd]  
				memcpy(&sc[Xd], &list[y1][x1 * P.nBytesPerPixel], P.nBytesPerPixel); //닒都貫暠릿齡獗羹鑒앴   
			}   
			else   
			{ // flag된黨1，崗窟昑코꿨랬   
				a = (double) list[y1][x1]; //닒都貫暠혤鑒앴   
				b = (double) list[y1][x2];   
				c = (double) list[y2][x1];   
				d = (double) list[y2][x2];   
				t1 = (1 - p) * a + p * b; //崗窟昑코꿨셕炬   
				t2 = (1 - p) * c + p * d;   
				t3 = (1 - q) * t1 + q * t2;   
				//錦맣힛  
				sc[Xd] = (BYTE) t3;   
			}   
		}   
		SetRectValue(Imgn, 0, Yd, ww, 1, sc);   
	}  

	//png暠獗
	if ( P.nBitCount == 32 )
	{
		LPVOID pBitsSrc = NULL;
		BYTE * psrc = NULL;
		BITMAP stBmpInfo;

		HBITMAP hBmp = (HBITMAP)m_ImageClone;

		::GetObject(hBmp, sizeof(BITMAP), &stBmpInfo);

		if (32 != stBmpInfo.bmBitsPixel || NULL == stBmpInfo.bmBits)
			return;

		psrc = (BYTE *) stBmpInfo.bmBits;

		for (int nPosY = 0; nPosY < abs(stBmpInfo.bmHeight); nPosY++)
		{
			for (int nPosX = stBmpInfo.bmWidth; nPosX > 0; nPosX--)
			{
				BYTE alpha  = psrc[3];
				psrc[0] = (BYTE)((psrc[0] * alpha) / 255);
				psrc[1] = (BYTE)((psrc[1] * alpha) / 255);
				psrc[2] = (BYTE)((psrc[2] * alpha) / 255);
				psrc += 4;
			}
		}
	}

	free(list); //姦렴묏鱗데禱   
	free(sc);   
}

void CImageEx::SetRotation( int nAngle )
{
	if ( nAngle == 0 ) return;

	m_ImageClone.Destroy();
	RotateCimage(&m_ImageClone,nAngle);
}

//////////////////////////////////////////////////////////////////////////////////
//scpark added
/*
void		safe_release()
{
	if (dst.IsNull() == false)
	{
		dst.ReleaseDC();
		dst.Destroy();
	}
}
*/
bool CImageEx::get_crop(CImageEx &dst, int sx, int sy, int sw, int sh)
{
	dst.DestroyImage();
	/*
	if (dst.IsNull() == false)
	{
		dst.ReleaseDC();
		dst.Destroy();
	}
	*/
	dst.Create(sw, sh, CImage::GetBPP());
	HDC hDstDC = dst.GetDC();
	CImage::BitBlt(hDstDC, 0, 0, sw, sh, sx, sy);
	//dst.ReleaseDC();
	return true;
}

// SetPixel 대용
void CImageEx::set_pixel(int x, int y, COLORREF c)
{
	// m_image.SetPixel() call ::SetPixel() which is too slow
	// since it has to work with all DCs.

	BYTE *p = (BYTE*)CImage::GetPixelAddress(x, y);
	if (CImage::GetBPP() == 16)
	{
		*(WORD *)p = (WORD)(((c&0xf80000) >> 19) | ((c&0xf800) >> 6) | ((c&0xf8) << 7));
	}
	else
	{
		*p++ = GetBValue(c);
		*p++ = GetGValue(c);
		*p = GetRValue(c);
	}
}

// GetPixel 대용. 표준 GetPixel과의 계산 속도는 현저하다.
COLORREF CImageEx::get_pixel(int x, int y)
{
	COLORREF result = *((COLORREF*)CImage::GetPixelAddress(x,y));

	// 메모리에서 가져올때, blue와 red위치가 바뀌어서 가져와진다
	BYTE r = GetBValue(result);
	BYTE g = GetGValue(result);
	BYTE b = GetRValue(result);

	return RGB(r,g,b);
}

void CImageEx::resize(CImageEx &thumb, int new_height)
{
	int w = CImage::GetWidth();
	int h = CImage::GetHeight();
	int tnh = new_height;
	int tnw = new_height*w/h;
	int bpp = CImage::GetBPP();

	thumb.DestroyImage();
	thumb.Create(tnw, tnh, bpp);
	HDC h2=thumb.GetDC(); //this returns undefined ?
	SetStretchBltMode(h2,HALFTONE);
	CImage::StretchBlt(h2,0,0,tnw,tnh,SRCCOPY);
}

