#include "XP3RepackForm.h"
#include "StorageImpl.h"
#include "cocos2d/MainScene.h"
#include "PreferenceForm.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "XP3ArchiveRepack.h"
#include "ui/UIListView.h"
#include "ui/UILoadingBar.h"
#include "ui/UIText.h"
#include "ui/UIButton.h"
#include "ui/UICheckBox.h"
#include "Platform.h"
#include "MessageBox.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "TickCount.h"
#include "2d/CCLayer.h"
#include "2d/CCActionInterval.h"
#include "platform/CCFileUtils.h"
#include "platform/CCDevice.h"

using namespace cocos2d;
using namespace cocos2d::ui;
bool TVPGetXP3ArchiveOffset(tTJSBinaryStream *st, const ttstr name, tjs_uint64 & offset, bool raise);

static void WalkDir(const ttstr &dir, const std::function<void(const ttstr&, tjs_uint64)>& cb) {
	std::vector<ttstr> subdirs;
	std::vector<std::pair<std::string, tjs_uint64> > files;
	TVPGetLocalFileListAt(dir, [&](const ttstr& path, tTVPLocalFileInfo* info) {
		if (info->Mode & S_IFDIR) {
			subdirs.emplace_back(path);
		} else {
			files.emplace_back(info->NativeName, info->Size);
		}
	});
	ttstr prefix = dir + TJS_W("/");

	for (std::pair<std::string, tjs_uint64> &it : files) {
		cb(prefix + it.first, it.second);
	}
	for (ttstr &path : subdirs) {
		WalkDir(prefix + path, cb);
	}
}

class TVPXP3Repacker {
	const int UpdateMS = 100; // update rate 10 fps
public:
	TVPXP3Repacker(const std::string &rootdir) : RootDir(rootdir) {}
	~TVPXP3Repacker();
	void Start(std::vector<std::string> &filelist, const std::string &xp3filter);
	void SetOption(const std::string &name, bool v) {
		ArcRepacker.SetOption(name, v);
	}

private:
	void OnNewFile(int idx, uint64_t size, const std::string &filename);
	void OnNewArchive(int idx, uint64_t size, const std::string &filename);
	void OnProgress(uint64_t total_size, uint64_t arc_size, uint64_t file_size);
	void OnError(int errcode, const std::string &errmsg);
	void OnEnded();

	TVPSimpleProgressForm *ProgressForm = nullptr;
	XP3ArchiveRepackAsync ArcRepacker;

	std::string RootDir;
	std::string CurrentFileName, CurrentArcName;
	uint64_t TotalSize, CurrentFileSize, CurrentArcSize;
	int CurrentFileIndex;
	tjs_uint32 LastUpdate = 0;
};

TVPXP3Repacker::~TVPXP3Repacker()
{
	if (ProgressForm) {
		TVPMainScene::GetInstance()->popUIForm(ProgressForm, TVPMainScene::eLeaveAniNone);
		ProgressForm = nullptr;
	}
	cocos2d::Device::setKeepScreenOn(false);
}

void TVPXP3Repacker::Start(std::vector<std::string> &filelist, const std::string &xp3filter)
{
	if (!ProgressForm) {
		ProgressForm = TVPSimpleProgressForm::create();
		TVPMainScene::GetInstance()->pushUIForm(ProgressForm, TVPMainScene::eEnterAniNone);
		std::vector<std::pair<std::string, std::function<void(cocos2d::Ref*)> > > vecButtons;
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		vecButtons.emplace_back(locmgr->GetText("stop"), [this](Ref*) {
			ArcRepacker.Stop();
		});
		ProgressForm->initButtons(vecButtons);
		ProgressForm->setTitle(locmgr->GetText("archive_repack_proc_title"));
		ProgressForm->setPercentOnly(0);
		ProgressForm->setPercentOnly2(0);
		ProgressForm->setPercentText("");
		ProgressForm->setPercentText2("");
		ProgressForm->setContent("");
	}
	ArcRepacker.SetCallback(
		std::bind(&TVPXP3Repacker::OnNewFile, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&TVPXP3Repacker::OnNewArchive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&TVPXP3Repacker::OnProgress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&TVPXP3Repacker::OnError, this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&TVPXP3Repacker::OnEnded, this));
	if (cocos2d::FileUtils::getInstance()->isFileExist(xp3filter)) {
		ArcRepacker.SetXP3Filter(xp3filter);
	}
	TotalSize = 0;
	for (const std::string &name : filelist) {
		TotalSize += ArcRepacker.AddTask(name);
	}
	cocos2d::Device::setKeepScreenOn(true);
	ArcRepacker.Start();
}

