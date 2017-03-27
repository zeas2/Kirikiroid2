#include "FileSelectorForm.h"
#include "StorageImpl.h"
#include "ui/UIListView.h"
#include "ui/UIHelper.h"
#include "ui/UIButton.h"
#include "ui/UIText.h"
#include "ui/UITextField.h"
#include "ui/UICheckBox.h"
#include "Platform.h"
#include "cocos2d/MainScene.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "CCFileUtils.h"
#include "base/CCDirector.h"
#include "MessageBox.h"
#include "platform/CCDevice.h"

using namespace cocos2d;
using namespace cocos2d::extension;
using namespace cocos2d::ui;

const char * const FileName_Cell = "ui/FileItem.csb";
static TVPListForm* _listform;
#define MOVE_INCH            7.0f/160.0f
static const std::string str_long_press("long_press");

static float convertDistanceFromPointToInch(const Vec2& dis)
{
	auto glview = cocos2d::Director::getInstance()->getOpenGLView();
	int dpi = cocos2d::Device::getDPI();
	float distance = Vec2(dis.x * glview->getScaleX() / dpi, dis.y * glview->getScaleY() / dpi).getLength();
	return distance;
}

static bool IsPathExist(const std::string &path) {
	tTVP_stat s;
	if (!TVP_stat(path.c_str(), s)) {
		return false; // not exist
	}
	return true;
}

std::pair<std::string, std::string> TVPBaseFileSelectorForm::PathSplit(const std::string &path) {
	std::pair<std::string, std::string> ret;
	if (path.size() <= 1) {
		ret.second = path;
		return ret;
	}
	for (auto it = path.end() - 1; it > path.begin(); --it) {
		switch (*it) {
		case '/': case '\\':
			if (it == path.end() - 1) {
				ret.second = path;
				return ret;
			}
			ret.first = std::string(path.begin(), it);
			ret.second = std::string(it + 1, path.end());
			while(!ret.first.empty()) {
				char c = ret.first.back();
				if (c == '/' || c == '\\') {
					ret.first.pop_back();
				} else {
					break;
				}
			}
#if CC_PLATFORM_WIN32 != CC_TARGET_PLATFORM && CC_PLATFORM_WINRT != CC_TARGET_PLATFORM && CC_PLATFORM_WP8 != CC_TARGET_PLATFORM
			if (ret.first.empty()) ret.first = "/"; // posix root
#endif
			return ret;
		default:
			break;
		}
	}
	ret.second = path;
	return ret;
}

TVPBaseFileSelectorForm::TVPBaseFileSelectorForm()
	: FileList(nullptr)
{
	std::vector<std::string> paths;
	getShortCutDirList(paths); // for init 
	std::vector<std::string> appPath = TVPGetAppStoragePath();
	if (appPath.empty() && paths.size() == 1) { // ios-like sandbox environment
		RootPathLen = paths.front().size();
	}
}

TVPBaseFileSelectorForm::~TVPBaseFileSelectorForm() {
	CC_SAFE_RELEASE_NULL(CellTemplateForSize);
}

void TVPBaseFileSelectorForm::bindHeaderController(const NodeMap &allNodes)
{
	_title = static_cast<Button*>(allNodes.findController("title"));
	if (_title) {
		_title->setEnabled(true);
		_title->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onTitleClicked, this, std::placeholders::_1));
	}
}

void TVPBaseFileSelectorForm::bindBodyController(const NodeMap &allNodes) {
	Node *TableNode = allNodes.findController("table");
	FileList = TableView::create(this, TableNode->getContentSize());
	FileList->setVerticalFillOrder(TableView::VerticalFillOrder::TOP_DOWN);
	FileList->setAnchorPoint(Vec2::ZERO);
	FileList->setClippingToBounds(false);
	TableNode->addChild(FileList);
// 	ListView::ccListViewCallback func = [this](Ref* cell, ListView::EventType e){
// 		if (e == ListView::EventType::ON_SELECTED_ITEM_END) {
// 			onCellClicked(static_cast<ListView*>(cell)->getCurSelectedIndex());
// 		}
// 	};
// 	FileList->addEventListener(func);

	if (NaviBar.Left) {
		NaviBar.Left->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onBackClicked, this, std::placeholders::_1));
	}
}

