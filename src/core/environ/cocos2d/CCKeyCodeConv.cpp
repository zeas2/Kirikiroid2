#include "CCKeyCodeConv.h"
#include "cocos/base/CCController.h"
#include "vkdefine.h"

int TVPConvertMouseBtnToVKCode(tTVPMouseButton _mouseBtn)
{
	int btncode;
	switch (_mouseBtn) {
	case mbLeft:    btncode = VK_LBUTTON;	break;
	case mbMiddle:  btncode = VK_MBUTTON;	break;
	case mbRight:   btncode = VK_RBUTTON;	break;
	default:		btncode = 0;			break;
	}
	return btncode;
}

int TVPConvertKeyCodeToVKCode(cocos2d::EventKeyboard::KeyCode keyCode)
{
#define CASE(x) case cocos2d::EventKeyboard::KeyCode::KEY_##x:	return VK_##x
	switch (keyCode) {
		CASE(0);
		CASE(1);
		CASE(2);
		CASE(3);
		CASE(4);
		CASE(5);
		CASE(6);
		CASE(7);
		CASE(8);
		CASE(9);
		CASE(A);
		CASE(B);
		CASE(C);
		CASE(D);
		CASE(E);
		CASE(F);
		CASE(G);
		CASE(H);
		CASE(I);
		CASE(J);
		CASE(K);
		CASE(L);
		CASE(M);
		CASE(N);
		CASE(O);
		CASE(P);
		CASE(Q);
		CASE(R);
		CASE(S);
		CASE(T);
		CASE(U);
		CASE(V);
		CASE(W);
		CASE(X);
		CASE(Y);
		CASE(Z);
		CASE(F1);
		CASE(F2);
		CASE(F3);
		CASE(F4);
		CASE(F5);
		CASE(F6);
		CASE(F7);
		CASE(F8);
		CASE(F9);
		CASE(F10);
		CASE(F11);
		CASE(F12);
		CASE(PAUSE);
		CASE(PRINT);
		CASE(ESCAPE);
	case cocos2d::EventKeyboard::KeyCode::KEY_BACK_TAB:
		CASE(TAB);
		CASE(RETURN);
	case cocos2d::EventKeyboard::KeyCode::KEY_SCROLL_LOCK:	return VK_SCROLL;
	case cocos2d::EventKeyboard::KeyCode::KEY_SYSREQ:	return VK_SNAPSHOT;
	case cocos2d::EventKeyboard::KeyCode::KEY_BREAK:	return VK_CANCEL;
	case cocos2d::EventKeyboard::KeyCode::KEY_BACKSPACE:	return VK_BACK;
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPS_LOCK:	return VK_CAPITAL;
	case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_SHIFT:	return VK_SHIFT; // LR the same
	case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_SHIFT:	return VK_SHIFT;
	case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_CTRL:	return VK_CONTROL; // LR the same
	case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_CTRL:	return VK_CONTROL;
	case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ALT:	return VK_MENU;
	case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ALT:	return VK_MENU;
	case cocos2d::EventKeyboard::KeyCode::KEY_MENU:	return VK_APPS;
	case cocos2d::EventKeyboard::KeyCode::KEY_HYPER:	return VK_LWIN;
		CASE(INSERT);
		CASE(HOME);
		CASE(DELETE);
		CASE(END);
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_PG_UP:
	case cocos2d::EventKeyboard::KeyCode::KEY_PG_UP:	return VK_PRIOR;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_PG_DOWN:
	case cocos2d::EventKeyboard::KeyCode::KEY_PG_DOWN:	return VK_NEXT;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_LEFT:
	case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW:	return VK_LEFT;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_RIGHT:
	case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW:	return VK_RIGHT;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_UP:
	case cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW:	return VK_UP;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_DOWN:
	case cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW:	return VK_DOWN;
	case cocos2d::EventKeyboard::KeyCode::KEY_NUM_LOCK:	return VK_NUMLOCK;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_PLUS:	return VK_ADD;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_MINUS:	return VK_SUBTRACT;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_MULTIPLY:	return VK_MULTIPLY;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_DIVIDE:	return VK_DIVIDE;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_ENTER:	return VK_RETURN;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_HOME:	return VK_HOME;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_FIVE:	return VK_NUMPAD5;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_END:	return VK_END;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_INSERT:	return VK_INSERT;
	case cocos2d::EventKeyboard::KeyCode::KEY_KP_DELETE:	return VK_DELETE;
		CASE(SPACE);
	case cocos2d::EventKeyboard::KeyCode::KEY_EXCLAM:	return VK_SPACE;
	case cocos2d::EventKeyboard::KeyCode::KEY_QUOTE:	return VK_OEM_7;
	case cocos2d::EventKeyboard::KeyCode::KEY_COMMA:	return VK_OEM_COMMA;
	case cocos2d::EventKeyboard::KeyCode::KEY_MINUS:	return VK_OEM_MINUS;
	case cocos2d::EventKeyboard::KeyCode::KEY_PERIOD:	return VK_OEM_PERIOD;
	case cocos2d::EventKeyboard::KeyCode::KEY_EQUAL: return VK_OEM_PLUS;
	case cocos2d::EventKeyboard::KeyCode::KEY_SLASH:	return VK_OEM_2;
	case cocos2d::EventKeyboard::KeyCode::KEY_SEMICOLON:	return VK_OEM_1;
	case cocos2d::EventKeyboard::KeyCode::KEY_BACK_SLASH:	return VK_OEM_5;
	case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_BRACE:	return VK_OEM_4;
	case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_BRACE:	return VK_OEM_6;
	case cocos2d::EventKeyboard::KeyCode::KEY_PLAY:	return VK_PLAY;
	case cocos2d::EventKeyboard::KeyCode::KEY_NUMBER:
	case cocos2d::EventKeyboard::KeyCode::KEY_DOLLAR:
	case cocos2d::EventKeyboard::KeyCode::KEY_PERCENT:
	case cocos2d::EventKeyboard::KeyCode::KEY_CIRCUMFLEX:
	case cocos2d::EventKeyboard::KeyCode::KEY_AMPERSAND:
	case cocos2d::EventKeyboard::KeyCode::KEY_APOSTROPHE:
	case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_PARENTHESIS:
	case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_PARENTHESIS:
	case cocos2d::EventKeyboard::KeyCode::KEY_ASTERISK:
	case cocos2d::EventKeyboard::KeyCode::KEY_COLON:
	case cocos2d::EventKeyboard::KeyCode::KEY_LESS_THAN:
	case cocos2d::EventKeyboard::KeyCode::KEY_PLUS:
	case cocos2d::EventKeyboard::KeyCode::KEY_GREATER_THAN:
	case cocos2d::EventKeyboard::KeyCode::KEY_QUESTION:
	case cocos2d::EventKeyboard::KeyCode::KEY_AT:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_A:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_B:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_C:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_D:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_E:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_F:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_G:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_H:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_I:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_J:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_K:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_L:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_M:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_N:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_O:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_P:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_Q:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_R:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_S:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_T:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_U:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_V:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_W:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_X:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_Y:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_Z:
	case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_BRACKET:
	case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_BRACKET:
	case cocos2d::EventKeyboard::KeyCode::KEY_UNDERSCORE:
	case cocos2d::EventKeyboard::KeyCode::KEY_GRAVE:
	case cocos2d::EventKeyboard::KeyCode::KEY_BAR:
	case cocos2d::EventKeyboard::KeyCode::KEY_TILDE:
	case cocos2d::EventKeyboard::KeyCode::KEY_EURO:
	case cocos2d::EventKeyboard::KeyCode::KEY_POUND:
	case cocos2d::EventKeyboard::KeyCode::KEY_YEN:
	case cocos2d::EventKeyboard::KeyCode::KEY_MIDDLE_DOT:
	case cocos2d::EventKeyboard::KeyCode::KEY_SEARCH:
	case cocos2d::EventKeyboard::KeyCode::KEY_DPAD_LEFT:
	case cocos2d::EventKeyboard::KeyCode::KEY_DPAD_RIGHT:
	case cocos2d::EventKeyboard::KeyCode::KEY_DPAD_UP:
	case cocos2d::EventKeyboard::KeyCode::KEY_DPAD_DOWN:
	case cocos2d::EventKeyboard::KeyCode::KEY_DPAD_CENTER:
	case cocos2d::EventKeyboard::KeyCode::KEY_ENTER:
	default: return 0;
	}
#undef CASE
}

