#include "InGameMenuForm.h"
#include "cocos2d/MainScene.h"
#include "ui/UIButton.h"
#include "ui/UIListView.h"
#include "ui/UIText.h"
#include "MenuItemImpl.h"
#include "ui/UIHelper.h"
#include "tjsGlobalStringMap.h"

using namespace cocos2d;
using namespace cocos2d::ui;

const char * const FileName_NaviBar = "ui/NaviBar.csb";
const char * const FileName_Body = "ui/ListView.csb";

TVPInGameMenuForm * TVPInGameMenuForm::create(const std::string& title, tTJSNI_MenuItem *item) {
	TVPInGameMenuForm *ret = new TVPInGameMenuForm;
	ret->autorelease();
	ret->initFromFile(FileName_NaviBar, FileName_Body, nullptr);
	ret->initMenu(title, item);
	return ret;
}

void TVPInGameMenuForm::bindBodyController(const NodeMap &allNodes) {
	_list = static_cast<ListView*>(allNodes.findController("list"));
	if (NaviBar.Left) {
		NaviBar.Left->addClickEventListener([this](cocos2d::Ref*){
			TVPMainScene::GetInstance()->popUIForm(this);
		});
	}
}

void TVPInGameMenuForm::bindHeaderController(const NodeMap &allNodes)
{
	_title = static_cast<Button*>(allNodes.findController("title"));
	if (_title) _title->setEnabled(false);
}

void TVPInGameMenuForm::initMenu(const std::string& title, tTJSNI_MenuItem *item) {
	_list->removeAllItems();
	if (_title) {
		if (title.empty()) {
			ttstr caption; item->GetCaption(caption);
			_title->setTitleText(caption.AsStdString());
		} else {
			_title->setTitleText(title);
		}
	}

	int count = item->GetChildren().size();
	int idx = 0;
	ttstr seperator = TJS::TJSMapGlobalStringMap(TJS_W("-"));
	for (int i = 0; i < count; ++i) {
		tTJSNI_MenuItem *subitem = static_cast<tTJSNI_MenuItem*>(item->GetChildren().at(i));
		ttstr caption; subitem->GetCaption(caption);
		if (caption.IsEmpty() || caption == TJS_W("+")) continue;
		_list->pushBackCustomItem(createMenuItem(idx, subitem, caption.AsStdString()));
		if(caption != seperator)
			++idx;
	}
}

cocos2d::ui::Widget * TVPInGameMenuForm::createMenuItem(int idx, tTJSNI_MenuItem *item, const std::string &caption) {
	iPreferenceItem *ret = nullptr;
	const Size &size = _list->getContentSize();
	if (!item->GetChildren().empty()) {
		ret = CreatePreferenceItem<tPreferenceItemSubDir>(idx, size, caption);
		ret->addClickEventListener([=](Ref*){
			TVPMainScene::GetInstance()->pushUIForm(create(caption, item));
		});
	} else if (item->GetGroup() > 0 || item->GetRadio()) {
		auto getter = [=]()->bool{ return item->GetChecked(); };
		auto setter = [=](bool b){
			item->OnClick();
			TVPMainScene::GetInstance()->popAllUIForm();
		};
		ret = CreatePreferenceItem<tPreferenceItemCheckBox>(idx, size, caption,
			[=](tPreferenceItemCheckBox* item) {
			item->_getter = getter;
			item->_setter = setter;
		});
	} else if (item->GetChecked()) {
		auto getter = [=]()->bool{ return item->GetChecked(); };
		auto setter = [=](bool b){ item->OnClick(); };
		ret = CreatePreferenceItem<tPreferenceItemCheckBox>(idx, size, caption,
			[=](tPreferenceItemCheckBox* item) {
			item->_getter = getter;
			item->_setter = setter;
		});
	} else if(caption == "-") {
		CSBReader reader;
		Widget * root = static_cast<Widget*>(reader.Load("ui/comctrl/SeperateItem.csb"));
		Size rootsize = root->getContentSize();
		rootsize.width = size.width;
		root->setContentSize(rootsize);
		ui::Helper::doLayout(root);
		return root;
	} else {
		ret = CreatePreferenceItem<tPreferenceItemConstant>(idx, size, caption);
		ret->addClickEventListener([=](Ref*){
			TVPMainScene::GetInstance()->scheduleOnce(
				std::bind(&TVPMainScene::popAllUIForm, TVPMainScene::GetInstance()), 0, "close_menu");
			item->OnClick();
		});
		ret->setTouchEnabled(true);
	}
	return ret;
}

void TVPShowPopMenu(tTJSNI_MenuItem* menu) {
	TVPMainScene::GetInstance()->pushUIForm(TVPInGameMenuForm::create(std::string(), menu));
}
