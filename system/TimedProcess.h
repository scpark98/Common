/*
- CTimer를 사용하기 번거로운 경우를 위해 만들어진 클래스
- 출처 : http://www.devpia.com/MAEUL/Contents/Detail.aspx?BoardID=51&MAEULNO=20&no=8615&page=5
- 장점 : CTimer보다 간단하며 CWnd를 상속받지 않아도 된다.
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
		BOOL bRepeat;                                            // 반복 실행 여부
		TIMEDPROCESSCALLBACK lpfCallback;       // 콜백 함수
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

		// 부모 윈도우를 갖지 않기 때문에 윈도우를 닫는 함수를 직접 호출한다
		// 그렇지 않으면 프로그램 종료시, OnDestroy 함수가 호출되지 않았다는 경고가 발생한다
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

					// 예약 시간이 지났으면 콜백 함수를 실행한다
					if (pTimedProcess)
					{
						// 콜백 함수를 백업한다
						TIMEDPROCESSCALLBACK lpfCallback = pTimedProcess->lpfCallback;

						// 반복 실행 요청이 아니면 리스트에서 삭제한다
						if (!pTimedProcess->bRepeat)
						{
							KillTimer(nID);
							m_mapList.RemoveKey(nID);
							delete pTimedProcess;
						}

						// 콜백 함수를 가장 마지막에 호출한다
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
			// Timer 를 사용하기 위해서 보이지 않는 Window 를 생성한다
			if (!CreateEx(0, AfxRegisterWndClass(0), _T(""), WS_POPUP, CRect(0, 0, 0, 0), NULL, NULL))
			{
				ASSERT(FALSE);
				return FALSE;
			}
			m_bCreated = TRUE;
		}

		m_pObject = pObject;

		TIMED_PROCESS* pTimedProcess;

		// 이미 예약 작업이 존재하면 이전 설정을 취소하고 새로 설정한다
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
		// 지정한 예약 작업을 취소한다
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
		// 모든 예약 작업을 취소한다
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