static const std::string _path_current(".");
static const std::string _path_parent("..");
static const std::string str_diricon("dir_icon");
static const std::string str_select("select_check");
static const std::string str_filename("filename");
void TVPBaseFileSelectorForm::ListDir(std::string path) {
	std::pair<std::string, std::string> split_path = PathSplit(path);
	ParentPath = split_path.first;
	if (_title) {
#if CC_PLATFORM_WIN32 == CC_TARGET_PLATFORM
		// for better screenshot
		_title->setTitleFontName("SIMHEI.ttf");
		if (!split_path.second.empty() && (split_path.second.back() == '/' || split_path.second.back() == '\\')) {
			split_path.second.pop_back();
		}
#endif
		_title->setTitleText(split_path.second);

		Size dispSize = _title->getTitleRenderer()->getContentSize();
		Size realSize = _title->getContentSize();
		if (dispSize.width > realSize.width) {
			const std::string suffix("...");
			_title->setTitleText(suffix);
			float suffixlen = _title->getTitleRenderer()->getContentSize().width * 1.5;
			float ratio = (realSize.width - suffixlen) / dispSize.width;
			ttstr path = split_path.second;
			int charCutCount = path.length() * ratio - suffix.size() + 1;
			path = path.SubString(0, charCutCount);
			_title->setTitleText(path.AsStdString() + suffix);
		}
	}

	if (path.size() > RootPathLen && path.back() == '/') {
		path.pop_back();
	}

	if (NaviBar.Left) {
		NaviBar.Left->setVisible(path.size() > RootPathLen);
	}

	CurrentDirList.clear(); CurrentDirList.reserve(16);
	TVPListDir(path, [&](const std::string &name, int mask) {
		if (mask & (S_IFREG | S_IFDIR)) {
			if (name.empty() || name.front() == '.') return;
			CurrentDirList.resize(CurrentDirList.size() + 1);
			FileInfo &info = CurrentDirList.back();
			info.NameForDisplay = name;
			info.NameForCompare = name;
			info.IsDir = mask & S_IFDIR;
			std::transform(info.NameForCompare.begin(), info.NameForCompare.end(), info.NameForCompare.begin(), [](int c)->int {
				if (c <= 'Z' && c >= 'A')
					return c - ('A' - 'a');
				return c;
			});
		}
	});
	// fill fullpath
	for (auto it = CurrentDirList.begin(); it != CurrentDirList.end(); ++it) {
		it->FullPath = path + "/" + it->NameForDisplay;
	}
	std::sort(CurrentDirList.begin(), CurrentDirList.end());

	// update
	bool keepPos = CurrentPath == path;
	CurrentPath = path;
	if (keepPos) {
		ReloadTableViewAndKeepPos(FileList);
	} else {
		FileList->reloadData();
	}

	if (!_selectedFileIndex.empty()) {
		_selectedFileIndex.clear();
		updateFileMenu();
	} else if (_clipboardForFileManager.size()) {
		updateFileMenu();
	}
}

void TVPBaseFileSelectorForm::onCellClicked(int idx) {
	const FileInfo &info = CurrentDirList[idx];
	if (info.IsDir) {
		ListDir(info.FullPath);
	}
}

