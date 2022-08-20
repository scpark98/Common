#ifndef ASYNCSOCKETSERVERIMPL_H
#define ASYNCSOCKETSERVERIMPL_H
#pragma once
#pragma warning(push)
#pragma warning(disable:4995)
#include <vector>
#include <list>
#include <algorithm>
#pragma warning(pop)
#include "CritSection.h"
#include "SocketHandle.h"


/**
 * SocketIOBuffer
 * Overlapped I/O buffer
 */
class SocketIOBuffer
{
public:
    SocketIOBuffer(SOCKET sock)
    : _socket(sock)
    {
        // by default - no allocation
        memset(&_Overlapped, 0, sizeof(_Overlapped));
    }
    explicit SocketIOBuffer(SOCKET sock, size_t size)
    : _socket(sock)
    {
        memset(&_Overlapped, 0, sizeof(_Overlapped));
        _data.resize( size );
    }
    SocketIOBuffer(const SocketIOBuffer& sbuf)
    {
        Copy(sbuf);
    }
    SocketIOBuffer& operator=(const SocketIOBuffer& sbuf)
    {
        return Copy(sbuf);
    }
    ~SocketIOBuffer()
    {
        Free();
    }
    bool    IsValid() const { return (_socket != INVALID_SOCKET); }
    void    ReAlloc( size_t count) { _data.resize(count); }
    void    Free()                 { _data.clear(); }
    size_t  BufferSize() const { return _data.size(); }
    SocketIOBuffer& Copy(const SocketIOBuffer& sbuf)
    {
        if ( this != &sbuf )
        {
            _socket = sbuf._socket;
            _sockAddr = sbuf._sockAddr;
            if ( !sbuf._data.empty() ) {
                _data.resize( sbuf._data.size() );
                memcpy(&_data[0], &(sbuf._data[0]), _data.size());
            }
        }
        return *this;
    }

    // Quick access operator
    operator SOCKET()       { return _socket; }
    operator SOCKET() const { return _socket; }
    operator SockAddrIn&()  { return _sockAddr; }
    operator const SockAddrIn&() const { return _sockAddr; }
    operator LPSOCKADDR()   { return _sockAddr; }
    operator LPWSAOVERLAPPED() { return &_Overlapped; }
    operator LPBYTE()       { return &_data[0]; }

    bool     IsEqual(const SocketIOBuffer& sbuf) const { return (_socket == sbuf._socket); }
    bool     operator==(const SocketIOBuffer& sbuf) const { return IsEqual( sbuf ); }
    bool     operator==(SOCKET sock) const { return (_socket == sock); }
    bool     operator!=(const SocketIOBuffer& sbuf) const { return !IsEqual( sbuf ); }
private:
    SOCKET     _socket;
    SockAddrIn _sockAddr;
    WSAOVERLAPPED _Overlapped;
    std::vector<unsigned char> _data;
    SocketIOBuffer();
};

typedef std::list<SocketIOBuffer> SocketBufferList;

static DWORD WM_ADD_CONNECTION = WM_USER+0x101;

/**
 * IASocketServerHandler
 * Event handler that ASocketServerImpl<T> must implement
 * This class is not required, you can do the same thing as long your class exposes these functions.
 * (These functions are not pure to save you some typing)
 */
class IASocketServerHandler
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
 * ASocketServerImpl<T, tBufferSize>
 * Because <typename T> may refer to any class of your choosing,
 * Server Communication wrapper
 */
template <typename T, size_t tBufferSize = 2048>
class ASocketServerImpl
{
    typedef ASocketServerImpl<T, tBufferSize> thisClass;
public:
    ASocketServerImpl()
    : _pInterface(0)
    , _thread(0)
    , _threadId(0)
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

    const SocketBufferList& GetSocketList() const
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
		if ( _sockets.size() == 0 )
			return;

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

