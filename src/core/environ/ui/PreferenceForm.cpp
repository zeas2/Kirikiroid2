#include "PreferenceForm.h"
#include "ui/UIText.h"
#include "ui/UIHelper.h"
#include "ui/UICheckBox.h"
#include "ui/UIListView.h"
#include "ui/UISlider.h"
#include "ui/UIButton.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "ConfigManager/GlobalConfigManager.h"
#include "tinyxml2/tinyxml2.h"
#include "cocos2d/MainScene.h"
#include "2d/CCSprite.h"
#include "SeletListForm.h"
#include "FileSelectorForm.h"
#include "Platform.h"

using namespace cocos2d;
using namespace cocos2d::ui;

void TVPPreferenceForm::initPref(const tPreferenceScreen *config) {
	Config = config;
	PrefList->removeAllItems();
	LocaleConfigManager::GetInstance()->initText(_title, config->Title);
	for (int idx = 0; idx < config->Preferences.size(); ++idx) {
		auto info = config->Preferences[idx];
		if (info) {
			PrefList->pushBackCustomItem(info->createItem(idx));
		}
	}
	Widget *nullcell = new Widget();
	nullcell->setContentSize(Size(PrefList->getContentSize().width, 200));
	PrefList->pushBackCustomItem(nullcell);
}

void TVPPreferenceForm::bindBodyController(const NodeMap &allNodes) {
	PrefList = static_cast<ListView*>(allNodes.findController("list"));
	if (NaviBar.Left) {
		NaviBar.Left->addClickEventListener([this](cocos2d::Ref*){
			TVPMainScene::GetInstance()->popUIForm(this);
		});
	}
}

void TVPPreferenceForm::bindHeaderController(const NodeMap &allNodes)
{
	_title = static_cast<Button*>(allNodes.findController("title"));
	if (_title) _title->setEnabled(false);
}

void tPreferenceScreen::clear() {
	for (auto p : Preferences) delete p;
	Preferences.clear();
}

void iPreferenceItem::initFromInfo(int idx, Size size, const std::string& title) {
	init();
	CSBReader reader;
	Node * root = reader.Load(getUIFileName());
	size.height = root->getContentSize().height;
	setContentSize(size);
	root->setContentSize(size);
	ui::Helper::doLayout(root);
	addChild(root);
	_title = static_cast<Text*>(reader.findController("title"));
	if (!title.empty()) _title->setString(title);
	BgOdd = reader.findController("bg_odd", false);
	BgEven = reader.findController("bg_even", false);
	if (BgOdd) BgOdd->setVisible((idx + 1) & 1);
	if (BgEven) BgEven->setVisible(idx & 1);
	initController(reader);
}

class HackCheckBox : public CheckBox {
public:
	void fireReleaseUpEvent() {
		bool newstat = !_isSelected;
		setSelected(newstat);
		dispatchSelectChangedEvent(newstat);
	}
};

tPreferenceItemCheckBox::tPreferenceItemCheckBox()
	: highlight(nullptr)
{

}

void tPreferenceItemCheckBox::initController(const NodeMap &allNodes) {
	checkbox = static_cast<CheckBox*>(allNodes.findController("checkbox"));
	checkbox->setSelected(_getter());
	checkbox->addEventListener([=](Ref*, CheckBox::EventType e) {
		this->_setter(e == CheckBox::EventType::SELECTED);
	});
	highlight = allNodes.findController("highlight");
	setTouchEnabled(true);
	addClickEventListener([this](Ref*){
		static_cast<HackCheckBox*>(checkbox)->fireReleaseUpEvent();
	});
}

const char* tPreferenceItemCheckBox::getUIFileName() const {
	return "ui/comctrl/CheckBoxItem.csb";
}

void tPreferenceItemCheckBox::onPressStateChangedToNormal() {
	if (highlight) highlight->setVisible(false);
}

void tPreferenceItemCheckBox::onPressStateChangedToPressed() {
	if (highlight) highlight->setVisible(true);
}

