/*
- CTimer�� ����ϱ� ���ŷο� ��츦 ���� ������� Ŭ����
- ��ó : http://www.devpia.com/MAEUL/Contents/Detail.aspx?BoardID=51&MAEULNO=20&no=8615&page=5
- ���� : CTimer���� �����ϸ� CWnd�� ��ӹ��� �ʾƵ� �ȴ�.
*/

#pragma once


// CTimedProcess

template <class T>
class CTimedProcess : public CWnd
{
public:
	typedef void (T::*TIMEDPROCESSCALLBACK)();

	struct TIMED_PROCESS
	{
		BOOL bRepeat;                                            // �ݺ� ���� ����
		TIMEDPROCESSCALLBACK lpfCallback;       // �ݹ� �Լ�
	};

public:
	CTimedProcess()
	{
		m_bCreated = FALSE;
		m_pObject = NULL;
	}

	virtual ~CTimedProcess()
	{
		StopAll();

		// �θ� �����츦 ���� �ʱ� ������ �����츦 �ݴ� �Լ��� ���� ȣ���Ѵ�
		// �׷��� ������ ���α׷� �����, OnDestroy �Լ��� ȣ����� �ʾҴٴ� ��� �߻��Ѵ�
		DestroyWindow();
	}

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_TIMER:
			{
				ASSERT(m_bCreated && m_pObject);

				UINT nID = wParam;
				TIMED_PROCESS* pTimedProcess;
				if (!m_mapList.Lookup(nID, pTimedProcess))
				{
					ASSERT(FALSE);

					KillTimer(nID);
				}
				else
				{
					ASSERT(pTimedProcess);
					ASSERT(pTimedProcess->lpfCallback);

					// ���� �ð��� �������� �ݹ� �Լ��� �����Ѵ�
					if (pTimedProcess)
					{
						// �ݹ� �Լ��� ����Ѵ�
						TIMEDPROCESSCALLBACK lpfCallback = pTimedProcess->lpfCallback;

						// �ݺ� ���� ��û�� �ƴϸ� ����Ʈ���� �����Ѵ�
						if (!pTimedProcess->bRepeat)
						{
							KillTimer(nID);
							m_mapList.RemoveKey(nID);
							delete pTimedProcess;
						}

						// �ݹ� �Լ��� ���� �������� ȣ���Ѵ�
						if (m_pObject && lpfCallback)
						{
							(m_pObject->*lpfCallback)();
						}
					}
				}
				return 0;
			}
		}
		return CWnd::WindowProc(message, wParam, lParam);
	}

public:
	BOOL Set(T* pObject, TIMEDPROCESSCALLBACK lpfCallback, UINT nID, UINT nWaitTime, BOOL bRepeat = FALSE)
	{
		ASSERT(pObject);
		ASSERT(lpfCallback);

		if (pObject == NULL || lpfCallback == NULL)
		{
			ASSERT(FALSE);
			return FALSE;
		}

		if (!m_bCreated)
		{
			// Timer �� ����ϱ� ���ؼ� ������ �ʴ� Window �� �����Ѵ�
			if (!CreateEx(0, AfxRegisterWndClass(0), _T(""), WS_POPUP, CRect(0, 0, 0, 0), NULL, NULL))
			{
				ASSERT(FALSE);
				return FALSE;
			}
			m_bCreated = TRUE;
		}

		m_pObject = pObject;

		TIMED_PROCESS* pTimedProcess;

		// �̹� ���� �۾��� �����ϸ� ���� ������ ����ϰ� ���� �����Ѵ�
		if (m_mapList.Lookup(nID, pTimedProcess))
		{
			ASSERT(pTimedProcess);
			ASSERT(pTimedProcess->lpfCallback);

			KillTimer(nID);
		}
		else
		{
			pTimedProcess = new TIMED_PROCESS;
			m_mapList[nID] = pTimedProcess;
		}

		pTimedProcess->bRepeat = bRepeat;
		pTimedProcess->lpfCallback = lpfCallback;
		SetTimer(nID, nWaitTime, NULL);

		return TRUE;
	}

	BOOL Stop(UINT nID)
	{
		// ������ ���� �۾��� ����Ѵ�
		TIMED_PROCESS* pTimedProcess;
		if (m_mapList.Lookup(nID, pTimedProcess))
		{
			ASSERT(pTimedProcess);
			ASSERT(pTimedProcess->lpfCallback);

			KillTimer(nID);
			m_mapList.RemoveKey(nID);
			delete pTimedProcess;
			return TRUE;
		}
		else
		{
			ASSERT(FALSE);
			return FALSE;
		}
	}

	void StopAll()
	{
		// ��� ���� �۾��� ����Ѵ�
		POSITION pos = m_mapList.GetStartPosition();

		while (pos != NULL)
		{
			UINT nID;
			TIMED_PROCESS* pTimedProcess;
			m_mapList.GetNextAssoc(pos, nID, pTimedProcess);
			ASSERT(pTimedProcess);
			ASSERT(pTimedProcess->lpfCallback);

			KillTimer(nID);
			m_mapList.RemoveKey(nID);
			delete pTimedProcess;
		}
		m_mapList.RemoveAll();
	}

protected:
	BOOL m_bCreated;
	CMap<UINT, UINT, TIMED_PROCESS*, TIMED_PROCESS* > m_mapList;
	T* m_pObject;
};
