//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// message management
//---------------------------------------------------------------------------
#ifndef tjsMessageH
#define tjsMessageH

#include "tjsVariant.h"
#include "tjsString.h"

namespace TJS
{
//---------------------------------------------------------------------------
// this class maps message and its object
//---------------------------------------------------------------------------
extern void TJSAddRefMessageMapper();
extern void TJSReleaseMessageMapper();
class tTJSMessageHolder;
extern void TJSRegisterMessageMap(const tjs_char *name, tTJSMessageHolder *holder);
extern void TJSUnregisterMessageMap(const tjs_char *name);
extern bool TJSAssignMessage(const tjs_char *name, const tjs_char *newmsg);
extern ttstr TJSCreateMessageMapString();
TJS_EXP_FUNC_DEF(ttstr, TJSGetMessageMapMessage, (const tjs_char *name));
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// a simple class to hold message
// this holder should be created as a static object
//---------------------------------------------------------------------------
class tTJSMessageHolder
{
	const tjs_char *Name;
	const tjs_char *DefaultMessage;
	tjs_char *AssignedMessage;

public:
	tTJSMessageHolder(const tjs_char *name, const tjs_char *defmsg, bool regist = true)
	{
		/* "name" and "defmsg" must point static area */
		AssignedMessage = NULL;
		Name = NULL;
		DefaultMessage = defmsg;
		TJSAddRefMessageMapper();
		if(regist)
		{
			Name = name;
			TJSRegisterMessageMap(Name, this);
		}
	}

	~tTJSMessageHolder()
	{
		if(Name) TJSUnregisterMessageMap(Name);
		if(AssignedMessage) delete [] AssignedMessage, AssignedMessage = NULL;
		TJSReleaseMessageMapper();
	}

	void AssignMessage(const tjs_char *msg)
	{
		if(AssignedMessage) delete [] AssignedMessage, AssignedMessage = NULL;
		AssignedMessage = new tjs_char[TJS_strlen(msg) + 1];
		TJS_strcpy(AssignedMessage, msg);
	}

	operator const tjs_char * ()
		{ return AssignedMessage?AssignedMessage:DefaultMessage; }
		/* this function may called after destruction */
};
//---------------------------------------------------------------------------
}
extern ttstr TVPGetMessageByLocale(const std::string &key);
#endif