    SocketIOBuffer* GetConnectionBuffer(SOCKET sock)
    {
        AutoThreadSection aSection(&_critSection);
        SocketBufferList::iterator iter = std::find(_sockets.begin(), _sockets.end(), sock);
        return ((iter != _sockets.end()) ? &(*iter) : NULL);
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
    void IoRun();
    bool AsyncRead(SocketIOBuffer* pBuffer);
    void OnIOComplete(DWORD dwError, SocketIOBuffer* pBuffer, DWORD cbTransferred, DWORD dwFlags);

    static DWORD WINAPI SocketServerProc(thisClass* _this);
    static DWORD WINAPI SocketServerIOProc(thisClass* _this);
    static void  WINAPI CompletionRoutine(DWORD dwError, DWORD cbTransferred,
                                          LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

    T*              _pInterface;
    HANDLE          _thread;
    DWORD           _threadId;
    ThreadSection   _critSection;
    CSocketHandle   _socket;
    SocketBufferList _sockets;
};


template <typename T, size_t tBufferSize>
void ASocketServerImpl<T, tBufferSize>::CloseAllConnections()
{
    AutoThreadSection aSection(&_critSection);
    if ( !_sockets.empty() )
    {
        // NOTE(elaurentin): this function closes all connections but handles are kept inside of list
        // (socket handles are removed by the pooling thread)
        SocketBufferList::iterator iter;
        for(iter = _sockets.begin(); iter != _sockets.end(); ++iter)
        {
            CloseConnection( (*iter) );
        }
    }
}

template <typename T, size_t tBufferSize>
bool ASocketServerImpl<T, tBufferSize>::StartServer(LPCTSTR pszHost, LPCTSTR pszServiceName, int nFamily, int nType, UINT uOptions)
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
void ASocketServerImpl<T, tBufferSize>::Run()
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
        HANDLE ioThread  = NULL;
        DWORD ioThreadId = 0L;

        ioThread = CreateThreadT(0, 0, SocketServerIOProc, this, 0, &ioThreadId);
        if ( ioThread == NULL )
        {
            DWORD dwError = GetLastError();
            if ( _pInterface != NULL ) {
                _pInterface->OnConnectionError(*this, dwError);
            }
        }

        // In TCP mode, use an I/O thread to process all requests
        while( _socket.IsOpen() )
        {
            SOCKET newSocket = CSocketHandle::WaitForConnection(sock);
            if (!_socket.IsOpen())
                break;

            if (!PostThreadMessage(ioThreadId, WM_ADD_CONNECTION, 0,
                                   static_cast<ULONG_PTR>(newSocket)))
            {
                // Notification: OnConnectionFailure
                if ( _pInterface != NULL ) {
                    _pInterface->OnConnectionFailure(*this, newSocket);
                }
            }
        }

        // close all connections
        CloseAllConnections();

        // wait for io thread
        if ( ioThread != NULL )
        {
            PostThreadMessage(ioThreadId, WM_QUIT, 0, 0);
            WaitForSingleObject(ioThread, INFINITE);
            CloseHandle(ioThread);
        }
        ResetConnectionList();
    }
    else
    {
        // UDP mode - let's reuse our thread here
        try {
            SocketIOBuffer ioBuffer(_socket.GetSocket(), tBufferSize);
            AsyncRead( &ioBuffer );

            // Save our thread id so we can exit gracefully
            _threadId = GetCurrentThreadId();
            // Process UDP packets
            IoRun();
        } catch(std::bad_alloc&) { /* memory exception */
            if ( _pInterface != NULL ) {
                _pInterface->OnConnectionError(*this, ERROR_NOT_ENOUGH_MEMORY);
            }
        }
    }

    // Notification: OnThreadExit
    if ( _pInterface != NULL ) {
        _pInterface->OnThreadExit(*this);
    }
}

template <typename T, size_t tBufferSize>
void ASocketServerImpl<T, tBufferSize>::IoRun()
{
    _ASSERTE( _pInterface != NULL && "Need an interface to pass events");
    MSG msg;
    ::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // Notification: OnThreadLoopEnter
    if ( _pInterface != NULL ) {
        _pInterface->OnThreadLoopEnter(*this);
    }

    DWORD dwResult;
    while( (dwResult = MsgWaitForMultipleObjectsEx(0, NULL, INFINITE, QS_ALLEVENTS, MWMO_ALERTABLE)) != WAIT_FAILED)
    {
        msg.message = 0;
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        // exit on WM_QUIT or main socket closed
        if ( msg.message == WM_QUIT || !_socket.IsOpen() )
            break;
        else if ( msg.message == WM_ADD_CONNECTION )
        {
            SOCKET sock = static_cast<SOCKET>(msg.lParam);
            AddConnection(sock);

            SocketIOBuffer* pBuffer = GetConnectionBuffer(sock);
            _ASSERTE( pBuffer != NULL );
            if ( pBuffer != NULL )
            {
                pBuffer->ReAlloc( tBufferSize );
                if (!AsyncRead(pBuffer))
                {
                    // remove and close connection
                    RemoveConnection( sock );
                    CloseConnection( sock );
                }
                else
                {
                    // Notification: OnAddConnection
                    if ( _pInterface != NULL ) {
                        _pInterface->OnAddConnection(*this, sock);
                    }
                }
            }
        }
    }

    // Notification: OnThreadLoopLeave
    if ( _pInterface != NULL ) {
        _pInterface->OnThreadLoopLeave(*this);
    }
}

template <typename T, size_t tBufferSize>
bool ASocketServerImpl<T, tBufferSize>::AsyncRead(SocketIOBuffer* pBuffer)
{
    CSocketHandle sockHandle;
    DWORD dwRead;
    SocketIOBuffer& sbuf = (*pBuffer);
    SOCKET sock = static_cast<SOCKET>(sbuf);
    sockHandle.Attach( sock );

    int type = sockHandle.GetSocketType();
    LPWSAOVERLAPPED lpOverlapped = static_cast<LPWSAOVERLAPPED>(sbuf);
    lpOverlapped->hEvent  = reinterpret_cast<HANDLE>(this);
    lpOverlapped->Pointer = reinterpret_cast<PVOID>(pBuffer);

    if (type == SOCK_STREAM) {
        // TCP - save current peer address
        sockHandle.GetPeerName( sbuf );
        dwRead = sockHandle.ReadEx(static_cast<LPBYTE>(sbuf),      // buffer
                            static_cast<DWORD>(sbuf.BufferSize()), // buffer size
                            NULL,                    // sockaddr
                            lpOverlapped,            // overlapped
                            thisClass::CompletionRoutine );
    } else {
        dwRead = sockHandle.ReadEx(static_cast<LPBYTE>(sbuf),      // buffer
                            static_cast<DWORD>(sbuf.BufferSize()), // buffer size
                            static_cast<LPSOCKADDR>(sbuf),         // sockaddr
                            lpOverlapped,            // overlapped
                            thisClass::CompletionRoutine );
    }
    sockHandle.Detach();
    return ( dwRead != -1L );
}

