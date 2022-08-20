#ifndef SOCKETSERVERIMPL_H
#define SOCKETSERVERIMPL_H
#pragma once
#pragma warning(push)
#pragma warning(disable:4995)
#include <vector>
#include <list>
#pragma warning(pop)
#include "CritSection.h"
#include "ThreadPool.hpp"
#include "SocketHandle.h"


typedef std::list<SOCKET> SocketList;

/**
 * ISocketServerHandler
 * Event handler that SocketServerImpl<T> must implement
 * This class is not required, you can do the same thing as long your class exposes these functions.
 * (These functions are not pure to save you some typing)
 */
class ISocketServerHandler
{
public:
    virtual void OnThreadBegin(CSocketHandle* ) {}
    virtual void OnThreadExit(CSocketHandle* )  {}
    virtual void OnThreadLoopEnter(CSocketHandle* ) {}
    virtual void OnThreadLoopLeave(CSocketHandle* ) {}
    virtual void OnAddConnection(CSocketHandle* , SOCKET ) {}
    virtual void OnRemoveConnection(CSocketHandle* , SOCKET ) {}
    virtual void OnDataReceived(CSocketHandle* , const BYTE* , DWORD, SOCKET, const SockAddrIn& ) {}
    virtual void OnConnectionFailure(CSocketHandle*, SOCKET) {}
    virtual void OnConnectionDropped(CSocketHandle*, SOCKET ) {}
    virtual void OnConnectionError(CSocketHandle* , DWORD ) {}
};


/**
 * SocketServerImpl<T, tBufferSize>
 * Because <typename T> may refer to any class of your choosing,
 * Server Communication wrapper
 */
template <typename T, size_t tBufferSize = 2048>
class SocketServerImpl
{
    typedef SocketServerImpl<T, tBufferSize> thisClass;
public:
    SocketServerImpl()
    : _pInterface(0)
    , _thread(0)
    {
    }

    void SetInterface(T* pInterface)
    {
        ::InterlockedExchangePointer(reinterpret_cast<void**>(&_pInterface), pInterface);
    }

    operator CSocketHandle*() throw()
    {
        return( &_socket );
    }

    CSocketHandle* operator->() throw()
    {
        return( &_socket );
    }

    bool IsOpen() const
    {
        return _socket.IsOpen();
    }

    bool CreateSocket(LPCTSTR pszHost, LPCTSTR pszServiceName, int nFamily, int nType, UINT uOptions = 0)
    {
        return _socket.CreateSocket(pszHost, pszServiceName, nFamily, nType, uOptions);
    }

    void Close()
    {
        _socket.Close();
    }

    DWORD Read(LPBYTE lpBuffer, DWORD dwSize, LPSOCKADDR lpAddrIn = NULL, DWORD dwTimeout = INFINITE)
    {
        return _socket.Read(lpBuffer, dwSize, lpAddrIn, dwTimeout);
    }

    DWORD Write(const LPBYTE lpBuffer, DWORD dwCount, const LPSOCKADDR lpAddrIn = NULL, DWORD dwTimeout = INFINITE)
    {
        return _socket.Write(lpBuffer, dwCount, lpAddrIn, dwTimeout);
    }

    const SocketList& GetSocketList() const
    {
        // direct access! - use Lock/Unlock to protect
        return _sockets;
    }

    bool Lock()
    {
        return _critSection.Lock();
    }

    bool Unlock()
    {
        return _critSection.Unlock();
    }

    void ResetConnectionList()
    {
        AutoThreadSection aSection(&_critSection);
        _sockets.clear();
    }

    size_t GetConnectionCount() const
    {
        AutoThreadSection aSection(&_critSection);
        return _sockets.size();
    }

    void AddConnection(SOCKET sock)
    {
        AutoThreadSection aSection(&_critSection);
        _sockets.push_back( sock );
    }