void TVPBaseFileSelectorForm::onCellLongPress(int idx)
{
	if (_fileOperateMenuNode) { // full file function
		if (!_selectedFileIndex.empty()) {
			return;
		}
		if (!_fileOperateMenu) {
			CSBReader reader;
			_fileOperateMenu = reader.Load("ui/FileManageMenu.csb");
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("title"));
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("titleUnselect"));
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("titleView", false));
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("titleCopy"));
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("titleCut"));
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("titlePaste"));
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("titleUnpack", false));
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("titleRepack", false));
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("titleDelete"));
			LocaleConfigManager::GetInstance()->initText(reader.findController<Text>("titleSendTo"));
			_fileOperateMenulist = reader.findController<ListView>("list");
			_fileOperateCell_unselect = reader.findWidget("unselect");
			_fileOperateCell_view = reader.findWidget("view", false);
			_fileOperateCell_copy = reader.findWidget("copy");
			_fileOperateCell_cut = reader.findWidget("cut");
			_fileOperateCell_paste = reader.findWidget("paste");
			_fileOperateCell_delete = reader.findWidget("delete");
			_fileOperateCell_repack = reader.findWidget("repack", false);
			_fileOperateCell_unpack = reader.findWidget("unpack", false);
			_fileOperateCell_sendto = reader.findWidget("sendto");
			Widget *btn;;
			if ((btn = reader.findWidget("btnUnselect"))) {
				btn->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onUnselectClicked, this, std::placeholders::_1));
			}
			if ((btn = reader.findWidget("btnView", false))) {
				btn->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onViewClicked, this, std::placeholders::_1));
			}
			if ((btn = reader.findWidget("btnCopy"))) {
				btn->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onCopyClicked, this, std::placeholders::_1));
			}
			if ((btn = reader.findWidget("btnCut"))) {
				btn->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onCutClicked, this, std::placeholders::_1));
			}
			if ((btn = reader.findWidget("btnPaste"))) {
				btn->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onPasteClicked, this, std::placeholders::_1));
			}
			if ((btn = reader.findWidget("btnUnpack", false))) {
				btn->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onUnpackClicked, this, std::placeholders::_1));
			}
			if ((btn = reader.findWidget("btnRepack", false))) {
				btn->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onRepackClicked, this, std::placeholders::_1));
			}
			if ((btn = reader.findWidget("btnDelete"))) {
				btn->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onDeleteClicked, this, std::placeholders::_1));
			}
			if ((btn = reader.findWidget("btnSendTo"))) {
				btn->addClickEventListener(std::bind(&TVPBaseFileSelectorForm::onSendToClicked, this, std::placeholders::_1));
			}
			_fileOperateMenulist->removeAllItems();
			float scale = 1.5;
			_fileOperateMenu->setScale(1 / scale);
			_fileOperateMenu->setContentSize(_fileOperateMenuNode->getContentSize() * scale);
			_fileOperateMenu->setPosition(_fileOperateMenuNode->getPosition());
			_fileOperateMenuNode->getParent()->addChild(_fileOperateMenu);
			ui::Helper::doLayout(_fileOperateMenu);
			_fileOperateMenu->setVisible(false);
		}
		if (!_fileOperateMenu->isVisible()) {
			_fileOperateMenu->setVisible(true);
		//	_fileOperateMenu->setAnchorPoint(Vec2(1, 0));
		}

		FileList->setTouchEnabled(false); // trick to release all touches
		FileList->setTouchEnabled(true);
		_selectedFileIndex.clear();
		_selectedFileIndex.insert(idx);
		updateFileMenu();
		return;
	}
	// simple file manage
	LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
	const char *btnTitles[] = {
		localeMgr->GetText("delete").c_str(),
		localeMgr->GetText("cancel").c_str(),
	};
	int n = TVPShowSimpleMessageBox(
		localeMgr->GetText("file_operate_menu_text").c_str(),
		localeMgr->GetText("file_operate_menu_title").c_str(),
		sizeof(btnTitles) / sizeof(btnTitles[0]), btnTitles);
	const FileInfo &info = CurrentDirList[idx];
	switch (n) {
	case 0:
		_selectedFileIndex.insert(idx);
		onDeleteClicked(nullptr);
		_selectedFileIndex.clear();
		break;
	default:
		break;
	}
}

