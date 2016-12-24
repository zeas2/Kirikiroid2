#pragma once

#include "2d/CCNode.h"
#include "2d/CCLabel.h"
#include <deque>
#include <vector>
#include "tjsCommHead.h"

class TVPConsoleWindow : public cocos2d::Node {
	TVPConsoleWindow();
public:
	static TVPConsoleWindow* create(int fontSize, cocos2d::Node *parent);

	void addLine(const ttstr &line, cocos2d::Color3B clr);

	void setFontSize(float size);

	virtual void visit(cocos2d::Renderer *renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags) override;
private:
	float _fontSize;

	std::deque<cocos2d::Label*> _dispLabels;
	std::vector<cocos2d::Label*> _unusedLabels;
	std::deque<std::pair<ttstr, cocos2d::Color3B> > _queuedLines;
	unsigned int _maxQueueSize;
};
