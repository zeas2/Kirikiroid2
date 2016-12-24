//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// iTJSDispatch2 interface definition
//---------------------------------------------------------------------------
#ifndef tjsInterfaceH
#define tjsInterfaceH

#include "tjsConfig.h"
#include "tjsErrorDefs.h"

namespace TJS
{
//---------------------------------------------------------------------------

/*[*/
//---------------------------------------------------------------------------
// call flag type
//---------------------------------------------------------------------------
#define TJS_MEMBERENSURE		0x00000200 // create a member if not exists
#define TJS_MEMBERMUSTEXIST     0x00000400 // member *must* exist ( for Dictionary/Array )
#define TJS_IGNOREPROP			0x00000800 // ignore property invoking
#define TJS_HIDDENMEMBER		0x00001000 // member is hidden
#define TJS_STATICMEMBER		0x00010000 // member is not registered to the
										   // object (internal use)

#define TJS_ENUM_NO_VALUE		0x00100000 // values are not retrieved
										   // (for EnumMembers)

#define TJS_NIS_REGISTER		0x00000001 // set native pointer
#define TJS_NIS_GETINSTANCE		0x00000002 // get native pointer

#define TJS_CII_ADD				0x00000001 // register name
										   // 'num' argument passed to CII is to be igonored.
#define TJS_CII_GET				0x00000000 // retrieve name

#define TJS_CII_SET_FINALIZE	0x00000002 // register "finalize" method name
										   // (set empty string not to call the method)
										   // 'num' argument passed to CII is to be igonored.
#define TJS_CII_SET_MISSING		0x00000003 // register "missing" method name.
										   // the method is called when the member is not present.
										   // (set empty string not to call the method)
										   // 'num' argument passed to CII is to be igonored.
										   // the method is to be called with three arguments;
										   // get_or_set    : false for get, true for set
										   // name          : member name
										   // value         : value property; you must
										   //               : dereference using unary '*' operator.
										   // the method must return true for found, false for not-found.
#define TJS_CII_SET_SUPRECLASS	0x00000004 // register super class instance
#define TJS_CII_GET_SUPRECLASS	0x00000005 // retrieve super class instance

#define TJS_OL_LOCK				0x00000001 // Lock the object
#define TJS_OL_UNLOCK			0x00000002 // Unlock the object



//---------------------------------------------------------------------------
// 	Operation  flag
//---------------------------------------------------------------------------

#define TJS_OP_BAND				0x0001
#define TJS_OP_BOR				0x0002
#define TJS_OP_BXOR				0x0003
#define TJS_OP_SUB				0x0004
#define TJS_OP_ADD				0x0005
#define TJS_OP_MOD				0x0006
#define TJS_OP_DIV				0x0007
#define TJS_OP_IDIV				0x0008
#define TJS_OP_MUL				0x0009
#define TJS_OP_LOR				0x000a
#define TJS_OP_LAND				0x000b
#define TJS_OP_SAR				0x000c
#define TJS_OP_SAL				0x000d
#define TJS_OP_SR				0x000e
#define TJS_OP_INC				0x000f
#define TJS_OP_DEC				0x0010

#define TJS_OP_MASK				0x001f

#define TJS_OP_MIN				TJS_OP_BAND
#define TJS_OP_MAX				TJS_OP_DEC

/* SAR = Shift Arithmetic Right, SR = Shift (bitwise) Right */



//---------------------------------------------------------------------------
// iTJSDispatch
//---------------------------------------------------------------------------
/*
	iTJSDispatch interface
*/
class tTJSVariant;
class tTJSVariantClosure;
class tTJSVariantString;
class iTJSNativeInstance;
class iTJSDispatch2
{
/*
	methods, that have "ByNum" at the end of the name, have
	"num" parameter that enables the function to call a member with number directly.
	following two have the same effect:
	FuncCall(NULL, L"123", NULL, 0, NULL, NULL);
	FuncCallByNum(NULL, 123, NULL, 0, NULL, NULL);
*/

public:
	virtual tjs_uint TJS_INTF_METHOD AddRef(void) = 0;
	virtual tjs_uint TJS_INTF_METHOD Release(void) = 0;

public:

