#include <stdio.h>
#include <list>
#include <map>

using namespace std;

#define NCB_MODULE_NAME TJS_W("perspective.dll")

static const char *copyright = 
"----- AntiGrainGeometry Copyright START -----\n"
"Anti-Grain Geometry - Version 2.4\n"
"Copyright (C) 2002-2005 Maxim Shemanarev (McSeem)\n"
"\n"
"Permission to copy, use, modify, sell and distribute this software\n"
"is granted provided this copyright notice appears in all copies. \n"
"This software is provided \"as is\" without express or implied\n"
"warranty, and with no claim as to its suitability for any purpose.\n"
"----- AntiGrainGeometry Copyright END -----\n";

#include "ncbind/ncbind.hpp"

#include "LayerExBase.h"
#include "LayerImpl.h"
#include "tjsNative.h"
#include "RenderManager.h"
#if 0
/**
 * 透視変換コピー
 * @param src
 * @param sleft
 * @param stop
 * @param swidth
 * @param sheight
 * @param x1 左上隅
 * @param y1
 * @param x2 右上隅
 * @param y2
 * @param x3 左下隅
 * @param y3
 * @param x4 右下隅
 * @param y4
 */
class tPerspectiveCopy : public tTJSDispatch
{
protected:
public:
	tjs_error TJS_INTF_METHOD FuncCall(
		tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis) {

		if (numparams < 13) return TJS_E_BADPARAMCOUNT;

		NI_LayerExBase *dest;
		if ((dest = NI_LayerExBase::getNative(objthis)) == NULL) return TJS_E_NATIVECLASSCRASH;
		dest->reset(objthis);
		
		NI_LayerExBase *src;
		if ((src = NI_LayerExBase::getNative(param[0]->AsObjectNoAddRef())) == NULL) return TJS_E_NATIVECLASSCRASH;
		src->reset(param[0]->AsObjectNoAddRef());
		
		double g_x1 = param[1]->AsReal();
		double g_y1 = param[2]->AsReal();
		double g_x2 = g_x1 + param[3]->AsReal();
		double g_y2 = g_y1 + param[4]->AsReal();

		double quad[8];
        quad[0] = param[5]->AsReal(); // 左上
        quad[1] = param[6]->AsReal();
        quad[2] = param[7]->AsReal(); // 右上
        quad[3] = param[8]->AsReal();
		quad[4] = param[11]->AsReal(); // 右下
        quad[5] = param[12]->AsReal(); 
        quad[6] = param[9]->AsReal(); // 左下
        quad[7] = param[10]->AsReal();


		{
			/// ソースの準備
			unsigned char *buffer = src->_buffer;
			// AGG 用に先頭位置に補正
			if (src->_pitch < 0) {
				buffer += int(src->_height - 1) * src->_pitch;
			}

			typedef agg::image_accessor_clip<pixfmt> source_type;
			agg::rendering_buffer rbufs(buffer, src->_width, src->_height, src->_pitch);
            pixfmt pixbuf(rbufs);
			source_type rbuf_src(pixbuf, agg::rgba_pre(0, 0, 0, 0)/*buffer, src->_width, src->_height, src->_pitch*/);

			/// レンダリング用バッファ
			buffer = dest->_buffer;
			// AGG 用に先頭位置に補正
			if (dest->_pitch < 0) {
				buffer += int(dest->_height - 1) * dest->_pitch;
			}
			agg::rendering_buffer rbuf(buffer, dest->_width, dest->_height, dest->_pitch);

			// レンダラの準備
			pixfmt_pre  pixf_pre(rbuf);
			renderer_base_pre rb_pre(pixf_pre);
			
			g_rasterizer.clip_box(0, 0, dest->_width, dest->_height);
			g_rasterizer.reset();
			g_rasterizer.move_to_d(quad[0], quad[1]);
			g_rasterizer.line_to_d(quad[2], quad[3]);
			g_rasterizer.line_to_d(quad[4], quad[5]);
			g_rasterizer.line_to_d(quad[6], quad[7]);
			
			// 変形コピー
			agg::trans_perspective tr(quad, g_x1, g_y1, g_x2, g_y2);
			//if(tr.is_valid())
			{
				typedef agg::span_interpolator_linear<agg::trans_perspective> interpolator_type;
				typedef agg::span_subdiv_adaptor<interpolator_type> subdiv_adaptor_type;
				interpolator_type interpolator(tr);
				subdiv_adaptor_type subdiv_adaptor(interpolator);

				typedef agg::span_image_filter_rgba_2x2</*color_type*/source_type,
				//agg::order_bgra,
				subdiv_adaptor_type> span_gen_type;
				typedef agg::span_allocator<agg::rgba8> span_alloc_type;
				typedef agg::renderer_scanline_aa<renderer_base_pre, span_alloc_type, span_gen_type> renderer_type;

				span_alloc_type sa;
				agg::image_filter_hermite filter_kernel;
				agg::image_filter_lut filter(filter_kernel, false);
				
				span_gen_type sg(//sa, 
								 rbuf_src, 
								 //agg::rgba_pre(0, 0, 0, 0),
								 subdiv_adaptor,
								 filter);
				
				renderer_type ri(rb_pre, sa, sg);
				agg::render_scanlines(g_rasterizer, g_scanline, ri);
			}

		}

		dest->redraw(objthis);
		return TJS_S_OK;
	}
};
#endif
static int TJS_NATIVE_CLASSID_NAME = -1;