void tPreferenceItemConstant::initController(const NodeMap &allNodes) {
	highlight = allNodes.findController("highlight");
	allNodes.findController("dir_icon")->setVisible(false);
	Size origSize = _title->getContentSize();
	_title->setTextAreaSize(Size::ZERO);
	std::string s = _title->getString();
	Size sizeTmp = _title->getVirtualRendererSize();
	float addHeight = 0;
	if (sizeTmp.width < origSize.width) { // single line
		sizeTmp.width = origSize.width;;
		sizeTmp.height = 0;
		_title->setTextAreaSize(sizeTmp);
		addHeight = _title->getVirtualRendererSize().height - origSize.height;
		if (addHeight < 0) addHeight = 0;
	} else { // multi line
		sizeTmp.width = origSize.width;;
		sizeTmp.height = 0;
		_title->setTextAreaSize(sizeTmp);
		sizeTmp = _title->getVirtualRendererSize();
		_title->setContentSize(sizeTmp);
		addHeight = sizeTmp.height - origSize.height;
	}
	Node *root = getChildren().front();
	sizeTmp = root->getContentSize();
	sizeTmp.height += addHeight;
	root->setContentSize(sizeTmp);
	ui::Helper::doLayout(root);
	setContentSize(sizeTmp);
}

const char* tPreferenceItemSubDir::getUIFileName() const  {
	return "ui/comctrl/SubDirItem.csb";
}

void tPreferenceItemWithHighlight::initController(const NodeMap &allNodes) {
	setTouchEnabled(true);
	highlight = allNodes.findController("highlight");
}

void tPreferenceItemWithHighlight::onPressStateChangedToNormal() {
	if (highlight) highlight->setVisible(false);
}

void tPreferenceItemWithHighlight::onPressStateChangedToPressed() {
	if (highlight) highlight->setVisible(true);
}

tPreferenceItemSubDir::tPreferenceItemSubDir()
{

}

void tPreferenceItemSelectList::initController(const NodeMap &allNodes) {
	highlight = allNodes.findController("highlight");
	selected = static_cast<Text*>(allNodes.findController("selected"));
	updateHightlight();
	setTouchEnabled(true);
	addClickEventListener(std::bind(&tPreferenceItemSelectList::showForm, this, std::placeholders::_1));
}

const char* tPreferenceItemSelectList::getUIFileName() const  {
	return "ui/comctrl/SelectListItem.csb";
}

void tPreferenceItemSelectList::showForm(cocos2d::Ref*) {
	std::vector<std::string> lst;
	for (const std::pair<std::string, std::string>& item : CurInfo->getListInfo()) {
		lst.emplace_back(item.first);
	}
	TVPSelectListForm *form = TVPSelectListForm::create(lst, highlightTid, [this](int idx){
		const std::pair<std::string, std::string>& item = CurInfo->getListInfo()[idx];
		highlightTid = item.first;
		if (highlightTid.empty())
			selected->setString(item.second);
		else
			LocaleConfigManager::GetInstance()->initText(selected, highlightTid);
		_setter(item.second);
	});
	TVPMainScene::GetInstance()->pushUIForm(form, TVPMainScene::eEnterFromBottom);
}

void tPreferenceItemSelectList::onPressStateChangedToNormal() {
	if (highlight) highlight->setVisible(false);
}

void tPreferenceItemSelectList::onPressStateChangedToPressed() {
	if (highlight) highlight->setVisible(true);
}

tPreferenceItemSelectList::tPreferenceItemSelectList()
	: highlight(nullptr)
{

}

void tPreferenceItemSelectList::updateHightlight() {
	std::string val = _getter();
	for (const std::pair<std::string, std::string>& item : CurInfo->getListInfo()) {
		if (item.second == val) {
			highlightTid = item.first;
			LocaleConfigManager::GetInstance()->initText(selected, item.first);
			break;
		}
	}
}

tPreferenceItemKeyValPair::tPreferenceItemKeyValPair() {
	highlight = nullptr;
	selected = nullptr;
}

void tPreferenceItemKeyValPair::initController(const NodeMap &allNodes) {
	highlight = allNodes.findController("highlight");
	selected = static_cast<Text*>(allNodes.findController("selected"));
	std::pair<std::string, std::string> val = _getter();
	_title->setString(val.first);
	selected->setString(val.second);
	setTouchEnabled(true);
	addClickEventListener(std::bind(&tPreferenceItemKeyValPair::showInput, this, std::placeholders::_1));
}

