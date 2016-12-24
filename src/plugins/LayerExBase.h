#ifndef __LayerExBase__
#define __LayerExBase__

//#include <windows.h>
//#include "tp_stub.h"
#include "tjsNative.h"

/**
 * レイヤ拡張 基本情報保持用ネイティブインスタンス。
 */
class NI_LayerExBase : public tTJSNativeInstance
{
protected:
	// レイヤから情報を取得するためのプロパティ
	// 少しでも高速化するためキャッシュしておく
	static iTJSDispatch2 * _leftProp;
	static iTJSDispatch2 * _topProp;
	static iTJSDispatch2 * _widthProp;
	static iTJSDispatch2 * _heightProp;
	static iTJSDispatch2 * _pitchProp;
	static iTJSDispatch2 * _bufferProp;
	static iTJSDispatch2 * _updateProp;

public:
	// レイヤ情報比較保持用
	tjs_int _width;
	tjs_int _height;
	tjs_int _pitch;
	unsigned char *_buffer;

public:
	// クラスＩＤ保持用
	static int classId;
	static void init(iTJSDispatch2 *layerobj);
	static void unInit();
	
	/**
	 * ネイティブオブジェクトの取得
	 * @param layerobj レイヤオブジェクト
	 * @return ネイティブオブジェクト
	 */
	static NI_LayerExBase *getNative(iTJSDispatch2 *objthis, bool create=true);

	/**
	 * 再描画要請
	 * @param layerobj レイヤオブジェクト
	 */
	void redraw(iTJSDispatch2 *layerobj);
	
	/**
	 * グラフィックを初期化する
	 * レイヤのビットマップ情報が変更されている可能性があるので毎回チェックする。
	 * 変更されている場合は描画用のコンテキストを組みなおす
	 * @param layerobj レイヤオブジェクト
	 * @return 初期化実行された場合は true を返す
	 */
	void reset(iTJSDispatch2 *layerobj);
	
public:
	/**
	 * コンストラクタ
	 */
	NI_LayerExBase();
};

#endif
