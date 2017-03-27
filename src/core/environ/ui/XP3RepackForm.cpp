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

using namespace cocos2d;
using namespace cocos2d::ui;
bool TVPGetXP3ArchiveOffset(tTJSBinaryStream *st, const ttstr name, tjs_uint64 & offset, bool raise);

static void WalkDir(const ttstr &dir, const std::function<void(const std::string&, tjs_uint64)>& cb) {
	std::vector<ttstr> subdirs;
	std::vector<std::pair<std::string, tjs_uint64> > files;
	TVPGetLocalFileListAt(dir, [&](const ttstr& path, tTVPLocalFileInfo* info) {
		if (info->Mode & S_IFDIR) {
			subdirs.emplace_back(path);
		} else {
			files.emplace_back(info->NativeName, info->Size);
		}
	});
	for (std::pair<std::string, tjs_uint64> &it : files) {
		cb(it.first, it.second);
	}
	for (ttstr &path : subdirs) {
		WalkDir(path, cb);
	}
}

class TVPXP3RepackForm : public iTVPFloatForm {
public:
	virtual void bindBodyController(const NodeMap &allNodes);
	static TVPXP3RepackForm* show();
	void initData(std::vector<std::string> &filelist, const std::string &xp3filter);

private:
	cocos2d::ui::LoadingBar *ProgressBarTotal, *ProgressBarCurrent;
	cocos2d::ui::Text *TextTotalProgress, *TextCurrentProgress; // "%d/%d(%d/%d MB)"
	cocos2d::ui::Text *TextCurrentFileName, *TextSpeed; // "~XXX MB/s"

	XP3ArchiveRepackAsync *Packer = nullptr;

	uint64_t TotalSize;
};

void TVPXP3RepackForm::bindBodyController(const NodeMap &allNodes)
{
	LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
	ProgressBarTotal =
		static_cast<cocos2d::ui::LoadingBar*>(allNodes.findController("bar_total"));
	ProgressBarCurrent =
		static_cast<cocos2d::ui::LoadingBar*>(allNodes.findController("bar_current"));
	TextTotalProgress =
		static_cast<cocos2d::ui::Text*>(allNodes.findController("text_total"));
	TextCurrentProgress =
		static_cast<cocos2d::ui::Text*>(allNodes.findController("text_current"));
	TextCurrentFileName =
		static_cast<cocos2d::ui::Text*>(allNodes.findController("text_filename"));
	TextSpeed =
		static_cast<cocos2d::ui::Text*>(allNodes.findController("text_speed"));
	Text* text = static_cast<Text*>(allNodes.findController("title"));
	if (text) {
		text->setString(locmgr->GetText(text->getString()));
	}
}

TVPXP3RepackForm* TVPXP3RepackForm::show()
{
	TVPXP3RepackForm *form = new TVPXP3RepackForm;
	form->initFromFile("ui/XP3RepackDialog.csb");
	TVPMainScene::GetInstance()->pushUIForm(form, TVPMainScene::eEnterFromBottom);
	return form;
}

void TVPXP3RepackForm::initData(std::vector<std::string> &filelist, const std::string &xp3filter)
{
	if (Packer) delete Packer;
	Packer = new XP3ArchiveRepackAsync(xp3filter);
	TotalSize = 0;
	for (const std::string &name : filelist) {
		TotalSize += Packer->AddTask(name);
	}

}

class TVPXP3RepackFileListForm : public iTVPFloatForm {
public:
	virtual void bindBodyController(const NodeMap &allNodes);
	static TVPXP3RepackFileListForm* show(std::vector<std::string> &filelist, const std::string &dir);
	void initData(std::vector<std::string> &filelist, const std::string &dir);
	void close();

private:
	void onOkClicked(Ref*);

	cocos2d::ui::ListView *listView;

	std::string RootDir;
	std::vector<std::string> FileList;
};

void TVPXP3RepackFileListForm::bindBodyController(const NodeMap &allNodes)
{
	LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
	Button *btn = static_cast<Button*>(allNodes.findWidget("close"));
	if (btn) {
		btn->setTitleText(locmgr->GetText(btn->getTitleText()));
		btn->addClickEventListener([this](Ref*) { close(); });
	}
	btn = static_cast<Button*>(allNodes.findWidget("cancel"));
	if (btn) {
		btn->setTitleText(locmgr->GetText(btn->getTitleText()));
		btn->addClickEventListener([this](Ref*) { close(); });
	}
	btn = static_cast<Button*>(allNodes.findWidget("ok"));
	if (btn) {
		btn->setTitleText(locmgr->GetText(btn->getTitleText()));
		btn->addClickEventListener([this](Ref*) { close(); });
	}
	btn->addClickEventListener(std::bind(&TVPXP3RepackFileListForm::onOkClicked, this, std::placeholders::_1));
	listView = static_cast<ListView*>(allNodes.findController("list"));
	Text* text = static_cast<Text*>(allNodes.findController("desc"));
	if (text) {
		text->setString(locmgr->GetText(text->getString()));
	}
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
	RootDir = dir;
	FileList.swap(filelist);
	int tag = 256;
	Size size = listView->getContentSize();
	for (const std::string &filename : FileList) {
		tPreferenceItemCheckBox *cell = CreatePreferenceItem<tPreferenceItemCheckBox>(size, filename,
			[](tPreferenceItemCheckBox* p) {
			p->_getter = []()->bool { return true; };
			p->_setter = [](bool v) {};
		});
		cell->setTag(tag++);
		listView->pushBackCustomItem(cell);
	}
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
			return checkbox->getSelectedState();
		}
	};
	std::vector<std::string> filelist; filelist.reserve(FileList.size());
	auto &allCell = listView->getItems();
	for (int i = 0; i < allCell.size(); ++i) {
		Widget *cell = allCell.at(i);
		if (static_cast<HackPreferenceItemCheckBox*>(cell)->getCheckBoxState()) {
			filelist.push_back(FileList[cell->getTag() - 256]);
		}
	}
	if (filelist.empty())
		return;
	TVPXP3RepackForm *form = TVPXP3RepackForm::show();
	form->initData(filelist, RootDir + "/xp3filter.tjs");
	close();
}

void TVPProcessXP3Repack(const std::string &dir)
{
	std::vector<std::string> filelist;
	WalkDir(dir, [&](const std::string& path, tjs_uint64) {
		tjs_uint64 offset;
		ttstr strpath(path);
		tTVPLocalFileStream *st = new tTVPLocalFileStream(strpath, strpath, TJS_BS_READ);
		if (TVPGetXP3ArchiveOffset(st, strpath, offset, false)) {
			filelist.emplace_back(path);
		}
		delete st;
	});
	if (filelist.empty()) {
		// TODO warning
	} else {
		TVPXP3RepackFileListForm::show(filelist, dir);
	}
}