static tjs_error PerspectiveCopy_GL( tTJSVariant *result, 
    tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis)
{
    if(!objthis) return TJS_E_NATIVECLASSCRASH;
    tTJSNI_Layer *_this;
    { 
        tjs_error hr; 
        hr = objthis->NativeInstanceSupport(TJS_NIS_GETINSTANCE, 
            tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&_this); 
        if(TJS_FAILED(hr)) return TJS_E_NATIVECLASSCRASH; 
    }

    if (numparams < 13) return TJS_E_BADPARAMCOUNT;

    tTJSNI_BaseLayer * src = NULL;
    tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
    if(clo.Object)
    {
        if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
            tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
            TVPThrowExceptionMessage(TVPSpecifyLayer);
    }

    double g_x1 = param[1]->AsReal();
    double g_y1 = param[2]->AsReal();
    double g_x2 = g_x1 + param[3]->AsReal();
    double g_y2 = g_y1 + param[4]->AsReal();

    tTVPPointD srcpt[4] = {
        { g_x1, g_y1 },
        { g_x2, g_y1 },
		{ g_x1, g_y2 },
        { g_x2, g_y2 },
    };
    tTVPPointD dstpt[4] = {
        { param[5 ]->AsReal(), param[6 ]->AsReal() }, // 左上
        { param[7 ]->AsReal(), param[8 ]->AsReal() }, // 右上
		{ param[9]->AsReal(), param[10]->AsReal() }, // 左下
        { param[11]->AsReal(), param[12]->AsReal() }, // 右下
    };
	static iTVPRenderMethod *method = TVPGetRenderManager()->GetRenderMethod("PerspectiveAlphaBlend_a");
	static int id_opa = method->EnumParameterID("opacity");
	method->SetParameterOpa(id_opa, 255);
	iTVPTexture2D *tex = src->GetMainImage()->GetTexture();
	tRenderTexQuadArray::Element src_tex[] = {
		tRenderTexQuadArray::Element(tex, srcpt)
	};
	int w = _this->GetMainImage()->GetWidth(), h = _this->GetMainImage()->GetHeight();
	tTVPRect rctar(w, h, 0, 0);
	for (int i = 0; i < 4; ++i) {
		if (rctar.left > dstpt[i].x) rctar.left = dstpt[i].x;
		if (rctar.top > dstpt[i].y) rctar.top = dstpt[i].y;
		if (rctar.right < dstpt[i].x) rctar.right = dstpt[i].x;
		if (rctar.bottom < dstpt[i].y) rctar.bottom = dstpt[i].y;
	}
	if (rctar.left < 0) rctar.left = 0;
	if (rctar.top < 0) rctar.top = 0;
	if (rctar.right > w) rctar.right = w;
	if (rctar.bottom > h) rctar.bottom = h;
	TVPGetRenderManager()->OperatePerspective(method, 1,
		_this->GetMainImage()->GetTextureForRender(method->IsBlendTarget(), &rctar), nullptr, rctar, dstpt,
		tRenderTexQuadArray(src_tex));
	_this->Update();
    return TJS_S_OK;
}

static void
addMethod(iTJSDispatch2 *dispatch, const tjs_char *methodName, tTJSDispatch *method)
{
	tTJSVariant var = tTJSVariant(method);
	method->Release();
	dispatch->PropSet(
		TJS_MEMBERENSURE, // メンバがなかった場合には作成するようにするフラグ
		methodName, // メンバ名 ( かならず TJS_W( ) で囲む )
		NULL, // ヒント ( 本来はメンバ名のハッシュ値だが、NULL でもよい )
		&var, // 登録する値
		dispatch // コンテキスト
		);
}

static void
delMethod(iTJSDispatch2 *dispatch, const tjs_char *methodName)
{
	dispatch->DeleteMember(
		0, // フラグ ( 0 でよい )
		methodName, // メンバ名
		NULL, // ヒント
		dispatch // コンテキスト
		);
}

//---------------------------------------------------------------------------
void InitPlugin_Perspective()
{
// 	if (TVPGetRenderManager()->IsSoftware()) {
// 	    TVPAddImportantLog(ttstr(copyright));
// 	
// 	    // クラスオブジェクトチェック
// 	    if ((NI_LayerExBase::classId = TJSFindNativeClassID(TJS_W("LayerExBase"))) <= 0) {
// 		    NI_LayerExBase::classId = TJSRegisterNativeClass(TJS_W("LayerExBase"));
// 	    }
// 	
// 	    {
// 		    // TJS のグローバルオブジェクトを取得する
// 		    iTJSDispatch2 * global = TVPGetScriptDispatch();
// 
// 		    // Layer クラスオブジェクトを取得
// 		    tTJSVariant varScripts;
// 		    TVPExecuteExpression(TJS_W("Layer"), &varScripts);
// 		    iTJSDispatch2 *dispatch = varScripts.AsObjectNoAddRef();
// 		    if (dispatch) {
// 			    // プロパティ初期化
// 			    NI_LayerExBase::init(dispatch);
// 
// 			    // 専用メソッドの追加
// 			    addMethod(dispatch, TJS_W("perspectiveCopy"), new tPerspectiveCopy());
// 		    }
// 
// 		    global->Release();
// 	    }
//     } else {
        iTJSDispatch2 * global = TVPGetScriptDispatch();
        tTJSVariant varScripts;
        global->PropGet(0, TJS_W("Layer"), nullptr, &varScripts, global);
        iTJSDispatch2 *dispatch = varScripts.AsObjectNoAddRef();
        if (dispatch) {
            addMethod(dispatch, TJS_W("perspectiveCopy"), TJSCreateNativeClassMethod(PerspectiveCopy_GL));
        }
    //}
}

NCB_PRE_REGIST_CALLBACK(InitPlugin_Perspective);
