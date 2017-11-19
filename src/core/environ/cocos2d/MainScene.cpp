#include "MainScene.h"
#include "cocos2d.h"
#include "cocos-ext.h"
#include "tjsCommHead.h"
#include "StorageIntf.h"
#include "EventIntf.h"
#include "SysInitImpl.h"
#include "WindowImpl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapImpl.h"
#include "ui/BaseForm.h"
#include "ui/GameMainMenu.h"
#include "ui/UIHelper.h"
#include "TickCount.h"
#include "Random.h"
#include "UtilStreams.h"
#include "vkdefine.h"
#include "base/CCEventListenerController.h"
#include "base/CCController.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "Platform.h"
#include "ui/ConsoleWindow.h"
#include "ui/FileSelectorForm.h"
#include "ui/DebugViewLayerForm.h"
#include "Application.h"
#include "ScriptMgnIntf.h"
#include "win32/TVPWindow.h"
#include "VelocityTracker.h"
#include "SystemImpl.h"
#include "RenderManager.h"
#include "VideoOvlIntf.h"
#include "Exception.h"
#include "win32/SystemControl.h"

USING_NS_CC;

enum SCENE_ORDER {
	GAME_SCENE_ORDER,
	GAME_CONSOLE_ORDER,
	GAME_WINMGR_ORDER, // also for the virtual mouse cursor
	GAME_MENU_ORDER,
	UI_NODE_ORDER,
};

const float UI_CHANGE_DURATION = 0.3f;
class TVPWindowLayer;
static TVPWindowLayer *_lastWindowLayer, *_currentWindowLayer;
class TVPWindowManagerOverlay;
static TVPWindowManagerOverlay *_windowMgrOverlay = nullptr;
static TVPConsoleWindow* _consoleWin = nullptr;
static float _touchMoveThresholdSq;
static cocos2d::Node *_mouseCursor;
static float _mouseCursorScale;
static Vec2 _mouseTouchPoint, _mouseBeginPoint;
static std::set<Touch*> _mouseTouches;
static tTVPMouseButton _mouseBtn;
static int _touchBeginTick;
static bool _virutalMouseMode = false;
static bool _mouseMoved, _mouseClickedDown;
static tjs_uint8 _scancode[0x200];
static Label *_fpsLabel = nullptr;

#include "CCKeyCodeConv.h"

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

static void(*_postUpdate)() = nullptr;
void TVPSetPostUpdateEvent(void(*f)()) { _postUpdate = f; }

static void _refadeMouseCursor() {
	_mouseCursor->stopAllActions();
	_mouseCursor->setOpacity(255);
	_mouseCursor->runAction(Sequence::createWithTwoActions(DelayTime::create(3), FadeOut::create(0.3)));
}

static void AdjustNumerAndDenom(tjs_int &n, tjs_int &d)
{
	tjs_int a = n;
	tjs_int b = d;
	while (b)
	{
		tjs_int t = b;
		b = a % b;
		a = t;
	}
	n = n / a;
	d = d / a;
}

bool TVPGetKeyMouseAsyncState(tjs_uint keycode, bool getcurrent)
{
	if (keycode >= sizeof(_scancode) / sizeof(_scancode[0])) return false;
	tjs_uint8 code = _scancode[keycode];
	_scancode[keycode] &= 1;
	return code & (getcurrent ? 1 : 0x10);
}

bool TVPGetJoyPadAsyncState(tjs_uint keycode, bool getcurrent)
{
	if (keycode >= sizeof(_scancode) / sizeof(_scancode[0])) return false;
	tjs_uint8 code = _scancode[keycode];
	_scancode[keycode] &= 1;
	return code & (getcurrent ? 1 : 0x10);
}

void TVPForceSwapBuffer();
void TVPProcessInputEvents();
void TVPControlAdDialog(int adType, int arg1, int arg2);
// void TVPShowIME(int x, int y, int w, int h);
// void TVPHideIME();

int TVPDrawSceneOnce(int interval) {
	static tjs_uint64 lastTick = TVPGetRoughTickCount32();
	tjs_uint64 curTick = TVPGetRoughTickCount32();
	int remain = interval - (curTick - lastTick);
	if (remain <= 0) {
		if (_postUpdate) _postUpdate();
		Director* director = Director::getInstance();
		director->drawScene(/*true*/);
		TVPForceSwapBuffer();
		lastTick = curTick;
		return 0;
	} else {
		return remain;
	}
}

struct tTVPCursor {
	Node *RootNode;
	Action *Anim = nullptr;
};

#pragma pack(push)
#pragma pack(1)
enum eIconType {
	eIconTypeNone,
	eIconTypeICO,
	eIconTypeCUR
};

struct ICONDIR {
	uint16_t idReserved; // must be 0
	uint16_t idType; //eIconType
	uint16_t idCount;
};

struct ICODIREntry {
	uint8_t bWidth; // 0 -> 256
	uint8_t bHeight; // 0 -> 256
	uint8_t bColorCount;
	uint8_t bReserved;
	union {
		struct {
			uint16_t wPlanes;
			uint16_t wBitCount;
		};
		struct {
			uint16_t wHotSpotX;
			uint16_t wHotSpotY;
		};
	};
	uint32_t dwBytesInRes;
	uint32_t dwImageOffset;
};
#pragma pack(pop)

Sprite *TVPLoadCursorCUR(tTJSBinaryStream *pStream) {
	ICONDIR header;
	pStream->ReadBuffer(&header, sizeof(header));
	if ((header.idReserved != 0) || (header.idType != eIconTypeCUR) || (header.idCount == 0)) {
		return nullptr;
	}

	std::vector<ICODIREntry> cur_dir;
	cur_dir.resize(header.idCount);
	pStream->ReadBuffer(&cur_dir[0], sizeof(ICODIREntry)* header.idCount);
	ICODIREntry bestentry = { 0 };
	for (int i = 0; i < cur_dir.size(); ++i) {
		const ICODIREntry &entry = cur_dir[i];
		if (entry.bHeight > bestentry.bHeight) bestentry = entry;
	}
	cur_dir.clear();
	cur_dir.emplace_back(bestentry);

	for (int i = 0; i < cur_dir.size(); ++i) {
		const ICODIREntry &entry = cur_dir[i];
		int bWidth = entry.bWidth;
		int bHeight = entry.bHeight;
		int bColorCount = entry.bColorCount;
		if (!bWidth) bWidth = 256;
		if (!bHeight) bHeight = 256;
		if (!bColorCount) bColorCount = 256;

		pStream->SetPosition(entry.dwImageOffset);
		TVPBITMAPINFOHEADER bmhdr;
		pStream->ReadBuffer(&bmhdr, sizeof(bmhdr));
		if (bmhdr.biSize != 40) continue;
		if (bmhdr.biCompression) continue;
		int bmpPitch, pad;
		switch (bmhdr.biBitCount) {
		case 1:
			bmpPitch = (bmhdr.biWidth + 7) >> 3;
			pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
			break;
		case 4:
			bmpPitch = (bmhdr.biWidth + 1) >> 1;
			pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
			break;
		case 8:
			bmpPitch = bmhdr.biWidth;
			pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
			break;
		case 32:
			break;
		default:
			continue;
		}
		bmhdr.biHeight /= 2;
		std::vector<unsigned char> pixbuf; pixbuf.resize(bmhdr.biWidth * bmhdr.biHeight * 4);
		tjs_uint32 palette[256];
		if (bmhdr.biBitCount <= 8) {
			if (bmhdr.biClrUsed == 0) bmhdr.biClrUsed = 1 << bmhdr.biBitCount;
			for (int j = 0; j < bmhdr.biClrUsed; ++j) {
				union {
					tjs_uint32 u32;
					tjs_uint8 u8[4];
				} clr, rclr;
				pStream->ReadBuffer(&clr.u32, 4);
				rclr.u8[0] = clr.u8[2];
				rclr.u8[1] = clr.u8[1];
				rclr.u8[2] = clr.u8[0];
				rclr.u8[3] = clr.u8[3];
				palette[j] = rclr.u32;
			}
		}
		/* Read the surface pixels.  Note that the bmp image is upside down */
		int pitch = bmhdr.biWidth * 4;
		tjs_uint8 *bits = (tjs_uint8 *)&pixbuf[0] + (bmhdr.biHeight * pitch);
		switch (bmhdr.biBitCount) {
		case 1:
		case 4:
		case 8:
			while (bits > (tjs_uint8 *)&pixbuf[0]) {
				bits -= pitch;
				tjs_uint8 pixel = 0;
				int shift = (8 - bmhdr.biBitCount);
				for (i = 0; i < bmhdr.biWidth; ++i) {
					if (i % (8 / bmhdr.biBitCount) == 0) {
						pStream->ReadBuffer(&pixel, 1);
					}
					*((tjs_uint32 *)bits + i) = (palette[pixel >> shift]);
					pixel <<= bmhdr.biBitCount;
				}
			}
			/* Skip padding bytes, ugh */
			if (pad) {
				tjs_uint8 padbyte;
				for (i = 0; i < pad; ++i) {
					pStream->ReadBuffer(&padbyte, 1);
				}
			}
			break;
		case 32:
			bmpPitch = bmhdr.biWidth * 4;
			pad = 0;
			while (bits > (tjs_uint8 *) &pixbuf[0]) {
				bits -= pitch;
				pStream->ReadBuffer(bits, pitch);
			}
			break;
		}

		/* Read the mask pixels.  Note that the bmp image is upside down */
		bits = (tjs_uint8 *)&pixbuf[0] + (bmhdr.biHeight * pitch);
		bmpPitch = (bmhdr.biWidth + 7) >> 3;
		pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
		while (bits > (tjs_uint8 *)&pixbuf[0]) {
			tjs_uint8 pixel = 0;
			int shift = (8 - 1);
			bits -= pitch;
			for (i = 0; i < bmhdr.biWidth; ++i) {
				if (i % (8 / 1) == 0) {
					pStream->ReadBuffer(&pixel, 1);
				}
				*((tjs_uint32 *)bits + i) |= ((pixel >> shift) ? 0 : 0xFF000000);
				pixel <<= 1;
			}
			/* Skip padding bytes, ugh */
			if (pad) {
				tjs_uint8 padbyte;
				for (i = 0; i < pad; ++i) {
					pStream->ReadBuffer(&padbyte, 1);
				}
			}
		}
		cocos2d::Image *surface = new cocos2d::Image;
		surface->initWithRawData(&pixbuf[0], pixbuf.size(), bmhdr.biWidth, bmhdr.biHeight, 32, false);
		Texture2D *tex = new Texture2D();
		tex->initWithImage(surface);
		Sprite *sprite = Sprite::create();
		sprite->setTexture(tex);
		sprite->setTextureRect(Rect(0, 0, bmhdr.biWidth, bmhdr.biHeight));
		sprite->setAnchorPoint(Vec2((float)entry.wHotSpotX / bmhdr.biWidth, 1.f - (float)entry.wHotSpotY / bmhdr.biHeight));
		float scale = sqrtf(Device::getDPI() / 150.f);
		sprite->setScale(scale);
		return sprite; // only the first one
	}
	return nullptr;
}

tTVPCursor *TVPLoadCursorANI(tTJSBinaryStream *pStream) {
	// TODO http://www.gdgsoft.com/anituner/help/aniformat.htm
	return nullptr;
}

tTVPCursor *TVPLoadCursor(tTJSBinaryStream *stream) {
	if (!stream) return nullptr;
	unsigned char sig[4];
	stream->Read(sig, 4);
	stream->SetPosition(0);
	if (memcmp(sig, "RIFF", 4) == 0) { // ani format
		return TVPLoadCursorANI(stream);
	} else { // cur format
		Sprite * cur = TVPLoadCursorCUR(stream);
		tTVPCursor * ret = nullptr;
		if (cur) {
			ret = new tTVPCursor;
			ret->RootNode = cur;
			cur->retain();
		}
		return ret;
	}
}