int TVPConvertPadKeyCodeToVKCode(int keyCode)
{
	switch (keyCode) {
	case cocos2d::Controller::BUTTON_A: return VK_PAD1;
	case cocos2d::Controller::BUTTON_B: return VK_PAD2;
	case cocos2d::Controller::BUTTON_C: return VK_PAD3;
	case cocos2d::Controller::BUTTON_X: return VK_PAD4;
	case cocos2d::Controller::BUTTON_Y: return VK_PAD5;
	case cocos2d::Controller::BUTTON_Z: return VK_PAD6;
	case cocos2d::Controller::BUTTON_LEFT_SHOULDER: return VK_PAD7;
	case cocos2d::Controller::BUTTON_RIGHT_SHOULDER: return VK_PAD8;
	case cocos2d::Controller::BUTTON_LEFT_THUMBSTICK: return VK_PAD9;
	case cocos2d::Controller::BUTTON_RIGHT_THUMBSTICK: return VK_PAD10;
	case cocos2d::Controller::BUTTON_START: return VK_PAD9;
	case cocos2d::Controller::BUTTON_SELECT: return VK_PAD10;
	case cocos2d::Controller::AXIS_LEFT_TRIGGER: return VK_PAD5;
	case cocos2d::Controller::AXIS_RIGHT_TRIGGER: return VK_PAD6;
	case cocos2d::Controller::BUTTON_PAUSE: return VK_PAD7;
	case cocos2d::Controller::BUTTON_DPAD_UP: return VK_PADUP;
	case cocos2d::Controller::BUTTON_DPAD_DOWN: return VK_PADDOWN;
	case cocos2d::Controller::BUTTON_DPAD_LEFT: return VK_PADLEFT;
	case cocos2d::Controller::BUTTON_DPAD_RIGHT: return VK_PADRIGHT;
	case cocos2d::Controller::BUTTON_DPAD_CENTER:
	default: return 0;
	}
}


