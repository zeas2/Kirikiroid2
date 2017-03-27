#pragma once
#include "ComplexRect.h"
#include <unordered_map>
#include <stdint.h>
#include <string>

#ifndef GL_ZERO
#define GL_ZERO 0
#define GL_ONE  1
#endif
#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_ONE_MINUS_DST_ALPHA            0x0305
#define GL_FUNC_ADD 0x8006
#endif
#ifndef GL_MAX
#define GL_MIN 0x8007
#define GL_MAX 0x8008
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#endif
#if 0
struct _BITMAPINFO;
//---------------------------------------------------------------------------
// tTVPBitmap : internal bitmap object
//---------------------------------------------------------------------------
class tTVPBitmap
{
	tjs_int RefCount;

	void * Bits; // pointer to bitmap bits
	_BITMAPINFO *BitmapInfo; // DIB information
	tjs_int BitmapInfoSize;

	tjs_int PitchBytes; // bytes required in a line
	tjs_int PitchStep; // step bytes to next(below) line
	tjs_int Width; // actual width
	tjs_int Height; // actual height

public:
	tTVPBitmap(tjs_uint width, tjs_uint height, tjs_uint bpp);

	tTVPBitmap(const tTVPBitmap & r);

	~tTVPBitmap();

	void Allocate(tjs_uint width, tjs_uint height, tjs_uint bpp);

	void AddRef(void)
	{
		RefCount++;
	}

	void Release(void)
	{
		if (RefCount == 1)
			delete this;
		else
			RefCount--;
	}

	tjs_uint GetWidth() const { return Width; }
	tjs_uint GetHeight() const { return Height; }

	tjs_uint GetBPP() const;
	bool Is32bit() const;
	bool Is8bit() const;


	void * GetScanLine(tjs_uint l) const;

	tjs_int GetPitch() const { return PitchStep; }

	bool IsIndependent() const { return RefCount == 1; }

	const void * GetBits() const { return Bits; }
};
#else
//#include "LayerBitmapImpl.h"
#endif

struct TVPTextureFormat {
	enum e {
		None = 0,
		Gray = 1,
		RGB = 3,
		RGBA = 4,
		// for opengl compressed texture, the argument pitch = block_width | (block_height << 16) or block_size
		Compressed = 0x10000,
		CompressedEnd = 0x20000,
	};
};

namespace cocos2d {
	class Texture2D;
};

class iTVPTexture2D
{
protected:
	int RefCount;
	tjs_int Width; // actual width
	tjs_int Height; // actual height
	//int Flags, TexWidth, TexHeight, ActualWidth, ActualHeight;
	iTVPTexture2D(tjs_int w, tjs_int h) : Width(w), Height(h), RefCount(1) {}
public:
	virtual ~iTVPTexture2D() {};
	void AddRef() { ++RefCount; }
	virtual void Release();
	tjs_uint GetWidth() const { return Width; }
	tjs_uint GetHeight() const { return Height; }
	virtual tjs_uint GetInternalWidth() const { return Width; }
	virtual tjs_uint GetInternalHeight() const { return Height; }
	virtual void SetSize(unsigned int w, unsigned int h) { // may lost image content
		Width = w; Height = h;
	}

	virtual TVPTextureFormat::e GetFormat() const = 0;
	virtual const void * GetScanLineForRead(tjs_uint l) { return nullptr; }
	virtual const void * GetPixelData() { return GetScanLineForRead(0); }
	virtual void * GetScanLineForWrite(tjs_uint l) { return (void*)GetScanLineForRead(l); }
	virtual tjs_int GetPitch() const { return 0x100000; }
	bool IsIndependent() const { return RefCount == 1; }

	//virtual tGLTexture* GetTexture() = 0;
	virtual void Update(const void *pixel, TVPTextureFormat::e format, int pitch, const tTVPRect& rc) = 0;
	virtual uint32_t GetPoint(int x, int y) = 0;
	virtual void SetPoint(int x, int y, uint32_t clr) = 0;
	virtual bool IsStatic() = 0; // aka. is readonly
	virtual bool IsOpaque() = 0;
	//virtual void RefreshBitmap() = 0;
	virtual cocos2d::Texture2D* GetAdapterTexture(cocos2d::Texture2D* origTex) = 0;
	virtual bool GetScale(float &x, float &y) { x = 1.f; y = 1.f; return true; }

	static void RecycleProcess();
};

class iTVPRenderMethod
{
protected:
	virtual ~iTVPRenderMethod() {} // undeletable
	std::string Name;
public:
	// the parameter id should not change in whole lifecycle, valid id >= 0
	virtual int EnumParameterID(const char *name) { return -1; };
	virtual void SetParameterUInt(int id, unsigned int Value) {};
	virtual void SetParameterInt(int id, int Value) {};
	virtual void SetParameterPtr(int id, const void *Value) {};
	virtual void SetParameterFloat(int id, float Value) {};
	virtual void SetParameterColor4B(int id, unsigned int clr) {};
	virtual void SetParameterOpa(int id, int Value) {};
	virtual void SetParameterFloatArray(int id, float *Value, int nElem) {};
	virtual iTVPRenderMethod* SetBlendFuncSeparate(int func, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha) { return this; }
	virtual bool IsBlendTarget() { return true; }
	void SetName(const std::string &name) { Name = name; }
	const std::string &GetName() { return Name; }
};