void TVPXP3Repacker::OnNewFile(int idx, uint64_t size, const std::string &filename)
{
	tjs_uint32 tick = TVPGetRoughTickCount32();
	if ((int)(tick - LastUpdate) < UpdateMS && size < 1024 * 1024) {
		return;
	}
	CurrentFileIndex = idx;
	Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, size, filename] {
		CurrentFileSize = size;
		ProgressForm->setContent(CurrentArcName + ">" + filename);
	});
}

void TVPXP3Repacker::OnNewArchive(int idx, uint64_t size, const std::string &filename)
{
	int prefixlen = RootDir.length() + 1;
	std::string name = filename.substr(prefixlen);
	Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, name, size] {
		CurrentArcSize = size;
		CurrentArcName = name;
	});
}

void TVPXP3Repacker::OnProgress(uint64_t total_size, uint64_t arc_size, uint64_t file_size)
{
	tjs_uint32 tick = TVPGetRoughTickCount32();
	if ((int)(tick - LastUpdate) < UpdateMS) {
		return;
	}

	LastUpdate = tick;
	Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, total_size, arc_size] {
		ProgressForm->setPercentOnly2((float)total_size / TotalSize);
		ProgressForm->setPercentOnly((float)arc_size / CurrentArcSize);
		char buf[64];
		int sizeMB = static_cast<int>(total_size / (1024 * 1024)),
			totalMB = static_cast<int>(TotalSize / (1024 * 1024));
		sprintf(buf, "%d / %dMB", sizeMB, totalMB);
		ProgressForm->setPercentText2(buf);
		sizeMB = static_cast<int>(arc_size / (1024 * 1024));
		totalMB = static_cast<int>(CurrentArcSize / (1024 * 1024));
		sprintf(buf, "%d / %dMB", sizeMB, totalMB);
		ProgressForm->setPercentText(buf);
	});
}

void TVPXP3Repacker::OnError(int errcode, const std::string &errmsg)
{
	if (errcode < 0) {
		Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, errcode, errmsg] {
			char buf[64];
			sprintf(buf, "Error %d\n", errcode);
			ttstr strmsg(buf);
			strmsg += errmsg;
			TVPShowSimpleMessageBox(strmsg, TJS_W("XP3Repack Error"));
		});
	}
}

void TVPXP3Repacker::OnEnded()
{
	Director::getInstance()->getScheduler()->performFunctionInCocosThread([this] {
		delete this;
	});
}

class TVPXP3RepackFileListForm : public iTVPBaseForm {
public:
	virtual ~TVPXP3RepackFileListForm();
	virtual void bindBodyController(const NodeMap &allNodes);
	static TVPXP3RepackFileListForm* show(std::vector<std::string> &filelist, const std::string &dir);
	void initData(std::vector<std::string> &filelist, const std::string &dir);
	void close();

private:
	void onOkClicked(Ref*);

	cocos2d::ui::ListView *ListViewFiles, *ListViewPref;

	std::string RootDir;
	std::vector<std::string> FileList;

	TVPXP3Repacker *m_pRepacker = nullptr;
};

TVPXP3RepackFileListForm::~TVPXP3RepackFileListForm()
{
	if (m_pRepacker)
		delete m_pRepacker;
}

void TVPXP3RepackFileListForm::bindBodyController(const NodeMap &allNodes)
{
	LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
	ui::ScrollView *btnList = allNodes.findController<ui::ScrollView>("btn_list");
	Size containerSize = btnList->getContentSize();
	btnList->setInnerContainerSize(containerSize);
	Widget *btnCell = allNodes.findWidget("btn_cell");
	Button *btn = allNodes.findController<Button>("btn");
	int nButton = 2;

	locmgr->initText(allNodes.findController<Text>("title"), "XP3 Repack");

	btn->setTitleText(locmgr->GetText("start"));
	btn->addClickEventListener(std::bind(&TVPXP3RepackFileListForm::onOkClicked, this, std::placeholders::_1));
	btnCell->setPositionX(containerSize.width / (nButton + 1) * 1);
	btnList->addChild(btnCell->clone());

	btn->setTitleText(locmgr->GetText("cancel"));
	btn->addClickEventListener([this](Ref*) { close(); });
	btnCell->setPositionX(containerSize.width / (nButton + 1) * 2);
	btnList->addChild(btnCell->clone());

	btn->removeFromParent();

	ListViewFiles = allNodes.findController<ListView>("list_2");
	ListViewPref = allNodes.findController<ListView>("list_1");
}