tTVPMouseButton TVP_TMouseButton_To_tTVPMouseButton(int button) {
	return (tTVPMouseButton)button;
}
// instead of TTVPWindowForm
class TVPWindowLayer : public cocos2d::extension::ScrollView, public iWindowLayer {
	typedef cocos2d::extension::ScrollView inherit;
	tTJSNI_Window *TJSNativeInstance;
	tjs_int ActualZoomDenom; // Zooming factor denominator (actual)
	tjs_int ActualZoomNumer; // Zooming factor numerator (actual)
	Sprite *DrawSprite = nullptr;
	Node *PrimaryLayerArea = nullptr;
	int LayerWidth = 0, LayerHeight = 0;
	//iTVPTexture2D *DrawTexture = nullptr;
	TVPWindowLayer *_prevWindow, *_nextWindow;
	friend class TVPWindowManagerOverlay;
	friend class TVPMainScene;
	int _LastMouseX = 0, _LastMouseY = 0;
	std::string _caption;
//	std::map<tTJSNI_BaseVideoOverlay*, Sprite*> _AllOverlay;
	float _drawSpriteScaleX = 1.0f, _drawSpriteScaleY = 1.0f;
	float _drawTextureScaleX = 1.f, _drawTextureScaleY = 1.f;
	bool UseMouseKey = false, MouseLeftButtonEmulatedPushed = false, MouseRightButtonEmulatedPushed = false;
	bool LastMouseMoved = false, Visible = false;
	tjs_uint32 LastMouseKeyTick = 0;
	tjs_int MouseKeyXAccel = 0;
	tjs_int MouseKeyYAccel = 0;
	int LastMouseDownX = 0, LastMouseDownY = 0;
	VelocityTrackers TouchVelocityTracker;
	VelocityTracker MouseVelocityTracker;
	static const int TVP_MOUSE_MAX_ACCEL = 30;
	static const int TVP_MOUSE_SHIFT_ACCEL = 40;
	static const int TVP_TOOLTIP_SHOW_DELAY = 500;

public:
	TVPWindowLayer(tTJSNI_Window *w)
		: TJSNativeInstance(w)
	{
		_nextWindow = nullptr;
		_prevWindow = _lastWindowLayer;
		_lastWindowLayer = this;
		ActualZoomDenom = 1;
		ActualZoomNumer = 1;
		if (_prevWindow) {
			_prevWindow->_nextWindow = this;
		}
	}

	virtual ~TVPWindowLayer() {
		if (_lastWindowLayer == this) _lastWindowLayer = _prevWindow;
		if (_nextWindow) _nextWindow->_prevWindow = _prevWindow;
		if (_prevWindow) _prevWindow->_nextWindow = _nextWindow;

		if (_currentWindowLayer == this) {
			TVPWindowLayer *anotherWin = _lastWindowLayer;
			while (anotherWin && !anotherWin->isVisible()) {
				anotherWin = anotherWin->_prevWindow;
			}
			if (anotherWin && anotherWin->isVisible()) {
				anotherWin->setPosition(0, 0);
				//anotherWin->setVisible(true);
			}
			_currentWindowLayer = anotherWin;
		}
	}

	bool init() {
		bool ret = inherit::init();
		setClippingToBounds(false);
		DrawSprite = Sprite::create();
		DrawSprite->setAnchorPoint(Vec2(0, 1)); // top-left
		PrimaryLayerArea = Node::create();
		addChild(PrimaryLayerArea);
		PrimaryLayerArea->addChild(DrawSprite);
		setAnchorPoint(Size::ZERO);
		EventListenerMouse *evmouse = EventListenerMouse::create();
		evmouse->onMouseScroll = std::bind(&TVPWindowLayer::onMouseScroll, this, std::placeholders::_1);
		evmouse->onMouseDown = std::bind(&TVPWindowLayer::onMouseDownEvent, this, std::placeholders::_1);
		evmouse->onMouseUp = std::bind(&TVPWindowLayer::onMouseUpEvent, this, std::placeholders::_1);
		evmouse->onMouseMove = std::bind(&TVPWindowLayer::onMouseMoveEvent, this, std::placeholders::_1);
		_eventDispatcher->addEventListenerWithSceneGraphPriority(evmouse, this);
		setTouchEnabled(false);
		//_touchListener->setSwallowTouches(true);
		setVisible(false);
		return ret;
	}

	static TVPWindowLayer *create(tTJSNI_Window *w) {
		TVPWindowLayer *ret = new TVPWindowLayer (w);
		ret->init();
		ret->autorelease();
		return ret;
	}

	virtual cocos2d::Node *GetPrimaryArea() override {
		return PrimaryLayerArea;
	}

	virtual Vec2 minContainerOffset() override {
		const Size &size = getContentSize();
		float scale = _container->getScale();
		Vec2 ret(_viewSize.width - size.width * scale * _drawSpriteScaleX,
			_viewSize.height - size.height * scale * _drawSpriteScaleY);

		if (ret.x > 0) {
			ret.x /= 2;
		} else {
			//ret.x = 0;
		}
		if (ret.y > 0) {
			ret.y /= 2;
		} else {
			//ret.y = 0;
		}
		return ret;
	}

	virtual Vec2 maxContainerOffset() override {
		// bottom-left
		const Size &size = getContentSize();
		float scale = _container->getScale();
		Vec2 ret(_viewSize.width - size.width * scale * _drawSpriteScaleX,
			_viewSize.height - size.height * scale * _drawSpriteScaleY);
		if (ret.x > 0) {
			ret.x /= 2;
		} else {
			ret.x = 0;
		}
		if (ret.y > 0) {
			ret.y /= 2;
		} else {
			ret.y = 0;
		}
		return ret;
	}

	void onMouseDownEvent(Event *_e) {
		EventMouse *e = static_cast<EventMouse*>(_e);
		int btn = e->getMouseButton();
		switch (btn) {
		case 1:
			_mouseBtn = mbRight;
			onMouseDown(e->getLocation());
			break;
		case 2:
			_mouseBtn = mbMiddle;
			onMouseDown(e->getLocation());
			break;
		default:
			break;
		}
	}

	void onMouseUpEvent(Event *_e) {
		EventMouse *e = static_cast<EventMouse*>(_e);
		int btn = e->getMouseButton();
		switch (btn) {
		case 1:
			_mouseBtn = mbRight;
			onMouseUp(e->getLocation());
			break;
		case 2:
			_mouseBtn = mbMiddle;
			onMouseUp(e->getLocation());
			break;
		default:
			break;
		}
	}

	void onMouseMoveEvent(Event *_e) {
		if (!_virutalMouseMode && _currentWindowLayer == this && !_touchMoved) {
			EventMouse *e = static_cast<EventMouse*>(_e);
			Vec2 pt(e->getCursorX(), e->getCursorY());
			onMouseMove(pt);
		}
	}

	void onMouseScroll(Event *_e) {
		EventMouse *e = static_cast<EventMouse*>(_e);
		if (!_windowMgrOverlay) {
			Vec2 nsp = PrimaryLayerArea->convertToNodeSpace(e->getLocation());
			int X = nsp.x, Y = PrimaryLayerArea->getContentSize().height - nsp.y;
			TJSNativeInstance->OnMouseWheel(TVPGetCurrentShiftKeyState(), e->getScrollY() > 0 ? -120 : 120, X, Y);
			return;
		}
		float scale = getZoomScale();
		if (e->getScrollY() > 0) {
			scale *= 0.9f;
		} else {
			scale *= 1.1f;
		}
		setZoomScale(scale);
		setContentOffset(getContentOffset());
		updateInset();
		relocateContainer(false);
	}

	virtual bool onTouchBegan(Touch *touch, Event *unused_event) override {
		if (_windowMgrOverlay) return inherit::onTouchBegan(touch, unused_event);
		if (std::find(_touches.begin(), _touches.end(), touch) == _touches.end())
		{
			_touches.push_back(touch);
		}
		switch (_touches.size()) {
		case 1:
			_touchPoint = touch->getLocation();
			_touchMoved = false;
			_touchLength = 0.0f;
			_touchBeginTick = TVPGetRoughTickCount32();
			_mouseBtn = ::mbLeft;
			break;
		case 2:
			_mouseBtn = ::mbRight;
			_touchPoint = (_touchPoint + touch->getLocation()) / 2;
			break;
		case 3:
			_mouseBtn = ::mbMiddle;
			//_touchPoint = (_touchPoint + touch->getLocation()) / 2;
		default:
			break;
		}
		return true;
	}

