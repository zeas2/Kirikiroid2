#include "ncbind/ncbind.hpp"
#include "MsgIntf.h"

#define NCB_MODULE_NAME TJS_W("getabout.dll")
NCB_ATTACH_FUNCTION(getAboutString, System, TVPGetAboutString);
