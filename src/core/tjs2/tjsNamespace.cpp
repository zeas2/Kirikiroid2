//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Name Space Processing
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsInterface.h"
#include "tjsNamespace.h"
namespace TJS
{
//---------------------------------------------------------------------------
// tTJSLocalSymbolList
//---------------------------------------------------------------------------
/*
	symbol list class for compile-time local variables look-up
*/
tTJSLocalSymbolList::tTJSLocalSymbolList(tjs_int LocalCountStart)
{
	this->LocalCountStart=LocalCountStart;
	StartWriteAddr = NULL;
	CountWriteAddr = NULL;
}
//---------------------------------------------------------------------------
tTJSLocalSymbolList::~tTJSLocalSymbolList(void)
{
	if(StartWriteAddr)
	{
		*StartWriteAddr = LocalCountStart;
	}

	if(CountWriteAddr)
	{
		tjs_int num = GetCount();
		*CountWriteAddr = num;
	}

	size_t i;
	for(i=0;i<List.size();i++)
	{
		if(List[i]) delete [] List[i]->Name;
		delete List[i];
	}
}
//---------------------------------------------------------------------------
void tTJSLocalSymbolList::SetWriteAddr(tjs_int *StartWriteAddr, tjs_int *CountWriteAddr)
{
	this->StartWriteAddr = StartWriteAddr;
	this->CountWriteAddr = CountWriteAddr;
}
//---------------------------------------------------------------------------
void tTJSLocalSymbolList::Add(const tjs_char * name)
{
	if(Find(name)==-1)
	{
		tTJSLocalSymbol *newsym=new tTJSLocalSymbol;
		newsym->Name=new tjs_char[TJS_strlen(name)+1];
		TJS_strcpy(newsym->Name,name);
		size_t i;
		for(i=0;i<List.size();i++)
		{
			tTJSLocalSymbol *sym=List[i];
			if(sym==NULL)
			{
				List[i]=newsym;
				return;
			}
		}
		List.push_back(newsym);
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSLocalSymbolList::Find(const tjs_char *name)
{
	size_t i;
	for(i=0;i<List.size();i++)
	{
		tTJSLocalSymbol *sym=List[i];
		if(sym)
		{
			if(!TJS_strcmp(sym->Name,name))
				return (tjs_int)i;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
void tTJSLocalSymbolList::Remove(const tjs_char *name)
{
	tjs_int idx=Find(name);
	if(idx!=-1)
	{
		tTJSLocalSymbol *sym=List[idx];
		delete [] sym->Name;
		delete sym;
		List[idx]=NULL;  // un-used
	}
}
//---------------------------------------------------------------------------
// tTJSLocalNamespace
//---------------------------------------------------------------------------
/*
 a class for compile-time local variables look-up
*/
#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint time_ns_Push = 0;
tjs_uint time_ns_Pop = 0;
tjs_uint time_ns_Find = 0;
tjs_uint time_ns_Add = 0;
tjs_uint time_ns_Remove = 0;
tjs_uint time_ns_Commit = 0;
#endif

//---------------------------------------------------------------------------
tTJSLocalNamespace::tTJSLocalNamespace(void)
{
	MaxCount=0;
	CurrentCount=0;
	MaxCountWriteAddr = NULL;
}
//---------------------------------------------------------------------------
tTJSLocalNamespace::~tTJSLocalNamespace(void)
{
	if(MaxCountWriteAddr)
	{
		*MaxCountWriteAddr = MaxCount;
	}
}
//---------------------------------------------------------------------------
void tTJSLocalNamespace::SetMaxCountWriteAddr(tjs_int * MaxCountWriteAddr)
{
	this->MaxCountWriteAddr = MaxCountWriteAddr;
}
//---------------------------------------------------------------------------
tjs_int tTJSLocalNamespace::GetCount(void)
{
	tjs_int count=0;
	size_t i;
	for(i=0;i<Levels.size();i++)
	{
		tTJSLocalSymbolList * list= Levels[i];
		count+= list->GetCount();
	}
	return count;
}
//---------------------------------------------------------------------------
void tTJSLocalNamespace::Push()
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_ns_Push);
#endif
	CurrentCount=GetCount();
	tTJSLocalSymbolList * list=new tTJSLocalSymbolList(CurrentCount);
	Levels.push_back(list);
}
//---------------------------------------------------------------------------
void tTJSLocalNamespace::Pop(void)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_ns_Pop);
#endif
	tTJSLocalSymbolList * list= Levels[Levels.size()-1];

	Commit();

	CurrentCount= list->GetLocalCountStart();

	Levels.pop_back();

	delete list;
}
//---------------------------------------------------------------------------
tjs_int tTJSLocalNamespace::Find(const tjs_char *name)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_ns_Find);
#endif
	// search "name"
	tjs_int i;  /* signed */
	for(i=(tjs_int)(Levels.size()-1); i>=0; i--)
	{
		tTJSLocalSymbolList* list = Levels[i];
		tjs_int lidx = list->Find(name);
		if(lidx!=-1)
		{
			lidx += list->GetLocalCountStart();
			return lidx;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
tjs_int tTJSLocalNamespace::GetLevel(void)
{
	// gets current namespace depth
	return (tjs_int)Levels.size();
}
//---------------------------------------------------------------------------
void tTJSLocalNamespace::Add(const tjs_char * name)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_ns_Add);
#endif
	// adds "name" to namespace
	tTJSLocalSymbolList * top = GetTopSymbolList();
	if(!top) return; // this is global
	top->Add(name);
}
//---------------------------------------------------------------------------
void tTJSLocalNamespace::Remove(const tjs_char *name)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_ns_Remove);
#endif
	tjs_int i;
	for(i=(tjs_int)(Levels.size()-1); i>=0; i--)
	{
		tTJSLocalSymbolList* list = Levels[i];
		tjs_int lidx = list->Find(name);
		if(lidx!=-1)
		{
			list->Remove(name);
			return;
		}
	}
}
//---------------------------------------------------------------------------
void tTJSLocalNamespace::Commit(void)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_ns_Commit);
#endif
	// compute MaxCount
	tjs_int i;
	tjs_int count = 0;
	for(i=(tjs_int)(Levels.size()-1); i>=0; i--)
	{
		tTJSLocalSymbolList* list = Levels[i];
		count += list->GetCount();
	}
	if(MaxCount < count) MaxCount = count;
}
//---------------------------------------------------------------------------
tTJSLocalSymbolList * tTJSLocalNamespace::GetTopSymbolList()
{
	// returns top symbol list
	if(Levels.size() == 0) return NULL;
	return (tTJSLocalSymbolList *)(Levels[Levels.size()-1]);
}
//---------------------------------------------------------------------------
void tTJSLocalNamespace::Clear(void)
{
	// all clear
	while(Levels.size()) Pop();
}
//---------------------------------------------------------------------------

} // namespace TJS