	virtual void onTouchMoved(Touch* touch, Event* unused_event) override {
		if (_windowMgrOverlay) return inherit::onTouchMoved(touch, unused_event);
		if (TJSNativeInstance) {
			if (_touches.size() == 1) {
				if (!_touchMoved && (TVPGetRoughTickCount32() - _touchBeginTick > 150 || _touchPoint.getDistanceSq(touch->getLocation()) > _touchMoveThresholdSq)) {
					Vec2 nsp = PrimaryLayerArea->convertToNodeSpace(_touchPoint);
					_LastMouseX = nsp.x, _LastMouseY = PrimaryLayerArea->getContentSize().height - nsp.y;
					TVPPostInputEvent(new tTVPOnMouseMoveInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, TVPGetCurrentShiftKeyState()));
					_scancode[_ConvertMouseBtnToVKCode(_mouseBtn)] = 0x11;
					TVPPostInputEvent(new tTVPOnMouseDownInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, _mouseBtn, TVPGetCurrentShiftKeyState()));
					_touchMoved = true;
				} else if (_touchMoved) {
					Vec2 nsp = PrimaryLayerArea->convertTouchToNodeSpace(touch);
					_LastMouseX = nsp.x, _LastMouseY = PrimaryLayerArea->getContentSize().height - nsp.y;
					TVPPostInputEvent(new tTVPOnMouseMoveInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, TVPGetCurrentShiftKeyState()), TVP_EPT_DISCARDABLE);
					int pos = (_LastMouseY << 16) + _LastMouseX;
					TVPPushEnvironNoise(&pos, sizeof(pos));
				}
			}
		}
	}

	virtual void onTouchEnded(Touch *touch, Event *unused_event) override {
		if (_windowMgrOverlay) return inherit::onTouchEnded(touch, unused_event);
		auto touchIter = std::find(_touches.begin(), _touches.end(), touch);

		if (touchIter != _touches.end())
		{
			if (_touches.size() == 1)
			{
				if (TJSNativeInstance) {
					Vec2 nsp = PrimaryLayerArea->convertTouchToNodeSpace(touch);
					_LastMouseX = nsp.x, _LastMouseY = PrimaryLayerArea->getContentSize().height - nsp.y;
					if (!_touchMoved) {
						TVPPostInputEvent(new tTVPOnMouseMoveInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, TVPGetCurrentShiftKeyState()));
						Vec2 nsp = PrimaryLayerArea->convertToNodeSpace(_touchPoint);
						TVPPostInputEvent(new tTVPOnMouseDownInputEvent(TJSNativeInstance, nsp.x,
							PrimaryLayerArea->getContentSize().height - nsp.y, _mouseBtn, TVPGetCurrentShiftKeyState()));
						TVPPostInputEvent(new tTVPOnClickInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY));
					}
					_scancode[_ConvertMouseBtnToVKCode(_mouseBtn)] = 0x10;
					TVPPostInputEvent(new tTVPOnMouseUpInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, _mouseBtn, TVPGetCurrentShiftKeyState()));
				}
			}
			_touches.erase(touchIter);
		}

		if (_touches.size() == 0)
		{
			_dragging = false;
			_touchMoved = false;
			_scancode[_ConvertMouseBtnToVKCode(_mouseBtn)] &= 0x10;
		}
	}

	virtual void onTouchCancelled(Touch *touch, Event *unused_event) override {
		if (_windowMgrOverlay) return inherit::onTouchCancelled(touch, unused_event);
		auto touchIter = std::find(_touches.begin(), _touches.end(), touch);
		if (touchIter != _touches.end())
		{
			_touches.erase(touchIter);
		}

		if (_touches.size() == 0)
		{
			_dragging = false;
			_touchMoved = false;

			if (TJSNativeInstance) {
				Vec2 nsp = PrimaryLayerArea->convertTouchToNodeSpace(touch);
				_LastMouseX = nsp.x, _LastMouseY = PrimaryLayerArea->getContentSize().height - nsp.y;
				TVPPostInputEvent(new tTVPOnMouseUpInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, _mouseBtn, TVPGetCurrentShiftKeyState()));
			}
		}
	}

	void onMouseDown(const Vec2 &pt) {
		Vec2 nsp = PrimaryLayerArea->convertToNodeSpace(pt);
		_LastMouseX = nsp.x, _LastMouseY = PrimaryLayerArea->getContentSize().height - nsp.y;
		_scancode[_ConvertMouseBtnToVKCode(_mouseBtn)] = 0x11;
		TVPPostInputEvent(new tTVPOnMouseDownInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, _mouseBtn, TVPGetCurrentShiftKeyState()));
	}

	void onMouseUp(const Vec2 &pt) {
		Vec2 nsp = PrimaryLayerArea->convertToNodeSpace(pt);
		_LastMouseX = nsp.x, _LastMouseY = PrimaryLayerArea->getContentSize().height - nsp.y;
		_scancode[_ConvertMouseBtnToVKCode(_mouseBtn)] &= 0x10;
		TVPPostInputEvent(new tTVPOnMouseUpInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, _mouseBtn, TVPGetCurrentShiftKeyState()));
	}

	void onMouseMove(const Vec2 &pt) {
		Vec2 nsp = PrimaryLayerArea->convertToNodeSpace(pt);
		_LastMouseX = nsp.x, _LastMouseY = PrimaryLayerArea->getContentSize().height - nsp.y;
		TVPPostInputEvent(new tTVPOnMouseMoveInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, TVPGetCurrentShiftKeyState()), TVP_EPT_DISCARDABLE);
		int pos = (_LastMouseY << 16) + _LastMouseX;
		TVPPushEnvironNoise(&pos, sizeof(pos));
	}

	void onMouseClick(const Vec2 &pt) {
		Vec2 nsp = PrimaryLayerArea->convertToNodeSpace(pt);
		_LastMouseX = nsp.x, _LastMouseY = PrimaryLayerArea->getContentSize().height - nsp.y;
		TVPPostInputEvent(new tTVPOnMouseMoveInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, TVPGetCurrentShiftKeyState()), TVP_EPT_DISCARDABLE);
		_scancode[_ConvertMouseBtnToVKCode(_mouseBtn)] = 0x10;
		TVPPostInputEvent(new tTVPOnMouseDownInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, _mouseBtn, TVPGetCurrentShiftKeyState()));
		TVPPostInputEvent(new tTVPOnClickInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY));
		TVPPostInputEvent(new tTVPOnMouseUpInputEvent(TJSNativeInstance, _LastMouseX, _LastMouseY, _mouseBtn, TVPGetCurrentShiftKeyState()));
	}

	virtual void SetPaintBoxSize(tjs_int w, tjs_int h) {
		LayerWidth = w; LayerHeight = h;
		RecalcPaintBox();
	}

	virtual bool GetFormEnabled() {
		return isVisible();
	}

	virtual void SetDefaultMouseCursor() {
		;
	}
	virtual void GetCursorPos(tjs_int &x, tjs_int &y) {
		x = _LastMouseX;
		y = _LastMouseY;
	}

	virtual void SetCursorPos(tjs_int x, tjs_int y) {
		Vec2 worldPt = PrimaryLayerArea->convertToWorldSpace(Vec2(x, PrimaryLayerArea->getContentSize().height - y));
		Vec2 pt = getParent()->convertToNodeSpace(worldPt);
		_LastMouseX = pt.x;
		_LastMouseY = pt.y;
		if (_mouseCursor) {
			_mouseCursor->setPosition(pt);
			_refadeMouseCursor();
		}
	}

	virtual void SetHintText(const ttstr &text) {

	}

	tjs_int _textInputPosY;

	virtual void SetAttentionPoint(tjs_int left, tjs_int top, const struct tTVPFont * font) override {
		_textInputPosY = top;
	}

	virtual void SetImeMode(tTVPImeMode mode) {
		switch (mode) {
		case ::imDisable:
		case ::imClose:
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
			TVPHideIME();
#else
//#ifdef _MSC_VER
			TVPMainScene::GetInstance()->detachWithIME();
#endif
			break;
		case ::imOpen:
			//TVPMainScene::GetInstance()->attachWithIME();
			//break;
		default:
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
		{
			Size screenSize = cocos2d::Director::getInstance()->getOpenGLView()->getFrameSize();
			TVPShowIME(0, _textInputPosY, screenSize.width, screenSize.height / 4);
		}
#else
//#ifdef _MSC_VER
			TVPMainScene::GetInstance()->attachWithIME();
#endif
			break;
		}
	}

	virtual void ZoomRectangle(
		tjs_int & left, tjs_int & top,
		tjs_int & right, tjs_int & bottom) {
		left = tjs_int64(left) * ActualZoomNumer / ActualZoomDenom;
		top = tjs_int64(top) * ActualZoomNumer / ActualZoomDenom;
		right = tjs_int64(right) * ActualZoomNumer / ActualZoomDenom;
		bottom = tjs_int64(bottom) * ActualZoomNumer / ActualZoomDenom;
	}

	virtual void BringToFront() {
		if (_currentWindowLayer != this) {
			if (_currentWindowLayer) {
				const Size &size = _currentWindowLayer->getViewSize();
				_currentWindowLayer->setPosition(Vec2(size.width, 0));
				_currentWindowLayer->TJSNativeInstance->OnReleaseCapture();
			}
			_currentWindowLayer = this;
		}
	}

	virtual void ShowWindowAsModal() {
		in_mode_ = true;
		setVisible(true);
		BringToFront();
		if (_consoleWin) {
			_consoleWin->removeFromParent();
			_consoleWin = nullptr;
			TVPMainScene::GetInstance()->scheduleUpdate();

			cocos2d::Director::getInstance()->purgeCachedData();
			TVPControlAdDialog(0x10002, 0, 0); // ensure to close banner ad
		}
		Director* director = Director::getInstance();
		modal_result_ = 0;
		while (this == _currentWindowLayer && !modal_result_) {
			int remain = TVPDrawSceneOnce(30); // 30 fps
			TVPProcessInputEvents(); // for iOS
			if (::Application->IsTarminate()) {
				modal_result_ = mrCancel;
			} else if (modal_result_ != 0) {
				break;
			} else if (remain > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(remain));
			}
		}
		in_mode_ = false;
	}

	virtual bool GetVisible() {
		return isVisible();
	}

	virtual void SetVisible(bool bVisible) {
		Visible = bVisible;
		setVisible(bVisible);
		if (bVisible) {
			BringToFront();
		} else {
			if (_currentWindowLayer == this) {
				_currentWindowLayer = _prevWindow ? _prevWindow : _nextWindow;
			}
		}
	}

	virtual const char *GetCaption() {
		return _caption.c_str();
	}

	virtual void SetCaption(const std::string &s) {
		_caption = s;
	}

	void ResetDrawSprite() {
		if (DrawSprite) {
			Size size = getContentSize();
			float scale = (float)ActualZoomNumer / ActualZoomDenom;
#ifdef _DEBUG
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
			printf("reset sprite: size=(%f,%f), Numer=%d, Denom=%d Layer=(%d,%d)\n",
				size.width, size.height, ActualZoomNumer, ActualZoomDenom, LayerWidth, LayerHeight);
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE);
			if (!LayerWidth || !LayerHeight) {
				LayerHeight = LayerHeight;
			}
#endif
			size = size / scale;
			//DrawSprite->setTextureRect(Rect(0, 0, size.width, size.height));
			DrawSprite->setScale(_drawTextureScaleX, _drawTextureScaleY);
			DrawSprite->setTextureRect(Rect(0, 0, LayerWidth, LayerHeight));
			DrawSprite->setPosition(Vec2(0, size.height));
			PrimaryLayerArea->setContentSize(size);
			PrimaryLayerArea->setScale(scale);
		}
	}
	void RecalcPaintBox() {
		if (!LayerWidth || !LayerHeight) return;
		ResetDrawSprite();
		Size size = getViewSize();
		Size contSize = getContentSize();
		float r = size.width / size.height;
		float R = contSize.width / contSize.height;
		float scale; Vec2 offset;
		if (R > r) {
			scale = size.width / contSize.width;
 			offset.x = 0;
			offset.y = (size.height - contSize.height * scale) / 2;
		} else {
			scale = size.height / contSize.height;
			offset.x = (size.width - contSize.width * scale) / 2;
 			offset.y = 0;
		}
		setMinScale(scale);
		setMaxScale(scale * 2);
		setZoomScale(scale);
		setContentOffset(offset);
		updateInset();
	}
	virtual void SetWidth(tjs_int w) {
		Size size = getContentSize();
		size.width = w;
		setContentSize(size);
		RecalcPaintBox();
	}
	virtual void SetHeight(tjs_int h) {
		Size size = getContentSize();
		size.height = h;
		setContentSize(size);
		RecalcPaintBox();
	}
	virtual void SetSize(tjs_int w, tjs_int h) {
		setContentSize(Size(w,h));
		RecalcPaintBox();
	}
	virtual void GetSize(tjs_int &w, tjs_int &h) {
		Size size = getContentSize();
		w = size.width; h = size.height;
	}
	virtual void GetWinSize(tjs_int &w, tjs_int &h) {
		Size size = getViewSize();
		w = size.width; h = size.height;
	}
	virtual tjs_int GetWidth() const override { return getContentSize().width; }
	virtual tjs_int GetHeight() const override { return getContentSize().height; }
	virtual void SetZoom(tjs_int numer, tjs_int denom) {
		AdjustNumerAndDenom(numer, denom);
		ZoomNumer = numer;
		ZoomDenom = denom;
		ActualZoomDenom = denom;
		ActualZoomNumer = numer;
		RecalcPaintBox();
	}

	virtual void UpdateDrawBuffer(iTVPTexture2D *tex) {
		if (!tex) return;
//		iTVPRenderManager *mgr = TVPGetRenderManager();
// 		if (!mgr->IsSoftware()) {
// 			static iTVPRenderMethod *method = TVPGetRenderManager()->GetRenderMethod("CopyOpaqueImage");
// 			Size size = DrawSprite->getContentSize();
// 			tTVPRect rctar(0, 0, size.width, size.height);
// 			if (!DrawTexture) {
// 				DrawTexture = mgr->CreateTexture2D(nullptr, 0, size.width, size.height, TVPTextureFormat::RGBA);
// 			} else {
// 				DrawTexture->SetSize(size.width, size.height);
// 			}
// 			std::pair<iTVPTexture2D*, tTVPRect> src_tex[] = {
// 				std::pair<iTVPTexture2D*, tTVPRect>(tex, rctar)
// 			};
// 			mgr->OperateRect(method, DrawTexture, nullptr, rctar, src_tex);
// 			tex = DrawTexture;
// 		}
		Texture2D *tex2d = DrawSprite->getTexture();
		Texture2D *newtex = tex->GetAdapterTexture(tex2d);
		if (tex2d != newtex) {
			DrawSprite->setTexture(newtex);
			float sw, sh;
			tex->GetScale(_drawTextureScaleX, _drawTextureScaleY);
			if (_drawTextureScaleX == 1.f) sw = LayerWidth;// tex->GetWidth();
			else {
				sw = tex->GetInternalWidth() * ((float)LayerWidth / tex->GetWidth());
				_drawTextureScaleX = 1 / _drawTextureScaleX;
			}
			if (_drawTextureScaleY == 1.f) sh = LayerHeight;// tex->GetHeight();
			else {
				sh = tex->GetInternalHeight()* ((float)LayerHeight / tex->GetHeight());
				_drawTextureScaleY = 1 / _drawTextureScaleY;
			}
			DrawSprite->setTextureRect(Rect(0, 0, sw, sh));
			DrawSprite->setBlendFunc(BlendFunc::DISABLE);
			ResetDrawSprite();
		}
	}

	tTJSNI_Window* GetWindow() { return TJSNativeInstance; }
