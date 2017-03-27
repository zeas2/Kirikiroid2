#include "ncbind/ncbind.hpp"
#include "XP3Archive.h"
#include "SystemIntf.h"
#include "tjsNative.h"
#include <assert.h>
#include <list>
#include "tjsDebug.h"
#include "xp3filter.h"
#include "SysInitIntf.h"
#include "ThreadIntf.h"
#include <memory>
#include <thread>

#define NCB_MODULE_NAME TJS_W("xp3filter.dll")

tjs_error TJS_INTF_METHOD CBinaryAccessor::OperationByNum( /* operation with member by index number */ tjs_uint32 flag, /* calling flag */ tjs_int num, /* index number */ tTJSVariant *result, /* result ( can be NULL ) */ const tTJSVariant *param, /* parameters */ iTJSDispatch2 *objthis /* object as "this" */)
{
	num += m_curPos;
	if (num < 0 || num >= m_length) return TJS_E_MEMBERNOTFOUND;
	unsigned char opnum = param->AsInteger();
	switch (flag & TJS_OP_MASK) {
	case TJS_OP_BAND: m_buff[num] &= opnum; break;
	case TJS_OP_BOR:  m_buff[num] |= opnum; break;
	case TJS_OP_BXOR: m_buff[num] ^= opnum; break;
	case TJS_OP_SUB:  m_buff[num] -= opnum; break;
	case TJS_OP_ADD:  m_buff[num] += opnum; break;
	case TJS_OP_MOD:  m_buff[num] %= opnum; break;
	case TJS_OP_DIV:  m_buff[num] /= (signed char)opnum; break;
	case TJS_OP_IDIV: m_buff[num] /= opnum; break;
	case TJS_OP_MUL:  m_buff[num] *= opnum; break;
	case TJS_OP_LOR:  m_buff[num] = m_buff[num] || opnum; break;
	case TJS_OP_LAND: m_buff[num] = m_buff[num] && opnum; break;
	case TJS_OP_SAR:  m_buff[num] >>= opnum; break;
	case TJS_OP_SAL:  m_buff[num] <<= opnum; break;
	case TJS_OP_SR:   m_buff[num] >>= opnum; break;
	case TJS_OP_INC:  m_buff[num] ++; break;
	case TJS_OP_DEC:  m_buff[num] --; break;
	default:
		return TJS_E_NOTIMPL;
	}

	return TJS_S_OK;
}

tjs_error TJS_INTF_METHOD CBinaryAccessor::Operation( /* operation with member */ tjs_uint32 flag, /* calling flag */ const tjs_char *membername, /* member name ( NULL for a default member ) */ tjs_uint32 *hint, /* hint for the member name (in/out) */ tTJSVariant *result, /* result ( can be NULL ) */ const tTJSVariant *param, /* parameters */ iTJSDispatch2 *objthis /* object as "this" */)
{
	if (membername) {
		static const ttstr str_ptr(TJS_W("ptr"));
		if (hint) {
			static const tjs_uint32 hash_ptr = tTJSHashFunc<tjs_char *>::Make(str_ptr.c_str());
			if (!*hint)
				*hint = tTJSHashFunc<tjs_char *>::Make(membername);
			if (*hint != hash_ptr)
				return TJS_E_NOTIMPL;
		} else if (str_ptr != membername) {
			return TJS_E_NOTIMPL;
		}
	}
	tjs_uint32 op = flag & TJS_OP_MASK;
	switch (op) {
	case TJS_OP_ADD:
		m_curPos += param->AsInteger();
		break;
	case TJS_OP_SUB:
		m_curPos -= param->AsInteger();
		break;
	case TJS_OP_INC:
		++m_curPos;
		break;
	case TJS_OP_DEC:
		--m_curPos;
		break;
	default:
		return TJS_E_NOTIMPL;
	}
	return TJS_S_OK;
}

tjs_error TJS_INTF_METHOD CBinaryAccessor::IsValid( /* get validation, returns TJS_S_TRUE (valid) or TJS_S_FALSE (invalid) */ tjs_uint32 flag, /* calling flag */ const tjs_char *membername, /* member name ( NULL for a default member ) */ tjs_uint32 *hint, /* hint for the member name (in/out) */ iTJSDispatch2 *objthis /* object as "this" */)
{
	return m_buff ? TJS_S_TRUE : TJS_S_FALSE;
}

