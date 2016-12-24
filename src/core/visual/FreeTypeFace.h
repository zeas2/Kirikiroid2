//---------------------------------------------------------------------------
/*
	Risa [りさ]      alias 吉里吉里3 [kirikiri-3]
	 stands for "Risa Is a Stagecraft Architecture"
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//! @file
//! @brief FreeType の Face 基底クラスの定義
//---------------------------------------------------------------------------

#ifndef FREETYPEFACE_H
#define FREETYPEFACE_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4819)
#endif
#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <vector>
#include <string>

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/**
 * FreeType Face の基底クラス
 */
class tBaseFreeTypeFace
{
public:
	virtual FT_Face GetFTFace() const = 0; //!< FreeType の Face オブジェクトを返す
	virtual void GetFaceNameList(std::vector<ttstr> & dest) const = 0; //!< このフォントファイルが持っているフォントを配列として返す
	virtual ~tBaseFreeTypeFace() {;}
	virtual tjs_char GetDefaultChar() const = 0; //!< 描画できない時に描画する文字コードを返す
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------



#endif