template <typename TElem>
class tRenderTextureArray {
	const std::pair<iTVPTexture2D*, TElem>* pElem;
	size_t nCount;

public:
	typedef std::pair<iTVPTexture2D*, TElem> Element;
	tRenderTextureArray() : pElem(nullptr), nCount(0) {}
	tRenderTextureArray(std::pair<iTVPTexture2D*, TElem> *p, size_t n) : pElem(p), nCount(n) {}

	template<typename T>
	tRenderTextureArray(const T& arr) {
		pElem = arr;
		nCount = sizeof(arr) / sizeof(arr[0]);
	}

	const std::pair<iTVPTexture2D*, TElem>& operator[](size_t i) const {
		return pElem[i];
	}

	size_t size() const { return nCount; }
};

typedef tRenderTextureArray<tTVPRect> tRenderTexRectArray;
typedef tRenderTextureArray<const tTVPPointD*> tRenderTexQuadArray;

class tTVPBitmap;
namespace TJS {
	class tTJSBinaryStream;
}
class iTVPRenderManager
{
protected:
	virtual ~iTVPRenderManager() {} // undeletable
	void RegisterRenderMethod(const char *name, iTVPRenderMethod* method);
	virtual iTVPRenderMethod* GetRenderMethodFromScript(const char *script, int nTex, unsigned int flags) { return nullptr; }
	std::unordered_map<uint32_t, iTVPRenderMethod*> AllMethods;

public:
	void Initialize();

public:
#define RENDER_CREATE_TEXTURE_FLAG_ANY 0
#define RENDER_CREATE_TEXTURE_FLAG_STATIC 1
#define RENDER_CREATE_TEXTURE_FLAG_NO_COMPRESS 2
	virtual iTVPTexture2D* CreateTexture2D(const void *pixel, int pitch, unsigned int w, unsigned int h,
		TVPTextureFormat::e format, int flags = RENDER_CREATE_TEXTURE_FLAG_ANY) = 0;
	virtual iTVPTexture2D* CreateTexture2D(tTVPBitmap* bmp) = 0; // for province image
	virtual iTVPTexture2D* CreateTexture2D(TJS::tTJSBinaryStream* s) = 0; // for compressed or special image format
	virtual iTVPTexture2D* CreateTexture2D( // create and copy content from exist texture
		unsigned int neww, unsigned int newh, iTVPTexture2D* tex) = 0;

	// each method is singleton in whole lifecycle
	virtual iTVPRenderMethod* GetRenderMethod(const char *name, uint32_t *hint = nullptr);
#define RENDER_METHOD_FLAG_NONE 0
#define RENDER_METHOD_FLAG_TARGET_AS_INPUT 1
	iTVPRenderMethod* CompileRenderMethod(const char *name, const char *glsl_script, int nTex, unsigned int flags = 0);
	iTVPRenderMethod* GetOrCompileRenderMethod(const char *name, uint32_t *hint, const char *glsl_script, int nTex, unsigned int flags = 0);

	virtual bool IsSoftware() { return false; }
	virtual const char *GetName() = 0;

	virtual bool GetRenderStat(unsigned int &drawCount, uint64_t &vmemsize) = 0;
	virtual bool GetTextureStat(iTVPTexture2D *texture, uint64_t &vmemsize) { return false; }

	virtual void BeginStencil(iTVPTexture2D* reftex) {}
	virtual void EndStencil() {}

	virtual void SetRenderTarget(iTVPTexture2D *target) {} // for manual rendering

	// interface to access custom parameter
	virtual int EnumParameterID(const char *name) { return -1; }
	virtual void SetParameterUInt(int id, unsigned int Value) {};
	virtual void SetParameterInt(int id, int Value) {};
	virtual void SetParameterPtr(int id, const void *Value) {};
	virtual void SetParameterFloat(int id, float Value) {};

	// -------------- operations ----------------
	// dst x Tex1 x ... x TexN -> dst
	// referenced target texture would be used if target texture is required as source
	virtual void OperateRect(iTVPRenderMethod* method, iTVPTexture2D *tar, iTVPTexture2D *reftar,
		const tTVPRect& rctar, const tRenderTexRectArray &textures) = 0;

	// src x dst -> tar
	virtual void OperateTriangles(iTVPRenderMethod* method, int nTriangles,
		iTVPTexture2D *target, iTVPTexture2D *reftar, const tTVPRect& rcclip, const tTVPPointD* pttar,
		const tRenderTexQuadArray &textures) = 0;

	// src -> tar
	virtual void OperatePerspective(iTVPRenderMethod* method, int nQuads, iTVPTexture2D *target,
		iTVPTexture2D *reftar, const tTVPRect& rcclip, const tTVPPointD* pttar/*quad{lt,rt,lb,rb}*/,
		const tRenderTexQuadArray &textures) = 0;

public:
	// utility function
	iTVPRenderMethod* GetRenderMethod(tjs_int opa, bool hda, int/*tTVPBBBltMethod*/ method);
	struct tRenderMethodCache* RenderMethodCache = nullptr;

};

void TVPRegisterRenderManager(const char* name, iTVPRenderManager*(*func)());

#define REGISTER_RENDERMANAGER(MGR, NAME) \
	static iTVPRenderManager* __ ## MGR ## Factory() { return new MGR(); } \
	static class __ ## MGR ## AutoReigster{ \
	public: __ ## MGR ## AutoReigster() { TVPRegisterRenderManager(#NAME, __ ## MGR ## Factory); } \
	} __ ## MGR ## AutoReigster_instance;

iTVPRenderManager *TVPGetRenderManager();
namespace TJS { class tTJSString; }
iTVPRenderManager *TVPGetRenderManager(const TJS::tTJSString &name);
bool TVPIsSoftwareRenderManager();