    void RemoveConnection(SOCKET sock)
    {
        AutoThreadSection aSection(&_critSection);
        _sockets.remove( sock );
    }

    bool CloseConnection(SOCKET sock)
    {
        return CSocketHandle::ShutdownConnection( sock );
    }

    void CloseAllConnections();

    bool StartServer(LPCTSTR pszHost, LPCTSTR pszServiceName, int nFamily, int nType, UINT uOptions = 0);
    void Terminate(DWORD dwTimeout = INFINITE);

    static bool IsConnectionDropped(DWORD dwError);

protected:
    void Run();
    void OnConnection(ULONG_PTR s);
    static DWORD WINAPI SocketServerProc(thisClass* _this);

    T*              _pInterface;
    HANDLE          _thread;
    ThreadSection   _critSection;
    CSocketHandle   _socket;
    SocketList      _sockets;
};


template <typename T, size_t tBufferSize>
void SocketServerImpl<T, tBufferSize>::CloseAllConnections()
{
    AutoThreadSection aSection(&_critSection);
    if ( !_sockets.empty() )
    {
        // NOTE(elaurentin): this function closes all connections but handles are kept inside of list
        // (socket handles are removed by the pooling thread)
        SocketList::iterator iter;
        for(iter = _sockets.begin(); iter != _sockets.end(); ++iter)
        {
            CloseConnection( (*iter) );
        }
    }
}

template <typename T, size_t tBufferSize>
bool SocketServerImpl<T, tBufferSize>::StartServer(LPCTSTR pszHost, LPCTSTR pszServiceName, int nFamily, int nType, UINT uOptions)
{
    // must be closed first...
    if ( IsOpen() ) return false;
    bool result = false;
    result = _socket.CreateSocket(pszHost, pszServiceName, nFamily, nType, uOptions);
    if ( result )
    {
        _thread = AtlCreateThread(SocketServerProc, this);
        if ( _thread == NULL )
        {
            DWORD dwError = GetLastError();
            _socket.Close();
            SetLastError(dwError);
            result = false;
        }
    }
    return result;
}

template <typename T, size_t tBufferSize>
void SocketServerImpl<T, tBufferSize>::OnConnection(ULONG_PTR s)
{
    SockAddrIn addrIn;
    std::vector<unsigned char> data;
    data.resize( tBufferSize );

    DWORD dwRead;
    DWORD dwError;
    SOCKET sock = static_cast<SOCKET>(static_cast<ULONG>(s));
    CSocketHandle sockHandle;
    sockHandle.Attach(sock);
    sockHandle.GetPeerName( addrIn );
    int type = sockHandle.GetSocketType();

    // Notification: OnThreadLoopEnter
    if ( _pInterface != NULL ) {
        _pInterface->OnThreadLoopEnter(*this);
    }

    if (type == SOCK_STREAM) {
        AddConnection( sock );

        // Notification: OnAddConnection
        if ( _pInterface != NULL ) {
            _pInterface->OnAddConnection(*this, sock);
        }
    }

    if (type == SOCK_STREAM) {
        _socket.GetPeerName( addrIn );
    }

    // Connection loop
    while ( sockHandle.IsOpen() )
    {
        if (type == SOCK_STREAM)
        {
            dwRead = sockHandle.Read(&data[0], tBufferSize, NULL, INFINITE);
        }
        else
        {
            dwRead = sockHandle.Read(&data[0], tBufferSize, addrIn, INFINITE);
        }

        if ( ( dwRead != -1L ) && (dwRead > 0))
        {
            // Notification: OnDataReceived
            if ( _pInterface != NULL ) {
                _pInterface->OnDataReceived(*this, &data[0], dwRead, addrIn);
            }
        }
        else if (type == SOCK_STREAM && dwRead == 0L )
        {
            // connection broken
            if ( _pInterface != NULL ) {
                _pInterface->OnConnectionDropped(*this);
            }
            break;
        }
        else if ( dwRead == -1L)
        {
            dwError = GetLastError();
            if ( _pInterface != NULL )
            {
                if (IsConnectionDropped( dwError) ) {
                    // Notification: OnConnectionDropped
                    if (type == SOCK_STREAM || (dwError == WSAENOTSOCK || dwError == WSAENETDOWN))
                    {
                        _pInterface->OnConnectionDropped(*this);
                        break;
                    }
                }
                // Notification: OnConnectionError
                _pInterface->OnConnectionError(*this, dwError);
            }
            else {
                break;
            }
        }
    }

    // remove this connection from our list
    if (type == SOCK_STREAM) {
        RemoveConnection( sock );

        // Notification: OnRemoveConnection
        if ( _pInterface != NULL ) {
            _pInterface->OnRemoveConnection(*this, sock);
        }
    }

    // Detach or Close this socket (TCP-mode only)
    if (type != SOCK_STREAM ) {
        sockHandle.Detach();
    }
    data.clear();

    // Notification: OnThreadLoopLeave
    if ( _pInterface != NULL ) {
        _pInterface->OnThreadLoopLeave(*this);
    }
}

