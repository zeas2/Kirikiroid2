#include "MessageBox.h"
#include "cocos2d/MainScene.h"
#include "ui/UIButton.h"
#include "ui/UIText.h"
#include "ui/UIScrollView.h"
#include "ui/UIHelper.h"
#include "ui/UILoadingBar.h"
#include "2d/CCLabel.h"
#include "ConfigManager/LocaleConfigManager.h"

using namespace cocos2d;
using namespace cocos2d::ui;

void TVPMessageBoxForm::show(const std::string &caption, const std::string &text, int nBtns, const std::string *btnText, const std::function<void(int)> &callback)
{
	TVPMessageBoxForm *ret = new TVPMessageBoxForm;
	ret->autorelease();
	ret->init(caption, text, nBtns, btnText, callback);
	TVPMainScene::GetInstance()->pushUIForm(ret, TVPMainScene::eEnterAniNone);
}

void TVPMessageBoxForm::showYesNo(const std::string &caption, const std::string &text, const std::function<void(int)> &callback)
{
	LocaleConfigManager *mgr = LocaleConfigManager::GetInstance();
	std::string btns[2] = {
		mgr->GetText("msgbox_yes"),
		mgr->GetText("msgbox_no")
	};
	return show(caption, text, 2, btns, callback);
}

void TVPMessageBoxForm::init(const std::string &caption, const std::string &text, int nBtns, const std::string *btnText, const std::function<void(int)> &callback)
{
	_callback = callback;

	initFromFile(nullptr, "ui/MessageBox.csb", nullptr);

	if (_title) _title->setString(caption);
	if (_textContent) {
		_textContent->setString("");
		_textContent->ignoreContentAdaptWithSize(true);
		_textContent->setString(text);
		const Size &textSize = _textContent->getContentSize();
		const Size &viewSize = _textContainer->getInnerContainerSize();
		if (textSize.height > viewSize.height) {
			_textContainer->setInnerContainerSize(textSize);
			_textContent->setPosition(Vec2(0, textSize.height));
		}
	}
	_btnModel->retain();
	_btnModel->removeFromParentAndCleanup(false);

	std::vector<Widget *> btns;

	float totalWidth = 0;

	Size origsize = _btnModel->getContentSize();
	Size btnSize = _btnBody->getContentSize();

	for (int i = 0; i < nBtns; ++i) {
		_btnBody->setTitleText(btnText[i]);
		Size textSize = _btnBody->getTitleRenderer()->getContentSize();
		float fontSize = _btnBody->getTitleFontSize();
		textSize.width += fontSize;
		_btnBody->addClickEventListener([this, i](Ref* node) {
			retain();
			TVPMainScene::GetInstance()->popUIForm(this, TVPMainScene::eLeaveAniNone);
			if (_callback) _callback(i);
			release();
		});
		Size size = _btnModel->getContentSize();
		if (btnSize.width < textSize.width) {
			Size size = origsize;
			size.width += textSize.width - btnSize.width;
			_btnModel->setContentSize(size);
			ui::Helper::doLayout(_btnModel);
		} else if (size.width != origsize.width) {
			_btnModel->setContentSize(origsize);
			ui::Helper::doLayout(_btnModel);
		}
		Widget *btn = _btnModel->clone();
		totalWidth += btn->getContentSize().width;
		btns.emplace_back(btn);
		_btnList->addChild(btn);
		btn->setTag(i);
	}
	float gap = _btnList->getContentSize().width - totalWidth;
	if (gap < 0) {
		gap /= nBtns + 1;
	} else {
		gap /= nBtns + 1;
	}
	float x = gap;

	for (Widget *btn : btns) {
		Vec2 pos = btn->getPosition();
		pos.x = x;
		btn->setPosition(pos);
		x += btn->getContentSize().width + gap;
	}
	_btnModel->release();
	_btnModel = nullptr;
}