const char* tPreferenceItemKeyValPair::getUIFileName() const  {
	return "ui/comctrl/SelectListItem.csb";
}

void tPreferenceItemKeyValPair::onPressStateChangedToNormal() {
	if (highlight) highlight->setVisible(false);
}

void tPreferenceItemKeyValPair::onPressStateChangedToPressed() {
	if (highlight) highlight->setVisible(true);
}

void tPreferenceItemKeyValPair::showInput(cocos2d::Ref*) {
	std::pair<std::string, std::string> val = _getter();
	TVPTextPairInputForm *form = TVPTextPairInputForm::create(val.first, val.second,
		[this](const std::string &t1, const std::string &t2){
		_title->setString(t1);
		selected->setString(t2);
		_setter(std::make_pair(t1, t2));
	});
	TVPMainScene::GetInstance()->pushUIForm(form, TVPMainScene::eEnterFromBottom);
}

void tPreferenceItemKeyValPair::updateText() {
	std::pair<std::string, std::string> val = _getter();
	if (_title) _title->setString(val.first + " =");
	if (selected) selected->setString(val.second);
}

TVPCustomPreferenceForm * TVPCustomPreferenceForm::create(const std::string &tid_title, int count,
	const std::function<std::pair<std::string, std::string>(int)> &getter,
	const std::function<void(int, const std::pair<std::string, std::string>&)> &setter) {
	TVPCustomPreferenceForm *ret = new TVPCustomPreferenceForm;
	ret->initFromFile("ui/NaviBar.csb", "ui/ListView.csb", nullptr);
	ret->initFromInfo(tid_title, count, getter, setter);
	ret->autorelease();
	return ret;
}

void TVPCustomPreferenceForm::bindBodyController(const NodeMap &allNodes) {
	_listview = static_cast<ListView*>(allNodes.findController("list"));
	if (NaviBar.Left) {
		NaviBar.Left->addClickEventListener([this](cocos2d::Ref*){
			TVPMainScene::GetInstance()->popUIForm(this);
		});
	}
}

void TVPCustomPreferenceForm::bindHeaderController(const NodeMap &allNodes)
{
	_title = static_cast<Button*>(allNodes.findController("title"));
	if (_title) _title->setEnabled(false);
}

void TVPCustomPreferenceForm::initFromInfo(const std::string &tid_title, int count,
	const std::function<std::pair<std::string, std::string>(int)> &getter,
	const std::function<void(int, const std::pair<std::string, std::string>&)> &setter) {
	_getter = getter;
	_setter = setter;
	if (_title) LocaleConfigManager::GetInstance()->initText(_title, tid_title);
	if (!_listview) return;
	_listview->removeAllItems();
	Size size = _listview->getContentSize();
	CSBReader reader;
	for (int i = 0; i < count; ++i) {
		tPreferenceItemKeyValPair *item = new tPreferenceItemKeyValPair;
		item->_getter = [=]()->std::pair<std::string, std::string>{return _getter(i); };
		item->_setter = [=](std::pair<std::string, std::string> val){
			if (val.first.empty() && val.second.empty()) return;
			_setter(i, val);
		};
		item->autorelease();
		item->initFromInfo(i, size, nullptr);
		_listview->pushBackCustomItem(item);
	}
	Widget *nullcell = new Widget();
	nullcell->setContentSize(Size(_listview->getContentSize().width, 200));
	_listview->pushBackCustomItem(nullcell);
}

void iPreferenceItemSlider::initController(const NodeMap &allNodes) {
	_slider = dynamic_cast<Slider*>(allNodes.findController("slider"));
	_reset = dynamic_cast<Button*>(allNodes.findController("reset"));
	if (_reset) {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		_reset->setTitleText(locmgr->GetText(_reset->getTitleText()));
	}

	float val = _getter();
	_slider->setPercent(val * 100.f);
	_slider->addTouchEventListener([this](Ref* p, Widget::TouchEventType e) {
		Slider* slider = static_cast<Slider*>(p);
		switch (e) {
		case Widget::TouchEventType::ENDED:
		case Widget::TouchEventType::CANCELED:
			_setter(slider->getPercent() / 100.f);
			break;
		default:
			break;
		}
	});
}

