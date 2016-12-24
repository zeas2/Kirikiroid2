#include "ncbind/ncbind.hpp"
#include "PluginIntf.h"
#define NCB_MODULE_NAME TJS_W("saveStruct.dll")

class tTVPStringStream
{
	tTJSBinaryStream *stream;
	const tjs_char *_newline;

public:
	tTVPStringStream(tTJSBinaryStream *s, bool onlyLF = false);
	void write(tjs_char c);
	void write(const tjs_char *s);
	void write(tTVInteger var);
	void write(tTVReal var);
	void newline();
};

tTVPStringStream::tTVPStringStream(tTJSBinaryStream *s, bool onlyLF /*= false*/) : stream(s) {
	if (onlyLF)  _newline = TJS_W("\n");
	else        _newline = TJS_W("\r\n");
}

void tTVPStringStream::write(tjs_char c) {
	stream->Write(&c, sizeof(c));
}

void tTVPStringStream::write(const tjs_char *s) {
	stream->Write(s, TJS_strlen(s) * sizeof(*s));
}

void tTVPStringStream::write(tTVInteger var) {
	ttstr s = tTJSVariant(var).AsString();
	stream->Write(s.c_str(), s.length() * sizeof(tjs_char));
}

void tTVPStringStream::write(tTVReal var) {
	ttstr s = tTJSVariant(var).AsString();
	stream->Write(s.c_str(), s.length() * sizeof(tjs_char));
}

void tTVPStringStream::newline() {
	write(_newline);
}

static void
quoteString(const tjs_char *str, tTVPStringStream *writer)
{
	if (str) {
		writer->write((tjs_char)'"');
		const tjs_char *p = str;
		int ch;
		while ((ch = *p++)) {
			if (ch == '"') {
				writer->write(TJS_W("\\\""));
			} else if (ch == '\\') {
				writer->write(TJS_W("\\\\"));
			} else {
				writer->write((tjs_char)ch);
			}
		}
		writer->write((tjs_char)'"');
	} else {
		writer->write(TJS_W("\"\""));
	}
}

static void quoteOctet(tTJSVariantOctet *octet, tTVPStringStream *writer)
{
  const tjs_uint8 *data = octet->GetData();
  tjs_uint length = octet->GetLength();
  writer->write(TJS_W("<% "));
  for (tjs_uint i = 0; i < length; i++) {
    tjs_char buf[256];
    TJS_sprintf(buf, TJS_W("%02x "), data[i]);
    writer->write(buf);
  }
  writer->write(TJS_W("%>"));
}

static void getVariantString(tTJSVariant &var, tTVPStringStream *writer);

/**
 * 辞書の内容表示用の呼び出しロジック
 */
class DictMemberDispCaller : public tTJSDispatch /** EnumMembers 用 */
{
protected:
	tTVPStringStream *writer;
	bool first;
public:
	DictMemberDispCaller(tTVPStringStream *writer) : writer(writer) { first = true; };
	virtual tjs_error TJS_INTF_METHOD FuncCall( // function invocation
												tjs_uint32 flag,			// calling flag
												const tjs_char * membername,// member name ( NULL for a default member )
												tjs_uint32 *hint,			// hint for the member name (in/out)
												tTJSVariant *result,		// result
												tjs_int numparams,			// number of parameters
												tTJSVariant **param,		// parameters
												iTJSDispatch2 *objthis		// object as "this"
												) {
		if (numparams > 1) {
			tTVInteger flag = param[1]->AsInteger();
			if (!(flag & TJS_HIDDENMEMBER)) {
				if (first) {
					first = false;
				} else {
					writer->write((tjs_char)',');
					writer->newline();
				}
				const tjs_char *name = param[0]->GetString();
				quoteString(name, writer);
				writer->write(TJS_W("=>"));
				getVariantString(*param[2], writer);
			}
		}
		if (result) {
			*result = true;
		}
		return TJS_S_OK;
	}
};

static void getDictString(iTJSDispatch2 *dict, tTVPStringStream *writer)
{
	writer->write(TJS_W("%["));
	//writer->addIndent();
	DictMemberDispCaller *caller = new DictMemberDispCaller(writer);
	tTJSVariantClosure closure(caller);
	dict->EnumMembers(TJS_IGNOREPROP, &closure, dict);
	caller->Release();
	//writer->delIndent();
	writer->write((tjs_char)']');
}

