#pragma once
#include "ComplexRect.h"
#include <string>

struct SDL_Renderer;
struct SDL_Window;
struct tTVPRect;

struct texRect
{
    int x, y, w, h;
    texRect() : x(0), y(0), w(0), h(0) {}
    texRect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}
    texRect(const tTVPRect& rc);
};

extern unsigned int TVPMaxTextureSize;

void TVPGLInitPrimaryRender(bool needShader);

SDL_Renderer *TVPGLCreateRender(bool needShader = false);

enum eTextureProgram
{
    texProgramNone = 0,

    // operation with no texture
    //texFillARGB,

    // operation from one texture
    //texFillMask,
    //texFillColor,
    //texCopy,
    //texCopyOpaqueImage,
    texDoGrayScale,

    texAdditiveAlphaToAlpha,
    //texAlphaToAdditiveAlpha,

    //texRemoveConstOpacity,
    //texConstColorAlphaBlend,
    //texConstColorAlphaBlend_a,
    texConstColorAlphaBlend_d,
    texAdjustGamma,
    texAdjustGamma_a,

    // operation with two texture
    //texCopyColor,
    //texCopyMask,
    texConstAlpha,
    //texConstAlpha_a,
    texConstAlpha_d,
    texConstAlpha_SD_d,
    texConstAlpha_SD_a,
    //texAlphaBlend,
    //texAlphaBlend_a,
    //texAlphaBlend_d,
    //texAddBlend,
    //texSubBlend,
    //texMulBlend,
    //texAddAlpha,
    //texAddAlpha_a,
    //texApplyColorMap,
    //texApplyColorMap_d,
    //texApplyColorMap_a,
    //texRemoveOpacity,
    //texColorDodgeBlend,
    texDarkenBlend,
    texLightenBlend,
    //texScreenBlend,
    // ps series
    //texPsAlphaBlend = texAlphaBlend,
    //texPsAddBlend,
    //texPsSubBlend,
    //texPsMulBlend,
    //texPsScreenBlend,
    //texPsOverlayBlend,
    texPsHardLightBlend,
    texPsSoftLightBlend,
    texPsColorDodgeBlend,
    texPsColorDodge5Blend,
    texPsColorBurnBlend,
    texPsLightenBlend,
    texPsDarkenBlend,
    texPsDiffBlend,
    texPsDiff5Blend,
    texPsExclusionBlend,

    // operation with rule
    texUnivTransBlend,
    texUnivTransBlend_d,
    texUnivTransBlend_a,

    texShaderUser,
    texShaderCount
};

enum eVertShader {
    eVertShaderNone,
	eVertShader0Tex,
	eVertShader1Tex,
    eVertShader2Tex,
    eVertShader3Tex,

    eVertShaderCount
};
extern int TVPVertShaders[eVertShaderCount];

enum eTextureFormat {
    texNone = 0,
    texGray = 1,
    texRGB  = 3,
    texRGBA = 4,
};

class iTVPTexture
{
public:
    int RefCount;
    int Flags, TexWidth, TexHeight, ActualWidth, ActualHeight;
    iTVPTexture(unsigned int w, unsigned int h, unsigned int aw, unsigned int ah);
    void AddRef();
    virtual void Release() = 0;
    //virtual tGLTexture* GetTexture() = 0;
    virtual void Update(const void *pixel, eTextureFormat format, unsigned int pitch, int x, int y, int w, int h) = 0;
    virtual unsigned long GetPoint(int x, int y) = 0;
    virtual bool SetPoint(int x, int y, unsigned long clr) = 0;
	virtual bool IsStatic() = 0;
    //virtual void RefreshBitmap() = 0;
};

typedef int tGLShader;

struct GLFunction
{
private:
    GLFunction();
public:
    static void Finish();

    static void UseProgram(tGLShader shader);
    static int GetUniformLocation(tGLShader prog, const char *name);
    static int GetAttribLocation(tGLShader prog, const char *name);

    static void Uniform1i(int location, int i);
    static void Uniform2i(int location, int i1, int i2);
    static void Uniform1f(int location, float f1);
    static void Uniform2f(int location, float f1, float f2);
    static void Uniform4f(int location, float f1, float f2, float f3, float f4);
};

