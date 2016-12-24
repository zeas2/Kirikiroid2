

#ifndef __MOUSE_CURSOR_H__
#define __MOUSE_CURSOR_H__

#include <vector>

class MouseCursor {
	enum {
		CURSOR_APPSTARTING,	// 標準矢印カーソルおよび小型砂時計カーソル
		CURSOR_ARROW,		// 標準矢印カーソル
		CURSOR_CROSS,		// 十字カーソル
		CURSOR_HAND,		// ハンドカーソル
		CURSOR_IBEAM,		// アイビーム (縦線) カーソル
		CURSOR_HELP,		// 矢印と疑問符
		CURSOR_NO,			// 禁止カーソル
		CURSOR_SIZEALL,		// 4 方向矢印カーソル
		CURSOR_SIZENESW,	// 斜め左下がりの両方向矢印カーソル
		CURSOR_SIZENS,		// 上下両方向矢印カーソル
		CURSOR_SIZENWSE,	// 斜め右下がりの両方向矢印カーソル
		CURSOR_SIZEWE,		// 左右両方向矢印カーソル
		CURSOR_UPARROW,		// 垂直の矢印カーソル
		CURSOR_WAIT,		// 砂時計カーソル 
		CURSOR_EOT,
	};
	static const int CURSOR_OFFSET = 22;
	static const int CURSOR_INDEXES_NUM = 24;
	static const int CURSOR_INDEXES[CURSOR_INDEXES_NUM]; // 内部のカーソルインデックスと公開カーソルインデックスの変換テーブル
//	static std::vector<HCURSOR> CURSOR_HANDLES_FOR_INDEXES;	// 全カーソルのハンドル、新規読込みされたものは追加される

// 	static const LPTSTR CURSORS[CURSOR_EOT];	// カーソルとリソースIDの対応テーブル
// 	static HCURSOR CURSOR_HANDLES[CURSOR_EOT];	// デフォルトカーソルのハンドルテーブル
	static const int INVALID_CURSOR_INDEX = 0x7FFFFFFF;	// 無効なカーソルインデックス
	static bool CURSOR_INITIALIZED;	// カーソル初期化済みか否か

	static bool is_cursor_hide_;	// カーソルがcrNoneで非表示になっているかどうか

public:
	static void Initialize();
	static void Finalize();
	static void SetMouseCursor( int index );
// 	static int AddCursor( HCURSOR hCursor ) {
// 		int index = (int)CURSOR_HANDLES_FOR_INDEXES.size();
// 		CURSOR_HANDLES_FOR_INDEXES.push_back( hCursor );
// 		return index - CURSOR_OFFSET;
// 	}

private:
	int cursor_index_;

	int GetCurrentCursor();

public:
	MouseCursor() : cursor_index_(INVALID_CURSOR_INDEX) {}
	MouseCursor( int index ) : cursor_index_(index) {}

	void UpdateCursor();

//	void SetCursorIndex( int index, HWND hWnd );
};

#endif