template <typename T, size_t tBufferSize>
void ASocketServerImpl<T, tBufferSize>::OnIOComplete(DWORD dwError, SocketIOBuffer* pBuffer, DWORD cbTransferred, DWORD /*dwFlags*/)
{
    _ASSERTE( _pInterface != NULL && "Need an interface to pass events");
    _ASSERTE( pBuffer != NULL && "Invalid Buffer");
    if ( pBuffer != NULL )
    {
        CSocketHandle sockHandle;
        SOCKET sock = static_cast<SOCKET>(*pBuffer);
        sockHandle.Attach( sock );
        int type = sockHandle.GetSocketType();
        if ( dwError == NOERROR )
        {
            if (type == SOCK_STREAM && cbTransferred == 0L )
            {
                // connection broken
                if ( _pInterface != NULL ) {
                    _pInterface->OnConnectionDropped(*this, sock);
					TRACE( "disconnected = %d\n", sock );
                }

                // remove connection
                RemoveConnection( sock );

                if ( _pInterface != NULL ) {
                    // Notification: OnRemoveConnection
                    _pInterface->OnRemoveConnection(*this, sock);
                }
            }
            else
            {
                // Notification: OnDataReceived
                if ( _pInterface != NULL ) {
                    _pInterface->OnDataReceived(*this,
                            static_cast<LPBYTE>(*pBuffer),     // Data
                                                cbTransferred, // Number of bytes
												sock,			//client socket
                            static_cast<SockAddrIn&>(*pBuffer));
                }

                // schedule another read for this socket
                AsyncRead( pBuffer );
            }
        }
        else
        {
            for ( ; _pInterface != NULL; )
            {
                if (IsConnectionDropped( dwError) ) {
                    // Notification: OnConnectionDropped
                    if (type == SOCK_STREAM || (dwError == WSAENOTSOCK || dwError == WSAENETDOWN))
                    {
                        _pInterface->OnConnectionDropped(*this, sock);
                        type = -1; // don't schedule other request
                        break;
                    }
                }
                // Notification: OnConnectionError
                _pInterface->OnConnectionError(*this, dwError);
                break;
            }

            // Schedule read request
            if ((type == SOCK_STREAM || type == SOCK_DGRAM) && _socket.IsOpen() ) {
                AsyncRead( pBuffer );
            }
        }
        // Detach or Close this socket (TCP-mode only)
        if (type != SOCK_STREAM ||
           (type == SOCK_STREAM && cbTransferred != 0L) )
        {
            sockHandle.Detach();
        }
    }
}

template <typename T, size_t tBufferSize>
void ASocketServerImpl<T, tBufferSize>::Terminate(DWORD dwTimeout /*= INFINITE*/)
{
    _socket.Close();
    if ( _thread != NULL )
    {
        if ( _threadId != 0 ) {
            PostThreadMessage(_threadId, WM_QUIT, 0, 0);
        }
        if ( WaitForSingleObject(_thread, dwTimeout) == WAIT_TIMEOUT ) {
            TerminateThread(_thread, 1);
        }
        CloseHandle(_thread);
        _thread = NULL;
        _threadId = 0L;
    }
}

template <typename T, size_t tBufferSize>
DWORD WINAPI ASocketServerImpl<T, tBufferSize>::SocketServerProc(thisClass* _this)
{
    if ( _this != NULL )
    {
        _this->Run();
    }
    return 0;
}

template <typename T, size_t tBufferSize>
DWORD WINAPI ASocketServerImpl<T, tBufferSize>::SocketServerIOProc(thisClass* _this)
{
    if ( _this != NULL )
    {
        _this->IoRun();
    }
    return 0;
}

template <typename T, size_t tBufferSize>
void  WINAPI ASocketServerImpl<T, tBufferSize>::CompletionRoutine(DWORD dwError, DWORD cbTransferred,
                                        LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
    thisClass* _this = reinterpret_cast<thisClass*>( lpOverlapped->hEvent );
    if ( _this != NULL )
    {
        SocketIOBuffer* pBuffer = reinterpret_cast<SocketIOBuffer*>(lpOverlapped->Pointer);
        _this->OnIOComplete(dwError, pBuffer, cbTransferred, dwFlags);
    }
}

template <typename T, size_t tBufferSize>
bool ASocketServerImpl<T, tBufferSize>::IsConnectionDropped(DWORD dwError)
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

#endif //ASYNCSOCKETSERVERIMPL_H