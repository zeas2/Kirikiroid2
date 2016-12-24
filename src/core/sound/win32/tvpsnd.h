/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Oct 03 00:37:04 2000
 */
/* Compiler settings for tvpsnd.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

//#include "rpc.h"
//#include "rpcndr.h"

#ifndef __tvpsnd_h__
#define __tvpsnd_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ITSSMediaBaseInfo_FWD_DEFINED__
#define __ITSSMediaBaseInfo_FWD_DEFINED__
typedef interface ITSSMediaBaseInfo ITSSMediaBaseInfo;
#endif 	/* __ITSSMediaBaseInfo_FWD_DEFINED__ */


#ifndef __ITSSStorageProvider_FWD_DEFINED__
#define __ITSSStorageProvider_FWD_DEFINED__
typedef interface ITSSStorageProvider ITSSStorageProvider;
#endif 	/* __ITSSStorageProvider_FWD_DEFINED__ */


#ifndef __ITSSModule_FWD_DEFINED__
#define __ITSSModule_FWD_DEFINED__
typedef interface ITSSModule ITSSModule;
#endif 	/* __ITSSModule_FWD_DEFINED__ */


#ifndef __ITSSWaveDecoder_FWD_DEFINED__
#define __ITSSWaveDecoder_FWD_DEFINED__
typedef interface ITSSWaveDecoder ITSSWaveDecoder;
#endif 	/* __ITSSWaveDecoder_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __TVPSndSysLib_LIBRARY_DEFINED__
#define __TVPSndSysLib_LIBRARY_DEFINED__

/* library TVPSndSysLib */
/* [helpstring][version][uuid] */ 

typedef /* [helpstring][version][uuid] */ struct  tagTSSWaveFormat
    {
    /* [helpstring] */ unsigned long dwSamplesPerSec;
    /* [helpstring] */ unsigned long dwChannels;
    /* [helpstring] */ unsigned long dwBitsPerSample;
    /* [helpstring] */ unsigned long dwSeekable;
    /* [helpstring] */ unsigned __int64 ui64TotalSamples;
    /* [helpstring] */ unsigned long dwTotalTime;
    long dwReserved1;
    long dwReserved2;
    }	TSSWaveFormat;


EXTERN_C const IID LIBID_TVPSndSysLib;

#ifndef __ITSSMediaBaseInfo_INTERFACE_DEFINED__
#define __ITSSMediaBaseInfo_INTERFACE_DEFINED__

