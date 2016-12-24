#pragma once
#include "BaseForm.h"

class TVPMessageBoxForm : public iTVPBaseForm {
public:
	static void show(const std::string &caption, const std::string &text, int nBtns,
		const std::string *btnText, const std::function<void(int)> &callback);

	static void showYesNo(const std::string &caption, const std::string &text, const std::function<void(int)> &callback);

private:
	void init(const std::string &caption, const std::string &text, int nBtns,
		const std::string *btnText, const std::function<void(int)> &callback);
	virtual void bindBodyController(const NodeMap &allNodes) override;
	virtual void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

	cocos2d::ui::ScrollView *_textContainer; // "text"
	cocos2d::Node *_btnList; // parent of "btn"
	cocos2d::ui::Text* _title, *_textContent; // "content"
	cocos2d::ui::Widget *_btnModel; // "btn"
	cocos2d::ui::Button *_btnBody;

	std::function<void(int)> _callback;
};