const std::unordered_map<std::string, int> & TVPGetVKCodeNameMap()
{
	static std::unordered_map<std::string, int> ret({
#define CASE(x) { #x, VK_##x }
		CASE(0),
		CASE(1),
		CASE(2),
		CASE(3),
		CASE(4),
		CASE(5),
		CASE(6),
		CASE(7),
		CASE(8),
		CASE(9),
		CASE(A),
		CASE(B),
		CASE(C),
		CASE(D),
		CASE(E),
		CASE(F),
		CASE(G),
		CASE(H),
		CASE(I),
		CASE(J),
		CASE(K),
		CASE(L),
		CASE(M),
		CASE(N),
		CASE(O),
		CASE(P),
		CASE(Q),
		CASE(R),
		CASE(S),
		CASE(T),
		CASE(U),
		CASE(V),
		CASE(W),
		CASE(X),
		CASE(Y),
		CASE(Z),
		CASE(F1),
		CASE(F2),
		CASE(F3),
		CASE(F4),
		CASE(F5),
		CASE(F6),
		CASE(F7),
		CASE(F8),
		CASE(F9),
		CASE(F10),
		CASE(F11),
		CASE(F12),
		CASE(PAUSE),
		CASE(PRINT),
		CASE(ESCAPE),
		CASE(TAB),
		CASE(RETURN),
		CASE(SCROLL),
		CASE(SNAPSHOT),
		CASE(CANCEL),
		CASE(BACK),
		CASE(CAPITAL),
		CASE(SHIFT),
		CASE(CONTROL),
		CASE(MENU),
		CASE(APPS),
		CASE(LWIN),
		CASE(INSERT),
		CASE(HOME),
		CASE(DELETE),
		CASE(END),
		CASE(PRIOR),
		CASE(NEXT),
		CASE(LEFT),
		CASE(RIGHT),
		CASE(UP),
		CASE(DOWN),
		CASE(NUMLOCK),
		CASE(ADD),
		CASE(SUBTRACT),
		CASE(MULTIPLY),
		CASE(DIVIDE),
		CASE(HOME),
		CASE(NUMPAD5),
		CASE(END),
		CASE(INSERT),
		CASE(DELETE),
		CASE(SPACE),
		CASE(OEM_COMMA),
		CASE(OEM_MINUS),
		CASE(OEM_PERIOD),
		CASE(OEM_PLUS),
		CASE(OEM_1),
		CASE(OEM_2),
		CASE(OEM_4),
		CASE(OEM_5),
		CASE(OEM_6),
		CASE(OEM_7),
		CASE(PLAY),
#undef CASE
	});
	return ret;
}

std::string TVPGetVKCodeName(int keyCode)
{
	return "";
}
