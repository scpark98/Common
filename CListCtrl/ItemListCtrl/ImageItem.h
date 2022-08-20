#if !defined(AFX_PHOTOITEM_H__BB178519_3651_4BA2_8901_53527ADEC526__INCLUDED_)
#define AFX_PHOTOITEM_H__BB178519_3651_4BA2_8901_53527ADEC526__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PhotoItem.h : header file
//

#include "Item.h"

class CImage;

//---------------------------------------------------------------------------
// 클래스설명 : 2004. 3. 5 : CItemList , CImageList 변경함.
//---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////
class CImageItem : public CItem
{
public:
	CImageItem(); 

	CImageItem(UINT nResource);
	CImageItem(CImage * pImage); 
	CImageItem(CString strPath);
	CImageItem(Color clrImage, int nWidth, int nHeight);

	virtual ~CImageItem();

protected:
	CImage*	m_pImage;

protected:
	int	m_nFrameCount;
	int m_nFramePos;

public:
	CRect CalcImageRect(CRect rcImage);
	CSize CalcThumnailSize(CImage * pImage, int nThumbWidth, int nThumbHeight);

public:
	void InitAnimation();
	bool IsAnimation();

public:
	CImage * GetImage();

	virtual void SetImage(UINT nResource);
	virtual void SetImage(CImage * pImage);
	virtual void SetImage(CString strImage);
	virtual void SetImage(Color clrImage, int nWidth, int nHeight);

	virtual void SetThumbnailImage(UINT nResource);
	virtual void SetThumbnailImage(CImage * pImage);
	virtual void SetThumbnailImage(CString strImage);
	virtual void SetThumbnailImage(Color clrImage, int nWidth, int nHeight);

protected:
	virtual void DrawText(CDC * pDC);
	virtual void DrawImage(CDC * pDC);

public:
	virtual void Draw(CDC * pDC);
	virtual void Draw(CDC * pDC , int nOffsetX , int nOffsetY );

	virtual void DrawAnimation(CDC * pDC);
	virtual void DrawAnimation(CDC * pDC , int nOffsetX , int nOffsetY );
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PHOTOITEM_H__BB178519_3651_4BA2_8901_53527ADEC526__INCLUDED_)
