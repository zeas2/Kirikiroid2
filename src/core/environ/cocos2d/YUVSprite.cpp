#include "YUVSprite.h"

using namespace cocos2d;

// #define USE_VAO

static const char *_yuvRenderProgram_fsh =
	"#ifdef GL_ES"						"\n"
	"precision lowp float;"				"\n"
	"#endif"							"\n"

	"varying vec4 v_fragmentColor;"				"\n"
	"varying vec2 v_texCoord;"				"\n"

	"void main() {"												"\n"
	"	vec3 yuv;"												"\n"
	"	yuv.x = texture2D(CC_Texture0, v_texCoord).r - 0.0625;"			"\n"
	"	yuv.y = texture2D(CC_Texture1, v_texCoord).r - 0.5;"	"\n"
	"	yuv.z = texture2D(CC_Texture2, v_texCoord).r - 0.5;"	"\n"
	"	vec3 rgb = mat3(1.164, 1.164, 1.164,"							"\n"
	"		0.0, -0.392, 2.017,"							"\n"
	"		1.596, -0.813, 0.0) * yuv;"						"\n"
	"	gl_FragColor = vec4(rgb, 1.0) * v_fragmentColor;"		"\n"
	"}"
;

static cocos2d::GLProgram* _getYUVRenderProgram() {
	static cocos2d::GLProgram* _program = nullptr;
	if (!_program) {
		static cocos2d::EventListenerCustom *_backgroundListener = nullptr;
		if (!_backgroundListener) {
			_backgroundListener = cocos2d::EventListenerCustom::create(EVENT_RENDERER_RECREATED, [](cocos2d::EventCustom*)	{
				_program->reset();
				_program->initWithByteArrays(cocos2d::ccPositionTextureColor_vert, _yuvRenderProgram_fsh);
				_program->link();
				_program->updateUniforms();
			});
			cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(_backgroundListener, -1);
		}
		_program = cocos2d::GLProgram::createWithByteArrays(cocos2d::ccPositionTextureColor_vert, _yuvRenderProgram_fsh);
	}
	return _program;
}

void TVPYUVSprite::setupVBOAndVAO()
{
#ifdef USE_VAO
	//generate vbo and vao for trianglesCommand
	glGenVertexArrays(1, &_buffersVAO);
	cocos2d::GL::bindVAO(_buffersVAO);

	glGenBuffers(2, &_buffersVBO[0]);

	glBindBuffer(GL_ARRAY_BUFFER, _buffersVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(_polyInfo.triangles.verts[0]) * _polyInfo.triangles.vertCount, _polyInfo.triangles.verts, GL_DYNAMIC_DRAW);

	// vertices
	GL::enableVertexAttribs(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(_polyInfo.triangles.verts[0]), (GLvoid*)offsetof(cocos2d::V3F_C4B_T2F, vertices));

	// colors
	GL::enableVertexAttribs(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(_polyInfo.triangles.verts[0]), (GLvoid*)offsetof(cocos2d::V3F_C4B_T2F, colors));

	// tex coords
	GL::enableVertexAttribs(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(_polyInfo.triangles.verts[0]), (GLvoid*)offsetof(cocos2d::V3F_C4B_T2F, texCoords));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffersVBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(_polyInfo.triangles.indices[0]) * _polyInfo.triangles.indexCount, _polyInfo.triangles.indices, GL_DYNAMIC_DRAW);

	// Must unbind the VAO before changing the element buffer.
	GL::bindVAO(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CHECK_GL_ERROR_DEBUG();
#endif
}

void TVPYUVSprite::updateTextureDataInternal(cocos2d::Texture2D *pTex, const void* data, int width, int height, cocos2d::Texture2D::PixelFormat pixfmt)
{
	const cocos2d::Size &size = pTex->getContentSize();
	cocos2d::Size videoSize(width, height);
	if (size.width != videoSize.width || size.height != videoSize.height || pTex->getPixelFormat() != pixfmt) {
		if (size.width < videoSize.width || size.height < videoSize.height || pTex->getPixelFormat() != pixfmt) {
			ssize_t datasize = width * height;
			switch (pixfmt) {
			case cocos2d::Texture2D::PixelFormat::BGRA8888:
			case cocos2d::Texture2D::PixelFormat::RGBA8888:
				datasize *= 4; break;
			case cocos2d::Texture2D::PixelFormat::RGB888:
				datasize *= 3; break;
			case cocos2d::Texture2D::PixelFormat::RGB565:
			case cocos2d::Texture2D::PixelFormat::AI88:
			case cocos2d::Texture2D::PixelFormat::RGBA4444:
				datasize *= 2; break;
			default:
				break;
			}
			pTex->initWithData(data, datasize, pixfmt,
				width, height, cocos2d::Size::ZERO);
			pTex = nullptr;
		}
}
	if (pTex) {
		pTex->updateWithData(data, 0, 0, width, height);
	}
}