	virtual tjs_error TJS_INTF_METHOD
	FuncCall( // function invocation
		tjs_uint32 flag,			// calling flag
		const tjs_char * membername,// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		tTJSVariant *result,		// result
		tjs_int numparams,			// number of parameters
		tTJSVariant **param,		// parameters
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	FuncCallByNum( // function invocation by index number
		tjs_uint32 flag,			// calling flag
		tjs_int num,				// index number
		tTJSVariant *result,		// result
		tjs_int numparams,			// number of parameters
		tTJSVariant **param,		// parameters
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	PropGet( // property get
		tjs_uint32 flag,			// calling flag
		const tjs_char * membername,// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		tTJSVariant *result,		// result
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	PropGetByNum( // property get by index number
		tjs_uint32 flag,			// calling flag
		tjs_int num,				// index number
		tTJSVariant *result,		// result
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	PropSet( // property set
		tjs_uint32 flag,			// calling flag
		const tjs_char *membername,	// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		const tTJSVariant *param,	// parameters
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	PropSetByNum( // property set by index number
		tjs_uint32 flag,			// calling flag
		tjs_int num,				// index number
		const tTJSVariant *param,	// parameters
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	GetCount( // get member count
		tjs_int *result,         	// variable that receives the result
		const tjs_char *membername,	// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		iTJSDispatch2 *objthis      // object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	GetCountByNum( // get member count by index number
		tjs_int *result,			// variable that receives the result
		tjs_int num,				// by index number
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	PropSetByVS( // property set by tTJSVariantString, for internal use
		tjs_uint32 flag,			// calling flag
		tTJSVariantString *membername, // member name ( NULL for a default member )
		const tTJSVariant *param,	// parameters
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	EnumMembers( // enumerate members
		tjs_uint32 flag,			// enumeration flag
		tTJSVariantClosure *callback,	// callback function interface ( called on each member )
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	DeleteMember( // delete member
		tjs_uint32 flag,			// calling flag
		const tjs_char *membername,	// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	DeleteMemberByNum( // delete member by index number
		tjs_uint32 flag,			// calling flag
		tjs_int num,				// index number
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	Invalidate( // invalidation
		tjs_uint32 flag,			// calling flag
		const tjs_char *membername,	// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	InvalidateByNum( // invalidation by index number
		tjs_uint32 flag,			// calling flag
		tjs_int num,				// index number
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	IsValid( // get validation, returns TJS_S_TRUE (valid) or TJS_S_FALSE (invalid)
		tjs_uint32 flag,			// calling flag
		const tjs_char *membername,	// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	IsValidByNum( // get validation by index number, returns TJS_S_TRUE (valid) or TJS_S_FALSE (invalid)
		tjs_uint32 flag,			// calling flag
		tjs_int num,				// index number
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	CreateNew( // create new object
		tjs_uint32 flag,			// calling flag
		const tjs_char * membername,// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		iTJSDispatch2 **result,		// result
		tjs_int numparams,			// number of parameters
		tTJSVariant **param,		// parameters
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	CreateNewByNum( // create new object by index number
		tjs_uint32 flag,			// calling flag
		tjs_int num,				// index number
		iTJSDispatch2 **result,		// result
		tjs_int numparams,			// number of parameters
		tTJSVariant **param,		// parameters
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	Reserved1(
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	IsInstanceOf( // class instance matching returns TJS_S_FALSE or TJS_S_TRUE
		tjs_uint32 flag,			// calling flag
		const tjs_char *membername,	// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		const tjs_char *classname,	// class name to inquire
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	IsInstanceOfByNum( // class instance matching by index number
		tjs_uint32 flag,			// calling flag
		tjs_int num,					// index number
		const tjs_char *classname,	// class name to inquire
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	Operation( // operation with member
		tjs_uint32 flag,			// calling flag
		const tjs_char *membername,	// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		tTJSVariant *result,		// result ( can be NULL )
		const tTJSVariant *param,	// parameters
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	OperationByNum( // operation with member by index number
		tjs_uint32 flag,			// calling flag
		tjs_int num,				// index number
		tTJSVariant *result,		// result ( can be NULL )
		const tTJSVariant *param,	// parameters
		iTJSDispatch2 *objthis		// object as "this"
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	NativeInstanceSupport( // support for native instance
		tjs_uint32 flag,			// calling flag
		tjs_int32 classid,			// native class ID
		iTJSNativeInstance **pointer// object pointer
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	ClassInstanceInfo( // support for class instance information
		tjs_uint32 flag,			// calling flag
		tjs_uint num,				// index number
		tTJSVariant *value			// the name
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	Reserved2(
		) = 0;

	virtual tjs_error TJS_INTF_METHOD
	Reserved3(
		) = 0;


};
//---------------------------------------------------------------------------
class iTJSNativeInstance
{
public:
	virtual tjs_error TJS_INTF_METHOD Construct(tjs_int numparams,
		tTJSVariant **param, iTJSDispatch2 *tjs_obj) = 0;
		// TJS constructor
	virtual void TJS_INTF_METHOD Invalidate() = 0;
		// called before destruction
	virtual void TJS_INTF_METHOD Destruct() = 0;
		// must destruct itself
};
/*]*/
//---------------------------------------------------------------------------




} // namespace TJS

using namespace TJS;
#endif
