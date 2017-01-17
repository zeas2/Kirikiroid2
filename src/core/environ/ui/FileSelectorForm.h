#pragma once
#include "BaseForm.h"
#include "GUI/CCScrollView/CCTableView.h"

class TVPListForm : public cocos2d::Node {
public:
	virtual ~TVPListForm();;
	static TVPListForm * create(const std::vector<cocos2d::ui::Widget*> &cells);

	void initFromInfo(const std::vector<cocos2d::ui::Widget*> &cells);

	void show(); // for background fading

	void close();

private:
	bool onMaskTouchBegan(cocos2d::Touch *t, cocos2d::Event *);

	cocos2d::Node *_root;
};

class TVPBaseFileSelectorForm : public iTVPBaseForm, public cocos2d::extension::TableViewDataSource {
public:
	TVPBaseFileSelectorForm();
	virtual ~TVPBaseFileSelectorForm();

	virtual cocos2d::Size tableCellSizeForIndex(cocos2d::extension::TableView *table, ssize_t idx) override;
	virtual cocos2d::extension::TableViewCell* tableCellAtIndex(cocos2d::extension::TableView *table, ssize_t idx) override;
	virtual ssize_t numberOfCellsInTableView(cocos2d::extension::TableView *table) override;
	virtual void onCellClicked(int idx);
	virtual void onCellLongPress(int idx);
	virtual void rearrangeLayout() override;
	static std::pair<std::string, std::string> PathSplit(const std::string &path);

protected:
	virtual void bindBodyController(const NodeMap &allNodes) override;
	virtual void bindHeaderController(const NodeMap &allNodes) override;

	void ListDir(std::string path);
	virtual void getShortCutDirList(std::vector<std::string> &pathlist);

	void onCellItemClicked(cocos2d::Ref *owner);
	void onTitleClicked(cocos2d::Ref *owner);
	void onBackClicked(cocos2d::Ref *owner);

	cocos2d::extension::TableView *FileList;
	cocos2d::ui::Button *_title;

	struct FileInfo {
		std::string FullPath;
		std::string NameForDisplay;
		std::string NameForCompare;
		bool IsDir;

		bool operator < (const FileInfo &rhs) const;
	};

	int RootPathLen = 1; // '/'
	std::vector<FileInfo> CurrentDirList;
	std::string ParentPath, CurrentPath;
	class FileItemCell;
	class FileItemCellImpl : public TTouchEventRouter {
	public:
		FileItemCellImpl() : _set(false), _owner(nullptr) {}
		
		static FileItemCellImpl *create(const char * filename, float width, const FileInfo &info) {
			FileItemCellImpl* ret = new FileItemCellImpl();
			ret->autorelease();
			ret->init();
			ret->initFromFile(filename, width);
			ret->setInfo(info);
			return ret;
		}

		void initFromFile(const char * filename, float width);

		void setInfo(const FileInfo &info);

		void reset() {
			_set = false;
		}

		bool isSet() {
			return _set;
		}

		void setOwner(FileItemCell* owner) {
			_owner = owner;
		}

	private:
		void onClicked(cocos2d::Ref*);

		bool _set;
		cocos2d::Size OrigCellModelSize, CellTextAreaSize, OrigCellTextSize;
		cocos2d::ui::Text *FileNameNode;
		cocos2d::Node *DirIcon, *_root;
		FileItemCell *_owner;
	} *CellTemplateForSize = nullptr;
	FileItemCellImpl* FetchCell(FileItemCellImpl* CellModel, cocos2d::extension::TableView *table, ssize_t idx);

	class FileItemCell : public cocos2d::extension::TableViewCell {
		typedef cocos2d::extension::TableViewCell inherit;

	public:
		FileItemCell(TVPBaseFileSelectorForm *owner) : _owner(owner), _impl(nullptr) {}

		static FileItemCell *create(TVPBaseFileSelectorForm *owner) {
			FileItemCell *ret = new FileItemCell(owner);
			ret->init();
			ret->autorelease();
			return ret;
		}

		// retained
		FileItemCellImpl* detach() {
			FileItemCellImpl *ret = _impl;
			if (ret) {
				_impl = nullptr;
				ret->retain();
				ret->removeFromParent();
				ret->reset();
			}
			return ret;
		}

		// release
		void attach(FileItemCellImpl *impl) {
			_impl = impl;
			addChild(impl);
			setContentSize(impl->getContentSize());
			impl->setOwner(this);
			impl->release();
		}

		void onClicked() {
			_owner->onCellClicked(getIdx());
		}

		void onLongPress() {
			_owner->onCellLongPress(getIdx());
		}

	private:
		FileItemCellImpl *_impl;
		TVPBaseFileSelectorForm *_owner;
	};
};

class TVPFileSelectorForm : public TVPBaseFileSelectorForm {
	typedef TVPBaseFileSelectorForm inherit;

public:
	static TVPFileSelectorForm *create(const std::string &initfilename, const std::string &initdir, bool issave);
	void initFromPath(const std::string &initfilename, const std::string &initdir, bool issave);
	void setOnClose(const std::function<void(const std::string &)> &func) { _funcOnClose = func; }

protected:
	virtual void bindFooterController(const NodeMap &allNodes) override;
	virtual void onCellClicked(int idx) override;
	void close();

	cocos2d::ui::Button *_buttonOK, *_buttonCancel;
	cocos2d::ui::TextField *_input;
	std::function<void(const std::string &)> _funcOnClose;
	std::string _result;
	bool _isSaveMode;
};
