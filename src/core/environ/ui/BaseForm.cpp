#include "BaseForm.h"
#include "cocos2d.h"
#include "cocostudio/ActionTimeline/CSLoader.h"
#include "Application.h"
#include "ui/UIWidget.h"
#include "cocos2d/MainScene.h"
#include "ui/UIHelper.h"
#include "ui/UIText.h"
#include "ui/UIButton.h"
#include "ui/UIListView.h"
#include "Platform.h"
#include "cocostudio/ActionTimeline/CCActionTimeline.h"

using namespace cocos2d;
using namespace cocos2d::ui;

NodeMap::NodeMap() : FileName(nullptr) { }
NodeMap::NodeMap(const char *filename, cocos2d::Node* node) : FileName(filename) {
	initFromNode(node);
}

cocos2d::Node * NodeMap::findController(const std::string &name, bool notice) const {
	auto it = this->find(name);
	if (it != this->end())
		return it->second;
	if (notice) {
		std::string warntext("Node ");
		warntext += name.c_str();
		warntext += " not exist in ";
		warntext += FileName;
		const char *btn = "OK";
		TVPShowSimpleMessageBox(warntext.c_str(), "Fail to load ui", 1, &btn);
	}
	return nullptr;
}

void NodeMap::initFromNode(cocos2d::Node* node) {
	const Vector<Node*>& childlist = node->getChildren();
	for (auto it = childlist.begin(); it != childlist.end(); ++it) {
		Node *child = *it; std::string name = child->getName();
		if (!name.empty()) (*this)[name] = child;
		initFromNode(child);
	}
}

cocos2d::ui::Widget * NodeMap::findWidget(const std::string &name) const {
	return static_cast<Widget*>(findController(name));
}

Node* CSBReader::Load(const char *filename) {
	clear();
	FileName = filename;
	Node* ret = CSLoader::createNode(filename, [this](Ref* p){
		Node* node = static_cast<Node*>(p);
		std::string name = node->getName();
		if (!name.empty()) operator[](name) = node;
		int nAction = node->getNumberOfRunningActions();
		if (nAction == 1) {
			cocostudio::timeline::ActionTimeline* action =
				dynamic_cast<cocostudio::timeline::ActionTimeline*>(node->getActionByTag(node->getTag()));
			if (action && action->IsAnimationInfoExists("autoplay")) {
				action->play("autoplay", true);
			}
		}
	});
	if (!ret) {
		TVPShowSimpleMessageBox(filename, "Fail to load ui file");
	}
	return ret;
}

iTVPBaseForm::~iTVPBaseForm() {

}

void iTVPBaseForm::Show() {

}

bool iTVPBaseForm::initFromFile(const char *navibar, const char *body, const char *bottombar, cocos2d::Node *parent) {
	bool ret = cocos2d::Node::init();
	//NaviBar.Title = nullptr;
	NaviBar.Left = nullptr;
	NaviBar.Right = nullptr;
	NaviBar.Root = nullptr;
	CSBReader reader;
	if (navibar) {
		NaviBar.Root = reader.Load(navibar);
		if (!NaviBar.Root) {
			return false;
		}
// 		NaviBar.Title = static_cast<Button*>(reader.findController("title"));
// 		if (NaviBar.Title) {
// 			NaviBar.Title->setEnabled(false); // normally
// 		}
		NaviBar.Left = static_cast<Button*>(reader.findController("left", false));
		NaviBar.Right = static_cast<Widget*>(reader.findController("right", false));
		bindHeaderController(reader);
	}

	BottomBar.Root = nullptr;
	//BottomBar.Panel = nullptr;
	if (bottombar) {
		BottomBar.Root = reader.Load(bottombar);
		if (!BottomBar.Root) {
			return false;
		}
		//BottomBar.Panel = static_cast<ListView*>(reader.findController("panel"));
		bindFooterController(reader);
	}
	RootNode = static_cast<Widget*>(reader.Load(body));
	if (!RootNode) {
		return false;
	}
	if (!parent) {
		parent = this;
	}
	parent->addChild(RootNode);
	if (NaviBar.Root) parent->addChild(NaviBar.Root);
	if (BottomBar.Root) parent->addChild(BottomBar.Root);
	rearrangeLayout();
	bindBodyController(reader);
	return ret;
}