#if 0
	virtual void AddOverlay(tTJSNI_BaseVideoOverlay *ovl) {
		if (_AllOverlay.find(ovl) != _AllOverlay.end()) return;
		Sprite *pSprite = Sprite::create();
		cocos2d::Texture2D* pTex = new cocos2d::Texture2D;
		pSprite->setTexture(pTex);
		//pSprite->setFlippedY(true);
		pSprite->setAnchorPoint(Vec2(0, 1));
		PrimaryLayerArea->addChild(pSprite);
		_AllOverlay[ovl] = pSprite;
	}
	virtual void RemoveOverlay(tTJSNI_BaseVideoOverlay *ovl) {
		auto it = _AllOverlay.find(ovl);
		if (it == _AllOverlay.end()) return;
		it->second->removeFromParent();
		_AllOverlay.erase(it);
	}
	virtual void UpdateOverlay() {
		for (auto it : _AllOverlay) {
			Sprite *pSprite = it.second;
			if (!it.first->GetVisible()) {
				pSprite->setVisible(false);
				continue;
			} else {
				pSprite->setVisible(true);
			}
			tjs_int w, h;
			if (!it.first->GetVideoSize(w, h)) continue;
			Size videoSize(w, h);
			cocos2d::Texture2D *pTex = pSprite->getTexture();
			const Size &size = pTex->getContentSize();
			std::function<void(const void*, int, int, int)> drawer;
			if (size.width != videoSize.width || size.height != videoSize.height) {
				if (size.width < videoSize.width || size.height < videoSize.height) {
					drawer = [pTex, pSprite](const void* data, int pitch, int w, int h) {
						pTex->initWithData(data, pitch * h, cocos2d::Texture2D::PixelFormat::RGBA8888,
							w, h, cocos2d::Size::ZERO);
						pSprite->setTextureRect(Rect(0, 0, w, h));
					};
				} else {
					pSprite->setTextureRect(Rect(0, 0, videoSize.width, videoSize.height));
				}
			}
			if (!drawer) {
				drawer = [pTex](const void* data, int pitch, int w, int h) {
					pTex->updateWithData(data, 0, 0, w, h);
				};
			}
			if (!it.first->DrawToTexture(drawer)) continue;
			const tTVPRect & rc = it.first->GetBounds();
			float scaleX = rc.get_width() / videoSize.width;
			float scaleY = rc.get_height() / videoSize.height;
			if (scaleX != pSprite->getScaleX())
				pSprite->setScaleX(scaleX);
			if(scaleY != pSprite->getScaleY())
				pSprite->setScaleY(scaleY);
			Vec2 pos = pSprite->getPosition();
			int top = PrimaryLayerArea->getContentSize().height - rc.top;
			if ((int)pos.x != rc.left || (int)pos.y != top) {
				pSprite->setPosition(rc.left, top);
			}
		}
	}
#endif
	void toogleFillScale() {
		float scaleX = PrimaryLayerArea->getScaleX();
		float scaleY = PrimaryLayerArea->getScaleY();
		const Size &drawSize = PrimaryLayerArea->getContentSize();
		Size viewSize = getViewSize();
		float R = viewSize.width / viewSize.height;
		float r = drawSize.width / drawSize.height;
		if (fabs(R - r) < 0.01) {
			return; // do not fill border if screen ratio is almost the same
		}

		if (scaleX == scaleY) {
			if (R > r) { // border @ left/right
				_drawSpriteScaleX = R / r;
				PrimaryLayerArea->setScaleX(scaleY * _drawSpriteScaleX);
			} else { // border @ top/bottom
				_drawSpriteScaleY = r / R;
				PrimaryLayerArea->setScaleY(scaleX * _drawSpriteScaleY);
			}
		} else {
			PrimaryLayerArea->setScale(std::min(scaleX, scaleY));
			_drawSpriteScaleX = 1.0f;
			_drawSpriteScaleY = 1.0f;
		}

		updateInset();
		setContentOffset(Vec2::ZERO);
		relocateContainer(false);
	}

	virtual void InvalidateClose() override {
		// closing action by object invalidation;
		// this will not cause any user confirmation of closing the window.

		//TVPRemoveWindowLayer(this);
		this->removeFromParent(); // and delete this
	}
	virtual bool GetWindowActive() override {
		return _currentWindowLayer == this;
	}
	int GetMouseButtonState() const {
		int s = 0;
		if (TVPGetAsyncKeyState(VK_LBUTTON)) s |= ssLeft;
		if (TVPGetAsyncKeyState(VK_RBUTTON)) s |= ssRight;
		if (TVPGetAsyncKeyState(VK_MBUTTON)) s |= ssMiddle;
		return s;
	}
	void OnMouseDown(int button, int shift, int x, int y) {
		//if (!CanSendPopupHide()) DeliverPopupHide();

		MouseVelocityTracker.addMovement(TVPGetRoughTickCount32(), (float)x, (float)y);

		LastMouseDownX = x;
		LastMouseDownY = y;

		if (TJSNativeInstance) {
			tjs_uint32 s = shift;
			s |= GetMouseButtonState();
			tTVPMouseButton b = TVP_TMouseButton_To_tTVPMouseButton(button);
			TVPPostInputEvent(new tTVPOnMouseDownInputEvent(TJSNativeInstance, x, y, b, s));
		}
	}
	void OnMouseClick(int button, int shift, int x, int y) {
		// fire click event
		if (TJSNativeInstance) {
			TVPPostInputEvent(new tTVPOnClickInputEvent(TJSNativeInstance, LastMouseDownX, LastMouseDownY));
		}
	}

	void GenerateMouseEvent(bool fl, bool fr, bool fu, bool fd) {
		if (!fl && !fr && !fu && !fd) {
			if (TVPGetRoughTickCount32() - 45 < LastMouseKeyTick) return;
		}

		bool shift = 0 != (TVPGetKeyMouseAsyncState(VK_SHIFT, true));
		bool left = fl || TVPGetKeyMouseAsyncState(VK_LEFT, true) || TVPGetJoyPadAsyncState(VK_PADLEFT, true);
		bool right = fr || TVPGetKeyMouseAsyncState(VK_RIGHT, true) || TVPGetJoyPadAsyncState(VK_PADRIGHT, true);
		bool up = fu || TVPGetKeyMouseAsyncState(VK_UP, true) || TVPGetJoyPadAsyncState(VK_PADUP, true);
		bool down = fd || TVPGetKeyMouseAsyncState(VK_DOWN, true) || TVPGetJoyPadAsyncState(VK_PADDOWN, true);

		uint32_t flags = 0;
		if (left || right || up || down) flags |= /*MOUSEEVENTF_MOVE*/1;

		if (!right && !left && !up && !down) {
			LastMouseMoved = false;
			MouseKeyXAccel = MouseKeyYAccel = 0;
		}

		if (!shift) {
			if (!right && left && MouseKeyXAccel > 0) MouseKeyXAccel = -0;
			if (!left && right && MouseKeyXAccel < 0) MouseKeyXAccel = 0;
			if (!down && up && MouseKeyYAccel > 0) MouseKeyYAccel = -0;
			if (!up && down && MouseKeyYAccel < 0) MouseKeyYAccel = 0;
		} else {
			if (left) MouseKeyXAccel = -TVP_MOUSE_SHIFT_ACCEL;
			if (right) MouseKeyXAccel = TVP_MOUSE_SHIFT_ACCEL;
			if (up) MouseKeyYAccel = -TVP_MOUSE_SHIFT_ACCEL;
			if (down) MouseKeyYAccel = TVP_MOUSE_SHIFT_ACCEL;
		}

		if (right || left || up || down) {
			if (left) if (MouseKeyXAccel > -TVP_MOUSE_MAX_ACCEL)
				MouseKeyXAccel = MouseKeyXAccel ? MouseKeyXAccel - 2 : -2;
			if (right) if (MouseKeyXAccel < TVP_MOUSE_MAX_ACCEL)
				MouseKeyXAccel = MouseKeyXAccel ? MouseKeyXAccel + 2 : +2;
			if (!left && !right) {
				if (MouseKeyXAccel > 0) MouseKeyXAccel--;
				else if (MouseKeyXAccel < 0) MouseKeyXAccel++;
			}

			if (up) if (MouseKeyYAccel > -TVP_MOUSE_MAX_ACCEL)
				MouseKeyYAccel = MouseKeyYAccel ? MouseKeyYAccel - 2 : -2;
			if (down) if (MouseKeyYAccel < TVP_MOUSE_MAX_ACCEL)
				MouseKeyYAccel = MouseKeyYAccel ? MouseKeyYAccel + 2 : +2;
			if (!up && !down) {
				if (MouseKeyYAccel > 0) MouseKeyYAccel--;
				else if (MouseKeyYAccel < 0) MouseKeyYAccel++;
			}

		}

		if (flags) {
			_LastMouseX += MouseKeyXAccel >> 1;
			_LastMouseY += MouseKeyYAccel >> 1;
			LastMouseMoved = true;
		}
		LastMouseKeyTick = TVPGetRoughTickCount32();
	}

	virtual void InternalKeyDown(tjs_uint16 key, tjs_uint32 shift) override {
		tjs_uint32 tick = TVPGetRoughTickCount32();
		TVPPushEnvironNoise(&tick, sizeof(tick));
		TVPPushEnvironNoise(&key, sizeof(key));
		TVPPushEnvironNoise(&shift, sizeof(shift));

		if (UseMouseKey) {
			if (key == VK_RETURN || key == VK_SPACE || key == VK_ESCAPE || key == VK_PAD1 || key == VK_PAD2) {
				Vec2 p(_LastMouseX, _LastMouseY);
				Size size = PrimaryLayerArea->getContentSize();
				if (p.x >= 0 && p.y >= 0 && p.x < size.width && p.y < size.height) {
					if (key == VK_RETURN || key == VK_SPACE || key == VK_PAD1) {
						MouseLeftButtonEmulatedPushed = true;
						OnMouseDown(mbLeft, 0, p.x, p.y);
					}

					if (key == VK_ESCAPE || key == VK_PAD2) {
						MouseRightButtonEmulatedPushed = true;
						OnMouseDown(mbLeft, 0, p.x, p.y);
					}
				}
				return;
			}

			switch (key) {
			case VK_LEFT:
			case VK_PADLEFT:
				if (MouseKeyXAccel == 0 && MouseKeyYAccel == 0) {
					GenerateMouseEvent(true, false, false, false);
					LastMouseKeyTick = TVPGetRoughTickCount32() + 100;
				}
				return;
			case VK_RIGHT:
			case VK_PADRIGHT:
				if (MouseKeyXAccel == 0 && MouseKeyYAccel == 0)
				{
					GenerateMouseEvent(false, true, false, false);
					LastMouseKeyTick = TVPGetRoughTickCount32() + 100;
				}
				return;
			case VK_UP:
			case VK_PADUP:
				if (MouseKeyXAccel == 0 && MouseKeyYAccel == 0)
				{
					GenerateMouseEvent(false, false, true, false);
					LastMouseKeyTick = TVPGetRoughTickCount32() + 100;
				}
				return;
			case VK_DOWN:
			case VK_PADDOWN:
				if (MouseKeyXAccel == 0 && MouseKeyYAccel == 0)
				{
					GenerateMouseEvent(false, false, false, true);
					LastMouseKeyTick = TVPGetRoughTickCount32() + 100;
				}
				return;
			}
		}
		TVPPostInputEvent(new tTVPOnKeyDownInputEvent(TJSNativeInstance, key, shift));
	}
	void InternalKeyUp(tjs_uint16 key, tjs_uint32 shift) {
		tjs_uint32 tick = TVPGetRoughTickCount32();
		TVPPushEnvironNoise(&tick, sizeof(tick));
		TVPPushEnvironNoise(&key, sizeof(key));
		TVPPushEnvironNoise(&shift, sizeof(shift));
		if (TJSNativeInstance) {
			if (UseMouseKey /*&& PaintBox*/) {
				if (key == VK_RETURN || key == VK_SPACE || key == VK_ESCAPE || key == VK_PAD1 || key == VK_PAD2) {
					Vec2 p(_LastMouseX, _LastMouseY);
					Size size = PrimaryLayerArea->getContentSize();
					if (p.x >= 0 && p.y >= 0 && p.x < size.width && p.y < size.height) {
						if (key == VK_RETURN || key == VK_SPACE || key == VK_PAD1) {
							OnMouseClick(mbLeft, 0, p.x, p.y);
							MouseLeftButtonEmulatedPushed = false;
							OnMouseUp(mbLeft, 0, p.x, p.y);
						}

						if (key == VK_ESCAPE || key == VK_PAD2) {
							MouseRightButtonEmulatedPushed = false;
							OnMouseUp(mbRight, 0, p.x, p.y);
						}
					}
					return;
				}
			}

			TVPPostInputEvent(new tTVPOnKeyUpInputEvent(TJSNativeInstance, key, shift));
		}
	}
	virtual void OnKeyUp(tjs_uint16 vk, int shift) override {
		tjs_uint32 s = (shift);
		s |= GetMouseButtonState();
		InternalKeyUp(vk, s);
	}
	virtual void OnKeyPress(tjs_uint16 vk, int repeat, bool prevkeystate, bool convertkey) override {
		if (TJSNativeInstance && vk) {
			if (UseMouseKey && (vk == 0x1b || vk == 13 || vk == 32)) return;
			// UNICODE 卅及匹公及引引傾仄化仄引丹
			TVPPostInputEvent(new tTVPOnKeyPressInputEvent(TJSNativeInstance, vk));
		}
	}
	tTVPImeMode LastSetImeMode = ::imDisable;
	tTVPImeMode DefaultImeMode = ::imDisable;
	virtual tTVPImeMode GetDefaultImeMode() const override { return DefaultImeMode; }
	virtual void ResetImeMode() override { SetImeMode(DefaultImeMode); }
	bool Closing = false, ProgramClosing = false, CanCloseWork = false;
	bool in_mode_ = false; // is modal
	int modal_result_ = 0;
	enum CloseAction {
		caNone,
		caHide,
		caFree,
		caMinimize
	};
	void OnClose(CloseAction& action) {
		if (modal_result_ == 0)
			action = caNone;
		else
			action = caHide;

		if (ProgramClosing) {
			if (TJSNativeInstance) {
				if (TJSNativeInstance->IsMainWindow()) {
					// this is the main window
				} else 			{
					// not the main window
					action = caFree;
				}
				//if (TVPFullScreenedWindow != this) {
					// if this is not a fullscreened window
				//	SetVisible(false);
				//}
				iTJSDispatch2 * obj = TJSNativeInstance->GetOwnerNoAddRef();
				TJSNativeInstance->NotifyWindowClose();
				obj->Invalidate(0, NULL, NULL, obj);
				TJSNativeInstance = NULL;
				SetVisible(false);
				scheduleOnce([this](float){removeFromParent(); }, 0, "remove");
			}
		}
	}
	bool OnCloseQuery() {
		// closing actions are 3 patterns;
		// 1. closing action by the user
		// 2. "close" method
		// 3. object invalidation

		if (TVPGetBreathing()) {
			return false;
		}

		// the default event handler will invalidate this object when an onCloseQuery
		// event reaches the handler.
		if (TJSNativeInstance && (modal_result_ == 0 ||
			modal_result_ == mrCancel/* mrCancel=when close button is pushed in modal window */)) {
			iTJSDispatch2 * obj = TJSNativeInstance->GetOwnerNoAddRef();
			if (obj) {
				tTJSVariant arg[1] = { true };
				static ttstr eventname(TJS_W("onCloseQuery"));

				if (!ProgramClosing) {
					// close action does not happen immediately
					if (TJSNativeInstance) {
						TVPPostInputEvent(new tTVPOnCloseInputEvent(TJSNativeInstance));
					}

					Closing = true; // waiting closing...
				//	TVPSystemControl->NotifyCloseClicked();
					return false;
				} else {
					CanCloseWork = true;
					TVPPostEvent(obj, obj, eventname, 0, TVP_EPT_IMMEDIATE, 1, arg);
					TVPDrawSceneOnce(0); // for post event
					// this event happens immediately
					// and does not return until done
					return CanCloseWork; // CanCloseWork is set by the event handler
				}
			} else {
				return true;
			}
		} else {
			return true;
		}
	}
	virtual void Close() override {
		// closing action by "close" method
		if (Closing) return; // already waiting closing...

		ProgramClosing = true;
		try {
			//tTVPWindow::Close();
			if (in_mode_) {
				modal_result_ = mrCancel;
			} else if (OnCloseQuery()) {
				CloseAction action = caFree;
				OnClose(action);
				switch (action) {
				case caNone:
					break;
				case caHide:
					Hide();
					break;
				case caMinimize:
					//::ShowWindow(GetHandle(), SW_MINIMIZE);
					break;
				case caFree:
				default:
					scheduleOnce([this](float){
						removeFromParent();
					}, 0, "Close");
					//::PostMessage(GetHandle(), TVP_EV_WINDOW_RELEASE, 0, 0);
					break;
				}
			}
		}
		catch (...) {
			ProgramClosing = false;
			throw;
		}
		ProgramClosing = false;
	}
	virtual void OnCloseQueryCalled(bool b) {
		// closing is allowed by onCloseQuery event handler
		if (!ProgramClosing) {
			// closing action by the user
			if (b) {
				if (in_mode_)
					modal_result_ = 1; // when modal
				else
					SetVisible(false);  // just hide

				Closing = false;
				if (TJSNativeInstance) {
					if (TJSNativeInstance->IsMainWindow()) {
						// this is the main window
						iTJSDispatch2 * obj = TJSNativeInstance->GetOwnerNoAddRef();
						obj->Invalidate(0, NULL, NULL, obj);
						// TJSNativeInstance = NULL; // 仇及僇蕆匹反暫卞this互祅壺今木化中月凶戶﹜丟件田奈尺失弁本旦仄化反中仃卅中
					}
				} else {
					delete this;
				}
			} else {
				Closing = false;
			}
		} else {
			// closing action by the program
			CanCloseWork = b;
		}
	}
	virtual void UpdateWindow(tTVPUpdateType type) {
		if (TJSNativeInstance) {
			tTVPRect r;
			r.left = 0;
			r.top = 0;
			r.right = LayerWidth;
			r.bottom = LayerHeight;
			TJSNativeInstance->NotifyWindowExposureToLayer(r);
			TVPDeliverWindowUpdateEvents();
		}
	}
	virtual void SetVisibleFromScript(bool b) {
		SetVisible(b);
// 		if (Focusable) {
// 			SetVisible(b);
// 		} else {
// 			if (!GetVisible()) {
// 				// just show window, not activate
// 				SetWindowPos(GetHandle(), GetStayOnTop() ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
// 				SetVisible(true);
// 			} else {
// 				SetVisible(false);
// 			}
// 		}
	}
	virtual void SetUseMouseKey(bool b) {
		UseMouseKey = b;
		if (b) {
			MouseLeftButtonEmulatedPushed = false;
			MouseRightButtonEmulatedPushed = false;
			LastMouseKeyTick = TVPGetRoughTickCount32();
		} else {
			if (MouseLeftButtonEmulatedPushed) {
				MouseLeftButtonEmulatedPushed = false;
				OnMouseUp(mbLeft, 0, _LastMouseX, _LastMouseY);
			}
			if (MouseRightButtonEmulatedPushed) {
				MouseRightButtonEmulatedPushed = false;
				OnMouseUp(mbRight, 0, _LastMouseX, _LastMouseY);
			}
		}
	}
	virtual bool GetUseMouseKey() const { return UseMouseKey; }
	void OnMouseUp(int button, int shift, int x, int y) {
	//	TranslateWindowToDrawArea(x, y);
	//	ReleaseMouseCapture();
		MouseVelocityTracker.addMovement(TVPGetRoughTickCount32(), (float)x, (float)y);
		if (TJSNativeInstance) {
			tjs_uint32 s = shift;
			s |= GetMouseButtonState();
			tTVPMouseButton b = TVP_TMouseButton_To_tTVPMouseButton(button);
			TVPPostInputEvent(new tTVPOnMouseUpInputEvent(TJSNativeInstance, x, y, b, s));
		}
	}
	virtual void ResetTouchVelocity(tjs_int id) {
		TouchVelocityTracker.end(id);
	}
	virtual void ResetMouseVelocity() {
		MouseVelocityTracker.clear();
	}
	bool GetMouseVelocity(float& x, float& y, float& speed) const {
		if (MouseVelocityTracker.getVelocity(x, y)) {
			speed = hypotf(x, y);
			return true;
		}
		return false;
	}

	virtual void TickBeat() override {
		bool focused = _currentWindowLayer == this;
		// mouse key
		if (UseMouseKey &&  focused) {
			GenerateMouseEvent(false, false, false, false);
		}
	}
};

