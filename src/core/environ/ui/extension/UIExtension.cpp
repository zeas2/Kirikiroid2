#include "UIExtension.h"
#include "base/ObjectFactory.h"
#include "ui/UIWidget.h"
#include "cocostudio/ActionTimeline/CSLoader.h"
#include "cocostudio/WidgetReader/NodeReader/NodeReader.h"

USING_NS_CC;
using namespace cocos2d::ui;

#define XKPAGEVIEW_TAG 10086

XKPageView *XKPageView::create(Size size, XKPageViewDelegate *delegate)
{
	XKPageView *page  = new XKPageView();
	if (page && page -> init(size, delegate)) {
		page ->autorelease();
	}else {
		CC_SAFE_RELEASE(page);
	}
	return  page;
}

bool XKPageView::init(Size size, XKPageViewDelegate *delegate)
{
	if (!ScrollView::initWithViewSize(size)) {
		return false;
	}
	//必须有delegate，否则断掉  
	//CCASSERT(delegate, "delegate should not be NULL!");
	setClippingToBounds(false);
	setDelegate(delegate);
	if (_delegate) {
		//获取page的大小  
		pageSize  = _delegate ->sizeForPerPage();
	}
	//init Data  
	pageCount  = 0;
	current_index  = 0;

	return true;
}


void XKPageView::adjust(float offset)
{
	Vec2 vec;
	float xOrY;
	const Size &size = getViewSize();
	if (_direction == ScrollView::Direction::HORIZONTAL) {
		vec = Vec2((size.width - pageSize.width) / 2 - current_index * pageSize.width, 0);
		xOrY  = pageSize.width;
	}else {
		vec = Vec2(0, (size.height - pageSize.height) / 2 - current_index * pageSize.height);
		xOrY  = pageSize.height;
	}

	if (std::abs(offset) < xOrY / 2){
		this->setContentOffsetInDuration(vec, 0.1f);
		return;
	}

	int i  = std::abs(offset  / xOrY) + 1;
	if (offset  < 0) {
		current_index += i;
		if (current_index >= pageCount){
			current_index = pageCount - 1;
		}
	}else {
		current_index  -= i;
		if (current_index < 0) {
			current_index = 0;
		}
	}

	if (_direction  == ScrollView::Direction::HORIZONTAL) {
		vec = Vec2((size.width - pageSize.width) / 2 - current_index * pageSize.width, 0);
	}else {
		vec = Vec2(0, (size.height - pageSize.height) / 2 - current_index * pageSize.height);
	}

	this ->setContentOffsetInDuration(vec, 0.15f);
}

void XKPageView::setContentOffset(Vec2 offset)
{
	ScrollView::setContentOffset(offset);
	if (_delegate  != nullptr)
	{
		_delegate ->pageViewDidScroll(this);
	}
}

void XKPageView::setContentOffsetInDuration(Vec2 offset, float dt)
{
	ScrollView::setContentOffsetInDuration(offset, dt);
	this->schedule(CC_SCHEDULE_SELECTOR(XKPageView::performedAnimatedScroll));
}

void XKPageView::performedAnimatedScroll(float dt)
{
	if (_dragging)
	{
		this->unschedule(CC_SCHEDULE_SELECTOR(XKPageView::performedAnimatedScroll));
		return;
	}

	if (_delegate  != nullptr)
	{
		_delegate ->pageViewDidScroll(this);
	}
}

void XKPageView::addPage(Node *node)
{
	if (_direction  == ScrollView::Direction::HORIZONTAL) {
		node ->setPosition(Point(pageCount * pageSize.width  + node ->getPositionX(), node ->getPositionY()));
		this ->setContentSize(Size((pageCount  + 1) * pageSize.width, pageSize.height));
	}else {
		node ->setPosition(Point(node ->getPositionX(), pageCount * pageSize.height  + node ->getPositionY()));
		this ->setContentSize(Size(pageSize.width, (pageCount  + 1) *pageSize.height));
	}
	node ->setTag(pageCount  + XKPAGEVIEW_TAG);
	_container ->addChild(node);
	pageCount ++;

}

