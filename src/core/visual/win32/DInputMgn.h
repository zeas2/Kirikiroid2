//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// DirectInput management
//---------------------------------------------------------------------------


#ifndef __DINPUTMGN_H__
#define __DINPUTMGN_H__

#define DIRECTINPUT_VERSION 0x0500
//#include <dinput.h>
#ifndef CALLBACK
#define CALLBACK
typedef const void  *LPCDIDEVICEINSTANCE;
#endif

#include <vector>

//---------------------------------------------------------------------------
// Enumerations/constants
//---------------------------------------------------------------------------
enum tTVPWheelDetectionType { wdtNone, wdtDirectInput, wdtWindowMessage };
enum tTVPJoyPadDetectionType { jdtNone, jdtDirectInput };
enum tTVPPadKeyFlag {
	pkfLeft,
	pkfRight,
	pkfUp,
	pkfDown,
//	pkfButton,
	pkfButton0,
	pkfButton1,
	pkfButton2,
	pkfButton3,
	pkfButton4,
	pkfButton5,
	pkfButton6,
	pkfButton7,
	pkfButton8,
	pkfButton9,
};
#define TVP_NUM_PAD_KEY (4 + 10)
		// count of supported pad buttions (including cross keys)
extern const int TVPPadVirtualKeyMap[TVP_NUM_PAD_KEY];
extern tTVPWheelDetectionType TVPWheelDetectionType; // = wdtDirectInput;
extern tTVPJoyPadDetectionType TVPJoyPadDetectionType; // = jdtDirectInput;




//---------------------------------------------------------------------------
// tTVPKeyRepeatEmulator : A class for emulating keyboard key repeats.
//---------------------------------------------------------------------------
class tTVPKeyRepeatEmulator
{
	tjs_uint32 PressedTick;
	bool  Pressed;
	tjs_int LastRepeatCount;

	static tjs_int HoldTime; // keyboard key-repeats hold-time
	static tjs_int IntervalTime; // keyboard key-repeats interval-time


	/*
		          hold time           repeat interval time
		   <------------------------> <--->
		   +------------------------+ +-+ +-+ +-+
		___|                        |_| |_| |_| |____
		   <------------------------------------->
		   key pressed                           key released
		-----------------------------------------------------------> time

	*/

	static void GetKeyRepeatParam();

public:
	tTVPKeyRepeatEmulator();
	~tTVPKeyRepeatEmulator();

	void Down();
	void Up();

	tjs_int GetRepeatCount();
};
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTVPDirectInputDevice : A base class for managing DirectInput device
//---------------------------------------------------------------------------
struct IDirectInputDevice2;
class tTVPDirectInputDevice
{
protected:
	IDirectInputDevice2 * Device;

public:
	tTVPDirectInputDevice();

	virtual ~tTVPDirectInputDevice();

	void SetCooperativeLevel(int window);
		// set cooperatice level to the Device.
		// This may be called from the constructor
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTVPWheelDirectInputDevice : DirectInput device manager for mouse wheel
//---------------------------------------------------------------------------
class tTVPWheelDirectInputDevice : public tTVPDirectInputDevice
{
public:
	tTVPWheelDirectInputDevice(int window);
	~tTVPWheelDirectInputDevice();

	tjs_int GetWheelDelta();
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTVPPadDirectInputDevice : DirectInput device manager for JoyPad
//---------------------------------------------------------------------------
class tTVPPadDirectInputDevice : public tTVPDirectInputDevice
{
	tjs_uint32 LastPadState;
	tjs_uint32 LastPushedTrigger;

	tjs_uint32 KeyUpdateMask;

	tTVPKeyRepeatEmulator CrossKeysRepeater;
	tTVPKeyRepeatEmulator TriggerKeysRepeater;

	std::vector<tjs_uint16> UppedKeys;
	std::vector<tjs_uint16> DownedKeys;
	std::vector<tjs_uint16> RepeatKeys;

public:
	tTVPPadDirectInputDevice(int window);
	~tTVPPadDirectInputDevice();

private:
	static bool CALLBACK EnumJoySticksCallback(
		LPCDIDEVICEINSTANCE lpddi, void* pvRef);

private:
	void Update(tjs_uint32 newstate);

public:
	void UpdateWithCurrentState();
	void UpdateWithSuspendedState();

	void WindowActivated();
	void WindowDeactivated();

	const std::vector<tjs_uint16>& GetUppedKeys() const { return UppedKeys; }
	const std::vector<tjs_uint16>& GetDownedKeys() const { return DownedKeys; }
	const std::vector<tjs_uint16>& GetRepeatKeys() const { return RepeatKeys; }

private:
	tjs_uint32 GetState();

private:
	static std::vector<tTVPPadDirectInputDevice*> * PadDevices;
    static tjs_uint32 GlobalPadPushedFlag;

public:
	static tjs_uint32 GetGlobalPadState();
		// returns global (not on Window context) JoyPad State.

	static bool GetAsyncState(tjs_uint keycode, bool getcurrent);
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Utility functionss
//---------------------------------------------------------------------------
bool TVPGetJoyPadAsyncState(tjs_uint keycode, bool getcurrent);
//---------------------------------------------------------------------------


#endif