void iTVPBaseForm::rearrangeLayout() {
	float scale = TVPMainScene::GetInstance()->getUIScale();
	Size sceneSize = TVPMainScene::GetInstance()->getUINodeSize();
	setContentSize(sceneSize);
	Size bodySize = RootNode->getParent()->getContentSize();
	if (NaviBar.Root) {
		Size size = NaviBar.Root->getContentSize();
		size.width = bodySize.width / scale;
		NaviBar.Root->setContentSize(size);
		NaviBar.Root->setScale(scale);
		ui::Helper::doLayout(NaviBar.Root);
		size.height *= scale;
		bodySize.height -= size.height;
		NaviBar.Root->setPosition(0, bodySize.height);
	}
	if (BottomBar.Root) {
		Size size = BottomBar.Root->getContentSize();
		size.width = bodySize.width / scale;
		BottomBar.Root->setContentSize(size);
		BottomBar.Root->setScale(scale);
		ui::Helper::doLayout(BottomBar.Root);
		size.height *= scale;
		bodySize.height -= size.height;
		BottomBar.Root->setPosition(Vec2::ZERO);
	}
	if (RootNode) {
		bodySize.height /= scale;
		bodySize.width /= scale;
		RootNode->setContentSize(bodySize);
		RootNode->setScale(scale);
		ui::Helper::doLayout(RootNode);
		if (BottomBar.Root) RootNode->setPosition(Vec2(0, BottomBar.Root->getContentSize().height * scale));
	}
}

void iTVPBaseForm::onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event) {
	if (keyCode == cocos2d::EventKeyboard::KeyCode::KEY_BACK) {
		TVPMainScene::GetInstance()->popUIForm(this, TVPMainScene::eLeaveAniLeaveFromLeft);
	}
}

// void iTVPBaseForm::initBottomBar(std::vector<std::pair<std::string, std::function<void()> > > args) {
// 	if (!BottomBar.Panel) return;
// 	BottomBar.Panel->removeAllItems();
// 	CSBReader reader;
// 	for (auto it : args) {
// 		Widget *root = static_cast<Widget*>(reader.Load(it.first.c_str()));
// 		Widget *btn = static_cast<Widget*>(reader.findController("button"));
// 		std::function<void()> func = it.second;
// 		if (btn) btn->addClickEventListener([=](cocos2d::Ref*){ func(); });
// 		BottomBar.Panel->pushBackCustomItem(root);
// 	}
// }

#if CC_PLATFORM_WIN32 == CC_TARGET_PLATFORM
#include "ui/UIEditBox/UIEditBox.h"
#include "ui/UIEditBox/UIEditBoxImpl-win32.h"

static bool _isModaling;

int TVPShowSimpleInputBox(ttstr &text, const ttstr &caption, const ttstr &prompt, const std::vector<ttstr> &vecButtons) {
	Scale9Sprite *bg = Scale9Sprite::create();
	EditBox *box = EditBox::create(Size(1, 1), bg);
	box->setInputFlag(EditBox::InputFlag::SENSITIVE);
	box->setText(text.AsStdString().c_str());
	box->setPlaceHolder(prompt.AsStdString().c_str());
	class Delegate : public EditBoxDelegate {
		virtual void editBoxReturn(EditBox* editBox) {
			_isModaling = false;
		}
	};

	Delegate* del = new Delegate;
	box->setDelegate(del);

	class HackForEditBox : public EditBox {
	public:
		EditBoxImpl* getImpl() { return _editBoxImpl; }
	};
	EditBoxImplWin *impl = static_cast<EditBoxImplWin*>(static_cast<HackForEditBox*>(box)->getImpl());

	box->touchDownAction(nullptr, Widget::TouchEventType::ENDED);

	_isModaling = true;

	while (_isModaling)
	{
		MSG msg;
		if (!GetMessage(&msg, NULL, 0, 0))
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	text = box->getText();
	box->setDelegate(nullptr);
	delete del;

	return 0;
}
#endif