void TVPMessageBoxForm::bindBodyController(const NodeMap &allNodes)
{
	_title = dynamic_cast<Text*>(allNodes.findController("title"));
	_textContent = dynamic_cast<Text*>(allNodes.findController("content"));
	_textContainer = dynamic_cast<ScrollView*>(allNodes.findController("text"));

	_btnBody = dynamic_cast<Button*>(allNodes.findController("btnBody"));
	_btnModel = allNodes.findWidget("btn");
	_btnList = _btnModel->getParent();
}

void TVPMessageBoxForm::onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event)
{
	if (keyCode == cocos2d::EventKeyboard::KeyCode::KEY_BACK) {
		TVPMainScene::GetInstance()->popUIForm(this, TVPMainScene::eLeaveAniNone);
	}
}

TVPSimpleProgressForm* TVPSimpleProgressForm::create()
{
	TVPSimpleProgressForm* form = new TVPSimpleProgressForm;
	form->autorelease();
	form->initFromFile("ui/ProgressBox.csb");
	return form;
}

void TVPSimpleProgressForm::initButtons(const std::vector<std::pair<std::string, std::function<void(cocos2d::Ref*)> > > &vec)
{
	Size btnSize = _btnCell->getContentSize();
	float totalWidth = 0;
	float containerWidth = _btnContainer->getContentSize().width;
	float edge = _btnCell->getPosition().x;
	containerWidth -= edge * 2;
	std::vector<Node *> buttons;
	float btnEdge = btnSize.width - _btnButton->getTitleRenderer()->getContentSize().width;
	for (const auto &it : vec) {
		_btnButton->setTitleText(it.first);
		_btnButton->addClickEventListener(it.second);
		btnSize.width = _btnButton->getTitleRenderer()->getContentSize().width + btnEdge;
		_btnCell->setContentSize(btnSize);
		ui::Helper::doLayout(_btnCell);
		totalWidth += btnSize.width;
		Widget *btn = _btnCell->clone();
		buttons.push_back(btn);
		_btnContainer->addChild(btn);
	}
	float x = edge;
	float gap = (containerWidth - totalWidth) / buttons.size();
	for (Node *btn : buttons) {
		btn->setPositionX(x);
		x += btn->getContentSize().width;
	}
}

void TVPSimpleProgressForm::setTitle(const std::string &text)
{
	_textTitle->setString(text);
}
void TVPSimpleProgressForm::setContent(const std::string &text)
{
	_textContent->setString(text);
}

void TVPSimpleProgressForm::setPercentWithText(float percent)
{
	_progressBar[0]->setPercent(percent);
	char tmp[16];
	sprintf(tmp, "%2.2f%%", percent);
	_textProgress[0]->setString(tmp);
}
void TVPSimpleProgressForm::setPercentWithText2(float percent)
{
	_progressBar[1]->setPercent(percent);
	char tmp[16];
	sprintf(tmp, "%2.2f%%", percent);
	_textProgress[1]->setString(tmp);
}

void TVPSimpleProgressForm::setPercentOnly(float percent)
{
	_progressBar[0]->setPercent(percent * 100);
}

void TVPSimpleProgressForm::setPercentOnly2(float percent)
{
	_progressBar[1]->setPercent(percent * 100);
}

void TVPSimpleProgressForm::setPercentText(const std::string &text)
{
	_textProgress[0]->setString(text);
}

void TVPSimpleProgressForm::setPercentText2(const std::string &text)
{
	_textProgress[1]->setString(text);
}

void TVPSimpleProgressForm::setProgress2Visible(bool visible)
{
	// TODO
}

void TVPSimpleProgressForm::bindBodyController(const NodeMap &allNodes)
{
	LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
	_progressBar[0] = allNodes.findController<LoadingBar>("progrss_1");
	_progressBar[1] = allNodes.findController<LoadingBar>("progrss_2");
	_textProgress[0] = allNodes.findController<Text>("progress_text_1");
	_textProgress[1] = allNodes.findController<Text>("progress_text_2");
	_textContent = allNodes.findController<Text>("text");
	_textTitle = allNodes.findController<Text>("title");
	_btnContainer = allNodes.findController("btnList");
	_btnCell = allNodes.findWidget("btnCell");
	_btnButton = allNodes.findController<Button>("btn");
	_btnCell->removeFromParentAndCleanup(false);
}
