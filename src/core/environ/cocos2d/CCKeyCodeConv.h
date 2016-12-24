static int _ConvertMouseBtnToVKCode(tTVPMouseButton _mouseBtn) {
	int btncode;
	switch (_mouseBtn) {
	case mbLeft:    btncode = VK_LBUTTON;	break;
	case mbMiddle:  btncode = VK_MBUTTON;	break;
	case mbRight:   btncode = VK_RBUTTON;	break;
	default:		btncode = 0;			break;
	}
	return btncode;
}

static int _ConvertKeyCodeToVKCode(EventKeyboard::KeyCode keyCode) {
#define CASE(x) case EventKeyboard::KeyCode::KEY_##x:	return VK_##x
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
	case EventKeyboard::KeyCode::KEY_BACK_TAB:
		CASE(TAB);
		CASE(RETURN);
	case EventKeyboard::KeyCode::KEY_SCROLL_LOCK:	return VK_SCROLL;
	case EventKeyboard::KeyCode::KEY_SYSREQ:	return VK_SNAPSHOT;
	case EventKeyboard::KeyCode::KEY_BREAK:	return VK_CANCEL;
	case EventKeyboard::KeyCode::KEY_BACKSPACE:	return VK_BACK;
	case EventKeyboard::KeyCode::KEY_CAPS_LOCK:	return VK_CAPITAL;
	case EventKeyboard::KeyCode::KEY_LEFT_SHIFT:	return VK_SHIFT; // LR the same
	case EventKeyboard::KeyCode::KEY_RIGHT_SHIFT:	return VK_SHIFT;
	case EventKeyboard::KeyCode::KEY_LEFT_CTRL:	return VK_CONTROL; // LR the same
	case EventKeyboard::KeyCode::KEY_RIGHT_CTRL:	return VK_CONTROL;
	case EventKeyboard::KeyCode::KEY_LEFT_ALT:	return VK_MENU;
	case EventKeyboard::KeyCode::KEY_RIGHT_ALT:	return VK_MENU;
	case EventKeyboard::KeyCode::KEY_MENU:	return VK_APPS;
	case EventKeyboard::KeyCode::KEY_HYPER:	return VK_LWIN;
		CASE(INSERT);
		CASE(HOME);
		CASE(DELETE);
		CASE(END);
	case EventKeyboard::KeyCode::KEY_KP_PG_UP:
	case EventKeyboard::KeyCode::KEY_PG_UP:	return VK_PRIOR;
	case EventKeyboard::KeyCode::KEY_KP_PG_DOWN:
	case EventKeyboard::KeyCode::KEY_PG_DOWN:	return VK_NEXT;
	case EventKeyboard::KeyCode::KEY_KP_LEFT:
	case EventKeyboard::KeyCode::KEY_LEFT_ARROW:	return VK_LEFT;
	case EventKeyboard::KeyCode::KEY_KP_RIGHT:
	case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:	return VK_RIGHT;
	case EventKeyboard::KeyCode::KEY_KP_UP:
	case EventKeyboard::KeyCode::KEY_UP_ARROW:	return VK_UP;
	case EventKeyboard::KeyCode::KEY_KP_DOWN:
	case EventKeyboard::KeyCode::KEY_DOWN_ARROW:	return VK_DOWN;
	case EventKeyboard::KeyCode::KEY_NUM_LOCK:	return VK_NUMLOCK;
	case EventKeyboard::KeyCode::KEY_KP_PLUS:	return VK_ADD;
	case EventKeyboard::KeyCode::KEY_KP_MINUS:	return VK_SUBTRACT;
	case EventKeyboard::KeyCode::KEY_KP_MULTIPLY:	return VK_MULTIPLY;
	case EventKeyboard::KeyCode::KEY_KP_DIVIDE:	return VK_DIVIDE;
	case EventKeyboard::KeyCode::KEY_KP_ENTER:	return VK_RETURN;
	case EventKeyboard::KeyCode::KEY_KP_HOME:	return VK_HOME;
	case EventKeyboard::KeyCode::KEY_KP_FIVE:	return VK_NUMPAD5;
	case EventKeyboard::KeyCode::KEY_KP_END:	return VK_END;
	case EventKeyboard::KeyCode::KEY_KP_INSERT:	return VK_INSERT;
	case EventKeyboard::KeyCode::KEY_KP_DELETE:	return VK_DELETE;
		CASE(SPACE);
	case EventKeyboard::KeyCode::KEY_EXCLAM:	return VK_SPACE;
	case EventKeyboard::KeyCode::KEY_QUOTE:	return VK_OEM_7;
	case EventKeyboard::KeyCode::KEY_COMMA:	return VK_OEM_COMMA;
	case EventKeyboard::KeyCode::KEY_MINUS:	return VK_OEM_MINUS;
	case EventKeyboard::KeyCode::KEY_PERIOD:	return VK_OEM_PERIOD;
	case EventKeyboard::KeyCode::KEY_EQUAL: return VK_OEM_PLUS;
	case EventKeyboard::KeyCode::KEY_SLASH:	return VK_OEM_2;
	case EventKeyboard::KeyCode::KEY_SEMICOLON:	return VK_OEM_1;
	case EventKeyboard::KeyCode::KEY_BACK_SLASH:	return VK_OEM_5;
	case EventKeyboard::KeyCode::KEY_LEFT_BRACE:	return VK_OEM_4;
	case EventKeyboard::KeyCode::KEY_RIGHT_BRACE:	return VK_OEM_6;
	case EventKeyboard::KeyCode::KEY_PLAY:	return VK_PLAY;
	case EventKeyboard::KeyCode::KEY_NUMBER:
	case EventKeyboard::KeyCode::KEY_DOLLAR:
	case EventKeyboard::KeyCode::KEY_PERCENT:
	case EventKeyboard::KeyCode::KEY_CIRCUMFLEX:
	case EventKeyboard::KeyCode::KEY_AMPERSAND:
	case EventKeyboard::KeyCode::KEY_APOSTROPHE:
	case EventKeyboard::KeyCode::KEY_LEFT_PARENTHESIS:
	case EventKeyboard::KeyCode::KEY_RIGHT_PARENTHESIS:
	case EventKeyboard::KeyCode::KEY_ASTERISK:
	case EventKeyboard::KeyCode::KEY_COLON:
	case EventKeyboard::KeyCode::KEY_LESS_THAN:
	case EventKeyboard::KeyCode::KEY_PLUS:
	case EventKeyboard::KeyCode::KEY_GREATER_THAN:
	case EventKeyboard::KeyCode::KEY_QUESTION:
	case EventKeyboard::KeyCode::KEY_AT:
	case EventKeyboard::KeyCode::KEY_CAPITAL_A:
	case EventKeyboard::KeyCode::KEY_CAPITAL_B:
	case EventKeyboard::KeyCode::KEY_CAPITAL_C:
	case EventKeyboard::KeyCode::KEY_CAPITAL_D:
	case EventKeyboard::KeyCode::KEY_CAPITAL_E:
	case EventKeyboard::KeyCode::KEY_CAPITAL_F:
	case EventKeyboard::KeyCode::KEY_CAPITAL_G:
	case EventKeyboard::KeyCode::KEY_CAPITAL_H:
	case EventKeyboard::KeyCode::KEY_CAPITAL_I:
	case EventKeyboard::KeyCode::KEY_CAPITAL_J:
	case EventKeyboard::KeyCode::KEY_CAPITAL_K:
	case EventKeyboard::KeyCode::KEY_CAPITAL_L:
	case EventKeyboard::KeyCode::KEY_CAPITAL_M:
	case EventKeyboard::KeyCode::KEY_CAPITAL_N:
	case EventKeyboard::KeyCode::KEY_CAPITAL_O:
	case EventKeyboard::KeyCode::KEY_CAPITAL_P:
	case EventKeyboard::KeyCode::KEY_CAPITAL_Q:
	case EventKeyboard::KeyCode::KEY_CAPITAL_R:
	case EventKeyboard::KeyCode::KEY_CAPITAL_S:
	case EventKeyboard::KeyCode::KEY_CAPITAL_T:
	case EventKeyboard::KeyCode::KEY_CAPITAL_U:
	case EventKeyboard::KeyCode::KEY_CAPITAL_V:
	case EventKeyboard::KeyCode::KEY_CAPITAL_W:
	case EventKeyboard::KeyCode::KEY_CAPITAL_X:
	case EventKeyboard::KeyCode::KEY_CAPITAL_Y:
	case EventKeyboard::KeyCode::KEY_CAPITAL_Z:
	case EventKeyboard::KeyCode::KEY_LEFT_BRACKET:
	case EventKeyboard::KeyCode::KEY_RIGHT_BRACKET:
	case EventKeyboard::KeyCode::KEY_UNDERSCORE:
	case EventKeyboard::KeyCode::KEY_GRAVE:
	case EventKeyboard::KeyCode::KEY_BAR:
	case EventKeyboard::KeyCode::KEY_TILDE:
	case EventKeyboard::KeyCode::KEY_EURO:
	case EventKeyboard::KeyCode::KEY_POUND:
	case EventKeyboard::KeyCode::KEY_YEN:
	case EventKeyboard::KeyCode::KEY_MIDDLE_DOT:
	case EventKeyboard::KeyCode::KEY_SEARCH:
	case EventKeyboard::KeyCode::KEY_DPAD_LEFT:
	case EventKeyboard::KeyCode::KEY_DPAD_RIGHT:
	case EventKeyboard::KeyCode::KEY_DPAD_UP:
	case EventKeyboard::KeyCode::KEY_DPAD_DOWN:
	case EventKeyboard::KeyCode::KEY_DPAD_CENTER:
	case EventKeyboard::KeyCode::KEY_ENTER:
	default: return 0;
	}