// Array クラスメンバ
static iTJSDispatch2 *ArrayCountProp   = NULL;   // Array.count

static void getArrayString(iTJSDispatch2 *array, tTVPStringStream *writer)
{
	writer->write((tjs_char)'[');
	//writer->addIndent();
	tjs_int count = 0;
	{
		tTJSVariant result;
		if (TJS_SUCCEEDED(ArrayCountProp->PropGet(0, NULL, NULL, &result, array))) {
			count = result;
		}
	}
	for (tjs_int i=0; i<count; i++) {
		if (i != 0) {
			writer->write((tjs_char)',');
			//writer->newline();
		}
		tTJSVariant result;
		if (array->PropGetByNum(TJS_IGNOREPROP, i, &result, array) == TJS_S_OK) {
			getVariantString(result, writer);
		}
	}
	//writer->delIndent();
	writer->write((tjs_char)']');
}

static void
getVariantString(tTJSVariant &var, tTVPStringStream *writer)
{
	switch(var.Type()) {

	case tvtVoid:
		writer->write(TJS_W("void"));
		break;
		
	case tvtObject:
		{
			iTJSDispatch2 *obj = var.AsObjectNoAddRef();
			if (obj == NULL) {
				writer->write(TJS_W("null"));
			} else if (obj->IsInstanceOf(TJS_IGNOREPROP,NULL,NULL,TJS_W("Array"),obj) == TJS_S_TRUE) {
				getArrayString(obj, writer);
			} else {
				getDictString(obj, writer);
			}
		}
		break;
		
	case tvtString:
		quoteString(var.GetString(), writer);
		break;

        case tvtOctet:
               quoteOctet(var.AsOctetNoAddRef(), writer);
               break;

	case tvtInteger:
		writer->write(TJS_W("int "));
		writer->write(TJSIntegerToString((tTVInteger)var)->operator const tjs_char *());
		break;

	case tvtReal:
		writer->write(TJS_W("real "));
		writer->write((tTVReal)var);
		break;

	default:
		writer->write(TJS_W("void"));
		break;
	};
}

//---------------------------------------------------------------------------

/**
 * メソッド追加用
 */
class ArrayAdd {

public:
	ArrayAdd(){};

	/**
	 * save 形式での辞書または配列の保存
	 * @param filename ファイル名
	 * @param utf true なら UTF-8 で出力
	 * @param newline 改行コード 0:CRLF 1:LF
	 * @return 実行結果
	 */
	static tjs_error TJS_INTF_METHOD save2(tTJSVariant *result,
										   tjs_int numparams,
										   tTJSVariant **param,
										   iTJSDispatch2 *objthis) {
		if (numparams < 1) return TJS_E_BADPARAMCOUNT;
        tTJSBinaryStream *stream = TVPCreateStream(param[0]->AsString(), TJS_BS_WRITE);
        tTVPStringStream writer(stream,
            //numparams > 1 ? (int)*param[1] != 0: false,
            numparams > 2 ? (int)*param[2] : 0
            );
// 		IFileWriter writer(param[0]->GetString(),
// 						   numparams > 1 ? (int)*param[1] != 0: false,
// 						   numparams > 2 ? (int)*param[2] : 0
// 						   );
//                 writer.hex = true;
		tjs_int count = 0;
		{
			tTJSVariant result;
			if (TJS_SUCCEEDED(ArrayCountProp->PropGet(0, NULL, NULL, &result, objthis))) {
				count = result;
			}
		}
		for (tjs_int i=0; i<count; i++) {
			tTJSVariant result;
			if (objthis->PropGetByNum(TJS_IGNOREPROP, i, &result, objthis) == TJS_S_OK) {
				writer.write(result.GetString());
				writer.newline();
			}
		}
        delete stream;
		return TJS_S_OK;
	}

	/**
	 * saveStruct 形式でのオブジェクトの保存
	 * @param filename ファイル名
	 * @param utf true なら UTF-8 で出力
	 * @param newline 改行コード 0:CRLF 1:LF
	 * @return 実行結果
	 */
	static tjs_error TJS_INTF_METHOD saveStruct2(tTJSVariant *result,
												 tjs_int numparams,
												 tTJSVariant **param,
												 iTJSDispatch2 *objthis) {
		if (numparams < 1) return TJS_E_BADPARAMCOUNT;
        tTJSBinaryStream *stream = TVPCreateStream(param[0]->AsString(), TJS_BS_WRITE);
		tTVPStringStream writer(stream,
						   //numparams > 1 ? (int)*param[1] != 0: false,
						   numparams > 2 ? (int)*param[2] : 0
						   );
// 		IFileWriter writer(param[0]->GetString(),
// 						   numparams > 1 ? (int)*param[1] != 0: false,
// 						   numparams > 2 ? (int)*param[2] : 0
// 						   );
//                 writer.hex = true;
		getArrayString(objthis, &writer);
        delete stream;
		return TJS_S_OK;
	}
	