void TVPBaseFileSelectorForm::getShortCutDirList(std::vector<std::string> &pathlist) {
	std::vector<std::string> paths = TVPGetDriverPath();
	for (const std::string &path : paths) {
		pathlist.emplace_back(path);
	}
	std::vector<std::string> appPath = TVPGetAppStoragePath();
	for (auto path : appPath) {
		cocos2d::log("appPath: %s", path.c_str());
		pathlist.emplace_back(path);
	}
}

void TVPBaseFileSelectorForm::onTitleClicked(cocos2d::Ref *owner) {
	if (_listform) return;
	std::vector<std::string> paths;
	getShortCutDirList(paths);

	std::vector<Widget*> cells;
	std::vector<Button*> buttons;
	auto func = [this](cocos2d::Ref* node) {
		ListDir(static_cast<Button*>(node)->getCallbackName());
		TVPMainScene::GetInstance()->popUIForm(nullptr, TVPMainScene::eLeaveToBottom);
	};
	for (const std::string &path : paths) {
		CSBReader reader;
		Widget *cell = dynamic_cast<Widget*>(reader.Load("ui/ListItem.csb"));
		Button *item = dynamic_cast<Button*>(reader.findController("item"));
		item->setCallbackName(path);
		item->setTitleText(path);
		item->addClickEventListener(func);
		cells.emplace_back(cell);
		buttons.emplace_back(item);
	}
	_listform = TVPListForm::create(cells);
	_listform->show();
	// march all button's text in its width
	for (Button* btn : buttons) {
		Size dispSize = btn->getTitleRenderer()->getContentSize();
		Size realSize = btn->getContentSize();
		if (dispSize.width > realSize.width) {
			std::string text = btn->getTitleText();
			float ratio = realSize.width / dispSize.width;
			const std::string prefix("...");
			int charCutCount = text.size() * (1 - ratio) + prefix.size() + 1;
			btn->setTitleText(prefix + text.substr(charCutCount));
		}
	}
}

void TVPBaseFileSelectorForm::onBackClicked(cocos2d::Ref *owner) {
	ListDir(ParentPath);
}

TVPBaseFileSelectorForm::FileItemCellImpl* TVPBaseFileSelectorForm::FetchCell(FileItemCellImpl* CellModel, cocos2d::extension::TableView *table, ssize_t idx) {
	if (!CellModel) {
		CellModel = FileItemCellImpl::create(FileName_Cell, table->getViewSize().width);
		CellModel->setAnchorPoint(Vec2::ZERO);
		CellModel->setEventFunc([this](Widget::TouchEventType ev, Widget* sender, Touch *touch){
			Vec2 touchPoint = touch->getLocation();
			switch (ev) {
			case Widget::TouchEventType::BEGAN:
				FileList->onTouchBegan(touch, nullptr);
				break;
			case Widget::TouchEventType::MOVED:
				FileList->onTouchMoved(touch, nullptr);
				if (sender->isHighlighted() &&
					convertDistanceFromPointToInch(sender->getTouchBeganPosition() - touchPoint) > MOVE_INCH) {
					sender->setHighlighted(false);
				}
				break;

			case Widget::TouchEventType::CANCELED:
				FileList->onTouchCancelled(touch, nullptr);
				break;
			case Widget::TouchEventType::ENDED:
				FileList->onTouchEnded(touch, nullptr);
				break;
			}
		});
		CellModel->retain();
	}
	bool selected = _selectedFileIndex.find(idx) != _selectedFileIndex.end();
	CellModel->setInfo(CurrentDirList[idx], selected, !_selectedFileIndex.empty());
	return CellModel;
}

TableViewCell* TVPBaseFileSelectorForm::tableCellAtIndex(TableView *table, ssize_t idx) {
	TableViewCell *pCell = table->dequeueCell();
	FileItemCell *cell = nullptr;
	if (pCell) {
		cell = static_cast<FileItemCell *>(pCell);
	} else {
		cell = FileItemCell::create(this);
	}
	if ((size_t)idx >= CurrentDirList.size()) {
		cell->setVisible(false);
		return cell;
	}
	cell->setVisible(true);
	FileItemCellImpl *impl = cell->detach();
	cell->attach(FetchCell(impl, table, idx));
	return cell;
}

