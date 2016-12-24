#pragma once

#include "2d/CCScene.h"
#include "ui/UIWidget.h"
#include "base/CCIMEDelegate.h"

namespace cocos2d {
	class Controller;
}

class TVPWindowLayer;
class TVPGameMainMenu;
class TVPMainScene : public cocos2d::Scene, public cocos2d::IMEDelegate
{
	TVPMainScene();
	static TVPMainScene *create();
	virtual void update(float delta) override;
	void initialize();
	friend class TVPAppDelegate;
public:
	static TVPMainScene* GetInstance();
	static TVPMainScene* CreateInstance();

	static void setMaskLayTouchBegain(const std::function<bool(cocos2d::Touch *, cocos2d::Event *)> &func);

	enum eEnterAni {
		eEnterAniNone,
		eEnterAniOverFromRight,
		eEnterFromBottom,
	};

	void pushUIForm(cocos2d::Node *node, eEnterAni ani = eEnterAniOverFromRight);

	enum eLeaveAni {
		eLeaveAniNone,
		eLeaveAniLeaveFromLeft,
		eLeaveToBottom,
	};
	void popUIForm(cocos2d::Node *node, eLeaveAni ani = eLeaveAniLeaveFromLeft);
	void popAllUIForm();

	void addLayer(TVPWindowLayer* lay);
	cocos2d::Size getUINodeSize();
	cocos2d::Size getGameNodeSize() { return GameNode->getContentSize(); }
	void rotateUI();

	bool startupFrom(const std::string &path);

	float getUIScale();

	void showWindowManagerOverlay(bool bVisible);

	void toggleVirtualMouseCursor();
	void showVirtualMouseCursor(bool bVisible);
	bool isVirtualMouseMode() const;

	virtual bool attachWithIME() override;
	virtual bool detachWithIME() override;

	static void onCharInput(int keyCode);
	static void onTextInput(const std::string &text);

	static float convertCursorScale(float cfgScale/*0 ~ 1*/);

private:
	void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
	void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

	bool onTouchBegan(cocos2d::Touch *touch, cocos2d::Event *event);
	void onTouchMoved(cocos2d::Touch *touch, cocos2d::Event *event);
	void onTouchEnded(cocos2d::Touch *touch, cocos2d::Event *event);
	void onTouchCancelled(cocos2d::Touch *touch, cocos2d::Event *event);

	void onAxisEvent(cocos2d::Controller* ctrl, int code, cocos2d::Event *e);
	void onPadKeyDown(cocos2d::Controller* ctrl, int code, cocos2d::Event *e);
	void onPadKeyUp(cocos2d::Controller* ctrl, int code, cocos2d::Event *e);
	void onPadKeyRepeat(cocos2d::Controller* ctrl, int code, cocos2d::Event *e);

	virtual bool canAttachWithIME() override;
	virtual bool canDetachWithIME() override;
	virtual void deleteBackward();
	virtual void insertText(const char * text, size_t len);

	void doStartup(float dt, std::string path);

	float ScreenRatio;
	cocos2d::Size SceneSize, UISize;
	cocos2d::Node *UINode, *GameNode;
	cocos2d::EventListenerTouchOneByOne* _touchListener;
	TVPGameMainMenu *_gameMenu;
};