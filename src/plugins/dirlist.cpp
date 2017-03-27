//---------------------------------------------------------------------------
//#include <windows.h>
//#include "tp_stub.h"
#include "tp_stub.h"
#include "ncbind/ncbind.hpp"
//---------------------------------------------------------------------------

#define NCB_MODULE_NAME TJS_W("dirlist.dll")


//---------------------------------------------------------------------------
// 指定されたディレクトリ内のファイルの一覧を得る関数
//---------------------------------------------------------------------------
class tGetDirListFunction : public tTJSDispatch
{
	tjs_error TJS_INTF_METHOD FuncCall(
		tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
	{
		if(membername) return TJS_E_MEMBERNOTFOUND;

		// 引数 : ディレクトリ
		if(numparams < 1) return TJS_E_BADPARAMCOUNT;

		ttstr dir(*param[0]);

		if(dir.GetLastChar() != TJS_W('/'))
			TVPThrowExceptionMessage(TJS_W("'/' must be specified at the end of given directory name."));

		// OSネイティブな表現に変換
		dir = TVPNormalizeStorageName(dir);

		// Array クラスのオブジェクトを作成
		iTJSDispatch2 * array = TJSCreateArrayObject();
		if (!result) return TJS_S_OK;
		try {
			tTJSArrayNI* ni;
			array->NativeInstanceSupport(TJS_NIS_GETINSTANCE, TJSGetArrayClassID(), (iTJSNativeInstance**)&ni);
			TVPGetLocalName(dir);
			TVPGetLocalFileListAt(dir, [ni](const ttstr &name, tTVPLocalFileInfo* s) {
				if (s->Mode & (S_IFREG | S_IFDIR)) {
					ni->Items.emplace_back(name);
				}
			});
			*result = tTJSVariant(array, array);
			array->Release();
		}
		catch (...) {
			array->Release();
			throw;
		}

		// 戻る
		return TJS_S_OK;
	}
} * GetDirListFunction;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
static void PostRegistCallback()
{
	// GetDirListFunction の作成と登録
	tTJSVariant val;

	// TJS のグローバルオブジェクトを取得する
	iTJSDispatch2 * global = TVPGetScriptDispatch();

	// 1 まずオブジェクトを作成
	GetDirListFunction = new tGetDirListFunction();

	// 2 GetDirListFunction を tTJSVariant 型に変換
	val = tTJSVariant(GetDirListFunction);

	// 3 すでに val が GetDirListFunction を保持しているので、GetDirListFunction は
	//   Release する
	GetDirListFunction->Release();


	// 4 global の PropSet メソッドを用い、オブジェクトを登録する
	global->PropSet(
		TJS_MEMBERENSURE, // メンバがなかった場合には作成するようにするフラグ
		TJS_W("getDirList"), // メンバ名 ( かならず TJS_W( ) で囲む )
		NULL, // ヒント ( 本来はメンバ名のハッシュ値だが、NULL でもよい )
		&val, // 登録する値
		global // コンテキスト ( global でよい )
		);


	// - global を Release する
	global->Release();

	// val をクリアする。
	// これは必ず行う。そうしないと val が保持しているオブジェクト
	// が Release されず、次に使う TVPPluginGlobalRefCount が正確にならない。
	val.Clear();
}
NCB_POST_REGIST_CALLBACK(PostRegistCallback);