ssize_t TVPBaseFileSelectorForm::numberOfCellsInTableView(TableView *table) {
	return CurrentDirList.empty() ? 0 : CurrentDirList.size() + 1;
}

Size TVPBaseFileSelectorForm::tableCellSizeForIndex(TableView *table, ssize_t idx) {
	if ((size_t)idx >= CurrentDirList.size()) {
		return Size(table->getContentSize().width, 200);
	}
	FileInfo &info = CurrentDirList[idx];
	if (info.CellSize.width == 0.f) {
		if (!CellTemplateForSize) {
			CellTemplateForSize = FetchCell(nullptr, table, idx);
		} else {
			CellTemplateForSize->setInfo(CurrentDirList[idx], false, false);
		}
		info.CellSize = CellTemplateForSize->getContentSize();
	}
	return info.CellSize;
}

void TVPBaseFileSelectorForm::rearrangeLayout() {
	iTVPBaseForm::rearrangeLayout();
	if (FileList) FileList->setViewSize(FileList->getParent()->getContentSize());
}

void TVPBaseFileSelectorForm::onUnselectClicked(cocos2d::Ref *owner)
{
	clearFileMenu();
}

void TVPBaseFileSelectorForm::onViewClicked(cocos2d::Ref *owner)
{

}

void TVPBaseFileSelectorForm::onCopyClicked(cocos2d::Ref *owner)
{
	_clipboardForMoving = false;
	_clipboardForFileManager.clear();
	for (int idx : _selectedFileIndex) {
		_clipboardForFileManager.emplace_back(CurrentDirList[idx].FullPath);
	}
	_clipboardPath = CurrentPath;
	_selectedFileIndex.clear();
	updateFileMenu();
}

void TVPBaseFileSelectorForm::onCutClicked(cocos2d::Ref *owner)
{
	onCopyClicked(owner);
	_clipboardForMoving = true;
}

void TVPBaseFileSelectorForm::onPasteClicked(cocos2d::Ref *owner)
{
	// TODO progress bar
	auto func = _clipboardForMoving ? TVPRenameFile : TVPCopyFile;
	for (const std::string &path : _clipboardForFileManager) {
		auto split_path = PathSplit(path);
		std::string target = CurrentPath + "/" + split_path.second;
		if (IsPathExist(target)) {
			LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
			if (TVPShowSimpleMessageBoxYesNo(target, localeMgr->GetText("ensure_to_override_file")) != 0) {
				continue;
			}
		}
		if (!func(path, target)) {
			LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
			TVPShowSimpleMessageBox(target, localeMgr->GetText("file_operate_failed"));
			break;
		}
	}
	clearFileMenu();
	ListDir(CurrentPath);
}

void TVPBaseFileSelectorForm::onUnpackClicked(cocos2d::Ref *owner)
{

	clearFileMenu();
}

void TVPBaseFileSelectorForm::onRepackClicked(cocos2d::Ref *owner)
{

	clearFileMenu();
}

void TVPBaseFileSelectorForm::onDeleteClicked(cocos2d::Ref *owner)
{
	LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
	std::string content;
	if (_selectedFileIndex.size() == 1) {
		const FileInfo &info = CurrentDirList[*_selectedFileIndex.begin()];
		content = info.FullPath;
	} else {
		content = localeMgr->GetText("ensure_item_count");
		char tmp[64];
		snprintf(tmp, 64, content.c_str(), _selectedFileIndex.size());
		content = tmp;
	}
	if (TVPShowSimpleMessageBoxYesNo(content, localeMgr->GetText("ensure_to_delete_file")) == 0) {
		for (int idx : _selectedFileIndex) {
			TVPDeleteFile(CurrentDirList[idx].FullPath);
		}
		scheduleOnce([this](float) {
			ListDir(CurrentPath);
		}, 0, "refresh_path");
	}
}

