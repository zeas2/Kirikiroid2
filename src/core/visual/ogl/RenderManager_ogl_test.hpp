#define TEST_SHADER_COLOR 0xAA981255
#define TEST_SHADER_OPA 100
#define TEST_SHADER(_method, ...) _TEST_SHADER(#_method, this, \
	[&](std::vector<tRenderTexRectArray::Element> &src_tex, tTVPOGLRenderMethod *method){__VA_ARGS__; }, false);
#define TEST_SHADER_IGNORE_ALPHA(_method, ...) _TEST_SHADER(#_method, this, \
	[&](std::vector<tRenderTexRectArray::Element> &src_tex, tTVPOGLRenderMethod *method){__VA_ARGS__; }, true);

static tjs_uint32 *testbuff = new tjs_uint32[256 * 256 * 4 + 2];
static tjs_uint32 *testdata1 = testbuff + 0;
static tjs_uint32 *testdata2 = testdata1 + 256 * 256;
static tjs_uint32 *testdest = testdata2 + 256 * 256;
static tjs_uint8 *testrule = new tjs_uint8[256 * 256];
static tjs_uint32 *testtable = new tjs_uint32[256];
static tjs_uint32 testcolor = 0xFFFFFFFF;
static tTVPOGLTexture2D_mutatble *texdata[3];
static tTVPOGLTexture2D_mutatble *texdest = NULL;

static tjs_uint32 *testred = new tjs_uint32[256 * 256];
static tjs_uint32 *testblue = new tjs_uint32[256 * 256];
static tTVPOGLTexture2D_mutatble *texred = nullptr;
static tTVPOGLTexture2D_mutatble *texblue = nullptr;
static void ResetTestData() {
	if (!texdest) {
		texdest = new tTVPOGLTexture2D_mutatble(nullptr, 0, 256, 256, TVPTextureFormat::RGBA, 1, 1);
		for (int x = 0; x < 256; ++x) {
			for (int y = 0; y < 256; ++y) {
				struct clr {
					unsigned char a;
					unsigned char r;
					unsigned char g;
					unsigned char b;
				}   *clr1 = (clr*)(testdata1 + 256 * y + x),
					*clr2 = (clr*)(testdata2 + 256 * y + x);
				clr1->a = 255 - x; clr2->a = 255 - y;
				clr1->r = x; clr2->r = y;
				clr1->g = y; clr2->g = 255 - x;
				clr1->b = 255 - y; clr2->b = x;
				testrule[256 * y + x] = (x + y) / 2;
				testred[256 * y + x] = 0x0000FF | (x << 24);
				testblue[256 * y + x] = 0xFF0000 | (y << 24);
			}
		}
		texdata[2] = new tTVPOGLTexture2D_mutatble(testrule, 256, 256, 256, TVPTextureFormat::Gray, 1, 1);
		texdata[0] = new tTVPOGLTexture2D_mutatble(testdata1, 256 * 4, 256, 256, TVPTextureFormat::RGBA, 1, 1);
		texdata[1] = new tTVPOGLTexture2D_mutatble(testdata2, 256 * 4, 256, 256, TVPTextureFormat::RGBA, 1, 1);
		texred = new tTVPOGLTexture2D_mutatble(testred, 256 * 4, 256, 256, TVPTextureFormat::RGBA, 1, 1);
		texblue = new tTVPOGLTexture2D_mutatble(testblue, 256 * 4, 256, 256, TVPTextureFormat::RGBA, 1, 1);
	}
	memcpy(testdest, testdata2, 256 * 256 * 4);
	texdest->Update(testdata2, TVPTextureFormat::RGBA, 256 * 4, tTVPRect(0, 0, 256, 256));
}

static void CheckTestData(const char * funcname) {
	const unsigned char *linetex = (const unsigned char *)texdest->GetScanLineForRead(0);
	unsigned char *linecmp = (unsigned char *)testdest;
	for (int i = 0; i < 256 * 256; ++i) {
		int a1 = linetex[i * 4 + 3], a2 = linecmp[i * 4 + 3];
		if (std::abs(a1 - a2) > 3 || (a1 > 3 && (
			((std::abs(linetex[i * 4] - linecmp[i * 4]) * a1) >> 8) > 3 ||
			((std::abs(linetex[i * 4 + 1] - linecmp[i * 4 + 1]) * a1) >> 8) > 3 ||
			((std::abs(linetex[i * 4 + 2] - linecmp[i * 4 + 2]) * a1) >> 8) > 3))
			) {
			cocos2d::log("%s check fail", funcname);
			for (int j = 0; j < 256 * 256 * 4; ++j) {
				linecmp[j] = std::abs(linetex[j] - linecmp[j]);
			}
			assert(false);
			return;
		}
	}
	cocos2d::log("%s pass", funcname);
}

static void _TEST_IGNORE_ALPHA() {
	const unsigned char *linetex = (const unsigned char *)texdest->GetScanLineForRead(0);
	unsigned char *linecmp = (unsigned char *)testdest;
	for (int i = 0; i < 256 * 256; ++i) {
		linecmp[i * 4 + 3] = linetex[i * 4 + 3];
	}
}

static void _TEST_SHADER(const char *_method, iTVPRenderManager* _this,
	const std::function<void(std::vector<tRenderTexRectArray::Element> &, tTVPOGLRenderMethod*)> &fcall,
	bool ignoreAlpha) {
	tTVPOGLRenderMethod_Script *method = (tTVPOGLRenderMethod_Script *)_this->GetRenderMethod(_method);
	int id_opa = method->EnumParameterID("opacity"), id_clr = method->EnumParameterID("color");
	if (id_opa >= 0) method->SetParameterOpa(id_opa, TEST_SHADER_OPA);
	if (id_clr >= 0) method->SetParameterColor4B(id_clr, TEST_SHADER_COLOR);
	ResetTestData();
	tTVPRect rc(0, 0, 256, 256);
	std::vector<tRenderTexRectArray::Element> src_tex;
	for (int i = 0; i < method->GetValidTex(); ++i)
		src_tex.emplace_back(texdata[i], rc);
	fcall(src_tex, method);
	_this->OperateRect(method, texdest, nullptr, rc, src_tex.empty() ? tRenderTexRectArray() : tRenderTexRectArray(&src_tex[0], src_tex.size()));
	if (ignoreAlpha) _TEST_IGNORE_ALPHA();
	CheckTestData(_method);
}