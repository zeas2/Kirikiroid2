//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// System Main Window (Controller)
//---------------------------------------------------------------------------
#ifndef MainFormUnitH
#define MainFormUnitH
//---------------------------------------------------------------------------
#include "Classes.hpp"
#include "Controls.hpp"
//#include <StdCtrls.hpp>
#include "Forms.hpp"
//#include <ComCtrls.hpp>
// #include <ImgList.hpp>
// #include <ToolWin.hpp>
// #include <ExtCtrls.hpp>
//#include <Buttons.hpp>
#include "Menus.hpp"
#include "Dialogs.hpp"
//---------------------------------------------------------------------------
class TTVPMainForm// : public TForm
{
public:	// IDE 管理のコンポーネント
// 	TImageList *VerySmallIconImageList;
// 	TImageList *SmallIconImageList;
	//TToolBar *ToolBar;
// 	TToolButton *ShowScriptEditorButton;
// 	TToolButton *ShowConsoleButton;
// 	TToolButton *ToolButton3;
// 	TToolButton *ExitButton;
// 	TToolButton *EventButton;
// 	TPopupMenu *PopupMenu;
// 	TMenuItem *ShowScriptEditorMenuItem;
// 	TMenuItem *ShowConsoleMenuItem;
// 	TMenuItem *N1;
// 	TMenuItem *EnableEventMenuItem;
// 	TMenuItem *N2;
// 	TMenuItem *ExitMenuItem;
// 	TMenuItem *ShowAboutMenuItem;
// 	TMenuItem *N3;
// 	TMenuItem *DumpMenuItem;
// 	TToolButton *ToolButton1;
//     TToolButton *ShowWatchButton;
    //SDL_TimerID SystemWatchTimer;
// 	TMenuItem *ShowWatchMenuItem;
// 	TMenuItem *CopyImportantLogMenuItem;
// 	TMenuItem *CreateMessageMapFileMenuItem;
//	TSaveDialog *MessageMapFileSaveDialog;
// 	TMenuItem *ShowOnTopMenuItem;
// 	TMenuItem *N4;
// 	TMenuItem *ShowControllerMenuItem;
// 	TMenuItem *RestartScriptEngineMenuItem;
	void ShowScriptEditorButtonClick(TObject *Sender);
	void ExitButtonClick(TObject *Sender);
	void ShowConsoleButtonClick(TObject *Sender);
	void PopupMenuPopup(TObject *Sender);
	void EnableEventMenuItemClick(TObject *Sender);
	void ShowAboutMenuItemClick(TObject *Sender);
	void DumpMenuItemClick(TObject *Sender);
	void EventButtonClick(TObject *Sender);
	void ShowWatchButtonClick(TObject *Sender);
    void SystemWatchTimerTimer(TObject *Sender);
	void FormDestroy(TObject *Sender);
	void FormClose(TObject *Sender, TCloseAction &Action);
	void CopyImportantLogMenuItemClick(TObject *Sender);
	void CreateMessageMapFileMenuItemClick(TObject *Sender);
	void ShowOnTopMenuItemClick(TObject *Sender);
	void ShowControllerMenuItemClick(TObject *Sender);
	void RestartScriptEngineMenuItemClick(TObject *Sender);

private:	// ユーザー宣言
	bool ContinuousEventCalling;
	bool AutoShowConsoleOnError;
	bool ApplicationStayOnTop;
	bool ApplicationActivating;
	bool ApplicationNotMinimizing;

public:		// ユーザー宣言
	TTVPMainForm(TWinControl* Owner);
    ~TTVPMainForm();

	static TShortCut GetHotKeyFromOption(TShortCut def, const tjs_char * optname);
	static void SetHotKey(TMenuItem *item, const tjs_char * optname);

	bool CanHideAnyWindow();

	void SetConsoleVisible(bool b);
	bool GetConsoleVisible();
	void NotifyConsoleHiding();

	bool GetScriptEditorVisible();
	void NotifyScriptEditorHiding();

	bool GetWatchVisible();
	void NotifyWatchHiding();

	void SetAutoShowConsoleOnError(bool b) { AutoShowConsoleOnError = true; }
	bool GetAutoShowConsoleOnError() const { return AutoShowConsoleOnError; }

	void ShowController();
	void InvokeEvents();
	void CallDeliverAllEventsOnIdle();

	void BeginContinuousEvent();
	void EndContinuousEvent();

	void NotifyCloseClicked();
	void NotifyEventDelivered();
	void NotifyHaltWarnFormClosed();

	void NotifySystemError();

	void SetApplicationStayOnTop(bool b);
	bool GetApplicationStayOnTop() const { return ApplicationStayOnTop; }

	bool GetApplicationActivating() const { return ApplicationActivating; }
	bool GetApplicationNotMinimizing() const { return ApplicationNotMinimizing; }
    virtual void Dispatch(void*){};

	void ApplicationDeactivate();
	void ApplicationActivate();

protected:
//BEGIN_MESSAGE_MAP
//	VCL_MESSAGE_HANDLER( SDL_USEREVENT+0x30, TMessage, WMInvokeEvents)
//	VCL_MESSAGE_HANDLER( SDL_USEREVENT+0x32, TMessage, WMRearrangeModalWindows)
//END_MESSAGE_MAP(TForm)
//	void WMInvokeEvents(TMessage &Msg);
//	void WMRearrangeModalWindows(TMessage &Msg);
private:
	uint32_t LastCompactedTick;
	uint32_t LastContinuousTick;
	uint32_t LastCloseClickedTick;
	uint32_t LastShowModalWindowSentTick;
	uint32_t LastRehashedTick;

	uint32_t MixedIdleTick;

	void DeliverEvents();

	void ApplicationIdle(TObject *Sender, bool &Done);
	void ApplicationMinimize(TObject *Sender);
	void ApplicationRestore(TObject *Sender);

public: // hotkey management
	TShortCut ShowUpdateRectShortCut;
	TShortCut DumpLayerStructureShortCut;

};
//---------------------------------------------------------------------------
extern TTVPMainForm *TVPMainForm;
extern bool TVPMainFormAlive;
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Environment Profiling Support
//---------------------------------------------------------------------------
#include "IniFiles.hpp"
class tTVPProfileHolder : public TMemIniFile
{
public:
	tTVPProfileHolder(const AnsiString &fn) : TMemIniFile(fn) {};
	~tTVPProfileHolder() {};

	void WriteStrings(const AnsiString &section, const AnsiString &ident,
		TStrings * strings);
	void ReadStrings(const AnsiString &section, const AnsiString &ident,
		TStrings * strings);
};
extern void TVPWriteEnvironProfile();
extern void TVPEnvironProfileAddRef();
extern void TVPEnvironProfileRelease();
extern tTVPProfileHolder *TVPGetEnvironProfile();
//---------------------------------------------------------------------------



#endif