void TVPBaseFileSelectorForm::onSendToClicked(cocos2d::Ref *owner)
{
	clearFileMenu();
}

void TVPBaseFileSelectorForm::updateFileMenu()
{
	ReloadTableViewAndKeepPos(FileList);
	if (_selectedFileIndex.empty() && _clipboardForFileManager.empty()) { // close menu
		if (_fileOperateMenu->isVisible()) {
			_fileOperateMenu->setVisible(false);
		//	_fileOperateMenu->setAnchorPoint(Vec2(0, 0));
		}
		return;
	}
	_fileOperateMenulist->removeAllItems();
	_fileOperateMenulist->pushBackCustomItem(_fileOperateCell_unselect.get());
	if (!_clipboardForFileManager.empty() && _clipboardPath != CurrentPath) {
		_fileOperateMenulist->pushBackCustomItem(_fileOperateCell_paste.get());
	}
	if (_selectedFileIndex.size() == 1) {
		if (_fileOperateCell_view) _fileOperateMenulist->pushBackCustomItem(_fileOperateCell_view.get());
	//	_fileOperateMenulist->pushBackCustomItem(_fileOperateCell_unpack.get());
	//	_fileOperateMenulist->pushBackCustomItem(_fileOperateCell_repack.get());
	//	_fileOperateMenulist->pushBackCustomItem(_fileOperateCell_sendto.get());
	}
	if (!_selectedFileIndex.empty()) {
		_fileOperateMenulist->pushBackCustomItem(_fileOperateCell_copy.get());
		_fileOperateMenulist->pushBackCustomItem(_fileOperateCell_cut.get());
		_fileOperateMenulist->pushBackCustomItem(_fileOperateCell_delete.get());
	}
}

void TVPBaseFileSelectorForm::clearFileMenu()
{
	_selectedFileIndex.clear();
	_clipboardForFileManager.clear();
	updateFileMenu();
}

void TVPBaseFileSelectorForm::_onCellClicked(int idx)
{
	if (_selectedFileIndex.empty())
		return onCellClicked(idx);
	auto it = _selectedFileIndex.find(idx);
	if (it == _selectedFileIndex.end()) {
		_selectedFileIndex.insert(idx);
	} else {
		_selectedFileIndex.erase(idx);
	}
	updateFileMenu();
}

bool TVPBaseFileSelectorForm::FileInfo::operator<(const FileInfo &rhs) const {
	if (IsDir != rhs.IsDir) return IsDir > rhs.IsDir;
	return NameForCompare < rhs.NameForCompare;
}

TVPListForm * TVPListForm::create(const std::vector<cocos2d::ui::Widget*> &cells) {
	TVPListForm *ret = new TVPListForm;
	ret->initFromInfo(cells);
	ret->autorelease();
	return ret;
}

void TVPListForm::initFromInfo(const std::vector<cocos2d::ui::Widget*> &cells) {
	init();
	float scale = TVPMainScene::GetInstance()->getUIScale();
	cocos2d::Size sceneSize = TVPMainScene::GetInstance()->getUINodeSize() / scale;
	setScale(scale);
	setContentSize(sceneSize);
	CSBReader reader;
	_root = reader.Load("ui/ListView.csb");
	ListView* listview = static_cast<ListView*>(reader.findController("list"));
	float height = 10;
	for (Widget* cell : cells) {
		height += cell->getContentSize().height;
	}
	_root->setAnchorPoint(Size(0.5, 0.5));
	_root->setPosition(sceneSize / 2);
	sceneSize.width *= 0.8f;
	sceneSize.height *= 0.8f;
	if (height < sceneSize.height * scale) {
		sceneSize.height = height;
	}
	_root->setContentSize(sceneSize);
	ui::Helper::doLayout(_root);
	float width = listview->getContentSize().width;
	for (Widget* cell : cells) {
		Size size = cell->getContentSize();
		size.width = width;
		cell->setContentSize(size);
		ui::Helper::doLayout(cell);
		listview->pushBackCustomItem(cell);
	}
	if (listview->getItems().back()->getBottomBoundary() < 0) {
		listview->setClippingEnabled(true);
	} else {
		listview->setBounceEnabled(false);
	}
	addChild(_root);
}

