#pragma once
#include "2d/CCNode.h"
#include <unordered_map>
#include "ui/UIWidget.h"
#include "extensions/GUI/CCScrollView/CCTableViewCell.h"

namespace cocos2d {
	class Ref;
	class Node;
	namespace ui {
		class Widget;
		class Button;
		class Text;
		class ListView;
		class CheckBox;
		class PageView;
		class TextField;
		class Slider;
		class ScrollView;
		class LoadingBar;
	}
	namespace extension {
		class TableView;
	}
}

namespace cocostudio {
	namespace timeline {
		class ActionTimeline;
	}
}

class NodeMap : public std::unordered_map<std::string, cocos2d::Node*> {
protected:
	const char *FileName;
	void onLoadError(const std::string &name) const;

public:
	NodeMap();
	NodeMap(const char *filename, cocos2d::Node* node);
	template<typename T = cocos2d::Node>
	T *findController(const std::string &name, bool notice = true) const {
		cocos2d::Node *node = findController<cocos2d::Node>(name, notice);
		if (node) {
			T *ret = dynamic_cast<T*>(node);
			if (!ret) {
				onLoadError(name);
			}
			return ret;
		}
		return nullptr;
	}
	cocos2d::ui::Widget *findWidget(const std::string &name, bool notice = true) const {
		return findController<cocos2d::ui::Widget>(name, notice);
	}
	void initFromNode(cocos2d::Node* node);
};
template<> cocos2d::Node *NodeMap::findController<cocos2d::Node>(const std::string &name, bool notice) const;

class CSBReader : public NodeMap {
public:
	cocos2d::Node* Load(const char *filename);
};

class iTVPBaseForm : public cocos2d::Node {
public:
	iTVPBaseForm()
		: RootNode(nullptr){}
	virtual ~iTVPBaseForm();

	void Show();

	virtual void rearrangeLayout();
	virtual void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

protected:
	bool initFromFile(const char *navibar, const char *body, const char *bottombar, cocos2d::Node *parent = nullptr);
	bool initFromFile(const char *body) {
		return initFromFile(nullptr, body, nullptr);
	}

	virtual void bindBodyController(const NodeMap &allNodes) {}
	virtual void bindFooterController(const NodeMap &allNodes) {}
	virtual void bindHeaderController(const NodeMap &allNodes) {}

	cocos2d::ui::Widget *RootNode;

	struct {
		//cocos2d::ui::Button *Title;
		cocos2d::ui::Button *Left;
		cocos2d::ui::Widget *Right;
		cocos2d::Node *Root;
	} NaviBar;

	struct {
		//cocos2d::ui::ListView *Panel;
		cocos2d::Node *Root;
	} BottomBar;
};

class TTouchEventRouter : public cocos2d::ui::Widget {
public:
	typedef std::function<void(cocos2d::ui::Widget::TouchEventType event,
		cocos2d::ui::Widget* sender, cocos2d::Touch *touch)> EventFunc;

	static TTouchEventRouter *create() {
		TTouchEventRouter * ret = new TTouchEventRouter;
		ret->init();
		ret->autorelease();
		return ret;
	}

	void setEventFunc(const EventFunc &func) {
		_func = func;
	}

	virtual void interceptTouchEvent(cocos2d::ui::Widget::TouchEventType event,
		cocos2d::ui::Widget* sender, cocos2d::Touch *touch) override {
		if (_func) _func(event, sender, touch);
	}

private:
	EventFunc _func;
};

class TCommonTableCell : public cocos2d::extension::TableViewCell {
	typedef cocos2d::extension::TableViewCell inherit;

protected: // must be inherited
	TCommonTableCell() : _router(nullptr) {}

public:
	virtual ~TCommonTableCell() {
		if (_router) _router->release();
	}

	virtual void setContentSize(const cocos2d::Size& contentSize) {
		inherit::setContentSize(contentSize);
		if (_router) _router->setContentSize(contentSize);
	}

	virtual bool init() {
		bool ret = inherit::init();
		_router = TTouchEventRouter::create();
		return ret;
	}

protected:
	TTouchEventRouter *_router;
};

class iTVPFloatForm : public iTVPBaseForm {
public:
	virtual void rearrangeLayout() override;
};

void ReloadTableViewAndKeepPos(cocos2d::extension::TableView *pTableView);
