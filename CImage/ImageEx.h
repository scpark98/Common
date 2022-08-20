#ifndef IMAGE_HEAD_FILE
#define IMAGE_HEAD_FILE

#pragma once

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

//Í¼Æ¬¶ÔÏó
class CImageEx: public CImage
{
	//±äÁ¿¶¨Òå
protected:
	RECT							m_rcNinePart;
	CImage							m_ImageClone;
	BYTE							m_cbAlpha;

	//º¯Êý¶¨Òå
public:
	//¹¹Ôìº¯Êý
	CImageEx();
	//Îö¹¹º¯Êý
	virtual ~CImageEx();

	//ÉèÖÃº¯Êý
public:
	//¾Å¹¬¸ñ»æÍ¼
	void SetNinePart(CONST LPRECT lprcNinePart);
	//Alpha´¦Àí
	bool SetAlphaBit();
	//×ªHBITMAP
	HBITMAP ImageToBitmap();
	//»Ò¶È´¦Àí
	bool SetGray();
	//
	inline void SetAlpha(BYTE cbAlpha){ m_cbAlpha = cbAlpha; }
	void SetRotation(int nAngle);

	//scpark added
	bool		get_crop(CImageEx &dst, int sx, int sy, int sw, int sh);
	// GetPixel, SetPixel ´ë¿ë
	COLORREF	get_pixel(int x, int y);
	void		set_pixel(int x, int y, COLORREF c);
	void		resize(CImageEx &thumb, int new_height);
	void		safe_release();

	//¹ÜÀíº¯Êý
public:
	//Ïú»ÙÍ¼Æ¬
	bool DestroyImage();
	//¼ÓÔØÍ¼Æ¬
	bool LoadImage(LPCTSTR pszFileName);
	//¼ÓÔØÍ¼Æ¬
	bool LoadImage(HINSTANCE hInstance, LPCTSTR pszResourceName,LPCTSTR pszResourceType=TEXT("IMAGE"));

	//»æ»­º¯Êý
public:
	//»æ»­Í¼Ïñ
	BOOL DrawImage(CDC * pDC, INT nXPos, INT nYPos);
	//»æ»­Í¼Ïñ
	BOOL DrawImage(CDC * pDC, INT nXPos, INT nYPos, INT nDestWidth, INT nDestHeight);
	//»æ»­Í¼Ïñ
	BOOL DrawImage(CDC * pDC, RECT &rc);
	//»æ»­Í¼Ïñ
	BOOL DrawImage(CDC * pDC, INT nXDest, INT nYDest, INT nDestWidth, INT nDestHeight, INT nXScr, INT nYSrc);
	//»æ»­Í¼Ïñ
	BOOL DrawImage(CDC * pDC, INT nXDest, INT nYDest, INT nDestWidth, INT nDestHeight, INT nXScr, INT nYSrc, INT nSrcWidth, INT nSrcHeight);

	//ÇøÓò»æÖÆ
public:
	//»æ»­Í¼Ïñ
	bool Draw( CDC * pDC, INT x, INT y, INT cx, INT cy,INT nLeft,INT nTop,INT nRight,INT nBottom );
	//»æ»­Í¼Ïñ
	bool Draw( CDC * pDC, const RECT& rectDest, const RECT& rectSrc );
	//»æ»­Í¼Ïñ
	bool Draw( CDC * pDC, const RECT& rectDest );
	//»æ»­Í¼Ïñ
	bool DrawFrame( CDC * pDC, const RECT& rectDest );

	//À­Éì»æÖÆ
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