class iTVPShader
{
public:
    iTVPShader() {}
    static bool CompileShader(tGLShader shader, const std::string &src);
    static tGLShader CreateVertProgram(const std::string &vert_source);
    static tGLShader CreateFragProgram(const std::string &frag_source);
    static tGLShader CombineProgram(tGLShader vert_shader, tGLShader frag_shader);

public:
    virtual void apply() const {}
    virtual int getPosAttr() const { return -1; }
    virtual int getTexCoordAttr(int idx) const {return -1;}
};

// with opacity and color
class tTVPStandardShader : public iTVPShader
{
protected:
    float opacity;
	unsigned long Color;
    tGLShader program;
	int opa_uniform_location;
	int clr_uniform_location;
    int pos_attr_location;
    int tex0_coord_attr_location;
    int tex1_coord_attr_location;
    int tex2_coord_attr_location;
    int BlendFunc, BlendSrcRGB, BlendDstRGB, BlendSrcA, BlendDstA;
    bool IsEnableBlend, IsEnableBlendOpa;

    void initDefaultData();

public:
    tTVPStandardShader(tGLShader prog);
    tTVPStandardShader(tGLShader vert_prog, tGLShader frag_prog);
    tTVPStandardShader(tGLShader vert_prog, const std::string &frag_source);
    virtual void apply() const;
    virtual int getPosAttr() const;
    virtual int getTexCoordAttr(int idx) const;

    void init(int opa /*0~255*/, unsigned long clr);
    void setBlendFuncSeparate(int func, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);
    void enableBlendColorAsOpa(bool bEnableBlendOpa = false);
    bool isBlendEnabled();

public:
    // preset shaders
    static tTVPStandardShader* Copy;
    static tTVPStandardShader* CopyOpaqueImage;
	static tTVPStandardShader* FillARGB;
	static tTVPStandardShader* FillMask;
	static tTVPStandardShader* FillColor;
	static tTVPStandardShader* RemoveConstOpacity;
	static tTVPStandardShader* AdditiveAlphaToAlpha;
	static tTVPStandardShader* AlphaToAdditiveAlpha;

    static tTVPStandardShader* CopyColor;
    static tTVPStandardShader* CopyMask;
    //static tTVPStandardShader* ConstAlpha_SD_a;
    //static tTVPStandardShader* ConstAlpha_SD_d;
    static tTVPStandardShader* ConstAlpha;
    static tTVPStandardShader* ConstAlpha_a;
    static tTVPStandardShader* AlphaBlend;
    static tTVPStandardShader* AlphaBlend_a;
	static tTVPStandardShader* AlphaBlend_d;
	static tTVPStandardShader* AddBlend;
	static tTVPStandardShader* SubBlend;
	static tTVPStandardShader* MulBlend;
	static tTVPStandardShader* AddAlpha;
	static tTVPStandardShader* AddAlpha_a;
	static tTVPStandardShader* ApplyColorMap;
    static tTVPStandardShader* ApplyColorMap_a;
	static tTVPStandardShader* ApplyColorMap_d;
	static tTVPStandardShader* RemoveOpacity;

	static tTVPStandardShader* ColorDodgeBlend;
	static tTVPStandardShader* ScreenBlend;

	static tTVPStandardShader* PsAlphaBlend;
	static tTVPStandardShader* PsAddBlend;
	static tTVPStandardShader* PsSubBlend;
	static tTVPStandardShader* PsMulBlend;
    static tTVPStandardShader* PsScreenBlend;
    static tTVPStandardShader* PsOverlayBlend;
    static tTVPStandardShader* ConstColorAlphaBlend;
    static tTVPStandardShader* ConstColorAlphaBlend_a;
};

//void TVPTextureToBitmap(iTVPTexture2D* src, void *dst, unsigned int w, unsigned int h);

struct tTVPGLGammaAdjustData;
void TVPTextureAdjustGamma(iTVPTexture* src, eTextureProgram method,
    const texRect& rc, const tTVPGLGammaAdjustData& data);

void TVPClearScreen();
void TVPClearRect(const texRect& rc);

//---------------------------------------------------------------