/* interface ITSSMediaBaseInfo */
/* [object][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ITSSMediaBaseInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B4C4239B-7D27-43CC-8523-66955BDF59DF")
    ITSSMediaBaseInfo : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT __stdcall GetMediaType( 
            /* [in] */ LPWSTR shortname,
            /* [in] */ LPWSTR descbuf,
            /* [in] */ unsigned long descbuflen) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetLength( 
            /* [retval][out] */ unsigned long __RPC_FAR *length) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetTitle( 
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetCopyright( 
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetComment( 
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetArtist( 
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITSSMediaBaseInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITSSMediaBaseInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITSSMediaBaseInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITSSMediaBaseInfo __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetMediaType )( 
            ITSSMediaBaseInfo __RPC_FAR * This,
            /* [in] */ LPWSTR shortname,
            /* [in] */ LPWSTR descbuf,
            /* [in] */ unsigned long descbuflen);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetLength )( 
            ITSSMediaBaseInfo __RPC_FAR * This,
            /* [retval][out] */ unsigned long __RPC_FAR *length);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetTitle )( 
            ITSSMediaBaseInfo __RPC_FAR * This,
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetCopyright )( 
            ITSSMediaBaseInfo __RPC_FAR * This,
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetComment )( 
            ITSSMediaBaseInfo __RPC_FAR * This,
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetArtist )( 
            ITSSMediaBaseInfo __RPC_FAR * This,
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen);
        
        END_INTERFACE
    } ITSSMediaBaseInfoVtbl;

    interface ITSSMediaBaseInfo
    {
        CONST_VTBL struct ITSSMediaBaseInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITSSMediaBaseInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITSSMediaBaseInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITSSMediaBaseInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITSSMediaBaseInfo_GetMediaType(This,shortname,descbuf,descbuflen)	\
    (This)->lpVtbl -> GetMediaType(This,shortname,descbuf,descbuflen)

#define ITSSMediaBaseInfo_GetLength(This,length)	\
    (This)->lpVtbl -> GetLength(This,length)

#define ITSSMediaBaseInfo_GetTitle(This,buf,buflen)	\
    (This)->lpVtbl -> GetTitle(This,buf,buflen)

#define ITSSMediaBaseInfo_GetCopyright(This,buf,buflen)	\
    (This)->lpVtbl -> GetCopyright(This,buf,buflen)

#define ITSSMediaBaseInfo_GetComment(This,buf,buflen)	\
    (This)->lpVtbl -> GetComment(This,buf,buflen)

#define ITSSMediaBaseInfo_GetArtist(This,buf,buflen)	\
    (This)->lpVtbl -> GetArtist(This,buf,buflen)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT __stdcall ITSSMediaBaseInfo_GetMediaType_Proxy( 
    ITSSMediaBaseInfo __RPC_FAR * This,
    /* [in] */ LPWSTR shortname,
    /* [in] */ LPWSTR descbuf,
    /* [in] */ unsigned long descbuflen);


void __RPC_STUB ITSSMediaBaseInfo_GetMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSMediaBaseInfo_GetLength_Proxy( 
    ITSSMediaBaseInfo __RPC_FAR * This,
    /* [retval][out] */ unsigned long __RPC_FAR *length);


void __RPC_STUB ITSSMediaBaseInfo_GetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSMediaBaseInfo_GetTitle_Proxy( 
    ITSSMediaBaseInfo __RPC_FAR * This,
    /* [in] */ LPWSTR buf,
    /* [in] */ unsigned long buflen);


void __RPC_STUB ITSSMediaBaseInfo_GetTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSMediaBaseInfo_GetCopyright_Proxy( 
    ITSSMediaBaseInfo __RPC_FAR * This,
    /* [in] */ LPWSTR buf,
    /* [in] */ unsigned long buflen);


void __RPC_STUB ITSSMediaBaseInfo_GetCopyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSMediaBaseInfo_GetComment_Proxy( 
    ITSSMediaBaseInfo __RPC_FAR * This,
    /* [in] */ LPWSTR buf,
    /* [in] */ unsigned long buflen);


void __RPC_STUB ITSSMediaBaseInfo_GetComment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSMediaBaseInfo_GetArtist_Proxy( 
    ITSSMediaBaseInfo __RPC_FAR * This,
    /* [in] */ LPWSTR buf,
    /* [in] */ unsigned long buflen);


void __RPC_STUB ITSSMediaBaseInfo_GetArtist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITSSMediaBaseInfo_INTERFACE_DEFINED__ */


#ifndef __ITSSStorageProvider_INTERFACE_DEFINED__
#define __ITSSStorageProvider_INTERFACE_DEFINED__

/* interface ITSSStorageProvider */
/* [object][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ITSSStorageProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7DD61993-615D-481D-B060-A9FD48394430")
    ITSSStorageProvider : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT __stdcall GetStreamForRead( 
            /* [in] */ LPWSTR url,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *stream) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetStreamForWrite( 
            /* [in] */ LPWSTR url,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *stream) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetStreamForUpdate( 
            /* [in] */ LPWSTR url,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *stream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITSSStorageProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITSSStorageProvider __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITSSStorageProvider __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITSSStorageProvider __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetStreamForRead )( 
            ITSSStorageProvider __RPC_FAR * This,
            /* [in] */ LPWSTR url,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *stream);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetStreamForWrite )( 
            ITSSStorageProvider __RPC_FAR * This,
            /* [in] */ LPWSTR url,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *stream);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetStreamForUpdate )( 
            ITSSStorageProvider __RPC_FAR * This,
            /* [in] */ LPWSTR url,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *stream);
        
        END_INTERFACE
    } ITSSStorageProviderVtbl;

    interface ITSSStorageProvider
    {
        CONST_VTBL struct ITSSStorageProviderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITSSStorageProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITSSStorageProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITSSStorageProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITSSStorageProvider_GetStreamForRead(This,url,stream)	\
    (This)->lpVtbl -> GetStreamForRead(This,url,stream)

#define ITSSStorageProvider_GetStreamForWrite(This,url,stream)	\
    (This)->lpVtbl -> GetStreamForWrite(This,url,stream)

#define ITSSStorageProvider_GetStreamForUpdate(This,url,stream)	\
    (This)->lpVtbl -> GetStreamForUpdate(This,url,stream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT __stdcall ITSSStorageProvider_GetStreamForRead_Proxy( 
    ITSSStorageProvider __RPC_FAR * This,
    /* [in] */ LPWSTR url,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *stream);


void __RPC_STUB ITSSStorageProvider_GetStreamForRead_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSStorageProvider_GetStreamForWrite_Proxy( 
    ITSSStorageProvider __RPC_FAR * This,
    /* [in] */ LPWSTR url,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *stream);


void __RPC_STUB ITSSStorageProvider_GetStreamForWrite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSStorageProvider_GetStreamForUpdate_Proxy( 
    ITSSStorageProvider __RPC_FAR * This,
    /* [in] */ LPWSTR url,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *stream);


void __RPC_STUB ITSSStorageProvider_GetStreamForUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITSSStorageProvider_INTERFACE_DEFINED__ */


#ifndef __ITSSModule_INTERFACE_DEFINED__
#define __ITSSModule_INTERFACE_DEFINED__

/* interface ITSSModule */
/* [object][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ITSSModule;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A938D1A5-2980-498B-B051-99931D41895D")
    ITSSModule : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT __stdcall GetModuleCopyright( 
            /* [in] */ LPWSTR buffer,
            /* [in] */ unsigned long buflen) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetModuleDescription( 
            /* [in] */ LPWSTR buffer,
            /* [in] */ unsigned long buflen) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetSupportExts( 
            /* [in] */ unsigned long index,
            /* [in] */ LPWSTR mediashortname,
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetMediaInfo( 
            /* [in] */ LPWSTR url,
            /* [retval][out] */ ITSSMediaBaseInfo __RPC_FAR *__RPC_FAR *info) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetMediaSupport( 
            /* [in] */ LPWSTR url) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetMediaInstance( 
            /* [in] */ LPWSTR url,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *instance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITSSModuleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITSSModule __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITSSModule __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITSSModule __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetModuleCopyright )( 
            ITSSModule __RPC_FAR * This,
            /* [in] */ LPWSTR buffer,
            /* [in] */ unsigned long buflen);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetModuleDescription )( 
            ITSSModule __RPC_FAR * This,
            /* [in] */ LPWSTR buffer,
            /* [in] */ unsigned long buflen);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetSupportExts )( 
            ITSSModule __RPC_FAR * This,
            /* [in] */ unsigned long index,
            /* [in] */ LPWSTR mediashortname,
            /* [in] */ LPWSTR buf,
            /* [in] */ unsigned long buflen);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetMediaInfo )( 
            ITSSModule __RPC_FAR * This,
            /* [in] */ LPWSTR url,
            /* [retval][out] */ ITSSMediaBaseInfo __RPC_FAR *__RPC_FAR *info);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetMediaSupport )( 
            ITSSModule __RPC_FAR * This,
            /* [in] */ LPWSTR url);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetMediaInstance )( 
            ITSSModule __RPC_FAR * This,
            /* [in] */ LPWSTR url,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *instance);
        
        END_INTERFACE
    } ITSSModuleVtbl;

    interface ITSSModule
    {
        CONST_VTBL struct ITSSModuleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITSSModule_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITSSModule_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITSSModule_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITSSModule_GetModuleCopyright(This,buffer,buflen)	\
    (This)->lpVtbl -> GetModuleCopyright(This,buffer,buflen)

#define ITSSModule_GetModuleDescription(This,buffer,buflen)	\
    (This)->lpVtbl -> GetModuleDescription(This,buffer,buflen)

#define ITSSModule_GetSupportExts(This,index,mediashortname,buf,buflen)	\
    (This)->lpVtbl -> GetSupportExts(This,index,mediashortname,buf,buflen)

#define ITSSModule_GetMediaInfo(This,url,info)	\
    (This)->lpVtbl -> GetMediaInfo(This,url,info)

#define ITSSModule_GetMediaSupport(This,url)	\
    (This)->lpVtbl -> GetMediaSupport(This,url)

#define ITSSModule_GetMediaInstance(This,url,instance)	\
    (This)->lpVtbl -> GetMediaInstance(This,url,instance)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT __stdcall ITSSModule_GetModuleCopyright_Proxy( 
    ITSSModule __RPC_FAR * This,
    /* [in] */ LPWSTR buffer,
    /* [in] */ unsigned long buflen);


void __RPC_STUB ITSSModule_GetModuleCopyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSModule_GetModuleDescription_Proxy( 
    ITSSModule __RPC_FAR * This,
    /* [in] */ LPWSTR buffer,
    /* [in] */ unsigned long buflen);


void __RPC_STUB ITSSModule_GetModuleDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSModule_GetSupportExts_Proxy( 
    ITSSModule __RPC_FAR * This,
    /* [in] */ unsigned long index,
    /* [in] */ LPWSTR mediashortname,
    /* [in] */ LPWSTR buf,
    /* [in] */ unsigned long buflen);


void __RPC_STUB ITSSModule_GetSupportExts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSModule_GetMediaInfo_Proxy( 
    ITSSModule __RPC_FAR * This,
    /* [in] */ LPWSTR url,
    /* [retval][out] */ ITSSMediaBaseInfo __RPC_FAR *__RPC_FAR *info);


void __RPC_STUB ITSSModule_GetMediaInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSModule_GetMediaSupport_Proxy( 
    ITSSModule __RPC_FAR * This,
    /* [in] */ LPWSTR url);


void __RPC_STUB ITSSModule_GetMediaSupport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSModule_GetMediaInstance_Proxy( 
    ITSSModule __RPC_FAR * This,
    /* [in] */ LPWSTR url,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *instance);


void __RPC_STUB ITSSModule_GetMediaInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITSSModule_INTERFACE_DEFINED__ */


#ifndef __ITSSWaveDecoder_INTERFACE_DEFINED__
#define __ITSSWaveDecoder_INTERFACE_DEFINED__

/* interface ITSSWaveDecoder */
/* [object][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ITSSWaveDecoder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("313864E2-910E-496F-8A6D-43465C105B58")
    ITSSWaveDecoder : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT __stdcall GetFormat( 
            /* [in] */ TSSWaveFormat __RPC_FAR *format) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall Render( 
            /* [in] */ void __RPC_FAR *buf,
            /* [in] */ unsigned long bufsamplelen,
            /* [out] */ unsigned long __RPC_FAR *rendered,
            /* [retval][out] */ unsigned long __RPC_FAR *status) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall SetPosition( 
            /* [in] */ unsigned __int64 samplepos) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITSSWaveDecoderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITSSWaveDecoder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITSSWaveDecoder __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITSSWaveDecoder __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetFormat )( 
            ITSSWaveDecoder __RPC_FAR * This,
            /* [in] */ TSSWaveFormat __RPC_FAR *format);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *Render )( 
            ITSSWaveDecoder __RPC_FAR * This,
            /* [in] */ void __RPC_FAR *buf,
            /* [in] */ unsigned long bufsamplelen,
            /* [out] */ unsigned long __RPC_FAR *rendered,
            /* [retval][out] */ unsigned long __RPC_FAR *status);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *SetPosition )( 
            ITSSWaveDecoder __RPC_FAR * This,
            /* [in] */ unsigned __int64 samplepos);
        
        END_INTERFACE
    } ITSSWaveDecoderVtbl;

    interface ITSSWaveDecoder
    {
        CONST_VTBL struct ITSSWaveDecoderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITSSWaveDecoder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITSSWaveDecoder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITSSWaveDecoder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITSSWaveDecoder_GetFormat(This,format)	\
    (This)->lpVtbl -> GetFormat(This,format)

#define ITSSWaveDecoder_Render(This,buf,bufsamplelen,rendered,status)	\
    (This)->lpVtbl -> Render(This,buf,bufsamplelen,rendered,status)

#define ITSSWaveDecoder_SetPosition(This,samplepos)	\
    (This)->lpVtbl -> SetPosition(This,samplepos)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT __stdcall ITSSWaveDecoder_GetFormat_Proxy( 
    ITSSWaveDecoder __RPC_FAR * This,
    /* [in] */ TSSWaveFormat __RPC_FAR *format);


void __RPC_STUB ITSSWaveDecoder_GetFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSWaveDecoder_Render_Proxy( 
    ITSSWaveDecoder __RPC_FAR * This,
    /* [in] */ void __RPC_FAR *buf,
    /* [in] */ unsigned long bufsamplelen,
    /* [out] */ unsigned long __RPC_FAR *rendered,
    /* [retval][out] */ unsigned long __RPC_FAR *status);


void __RPC_STUB ITSSWaveDecoder_Render_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ITSSWaveDecoder_SetPosition_Proxy( 
    ITSSWaveDecoder __RPC_FAR * This,
    /* [in] */ unsigned __int64 samplepos);


void __RPC_STUB ITSSWaveDecoder_SetPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITSSWaveDecoder_INTERFACE_DEFINED__ */

#endif /* __TVPSndSysLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