tjs_error TJS_INTF_METHOD CBinaryAccessor::Invalidate( /* invalidation */ tjs_uint32 flag, /* calling flag */ const tjs_char *membername, /* member name ( NULL for a default member ) */ tjs_uint32 *hint, /* hint for the member name (in/out) */ iTJSDispatch2 *objthis /* object as "this" */)
{
	m_buff = NULL;
	return TJS_S_OK;
}

tjs_error TJS_INTF_METHOD CBinaryAccessor::GetCount( /* get member count */ tjs_int *result, /* variable that receives the result */ const tjs_char *membername, /* member name ( NULL for a default member ) */ tjs_uint32 *hint, /* hint for the member name (in/out) */ iTJSDispatch2 *objthis /* object as "this" */)
{
	if (membername) return TJS_E_MEMBERNOTFOUND;
	*result = m_length;
	return TJS_S_OK;
}

tjs_error TJS_INTF_METHOD CBinaryAccessor::PropSetByNum( /* property set by index number */ tjs_uint32 flag, /* calling flag */ tjs_int num, /* index number */ const tTJSVariant *param, /* parameters */ iTJSDispatch2 *objthis /* object as "this" */)
{
	num += m_curPos;
	if (num < 0 || num >= m_length) return TJS_E_MEMBERNOTFOUND;
	m_buff[num] = param->AsInteger();
	return TJS_S_OK;
}

tjs_error TJS_INTF_METHOD CBinaryAccessor::PropSet( /* property set */ tjs_uint32 flag, /* calling flag */ const tjs_char *membername, /* member name ( NULL for a default member ) */ tjs_uint32 *hint, /* hint for the member name (in/out) */ const tTJSVariant *param, /* parameters */ iTJSDispatch2 *objthis /* object as "this" */)
{
	if (!membername) return TJS_E_NOTIMPL;
	if (!TJS_strcmp(membername, TJS_W("ptr"))) {
		m_curPos = param->AsInteger();
		return TJS_S_OK;
	}
	return TJS_E_NOTIMPL;
}

tjs_error TJS_INTF_METHOD CBinaryAccessor::PropGetByNum( /* property get by index number */ tjs_uint32 flag, /* calling flag */ tjs_int num, /* index number */ tTJSVariant *result, /* result */ iTJSDispatch2 *objthis /* object as "this" */)
{
	num += m_curPos;
	if (num < 0 || num >= m_length) return TJS_E_MEMBERNOTFOUND;
	result->operator =((tjs_int32)m_buff[num]);
	return TJS_S_OK;
}

tjs_error TJS_INTF_METHOD CBinaryAccessor::PropGet( /* property get */ tjs_uint32 flag, /* calling flag */ const tjs_char * membername,/* member name ( NULL for a default member ) */ tjs_uint32 *hint, /* hint for the member name (in/out) */ tTJSVariant *result, /* result */ iTJSDispatch2 *objthis /* object as "this" */)
{
	if (!membername) return TJS_E_NOTIMPL;
	if (!TJS_strcmp(membername, TJS_W("count"))) {
		result->operator =((tjs_int64)m_length);
	} else if (!TJS_strcmp(membername, TJS_W("ptr"))) {
		result->operator =((tjs_int64)m_curPos);
	} else {
		result->Clear();
	}
	return TJS_S_OK;
}

tjs_error TJS_INTF_METHOD CBinaryAccessor::FuncCall( /* function invocation */ tjs_uint32 flag, /* calling flag */ const tjs_char * membername,/* member name ( NULL for a default member ) */ tjs_uint32 *hint, /* hint for the member name (in/out) */ tTJSVariant *result, /* result */ tjs_int numparams, /* number of parameters */ tTJSVariant **param, /* parameters */ iTJSDispatch2 *objthis /* object as "this" */)
{
	if (hint) {
		if (!*hint) {
			*hint = !TJS_strcmp(membername, TJS_W("xor"));
			if (*hint) {
				return FuncXor(numparams, param);
			}
		} else {
			return FuncXor(numparams, param);
		}
	} else if (!TJS_strcmp(membername, TJS_W("xor"))) {
		return FuncXor(numparams, param);
	}
	return TJS_E_NOTIMPL;
}