Sprite *TVPCreateCUR();
void tPreferenceItemCursorSlider::initController(const NodeMap &allNodes) {
	inherit::initController(allNodes);
	if (_reset) {
		_reset->addClickEventListener([this](Ref*){
			_slider->setPercent(_resetValue * 100.f);
			_icon->setScale(_curScaleConv(_resetValue));
			_setter(_resetValue);
		});
	}
	_icon = allNodes.findController("icon");
	_cursor = TVPCreateCUR();
	_icon->addChild(_cursor);
	_icon->setScale(_curScaleConv(_slider->getPercent() / 100.f));
	_slider->addEventListener([this](Ref* p, Slider::EventType e) {
		if (e == Slider::EventType::ON_PERCENTAGE_CHANGED) {
			Slider* slider = static_cast<Slider*>(p);
			_icon->setScale(_curScaleConv(slider->getPercent() / 100.f));
		}
	});
}

const char* tPreferenceItemCursorSlider::getUIFileName() const  {
	return "ui/comctrl/SliderIconItem.csb";
}

void tPreferenceItemCursorSlider::onEnter() {
	inherit::onEnter();
	float scale = 1;
	for (Node *p = _icon->getParent(); p; p = p->getParent()) {
		scale *= p->getScale();
	}
	_cursor->setScale(_cursor->getScale() / scale);
}

const char* tPreferenceItemTextSlider::getUIFileName() const  {
	return "ui/comctrl/SliderTextItem.csb";
}

void tPreferenceItemTextSlider::initController(const NodeMap &allNodes) {
	inherit::initController(allNodes);
	_text = dynamic_cast<Text*>(allNodes.findController("text"));
	if (_reset) {
		_reset->addClickEventListener([this](Ref*){
			_slider->setPercent(_resetValue * 100.f);
			_text->setString(_strScaleConv(_resetValue));
			_setter(_resetValue);
		});
	}
	_text->setString(_strScaleConv(_slider->getPercent() / 100.f));
	_slider->addEventListener([this](Ref* p, Slider::EventType e) {
		if (e == Slider::EventType::ON_PERCENTAGE_CHANGED) {
			Slider* slider = static_cast<Slider*>(p);
			_text->setString(_strScaleConv(slider->getPercent() / 100.f));
		}
	});
}

void tPreferenceItemFileSelect::initController(const NodeMap &allNodes)
{
	highlight = allNodes.findController("highlight");
	selected = static_cast<Text*>(allNodes.findController("selected"));
	updateHightlight();
	setTouchEnabled(true);
	addClickEventListener(std::bind(&tPreferenceItemFileSelect::showForm, this, std::placeholders::_1));
}

const char* tPreferenceItemFileSelect::getUIFileName() const
{
	return "ui/comctrl/SelectListItem.csb";
}

void tPreferenceItemFileSelect::onPressStateChangedToNormal()
{
	if (highlight) highlight->setVisible(false);
}

void tPreferenceItemFileSelect::onPressStateChangedToPressed()
{
	if (highlight) highlight->setVisible(true);
}

void tPreferenceItemFileSelect::showForm(cocos2d::Ref*)
{
	std::string fullname = _getter();
	std::string initname, initdir;
	if (!fullname.empty()) {
		std::pair<std::string, std::string> path = TVPBaseFileSelectorForm::PathSplit(fullname);
		initdir = path.first;
		initname = path.second;
	}
	if (initdir.empty()) {
		initdir = TVPGetDriverPath()[0];
	}
	TVPFileSelectorForm *form = TVPFileSelectorForm::create(initname, initdir, false);
	form->setOnClose([this](const std::string& result) {
		_setter(result);
		updateHightlight();
	});
	TVPMainScene::GetInstance()->pushUIForm(form);
}

void tPreferenceItemFileSelect::updateHightlight()
{
	if (selected) selected->setString(_getter());
}

