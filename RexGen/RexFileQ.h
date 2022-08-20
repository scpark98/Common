#pragma once

#include <queue>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>

using namespace std;
using namespace cv;

class REXFILEQ_DATA
{
public:
	IplImage *m_pImage;
	char m_sFname[MAX_PATH];
	int m_total;

	REXFILEQ_DATA(IplImage *image, const char* fname, int total)
	{
		if(image)
			m_pImage = cvCloneImage(image);
		else
			m_pImage = NULL;
		strcpy_s(m_sFname, fname);
	m_total = total;
	}
	~REXFILEQ_DATA()
	{
		if ( m_pImage )
		{
			cvReleaseImage(&m_pImage);
			m_pImage = NULL;
		}
	}

	CString GetRemovedSerialFName()
	{
		CString t = m_sFname;
		return t.Mid(9, t.GetLength() - 9);
	}
};


class RexFileQ
{
public:
	RexFileQ(void);
	~RexFileQ(void);

	char m_sFileQPath[MAX_PATH];
	int m_iCount;
	int m_iMaxMQ, m_iMaxFQ;
	queue<REXFILEQ_DATA*> m_image_mem_q;
	queue<REXFILEQ_DATA*> m_image_file_q;

	CString GetFullPath(CString fname);

	CCriticalSection m_csImage, m_csFile;
	void (*m_pProc_func)(REXFILEQ_DATA* data);
		
	int Init(const char* fq_path, bool clear, void (*p_func)(REXFILEQ_DATA*), int max_mq, int max_fq = 0);

	int PushData(REXFILEQ_DATA* data);

	int GetMemQSize() {
		int ret;
		m_csImage.Lock();
		ret = (int)m_image_mem_q.size();
		m_csImage.Unlock();
		return ret;
	}

	int GetFileQSize() {
		int ret;
		m_csFile.Lock();
		ret = (int)m_image_file_q.size();
		m_csFile.Unlock();
		return ret;
	}

	REXFILEQ_DATA* GetHead(CString *fname = NULL);

	REXFILEQ_DATA* PopData();

	bool m_bRunthread;
	CWinThread *m_pFileSaveThread, *m_pProcThread;
	static UINT FileSaveThread(LPVOID dParameter);
	static UINT ProcThread(LPVOID dParameter);
};

