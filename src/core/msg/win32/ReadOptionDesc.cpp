
#include "tjsCommHead.h"
#include "picojson.h"
#include "MsgIntf.h"
#include "DebugIntf.h"
#include "resource.h"
#include "CharacterSet.h"
#include "ReadOptionDesc.h"
#include "WindowsUtil.h"

class OptionDescReader {
	static const char* CATEGORY;
	static const char* OPTIONS;
	static const char* CAPTION;
	static const char* DESCRIPTION;
	static const char* NAME;
	static const char* TYPE;
	static const char* LENGTH;
	static const char* VALUE;
	static const char* VALUES;
	static const char* DESC;
	static const char* DEFAULT;
	static const char* USER;

	static bool GetBoolean( const char* name, const picojson::object& obj ) {
		std::map<std::string, picojson::value>::const_iterator v = obj.find(std::string(name));
		if( v != obj.end() ) {
			const picojson::value& val = v->second;
			if( val.is<bool>() ) {
				return val.get<bool>();
			}
		}
		return false;
	}
	static int GetInteger( const char* name, const picojson::object& obj ) {
		std::map<std::string, picojson::value>::const_iterator v = obj.find(std::string(name));
		if( v != obj.end() ) {
			const picojson::value& val = v->second;
			if( val.is<int>() ) {
				return (int)val.get<double>();
			}
		}
		return 0;
	}
	static void RetriveString( std::wstring& out, const picojson::object& obj, const char* name ) {
		std::map<std::string, picojson::value>::const_iterator v = obj.find(std::string(name));
		if( v != obj.end() ) {
			const picojson::value& val = v->second;
			if( val.is<std::string>() ) {
				TVPUtf8ToUtf16( out,  val.get<std::string>() );
			} else if( val.is<double>() ) {
				int vi = (int)val.get<double>();
				out = std::to_wstring(vi);
			} else if( val.is<bool>() ) {
				bool b = val.get<bool>();
				if( b ) out = L"true";
				else out = L"false";
			}
		}
	}
	void ParseOption( tTVPCommandOption& opt, const picojson::object& option );
	void ParseValue( tTVPCommandOptionsValue& val, const picojson::object& value );
public:
	tTVPCommandOptionList* Parse( const picojson::value& v );
};

const char* OptionDescReader::CATEGORY = "category";
const char* OptionDescReader::OPTIONS = "options";
const char* OptionDescReader::CAPTION = "caption";
const char* OptionDescReader::DESCRIPTION = "description";
const char* OptionDescReader::NAME = "name";
const char* OptionDescReader::TYPE = "type";
const char* OptionDescReader::LENGTH = "length";
const char* OptionDescReader::VALUE = "value";
const char* OptionDescReader::VALUES = "values";
const char* OptionDescReader::DESC = "desc";
const char* OptionDescReader::DEFAULT = "default";
const char* OptionDescReader::USER = "user";

void OptionDescReader::ParseValue( tTVPCommandOptionsValue& val, const picojson::object& value ) {
	RetriveString( val.Value, value, VALUE );
	RetriveString( val.Description, value, DESC );
	val.IsDefault = GetBoolean( DEFAULT, value );
}
void OptionDescReader::ParseOption( tTVPCommandOption& opt, const picojson::object& option ) {
	RetriveString( opt.Caption, option, CAPTION );
	RetriveString( opt.Description, option, DESCRIPTION );
	RetriveString( opt.Name, option, NAME );
	std::wstring type;
	RetriveString( type, option, TYPE );
	opt.Length = 0;
	if( type == std::wstring(L"select") ) {
		opt.Type = tTVPCommandOption::VT_Select;
	} else if( type == std::wstring(L"string") ) {
		opt.Type = tTVPCommandOption::VT_String;
		opt.Length = GetInteger( LENGTH, option );
		RetriveString( opt.Value, option, VALUE );
	} else {
		opt.Type = tTVPCommandOption::VT_Unknown;
	}
	opt.User = GetBoolean( USER, option );
	std::map<std::string, picojson::value>::const_iterator v = option.find(std::string(VALUES));
	if( v != option.end() ) {
		const picojson::value& val = v->second;
		if( val.is<picojson::array>() ) {
			const picojson::array& values = val.get<picojson::array>();
			size_t count = values.size();
			opt.Values.resize( count );
			for( size_t i = 0; i < count; i++ ) {
				tTVPCommandOptionsValue& value = opt.Values[i];
				const picojson::value& jval = values[i];
				if( jval.is<picojson::object>() ) {
					ParseValue( value, jval.get<picojson::object>() );
				}
			}
		}
	}
}

tTVPCommandOptionList* OptionDescReader::Parse( const picojson::value& v ) {
	tTVPCommandOptionList* result = NULL;
	if( v.is<picojson::array>() ) {
		result = new tTVPCommandOptionList();
		const picojson::array& categories = v.get<picojson::array>();
		size_t count = categories.size();
		result->Categories.resize( count );
		for( size_t i = 0; i < count; i++ ) {
			tTVPCommandOptionCategory& optioncat = result->Categories[i];
			const picojson::value& category = categories[i];
			if( category.is<picojson::object>() ) {
				const picojson::object& cat = category.get<picojson::object>();
				RetriveString( optioncat.Name, cat, CATEGORY );

				std::map<std::string, picojson::value>::const_iterator opt = cat.find(OPTIONS);
				if( opt != cat.end() ) {
					const picojson::value& options = opt->second;
					if( options.is<picojson::array>() ) {
						const picojson::array& optionarray = options.get<picojson::array>();
						size_t optcount = optionarray.size();
						optioncat.Options.resize( optcount );
						for( size_t j = 0; j < optcount; j++ ) {
							tTVPCommandOption& toption = optioncat.Options[j];
							const picojson::value& option = optionarray[j];
							if( option.is<picojson::object>() ) {
								ParseOption( toption, option.get<picojson::object>() );
							}
						}
					}
				}
			}
		}
	}
	return result;
}