KeyMapPreferenceForm::KeyMapPreferenceForm(iSysConfigManager* mgr)
	: _mgr(mgr)
{

}

void KeyMapPreferenceForm::initData()
{
	LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
	PrefList->removeAllItems();
	locmgr->initText(_title, "preference_keymap_title");

	Size size = PrefList->getContentSize();
#if 0
	tPreferenceItemConstant* celladd = CreatePreferenceItem<tPreferenceItemConstant>(0, size, locmgr->GetText("preference_keymap_add"));
	celladd->setTouchEnabled(true);
	celladd->addClickEventListener([this](Ref*) {
		TVPKeyPairSelectForm *form = TVPKeyPairSelectForm::create([this](int k) {
			TVPKeyPairSelectForm *form = TVPKeyPairSelectForm::create([this, k](int v) {
				_mgr->SetKeyMap(k, v);
				initData();
			});
			TVPMainScene::GetInstance()->pushUIForm(form, TVPMainScene::eEnterFromBottom);
		});
		TVPMainScene::GetInstance()->pushUIForm(form, TVPMainScene::eEnterFromBottom);
	});
	PrefList->pushBackCustomItem(celladd);

	const auto& keymap = _mgr->GetKeyMap();
	int counter = 0;
	for (const std::pair<int, int>& it : keymap) {
		if (it.first && it.second) {
			tPreferenceItemKeyMap *cell = new tPreferenceItemKeyMap;
			cell->initData(it.first, it.second, ++counter, size);
			cell->_onDelete = [this](tPreferenceItemDeletable* cell) {
				auto &keypair = static_cast<tPreferenceItemKeyMap*>(cell)->_keypair;
				_mgr->SetKeyMap(keypair.first, 0);
			};
			PrefList->pushBackCustomItem(cell);
		}
	}
#endif
	Widget *nullcell = new Widget();
	nullcell->setContentSize(Size(PrefList->getContentSize().width, 200));
	PrefList->pushBackCustomItem(nullcell);
}

KeyMapPreferenceForm* KeyMapPreferenceForm::create(iSysConfigManager* mgr)
{
	KeyMapPreferenceForm *ret = new KeyMapPreferenceForm(mgr);
	ret->autorelease();
	ret->initFromFile("ui/NaviBar.csb", "ui/ListView.csb", nullptr);
	ret->initData();
	return ret;
}

void tPreferenceItemDeletable::initController(const NodeMap &allNodes)
{
	_deleteIcon = allNodes.findWidget("delete");
	_scrollview = allNodes.findController<cocos2d::ui::ScrollView>("scrollview");
	_scrollview->setScrollBarEnabled(false);
	Size viewSize = _scrollview->getContentSize();
	float iconWidth = _deleteIcon->getContentSize().width;
	viewSize.width += iconWidth;
	_scrollview->setInnerContainerSize(viewSize);
	walkTouchEvent(_scrollview);
	_deleteIcon->addTouchEventListener(nullptr);
	_deleteIcon->addClickEventListener([this](Ref*) {
		if (_onDelete) _onDelete(this);
	});
}

const char* tPreferenceItemDeletable::getUIFileName() const
{
	return "ui/comctrl/DeletableItem.csb";
}

void tPreferenceItemDeletable::onTouchEvent(cocos2d::Ref* p, cocos2d::ui::Widget::TouchEventType ev)
{

}

void tPreferenceItemDeletable::walkTouchEvent(Widget* node)
{
	node->addTouchEventListener(std::bind(&tPreferenceItemDeletable::onTouchEvent,
		this, std::placeholders::_1, std::placeholders::_2));
	auto &vecChildren = node->getChildren();
	for (Node *child : vecChildren) {
		Widget *widget = dynamic_cast<Widget*>(child);
		if (widget) {
			walkTouchEvent(widget);
		}
	}
}

void tPreferenceItemKeyMap::initData(int k, int v, int idx, const cocos2d::Size &size)
{
	_keypair.first = k; _keypair.second = v;
	char buf[32];
	sprintf(buf, "%d <=> %d", k, v);
	initFromInfo(idx, size, buf);
}