Node *XKPageView::getPageAtIndex(int index)
{
	if (index  < pageCount && index  >= 0) {
		return _container ->getChildByTag(index  + XKPAGEVIEW_TAG);
	}
	return  NULL;
}

void XKPageView::setTouchEnabled(bool enabled) {
	_eventDispatcher->removeEventListener(_touchListener);
	_touchListener = nullptr;

	if (enabled)
	{
		_touchListener = EventListenerTouchOneByOne::create();
		_touchListener->onTouchBegan = CC_CALLBACK_2(XKPageView::onTouchBegan, this);
		_touchListener->onTouchMoved = CC_CALLBACK_2(XKPageView::onTouchMoved, this);
		_touchListener->onTouchEnded = CC_CALLBACK_2(XKPageView::onTouchEnded, this);
		_touchListener->onTouchCancelled = CC_CALLBACK_2(ScrollView::onTouchCancelled, this);

		_eventDispatcher->addEventListenerWithSceneGraphPriority(_touchListener, this);
	} else
	{
		_dragging = false;
		_touchMoved = false;
		_touches.clear();
	}
}

bool XKPageView::onTouchBegan(Touch *touch, Event *unusedEvent) {
	if (!this->isVisible() || !this->hasVisibleParents())
	{
		return false;
	}

	Rect frame = getViewRect();

	//dispatcher does not know about clipping. reject touches outside visible bounds.
	Vec2 nsp = convertToNodeSpace(touch->getLocation());
	Rect bb;
	bb.size = _contentSize;

	if (_touches.size() > 1 ||
		_touchMoved ||
		!bb.containsPoint(nsp))
	{
		return false;
	}

	_touches.push_back(touch);

	_touchPoint = nsp;
	_dragging = true; //dragging started
	_scrollDistance.setZero();
	_touchLength = 0.0f;
	if (_direction == ScrollView::Direction::HORIZONTAL) {
		current_offset = this->getContentOffset().x;
	} else {
		current_offset = this->getContentOffset().y;
	}
	return true;
}

void XKPageView::onTouchMoved(Touch *touch, Event *unusedEvent) {
	Vec2 moveDistance, newPoint;
	newPoint = this->convertTouchToNodeSpace(_touches[0]);
	moveDistance = newPoint - _touchPoint;
	if (_direction == ScrollView::Direction::HORIZONTAL)
		this->setContentOffset(Vec2(current_offset + moveDistance.x, 0));
	else
		this->setContentOffset(Vec2(0, current_offset + moveDistance.y));
}

void XKPageView::onTouchEnded(Touch *touch, Event *unusedEvent) {
	float start = current_offset, end;
	if (_direction == ScrollView::Direction::HORIZONTAL) {
		end = this->getContentOffset().x;
	} else {
		end = this->getContentOffset().y;
	}
	float offset = end - start;
	this->adjust(offset);
	_dragging = true;
	_touches.clear();
}

void XKPageView::setCurPageIndex(ssize_t idx) {
	if (idx < 0) idx = 0;
	if (idx >= pageCount) idx = pageCount - 1;
	current_index = idx;

	Vec2 vec;
	const Size &size = getViewSize();
	if (_direction == ScrollView::Direction::HORIZONTAL) {
		vec = Vec2((size.width - pageSize.width) / 2 - current_index * pageSize.width, 0);
	} else {
		vec = Vec2(0, (size.height - pageSize.height) / 2 - current_index * pageSize.height);
	}

	this->setContentOffset(vec);
}

class WidgetNodeReader : public cocostudio::NodeReader {
public:
	virtual cocos2d::Node* createNodeWithFlatBuffers(const flatbuffers::Table* nodeOptions)
	{
		Widget* node = Widget::create();

		setPropsWithFlatBuffers(node, nodeOptions);

		return node;
	}
	static WidgetNodeReader *getInstance() {
		static WidgetNodeReader *instance = new WidgetNodeReader;
		return instance;
	}
	static cocos2d::Ref *createInstance() {
		return getInstance();
	}
};

void TVPInitUIExtension() {
	CSLoader::getInstance();
	static cocos2d::ObjectFactory::TInfo __Type("NodeReader", WidgetNodeReader::createInstance);
}