bool TVPUtf8ToUtf16( std::wstring& out, const std::string& in ) {
	tjs_int len = TVPUtf8ToWideCharString( in.c_str(), NULL );
	if( len < 0 ) return false;
	wchar_t* buf = new wchar_t[len];
	if( buf ) {
		try {
			len = TVPUtf8ToWideCharString( in.c_str(), buf );
			if( len > 0 ) out.assign( buf, len );
			delete[] buf;
		} catch(...) {
			delete[] buf;
			throw;
		}
	}
	return len > 0;
}
bool TVPUtf16ToUtf8( std::string& out, const std::wstring& in ) {
	tjs_int len = TVPWideCharToUtf8String( in.c_str(), NULL );
	if( len < 0 ) return false;
	char* buf = new char[len];
	if( buf ) {
		try {
			len = TVPWideCharToUtf8String( in.c_str(), buf );
			if( len > 0 ) out.assign( buf, len );
			delete[] buf;
		} catch(...) {
			delete[] buf;
			throw;
		}
	}
	return len > 0;
}
tTVPCommandOptionList* ParseCommandDesc( const char *buf, unsigned int size ) {
	if( buf == NULL || size <= 0 ) return NULL;

	picojson::value v;
	std::string errorstr;
	picojson::parse( v, buf, buf+size, &errorstr );
	if( errorstr.empty() != true ) {
		std::wstring errmessage;
		if( TVPUtf8ToUtf16( errmessage, errorstr ) ) 	
			TVPAddImportantLog( errmessage.c_str() );
		return NULL;
	}
	OptionDescReader reader;
	tTVPCommandOptionList* opt = reader.Parse( v );
	return opt;
}
extern "C" {
static BOOL CALLBACK EnumResTypeProc( HMODULE hModule, LPTSTR lpszType, LONG_PTR lParam ) {
	if( !IS_INTRESOURCE(lpszType) ) {
		OutputDebugString( lpszType );
	}
	return TRUE;
}
static BOOL CALLBACK TVPEnumResNameProc( HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam ) {
	if( !IS_INTRESOURCE(lpszName) ) {
		OutputDebugString( lpszName );
	}
	return TRUE;
}
};
tTVPCommandOptionList* TVPGetPluginCommandDesc( const wchar_t* name ) {
	HMODULE hModule = ::LoadLibraryEx( name, NULL, LOAD_LIBRARY_AS_DATAFILE );
	if( hModule == NULL ) return NULL;
	const char *buf = NULL;
	unsigned int size = 0;
	tTVPCommandOptionList* ret = NULL;
	try {
		//BOOL typeret = ::EnumResourceTypes( hModule, (ENUMRESTYPEPROC)EnumResTypeProc,  NULL );
		//BOOL resret = ::EnumResourceNames( hModule, L"TEXT", (ENUMRESNAMEPROC)TVPEnumResNameProc, NULL );

		HRSRC hRsrc = ::FindResource(hModule, L"IDR_OPTION_DESC_JSON", L"TEXT" );
		if( hRsrc != NULL ) {
			size = ::SizeofResource( hModule, hRsrc );
			HGLOBAL hGlobal = ::LoadResource( hModule, hRsrc );
			if( hGlobal != NULL ) {
				buf = reinterpret_cast<const char*>(::LockResource(hGlobal));
			}
		}
		ret = ParseCommandDesc( buf, size );
	} catch(...) {
		::FreeLibrary( hModule );
		throw;
	}
	::FreeLibrary( hModule );
	return ret;
}
tTVPCommandOptionList* TVPGetEngineCommandDesc() {
	HMODULE hModule = ::GetModuleHandle(NULL);
	if( hModule == NULL ) return NULL;
	const char *buf = NULL;
	unsigned int size = 0;
	HRSRC hRsrc = ::FindResource(hModule, MAKEINTRESOURCE(IDR_OPTION_DESC_JSON), TEXT("TEXT"));
	if( hRsrc != NULL ) {
		size = ::SizeofResource( hModule, hRsrc );
		HGLOBAL hGlobal = ::LoadResource( hModule, hRsrc );
		if( hGlobal != NULL ) {
			buf = reinterpret_cast<const char*>(::LockResource(hGlobal));
		}
	}
	return ParseCommandDesc( buf, size );
}

void TVPMargeCommandDesc( tTVPCommandOptionList& dest, const tTVPCommandOptionList& src ) {
	tjs_uint count = (tjs_uint)src.Categories.size();
	std::vector<tjs_uint> addcat;
	addcat.reserve( count );
	for( tjs_uint i = 0; i < count; i++ ) {
		const tTVPCommandOptionCategory& srccat = src.Categories[i];
		tjs_uint dcnt = (tjs_uint)dest.Categories.size();
		bool found = false;
		for( tjs_uint j = 0; j < dcnt; j++ ) {
			tTVPCommandOptionCategory& dstcat = dest.Categories[j];
			if( dstcat.Name == srccat.Name ) {
				found = true;
				tjs_uint optcount = (tjs_uint)srccat.Options.size();
				dstcat.Options.reserve( dstcat.Options.size() + optcount );
				for( tjs_uint k = 0; k < optcount; k++ ) {
					dstcat.Options.push_back( srccat.Options[k] );
				}
				break;
			}
		}
		if( found == false ) {
			addcat.push_back( i );
		}
	}
	for( std::vector<tjs_uint>::const_iterator i = addcat.begin(); i != addcat.end(); i++ ) {
		tjs_uint idx = *i;
		dest.Categories.push_back( src.Categories[idx] );
	}
}
