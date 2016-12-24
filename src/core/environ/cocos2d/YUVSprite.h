#pragma once
#include "cocos2d.h"

class TVPYUVSprite : public cocos2d::Sprite {
	cocos2d::Texture2D* _textureU = nullptr, *_textureV = nullptr;
	cocos2d::Texture2D::PixelFormat _texfmtY = cocos2d::Texture2D::PixelFormat::NONE,
		_texfmtU = cocos2d::Texture2D::PixelFormat::NONE,
		_texfmtV = cocos2d::Texture2D::PixelFormat::NONE;
	cocos2d::CustomCommand _drawCommand;

	GLuint _buffersVAO;
	GLuint _buffersVBO[2]; //0: vertex  1: indices

	void setupVBOAndVAO();

	void updateTextureDataInternal(cocos2d::Texture2D *pTex, const void* data, int width, int height,
		cocos2d::Texture2D::PixelFormat pixfmt);

public:
	virtual ~TVPYUVSprite();

	static TVPYUVSprite* create();

	bool init();

	void updateTextureData(const void* data, int width, int height);

	void updateTextureData(
		const void* Y, int YW, int YH,
		const void* U, int UW, int UH,
		const void* V, int VW, int VH);

private:
	cocos2d::Mat4 _mv;

	void onDraw();

	void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags) override;
};