void TVPListForm::show() {
	TVPMainScene::setMaskLayTouchBegain(std::bind(&TVPListForm::onMaskTouchBegan, this, std::placeholders::_1, std::placeholders::_2));
	TVPMainScene::GetInstance()->pushUIForm(this, TVPMainScene::eEnterFromBottom);
}

bool TVPListForm::onMaskTouchBegan(cocos2d::Touch *t, cocos2d::Event *) {
	Rect rc;
	rc.size = getContentSize();
	if (rc.containsPoint(convertTouchToNodeSpace(t))) {
		TVPMainScene::GetInstance()->popUIForm(this, TVPMainScene::eLeaveToBottom);
		return true;
	}
	return false;
}

TVPListForm::~TVPListForm() {
	if (_listform == this) _listform = nullptr;
}

int TVPDrawSceneOnce(int interval);
void TVPProcessInputEvents();
std::string TVPShowFileSelector(const std::string &title, const std::string &initfilename, std::string initdir, bool issave)
{
	if (initdir.empty()) {
		ttstr dir = TVPGetAppPath();
		TVPGetLocalName(dir);
		initdir = dir.AsStdString();
	}
	std::string _fileSelectorResult;
	TVPFileSelectorForm* _fileSelector = TVPFileSelectorForm::create(initfilename, initdir, issave);
	_fileSelector->setOnClose([&](const std::string &result) {
		_fileSelectorResult = result;
		_fileSelector = nullptr;
	});
	TVPMainScene::GetInstance()->pushUIForm(_fileSelector);
	Director* director = Director::getInstance();
	while (_fileSelector) {
		TVPProcessInputEvents();
		int remain = TVPDrawSceneOnce(30); // 30 fps
		if (remain > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(remain));
		}
	}
	return _fileSelectorResult;
}

const char * const FileName_NaviBar = "ui/NaviBar.csb";
const char * const FileName_Body = "ui/TableView.csb";
const char * const FileName_BottomBar = "ui/BottomBarTextInput.csb";

TVPFileSelectorForm * TVPFileSelectorForm::create(const std::string &initfilename, const std::string &initdir, bool issave)
{
	TVPFileSelectorForm *ret = new TVPFileSelectorForm;
	ret->autorelease();
	ret->initFromPath(initfilename, initdir, issave);
	return ret;
}

void TVPFileSelectorForm::initFromPath(const std::string &initfilename, const std::string &initdir, bool issave) {
	_isSaveMode = issave;
	initFromFile(FileName_NaviBar, FileName_Body, FileName_BottomBar);
	_input->setString(initfilename);
	ListDir(initdir); // getCurrentDir()
}

void TVPFileSelectorForm::bindFooterController(const NodeMap &allNodes) {
	_buttonOK = static_cast<Button*>(allNodes.findController("ok"));
	_buttonCancel = static_cast<Button*>(allNodes.findController("cancel"));
	_input = static_cast<TextField*>(allNodes.findController("input"));
	
	LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
	localeMgr->initText(_buttonOK, "ok");
	localeMgr->initText(_buttonCancel, "cancel");
	_buttonOK->addClickEventListener([this](Ref*){
		_result = _input->getString();
		if (!_result.empty()) {
			_result = CurrentPath + "/" + _result;
			bool fileExist = FileUtils::getInstance()->isFileExist(_result);
			if (_isSaveMode) {
				if (fileExist) {
					LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
					TVPMessageBoxForm::showYesNo(localeMgr->GetText("ensure_to_override_file"), _result,
						[this](int result) {
						if (result == 0) { // yes
							close();
						}
					});
				} else {
					close();
				}
			} else { // read mode
				if (fileExist) {
					close();
				}
			}
		}
	});
	_buttonCancel->addClickEventListener([this](Ref*){
		_result.clear();
		close();
	});
}