TVPXP3RepackFileListForm* TVPXP3RepackFileListForm::show(std::vector<std::string> &filelist, const std::string &dir)
{
	TVPXP3RepackFileListForm *form = new TVPXP3RepackFileListForm;
	form->initFromFile("ui/CheckListDialog.csb");
	form->initData(filelist, dir);
	TVPMainScene::GetInstance()->pushUIForm(form, TVPMainScene::eEnterAniNone);
	return form;
}

void TVPXP3RepackFileListForm::initData(std::vector<std::string> &filelist, const std::string &dir)
{
	LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();

	RootDir = dir;
	FileList.swap(filelist);
	int tag = 256;
	Size size = ListViewFiles->getContentSize();
	int prefixlen = dir.length() + 1;
	for (size_t i = 0; i < FileList.size(); ++i) {
		std::string filename = FileList[i].substr(prefixlen);
		tPreferenceItemCheckBox *cell = CreatePreferenceItem<tPreferenceItemCheckBox>(i, size, filename,
			[](tPreferenceItemCheckBox* p) {
			p->_getter = []()->bool { return true; };
			p->_setter = [](bool v) {};
		});
		cell->setTag(tag++);
		ListViewFiles->pushBackCustomItem(cell);
	}

	m_pRepacker = new TVPXP3Repacker(RootDir);

	size = ListViewPref->getContentSize();
	ListViewPref->pushBackCustomItem(CreatePreferenceItem
		<tPreferenceItemConstant>(0, size, locmgr->GetText("archive_repack_desc")));

	ListViewPref->pushBackCustomItem(CreatePreferenceItem
		<tPreferenceItemCheckBox>(1, size, locmgr->GetText("archive_repack_merge_img"),
			[this](tPreferenceItemCheckBox* p) {
		p->_getter = []()->bool { return true; };
		p->_setter = [this](bool v) { m_pRepacker->SetOption("merge_mask_img", v); };
	}));

	ListViewPref->pushBackCustomItem(CreatePreferenceItem
		<tPreferenceItemCheckBox>(2, size, locmgr->GetText("archive_repack_conv_etc2"),
			[this](tPreferenceItemCheckBox* p) {
		p->_getter = []()->bool { return false; };
		p->_setter = [this](bool v) { m_pRepacker->SetOption("conv_etc2", v); };
	}));
}

void TVPXP3RepackFileListForm::close()
{
//	TVPMainScene::GetInstance()->popUIForm(this, TVPMainScene::eLeaveAniNone);
	removeFromParent();
}

void TVPXP3RepackFileListForm::onOkClicked(Ref*)
{
	class HackPreferenceItemCheckBox : public tPreferenceItemCheckBox {
	public:
		bool getCheckBoxState() {
			return checkbox->isSelected();
		}
	};
	std::vector<std::string> filelist; filelist.reserve(FileList.size());
	auto &allCell = ListViewFiles->getItems();
	for (int i = 0; i < allCell.size(); ++i) {
		Widget *cell = allCell.at(i);
		if (static_cast<HackPreferenceItemCheckBox*>(cell)->getCheckBoxState()) {
			filelist.push_back(FileList[cell->getTag() - 256]);
		}
	}
	if (filelist.empty())
		return;
	m_pRepacker->Start(filelist, RootDir + "/xp3filter.tjs");
	m_pRepacker = nullptr;
	close();
}

void TVPProcessXP3Repack(const std::string &dir)
{
	std::vector<std::string> filelist;
	bool hasXp3Filter = false;
	TVPGetLocalFileListAt(dir, [&](const ttstr& path, tTVPLocalFileInfo* info) {
		if (info->Mode & S_IFDIR) {
		} else if(path == TJS_W("xp3filter.tjs")) {
			hasXp3Filter = true;
		}
	});
	LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
	if (!hasXp3Filter) {
		if (TVPShowSimpleMessageBoxYesNo(locmgr->GetText("archive_repack_no_xp3filter"), locmgr->GetText("notice")) != 0) {
			return;
		}
	}
	WalkDir(dir, [&](const ttstr& strpath, tjs_uint64 size) {
		if (size < 32) return;
		tjs_uint64 offset;
		tTVPLocalFileStream *st = new tTVPLocalFileStream(strpath, strpath, TJS_BS_READ);
		if (TVPGetXP3ArchiveOffset(st, strpath, offset, false)) {
			filelist.emplace_back(strpath.AsStdString());
		}
		delete st;
	});
	if (filelist.empty()) {
		TVPShowSimpleMessageBox(locmgr->GetText("archive_repack_no_xp3").c_str(), "XP3Repack", 0, nullptr);
	} else {
		TVPXP3RepackFileListForm::show(filelist, dir);
	}
}
