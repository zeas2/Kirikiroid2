#include "TipsHelpForm.h"
#include "ui/UIHelper.h"
#include "ui/UIListView.h"
#include "cocos2d/MainScene.h"

using namespace cocos2d;
using namespace cocos2d::ui;

TVPTipsHelpForm* TVPTipsHelpForm::create() {
	TVPTipsHelpForm *ret = new TVPTipsHelpForm;
	ret->initFromFile(nullptr, "ui/help/AllTips.csb", nullptr);
	ret->autorelease();
	return ret;
}

TVPTipsHelpForm* TVPTipsHelpForm::show(const char *tipName) {
	TVPTipsHelpForm *ui = create();
	if (tipName) ui->setOneTip(tipName);
	TVPMainScene::GetInstance()->pushUIForm(ui, TVPMainScene::eEnterAniOverFromRight);
	return ui;
}

void TVPTipsHelpForm::setOneTip(const std::string &tipName) {
	auto &allCell = _tipslist->getItems();
	int cellCount = allCell.size();
	while (!_tipslist->getItems().empty()) {
		Node *cell = _tipslist->getItem(_tipslist->getItems().size() - 1);
		if (cell->getName() == tipName) {
			break;
		} else {
			_tipslist->removeLastItem();
		}
	}
	while (!_tipslist->getItems().empty()) {
		Node *cell = _tipslist->getItem(0);
		if (cell->getName() == tipName) {
			break;
		} else {
			_tipslist->removeItem(0);
		}
	}
}

void TVPTipsHelpForm::rearrangeLayout() {
	Size sceneSize = TVPMainScene::GetInstance()->getUINodeSize();
	Size rootSize = RootNode->getContentSize();
	float scale = sceneSize.width / rootSize.width;
	rootSize.height = rootSize.width * sceneSize.height / sceneSize.width;
	setContentSize(rootSize);
	setScale(scale);
	RootNode->setContentSize(rootSize);
	ui::Helper::doLayout(RootNode);
}

void TVPTipsHelpForm::bindBodyController(const NodeMap &allNodes) {
	_tipslist = dynamic_cast<ListView*>(allNodes.findController("tipslist"));
	Widget *btn_close = allNodes.findWidget("btn_close");
	btn_close->addClickEventListener([this](Ref* p){
		static_cast<Widget*>(p)->setEnabled(false);
		TVPMainScene::GetInstance()->popUIForm(this);
	});
	Widget *nullcell = new Widget();
	nullcell->setContentSize(Size(_tipslist->getContentSize().width, 200));
	_tipslist->pushBackCustomItem(nullcell);
}

