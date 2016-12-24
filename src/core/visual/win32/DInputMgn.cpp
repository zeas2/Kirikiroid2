//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// DirectInput management
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <algorithm>
//#include "WindowFormUnit.h"
#include "WindowImpl.h"
#include "EventIntf.h"
#include "MsgIntf.h"
#include "SysInitIntf.h"
#include "SystemImpl.h"
#include "tvpinputdefs.h"
#include "DInputMgn.h"

#include "DebugIntf.h"
#include "Application.h"
#include "TVPScreen.h"
#include "TickCount.h"

//---------------------------------------------------------------------------
// DirectInput management
//---------------------------------------------------------------------------
/*
	tvp32 uses DirectInput to detect wheel rotation. This will avoid
	DirectX/Windows bug which prevents wheel messages when the window is
	full-screened.
*/
tTVPWheelDetectionType TVPWheelDetectionType = wdtWindowMessage/*wdtDirectInput*/;
tTVPJoyPadDetectionType TVPJoyPadDetectionType = jdtNone/*jdtDirectInput*/;
static bool TVPDirectInputInit = false;
static tjs_int TVPDirectInputLibRefCount = 0;
//static HMODULE TVPDirectInputLibHandle = NULL; // module handle for dinput.dll
//static IDirectInput *TVPDirectInput = NULL; // DirectInput object
//---------------------------------------------------------------------------
#if 0
struct tTVPDIWheelData { LONG delta; }; // data structure for DirectInput(Mouse)
static DIOBJECTDATAFORMAT TVPWheelDIODF[] =
{ { &GUID_ZAxis, 0, DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 } };
static DIDATAFORMAT TVPWheelDIDF =
{
	sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), DIDF_RELAXIS,
	sizeof(tTVPDIWheelData), 1, TVPWheelDIODF
};
#endif
// Joystick (pad) related codes are contributed by Mr. Kiyobee @ TYPE-MOON.
// Say thanks to him.

