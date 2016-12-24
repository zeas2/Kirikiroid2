//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Version information dialog box
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include "Resource.h"

#include "MsgIntf.h"
#include "VersionFormUnit.h"
#include "SystemControl.h"
#include "WindowImpl.h"

#include "VersionFormUnit.h"
#include "DebugIntf.h"
#include "ClipboardIntf.h"

//---------------------------------------------------------------------------
// TVPCopyImportantLogToClipboard
//---------------------------------------------------------------------------
void TVPCopyImportantLogToClipboard()
{
	// get Direct3D driver information
	TVPEnsureDirect3DObject();
	TVPDumpDirect3DDriverInformation();

	// copy
	TVPClipboardSetText(TVPGetImportantLog());
}

static LRESULT WINAPI DlgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	switch( msg ) {
	case WM_INITDIALOG: {
		::SetDlgItemText( hWnd, IDC_INFOMATION_EDIT, TVPGetAboutString().AsStdString().c_str() );
		return TRUE;
	}
	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK) {
			//OKÉ{É^ÉìÇ™âüÇ≥ÇÍÇΩÇ∆Ç´ÇÃèàóù
			::EndDialog(hWnd, IDOK);
			return TRUE;
		} else if(LOWORD(wParam) == IDC_COPY_INFO_BUTTON) {
			TVPCopyImportantLogToClipboard();
			return TRUE;
		}
		break;
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}
//---------------------------------------------------------------------------
void TVPShowVersionForm()
{
	// get Direct3D driver information
	TVPEnsureDirect3DObject();
	TVPDumpDirect3DDriverInformation();
	::DialogBox( NULL, MAKEINTRESOURCE(IDD_VERSION_DIALOG), NULL, (DLGPROC)DlgProc );
}
//---------------------------------------------------------------------------




