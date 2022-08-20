#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP
#pragma warning(push)
#pragma warning(disable:4995)
#include <memory>
#pragma warning(pop)
#pragma once

class ThreadPool
{
    static const int MAX_THREADS = 50;
    template <typename T>
    struct ThreadParam
    {
        void (T::* _function)(); T* _pobject;
        ThreadParam(void (T::* function)(), T * pobject)
        : _function(function), _pobject(pobject) { }
    };
    template <typename T>
    struct ThreadParamEx
    {
        void (T::* _function)(ULONG_PTR); T* _pobject; ULONG_PTR _arg;
        ThreadParamEx(void (T::* function)(ULONG_PTR), T * pobject, ULONG_PTR arg)
        : _function(function), _pobject(pobject), _arg(arg) { }
    };

public:
    template <typename T>
    static bool QueueWorkItem(void (T::*function)(),
                                  T * pobject, ULONG nFlags = WT_EXECUTELONGFUNCTION)
    {
        std::auto_ptr< ThreadParam<T> > p(new ThreadParam<T>(function, pobject) );
        WT_SET_MAX_THREADPOOL_THREADS(nFlags, MAX_THREADS);
        bool result = false;
        if (::QueueUserWorkItem(WorkerThreadProc<T>,
                                p.get(),
                                nFlags))
        {
            p.release();
            result = true;
        }
        return result;
    }

    template <typename T>
    static bool QueueWorkItem(void (T::*function)(ULONG_PTR),
                                  T * pobject, ULONG_PTR arg, ULONG nFlags = WT_EXECUTELONGFUNCTION)
    {
        std::auto_ptr< ThreadParamEx<T> > p(new ThreadParamEx<T>(function, pobject, arg) );
        WT_SET_MAX_THREADPOOL_THREADS(nFlags, MAX_THREADS);
        bool result = false;
        if (::QueueUserWorkItem(WorkerThreadProcEx<T>,
                                p.get(),
                                nFlags))
        {
            p.release();
            result = true;
        }
        return result;
    }

private:

    template <typename T>
    static DWORD WINAPI WorkerThreadProc(LPVOID pvParam)
    {
        std::auto_ptr< ThreadParam<T> > p(static_cast< ThreadParam<T>* >(pvParam));
        try {
            (p->_pobject->*p->_function)();
        }
        catch(...) {}
        return 0;
    }

    template <typename T>
    static DWORD WINAPI WorkerThreadProcEx(LPVOID pvParam)
    {
        std::auto_ptr< ThreadParamEx<T> > p(static_cast< ThreadParamEx<T>* >(pvParam));
        try {
            (p->_pobject->*p->_function)(p->_arg);
        }
        catch(...) {}
        return 0;
    }

    ThreadPool();
};
#endif //THREADPOOL_HPP