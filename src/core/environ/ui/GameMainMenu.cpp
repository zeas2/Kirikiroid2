#include "GameMainMenu.h"
#include "cocos2d/MainScene.h"
#include "Application.h"
#include "WindowIntf.h"
#include "ui/UIHelper.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventListenerTouch.h"
#include "2d/CCActionInterval.h"
#include "TickCount.h"
#include "MenuItemImpl.h"
#include "InGameMenuForm.h"
#include "base/CCDirector.h"
#include "platform/CCGLView.h"
#include "Platform.h"

using namespace cocos2d;
using namespace cocos2d::ui;

const float _shrinkSpd = 700; // px/sec
const float _expandSpd = 700; // px/sec
const float _handlerFadeInTime = 0.15f;
const float _handlerFadeOutTime = 0.35f;

TVPGameMainMenu::TVPGameMainMenu(GLubyte opa) {
	_shrinked = false;
	_hitted = false;
	_handler_inactive_opacity = opa;
}

TVPGameMainMenu * TVPGameMainMenu::create(GLubyte opa) {
	TVPGameMainMenu *ret = new TVPGameMainMenu(opa);
	ret->init();
	ret->autorelease();
	return ret;
}

iTJSDispatch2* TVPGetMenuDispatch(tTVInteger hWnd);
tTJSNI_Window *TVPGetActiveWindow();

bool TVPGameMainMenu::init() {
	bool ret = Node::init();

	CSBReader reader;
	_root = reader.Load("ui/GameMenu.csb");
	Size size = TVPMainScene::GetInstance()->getGameNodeSize();
	float scale;
	if (size.width > size.height) {
		scale = size.height / 720;
	} else {
		scale = size.width / 720;
	}
	setScale(scale);
	size.width /= scale;
	size.height = _root->getContentSize().height;
	_root->setContentSize(size);
	ui::Helper::doLayout(_root);
	addChild(_root);

	_handler = reader.findController("handler");
	//_handler_inactive_opacity = _handler->getOpacity();
	_eventDispatcher->removeEventListenersForTarget(_handler);
	EventListenerTouchOneByOne *listener = EventListenerTouchOneByOne::create();
	listener->setSwallowTouches(true);
	listener->onTouchBegan = CC_CALLBACK_2(TVPGameMainMenu::onHandlerTouchBegan, this);
	listener->onTouchMoved = CC_CALLBACK_2(TVPGameMainMenu::onHandlerTouchMoved, this);
	listener->onTouchEnded = CC_CALLBACK_2(TVPGameMainMenu::onHandlerTouchEnded, this);
	listener->onTouchCancelled = CC_CALLBACK_2(TVPGameMainMenu::onHandlerTouchCancelled, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, _handler);

	listener = EventListenerTouchOneByOne::create();
	listener->setSwallowTouches(true);
	listener->onTouchBegan = CC_CALLBACK_2(TVPGameMainMenu::onBackgroundTouchBegan, this);
	listener->onTouchMoved = CC_CALLBACK_2(TVPGameMainMenu::onBackgroundTouchMoved, this);
	listener->onTouchEnded = CC_CALLBACK_2(TVPGameMainMenu::onBackgroundTouchEnded, this);
	listener->onTouchCancelled = CC_CALLBACK_2(TVPGameMainMenu::onBackgroundTouchCancelled, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, _root);

	reader.findWidget("btn_gamemenu")->addClickEventListener([this](Ref*){
		iTJSDispatch2 *menuobj = TVPGetMenuDispatch((tjs_intptr_t)TVPGetActiveWindow());
		if (!menuobj) return;
		tTJSNI_MenuItem *menu;
		menuobj->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_MenuItem::ClassID, (iTJSNativeInstance**)&menu);
		if (!menu->GetChildren().empty())
			TVPMainScene::GetInstance()->pushUIForm(TVPInGameMenuForm::create("GameMenu", menu));
		shrink();
	});

	reader.findWidget("btn_window")->addClickEventListener([this](Ref*){
		TVPMainScene::GetInstance()->showWindowManagerOverlay(true);
		shrink();
	});

	_icon_touch = reader.findController("icon_touch");
	_icon_mouse = reader.findController("icon_mouse");
	setMouseIcon(true);

	reader.findWidget("btn_mousemode")->addClickEventListener([this](Ref*){
		TVPMainScene::GetInstance()->toggleVirtualMouseCursor();
		setMouseIcon(!TVPMainScene::GetInstance()->isVirtualMouseMode());
		shrink();
	});

	reader.findWidget("btn_keyboard")->addClickEventListener([this](Ref*){
 		Size screenSize = cocos2d::Director::getInstance()->getOpenGLView()->getFrameSize();
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
		TVPShowIME(0, 0, screenSize.width, screenSize.height);
#else
		TVPMainScene::GetInstance()->attachWithIME();
#endif
		shrink();
	});

	reader.findWidget("btn_exit")->addClickEventListener([this](Ref*){
		Application->PostUserMessage([](){
			TVPGetActiveWindow()->Close();
		});
		shrink();
	});

	return ret;
}

void TVPGameMainMenu::setMouseIcon(bool bMouse) {
	if (_icon_mouse) _icon_mouse->setVisible(bMouse);
	if (_icon_touch) _icon_touch->setVisible(!bMouse);
}