CBinaryAccessor::CBinaryAccessor(unsigned char* buff, unsigned int len)
{
	m_buff = buff;
	m_length = len;
	m_curPos = 0;
}

tjs_error CBinaryAccessor::FuncAdd(tjs_int numparams, tTJSVariant **param)
{
	if (numparams < 3) {
		return TJS_E_BADPARAMCOUNT;
	}

	int bufoff = param[0]->AsInteger();
	int len = param[1]->AsInteger();
	unsigned char xorval = param[2]->AsInteger();

	unsigned char *buf = m_buff + m_curPos + bufoff;
	unsigned int i;
	for (i = 0; i < len; ++i)
		buf[i] += xorval;
	return TJS_S_OK;
}

tjs_error CBinaryAccessor::FuncXor(tjs_int numparams, tTJSVariant **param)
{
	if (numparams < 3) {
		return TJS_E_BADPARAMCOUNT;
	}

	int bufoff = param[0]->AsInteger();
	int len = param[1]->AsInteger();
	unsigned char xorval = param[2]->AsInteger();

	unsigned char *buf = m_buff + m_curPos + bufoff;
	unsigned char *pend = buf + len;
	if (len>32)
	{
		int PreFragLen = (unsigned char*)((((intptr_t)buf) + 7)&~7) - buf;
		for (int i = 0; i < PreFragLen; i++) *(buf++) ^= xorval;

		uint64_t k = /*0x101010101010101 * */xorval;
		k |= k << 8;
		k |= k << 16;
		k |= k << 32;
		unsigned char* pVecEnd = (unsigned char*)(((intptr_t)pend)&~7) - 7;
		while (buf < pVecEnd) {
			*(uint64_t*)(buf) ^= k;
			buf += 8;
		}
	}
	while (buf < pend) *(buf++) ^= xorval;

	return TJS_S_OK;
}

static bool _ManagedDecoderInited = false;
static bool _ManagedFilterInited = false;

struct XP3FilterDecoder {
	tTJS * ScriptEngine = new tTJS();
	tTJSVariantClosure ManagedDecoder;
	tTJSVariantClosure ManagedFilter;
	XP3FilterDecoder() : ManagedDecoder(nullptr), ManagedFilter(nullptr) { }
	~XP3FilterDecoder() {
		if (ScriptEngine) ScriptEngine->Release();
	}
};

// static std::vector<XP3FilterDecoder*> sTVPScriptEngineStack;
// static std::mutex sTVPScriptEngineStackLock;
static ttstr sXP3FilterScript;

class XP3FilterRegister : public tTJSDispatch
{
    typedef tTJSDispatch inherited;
protected:
    XP3FilterDecoder *Decoder;
public:

    XP3FilterRegister(XP3FilterDecoder *decoder) {
        Decoder = decoder;
        if(TJSObjectHashMapEnabled()) TJSAddObjectHashRecord(this);
    }
    ~XP3FilterRegister() {
        if(TJSObjectHashMapEnabled()) TJSRemoveObjectHashRecord(this);
    }

    tjs_error TJS_INTF_METHOD
        IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
        const tjs_char *classname,
        iTJSDispatch2 *objthis)
    {
        if(membername == NULL)
        {
            if(!TJS_strcmp(classname, TJS_W("Function"))) return TJS_S_TRUE;
        }

        return inherited::IsInstanceOf(flag, membername, hint, classname, objthis);
    }
    tjs_error  TJS_INTF_METHOD
        FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
        tTJSVariant *result,
        tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis)
    {
        if(membername) return inherited::FuncCall(flag, membername, hint,
            result, numparams, param, objthis);
        if(!objthis) return TJS_E_NATIVECLASSCRASH;

        if(result) result->Clear();

        if (numparams < 1) return TJS_E_BADPARAMCOUNT;
        if (Decoder->ManagedDecoder.Object) Decoder->ManagedDecoder.Release();
        Decoder->ManagedDecoder = param[0]->AsObjectClosure();
		_ManagedDecoderInited = true;
        return TJS_S_OK;
    }
};