#undef CASE
}

static int _ConvertPadKeyCodeToVKCode(int keyCode) {
	switch (keyCode) {
	case Controller::BUTTON_A: return VK_PAD1;
	case Controller::BUTTON_B: return VK_PAD2;
	case Controller::BUTTON_C: return VK_PAD3;
	case Controller::BUTTON_X: return VK_PAD4;
	case Controller::BUTTON_Y: return VK_PAD5;
	case Controller::BUTTON_Z: return VK_PAD6;
	case Controller::BUTTON_LEFT_SHOULDER: return VK_PAD7;
	case Controller::BUTTON_RIGHT_SHOULDER: return VK_PAD8;
	case Controller::BUTTON_LEFT_THUMBSTICK: return VK_PAD9;
	case Controller::BUTTON_RIGHT_THUMBSTICK: return VK_PAD10;
	case Controller::BUTTON_START: return VK_PAD9;
	case Controller::BUTTON_SELECT: return VK_PAD10;
	case Controller::AXIS_LEFT_TRIGGER: return VK_PAD5;
	case Controller::AXIS_RIGHT_TRIGGER: return VK_PAD6;
	case Controller::BUTTON_PAUSE: return VK_PAD7;
	case Controller::BUTTON_DPAD_UP: return VK_PADUP;
	case Controller::BUTTON_DPAD_DOWN: return VK_PADDOWN;
	case Controller::BUTTON_DPAD_LEFT: return VK_PADLEFT;
	case Controller::BUTTON_DPAD_RIGHT: return VK_PADRIGHT;
	case Controller::BUTTON_DPAD_CENTER:
	default: return 0;
	}
}