bool TVPGameMainMenu::onHandlerTouchBegan(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) {
	_hitted = false;
	_touchBeganPosition = _handler->convertToNodeSpace(touch->getLocation());
	Rect bb;
	bb.size = _handler->getContentSize();
	_hitted = bb.containsPoint(_touchBeganPosition);

	if (_hitted) {
		_root->stopAllActions();
		_handler->stopAllActions();
		_handler->runAction(FadeIn::create(_handlerFadeInTime));
		_draggingY = false;
		_draggingX = false;
		_touchBeganTime = TVPGetTickCount();
	}
	return _hitted;
}

void TVPGameMainMenu::onHandlerTouchMoved(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) {
	Vec2 nsp = _handler->convertToNodeSpace(touch->getLocation());
	Vec2 movDist = nsp - _touchBeganPosition;
	if (!_draggingY && std::abs(movDist.y) > 1) {
		_draggingY = true;
	}
	if (!_draggingX && std::abs(movDist.x) > 1) {
		_draggingX = true;
	}

	_handler->setPositionX(_handler->getPositionX() + movDist.x);
	const Vec2 &pos = _handler->getPosition();
	const Size &size = _handler->getContentSize();
	const Vec2 &anchor = _handler->getAnchorPointInPoints();
	const Size &rootsize = _root->getContentSize();
	float handlerLeftBoundary = pos.x - anchor.x;
	float handlerRightBoundary = handlerLeftBoundary + size.width;
	if (handlerLeftBoundary < 0) {
		_handler->setPositionX(anchor.x);
	} else {
		if (handlerRightBoundary > rootsize.width) {
			_handler->setPositionX(rootsize.width - size.width + anchor.x);
		}
	}

	float newPosY = _root->getPositionY() + movDist.y;
	if (newPosY < -rootsize.height) newPosY = -rootsize.height;
	else if (newPosY > 0) {
		float t = rootsize.height / 2;
		newPosY = (1 - 1 / (newPosY / t + 1)) * t;
	}
	_root->setPositionY(newPosY);
}

void TVPGameMainMenu::onHandlerTouchEnded(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) {
	_hitted = false;
	Vec2 nsp = _handler->convertToNodeSpace(touch->getLocation());
	bool isClick = TVPGetTickCount() - _touchBeganTime < 500;
	bool doExpand = false;
	if (isClick && !_draggingY) {
		doExpand = _shrinked ^ _draggingX;
	} else {
		const Size &rootsize = _root->getContentSize();
		if (_shrinked) {
			doExpand = _root->getPositionY() > -rootsize.height * 0.75f;
		} else {
			doExpand = _root->getPositionY() > -rootsize.height * 0.25f;
		}
	}
	if (doExpand) {
		expand();
	} else {
		shrink();
	}
}

void TVPGameMainMenu::onHandlerTouchCancelled(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) {
	onHandlerTouchEnded(touch, unusedEvent);
}

void TVPGameMainMenu::shrink() {
	if (_hitted) return;
	_shrinked = true;

	float h = _root->getContentSize().height * _root->getScaleY();
	float topBoundary = _root->getPosition().y + h;
	float duration = topBoundary / _shrinkSpd;

	_root->stopAllActions();
	_root->runAction(MoveTo::create(duration, Vec2(0, -h)));

	_handler->runAction(Sequence::createWithTwoActions(DelayTime::create(2),
		FadeTo::create(_handlerFadeOutTime, _handler_inactive_opacity)));
}

void TVPGameMainMenu::shrinkWithTime(float dur) {
	_shrinked = true;
	float h = _root->getContentSize().height * _root->getScaleY();
	float topBoundary = _root->getPosition().y + h;
	float duration = topBoundary / _shrinkSpd;
	_handler->setOpacity(255);
	_root->runAction(Sequence::createWithTwoActions(DelayTime::create(dur), MoveTo::create(duration, Vec2(0, -h))));
	_handler->runAction(Sequence::createWithTwoActions(DelayTime::create(2 + dur),
		FadeTo::create(_handlerFadeOutTime, _handler_inactive_opacity)));
}

void TVPGameMainMenu::expand() {
	if (_hitted) return;
	_shrinked = false;

	float bottomBoundary = _root->getPosition().y;
	float duration = std::abs(bottomBoundary) / _expandSpd;

	_root->stopAllActions();
	_root->runAction(MoveTo::create(duration, Vec2(0, 0)));
	_handler->runAction(FadeIn::create(_handlerFadeInTime));
}

bool TVPGameMainMenu::onBackgroundTouchBegan(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) {
	if (!_shrinked) {
		shrink();
		return true;
	}
	return false;
}

void TVPGameMainMenu::onBackgroundTouchMoved(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) {

}

void TVPGameMainMenu::onBackgroundTouchEnded(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) {

}

void TVPGameMainMenu::onBackgroundTouchCancelled(cocos2d::Touch *touch, cocos2d::Event *unusedEvent) {

}

void TVPGameMainMenu::toggle() {
	if (_hitted) return; // in touching
	if (_shrinked) expand();
	else shrink();
}

bool TVPGameMainMenu::isShrinked() {
	return _shrinked;
}
