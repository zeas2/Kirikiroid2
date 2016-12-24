
#include "ncbind/ncbind.hpp"

#define NCB_MODULE_NAME TJS_W("win32dialog.dll")

static void InitPlugin_WIN32Dialog()
{
	TVPExecuteScript(
		TJS_W("class WIN32Dialog {")
		TJS_W("	function messageBox(message, caption, type) {return !System.inform(message, caption, 2);}")
		TJS_W("}")
		);
}

NCB_PRE_REGIST_CALLBACK(InitPlugin_WIN32Dialog);
