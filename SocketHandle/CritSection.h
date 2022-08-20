#ifndef CRITSECTION_H
#define CRITSECTION_H

/**
 * Critical section
 * Critical Section object
 */
class ThreadSection
{
public:
    ThreadSection()
    {
        HRESULT hr = Init();
        (hr);
    }

    ~ThreadSection()
    {
        DeleteCriticalSection(&_CriticalSection);
    }

    /// Enter/Lock a critical section
    bool Lock()
    {
        bool result = false;
        __try
        {
			if ( _CriticalSection.DebugInfo != NULL )
			{
				EnterCriticalSection(&_CriticalSection);
				result = true;
			}
        }
        __except(STATUS_NO_MEMORY == GetExceptionCode())
        {
        }
        return result;
    }

    /// Leave/Unlock a critical section
    bool Unlock()
    {
        bool result = false;
        __try
        {
            LeaveCriticalSection(&_CriticalSection);
            result = true;
        }
        __except(STATUS_NO_MEMORY == GetExceptionCode())
        {
        }
        return result;
    }

private:
    /// Initialize critical section
    HRESULT Init() throw()
    {
        HRESULT hRes = E_FAIL;
        __try
        {
            InitializeCriticalSection(&_CriticalSection);
            hRes = S_OK;
        }
        // structured exception may be raised in low memory situations
        __except(STATUS_NO_MEMORY == GetExceptionCode())
        {
            hRes = E_OUTOFMEMORY;
        }
        return hRes;
    }

    ThreadSection(const ThreadSection & tSection);
    ThreadSection &operator=(const ThreadSection & tSection);
    CRITICAL_SECTION _CriticalSection;
};


/**
 * AutoThreadSection class
 * Critical section auto-lock
 */
class AutoThreadSection
{
public:
    /// auto section - constructor
    AutoThreadSection(__in ThreadSection* pSection)
    {
        _pSection = pSection;
        _pSection->Lock();
    }

    ~AutoThreadSection()
    {
        _pSection->Unlock();
    }
private:
    AutoThreadSection(const AutoThreadSection & tSection);
    AutoThreadSection &operator=(const AutoThreadSection & tSection);
    ThreadSection * _pSection;
};

#endif //CRITSECTION_H