tTJSNI_Window *TVPGetActiveWindow() {
	if (!_currentWindowLayer) return nullptr;
	return _currentWindowLayer->GetWindow();
}

class TVPWindowManagerOverlay : public iTVPBaseForm {
public:
	static TVPWindowManagerOverlay *create() {
		TVPWindowManagerOverlay* ret = new TVPWindowManagerOverlay();
		ret->autorelease();
		ret->initFromFile(nullptr, "ui/WinMgrOverlay.csb", nullptr);
		return ret;
	}
	virtual void rearrangeLayout() {
		Size sceneSize = TVPMainScene::GetInstance()->getGameNodeSize();
		setContentSize(sceneSize);
		RootNode->setContentSize(sceneSize);
		ui::Helper::doLayout(RootNode);
	}

	virtual void bindBodyController(const NodeMap &allNodes) {
		_left = static_cast<ui::Button*>(allNodes.findController("left"));
		_right = static_cast<ui::Button*>(allNodes.findController("right"));
		_ok = static_cast<ui::Button*>(allNodes.findController("ok"));

		auto funcUpdate = std::bind(&TVPWindowManagerOverlay::updateButtons, this);

		_left->addClickEventListener([=](Ref*){
			if (!_currentWindowLayer || !_currentWindowLayer->_prevWindow) return;
			//_currentWindowLayer->_prevWindow->setVisible(true);
			Size size = _currentWindowLayer->_prevWindow->getViewSize();
			_currentWindowLayer->_prevWindow->setPosition(-size.width, 0);
			_currentWindowLayer->_prevWindow->runAction(
				EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
			_currentWindowLayer->runAction(Sequence::createWithTwoActions(
				EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION, Vec2(size.width, 0))),
				Sequence::createWithTwoActions(Hide::create(), CallFunc::create(funcUpdate))
				));
			_currentWindowLayer = _currentWindowLayer->_prevWindow;
			_left->setVisible(false);
			_right->setVisible(false);
		});

		_right->addClickEventListener([=](Ref*){
			if (!_currentWindowLayer || !_currentWindowLayer->_nextWindow) return;
			//_currentWindowLayer->_nextWindow->setVisible(true);
			Size size = _currentWindowLayer->_nextWindow->getViewSize();
			_currentWindowLayer->_nextWindow->setPosition(size.width, 0);
			_currentWindowLayer->_nextWindow->runAction(
				EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
			_currentWindowLayer->runAction(Sequence::createWithTwoActions(
				EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION, Vec2(-size.width, 0))),
				Sequence::createWithTwoActions(Hide::create(), CallFunc::create(funcUpdate))
				));
			_currentWindowLayer = _currentWindowLayer->_nextWindow;
			_left->setVisible(false);
			_right->setVisible(false);
		});

		_ok->addClickEventListener([](Ref*){
			TVPMainScene::GetInstance()->showWindowManagerOverlay(false);
		});

		ui::Button* fillscr = static_cast<ui::Button*>(allNodes.findController("fillscr"));
		fillscr->addClickEventListener([](Ref*){
			if (!_currentWindowLayer) return;
			_currentWindowLayer->toogleFillScale();
		});

		updateButtons();
	}

	void updateButtons() {
		if (!_currentWindowLayer) return;
		if (_left) {
			TVPWindowLayer *pLay = _currentWindowLayer->_prevWindow;
			while (pLay && !pLay->Visible) {
				pLay = pLay->_prevWindow;
			}
			_left->setVisible(pLay && pLay->Visible);
		}
		if (_right) {
			TVPWindowLayer *pLay = _currentWindowLayer->_nextWindow;
			while (pLay && !pLay->Visible) {
				pLay = pLay->_nextWindow;
			}
			_right->setVisible(pLay && pLay->Visible);
		}
	}
	
