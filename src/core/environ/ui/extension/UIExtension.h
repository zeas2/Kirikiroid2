#pragma once
#include "extensions/GUI/CCScrollView/CCScrollView.h"

class XKPageView;
class XKPageViewDelegate
{
public:
	virtual ~XKPageViewDelegate(){};
	XKPageViewDelegate(){};
	virtual cocos2d::Size sizeForPerPage() = 0;
	virtual void pageViewDidScroll(XKPageView *pageView){};
};

class XKPageView : public cocos2d::extension::ScrollView
{
public:
	static XKPageView *create(cocos2d::Size size, XKPageViewDelegate *delegate);
	virtual bool init(cocos2d::Size size, XKPageViewDelegate *delegate);
	ssize_t getCurPageIndex() const { return current_index; }
	void setCurPageIndex(ssize_t idx);
	ssize_t getPageCount() const { return pageCount; }
public:
	void setPageSize(const cocos2d::Size &size) { pageSize = size; }
	virtual void setContentOffsetInDuration(cocos2d::Vec2 offset, float dt);
	virtual void setContentOffset(cocos2d::Vec2 offset);
	void setTouchEnabled(bool enabled);

private:
	virtual bool onTouchBegan(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) override;
	virtual void onTouchMoved(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) override;
	virtual void onTouchEnded(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) override;

	void performedAnimatedScroll(float dt);
	int current_index;
	float current_offset;

	void adjust(float offset);
	cocos2d::Size pageSize;
	CC_SYNTHESIZE(XKPageViewDelegate *, _delegate, Delegate);
public:
	int pageCount;
	void addPage(Node *node);
	cocos2d::Node *getPageAtIndex(int index);
};

void TVPInitUIExtension();