void TVPFileSelectorForm::onCellClicked(int idx) {
	FileInfo info = CurrentDirList[idx];
	TVPBaseFileSelectorForm::onCellClicked(idx);
	if (info.IsDir) {
	} else {
		_input->setString(info.NameForDisplay);
	}
}

void TVPFileSelectorForm::close() {
	if (_funcOnClose) _funcOnClose(_result);
	TVPMainScene::GetInstance()->popUIForm(this);
}

void TVPBaseFileSelectorForm::FileItemCellImpl::initFromFile(const char * filename, float width) {
	CSBReader reader;
	_root = reader.Load(filename);
	addChild(_root);
	OrigCellModelSize = _root->getContentSize();
	OrigCellModelSize.width = width;
	_root->setContentSize(OrigCellModelSize);
	setContentSize(OrigCellModelSize);
	DirIcon = reader.findController(str_diricon);
	SelectBox = reader.findController<CheckBox>(str_select);
	SelectBox->setTouchEnabled(false);
	FileNameNode = static_cast<Text *>(reader.findController(str_filename));
	if (DirIcon && FileNameNode) {
		CellTextAreaSize = DirIcon->getContentSize();
		CellTextAreaSize.width = OrigCellModelSize.width - CellTextAreaSize.width;
		CellTextAreaSize.height = 0;
		OrigCellTextSize = FileNameNode->getContentSize();
#if CC_PLATFORM_WIN32 == CC_TARGET_PLATFORM
		FileNameNode->setFontName("SIMHEI.ttf");
#endif
	}
	static const std::string str_highlight("highlight");
	Widget *HighLight = static_cast<Widget *>(reader.findController(str_highlight));
	if (HighLight) {
		HighLight->addClickEventListener(std::bind(&FileItemCellImpl::onClicked, this, std::placeholders::_1));
		HighLight->addTouchEventListener([this](Ref* p, Widget::TouchEventType ev){
			Widget* sender = static_cast<Widget*>(p);
			switch (ev) {
			case Widget::TouchEventType::BEGAN:
				sender->scheduleOnce([this, sender](float){
					if (sender->isHighlighted()) {
						sender->scheduleOnce([this, sender](float){
							_owner->onLongPress();
							sender->setHighlighted(false);
						//	sender->interceptTouchEvent(TouchEventType::CANCELED, sender, touch);
						}, 0, "delay_call");
					}
				}, 1.0f, str_long_press);
				break;
			case Widget::TouchEventType::CANCELED:
				sender->unschedule(str_long_press);
				break;
			}
		});
	}
}

void TVPBaseFileSelectorForm::FileItemCellImpl::setInfo(const FileInfo &info, bool selected, bool showSelect) {
	if (FileNameNode) {
		FileNameNode->ignoreContentAdaptWithSize(true);
		FileNameNode->setTextAreaSize(CellTextAreaSize);
		FileNameNode->setString(info.NameForDisplay);
		Size size(OrigCellModelSize);
		size.height += FileNameNode->getContentSize().height - OrigCellTextSize.height;
		_root->setContentSize(size);
		setContentSize(size);
		FileNameNode->ignoreContentAdaptWithSize(false);
	}
	if (DirIcon) DirIcon->setVisible(info.IsDir && !showSelect);
	SelectBox->setVisible(showSelect);
	if (showSelect) SelectBox->setSelected(selected);
	ui::Helper::doLayout(_root);
	_set = true;
}

void TVPBaseFileSelectorForm::FileItemCellImpl::onClicked(cocos2d::Ref* p) {
	Widget* sender = static_cast<Widget*>(p);
	if (sender->isScheduled(str_long_press)) {
		sender->unschedule(str_long_press);
		_owner->onClicked();
	}
}