template <typename T, size_t tBufferSize>
void SocketServerImpl<T, tBufferSize>::Run()
{
    _ASSERTE( _pInterface != NULL && "Need an interface to pass events");

    SOCKET sock = _socket.GetSocket();
    int type = _socket.GetSocketType();

    // Notification: OnThreadBegin
    if ( _pInterface != NULL ) {
        _pInterface->OnThreadBegin(*this);
    }

    if (type == SOCK_STREAM)
    {
        // In TCP mode, we need one thread per connection
        while( _socket.IsOpen() )
        {
            SOCKET newSocket = CSocketHandle::WaitForConnection(sock);
            if (!_socket.IsOpen())
                break;

            // run a new client thread for each connection
            // report failure if not a valid socket or threadpool failed
            if ((newSocket == INVALID_SOCKET) ||
                !ThreadPool::QueueWorkItem(&SocketServerImpl<T, tBufferSize>::OnConnection,
                                        this,
                                        static_cast<ULONG_PTR>(newSocket))
                                        )
            {
                // Notification: OnConnectionFailure
                if ( _pInterface != NULL ) {
                    _pInterface->OnConnectionFailure(*this, newSocket);
                }
            }
        }
        // close all connections
        CloseAllConnections();
    }
    else
    {
        // UDP - only one instance
        OnConnection( sock );
    }

    // Notification: OnThreadExit
    if ( _pInterface != NULL ) {
        _pInterface->OnThreadExit(*this);
    }
}

template <typename T, size_t tBufferSize>
void SocketServerImpl<T, tBufferSize>::Terminate(DWORD dwTimeout /*= INFINITE*/)
{
    _socket.Close();
    if ( _thread != NULL )
    {
        if ( WaitForSingleObject(_thread, dwTimeout) == WAIT_TIMEOUT ) {
            TerminateThread(_thread, 1);
        }
        CloseHandle(_thread);
        _thread = NULL;
    }
}

template <typename T, size_t tBufferSize>
DWORD WINAPI SocketServerImpl<T, tBufferSize>::SocketServerProc(thisClass* _this)
{
    if ( _this != NULL )
    {
        _this->Run();
    }
    return 0;
}

template <typename T, size_t tBufferSize>
bool SocketServerImpl<T, tBufferSize>::IsConnectionDropped(DWORD dwError)
{
    // see: winerror.h for definition
    switch( dwError )
    {
        case WSAENOTSOCK:
        case WSAENETDOWN:
        case WSAENETUNREACH:
        case WSAENETRESET:
        case WSAECONNABORTED:
        case WSAECONNRESET:
        case WSAESHUTDOWN:
        case WSAEHOSTDOWN:
        case WSAEHOSTUNREACH:
            return true;
        default:
            break;
    }
    return false;
}

#endif //SOCKETSERVERIMPL_H