private:
	ui::Button *_left, *_right, *_ok;
};

static std::function<bool(Touch *, Event *)> _func_mask_layer_touchbegan;

class MaskLayer : public LayerColor {
public:
	static LayerColor * create(const Color4B& color, GLfloat width, GLfloat height) {
		LayerColor * layer = LayerColor::create(color, width, height);
		auto listener = EventListenerTouchOneByOne::create();
		listener->setSwallowTouches(true);
		if (_func_mask_layer_touchbegan) {
			listener->onTouchBegan = _func_mask_layer_touchbegan;
			_func_mask_layer_touchbegan = nullptr;
		} else {
			listener->onTouchBegan = [](Touch *, Event *)->bool{return true; };
		}
		listener->onTouchMoved = [](Touch *, Event *) {};
		listener->onTouchEnded = [](Touch *, Event *) {};
		listener->onTouchCancelled = [](Touch *, Event *) {};

		Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, layer);

		return layer;
	}
};

static TVPMainScene *_instance = nullptr;

TVPMainScene* TVPMainScene::GetInstance() {
	return _instance;
}

TVPMainScene* TVPMainScene::CreateInstance() {
	_instance = create();
	return _instance;
}

void TVPMainScene::initialize() {
	auto glview = cocos2d::Director::getInstance()->getOpenGLView();
	Size screenSize = glview->getFrameSize();
	Size designSize = glview->getDesignResolutionSize();
	ScreenRatio = screenSize.height / designSize.height;
	designSize.width = designSize.height * screenSize.width / screenSize.height;
	initWithSize(designSize);
	addChild(LayerColor::create(Color4B::BLACK, designSize.width, designSize.height));
	GameNode = cocos2d::Node::create();
	// horizontal
	//std::swap(designSize.width, designSize.height);
	GameNode->setContentSize(designSize);
	UINode = cocos2d::Node::create();
	UINode->setContentSize(designSize);
	UINode->setAnchorPoint(Vec2::ZERO);
	UINode->setPosition(Vec2::ZERO);
	UISize = designSize;
	addChild(UINode, UI_NODE_ORDER);
	GameNode->setAnchorPoint(Vec2(0, 0));
	//GameNode->setRotation(-90);
	//GameNode->setPosition(getContentSize() / 2);
	addChild(GameNode, GAME_SCENE_ORDER);

	EventListenerKeyboard* keylistener = EventListenerKeyboard::create();
	keylistener->onKeyPressed = CC_CALLBACK_2(TVPMainScene::onKeyPressed, this);
	keylistener->onKeyReleased = CC_CALLBACK_2(TVPMainScene::onKeyReleased, this);
	_eventDispatcher->addEventListenerWithFixedPriority(keylistener, 1);

	_touchListener = EventListenerTouchOneByOne::create();
	_touchListener->onTouchBegan = CC_CALLBACK_2(TVPMainScene::onTouchBegan, this);
	_touchListener->onTouchMoved = CC_CALLBACK_2(TVPMainScene::onTouchMoved, this);
	_touchListener->onTouchEnded = CC_CALLBACK_2(TVPMainScene::onTouchEnded, this);
	_touchListener->onTouchCancelled = CC_CALLBACK_2(TVPMainScene::onTouchCancelled, this);

	_eventDispatcher->addEventListenerWithSceneGraphPriority(_touchListener, this);

	EventListenerController *ctrllistener = EventListenerController::create();
	ctrllistener->onAxisEvent = CC_CALLBACK_3(TVPMainScene::onAxisEvent, this);
	ctrllistener->onKeyDown = CC_CALLBACK_3(TVPMainScene::onPadKeyDown, this);
	ctrllistener->onKeyUp = CC_CALLBACK_3(TVPMainScene::onPadKeyUp, this);
	ctrllistener->onKeyRepeat = CC_CALLBACK_3(TVPMainScene::onPadKeyRepeat, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(ctrllistener, this);
}

TVPMainScene * TVPMainScene::create() {
	TVPMainScene * ret = new TVPMainScene;
	ret->initialize();
	ret->autorelease();

	_touchMoveThresholdSq = Device::getDPI() / 10;
	_touchMoveThresholdSq *= _touchMoveThresholdSq;
	return ret;
}

void TVPMainScene::pushUIForm(cocos2d::Node *ui, eEnterAni ani) {
	TVPControlAdDialog(0x10002, 1, 0);
	int n = UINode->getChildrenCount();
	if (ani == eEnterAniNone) {
		UINode->addChild(ui);
	} else if (ani == eEnterAniOverFromRight) {
		if (n > 0) {
			Size size = UINode->getContentSize();
			cocos2d::Node *lastui = UINode->getChildren().back();
			lastui->runAction(EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION,
				Vec2(size.width / -5, 0))));
			cocos2d::Node *ColorMask = MaskLayer::create(Color4B(0, 0, 0, 0), size.width, size.height);
			ColorMask->setPosition(Vec2(-size.width, 0));
			ui->addChild(ColorMask);
			ColorMask->runAction(FadeTo::create(UI_CHANGE_DURATION, 128));
			ui->setPosition(size.width, 0);
			ui->runAction(EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
			runAction(Sequence::createWithTwoActions(DelayTime::create(UI_CHANGE_DURATION), CallFunc::create([=](){
				ColorMask->removeFromParent();
			})));
		}
		UINode->addChild(ui);
	} else if (ani == eEnterFromBottom) {
		Size size = UINode->getContentSize();
		cocos2d::Node *ColorMask = MaskLayer::create(Color4B(0, 0, 0, 0), size.width, size.height);
		ColorMask->runAction(FadeTo::create(UI_CHANGE_DURATION, 128));
		ui->setPositionY(-ui->getContentSize().height);
		ColorMask->addChild(ui);
		UINode->addChild(ColorMask);
		ui->runAction(EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
	}
}

void TVPMainScene::popUIForm(cocos2d::Node *form, eLeaveAni ani) {
	int n = UINode->getChildrenCount();
	if (n <= 0) return;
	if (n == 1) {
		TVPControlAdDialog(0x10002, 0, 0);
	}
	auto children = UINode->getChildren();
	if (ani == eLeaveAniNone) {
		if (n > 1) {
			Node *lastui = children.at(n - 2);
			lastui->setPosition(0, 0);
		}
		Node *ui = children.back();
		if (form) CCAssert(form == ui, "must be the same form");
		ui->removeFromParent();
	} else if (ani == eLeaveAniLeaveFromLeft) {
		Node *ui = children.back();
		if (form) CCAssert(form == ui, "must be the same form");
		Size size = UINode->getContentSize();
		if (n > 1) {
			Node *lastui = children.at(n - 2);
			lastui->setPosition(size.width / -5, 0);
			lastui->runAction(EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
		}
		cocos2d::Node *ColorMask = MaskLayer::create(Color4B(0, 0, 0, 128), size.width, size.height);
		ColorMask->setPosition(Vec2(-size.width, 0));
		ui->addChild(ColorMask);
		ColorMask->runAction(FadeOut::create(UI_CHANGE_DURATION));
		ui->runAction(EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION, Vec2(size.width, 0))));
		runAction(Sequence::createWithTwoActions(DelayTime::create(UI_CHANGE_DURATION), CallFunc::create([=](){
			ui->removeFromParent();
		})));
	} else if (ani == eLeaveToBottom) {
		cocos2d::Node *ColorMask = children.back();
		ColorMask->runAction(FadeOut::create(UI_CHANGE_DURATION));
		Node *ui = ColorMask->getChildren().at(0);
		if (form) CCAssert(form == ui, "must be the same form");
		ui->runAction(EaseQuadraticActionIn::create(MoveTo::create(UI_CHANGE_DURATION, Vec2(0, -ui->getContentSize().height))));
		runAction(Sequence::createWithTwoActions(DelayTime::create(UI_CHANGE_DURATION), CallFunc::create([=](){
			ColorMask->removeFromParent();
		})));
	}
}

bool TVPMainScene::startupFrom(const std::string &path) {
	// startup from dir
#ifdef _MSC_VER
	//TVPSetSystemOption("outputlog", "yes");
// 	TVPSetSystemOption("ogl_dup_target", "yes");
	//_set_new_handler(_no_memory_cb_vc);
#endif
	if (!TVPCheckStartupPath(path)) {
		return false;
	}
	IndividualConfigManager *pGlobalCfgMgr = IndividualConfigManager::GetInstance();
	pGlobalCfgMgr->UsePreferenceAt(TVPBaseFileSelectorForm::PathSplit(path).first);
	if (UINode->getChildrenCount()) {
		popUIForm(nullptr);
	}

	if (GlobalConfigManager::GetInstance()->GetValue<bool>("keep_screen_alive", true)) {
		Device::setKeepScreenOn(true);
	}
	// 	if (pGlobalCfgMgr->GetValueBool("rot_screen_180", false)) {
// 		GameNode->setRotation(90);
// 	}

	scheduleOnce(std::bind(&TVPMainScene::doStartup, this, std::placeholders::_1, path), 0, "startup");

	return true;
}

void TVPMainScene::doStartup(float dt, std::string path) {
	unschedule("startup");
	IndividualConfigManager *pGlobalCfgMgr = IndividualConfigManager::GetInstance();
	_consoleWin = TVPConsoleWindow::create(14, nullptr);

	auto glview = cocos2d::Director::getInstance()->getOpenGLView();
	Size screenSize = glview->getFrameSize();
	float scale = screenSize.height / getContentSize().height;
	_consoleWin->setScale(1 / scale);
	_consoleWin->setContentSize(getContentSize() * scale);
	_consoleWin->setFontSize(16);
	GameNode->addChild(_consoleWin, GAME_CONSOLE_ORDER);
	::Application->StartApplication(path);
	// update one frame
	update(0);
	//_ResotreGLStatues(); // already in update()
	GLubyte handlerOpacity = pGlobalCfgMgr->GetValue<float>("menu_handler_opa", 0.15f) * 255;
	_gameMenu = TVPGameMainMenu::create(handlerOpacity);
	GameNode->addChild(_gameMenu, GAME_MENU_ORDER);
	_gameMenu->shrinkWithTime(1);
	if (_consoleWin) {
		_consoleWin->removeFromParent();
		_consoleWin = nullptr;
		scheduleUpdate();

		cocos2d::Director::getInstance()->purgeCachedData();
		TVPControlAdDialog(0x10002, 0, 0); // ensure to close banner ad
	}

	TVPWindowLayer *pWin = _lastWindowLayer;
	while (pWin) {
		pWin->setVisible(true);
		pWin = pWin->_prevWindow;
	}

	if (pGlobalCfgMgr->GetValue<bool>("showfps", false)) {
		_fpsLabel = cocos2d::Label::createWithTTF("", "DroidSansFallback.ttf", 16);
		_fpsLabel->setAnchorPoint(Vec2(0, 1));
		_fpsLabel->setPosition(Vec2(0, GameNode->getContentSize().height));
		_fpsLabel->setColor(Color3B::WHITE);
		_fpsLabel->enableOutline(Color4B::BLACK, 1);
		GameNode->addChild(_fpsLabel, GAME_MENU_ORDER);
	}
}