	/**
	 * saveStruct 形式で文字列化
	 * @param newline 改行コード 0:CRLF 1:LF
	 * @return 実行結果
	 */
	static tjs_error TJS_INTF_METHOD toStructString(tTJSVariant *result,
													tjs_int numparams,
													tTJSVariant **param,
													iTJSDispatch2 *objthis) {
		if (result) {
            tTVPMemoryStream ms;
            tTVPStringStream writer(&ms, numparams > 0 ? (int)*param[0] : 0);
			getArrayString(objthis, &writer);
            *result = (const tjs_char*)ms.GetInternalBuffer();
		}
		return TJS_S_OK;
	}
};

NCB_ATTACH_CLASS(ArrayAdd, Array) {
	RawCallback("save2", &ArrayAdd::save2, 0);
	RawCallback("saveStruct2", &ArrayAdd::saveStruct2, 0);
	RawCallback("toStructString", &ArrayAdd::toStructString, 0);
};

/**
 * メソッド追加用
 */
class DictAdd {

public:
	DictAdd(){};

	/**
	 * saveStruct 形式でのオブジェクトの保存
	 * @param filename ファイル名
	 * @param utf true なら UTF-8 で出力
	 * @param newline 改行コード 0:CRLF 1:LF
	 * @return 実行結果
	 */
	static tjs_error TJS_INTF_METHOD saveStruct2(tTJSVariant *result,
												 tjs_int numparams,
												 tTJSVariant **param,
												 iTJSDispatch2 *objthis) {
		if (numparams < 1) return TJS_E_BADPARAMCOUNT;
        tTJSBinaryStream *stream = TVPCreateStream(param[0]->AsString(), TJS_BS_WRITE);
		tTVPStringStream writer(stream,
						   //numparams > 1 ? (int)*param[1] != 0: false,
						   numparams > 2 ? (int)*param[2] : 0
						   );
		getDictString(objthis, &writer);
        delete stream;
		return TJS_S_OK;
	}
	
	/**
	 * saveStruct 形式で文字列化
	 * @param newline 改行コード 0:CRLF 1:LF
	 * @return 実行結果
	 */
	static tjs_error TJS_INTF_METHOD toStructString(tTJSVariant *result,
													tjs_int numparams,
													tTJSVariant **param,
													iTJSDispatch2 *objthis) {
		if (result) {
            tTVPMemoryStream ms;
            tTVPStringStream writer(&ms, numparams > 0 ? (int)*param[0] : 0);
			getDictString(objthis, &writer);
            writer.write((tjs_char)0);
			*result = (const tjs_char*)ms.GetInternalBuffer();
		}
		return TJS_S_OK;
	}
};

NCB_ATTACH_CLASS(DictAdd, Dictionary) {
	RawCallback("saveStruct2", &DictAdd::saveStruct2, TJS_STATICMEMBER);
	RawCallback("toStructString", &DictAdd::toStructString, TJS_STATICMEMBER);
};

/**
 * 登録処理後
 */
static void PostRegistCallback()
{
	// Array.count を取得
	{
		tTJSVariant varScripts;
		TVPExecuteExpression(TJS_W("Array"), &varScripts);
		iTJSDispatch2 *dispatch = varScripts.AsObjectNoAddRef();
		tTJSVariant val;
		if (TJS_FAILED(dispatch->PropGet(TJS_IGNOREPROP,
										 TJS_W("count"),
										 NULL,
										 &val,
										 dispatch))) {
			TVPThrowExceptionMessage(TJS_W("can't get Array.count"));
		}
		ArrayCountProp = val.AsObject();
	}
}

/**
 * 開放処理前
 */
static void PreUnregistCallback()
{
	if (ArrayCountProp) {
		ArrayCountProp->Release();
		ArrayCountProp = NULL;
	}
}

NCB_POST_REGIST_CALLBACK(PostRegistCallback);
NCB_PRE_UNREGIST_CALLBACK(PreUnregistCallback);
