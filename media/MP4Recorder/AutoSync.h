#pragma once

#include <afxmt.h>

class CAutoSync
{
private:
	CCriticalSection &m_syncObj;

public:
	CAutoSync(CCriticalSection &syncObj) : m_syncObj(syncObj) {
		m_syncObj.Lock();
	}
	~CAutoSync() {
		m_syncObj.Unlock();
	}
};