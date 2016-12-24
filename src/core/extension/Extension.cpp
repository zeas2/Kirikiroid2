
#include "tjsCommHead.h"
#include "Extension.h"

//---------------------------------------------------------------------------
// TVPAddClassHandler related
//---------------------------------------------------------------------------
struct tTVPAtClassInstallInfo {
	tTVPAtClassInstallInfo(const tjs_char* name, iTJSDispatch2* (*handler)(iTJSDispatch2*), const tjs_char* dependences ) {
		Name = name, Handler = handler;
		if( dependences ) {
			ttstr dep(dependences);
			const tjs_char* start = dependences;
			const tjs_char* cur = dependences;
			while( *cur ) {
				if( (*cur) == TJS_W(',') ) {
					if( start != cur ) {
						Dependences.push_back( ttstr(start, cur - start) );
					}
					start = cur+1;
				}
				cur++;
			}
			if( start != cur ) {
				Dependences.push_back(ttstr(start, cur - start));
			}
		}
	}
	const tjs_char* Name;
	iTJSDispatch2* (*Handler)(iTJSDispatch2*);
	std::vector<ttstr> Dependences;	// 依存クラスリスト
};
static std::vector<tTVPAtClassInstallInfo> *TVPAtClassInstallInfos = NULL;
static bool TVPAtInstallClass = false;
//---------------------------------------------------------------------------
void TVPAddClassHandler( const tjs_char* name, iTJSDispatch2* (*handler)(iTJSDispatch2*), const tjs_char* dependences ) {
	if(TVPAtInstallClass) return;

	if(!TVPAtClassInstallInfos) TVPAtClassInstallInfos = new std::vector<tTVPAtClassInstallInfo>();
	TVPAtClassInstallInfos->push_back(tTVPAtClassInstallInfo(name, handler, dependences));
}
//---------------------------------------------------------------------------
void TVPCauseAtInstallExtensionClass( iTJSDispatch2 *global )
{
	if(TVPAtInstallClass) return;
	TVPAtInstallClass = true;

	if( TVPAtClassInstallInfos ) {
		iTJSDispatch2* dsp;
		tTJSVariant val;
		std::vector<tTVPAtClassInstallInfo>::iterator i;
		for(i = TVPAtClassInstallInfos->begin(); i != TVPAtClassInstallInfos->end(); i++) {
			dsp = i->Handler(global);
			val = tTJSVariant( dsp/*, dsp*/);
			dsp->Release();
			global->PropSet(TJS_MEMBERENSURE|TJS_IGNOREPROP, i->Name, NULL, &val, global);
		}
		delete TVPAtClassInstallInfos;
		TVPAtClassInstallInfos = NULL;
	}
}
//---------------------------------------------------------------------------