TVPYUVSprite::~TVPYUVSprite()
{
	CC_SAFE_RELEASE(_textureU);
	CC_SAFE_RELEASE(_textureV);
}

TVPYUVSprite* TVPYUVSprite::create()
{
	TVPYUVSprite* sprite = new TVPYUVSprite;
	sprite->init();
	sprite->autorelease();
	return sprite;
}

bool TVPYUVSprite::init()
{
	initWithTexture(new cocos2d::Texture2D);
	setupVBOAndVAO();
	_drawCommand.func = std::bind(&TVPYUVSprite::onDraw, this);
	_textureU = new cocos2d::Texture2D; _textureU->retain();
	_textureV = new cocos2d::Texture2D; _textureV->retain();
	setGLProgram(_getYUVRenderProgram());
	return true;
}

void TVPYUVSprite::updateTextureData(const void* data, int width, int height)
{
	cocos2d::Texture2D *pTex = getTexture();
	const cocos2d::Size &size = pTex->getContentSize();
	updateTextureDataInternal(getTexture(), data, width, height, cocos2d::Texture2D::PixelFormat::RGBA8888);
	if (size.width != width || size.height != height) {
		setTextureRect(cocos2d::Rect(0, 0, width, height));
	}
}

void TVPYUVSprite::updateTextureData(const void* Y, int YW, int YH, const void* U, int UW, int UH, const void* V, int VW, int VH)
{
	cocos2d::Texture2D *pTex = getTexture();
	const cocos2d::Size &size = pTex->getContentSize();
	updateTextureDataInternal(getTexture(), Y, YW, YH, cocos2d::Texture2D::PixelFormat::I8);
	updateTextureDataInternal(_textureU, U, UW, UH, cocos2d::Texture2D::PixelFormat::I8);
	updateTextureDataInternal(_textureV, V, VW, VH, cocos2d::Texture2D::PixelFormat::I8);
	if (size.width != YW || size.height != YH) {
		setTextureRect(cocos2d::Rect(0, 0, YW, YH));
	}
}

void TVPYUVSprite::onDraw()
{
	CCAssert(_polyInfo.triangles.vertCount == 4, "");
#ifdef USE_VAO
	GL::bindVAO(_buffersVAO);
	glBindBuffer(GL_ARRAY_BUFFER, _buffersVBO[0]);
	//	glBufferData(GL_ARRAY_BUFFER, sizeof(_polyInfo.triangles.verts[0]) * _polyInfo.triangles.vertCount, nullptr, GL_DYNAMIC_DRAW);
	void *buf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(buf, _polyInfo.triangles.verts, sizeof(_polyInfo.triangles.verts[0]) * 4);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffersVBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(_polyInfo.triangles.indices[0]) * _polyInfo.triangles.indexCount, _polyInfo.triangles.indices, GL_DYNAMIC_DRAW);

	cocos2d::GL::bindTexture2DN(0, _texture->getName());
	cocos2d::GL::bindTexture2DN(1, _textureU->getName());
	cocos2d::GL::bindTexture2DN(2, _textureV->getName());
	cocos2d::GL::blendFunc(_blendFunc.src, _blendFunc.dst);
	_glProgramState->apply(_mv);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	GL::bindVAO(0);
	CHECK_GL_ERROR_DEBUG();
	return;
#endif

	cocos2d::GL::bindTexture2DN(0, _texture->getName());
	cocos2d::GL::bindTexture2DN(1, _textureU->getName());
	cocos2d::GL::bindTexture2DN(2, _textureV->getName());
	cocos2d::GL::blendFunc(_blendFunc.src, _blendFunc.dst);
	_glProgramState->apply(_mv);
	cocos2d::GL::enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POS_COLOR_TEX);
	cocos2d::Vec3 vertices[4] = {
		_polyInfo.triangles.verts[0].vertices,
		_polyInfo.triangles.verts[1].vertices,
		_polyInfo.triangles.verts[2].vertices,
		_polyInfo.triangles.verts[3].vertices
	};
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	cocos2d::Color4B colors[4] = {
		_polyInfo.triangles.verts[0].colors,
		_polyInfo.triangles.verts[1].colors,
		_polyInfo.triangles.verts[2].colors,
		_polyInfo.triangles.verts[3].colors
	};
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, colors);
	cocos2d::Tex2F texCoords[4] = {
		_polyInfo.triangles.verts[0].texCoords,
		_polyInfo.triangles.verts[1].texCoords,
		_polyInfo.triangles.verts[2].texCoords,
		_polyInfo.triangles.verts[3].texCoords
	};
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void TVPYUVSprite::draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags)
{
	_mv = transform;
	_drawCommand.init(_globalZOrder);
	renderer->addCommand(&_drawCommand);
}