extern ttstr TVPGetErrorDialogTitle();
void TVPOnError();
tjs_uint TVPGetGraphicCacheTotalBytes();
void TVPMainScene::update(float delta) {
	::Application->Run();
//	if (_currentWindowLayer) _currentWindowLayer->UpdateOverlay();
	iTVPTexture2D::RecycleProcess();
	//_ResotreGLStatues();
	if (_postUpdate) _postUpdate();
	if (_fpsLabel) {
		unsigned int drawCount;
		uint64_t vmemsize;
		TVPGetRenderManager()->GetRenderStat(drawCount, vmemsize);
		static timeval _lastUpdate;
		//static int _lastUpdateReq = gettimeofday(&_lastUpdate, nullptr);
		struct timeval now;
		gettimeofday(&now, nullptr);
		float _deltaTime = (now.tv_sec - _lastUpdate.tv_sec) + (now.tv_usec - _lastUpdate.tv_usec) / 1000000.0f;
		_lastUpdate = now;

		static float prevDeltaTime = 0.016f; // 60FPS
		static const float FPS_FILTER = 0.10f;
		static float _accumDt = 0;
		static unsigned int prevDrawCount = 0;
		_accumDt += _deltaTime;
		float dt = _deltaTime * FPS_FILTER + (1 - FPS_FILTER) * prevDeltaTime;
		prevDeltaTime = dt;
		if (drawCount > prevDrawCount)
			prevDrawCount = drawCount;

		char buffer[30];
		if (_accumDt > 0.1f) {
			sprintf(buffer, "%.1f (%d draws)", 1 / dt, drawCount);
			_fpsLabel->setString(buffer);
//#ifdef _MSC_VER
			std::string msg = buffer; msg += "\n";
			//sprintf(buffer, "%.2f MB", TVPGetGraphicCacheTotalBytes() / (float)(1024 * 1024));
			vmemsize >>= 10;
			sprintf(buffer, "%d MB(%.2f MB) %d MB\n", TVPGetSelfUsedMemory(), (float)vmemsize / 1024.f, TVPGetSystemFreeMemory());
			msg += buffer;
			_fpsLabel->setString(msg);
//#endif
			_accumDt = 0;
			prevDrawCount = 0;
		}
	}
}

cocos2d::Size TVPMainScene::getUINodeSize() {
	return UINode->getContentSize();
}

void TVPMainScene::addLayer(TVPWindowLayer* lay) {
	GameNode->addChild(lay, GAME_SCENE_ORDER);
	lay->setViewSize(GameNode->getContentSize());
	lay->setContentSize(lay->getViewSize());
// 	if (_currentWindowLayer) {
// 		_currentWindowLayer->setVisible(false);
// 	}
// 	_currentWindowLayer = lay;
}

void TVPMainScene::rotateUI() {
	float rot = UINode->getRotation();
	if (rot < 1) {
		UINode->setRotation(90);
		UINode->setContentSize(Size(UISize.height, UISize.width));
	} else {
		UINode->setRotation(0);
		UINode->setContentSize(UISize);
	}
	for (Node* ui : UINode->getChildren()) {
		static_cast<iTVPBaseForm*>(ui)->rearrangeLayout();
	}
}

void TVPMainScene::setMaskLayTouchBegain(const std::function<bool(cocos2d::Touch *, cocos2d::Event *)> &func) {
	_func_mask_layer_touchbegan = func;
}

static float _getUIScale() {
	auto glview = Director::getInstance()->getOpenGLView();
	float factor = (glview->getScaleX() + glview->getScaleY()) / 2;
	factor /= Device::getDPI(); // inch per pixel
	Size screenSize = glview->getFrameSize();
	Size designSize = glview->getDesignResolutionSize();
	designSize.width = designSize.height * screenSize.width / screenSize.height;
	screenSize.width = factor * designSize.width;
#ifdef _WIN32
	//if (screenSize.width > 3.5433) return 0.35f; // 7 inch @ 16:9 device
	return 0.35f;
#endif
// 	char tmp[128];
// 	sprintf(tmp, "screenSize.width = %f", (float)screenSize.width);
// 	TVPPrintLog(tmp);
// #if CC_PLATFORM_IOS == CC_TARGET_PLATFORM
// 	return /*sqrtf*/(0.0005f / factor) * screenSize.width;
// #else
	return /*sqrtf*/(0.0005f / factor) * screenSize.width;
//#endif
}

float TVPMainScene::getUIScale() {
	static float uiscale = _getUIScale();
	return uiscale;
}

void TVPMainScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event) {
	Vector<Node*>& uiChild = UINode->getChildren();
	if (!uiChild.empty()) {
		iTVPBaseForm* uiform = dynamic_cast<iTVPBaseForm*>(uiChild.back());
		if (uiform) uiform->onKeyPressed(keyCode, event);
		return;
	}
	switch (keyCode) {
	case EventKeyboard::KeyCode::KEY_MENU:
		if (UINode->getChildren().empty()) {
			if (_gameMenu) _gameMenu->toggle();
		}
		return;
		break;
	case EventKeyboard::KeyCode::KEY_BACK:
		if (!UINode->getChildren().empty()) {
			return;
		}
		if (_gameMenu && !_gameMenu->isShrinked()) {
			_gameMenu->shrink();
			return;
		}
		keyCode = EventKeyboard::KeyCode::KEY_ESCAPE;
		break;
#ifdef _DEBUG
	case EventKeyboard::KeyCode::KEY_PAUSE:
		GameNode->addChild(DebugViewLayerForm::create());
		return;
	case EventKeyboard::KeyCode::KEY_F12:
		if (TVPGetCurrentShiftKeyState() & ssShift) {
			std::vector<ttstr> btns({ "OK", "Cancel" });
			ttstr text; tTJSVariant result;
			if (TVPShowSimpleInputBox(text, "console command", "", btns) == 0) {
				TVPExecuteExpression(text, &result);
			}
			result = text;
		}
		break;
#endif
	default:
		break;
	}
	int code = _ConvertKeyCodeToVKCode(keyCode);
	if (!code) return;
	_scancode[code] = 0x11;
	if (_currentWindowLayer) {
		_currentWindowLayer->InternalKeyDown(code, TVPGetCurrentShiftKeyState());
	}
}

void TVPMainScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event) {
#ifdef _DEBUG
	if (keyCode == EventKeyboard::KeyCode::KEY_PAUSE) return;
#endif
	if (keyCode == EventKeyboard::KeyCode::KEY_MENU) return;
	if (keyCode == EventKeyboard::KeyCode::KEY_BACK) keyCode = EventKeyboard::KeyCode::KEY_ESCAPE;
	if (keyCode == EventKeyboard::KeyCode::KEY_PLAY) { // auto play
		iTJSDispatch2* global = TVPGetScriptDispatch();
		tTJSVariant var;
		if (global->PropGet(0, TJS_W("kag"), nullptr, &var, global) == TJS_S_OK && var.Type() == tvtObject) {
			iTJSDispatch2* kag = var.AsObjectNoAddRef();
			if (kag->PropGet(0, TJS_W("autoMode"), nullptr, &var, kag) == TJS_S_OK) {
				if (var.operator bool()) {
					if (kag->PropGet(0, TJS_W("cancelAutoMode"), nullptr, &var, kag) == TJS_S_OK && var.Type() == tvtObject) {
						iTJSDispatch2* fn = var.AsObjectNoAddRef();
						if (fn->IsInstanceOf(0, 0, 0, TJS_W("Function"), fn)) {
							tTJSVariant *args = nullptr;
							fn->FuncCall(0, nullptr, nullptr, nullptr, 0, &args, kag);
							return;
						}
					}
				} else {
					if (kag->PropGet(0, TJS_W("enterAutoMode"), nullptr, &var, kag) == TJS_S_OK && var.Type() == tvtObject) {
						iTJSDispatch2* fn = var.AsObjectNoAddRef();
						if (fn->IsInstanceOf(0, 0, 0, TJS_W("Function"), fn)) {
							tTJSVariant *args = nullptr;
							fn->FuncCall(0, nullptr, nullptr, nullptr, 0, &args, kag);
							return;
						}
					}
				}
			}
		}
		global->Release();
	}
	int code = _ConvertKeyCodeToVKCode(keyCode);
	if (!code) return;
	bool isPressed = _scancode[code] & 1;
	_scancode[code] &= 0x10;
	if (isPressed && _currentWindowLayer) {
		_currentWindowLayer->OnKeyUp(code, TVPGetCurrentShiftKeyState());
	}
}

TVPMainScene::TVPMainScene() {
	_gameMenu = nullptr;
	_windowMgrOverlay = nullptr;
}

void TVPMainScene::showWindowManagerOverlay(bool bVisible) {
	if (bVisible) {
		if (!_windowMgrOverlay) {
			if (_currentWindowLayer && _currentWindowLayer->in_mode_) return;
			_windowMgrOverlay = TVPWindowManagerOverlay::create();
			GameNode->addChild(_windowMgrOverlay, GAME_WINMGR_ORDER);
			_gameMenu->setVisible(false);
		}
	} else {
		if (_windowMgrOverlay) {
			_windowMgrOverlay->removeFromParent();
			_windowMgrOverlay = nullptr;
			_gameMenu->setVisible(true);
		}
	}
}

void TVPMainScene::popAllUIForm() {
    TVPControlAdDialog(0x10002, 0, 0);
	auto children = UINode->getChildren();
	for (auto ui : children) {
		Size size = getContentSize();
		cocos2d::Node *ColorMask = MaskLayer::create(Color4B(0, 0, 0, 128), size.width, size.height);
		ColorMask->setPosition(Vec2(-size.width, 0));
		ui->addChild(ColorMask);
		ColorMask->runAction(FadeOut::create(UI_CHANGE_DURATION));
		ui->runAction(EaseQuadraticActionOut::create(MoveTo::create(UI_CHANGE_DURATION, Vec2(size.width, 0))));
		runAction(Sequence::createWithTwoActions(DelayTime::create(UI_CHANGE_DURATION), CallFunc::create([=](){
			ui->removeFromParent();
		})));
	}
}

void TVPMainScene::toggleVirtualMouseCursor() {
	showVirtualMouseCursor(!_mouseCursor || !_mouseCursor->isVisible());
}

Sprite *TVPCreateCUR() {
	std::string fullPath = FileUtils::getInstance()->fullPathForFilename("default.cur");
	Data buf = FileUtils::getInstance()->getDataFromFile(fullPath);

	tTVPMemoryStream stream(buf.getBytes(), buf.getSize());
	return TVPLoadCursorCUR(&stream);
}

void TVPMainScene::showVirtualMouseCursor(bool bVisible) {
	if (!bVisible) {
		if (_mouseCursor) _mouseCursor->setVisible(false);
		_virutalMouseMode = bVisible;
		return;
	}
	if (!_mouseCursor) {
		_mouseCursor = TVPCreateCUR();
		if (!_mouseCursor) return;

		_mouseCursorScale = _mouseCursor->getScale() *
			convertCursorScale(IndividualConfigManager::GetInstance()->GetValue<float>("vcursor_scale", 0.5f));
		_mouseCursor->setScale(_mouseCursorScale);
		GameNode->addChild(_mouseCursor, GAME_WINMGR_ORDER);
		_mouseCursor->setPosition(GameNode->getContentSize() / 2);
	}
	_mouseCursor->setVisible(true);
	_virutalMouseMode = bVisible;
}

bool TVPMainScene::isVirtualMouseMode() const {
	return _mouseCursor && _mouseCursor->isVisible();
}

bool TVPMainScene::onTouchBegan(cocos2d::Touch *touch, cocos2d::Event *event) {
	if (UINode->getChildrenCount()) return false;
	if (!_currentWindowLayer) return false;
	if (!_virutalMouseMode || _windowMgrOverlay) return _currentWindowLayer->onTouchBegan(touch, event);
	_mouseTouches.insert(touch);
	switch (_mouseTouches.size()) {
	case 1:
		_mouseTouchPoint = GameNode->convertToNodeSpace(touch->getLocation());
		_mouseBeginPoint = _mouseCursor->getPosition();
		_touchBeginTick = TVPGetRoughTickCount32();
		_mouseMoved = false;
		_mouseClickedDown = false;
		_mouseBtn = ::mbLeft;
		_mouseCursor->stopAllActions();
		_mouseCursor->setOpacity(255);
		_mouseCursor->setScale(_mouseCursorScale);
		_mouseCursor->runAction(Sequence::createWithTwoActions(DelayTime::create(1), CallFuncN::create([this](Node*p){
			p->setScale(_mouseCursorScale * 0.8f);
			_currentWindowLayer->onMouseMove(GameNode->convertToWorldSpace(_mouseBeginPoint));
			_currentWindowLayer->onMouseDown(GameNode->convertToWorldSpace(_mouseBeginPoint));
			_mouseClickedDown = true;
			_mouseMoved = true;
		})));
		break;
	case 2:
		_mouseBtn = ::mbRight;
		_mouseTouchPoint = (_mouseTouchPoint + GameNode->convertToNodeSpace(touch->getLocation())) / 2;
		break;
	default:
		break;
	}
	return true;
}

