#pragma once
#include "BaseForm.h"
#include "base/CCRefPtr.h"

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

class TVPSimpleProgressForm : public iTVPBaseForm {
public:
	static TVPSimpleProgressForm* create();

	void initButtons(const std::vector<std::pair<std::string, std::function<void(cocos2d::Ref*)> > > &vec);

	void setTitle(const std::string &text);
	void setContent(const std::string &text);
	void setPercentWithText(float percent);
	void setPercentWithText2(float percent);
	void setPercentOnly(float percent);
	void setPercentOnly2(float percent);
	void setPercentText(const std::string &text);
	void setPercentText2(const std::string &text);
	void setProgress2Visible(bool visible);

private:
	void bindBodyController(const NodeMap &allNodes) override;

	cocos2d::ui::LoadingBar *_progressBar[2];
	cocos2d::ui::Text *_textTitle, *_textContent, *_textProgress[2];
	std::vector<cocos2d::ui::Button *> _vecButtons;
	cocos2d::Node *_btnContainer;
	// button template
	cocos2d::RefPtr<cocos2d::ui::Widget> _btnCell;
	cocos2d::ui::Button *_btnButton;
};
