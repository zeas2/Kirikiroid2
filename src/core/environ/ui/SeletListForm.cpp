#include "SeletListForm.h"
#include "ui/UIHelper.h"
#include "ui/UIPageView.h"
#include "ui/UIButton.h"
#include "ui/UIText.h"
#include "ui/UITextField.h"
#include "platform/CCDevice.h"
#include "cocos2d/MainScene.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "cocos2d/CCKeyCodeConv.h"

using namespace cocos2d;
using namespace cocos2d::ui;

const char * const FileName_Body = "ui/SelectList.csb";
const char * const FileName_Cell = "ui/SelectListItem.csb";

void iTVPHalfScreenForm::rearrangeLayout() {
	float scale = TVPMainScene::GetInstance()->getUIScale();
	Size sceneSize = TVPMainScene::GetInstance()->getUINodeSize();
	sceneSize.height *= 0.5f;
	setContentSize(sceneSize);
	if (RootNode) {
		sceneSize.width /= scale;
		sceneSize.height /= scale;
		RootNode->setContentSize(sceneSize);
		ui::Helper::doLayout(RootNode);
		RootNode->setScale(scale);
    }
}

TVPSelectListForm * TVPSelectListForm::create(const std::vector<std::string> &info,
	const std::string &highlight_tid, const std::function<void(int)> &funcok)
{
	TVPSelectListForm *ret = new TVPSelectListForm();
	ret->autorelease();
	ret->FuncOK = funcok;
	ret->initFromFile(nullptr, FileName_Body, nullptr);
	ret->initWithInfo(info, highlight_tid);
	return ret;
}

void TVPSelectListForm::bindBodyController(const NodeMap &allNodes) {
	Node *node = allNodes.findController("pageview");
	Size nodesize = node->getContentSize();
	pageView = XKPageView::create(nodesize, nullptr);
	node->addChild(pageView);
	pageView->setDirection(extension::ScrollView::Direction::VERTICAL);
	ui::Button *btn = static_cast<ui::Button*>(allNodes.findController("ok"));
	btn->addClickEventListener([this](Ref*){
		FuncOK(pageView->getCurPageIndex());
		TVPMainScene::GetInstance()->popUIForm(this, TVPMainScene::eLeaveToBottom);
	});
	btn = static_cast<ui::Button*>(allNodes.findController("cancel"));
	btn->addClickEventListener([this](Ref*){
		TVPMainScene::GetInstance()->popUIForm(this, TVPMainScene::eLeaveToBottom);
	});
}

void TVPSelectListForm::initWithInfo(const std::vector<std::string> &info, const std::string &highlight_tid) {
	//pageView->removeAllPages();
	Size size = pageView->getViewSize();
	bool size_set = false;
	int selectedIdx = 0;
	for (int idx = 0; idx < info.size(); ++idx) {
		const std::string &str = info[idx];
		CSBReader reader;
		Widget *cell = static_cast<Widget*>(reader.Load(FileName_Cell));
		if (!size_set) {
			size.height = cell->getContentSize().height;
			pageView->setPageSize(size);
			size_set = true;
		}
		Text *text = static_cast<Text*>(reader.findController("text"));

		Node *BgOdd = reader.findController("bg_odd", false);
		Node *BgEven = reader.findController("bg_even", false);
		if (BgOdd) BgOdd->setVisible((idx + 1) & 1);
		if (BgEven) BgEven->setVisible(idx & 1);

		cell->setContentSize(size);
		ui::Helper::doLayout(cell);
		LocaleConfigManager::GetInstance()->initText(text, str);
		//text->addChild(LayerColor::create(cocos2d::Color4B(255, 255, 0, 255), text->getContentSize().width, text->getContentSize().height));
		Layout *lay = Layout::create();
		lay->setContentSize(size);
		lay->addChild(cell);
		if (str == highlight_tid) {
			selectedIdx = pageView->getPageCount();
		}
		pageView->addPage(lay);
	}
	pageView->setCurPageIndex(selectedIdx);
}

TVPTextPairInputForm * TVPTextPairInputForm::create(const std::string &text1, const std::string &text2,
	const std::function<void(const std::string &, const std::string &)> &funcok) {
	TVPTextPairInputForm *ret = new TVPTextPairInputForm();
	ret->autorelease();
	ret->FuncOK = funcok;
	ret->initFromFile(nullptr, "ui/TextPairInput.csb", nullptr);
	ret->initWithInfo(text1, text2);
	return ret;
}

void TVPTextPairInputForm::bindBodyController(const NodeMap &allNodes) {
	input1 = static_cast<TextField*>(allNodes.findController("input1"));
	input2 = static_cast<TextField*>(allNodes.findController("input2"));

	ui::Button *btn = static_cast<ui::Button*>(allNodes.findController("ok"));
	btn->addClickEventListener([this](Ref*){
		FuncOK(input1->getString(), input2->getString());
		TVPMainScene::GetInstance()->popUIForm(this, TVPMainScene::eLeaveToBottom);
	});
	btn = static_cast<ui::Button*>(allNodes.findController("cancel"));
	btn->addClickEventListener([this](Ref*){
		TVPMainScene::GetInstance()->popUIForm(this, TVPMainScene::eLeaveToBottom);
	});
}

void TVPTextPairInputForm::initWithInfo(const std::string &text1, const std::string &text2) {
	input1->setString(text1);
	input2->setString(text2);
}

TVPKeyPairSelectForm * TVPKeyPairSelectForm::create(const std::function<void(int)> &funcok)
{
	TVPKeyPairSelectForm *ret = new TVPKeyPairSelectForm;
	ret->autorelease();
	ret->_funcok = funcok;
	ret->initFromFile(FileName_Body);
	ret->initWithInfo();
	return ret;
}

TVPKeyPairSelectForm::~TVPKeyPairSelectForm()
{
	if (_keylistener) {
		_eventDispatcher->removeEventListener(_keylistener);
	}
}

void TVPKeyPairSelectForm::initWithInfo()
{
	FuncOK = [this](int idx) {
		auto &namemap = TVPGetVKCodeNameMap();
		auto it = namemap.find(_keyinfo[idx]);
		if (it == namemap.end()) {
			return;
		}
		_funcok(it->second);
	};
	auto &namemap = TVPGetVKCodeNameMap();
	_keyinfo.clear();
	_keyinfo.reserve(namemap.size());
	for (const auto &it : namemap) {
		_keyinfo.emplace_back(it.first);
	}
	inherit::initWithInfo(_keyinfo, "0");
	_keylistener = EventListenerKeyboard::create();
	_keylistener->onKeyPressed = CC_CALLBACK_2(TVPKeyPairSelectForm::onKeyPressed, this);
	_keylistener->onKeyReleased = CC_CALLBACK_2(TVPKeyPairSelectForm::onKeyReleased, this);
	_eventDispatcher->addEventListenerWithFixedPriority(_keylistener, 1);
}

void TVPKeyPairSelectForm::onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event)
{
}

void TVPKeyPairSelectForm::onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event)
{
	unsigned int code = TVPConvertKeyCodeToVKCode(keyCode);
	if (!code || code >= 0x200) return;

	_funcok(code);
}