void TVPMainScene::onTouchMoved(cocos2d::Touch *touch, cocos2d::Event *event) {
	if (!_currentWindowLayer) return;
	if (!_virutalMouseMode || _windowMgrOverlay) return _currentWindowLayer->onTouchMoved(touch, event);
	if (_mouseTouches.size()) {
		Vec2 pt, newpt;
		if (_mouseTouches.size() == 1) {
			pt = touch->getLocation();
		} else if (_mouseTouches.size() == 2) {
			auto it = _mouseTouches.begin();
			pt = (*it)->getLocation();
			pt = (pt + (*++it)->getLocation()) / 2;
		}
		Vec2 moveDistance, newPoint;
		newPoint = GameNode->convertToNodeSpace(pt);
		moveDistance = newPoint - _mouseTouchPoint;
		pt = _mouseBeginPoint + moveDistance;
		Size size = GameNode->getContentSize();
		newpt = pt;
		if (pt.x < 0) newpt.x = 0;
		else if (pt.x > size.width) newpt.x = size.width;
		if (pt.y < 0) newpt.y = 0;
		else if (pt.y > size.height) newpt.y = size.height;
		_mouseBeginPoint += newpt - pt;
		_mouseCursor->setPosition(newpt);
		_currentWindowLayer->onMouseMove(GameNode->convertToWorldSpace(newpt));
		if (!_mouseMoved) {
			if (TVPGetRoughTickCount32() - _touchBeginTick > 1000) {
				_currentWindowLayer->onMouseDown(GameNode->convertToWorldSpace(newpt));
				_mouseClickedDown = true;
				_mouseMoved = true;
			} else if (moveDistance.getLengthSq() > _touchMoveThresholdSq) {
				_mouseCursor->stopAllActions();
				_mouseMoved = true;
			}
		}
	}
}

void TVPMainScene::onTouchEnded(cocos2d::Touch *touch, cocos2d::Event *event) {
	if (!_currentWindowLayer) return;
	if (!_virutalMouseMode || _windowMgrOverlay) return _currentWindowLayer->onTouchEnded(touch, event);
	if (_mouseTouches.size() == 1) {
		Vec2 pt = _mouseCursor->getPosition();
		if (!_mouseClickedDown && TVPGetRoughTickCount32() - _touchBeginTick < 150) {
			_currentWindowLayer->onMouseClick(GameNode->convertToWorldSpace(pt));
		} else if (_mouseClickedDown) {
			_currentWindowLayer->onMouseUp(GameNode->convertToWorldSpace(pt));
		}
	}
	_mouseTouches.erase(touch);
	if (_mouseTouches.size() == 0) {
		_mouseMoved = false;
		_mouseClickedDown = false;
		_refadeMouseCursor();
		_mouseCursor->setScale(_mouseCursorScale);
		_mouseCursor->stopAllActions();
	}
}

void TVPMainScene::onTouchCancelled(cocos2d::Touch *touch, cocos2d::Event *event) {
	if (!_currentWindowLayer) return;
	if (!_virutalMouseMode || _windowMgrOverlay) return _currentWindowLayer->onTouchCancelled(touch, event);
	_mouseTouches.erase(touch);
	_mouseMoved = false;
	_refadeMouseCursor();
}

bool TVPMainScene::attachWithIME()
{
	bool ret = IMEDelegate::attachWithIME();
	if (ret)
	{
		// open keyboard
		auto pGlView = Director::getInstance()->getOpenGLView();
		if (pGlView)
		{
#if (CC_TARGET_PLATFORM != CC_PLATFORM_WP8 && CC_TARGET_PLATFORM != CC_PLATFORM_WINRT)
			pGlView->setIMEKeyboardState(true);
#else
			pGlView->setIMEKeyboardState(true, "");
#endif
		}
	}
	return ret;
}

bool TVPMainScene::detachWithIME()
{
	bool ret = IMEDelegate::detachWithIME();
	if (ret)
	{
		// close keyboard
		auto glView = Director::getInstance()->getOpenGLView();
		if (glView)
		{
#if (CC_TARGET_PLATFORM != CC_PLATFORM_WP8 && CC_TARGET_PLATFORM != CC_PLATFORM_WINRT)
			glView->setIMEKeyboardState(false);
#else
			glView->setIMEKeyboardState(false, "");
#endif
		}
	}
	return ret;
}

bool TVPMainScene::canAttachWithIME()
{
	return true;
}

bool TVPMainScene::canDetachWithIME()
{
	return true;
}

void TVPMainScene::deleteBackward()
{
#ifndef _WIN32
	if (_currentWindowLayer) {
		_currentWindowLayer->InternalKeyDown(VK_BACK, TVPGetCurrentShiftKeyState());
		_currentWindowLayer->OnKeyUp(VK_BACK, TVPGetCurrentShiftKeyState());
	}
#endif
}

void TVPMainScene::insertText(const char * text, size_t len)
{
	std::string utf8(text, len);
	onTextInput(utf8);
}

void TVPMainScene::onCharInput(int keyCode) {
	_currentWindowLayer->OnKeyPress((tjs_char)keyCode, 0, false, false);
}

void TVPMainScene::onTextInput(const std::string &text) {
	std::u16string buf;
	if (StringUtils::UTF8ToUTF16(text, buf)) {
		for (int i = 0; i < buf.size(); ++i) {
			_currentWindowLayer->OnKeyPress(buf[i], 0, false, false);
		}
	}
}

void TVPMainScene::onAxisEvent(cocos2d::Controller* ctrl, int keyCode, cocos2d::Event *e) {

}

void TVPMainScene::onPadKeyDown(cocos2d::Controller* ctrl, int keyCode, cocos2d::Event *e) {
	if (!UINode->getChildren().empty()) return;
	int code = _ConvertPadKeyCodeToVKCode(keyCode);
	if (!code) return;
	_scancode[code] = 0x11;
	if (_currentWindowLayer) {
		_currentWindowLayer->InternalKeyDown(code, TVPGetCurrentShiftKeyState());
	}
}

void TVPMainScene::onPadKeyUp(cocos2d::Controller* ctrl, int keyCode, cocos2d::Event *e) {
	int code = _ConvertPadKeyCodeToVKCode(keyCode);
	if (!code) return;
	bool isPressed = _scancode[code] & 1;
	_scancode[code] &= 0x10;
	if (isPressed && _currentWindowLayer) {
		_currentWindowLayer->OnKeyUp(code, TVPGetCurrentShiftKeyState());
	}
}

void TVPMainScene::onPadKeyRepeat(cocos2d::Controller* ctrl, int code, cocos2d::Event *e) {

}

float TVPMainScene::convertCursorScale(float val/*0 ~ 1*/) {
	if (val <= 0.5f) {
		return 0.25f + (val * 2) * 0.75f;
	} else {
		return 1.f + (val - 0.5f) * 2.f;
	}
}

iWindowLayer *TVPCreateAndAddWindow(tTJSNI_Window *w) {
	TVPWindowLayer* ret = TVPWindowLayer::create(w);
	TVPMainScene::GetInstance()->addLayer(ret);
	if (_consoleWin) ret->setVisible(false);
	return ret;
}

void TVPRemoveWindowLayer(iWindowLayer *lay) {
	static_cast<TVPWindowLayer*>(lay)->removeFromParent();
}

void TVPConsoleLog(const ttstr &l, bool important) {
	static bool TVPLoggingToConsole = IndividualConfigManager::GetInstance()->GetValue<bool>("outputlog", true);
	if (!TVPLoggingToConsole) return;
	if (_consoleWin) {
		_consoleWin->addLine(l, important ? Color3B::YELLOW : Color3B::GRAY);
		TVPDrawSceneOnce(100); // force update in 10fps
	}
#ifdef _WIN32
	//cocos2d::log("%s", utf8.c_str());
	char buf[16384] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, l.c_str(), -1, buf, sizeof(buf), nullptr, FALSE);
	puts(buf);
#else
	std::string utf8;
	if (StringUtils::UTF16ToUTF8(l.c_str(), utf8))
		cocos2d::log("%s", utf8.c_str());
#endif
}

namespace TJS {
	static const int MAX_LOG_LENGTH = 16 * 1024;
	void TVPConsoleLog(const tjs_char *l) {
		std::string utf8;
		assert(sizeof(tjs_char) == sizeof(char16_t));
		std::u16string buf((const char16_t*)l);
		if (StringUtils::UTF16ToUTF8(buf, utf8))
			cocos2d::log("%s", utf8.c_str());
	}

	void TVPConsoleLog(const tjs_nchar *format, ...) {
		va_list args;
		va_start(args, format);
		char buf[MAX_LOG_LENGTH];
		vsnprintf(buf, MAX_LOG_LENGTH - 3, format, args);
		cocos2d::log("%s", buf);
		va_end(args);
	}
}

bool TVPGetScreenSize(tjs_int idx, tjs_int &w, tjs_int &h) {
	if (idx != 0) return false;
	const cocos2d::Size &size = cocos2d::Director::getInstance()->getOpenGLView()->getFrameSize();
	//w = size.height; h = size.width;
	w = 2048;
	h = w * (size.height / size.width);
	return true;
}

ttstr TVPGetDataPath() {
	std::string path = cocos2d::FileUtils::getInstance()->getWritablePath();
	return path;
}

#include "StorageImpl.h"
static std::string _TVPGetInternalPreferencePath() {
	std::string path = cocos2d::FileUtils::getInstance()->getWritablePath();
	path += ".preference";
	if (!TVPCheckExistentLocalFolder(path)) {
		TVPCreateFolders(path);
	}
	path += "/";
	return path;
}

const std::string &TVPGetInternalPreferencePath() {
	static std::string ret = _TVPGetInternalPreferencePath();
	return ret;
}

tjs_uint32 TVPGetCurrentShiftKeyState()
{
	tjs_uint32 f = 0;

	if (_scancode[VK_SHIFT] & 1) f |= ssShift;
	if (_scancode[VK_MENU] & 1) f |= ssAlt;
	if (_scancode[VK_CONTROL] & 1) f |= ssCtrl;
	if (_scancode[VK_LBUTTON] & 1) f |= ssLeft;
	if (_scancode[VK_RBUTTON] & 1) f |= ssRight;
	//if (_scancode[VK_MBUTTON] & 1) f |= TVP_SS_MIDDLE;

	return f;
}

ttstr TVPGetPlatformName()
{
	switch (cocos2d::Application::getInstance()->getTargetPlatform()) {
	case ApplicationProtocol::Platform::OS_WINDOWS:
		return "Win32";
	case ApplicationProtocol::Platform::OS_LINUX:
		return "Linux";
	case ApplicationProtocol::Platform::OS_MAC:
		return "MacOS";
	case ApplicationProtocol::Platform::OS_ANDROID:
		return "Android";
	case ApplicationProtocol::Platform::OS_IPHONE:
		return "iPhone";
	case ApplicationProtocol::Platform::OS_IPAD:
		return "iPad";
	case ApplicationProtocol::Platform::OS_BLACKBERRY:
		return "BlackBerry";
	case ApplicationProtocol::Platform::OS_NACL:
		return "Nacl";
	case ApplicationProtocol::Platform::OS_TIZEN:
		return "Tizen";
	case ApplicationProtocol::Platform::OS_WINRT:
		return "WinRT";
	case ApplicationProtocol::Platform::OS_WP8:
		return "WinPhone8";
	default:
		return "Unknown";
	}
}

ttstr TVPGetOSName()
{
	return TVPGetPlatformName();
}