#ifndef __SOME_HELPER__H__
#define __SOME_HELPER__H__

#include "gfx_printjob.h"

BOOL SortListCtrl(CListCtrl *pCtrl, int nCol, BOOL bAscending, int low = 0, int high = -1);

extern int g_nTotalPrintedPageNumber;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CListCtrlPrintJob
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CListCtrlPrintJob : public GPrintJob
{
public:
	CListCtrlPrintJob(CListCtrl *pCtrl, BOOL bPrintToDefaultPrinter);
	virtual ~CListCtrlPrintJob();
   
	CDialog		*CreatePrintDialog();
	void		OnPrint();
	void		SetHeadAndFootString( CString sHead = "", CString sLeftFoot = "",
									  CString sFoot = "", CString sRightFoot = "" )
				{
					m_sHead		= sHead;
					m_sLeftFoot	= sLeftFoot;
					m_sFoot		= sFoot;
					m_sRightFoot= sRightFoot;
				}
	CString		m_sHead;
	CString		m_sLeftFoot;
	CString		m_sFoot;
	CString		m_sRightFoot;

	int			GetTotalPrintedPageNumber();

protected:
	CListCtrl	*m_pListCtrl;
	int			m_ColCount;
	BOOL		m_bPrintToDefault;

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CListCtrlHeaderPage
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CListCtrlHeaderPage : public GPrintUnit
{
public:
   CListCtrlHeaderPage(GPrintJob *pJob);
   virtual ~CListCtrlHeaderPage();

   BOOL		Print();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CListCtrlDataPage
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CListCtrlDataPage : public GPrintUnit
{
public:
	CListCtrlDataPage(GPrintJob *pJob, CListCtrl *pCtrl, int firstCol, int lastCol);
	virtual ~CListCtrlDataPage();

	BOOL		Print();
	void		StartPage();
	void		DefineColHeadings();
	void		CreatePrintFonts();
	void		InitPrintMetrics();
	void		PrintColHeadings(UINT nFormat, UINT nEffects=0);

	void		PrintHeader();
	void		PrintFooter();

	void		SetHeadAndFootString( CString sHead = "", CString sLeftFoot = "",
									  CString sFoot = "", CString sRightFoot = "" )
				{
					m_sHead		= sHead;
					m_sLeftFoot	= sLeftFoot;
					m_sFoot		= sFoot;
					m_sRightFoot= sRightFoot;
				}

protected:
	PUFONTPAIR		m_fontPairBody;
	CFont			m_fontHeading;
	CFont			m_fontHeader;
	CFont			m_fontFooter;
	int				m_firstCol,
					m_lastCol;
	CListCtrl		*m_pListCtrl;

	CString			m_sHead;
	CString			m_sLeftFoot;
	CString			m_sFoot;
	CString			m_sRightFoot;

};


#endif
