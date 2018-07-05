#include "tjsTypes.h"
#include "tvpinputdefs.h"
#include "cocos2d.h"

int TVPConvertMouseBtnToVKCode(tTVPMouseButton _mouseBtn);
int TVPConvertKeyCodeToVKCode(cocos2d::EventKeyboard::KeyCode keyCode);
int TVPConvertPadKeyCodeToVKCode(int keyCode);
const std::unordered_map<std::string, int> &TVPGetVKCodeNameMap();
std::string TVPGetVKCodeName(int keyCode);