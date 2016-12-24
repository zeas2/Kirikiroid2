//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2013 T.Imoto and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------


#ifndef tjsOctPackH
#define tjsOctPackH


namespace TJS
{
extern tjs_error TJSOctetPack( tTJSVariant **args, tjs_int numargs, const std::vector<tTJSVariant>& items, tTJSVariant *result );
extern tjs_error TJSOctetUnpack( const tTJSVariantOctet * target, tTJSVariant **args, tjs_int numargs, tTJSVariant *result );
} // namespace TJS

#endif // tjsOctPackH
