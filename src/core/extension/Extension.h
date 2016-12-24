
#ifndef __EXTENSION_H__
#define __EXTENSION_H__

//---------------------------------------------------------------------------
// tTVPAtInstallClass
//---------------------------------------------------------------------------
extern void TVPAddClassHandler( const tjs_char* name, iTJSDispatch2* (*handler)(iTJSDispatch2*), const tjs_char* dependences );
struct tTVPAtInstallClass {
	tTVPAtInstallClass(const tjs_char* name, iTJSDispatch2* (*handler)(iTJSDispatch2*), const tjs_char* dependences) {
		TVPAddClassHandler(name, handler, dependences);
	}
};

/*

以下のような書式で、cpp に入れておくと、VM初期化後、クラスを追加する段階で追加してくれます。
静的リンクするとクラス追加されるpluginの様な機能です。
pluginは実行時にクラス等追加しますが、extensionはビルドする時に、入れるクラスを選択する形です。
static tTVPAtInstallClass TVPInstallClassFoo
	(TJS_W("ClassFoo"), TVPCreateNativeClass_ClassFoo,TJS_W("Window,Layer"));
呼び出される関数にはglobalが渡されるので、必要であればそこからメンバなど取得します。
登録は呼び出し側が返り値のiTJSDispatch2をglobalに登録するので、呼び出された関数内で登録する必要
はありません。
登録時依存クラスを3番目に指定可能ですが、現在のところ無視されています。
*/
extern void TVPCauseAtInstallExtensionClass( iTJSDispatch2 *global );


#endif // __EXTENSION_H__
