#pragma once
#include "RenderManager.h"

class tTVPRenderMethod_Software : public iTVPRenderMethod {
public:
	virtual void DoRender(
		iTVPTexture2D *_tar, const tTVPRect &rctar,
		iTVPTexture2D *_dst, const tTVPRect &rcdst,
		iTVPTexture2D *_src, const tTVPRect &rcsrc,
		iTVPTexture2D *rule, const tTVPRect &rcrule) = 0;
};