class XP3ContentFilterRegister : public XP3FilterRegister {
	typedef XP3FilterRegister inherited;
public:
	XP3ContentFilterRegister(XP3FilterDecoder *decoder) : XP3FilterRegister(decoder) {}
	tjs_error  TJS_INTF_METHOD
		FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
	{
		if (membername) return inherited::FuncCall(flag, membername, hint,
			result, numparams, param, objthis);
		if (!objthis) return TJS_E_NATIVECLASSCRASH;

		if (result) result->Clear();

		if (numparams < 1) return TJS_E_BADPARAMCOUNT;
		if (Decoder->ManagedFilter.Object) Decoder->ManagedFilter.Release();
		Decoder->ManagedFilter = param[0]->AsObjectClosure();
		_ManagedFilterInited = true;
		return TJS_S_OK;
	}
};

static XP3FilterDecoder* AddXP3Decoder() {
    XP3FilterDecoder *decoder = new XP3FilterDecoder;
    tTJSVariant val;
    iTJSDispatch2 *dsp;
	iTJSDispatch2 *global = decoder->ScriptEngine->GetGlobalNoAddRef();
#define REGISTER_OBJECT(classname, instance) \
    dsp = (instance); \
    val = tTJSVariant(dsp/*, dsp*/); \
    dsp->Release(); \
    global->PropSet(TJS_MEMBERENSURE|TJS_IGNOREPROP, TJS_W(#classname), NULL, \
    &val, global);
    REGISTER_OBJECT(Debug, TVPCreateNativeClass_Debug());
    REGISTER_OBJECT(System, TVPCreateNativeClass_System());
    tTJSNativeClass *cls = TVPCreateNativeClass_Storages();
    TJSNativeClassRegisterNCM(cls, TJS_W("setXP3ArchiveExtractionFilter"),
        new XP3FilterRegister(decoder),
		cls->GetClassName().c_str(), nitMethod, TJS_STATICMEMBER);
	TJSNativeClassRegisterNCM(cls, TJS_W("setXP3ArchiveContentFilter"),
		new XP3ContentFilterRegister(decoder),
		cls->GetClassName().c_str(), nitMethod, TJS_STATICMEMBER);
    REGISTER_OBJECT(Storages, cls);

	decoder->ScriptEngine->ExecScript(sXP3FilterScript);
//	sTVPScriptEngineStack.emplace_back(decoder);
	return decoder;
}

static std::map<std::thread::id, XP3FilterDecoder*> _thread_decoders;
#if 1 || (defined(_MSC_VER) /*&& _MSC_VER <= 1800*/) || defined(CC_TARGET_OS_IPHONE)
static std::mutex _decoders_mtx;
static std::vector<XP3FilterDecoder*> _cached_decoders;
static XP3FilterDecoder *FetchXP3Decoder() {
	std::lock_guard<std::mutex> lk(_decoders_mtx);
	auto it = _thread_decoders.find(std::this_thread::get_id());
	if (it != _thread_decoders.end()) {
		XP3FilterDecoder *ret = it->second;
		return ret;
	}
	static bool Inited = false;
	if (!Inited) {
		Inited = true;
		TVPAddOnThreadExitEvent([](){
			std::lock_guard<std::mutex> lk(_decoders_mtx);
			auto it = _thread_decoders.find(std::this_thread::get_id());
			if (it != _thread_decoders.end()) {
				_cached_decoders.emplace_back(it->second);
				_thread_decoders.erase(it);
			}
		});
	}
	XP3FilterDecoder *ret;
	if(!_cached_decoders.empty()) {
		ret = _cached_decoders.back();
		_cached_decoders.pop_back();
	} else {
		ret = AddXP3Decoder();
	}
	_thread_decoders[std::this_thread::get_id()] = ret;
	return ret;
}
#else
static XP3FilterDecoder *FetchXP3Decoder() {
	thread_local std::auto_ptr<XP3FilterDecoder> ret;
	if(!ret) ret = AddXP3Decoder();
	return ret.get();
}
#endif
tjs_int TVPXP3ArchiveContentFilterWrapper(const ttstr &filepath, const ttstr &archivename, tjs_uint64 filesize, tTJSVariant *ctx) {
	if (!_ManagedFilterInited) return 0;

	XP3FilterDecoder* decoder = FetchXP3Decoder();
	if (!decoder->ManagedFilter.Object) return 0;
	tTJSVariant FilePath(filepath);
	tTJSVariant ArcName(archivename);
	tTJSVariant FileSize((tjs_int64)filesize);
	tTJSVariant *vars[] = {
		&FilePath, &ArcName, &FileSize
	};
	tTJSVariant result;
	decoder->ManagedFilter.FuncCall(0, NULL, NULL, &result, sizeof(vars) / sizeof(vars[0]), vars, NULL);
	tjs_int ret = 0;
	if (result.Type() == tvtObject) {
		iTJSDispatch2* arr = result.AsObjectNoAddRef();
		ncbPropAccessor a(arr);
		ret = a.GetValue(0, ncbTypedefs::Tag<tjs_int>());
		*ctx = a.GetValue(1, ncbTypedefs::Tag<tTJSVariant>());
	}
	return ret;
}

void TVP_tTVPXP3ArchiveExtractionFilter_CONVENTION
    TVPXP3ArchiveExtractionFilterWrapper(tTVPXP3ExtractionFilterInfo *info, tTJSVariant *ctx)
{
    if (info->SizeOfSelf != sizeof(tTVPXP3ExtractionFilterInfo))
        TVPThrowExceptionMessage(TJS_W("Incompatible tTVPXP3ExtractionFilterInfo size"));
	XP3FilterDecoder* decoder = FetchXP3Decoder();
    if (decoder->ManagedDecoder.Object) {
        tTJSVariant FileHash = (tjs_int64)info->FileHash;
        tTJSVariant Offset = (tjs_int64)info->Offset;
		CBinaryAccessor *buf = new CBinaryAccessor((unsigned char*)info->Buffer, info->BufferSize);
        tTJSVariant Buffer(buf);
		buf->Release();
		tTJSVariant BufferSize((tjs_int64)info->BufferSize);
		tTJSVariant FileName(info->FileName);
        tTJSVariant *vars[] = {
            &FileHash, &Offset, &Buffer, &BufferSize, &FileName, ctx
        };
#if defined(WIN32) && defined(CHECK_CXDEC)
        unsigned char *pBackup = new unsigned char[info->BufferSize], *pBuffer = (unsigned char*)info->Buffer;
        memcpy(pBackup, info->Buffer, info->BufferSize);
#endif
		decoder->ManagedDecoder.FuncCall(0, NULL, NULL, NULL, sizeof(vars) / sizeof(vars[0]), vars, NULL);
#if defined(WIN32) && defined(CHECK_CXDEC)
		cxdec_decode(&dec_callback, info->FileHash, info->Offset, pBackup, info->BufferSize);
        for (int i = 0; i < info->BufferSize; ++i)
        {
            if (pBackup[i] != pBuffer[i]) {
                assert(false);
				break;
            }
        }
        delete []pBackup;
#endif
    }
}

void TVPSetXP3FilterScript(ttstr content) {
	if (sXP3FilterScript != content) {
		for (auto it : _thread_decoders) {
			delete it.second;
		}
		_thread_decoders.clear();
	}
	if (content.IsEmpty()) {
		TVPSetXP3ArchiveExtractionFilter(nullptr);
		TVPSetXP3ArchiveContentFilter(nullptr);
	} else {
		TVPSetXP3ArchiveExtractionFilter(TVPXP3ArchiveExtractionFilterWrapper);
		TVPSetXP3ArchiveContentFilter(TVPXP3ArchiveContentFilterWrapper);
	}
	sXP3FilterScript = content;
}

static void PostRegistCallback()
{
    ttstr path = TVPGetAppPath() + TJS_W("xp3filter.tjs");
    if (TVPIsExistentStorageNoSearch(path)) {
        iTJSTextReadStream * stream = TVPCreateTextStreamForRead(path, "");
        try
        {
            stream->Read(sXP3FilterScript, 0);
        }
        catch(...)
        {
            stream->Destruct();
            throw;
        }
        stream->Destruct();
    //    AddXP3Decoder();
		TVPSetXP3ArchiveExtractionFilter(TVPXP3ArchiveExtractionFilterWrapper);
		TVPSetXP3ArchiveContentFilter(TVPXP3ArchiveContentFilterWrapper);
    }
    //TVPSetXP3ArchiveExtractionFilter(TVPXP3ArchiveExtractionFilter);
}

NCB_POST_REGIST_CALLBACK(PostRegistCallback);
