#ifndef IMAGE_HEAD_FILE
#define IMAGE_HEAD_FILE

#pragma once

#include <afxwin.h>
#include <atlimage.h>
//#include "SkinUI.h"

//////////////////////////////////////////////////////////////////////////////////

struct IMAGEPARAMENT 
{          
	int     nWidth;   
	int     nHeight;   
	int     nBitCount;   
	int     nBytesPerLine;   
	int     nBytesPerPixel;   
	int     nNumColors;   
	int     nSize;   
};   

//暠튬뚤蹶
class CImageEx: public CImage
{
	//긴좆땍屢
protected:
	RECT							m_rcNinePart;
	CImage							m_ImageClone;
	BYTE							m_cbAlpha;

	//변鑒땍屢
public:
	//뭐芚변鑒
	CImageEx();
	//驕뭐변鑒
	virtual ~CImageEx();

	//零변鑒
public:
	//씽묵목삥暠
	void SetNinePart(CONST LPRECT lprcNinePart);
	//Alpha뇹잿
	bool SetAlphaBit();
	//瘻HBITMAP
	HBITMAP ImageToBitmap();
	//뿍똑뇹잿
	bool SetGray();
	//
	inline void SetAlpha(BYTE cbAlpha){ m_cbAlpha = cbAlpha; }
	void SetRotation(int nAngle);

	//scpark added
	bool		get_crop(CImageEx &dst, int sx, int sy, int sw, int sh);
	// GetPixel, SetPixel 대용
	COLORREF	get_pixel(int x, int y);
	void		set_pixel(int x, int y, COLORREF c);
	void		resize(CImageEx &thumb, int new_height);
	void		safe_release();

	//밗잿변鑒
public:
	//饋쁑暠튬
	bool DestroyImage();
	//속潼暠튬
	bool LoadImage(LPCTSTR pszFileName);
	//속潼暠튬
	bool LoadImage(HINSTANCE hInstance, LPCTSTR pszResourceName,LPCTSTR pszResourceType=TEXT("IMAGE"));

	//삥뺌변鑒
public:
	//삥뺌暠獗
	BOOL DrawImage(CDC * pDC, INT nXPos, INT nYPos);
	//삥뺌暠獗
	BOOL DrawImage(CDC * pDC, INT nXPos, INT nYPos, INT nDestWidth, INT nDestHeight);
	//삥뺌暠獗
	BOOL DrawImage(CDC * pDC, RECT &rc);
	//삥뺌暠獗
	BOOL DrawImage(CDC * pDC, INT nXDest, INT nYDest, INT nDestWidth, INT nDestHeight, INT nXScr, INT nYSrc);
	//삥뺌暠獗
	BOOL DrawImage(CDC * pDC, INT nXDest, INT nYDest, INT nDestWidth, INT nDestHeight, INT nXScr, INT nYSrc, INT nSrcWidth, INT nSrcHeight);

	//혐堵삥齡
public:
	//삥뺌暠獗
	bool Draw( CDC * pDC, INT x, INT y, INT cx, INT cy,INT nLeft,INT nTop,INT nRight,INT nBottom );
	//삥뺌暠獗
	bool Draw( CDC * pDC, const RECT& rectDest, const RECT& rectSrc );
	//삥뺌暠獗
	bool Draw( CDC * pDC, const RECT& rectDest );
	//삥뺌暠獗
	bool DrawFrame( CDC * pDC, const RECT& rectDest );

	//윗삥齡
public:
	//
	bool DrawExtrude(CDC*pDC,const RECT& rectDest,bool bLeft=false,int nPixel=1);

protected:
	//
	void  GetImageParament(CImage *pImg,IMAGEPARAMENT *ppImgParam); 
	int   InImage(CImage *pImg,int x,int y);   
	void  SetRectValue(CImage *pImg,int x,int y,int Dx,int Dy,BYTE *buf);   
	void  GetAllPalette(CImage *pImg,RGBQUAD *ColorTab);   
	void  SetAllPalette(CImage *pImg,RGBQUAD *ColorTab);  
	int   PaletteType(RGBQUAD   *ColorTab); 
	int   ImageType();   
	void RotateCimage(CImage *Imgn, int nAngle);  
};

//////////////////////////////////////////////////////////////////////////////////

#endif