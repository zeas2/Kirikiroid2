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
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ImgList.hpp>
#include <ToolWin.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <Menus.hpp>
#include <Dialogs.hpp>
//---------------------------------------------------------------------------
class TTVPMainForm : public TForm
{
__published:	// IDE 管理のコンポーネント
	TImageList *VerySmallIconImageList;
	TImageList *SmallIconImageList;
	TToolBar *ToolBar;
	TToolButton *ShowScriptEditorButton;
	TToolButton *ShowConsoleButton;
	TToolButton *ToolButton3;
	TToolButton *ExitButton;
	TToolButton *EventButton;
	TPopupMenu *PopupMenu;
	TMenuItem *ShowScriptEditorMenuItem;
	TMenuItem *ShowConsoleMenuItem;
	TMenuItem *N1;
	TMenuItem *EnableEventMenuItem;
	TMenuItem *N2;
	TMenuItem *ExitMenuItem;
	TMenuItem *ShowAboutMenuItem;
	TMenuItem *N3;
	TMenuItem *DumpMenuItem;
	TToolButton *ToolButton1;
	TToolButton *ShowWatchButton;
	TMenuItem *ShowWatchMenuItem;
	TTimer *SystemWatchTimer;
	TMenuItem *CopyImportantLogMenuItem;
	TMenuItem *CreateMessageMapFileMenuItem;
	TSaveDialog *MessageMapFileSaveDialog;
	TMenuItem *ShowOnTopMenuItem;
	TMenuItem *N4;
	TMenuItem *ShowControllerMenuItem;
	TMenuItem *RestartScriptEngineMenuItem;
	void __fastcall ShowScriptEditorButtonClick(TObject *Sender);
	void __fastcall ExitButtonClick(TObject *Sender);
	void __fastcall ShowConsoleButtonClick(TObject *Sender);
	void __fastcall PopupMenuPopup(TObject *Sender);
	void __fastcall EnableEventMenuItemClick(TObject *Sender);
	void __fastcall ShowAboutMenuItemClick(TObject *Sender);
	void __fastcall DumpMenuItemClick(TObject *Sender);
	void __fastcall EventButtonClick(TObject *Sender);
	void __fastcall ShowWatchButtonClick(TObject *Sender);
	void __fastcall SystemWatchTimerTimer(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall CopyImportantLogMenuItemClick(TObject *Sender);
	void __fastcall CreateMessageMapFileMenuItemClick(TObject *Sender);
	void __fastcall ShowOnTopMenuItemClick(TObject *Sender);
	void __fastcall ShowControllerMenuItemClick(TObject *Sender);
	void __fastcall RestartScriptEngineMenuItemClick(TObject *Sender);

private:	// ユーザー宣言
	bool ContinuousEventCalling;
	bool AutoShowConsoleOnError;
	bool ApplicationStayOnTop;
	bool ApplicationActivating;
	bool ApplicationNotMinimizing;

public:		// ユーザー宣言
	__fastcall TTVPMainForm(TComponent* Owner);

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

protected:
BEGIN_MESSAGE_MAP
	VCL_MESSAGE_HANDLER( WM_USER+0x30, TMessage, WMInvokeEvents)
	VCL_MESSAGE_HANDLER( WM_USER+0x32, TMessage, WMRearrangeModalWindows)
END_MESSAGE_MAP(TForm)
	void __fastcall WMInvokeEvents(TMessage &Msg);
	void __fastcall WMRearrangeModalWindows(TMessage &Msg);
private:
	DWORD LastCompactedTick;
	DWORD LastContinuousTick;
	DWORD LastCloseClickedTick;
	DWORD LastShowModalWindowSentTick;
	DWORD LastRehashedTick;

	DWORD MixedIdleTick;

	void DeliverEvents();

	void __fastcall ApplicationIdle(TObject *Sender, bool &Done);
	void __fastcall ApplicationActivate(TObject *Sender);
	void __fastcall ApplicationDeactivate(TObject *Sender);
	void __fastcall ApplicationMinimize(TObject *Sender);
	void __fastcall ApplicationRestore(TObject *Sender);

public: // hotkey management
	TShortCut ShowUpdateRectShortCut;
	TShortCut DumpLayerStructureShortCut;

};
//---------------------------------------------------------------------------
extern PACKAGE TTVPMainForm *TVPMainForm;
extern bool TVPMainFormAlive;
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Environment Profiling Support
//---------------------------------------------------------------------------
#include "inifiles.hpp"
class tTVPProfileHolder : public TMemIniFile
{
public:
	__fastcall tTVPProfileHolder(const AnsiString &fn) : TMemIniFile(fn) {};
	__fastcall ~tTVPProfileHolder() {};

	void __fastcall WriteStrings(const AnsiString &section, const AnsiString &ident,
		TStrings * strings);
	void __fastcall ReadStrings(const AnsiString &section, const AnsiString &ident,
		TStrings * strings);
};
extern void TVPWriteEnvironProfile();
extern void TVPEnvironProfileAddRef();
extern void TVPEnvironProfileRelease();
extern tTVPProfileHolder *TVPGetEnvironProfile();
//---------------------------------------------------------------------------



#endif
