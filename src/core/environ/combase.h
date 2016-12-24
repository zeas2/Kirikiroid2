#pragma once
#include "typedefine.h"
#include "win32type.h"
#include <stdint.h>

#ifndef S_OK
#define S_OK            ((HRESULT)0L)
#define S_FALSE         ((HRESULT)1L)
#define S_UNKNOWN       ((HRESULT)0x7FFFFFFFL)
#define E_NOINTERFACE   ((HRESULT)0x80004002L)
#define E_INVALIDARG    ((HRESULT)0x80000003L)
#define E_FAIL          ((HRESULT)0x80004005L)
#define E_NOTIMPL       ((HRESULT)0x80004001L)
#define E_NOINTERFACE   ((HRESULT)0x80004002L)
#define E_OUTOFMEMORY   ((HRESULT)0x8007000EL)
#define FAILED(hr)      (((HRESULT)(hr)) < 0)
#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define TRUE 1

typedef struct _IID
{
    uint32_t x;
	uint16_t s1;
	uint16_t s2;
	uint8_t  c[8];
} IID/*IID*/;
#define REFIID const IID &

#endif

#ifndef STDMETHODCALLTYPE
#ifdef WIN32
#define STDMETHODCALLTYPE __stdcall
#else
#define STDMETHODCALLTYPE
#endif

class IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject) = 0;

    virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;

    virtual ULONG STDMETHODCALLTYPE Release( void) = 0;

};

class ISequentialStream : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Read( 
        /* [annotation][write_to] */ 
        void *pv,
        /* [annotation][in] */ 
        ULONG cb,
        /* [annotation][out_opt] */ 
        ULONG *pcbRead) = 0;

    virtual HRESULT STDMETHODCALLTYPE Write( 
        /* [annotation] */ 
        const void *pv,
        /* [annotation][in] */ 
        ULONG cb,
        /* [annotation][out_opt] */ 
        ULONG *pcbWritten) = 0;

};

typedef char      OLECHAR;
typedef LPSTR     LPOLESTR;
typedef LPCSTR    LPCOLESTR;
typedef struct tagSTATSTG
{
    LPOLESTR pwcsName;
#ifdef _MAC
    //FSSpec is Macintosh only, defined in macos\files.h
    FSSpec *pspec;
#endif //_MAC
    DWORD type;
    ULARGE_INTEGER cbSize;
    FILETIME mtime;
    FILETIME ctime;
    FILETIME atime;
    DWORD grfMode;
    DWORD grfLocksSupported;
    //CLSID clsid;
    DWORD grfStateBits;
    DWORD reserved;
} 	STATSTG;

typedef 
    enum tagSTREAM_SEEK
{
    STREAM_SEEK_SET	= 0,
    STREAM_SEEK_CUR	= 1,
    STREAM_SEEK_END	= 2
} 	STREAM_SEEK;

typedef 
    enum tagSTATFLAG
{
    STATFLAG_DEFAULT	= 0,
    STATFLAG_NONAME	= 1,
    STATFLAG_NOOPEN	= 2
} 	STATFLAG;
typedef 
    enum tagSTGTY
{
    STGTY_STORAGE	= 1,
    STGTY_STREAM	= 2,
    STGTY_LOCKBYTES	= 3,
    STGTY_PROPERTY	= 4
} 	STGTY;
#define STGM_DIRECT             0x00000000L
#define STGM_TRANSACTED         0x00010000L
#define STGM_SIMPLE             0x08000000L
#define STGM_READ               0x00000000L
#define STGM_WRITE              0x00000001L
#define STGM_READWRITE          0x00000002L
#define STGM_SHARE_DENY_NONE    0x00000040L
#define STGM_SHARE_DENY_READ    0x00000030L
#define STGM_SHARE_DENY_WRITE   0x00000020L
#define STGM_SHARE_EXCLUSIVE    0x00000010L
struct IStream : public ISequentialStream
{
public:
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [annotation] */ 
        ULARGE_INTEGER *plibNewPosition) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize) = 0;

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [annotation][unique][in] */ 
        IStream *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [annotation] */ 
        ULARGE_INTEGER *pcbRead,
        /* [annotation] */ 
        ULARGE_INTEGER *pcbWritten) = 0;

    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE Revert( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) = 0;

    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) = 0;

    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG *pstatstg,
        /* [in] */ DWORD grfStatFlag) = 0;

    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IStream **ppstm) = 0;

};
#endif