//	in http://www.mediawars.ne.jp/~freemage/progs/other03.htm
const static tjs_uint32 q = 0x80000000;
#if 0
static DIOBJECTDATAFORMAT _c_rgodf[ ] = {
	{ &GUID_XAxis, FIELD_OFFSET(DIJOYSTATE, lX), q | DIDFT_AXIS | DIDFT_ANYINSTANCE, 256, },
	{ &GUID_YAxis, FIELD_OFFSET(DIJOYSTATE, lY), q | DIDFT_AXIS | DIDFT_ANYINSTANCE, 256, },
	{ &GUID_ZAxis, FIELD_OFFSET(DIJOYSTATE, lZ), q | DIDFT_AXIS | DIDFT_ANYINSTANCE, 256, },
	{ &GUID_RxAxis, FIELD_OFFSET(DIJOYSTATE, lRx), q | DIDFT_AXIS | DIDFT_ANYINSTANCE, 256, },
	{ &GUID_RyAxis, FIELD_OFFSET(DIJOYSTATE, lRy), q | DIDFT_AXIS | DIDFT_ANYINSTANCE, 256, },
	{ &GUID_RzAxis, FIELD_OFFSET(DIJOYSTATE, lRz), q | DIDFT_AXIS | DIDFT_ANYINSTANCE, 256, },
	{ &GUID_Slider, FIELD_OFFSET(DIJOYSTATE, rglSlider[0]), q | DIDFT_AXIS | DIDFT_ANYINSTANCE, 256, },
	{ &GUID_Slider, FIELD_OFFSET(DIJOYSTATE, rglSlider[1]), q | DIDFT_AXIS | DIDFT_ANYINSTANCE, 256, },
	{ &GUID_POV, FIELD_OFFSET(DIJOYSTATE, rgdwPOV[0]), q | DIDFT_POV | DIDFT_ANYINSTANCE, 0, },
	{ &GUID_POV, FIELD_OFFSET(DIJOYSTATE, rgdwPOV[1]), q | DIDFT_POV | DIDFT_ANYINSTANCE, 0, },
	{ &GUID_POV, FIELD_OFFSET(DIJOYSTATE, rgdwPOV[2]), q | DIDFT_POV | DIDFT_ANYINSTANCE, 0, },
	{ &GUID_POV, FIELD_OFFSET(DIJOYSTATE, rgdwPOV[3]), q | DIDFT_POV | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[0]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[1]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[2]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[3]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[4]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[5]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[6]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[7]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[8]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[9]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[10]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[11]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[12]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[13]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[14]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[15]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[16]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[17]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[18]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[19]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[20]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[21]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[22]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[23]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[24]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[25]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[26]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[27]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[28]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[29]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[30]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
	{ 0, FIELD_OFFSET(DIJOYSTATE, rgbButtons[31]), q | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
};
#define numObjects (sizeof(_c_rgodf) / sizeof(_c_rgodf[0]))
static DIDATAFORMAT c_dfPad =
{
	sizeof(DIDATAFORMAT),		//	structure size   構造体サイズ
	sizeof(DIOBJECTDATAFORMAT),	//	size of object data format オブジェクトデータ形式のサイズ
	DIDF_ABSAXIS,				//	absolute axis system 絶対軸座標系
	sizeof(DIJOYSTATE),			//	size of device data デバイスデータのサイズ
	numObjects, 				//	count of objects オブジェクト数
	_c_rgodf,					//	position 位置
};
#endif
static const tjs_int   PadAxisMax = +32767;
static const tjs_int   PadAxisMin = -32768;
static const tjs_int   PadAxisThreshold   = 95; // Assumes 95% value can turn input on. 95%の入力でOK
static const tjs_int   PadAxisUpperThreshold  = PadAxisMax * PadAxisThreshold / 100;
static const tjs_int   PadAxisLowerThreshold  = PadAxisMin * PadAxisThreshold / 100;
//static bool CALLBACK EnumJoySticksCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);
static tjs_uint32	PadLastTrigger;
static tjs_uint32 /*__fastcall*/ PadState();





#if 0
//---------------------------------------------------------------------------
// pad virtual key code map
//---------------------------------------------------------------------------
const int TVPPadVirtualKeyMap[TVP_NUM_PAD_KEY] = // tTVPPadKeyFlag to VK
{
	VK_PADLEFT,
	VK_PADRIGHT,
	VK_PADUP,
	VK_PADDOWN,
	VK_PAD1,
	VK_PAD2,
	VK_PAD3,
	VK_PAD4,
	VK_PAD5,
	VK_PAD6,
	VK_PAD7,
	VK_PAD8,
	VK_PAD9,
	VK_PAD10,
};
//---------------------------------------------------------------------------
static tTVPPadKeyFlag TVPVirtualKeyToPadCode(WORD vk)
{
	// VK to tTVPPadKeyFlag
	switch(vk)
	{
	case VK_PADLEFT:		return pkfLeft;
	case VK_PADRIGHT:		return pkfRight;
	case VK_PADUP:			return pkfUp;
	case VK_PADDOWN:		return pkfDown;
	case VK_PAD1:			return pkfButton0;
	case VK_PAD2:			return pkfButton1;
	case VK_PAD3:			return pkfButton2;
	case VK_PAD4:			return pkfButton3;
	case VK_PAD5:			return pkfButton4;
	case VK_PAD6:			return pkfButton5;
	case VK_PAD7:			return pkfButton6;
	case VK_PAD8:			return pkfButton7;
	case VK_PAD9:			return pkfButton8;
	case VK_PAD10:			return pkfButton9;
	}
	return (tTVPPadKeyFlag)-1;
}
//---------------------------------------------------------------------------
#endif



//---------------------------------------------------------------------------
// tTVPKeyRepeatEmulator : A class for emulating keyboard key repeats.
//---------------------------------------------------------------------------
tjs_int32 tTVPKeyRepeatEmulator::HoldTime = 500; // keyboard key-repeats hold-time
tjs_int32 tTVPKeyRepeatEmulator::IntervalTime = 30; // keyboard key-repeats interval-time
//---------------------------------------------------------------------------
void tTVPKeyRepeatEmulator::GetKeyRepeatParam()
{
	static tjs_int ArgumentGeneration = 0;
	if(ArgumentGeneration != TVPGetCommandLineArgumentGeneration())
	{
		ArgumentGeneration = TVPGetCommandLineArgumentGeneration();
		tTJSVariant val;
		if(TVPGetCommandLine(TJS_W("-paddelay"), &val))
		{
			HoldTime = (int)val;
		}
		if(TVPGetCommandLine(TJS_W("-padinterval"), &val))
		{
			IntervalTime = (int)val;
		}
	}
}
//---------------------------------------------------------------------------
#define TVP_REPEAT_LIMIT 10
tTVPKeyRepeatEmulator::tTVPKeyRepeatEmulator() : Pressed(false)
{
}
//---------------------------------------------------------------------------
tTVPKeyRepeatEmulator::~tTVPKeyRepeatEmulator()
{
}
//---------------------------------------------------------------------------
void tTVPKeyRepeatEmulator::Down()
{
	Pressed = true;
	PressedTick = TVPGetRoughTickCount32();
	LastRepeatCount = 0;
}
//---------------------------------------------------------------------------
void tTVPKeyRepeatEmulator::Up()
{
	Pressed = false;
}
//---------------------------------------------------------------------------
tjs_int tTVPKeyRepeatEmulator::GetRepeatCount()
{
	// calculate repeat count, from previous call of "GetRepeatCount" function.
	GetKeyRepeatParam();

	if(!Pressed) return 0;
	if(HoldTime<0) return 0;
	if(IntervalTime<=0) return 0;

	tjs_int elapsed = (tjs_int)(TVPGetRoughTickCount32() - PressedTick);

	elapsed -= HoldTime;
	if(elapsed < 0) return 0; // still in hold time

	elapsed /= IntervalTime;

	tjs_int ret = elapsed - LastRepeatCount;
	if(ret > TVP_REPEAT_LIMIT) ret = TVP_REPEAT_LIMIT;

	LastRepeatCount = elapsed;

	return ret;
}
//---------------------------------------------------------------------------



#if 0
//---------------------------------------------------------------------------
// DirectInput management
//---------------------------------------------------------------------------
IDirectInput * TVPAddRefDirectInput()
{
	// addref DirectInput.
	TVPDirectInputLibRefCount ++;
	if(TVPDirectInputInit)
	{
		// DirectInput is not to be loaded more than once
		// even if the loading is failed.
		return TVPDirectInput;
	}

	TVPDirectInputInit = true;

	TVPDirectInputLibHandle = ::LoadLibrary(TJS_W("dinput.dll"));
	if(!TVPDirectInputLibHandle) return NULL; // load error; is not a fatal error

	HRESULT (WINAPI *procDirectInputCreateW)
		(HINSTANCE hinst, DWORD dwVersion,
			LPDIRECTINPUTW *ppDI, LPUNKNOWN punkOuter);

	procDirectInputCreateW =
		(HRESULT (WINAPI *)(HINSTANCE, DWORD, LPDIRECTINPUTW*, LPUNKNOWN))
		GetProcAddress(TVPDirectInputLibHandle, "DirectInputCreateA");
	if(!procDirectInputCreateW)
	{
		// missing DirectInputCreate
		::FreeLibrary(TVPDirectInputLibHandle);
		TVPDirectInputLibHandle = NULL;
		return NULL;
	}

	HRESULT hr = procDirectInputCreateW(GetHInstance(), DIRECTINPUT_VERSION,
		&TVPDirectInput, NULL);
	if(FAILED(hr))
	{
		// DirectInputCreate failed
		FreeLibrary(TVPDirectInputLibHandle);
		TVPDirectInputLibHandle = NULL;
		return NULL;
	}

	return TVPDirectInput;
}
//---------------------------------------------------------------------------
void TVPReleaseDirectInput()
{
	TVPDirectInputLibRefCount--;
	if(TVPDirectInputLibRefCount == 0)
	{
		if(TVPDirectInput)
			TVPDirectInput->Release(), TVPDirectInput = NULL;
		if(TVPDirectInputLibHandle)
			FreeLibrary(TVPDirectInputLibHandle), TVPDirectInputLibHandle = NULL;
		TVPDirectInputInit = false;
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPDirectInputDevice : A base class for managing DirectInput device
//---------------------------------------------------------------------------
tTVPDirectInputDevice::tTVPDirectInputDevice()
{
	TVPAddRefDirectInput();
	Device = NULL;
}
//---------------------------------------------------------------------------
tTVPDirectInputDevice::~tTVPDirectInputDevice()
{
	if(Device)
	{
		Device->Unacquire();
		Device->Release();
	}
	TVPReleaseDirectInput();
}
//---------------------------------------------------------------------------
void tTVPDirectInputDevice::SetCooperativeLevel(HWND window)
{
	if(Device)
	{
		if(FAILED(Device->SetCooperativeLevel(
			window, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND)))
		{
			Device->Release();
			Device = NULL;
		}
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPWheelDirectInputDevice : DirectInput device manager for mouse wheel
//---------------------------------------------------------------------------
tTVPWheelDirectInputDevice::tTVPWheelDirectInputDevice(HWND window) :
	tTVPDirectInputDevice()
{
	if(TVPDirectInput)
	{
		if(TVPWheelDetectionType == wdtDirectInput)
		{
			// initialize mouse wheel
			IDirectInputDevice *dev = NULL;
			HRESULT hr = TVPDirectInput->CreateDevice(
				GUID_SysMouse, &dev, NULL);
			if(SUCCEEDED(hr) && dev)
			{
				hr	= dev->QueryInterface(
					IID_IDirectInputDevice2, (LPVOID*)&Device);
				dev->Release();
				if(FAILED(hr)) Device = NULL;
			}
			if(SUCCEEDED(hr) && Device)
			{
				if(FAILED(Device->SetDataFormat(&TVPWheelDIDF)))
				{
					Device->Release();
					Device = NULL;
				}
				if(Device) SetCooperativeLevel(window);
				if(Device) Device->Acquire();
			}
			else
			{
				Device = NULL;
			}
		}
	}
}
//---------------------------------------------------------------------------
tTVPWheelDirectInputDevice::~tTVPWheelDirectInputDevice()
{
}
//---------------------------------------------------------------------------
tjs_int tTVPWheelDirectInputDevice::GetWheelDelta()
{
	if(!Device) return 0;

	tTVPDIWheelData wd;
	ZeroMemory(&wd, sizeof(wd));
	HRESULT hr = Device->GetDeviceState(sizeof(wd), &wd);
	if(FAILED(hr))
	{
		DWORD tick = GetTickCount();
		do
		{
			hr = Device->Acquire();
		} while(hr == DIERR_INPUTLOST && (GetTickCount() - tick) < 20);
			// has 20ms timeout

		if(!FAILED(hr))
		{
			HRESULT hr = Device->GetDeviceState(sizeof(wd), &wd);
			if(FAILED(hr)) return 0;
		}
	}

	return (tjs_int)wd.delta;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPPadDirectInputDevice : DirectInput device manager for JoyPad
//---------------------------------------------------------------------------
std::vector<tTVPPadDirectInputDevice*>  *
	tTVPPadDirectInputDevice::PadDevices = NULL;
tjs_uint32 tTVPPadDirectInputDevice::GlobalPadPushedFlag = 0;
//---------------------------------------------------------------------------
tTVPPadDirectInputDevice::tTVPPadDirectInputDevice(HWND window) :
	tTVPDirectInputDevice()
{
	LastPadState = 0;
	LastPushedTrigger = 0;
	KeyUpdateMask = 0;

	if(TVPDirectInput)
	{
		if(TVPJoyPadDetectionType == jdtDirectInput)
		{
			// initialize joy pad
			// enumerate JoyStick devices. need DIRECTINPUT_VERSION 0x0500 or higher.
			HRESULT hr;
			hr = TVPDirectInput->EnumDevices(DIDEVTYPE_JOYSTICK,
				(LPDIENUMDEVICESCALLBACK)EnumJoySticksCallback,
				&Device, DIEDFL_ATTACHEDONLY);
			if(FAILED(hr) || Device==NULL)
			{
				Device	= NULL;
			}

			//	initialize joy stick device.
			if(Device)
			{
				if(FAILED(Device->SetDataFormat(&c_dfPad)))
				{
					Device->Release();
					Device = NULL;
				}
			}
			if(Device) SetCooperativeLevel(window);
			if(Device)
			{
				DIPROPRANGE	range;
				DIPROPDWORD	pw;
				ZeroMemory(&range, sizeof(range));
				ZeroMemory(&pw, sizeof(pw));
				range.diph.dwSize		= sizeof(DIPROPRANGE);
				pw.diph.dwSize			= sizeof(DIPROPDWORD);
				range.diph.dwHeaderSize	= pw.diph.dwHeaderSize	= sizeof(DIPROPHEADER);
				range.diph.dwHow		= pw.diph.dwHow			= DIPH_BYOFFSET;
				range.lMin				= PadAxisMin;
				range.lMax				= PadAxisMax;
				pw.dwData				= 5000;		//	upper 50 percent
				range.diph.dwObj	= pw.diph.dwObj	= DIJOFS_X;		//	X Axis
				hr = Device->SetProperty(DIPROP_RANGE, &range.diph);
				hr |=Device->SetProperty(DIPROP_DEADZONE, &pw.diph);
				range.diph.dwObj	= pw.diph.dwObj	= DIJOFS_Y;		//	Y Axis
				hr |=Device->SetProperty(DIPROP_RANGE, &range.diph);
				hr |=Device->SetProperty(DIPROP_DEADZONE, &pw.diph);
				if(FAILED(hr))
				{
					Device->Release();
					Device = NULL;
				}
			}
			if(Device) Device->Acquire();

			// for polling state
			LastPadState       = 0;
		}
	}

	// register self to PadDevices
	if(!PadDevices)
		PadDevices = new std::vector<tTVPPadDirectInputDevice*>();
	PadDevices->push_back(this);
}
//---------------------------------------------------------------------------
tTVPPadDirectInputDevice::~tTVPPadDirectInputDevice()
{
	// unregister from PadDevices
	std::vector<tTVPPadDirectInputDevice*>::iterator i;
	i = std::find(PadDevices->begin(), PadDevices->end(), this);
	if(i != PadDevices->end()) PadDevices->erase(i);
	if(PadDevices->size() == 0) delete PadDevices, PadDevices = NULL;
}
//---------------------------------------------------------------------------
bool CALLBACK tTVPPadDirectInputDevice::EnumJoySticksCallback(
	LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	HRESULT	hr;
	IDirectInputDevice*	dev;

	//	joy stick detect first is created.
	hr	= TVPDirectInput->CreateDevice(lpddi->guidInstance,
		(IDirectInputDevice**)&dev, NULL);

	if(FAILED(hr)) return DIENUM_CONTINUE;

	//	expanse DirectInputDevice to DirectInputDevice2
	hr	= dev->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)pvRef);
	dev->Release();
	if(FAILED(hr)) return DIENUM_CONTINUE;

	return DIENUM_STOP;
}
//---------------------------------------------------------------------------
void tTVPPadDirectInputDevice::Update(tjs_uint32 newstate)
{
	UppedKeys.clear();
	DownedKeys.clear();
	RepeatKeys.clear();

	tjs_uint32 downed = newstate & ~LastPadState; // newly pressed buttons
	tjs_uint32 upped  = ~newstate & LastPadState; // newly released buttons

	GlobalPadPushedFlag |= downed;


	// emulate key repeats.
	// key repeats are calculated about two groups independently.
	// one is cross-keys(up, down, left, right) and the other is
	// trigger buttons(button0 .. button9).
	const tjs_uint32 cross_group_mask =
		((1<<pkfLeft)|(1<<pkfRight)|(1<<pkfUp)|(1<<pkfDown));
	const tjs_uint32 trigger_group_mask = ~cross_group_mask;

	if(!(LastPadState & cross_group_mask) && (newstate & cross_group_mask))
		CrossKeysRepeater.Down(); // any pressed
	if(!(newstate     & cross_group_mask))
		CrossKeysRepeater.Up(); // all released

	if     (downed & trigger_group_mask) TriggerKeysRepeater.Down();
	else if(upped  & trigger_group_mask) TriggerKeysRepeater.Up();

	if(downed & trigger_group_mask) LastPushedTrigger = downed & trigger_group_mask;

	// scan downed buttons
	for(tjs_int i = 0; i < TVP_NUM_PAD_KEY; i++)
		if((1<<i) & downed) DownedKeys.push_back(TVPPadVirtualKeyMap[i]);
	// scan upped buttons
	for(tjs_int i = 0; i < TVP_NUM_PAD_KEY; i++)
		if((1<<i) & upped)  UppedKeys.push_back(TVPPadVirtualKeyMap[i]);
	// scan cross group repeats
	tjs_int cnt;
	cnt = CrossKeysRepeater.GetRepeatCount();
	if(cnt)
	{
		tjs_uint32 t = newstate & cross_group_mask;
		do
		{
			for(tjs_int i = 0; i < TVP_NUM_PAD_KEY; i++)
				if((1<<i) & t) RepeatKeys.push_back(TVPPadVirtualKeyMap[i]);
		} while(--cnt);
	}

	// scan trigger group repeats
	cnt = TriggerKeysRepeater.GetRepeatCount();
	if(cnt)
	{
		tjs_uint32 t = LastPushedTrigger;
		do
		{
			for(tjs_int i = 0; i < TVP_NUM_PAD_KEY; i++)
			{
				if((1<<i) & t)
				{
					RepeatKeys.push_back(TVPPadVirtualKeyMap[i]);
					break;
				}
			}
		} while(--cnt);
	}

	// update last state
	LastPadState = newstate;
}
//---------------------------------------------------------------------------
void tTVPPadDirectInputDevice::UpdateWithCurrentState()
{
	// called every 50ms intervally
	tjs_uint32 state = GetState();
	KeyUpdateMask |= ~state;
	Update(state & KeyUpdateMask);
}
//---------------------------------------------------------------------------
void tTVPPadDirectInputDevice::UpdateWithSuspendedState()
{
	// called when the window is inactive
	KeyUpdateMask = 0;
	Update(0);
}
//---------------------------------------------------------------------------
void tTVPPadDirectInputDevice::WindowActivated()
{
	// dummy read to obtain stable pad status ... nasty.

	GetState();
	GetState();
	GetState();
}
//---------------------------------------------------------------------------
void tTVPPadDirectInputDevice::WindowDeactivated()
{
	KeyUpdateMask = 0;
}
//---------------------------------------------------------------------------
tjs_uint32 tTVPPadDirectInputDevice::GetState()
{
	if(!Device)	return 0;

	// polling
	if(FAILED(Device->Poll()))
	{
		//  for lost.
		Device->Acquire();
		return 0;
	}

	DIJOYSTATE	js;
	ZeroMemory(&js, sizeof(js));
	HRESULT	hr = Device->GetDeviceState(sizeof(js), &js);
	if(FAILED(hr))
	{
		DWORD tick = GetTickCount();
		do
		{
			hr = Device->Acquire();
		} while(hr == DIERR_INPUTLOST && (GetTickCount() - tick) < 20);
			// has 20ms timeout

		if(!FAILED(hr))
		{
			HRESULT hr = Device->GetDeviceState(sizeof(js), &js);
			if(FAILED(hr)) return 0;
		}
	}

	//	structure DIJOYSTATE => unsigned integer JoyState
	tjs_uint32	press	= 0;
#define JOY_CROSSKEY(value, plus, minus)	((value)>=PadAxisUpperThreshold ? \
			(plus) : ((value)<=PadAxisLowerThreshold ? (minus) : 0))
#define	JOY_BUTTON(value, on)				((value&0x80) ? (on) : 0)
	press	|= JOY_CROSSKEY(js.lX, (1<<pkfRight), (1<<pkfLeft));
	press	|= JOY_CROSSKEY(js.lY, (1<<pkfDown), (1<<pkfUp));
/*#ifdef _DEBUG
	if(js.lX!=0 || js.lY!=0)
	{
		TVPAddLog(TJS_W("JoyStick: lX = ")+ttstr((int)js.lX)+TJS_W(", lY = ")+ttstr((int)js.lY));
	}
#endif
*/
	press	|= JOY_BUTTON(js.rgbButtons[0], 1<<pkfButton0);
	press	|= JOY_BUTTON(js.rgbButtons[1], 1<<pkfButton1);
	press	|= JOY_BUTTON(js.rgbButtons[2], 1<<pkfButton2);
	press	|= JOY_BUTTON(js.rgbButtons[3], 1<<pkfButton3);
	press	|= JOY_BUTTON(js.rgbButtons[4], 1<<pkfButton4);
	press	|= JOY_BUTTON(js.rgbButtons[5], 1<<pkfButton5);
	press	|= JOY_BUTTON(js.rgbButtons[6], 1<<pkfButton6);
	press	|= JOY_BUTTON(js.rgbButtons[7], 1<<pkfButton7);
	press	|= JOY_BUTTON(js.rgbButtons[8], 1<<pkfButton8);
	press	|= JOY_BUTTON(js.rgbButtons[9], 1<<pkfButton9);
#undef JOY_BUTTON
#undef JOY_CROSSKEY

	return press;
}
//---------------------------------------------------------------------------
tjs_uint32 tTVPPadDirectInputDevice::GetGlobalPadState()
{
	if(!PadDevices) return 0;
	std::vector<tTVPPadDirectInputDevice*>::iterator i;
	tjs_uint32 state = 0;
	for(i = PadDevices->begin(); i != PadDevices->end(); i++)
		state |= (*i)->LastPadState;
	return state;
}
//---------------------------------------------------------------------------
bool tTVPPadDirectInputDevice::GetAsyncState(tjs_uint keycode, bool getcurrent)
{
	// This emulates System.getKeyState
	tjs_uint32 bit;
	if(keycode != VK_PADANY)
	{
		tTVPPadKeyFlag flag = TVPVirtualKeyToPadCode(keycode);
		if(flag == (tTVPPadKeyFlag)-1) return false;

		bit = (1<<flag);
	}
	else
	{
		// returns whether any pad buttons are pressed
		bit = -1;
	}

	if(getcurrent)
	{
		return 0!=(GetGlobalPadState() & bit);
	}
	else
	{
		bool ret = 0!=(GlobalPadPushedFlag  & bit);
		GlobalPadPushedFlag &= ~bit;
		return ret;
	}
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
static void TVPUninitDirectInput()
{
	// release all devices
	Application->FreeDirectInputDeviceForWindows();
}
//---------------------------------------------------------------------------
static tTVPAtExit
	TVPUninitDirectInputAtExit(TVP_ATEXIT_PRI_RELEASE, TVPUninitDirectInput);
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// Utility functionss
//---------------------------------------------------------------------------
bool TVPGetJoyPadAsyncState(tjs_uint keycode, bool getcurrent)
{
	return tTVPPadDirectInputDevice::GetAsyncState(keycode, getcurrent);
}
//---------------------------------------------------------------------------
#endif
