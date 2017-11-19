//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Intermediate Code Context
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <algorithm>

#include "tjsInterCodeGen.h"
#include "tjsScriptBlock.h"
#include "tjsGlobalStringMap.h"

#include "tjs.tab.hpp"
#include "tjsError.h"
#include "tjsUtils.h"
#include "tjsDebug.h"
#include "tjsConstArrayData.h"

#define LEX_POS (Block->GetLexicalAnalyzer() -> GetCurrentPosition())
#define NODE_POS (node?node->GetPosition():-1)


/*
	'intermediate code' means that it is not a final code, yes.
	I thought this should be converted to native machine code before execute,
	at early phase of developping TJS2.
	But TJS2 intermediate VM code has functions that are too abstract to
	be converted to native machine code, so I decided that I write a code
	which directly drives intermediate VM code.
*/


//---------------------------------------------------------------------------
namespace TJS  // following is in the namespace
{
//---------------------------------------------------------------------------


int __yyerror(char *, void*);

#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint time_yylex = 0;
#endif

//---------------------------------------------------------------------------
int yylex(YYSTYPE *yylex, void *pm)
{
	// yylex ( is called from bison parser, returns lexical analyzer's return value )
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_yylex);
#endif

	tjs_int n;
	tjs_int t;
	t = ((tTJSScriptBlock *)pm)->GetLexicalAnalyzer()->GetNext(n);
	yylex->num = n;
	return t;
}
//---------------------------------------------------------------------------
int _yyerror(const tjs_char * msg, void *pm, tjs_int pos)
{
	// handles errors that happen in the compilation

	tTJSScriptBlock *sb = (tTJSScriptBlock*)pm;

	// message conversion
	ttstr str;
	/*if(!TJS_strncmp(msg, TJS_W("parse error, expecting "), 23))
	{
		str = TJSExpected;
		if(!TJS_strncmp(msg+23, TJS_W("T_SYMBOL"), 8))
			str.Replace(TJS_W("%1"), ttstr(TJSSymbol), false);
		else
			str.Replace(TJS_W("%1"), msg+23, false);

	}
	else */if(!TJS_strncmp(msg, TJS_W("syntax error"), 11))
	{
		str = TJSSyntaxError;
		str.Replace(TJS_W("%1"), ttstr(msg), false);
	}
	else
	{
		str = msg;
	}

	tjs_int errpos =
		pos == -1 ? sb->GetLexicalAnalyzer()->GetCurrentPosition(): pos;

	if(sb->CompileErrorCount == 0)
	{
		sb->SetFirstError(str.c_str(), errpos);
	}

	sb->CompileErrorCount++;

	tjs_char buf[43];
	TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), TJS_W(" at line %d"), 1+sb->SrcPosToLine(errpos));
	str += buf;

	sb->GetTJS()->OutputToConsole(str.c_str());

	return 0;
}

//---------------------------------------------------------------------------
int __yyerror(char * msg, void * pm)
{
	// yyerror ( for bison )
	ttstr str(msg);
	return _yyerror(str.c_str(), pm);
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSExprNode -- expression node
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
tTJSExprNode::tTJSExprNode()
{
	Op = 0;
	Nodes = NULL;
	Val = NULL;
	Position = -1;
}
//---------------------------------------------------------------------------
void tTJSExprNode::Clear()
{
	if(Nodes) delete Nodes;
	if(Val) delete Val;
	Nodes = NULL;
	Val = NULL;
}
//---------------------------------------------------------------------------
void tTJSExprNode::Add(tTJSExprNode *n)
{
	if(!Nodes)
		Nodes = new std::vector<tTJSExprNode*>;
	Nodes->push_back(n);
}
//---------------------------------------------------------------------------
void tTJSExprNode::AddArrayElement(const tTJSVariant & val)
{
	static tTJSString ss_add(TJS_W("add"));
	tTJSVariant arg(val);
	tTJSVariant *args[1] = {&arg};
	Val->AsObjectClosureNoAddRef().FuncCall(0, ss_add.c_str(), ss_add.GetHint(),
		NULL, 1, args, NULL);
}
//---------------------------------------------------------------------------
void tTJSExprNode::AddDictionaryElement(const tTJSString & name, const tTJSVariant & val)
{
	tTJSString membername(name);
	Val->AsObjectClosureNoAddRef().PropSet(TJS_MEMBERENSURE, membername.c_str(),
		membername.GetHint(), &val, NULL);
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSInterCodeContext -- intermediate context
//---------------------------------------------------------------------------
static tjs_int TJSGetContextHashSize(tTJSContextType type)
{
	switch(type)
	{
	case ctTopLevel:		return 0;
	case ctFunction:		return 1;
	case ctExprFunction: 	return 1;
	case ctProperty:		return 1;
	case ctPropertySetter:	return 0;
	case ctPropertyGetter:	return 0;
	case ctClass:			return TJS_NAMESPACE_DEFAULT_HASH_BITS;
	case ctSuperClassGetter:return 0;
	default:				return TJS_NAMESPACE_DEFAULT_HASH_BITS;
	}
}
//---------------------------------------------------------------------------
// is bytecode export
bool tTJSInterCodeContext::IsBytecodeCompile = false;
//---------------------------------------------------------------------------
tTJSInterCodeContext::tTJSInterCodeContext(tTJSInterCodeContext *parent,
	const tjs_char *name, tTJSScriptBlock *block, tTJSContextType type) :
		inherited(TJSGetContextHashSize(type)), Properties(NULL)
{
	inherited::CallFinalize = false;
		// this notifies to the class ancestor - "tTJSCustomObject", not to
		// call "finalize" TJS method at the invalidation.

	Parent = parent;

	PropGetter = PropSetter = SuperClassGetter = NULL;

	CodeArea = NULL;
	CodeAreaCapa = 0;
	CodeAreaSize = 0;

	_DataArea = NULL;
	_DataAreaCapa = 0;
	_DataAreaSize = 0;
	DataArea = NULL;
	DataAreaSize = 0;

	FrameBase = 1;

	SuperClassExpr = NULL;

	MaxFrameCount = 0;
	MaxVariableCount = 0;

	FuncDeclArgCount = 0;
	FuncDeclUnnamedArgArrayBase = 0;
	FuncDeclCollapseBase = -1;

	FunctionRegisterCodePoint = 0;


	PrevSourcePos = -1;
	SourcePosArraySorted = false;
	SourcePosArray = NULL;
	SourcePosArrayCapa = 0;
	SourcePosArraySize = 0;

#ifdef ENABLE_DEBUGGER
	DebuggerRegisterArea = NULL;
	if( Parent ) Parent->AddRef();
#endif	// ENABLE_DEBUGGER

	if(name)
	{
		Name = new tjs_char[TJS_strlen(name)+1];
		TJS_strcpy(Name, name);
	}
	else
	{
		Name = NULL;
	}

	try
	{
		AsGlobalContextMode = false;

		ContextType = type;

		switch(ContextType) // decide variable reservation count with context type
		{
			case ctTopLevel:		VariableReserveCount = 2; break;
			case ctFunction:		VariableReserveCount = 2; break;
			case ctExprFunction: 	VariableReserveCount = 2; break;
			case ctProperty:		VariableReserveCount = 0; break;
			case ctPropertySetter:	VariableReserveCount = 2; break;
			case ctPropertyGetter:	VariableReserveCount = 2; break;
			case ctClass:			VariableReserveCount = 2; break;
			case ctSuperClassGetter:VariableReserveCount = 2; break;
		}


		Block = block;
		block->Add(this);
		TJSVariantArrayStack = block->GetTJS()->GetVariantArrayStack();
		if(ContextType != ctTopLevel) Block->AddRef();
			// owner ScriptBlock hooks global object, so to avoid mutual reference lock.


		if(ContextType == ctClass)
		{
			// add class information to the class instance information
			if(MaxFrameCount < 1) MaxFrameCount = 1;

			tjs_int dp = PutData(tTJSVariant(Name));
			// const %1, name
			// addci %-1, %1
			// cl %1
			PutCode(VM_CONST, LEX_POS);
			PutCode(TJS_TO_VM_REG_ADDR(1));
			PutCode(TJS_TO_VM_REG_ADDR(dp));
			PutCode(VM_ADDCI);
			PutCode(TJS_TO_VM_REG_ADDR(-1));
			PutCode(TJS_TO_VM_REG_ADDR(1));
			PutCode(VM_CL);
			PutCode(TJS_TO_VM_REG_ADDR(1));

			// update FunctionRegisterCodePoint
			FunctionRegisterCodePoint = CodeAreaSize; // update FunctionRegisterCodePoint
		}
	}
	catch(...)
	{
		delete [] Name;
		throw;
	}
#ifdef ENABLE_DEBUGGER
	// 古いローカル変数は削除してしまう
	TJSDebuggerClearLocalVariable( GetClassName().c_str(), GetName(), Block->GetName(), FunctionRegisterCodePoint );
#endif	// ENABLE_DEBUGGER
}

//---------------------------------------------------------------------------
// for Byte code
tTJSInterCodeContext::tTJSInterCodeContext( tTJSScriptBlock *block, const tjs_char *name, tTJSContextType type,
	tjs_int32* code, tjs_int codeSize, tTJSVariant* data, tjs_int dataSize,
	tjs_int varcount, tjs_int verrescount, tjs_int maxframe, tjs_int argcount, tjs_int arraybase, tjs_int colbase, bool srcsorted,
	tSourcePos* srcPos, tjs_int srcPosSize, std::vector<tjs_int>& superpointer ) :
	inherited(TJSGetContextHashSize(type)), Properties(NULL)
{
	inherited::CallFinalize = false;
	Parent = NULL;
	PropGetter = PropSetter = SuperClassGetter = NULL;

	CodeArea = code;
	CodeAreaCapa = codeSize;
	CodeAreaSize = codeSize;

	_DataArea = NULL;
	_DataAreaCapa = 0;
	_DataAreaSize = 0;
	DataArea = data;
	DataAreaSize = dataSize;

	// copy
	size_t size = superpointer.size();
	SuperClassGetterPointer.reserve(size);
	for( size_t i = 0; i < size; i++ ) {
		SuperClassGetterPointer.push_back( superpointer[i] );
	}

	FrameBase = 1; // for code generate
	SuperClassExpr = NULL; // for code generate

	MaxFrameCount = maxframe;
	MaxVariableCount = varcount;
	VariableReserveCount = verrescount; //

	FuncDeclArgCount = argcount;
	FuncDeclUnnamedArgArrayBase = arraybase;
	FuncDeclCollapseBase = colbase;

	FunctionRegisterCodePoint = 0;


	PrevSourcePos = -1;
	SourcePosArraySorted = true;
	SourcePosArray = srcPos;
	SourcePosArrayCapa = srcPosSize;
	SourcePosArraySize = srcPosSize;

#ifdef ENABLE_DEBUGGER
	DebuggerRegisterArea = NULL;
#endif	// ENABLE_DEBUGGER

	if( name ) {
		Name = new tjs_char[TJS_strlen(name)+1];
		TJS_strcpy(Name, name);
	} else {
		Name = NULL;
	}

	try {
		AsGlobalContextMode = false;
		ContextType = type;
		Block = block;
		TJSVariantArrayStack = block->GetTJS()->GetVariantArrayStack();
	} catch(...) {
		delete [] Name;
		throw;
	}
}
//---------------------------------------------------------------------------
tTJSInterCodeContext::~tTJSInterCodeContext()
{
	if(Name) delete [] Name;
	Name = NULL;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::Finalize(void)
{
	if(PropSetter) PropSetter->Release(), PropSetter = NULL;
	if(PropGetter) PropGetter->Release(), PropGetter = NULL;
	if(SuperClassGetter) SuperClassGetter->Release(), SuperClassGetter = NULL;

	if(CodeArea) TJS_free(CodeArea), CodeArea = NULL;
	if(_DataArea)
	{
		for(tjs_int i=0; i<_DataAreaSize; i++) delete _DataArea[i];
		TJS_free(_DataArea);
		_DataArea = NULL;
	}
	if(DataArea)
	{
		delete [] DataArea;
		DataArea = NULL;
	}

	Block->Remove(this);

	if(ContextType!=ctTopLevel && Block) Block->Release();

	Namespace.Clear();

	ClearNodesToDelete();

	if(SourcePosArray) TJS_free(SourcePosArray), SourcePosArray = NULL;
	
#ifdef ENABLE_DEBUGGER
	if( Parent ) {
		Parent->Release();
		Parent = NULL;
	}
#endif	// ENABLE_DEBUGGER
	inherited::Finalize();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ClearNodesToDelete(void)
{
	if(NodeToDeleteVector.size())
	{
		for(tjs_int i=(tjs_int)(NodeToDeleteVector.size()-1); i>=0; i--)
		{
			delete NodeToDeleteVector[i];
		}
	}

	NodeToDeleteVector.clear();
}
//---------------------------------------------------------------------------
const tjs_char* tTJSInterCodeContext::GetContextTypeName() const
{
	switch(ContextType)
	{
	case ctTopLevel:		return TJS_W("top level script");
	case ctFunction:		return TJS_W("function");
	case ctExprFunction:	return TJS_W("function expression");
	case ctProperty:		return TJS_W("property");
	case ctPropertySetter:	return TJS_W("property setter");
	case ctPropertyGetter:	return TJS_W("property getter");
	case ctClass:			return TJS_W("class");
	case ctSuperClassGetter:return TJS_W("super class getter proxy");
	default:				return TJS_W("unknown");
	}
}
//---------------------------------------------------------------------------
ttstr tTJSInterCodeContext::GetShortDescription() const
{
	ttstr ret(TJS_W("(") + ttstr(GetContextTypeName()) + TJS_W(")"));

	const tjs_char *name;
	if(ContextType == ctPropertySetter || ContextType == ctPropertyGetter)
	{
		if(Parent)
			name = Parent->Name;
		else
			name = NULL;
	}
	else
	{
		name = Name;
	}

	if(name) ret += TJS_W(" ") + ttstr(name);

	return ret;
}
//---------------------------------------------------------------------------
ttstr tTJSInterCodeContext::GetShortDescriptionWithClassName() const
{
	ttstr ret(TJS_W("(") + ttstr(GetContextTypeName()) + TJS_W(")"));

	tTJSInterCodeContext * parent;

	const tjs_char *classname;
	const tjs_char *name;

	if(ContextType == ctPropertySetter || ContextType == ctPropertyGetter)
		parent = Parent ? Parent->Parent : NULL;
	else
		parent = Parent;

	if(parent)
		classname = parent->Name;
	else
		classname = NULL;

	if(ContextType == ctPropertySetter || ContextType == ctPropertyGetter)
	{
		if(Parent)
			name = Parent->Name;
		else
			name = NULL;
	}
	else
	{
		name = Name;
	}

	if(name)
	{
		ret += TJS_W(" ");
		if(classname) ret += ttstr(classname) + TJS_W(".");
		ret += ttstr(name);
	}

	return ret;
}
#ifdef ENABLE_DEBUGGER
//---------------------------------------------------------------------------
ttstr tTJSInterCodeContext::GetClassName() const
{
	ttstr ret;

	tTJSInterCodeContext * parent;

	const tjs_char *classname;

	if(ContextType == ctPropertySetter || ContextType == ctPropertyGetter)
		parent = Parent ? Parent->Parent : NULL;
	else
		parent = Parent;

	if(parent)
		classname = parent->Name;
	else
		classname = NULL;

	if( classname ) ret = ttstr(classname);

	return ret;
}
//---------------------------------------------------------------------------
ttstr tTJSInterCodeContext::GetSelfClassName() const
{
	ttstr ret;

	const tTJSInterCodeContext* parent;

	const tjs_char *classname;

	if(ContextType == ctPropertySetter || ContextType == ctPropertyGetter)
		parent = Parent ? Parent->Parent : NULL;
	else if( ContextType == ctClass )
		parent = this;
	else
		parent = Parent;

	if(parent)
		classname = parent->Name;
	else
		classname = NULL;

	if( classname ) ret = ttstr(classname);

	return ret;
}
#endif	// ENABLE_DEBUGGER
//---------------------------------------------------------------------------
void tTJSInterCodeContext::OutputWarning(const tjs_char *msg, tjs_int pos)
{
	ttstr str(TJSWarning);

	str += msg;

	tjs_int errpos =
		pos == -1 ? Block->GetLexicalAnalyzer()->GetCurrentPosition(): pos;

	str += TJS_W(" at ");
	str += Block->GetName();

	tjs_char buf[43];
	TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), TJS_W(" line %d"), 1+Block->SrcPosToLine(errpos));
	str += buf;

	Block->GetTJS()->OutputToConsole(str.c_str());
}
//---------------------------------------------------------------------------

#define TJS_INC_SIZE 256

#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint time_PutCode = 0;
#endif

tjs_int tTJSInterCodeContext::PutCode(tjs_int32 num, tjs_int32 pos)
{
	// puts code
	// num = operation code
	// pos = position in the script
	// this returns code address of newly put code
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_PutCode);
#endif

	if(CodeAreaSize >= CodeAreaCapa)
	{
		// must inflate the code area
		CodeArea = (tjs_int32*)TJS_realloc(CodeArea,
			sizeof(tjs_int32)*(CodeAreaCapa + TJS_INC_SIZE));
		if(!CodeArea) TJS_eTJSScriptError(TJSInsufficientMem, Block, pos);
		CodeAreaCapa += TJS_INC_SIZE;
	}

	if(pos!=-1)
	{
		if(PrevSourcePos != pos)
		{
			PrevSourcePos = pos;
			SourcePosArraySorted = false;
			if(!SourcePosArray)
			{
				SourcePosArray = (tSourcePos*)TJS_malloc(TJS_INC_SIZE *
					sizeof(tSourcePos));
				if(!SourcePosArray) _yyerror(TJSInsufficientMem, Block);
				SourcePosArrayCapa = TJS_INC_SIZE;
				SourcePosArraySize = 0;
			}
			if(SourcePosArraySize >= SourcePosArrayCapa)
			{
				SourcePosArray = (tSourcePos*)TJS_realloc(SourcePosArray,
					(SourcePosArrayCapa + TJS_INC_SIZE) * sizeof(tSourcePos));
				if(!SourcePosArray) TJS_eTJSScriptError(TJSInsufficientMem, Block, pos);
				SourcePosArrayCapa += TJS_INC_SIZE;
			}
			SourcePosArray[SourcePosArraySize].CodePos = CodeAreaSize;
			SourcePosArray[SourcePosArraySize].SourcePos = pos;
			SourcePosArraySize++;
		}
	}

	CodeArea[CodeAreaSize] = num;

	return CodeAreaSize++;
}
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint time_PutData = 0;
#endif

tjs_int tTJSInterCodeContext::PutData(const tTJSVariant &val)
{
	// puts data
	// val = data
	// return the data address
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_PutData);
#endif

	if(_DataAreaSize >= _DataAreaCapa)
	{
		// inflation of data area
		_DataArea = (tTJSVariant**)TJS_realloc(_DataArea,
			sizeof(tTJSVariant*)*(_DataAreaCapa + TJS_INC_SIZE));
		if(!_DataArea) TJS_eTJSScriptError(TJSInsufficientMem, Block, LEX_POS);
		_DataAreaCapa += TJS_INC_SIZE;
	}

	// search same data in the area
	if(_DataAreaSize)
	{
		tTJSVariant **ptr = _DataArea + _DataAreaSize-1;
		tjs_int count = 0;
		while(count < 20 // is waste of time if it exceeds 20 limit?
			)
		{
			if((*ptr)->DiscernCompareStrictReal(val))
			{
				return (tjs_int)(ptr - _DataArea); // re-use this
			}
			count ++;
			if(ptr == _DataArea) break;
			ptr --;
		}
	}

	tTJSVariant *v;
	if(val.Type() == tvtString)
	{
		// check whether the string can be shared
		v = new tTJSVariant(TJSMapGlobalStringMap(val));
	}
	else
	{
		v = new tTJSVariant(val);
	}
	_DataArea[_DataAreaSize] = v;

	return _DataAreaSize++;
}
//---------------------------------------------------------------------------
int TJS_USERENTRY tTJSInterCodeContext::tSourcePos::
	SortFunction(const void *a, const void *b)
{
	const tSourcePos *aa = (const tSourcePos*)a;
	const tSourcePos *bb = (const tSourcePos*)b;
	return aa->CodePos - bb->CodePos;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::SortSourcePos()
{
	if(!SourcePosArraySorted)
	{
		qsort(SourcePosArray, SourcePosArraySize, sizeof(tSourcePos),
			tSourcePos::SortFunction);
		SourcePosArraySorted = true;
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::FixCode(void)
{
	// code re-positioning and patch processing
	// TODO: tTJSInterCodeContext::FixCode fasten the algorithm

	// create 'regmember' instruction to register class members to 
	// newly created object
	if(ContextType == ctClass)
	{
		// generate a code
		tjs_int32 * code = new tjs_int32[1];
		code[0] = VM_REGMEMBER;

		// make a patch information
		// use FunctionRegisterCodePoint for insertion point
		FixList.push_back(tFixData(FunctionRegisterCodePoint, 0, 1, code, true));
	}

	// process funtion reservation to enable backward reference of
	// global/method functions
	if(NonLocalFunctionDeclVector.size() >= 1)
	{
		if(MaxFrameCount < 1) MaxFrameCount = 1;

		std::vector<tNonLocalFunctionDecl>::iterator func;

		// make function registration code to objthis

		// compute codesize
		tjs_int codesize = 2;
		for(func = NonLocalFunctionDeclVector.begin();
			func!= NonLocalFunctionDeclVector.end();
			func++)
		{
			if(func->ChangeThis) codesize += 10; else codesize += 7;
		}

		tjs_int32 *code = new tjs_int32[codesize];

		// generate code
		tjs_int32 *codep = code;
		for(func = NonLocalFunctionDeclVector.begin();
			func!= NonLocalFunctionDeclVector.end();
			func++)
		{
			// const %1, #funcdata
			*(codep++) = VM_CONST;
			*(codep++) = TJS_TO_VM_REG_ADDR(1);
			*(codep++) = TJS_TO_VM_REG_ADDR(func->DataPos);

			// chgthis %1, %-1
			if(func->ChangeThis)
			{
				*(codep++) = VM_CHGTHIS;
				*(codep++) = TJS_TO_VM_REG_ADDR(1);
				*(codep++) = TJS_TO_VM_REG_ADDR(-1);
			}

			// spds %-1.#funcname, %1
			*(codep++) = VM_SPDS;
			*(codep++) = TJS_TO_VM_REG_ADDR(-1); // -1 =  objthis
			*(codep++) = TJS_TO_VM_REG_ADDR(func->NameDataPos);
			*(codep++) = TJS_TO_VM_REG_ADDR(1);
		}

		// cl %1
		*(codep++) = VM_CL;
		*(codep++) = TJS_TO_VM_REG_ADDR(1);

		// make a patch information
		FixList.push_back(tFixData(FunctionRegisterCodePoint, 0, codesize, code, true));

		NonLocalFunctionDeclVector.clear();
	}

	// sort SourcePosVector
	SortSourcePos();

	// re-position patch
	std::list<tFixData>::iterator fix;

	for(fix = FixList.begin(); fix!=FixList.end(); fix++)
	{
		std::list<tjs_int>::iterator jmp;
		for(jmp = JumpList.begin(); jmp!=JumpList.end(); jmp++)
		{
			tjs_int jmptarget = CodeArea[*jmp + 1] + *jmp;
			if(*jmp >= fix->StartIP && *jmp < fix->Size + fix->StartIP)
			{
				// jmp is in the re-positioning target -> delete
				jmp = JumpList.erase(jmp);
			}
			else if((fix->BeforeInsertion?
				(jmptarget < fix->StartIP):(jmptarget <= fix->StartIP)
				&& *jmp > fix->StartIP + fix->Size) ||
				(*jmp < fix->StartIP && jmptarget >= fix->StartIP + fix->Size))
			{
				// jmp and its jumping-target is in the re-positioning target
				CodeArea[*jmp + 1] += fix->NewSize - fix->Size;
			}

			if(*jmp >= fix->StartIP + fix->Size)
			{
				// fix up jmp
				*jmp += fix->NewSize - fix->Size;
			}
		}

		// move the code
		if(fix->NewSize > fix->Size)
		{
			// when code inflates on fixing
			CodeArea = (tjs_int32*)TJS_realloc(CodeArea,
				sizeof(tjs_int32)*(CodeAreaSize + (fix->NewSize - fix->Size)));
			if(!CodeArea) TJS_eTJSScriptError(TJSInsufficientMem, Block, 0);
		}

		if(CodeAreaSize - (fix->StartIP + fix->Size) > 0)
		{
			// move the existing code
			memmove(CodeArea + fix->StartIP + fix->NewSize,
				CodeArea + fix->StartIP + fix->Size,
				sizeof(tjs_int32)* (CodeAreaSize - (fix->StartIP + fix->Size)));

			// move sourcepos
			for(tjs_int i = 0; i < SourcePosArraySize; i++)
			{
				if(SourcePosArray[i].CodePos >= fix->StartIP + fix->Size)
					SourcePosArray[i].CodePos += fix->NewSize - fix->Size;
			}
		}

		if(fix->NewSize && fix->Code)
		{
			// copy the new code
			memcpy(CodeArea + fix->StartIP, fix->Code, sizeof(tjs_int32)*fix->NewSize);
		}

		CodeAreaSize += fix->NewSize - fix->Size;
	}

	// eliminate redundant jump codes
	for(std::list<tjs_int>::iterator jmp = JumpList.begin();
		jmp!=JumpList.end(); jmp++)
	{
		tjs_int32 jumptarget = CodeArea[*jmp + 1] + *jmp;
		tjs_int32 jumpcode = CodeArea[*jmp];
		tjs_int addr = *jmp;
		addr += CodeArea[addr + 1];
		for(;;)
		{
			if(CodeArea[addr] == VM_JMP ||
				(CodeArea[addr] == jumpcode && (jumpcode == VM_JF || jumpcode == VM_JNF)))
			{
				// simple jump code or
				// JF after JF or JNF after JNF
				jumptarget = CodeArea[addr + 1] + addr; // skip jump after jump
				if(CodeArea[addr + 1] != 0)
					addr += CodeArea[addr + 1];
				else
					break; // must be an error
			}
			else if((CodeArea[addr] == VM_JF && jumpcode == VM_JNF) ||
				(CodeArea[addr] == VM_JNF && jumpcode == VM_JF))
			{
				// JF after JNF or JNF after JF
				jumptarget = addr + 2;
					// jump code after jump will not jump
				addr += 2;
			}
			else
			{
				// other codes
				break;
			}
		}
		CodeArea[*jmp + 1] = jumptarget - *jmp;
	}

	// convert jump addresses to VM address
	for(std::list<tjs_int>::iterator jmp = JumpList.begin();
		jmp!=JumpList.end(); jmp++)
	{
		CodeArea[*jmp + 1] = TJS_TO_VM_CODE_ADDR(CodeArea[*jmp + 1]);
	}

	JumpList.clear();
	FixList.clear();

}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::RegisterFunction()
{
	// registration of function to the parent's context

	if(!Parent) return;

	if(ContextType == ctPropertySetter)
	{
		Parent->PropSetter = this; AddRef();
		return;
	}
	if(ContextType == ctPropertyGetter)
	{
		Parent->PropGetter = this; AddRef();
		return;
	}

	if(ContextType == ctSuperClassGetter)
	{
		return; // these are already registered to parent context
	}


	if( ContextType != ctFunction &&  // ctExprFunction is not concerned here
		ContextType != ctProperty &&
		ContextType != ctClass)
	{
		return;
	}


	tjs_int data = -1;

	if(Parent->ContextType == ctTopLevel)
	{
		tTJSVariant val;
		val = this;
		data = Parent->PutData(val);
		val = Name;
		tjs_int name = Parent->PutData(val);
		bool changethis = ContextType == ctFunction ||
							ContextType == ctProperty;
		Parent->NonLocalFunctionDeclVector.push_back(
			tNonLocalFunctionDecl(data, name, changethis));
	}

	if(ContextType == ctFunction && Parent->ContextType == ctFunction)
	{
		// local functions
		// adds the function as a parent's local variable
		if(data == -1)
		{
			tTJSVariant val;
			val = this;
			data = Parent->PutData(val);
		}

		Parent->InitLocalFunction(Name, data);

	}

	if( Parent->ContextType == ctFunction ||
		Parent->ContextType == ctClass)
	{
		if( IsBytecodeCompile ) { // for bytecode export
			if( Properties == NULL ) Properties = new std::vector<tProperty*>();
			Properties->push_back( new tProperty( Name, this ) );
		}

		// register members to the parent object
		tTJSVariant val = this;
		Parent->PropSet(TJS_MEMBERENSURE|TJS_IGNOREPROP, Name, NULL, &val, Parent);
	}

}
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint time_Commit = 0;
#endif

void tTJSInterCodeContext::Commit()
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_Commit);
#endif

	// some context-related processing at final, and commits it
	if(ContextType == ctClass)
	{
		// clean up super class proxy
		if(SuperClassGetter) SuperClassGetter->Commit();
	}

	if(ContextType != ctProperty && ContextType != ctSuperClassGetter)
	{
		PutCode(VM_SRV, LEX_POS);
		PutCode(TJS_TO_VM_REG_ADDR(0));
		PutCode(VM_RET);
	}

	RegisterFunction();

	if(ContextType != ctProperty && ContextType != ctSuperClassGetter) FixCode();

	if(!DataArea)
	{
		DataArea = new tTJSVariant[_DataAreaSize];
		DataAreaSize = _DataAreaSize;

		for(tjs_int i = 0; i<_DataAreaSize; i++)
		{
			DataArea[i].CopyRef( *_DataArea[i]);
		}

		if(_DataArea)
		{
			for(tjs_int i = 0; i<_DataAreaSize; i++) delete _DataArea[i];
			TJS_free(_DataArea);
			_DataArea = NULL;
		}
	}

	if(ContextType == ctSuperClassGetter)
		MaxVariableCount = 2; // always 2
	else
		MaxVariableCount = Namespace.GetMaxCount();

	SuperClassExpr = NULL;

	ClearNodesToDelete();

	// compact SourcePosArray to just size
	if(SourcePosArraySize && SourcePosArray)
	{
		SourcePosArray = (tSourcePos*)TJS_realloc(SourcePosArray,
			SourcePosArraySize * sizeof(tSourcePos));
		if(!SourcePosArray) TJS_eTJSScriptError(TJSInsufficientMem, Block, 0);
		SourcePosArrayCapa = SourcePosArraySize;
	}

	// compact CodeArea to just size
	if(CodeAreaSize && CodeArea)
	{
		// must inflate the code area
		CodeArea = (tjs_int32*)TJS_realloc(CodeArea,
			sizeof(tjs_int32)*CodeAreaSize);
		if(!CodeArea) TJS_eTJSScriptError(TJSInsufficientMem, Block, 0);
		CodeAreaCapa = CodeAreaSize;
	}


	// set object type info for debugging
	if(TJSObjectHashMapEnabled())
		TJSObjectHashSetType(this, GetShortDescriptionWithClassName());


	// we do thus nasty thing because the std::vector does not free its storage
	// even we call 'clear' method...
#define RE_CREATE(place, type, classname) (&place)->type::~classname(); \
	new (&place) type ();

	RE_CREATE(NodeToDeleteVector, std::vector<tTJSExprNode *>, vector);
	RE_CREATE(CurrentNodeVector, std::vector<tTJSExprNode *>, vector);
	RE_CREATE(FuncArgStack, std::stack<tFuncArg>, stack);
	RE_CREATE(ArrayArgStack, std::stack<tArrayArg>, stack);
	RE_CREATE(NestVector, std::vector<tNestData>, vector);
	RE_CREATE(JumpList, std::list<tjs_int>, list);
	RE_CREATE(FixList, std::list<tFixData>, list);
	RE_CREATE(NonLocalFunctionDeclVector, std::vector<tNonLocalFunctionDecl>, vector);
}
//---------------------------------------------------------------------------
tjs_int tTJSInterCodeContext::CodePosToSrcPos(tjs_int codepos) const
{
	// converts from
	// CodeArea oriented position to source oriented position
	if(!SourcePosArray) return 0;

	const_cast<tTJSInterCodeContext*>(this)->SortSourcePos();

	tjs_uint s = 0;
	tjs_uint e = SourcePosArraySize;
	if(e==0) return 0;
	while(true)
	{
		if(e-s <= 1) return SourcePosArray[s].SourcePos;
		tjs_uint m = s + (e-s)/2;
		if(SourcePosArray[m].CodePos > codepos)
			e = m;
		else
			s = m;
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSInterCodeContext::FindSrcLineStartCodePos(tjs_int codepos) const
{
	// find code address which is the first instruction of the source line
	if(!SourcePosArray) return 0;

	tjs_int srcpos = CodePosToSrcPos(codepos);
	tjs_int line = Block->SrcPosToLine(srcpos);
	srcpos = Block->LineToSrcPos(line);

	tjs_int codeposmin = -1;
	for(tjs_int i = 0; i < SourcePosArraySize; i++)
	{
		if(SourcePosArray[i].SourcePos >= srcpos)
		{
			if(codeposmin == -1 || SourcePosArray[i].CodePos< codeposmin)
				codeposmin = SourcePosArray[i].CodePos;
		}
	}
	if(codeposmin < 0) codeposmin = 0;
	return codeposmin;
}
//---------------------------------------------------------------------------
ttstr tTJSInterCodeContext::GetPositionDescriptionString(tjs_int codepos) const
{
	return Block->GetLineDescriptionString(CodePosToSrcPos(codepos)) +
		TJS_W("[") + GetShortDescription() + TJS_W("]");
}
//---------------------------------------------------------------------------
static bool inline TJSIsModifySubType(tTJSSubType type)
	{ return type != stNone; }
static bool inline TJSIsCondFlagRetValue(tjs_int r)
	{ return r == TJS_GNC_CFLAG || r == TJS_GNC_CFLAG_I; }
static bool inline TJSIsFrame(tjs_int r)
	{ return r > 0; }
#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint time_this_proxy = 0;
#endif

tjs_int tTJSInterCodeContext::GenNodeCode(tjs_int & frame, tTJSExprNode *node,
	tjs_uint32 restype, tjs_int reqresaddr, const tSubParam  & param)
{
	// code generation of a given node

	// frame = register stack frame
	// node = target node
	// restype = required result type
	// reqresaddr = variable address which should receives the result
	//              ( currently not used )
	// param = additional parameters
	// returns: a register address that contains the result ( TJS_GNC_CFLAG
	//          for condition flags )

	tjs_int resaddr;

	tjs_int node_pos = NODE_POS;

	switch(node->GetOpecode())
	{
	case T_CONSTVAL: // constant value
	  {
		// a code that refers the constant value
		if(TJSIsModifySubType(param.SubType)) _yyerror(TJSCannotModifyLHS, Block);
		if(!(restype & TJS_RT_NEEDED)) return 0; // why here is called without a result necessity? ;-)
		tjs_int dp = PutData(node->GetValue());
		PutCode(VM_CONST, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
		return frame++;
	  }

	case T_IF: // 'if' operator
	  {
		// "if" operator
		// evaluate right node. then evaluate left node if the right results true.
		if(restype & TJS_RT_NEEDED) _yyerror(TJSCannotGetResult, Block);
//		if(TJSIsModifySubType(param.SubType)) _yyerror(TJSCannotModifyLHS, Block);
		tjs_int resaddr = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED|TJS_RT_CFLAG,
			0, tSubParam());
		bool inv = false;
		if(!TJSIsCondFlagRetValue(resaddr))
		{
			PutCode(VM_TT, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
		}
		else
		{
			if(resaddr == TJS_GNC_CFLAG_I) inv = true;
		}
		tjs_int addr = CodeAreaSize;
		AddJumpList();
		PutCode(inv?VM_JF:VM_JNF, node_pos);
		PutCode(0, node_pos); // *
		_GenNodeCode(frame, (*node)[0], 0, 0, param);
		CodeArea[addr + 1] = CodeAreaSize - addr; //  patch "*"
		return 0;
	  }

	case T_INCONTEXTOF: // 'incontextof' operator
	  {
		// "incontextof" operator
		// a special operator that changes objeect closure's context
		if(!(restype & TJS_RT_NEEDED)) return 0;
		tjs_int resaddr1, resaddr2;
		resaddr1 = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, param);
		resaddr2 = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, tSubParam());
		if(!TJSIsFrame(resaddr1))
		{
			PutCode(VM_CP, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
			resaddr1 = frame;
			frame++;
		}
		PutCode(VM_CHGTHIS, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr2), node_pos);

		return resaddr1;
	  }

	case T_COMMA: // ',' operator
		// comma operator
		_GenNodeCode(frame, (*node)[0], 0, 0, tSubParam());
		return _GenNodeCode(frame, (*node)[1], restype, reqresaddr, param);


	case T_SWAP: // '<->' operator
	  {
		// swap operator
		if(restype & TJS_RT_NEEDED) _yyerror(TJSCannotGetResult, Block);
		if(param.SubType) _yyerror(TJSCannotModifyLHS, Block);

		tjs_int resaddr1 = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0,
			tSubParam());

		if(!TJSIsFrame(resaddr1))
		{
			PutCode(VM_CP, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
			resaddr1 = frame;
			frame++;
		}

		tjs_int resaddr2 = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0,
			tSubParam());

		// create substitutions
		tSubParam param2;
		param2.SubType = stEqual;
		param2.SubAddress = resaddr2;
		_GenNodeCode(frame, (*node)[0], 0, 0, param2);

		param2.SubType = stEqual;
		param2.SubAddress = resaddr1;
		_GenNodeCode(frame, (*node)[1], 0, 0, param2);

		return 0;
	  }

	case T_EQUAL: // '=' operator
	  {
		// simple substitution
		if(param.SubType) _yyerror(TJSCannotModifyLHS, Block);

		if(restype & TJS_RT_CFLAG)
		{
			// '=' operator in boolean context
			OutputWarning(TJSSubstitutionInBooleanContext, node_pos);
		}

		resaddr = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, param);

		tSubParam param2;
		param2.SubType = stEqual;
		param2.SubAddress = resaddr;
		_GenNodeCode(frame, (*node)[0], 0, 0, param2);
		return resaddr;
	  }

	case T_AMPERSANDEQUAL:		// '&=' operator
	case T_VERTLINEEQUAL:		// '|=' operator
	case T_CHEVRONEQUAL:		// '^=' operator
	case T_MINUSEQUAL:			// ^-=' operator
	case T_PLUSEQUAL:			// '+=' operator
	case T_PERCENTEQUAL:		// '%=' operator
	case T_SLASHEQUAL:			// '/=' operator
	case T_BACKSLASHEQUAL:		// '\=' operator
	case T_ASTERISKEQUAL:		// '*=' operator
	case T_LOGICALOREQUAL:		// '||=' operator
	case T_LOGICALANDEQUAL:		// '&&=' operator
	case T_RARITHSHIFTEQUAL:	// '>>=' operator
	case T_LARITHSHIFTEQUAL:	// '<<=' operator
	case T_RBITSHIFTEQUAL:		// '>>>=' operator
	  {
		// operation and substitution operators like "&="
		if(param.SubType) _yyerror(TJSCannotModifyLHS, Block);
		resaddr = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, tSubParam());

		tSubParam param2;
		switch(node->GetOpecode()) // this may be sucking...
		{
		case T_AMPERSANDEQUAL:		param2.SubType = stBitAND;		break;
		case T_VERTLINEEQUAL:		param2.SubType = stBitOR;		break;
		case T_CHEVRONEQUAL:		param2.SubType = stBitXOR;		break;
		case T_MINUSEQUAL:			param2.SubType = stSub;			break;
		case T_PLUSEQUAL:			param2.SubType = stAdd;			break;
		case T_PERCENTEQUAL:		param2.SubType = stMod;			break;
		case T_SLASHEQUAL:			param2.SubType = stDiv;			break;
		case T_BACKSLASHEQUAL:		param2.SubType = stIDiv;		break;
		case T_ASTERISKEQUAL:		param2.SubType = stMul;			break;
		case T_LOGICALOREQUAL:		param2.SubType = stLogOR;		break;
		case T_LOGICALANDEQUAL:		param2.SubType = stLogAND;		break;
		case T_RARITHSHIFTEQUAL:	param2.SubType = stSAR;			break;
		case T_LARITHSHIFTEQUAL:	param2.SubType = stSAL;			break;
		case T_RBITSHIFTEQUAL:		param2.SubType = stSR;			break;
		}
		param2.SubAddress = resaddr;
		return _GenNodeCode(frame, (*node)[0], restype, reqresaddr, param2);
	  }

	case T_QUESTION: // '?' ':' operator
	  {
		// three-term operator ( ?  :  )
		tjs_int resaddr1, resaddr2;
		int frame1, frame2;
		resaddr = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED|TJS_RT_CFLAG, 0, tSubParam());
		bool inv = false;
		if(!TJSIsCondFlagRetValue(resaddr))
		{
			PutCode(VM_TT, node_pos);    // tt resaddr
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
		}
		else
		{
			if(resaddr == TJS_GNC_CFLAG_I) inv = true;
		}

		tjs_int cur_frame = frame;
		tjs_int addr1 = CodeAreaSize;
		AddJumpList();
		PutCode(inv?VM_JF:VM_JNF, node_pos);
		PutCode(0, node_pos); // patch

		resaddr1 = _GenNodeCode(frame, (*node)[1], restype, reqresaddr, param);

		if(restype & TJS_RT_CFLAG)
		{
			// condition flag required
			if(!TJSIsCondFlagRetValue(resaddr1))
			{
				PutCode(VM_TT, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr1));
			}
			else
			{
				if(resaddr1 == TJS_GNC_CFLAG_I)
					PutCode(VM_NF, node_pos); // invert flag
			}
		}
		else
		{
			if((restype & TJS_RT_NEEDED) &&
					!TJSIsCondFlagRetValue(resaddr1) && !TJSIsFrame(resaddr1))
			{
				PutCode(VM_CP, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
				resaddr1 = frame;
				frame++;
			}
		}
		frame1 = frame;

		tjs_int addr2 = CodeAreaSize;
		AddJumpList();
		PutCode(VM_JMP, node_pos);
		PutCode(0, node_pos); // patch
		CodeArea[addr1+1] = CodeAreaSize - addr1;
		frame = cur_frame;
		resaddr2 = _GenNodeCode(frame, (*node)[2], restype, reqresaddr, param);

		if(restype & TJS_RT_CFLAG)
		{
			// condition flag required
			if(!TJSIsCondFlagRetValue(resaddr2))
			{
				PutCode(VM_TT, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr2));
			}
			else
			{
				if(resaddr2 == TJS_GNC_CFLAG_I)
					PutCode(VM_NF, node_pos); // invert flag
			}
		}
		else
		{
			if((restype & TJS_RT_NEEDED) &&
					!TJSIsCondFlagRetValue(resaddr1) && resaddr1 != resaddr2)
			{
				PutCode(VM_CP, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr2), node_pos);
				frame++;
			}
		}
		frame2 = frame;

	  	CodeArea[addr2+1] = CodeAreaSize - addr2;
		frame = frame2 < frame1 ? frame1 : frame2;
		return (restype & TJS_RT_CFLAG)?TJS_GNC_CFLAG: resaddr1;
	  }

	case T_LOGICALOR: // '||' operator
	case T_LOGICALAND: // '&&' operator
	  {
		// "logical or" and "logical and"
		// these process with the "shortcut" :
		// OR  : does not evaluate right when left results true
		// AND : does not evaluate right when left results false
		if(param.SubType) _yyerror(TJSCannotModifyLHS, Block);
		tjs_int resaddr1, resaddr2;
		resaddr1 = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED|TJS_RT_CFLAG,
			0, tSubParam());
		bool inv = false;
		if(!TJSIsCondFlagRetValue(resaddr1))
		{
			PutCode(VM_TT, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
		}
		if(resaddr1 == TJS_GNC_CFLAG_I) inv = true;
		tjs_int addr1 = CodeAreaSize;
		AddJumpList();
		PutCode(node->GetOpecode() == T_LOGICALOR ?
			(inv?VM_JNF:VM_JF) : (inv?VM_JF:VM_JNF), node_pos);
		PutCode(0, node_pos); // *A
		resaddr2 = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED|TJS_RT_CFLAG,
			0, tSubParam());
		if(!TJSIsCondFlagRetValue(resaddr2))
		{
			PutCode(inv?VM_TF:VM_TT, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr2), node_pos);
		}
		else
		{
			if((inv != false) != (resaddr2==TJS_GNC_CFLAG_I))
				PutCode(VM_NF, node_pos); // invert flag
		}
		CodeArea[addr1 + 1] = CodeAreaSize - addr1; // patch *A
		if(!(restype & TJS_RT_CFLAG))
		{
			// requested result type is not condition flag
			if(TJSIsCondFlagRetValue(resaddr1) || !TJSIsFrame(resaddr1))
			{
				PutCode(inv?VM_SETNF:VM_SETF, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
				resaddr1 = frame;
				frame++;
			}
			else
			{
				PutCode(inv?VM_SETNF:VM_SETF, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
			}
		}
		return (restype & TJS_RT_CFLAG) ?
			(inv?TJS_GNC_CFLAG_I:TJS_GNC_CFLAG) : resaddr1;
	  }

	case T_INSTANCEOF: // 'instanceof' operator
	  {
		// instanceof operator
		tjs_int resaddr1, resaddr2;
		resaddr1 = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, tSubParam());
		if(!TJSIsFrame(resaddr1))
		{
			PutCode(VM_CP, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
			resaddr1 = frame;
			frame++;
		}
		resaddr2 = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, tSubParam());
		PutCode(VM_CHKINS, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr2), node_pos);
        return resaddr1;
	  }

	case T_VERTLINE:	// '|' operator
	case T_CHEVRON:		// '^' operator
	case T_AMPERSAND:	// binary '&' operator
	case T_RARITHSHIFT:	// '>>' operator
	case T_LARITHSHIFT:	// '<<' operator
	case T_RBITSHIFT:	// '>>>' operator
	case T_PLUS:		// binary '+' operator
	case T_MINUS:		// '-' operator
	case T_PERCENT:		// '%' operator
	case T_SLASH:		// '/' operator
	case T_BACKSLASH:	// '\' operator
	case T_ASTERISK:	// binary '*' operator
	  {
		// general two-term operators
		tjs_int resaddr1, resaddr2;
		if(TJSIsModifySubType(param.SubType)) _yyerror(TJSCannotModifyLHS, Block);
		resaddr1 = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, tSubParam());
		if(!TJSIsFrame(resaddr1))
		{
			PutCode(VM_CP, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
			resaddr1 = frame;
			frame++;
		}
		resaddr2 = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, tSubParam());
		tjs_int32 code;
		switch(node->GetOpecode())  // sucking....
		{
		case T_VERTLINE:		code = VM_BOR;		break;
		case T_CHEVRON:			code = VM_BXOR;		break;
		case T_AMPERSAND:		code = VM_BAND;		break;
		case T_RARITHSHIFT:		code = VM_SAR;		break;
		case T_LARITHSHIFT:		code = VM_SAL;		break;
		case T_RBITSHIFT:		code = VM_SR;		break;
		case T_PLUS:			code = VM_ADD;		break;
		case T_MINUS:			code = VM_SUB;		break;
		case T_PERCENT:			code = VM_MOD;		break;
		case T_SLASH:			code = VM_DIV;		break;
		case T_BACKSLASH:		code = VM_IDIV;		break;
		case T_ASTERISK:		code = VM_MUL;		break;
		}

		PutCode(code, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr2), node_pos);
		return resaddr1;
	  }

	case T_NOTEQUAL:		// '!=' operator
	case T_EQUALEQUAL:		// '==' operator
	case T_DISCNOTEQUAL:	// '!==' operator
	case T_DISCEQUAL:		// '===' operator
	case T_LT:				// '<' operator
	case T_GT:				// '>' operator
	case T_LTOREQUAL:		// '<=' operator
	case T_GTOREQUAL:		// '>=' operator
	  {
		// comparison operators
		tjs_int resaddr1, resaddr2;
		if(TJSIsModifySubType(param.SubType)) _yyerror(TJSCannotModifyLHS, Block);
		resaddr1 = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, tSubParam());
		if(!(restype & TJS_RT_CFLAG))
		{
			if(!TJSIsFrame(resaddr1))
			{
				PutCode(VM_CP, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
				resaddr1 = frame;
				frame++;
			}
		}
		resaddr2 = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, tSubParam());
		tjs_int32 code1, code2;
		switch(node->GetOpecode())  // ...
		{
		case T_NOTEQUAL:		code1 = VM_CEQ;		code2 = VM_SETNF; 	break;
		case T_EQUALEQUAL:		code1 = VM_CEQ;		code2 = VM_SETF;	break;
		case T_DISCNOTEQUAL:	code1 = VM_CDEQ;	code2 = VM_SETNF;	break;
		case T_DISCEQUAL:		code1 = VM_CDEQ;	code2 = VM_SETF;	break;
		case T_LT:				code1 = VM_CLT;		code2 = VM_SETF;	break;
		case T_GT:				code1 = VM_CGT;		code2 = VM_SETF;	break;
		case T_LTOREQUAL:		code1 = VM_CGT;		code2 = VM_SETNF;	break;
		case T_GTOREQUAL:		code1 = VM_CLT;		code2 = VM_SETNF;	break;
		}

		PutCode(code1, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr2), node_pos);

		if(!(restype & TJS_RT_CFLAG))
		{
			PutCode(code2, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr1), node_pos);
		}

		return (restype & TJS_RT_CFLAG) ?
			(code2 == VM_SETNF?TJS_GNC_CFLAG_I:TJS_GNC_CFLAG):resaddr1;
	  }

	case T_EXCRAMATION: // pre-positioned '!' operator
	  {
		// logical not
		if(TJSIsModifySubType(param.SubType)) _yyerror(TJSCannotModifyLHS, Block);
		resaddr = _GenNodeCode(frame, (*node)[0], restype, reqresaddr, tSubParam());
		if(!(restype & TJS_RT_CFLAG))
		{
			// value as return value required
			if(!TJSIsFrame(resaddr))
			{
				PutCode(VM_CP, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
				resaddr = frame;
				frame++;
			}
			PutCode(VM_LNOT, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			return resaddr;
		}
		else
		{
			// condifion flag required
			if(!TJSIsCondFlagRetValue(resaddr))
			{
				PutCode(VM_TF, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr));
				return TJS_GNC_CFLAG;
			}

			return resaddr == TJS_GNC_CFLAG_I ? TJS_GNC_CFLAG : TJS_GNC_CFLAG_I;
				// invert flag
		}

	  }

	case T_TILDE:		// '~' operator
	case T_SHARP:		// '#' operator
	case T_DOLLAR:		// '$' operator
	case T_UPLUS:		// unary '+' operator
	case T_UMINUS:		// unary '-' operator
	case T_INVALIDATE:	// 'invalidate' operator
	case T_ISVALID:		// 'isvalid' operator
	case T_EVAL:		// post-positioned '!' operator
	case T_INT:			// 'int' operator
	case T_REAL:		// 'real' operator
	case T_STRING:		// 'string' operator
	case T_OCTET:		// 'octet' operator
	  {
		// general unary operators
		if(TJSIsModifySubType(param.SubType)) _yyerror(TJSCannotModifyLHS, Block);
		resaddr = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, tSubParam());
		if(!TJSIsFrame(resaddr))
		{
			PutCode(VM_CP, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			resaddr = frame;
			frame++;
		}
		tjs_int32 code;
		switch(node->GetOpecode())
		{
//			case T_EXCRAMATION:		code = VM_LNOT;			break;
			case T_TILDE:			code = VM_BNOT;			break;
			case T_SHARP:			code = VM_ASC;			break;
			case T_DOLLAR:			code = VM_CHR;			break;
			case T_UPLUS:			code = VM_NUM;			break;
			case T_UMINUS:			code = VM_CHS;			break;
			case T_INVALIDATE:		code = VM_INV;			break;
			case T_ISVALID:			code = VM_CHKINV;		break;
			case T_TYPEOF:			code = VM_TYPEOF;		break;
			case T_EVAL:			code = (restype & TJS_RT_NEEDED)?
										   VM_EVAL:VM_EEXP;

							// warn if T_EVAL is used in non-global position
							if(TJSWarnOnNonGlobalEvalOperator &&
								ContextType != ctTopLevel)
								OutputWarning(TJSWarnEvalOperator);

															break;
			case T_INT:				code = VM_INT;			break;
			case T_REAL:			code = VM_REAL;			break;
			case T_STRING:			code = VM_STR;			break;
			case T_OCTET:			code = VM_OCTET;		break;
		}
		PutCode(code, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);


		return resaddr;
	  }


	case T_TYPEOF:  // 'typeof' operator
	  {
		// typeof
		if(TJSIsModifySubType(param.SubType)) _yyerror(TJSCannotModifyLHS, Block);
		bool haspropnode;
		tTJSExprNode *cnode = (*node)[0];
		if(cnode->GetOpecode() == T_DOT || cnode->GetOpecode() == T_LBRACKET ||
			cnode->GetOpecode() == T_WITHDOT)
			haspropnode = true;
		else
			haspropnode = false;

		if(haspropnode)
		{
			// has property access node
			tSubParam param2;
			param2.SubType = stTypeOf;
			return _GenNodeCode(frame, cnode, TJS_RT_NEEDED, 0, param2);
		}
		else
		{
			// normal operation
			resaddr = _GenNodeCode(frame, cnode, TJS_RT_NEEDED, 0, tSubParam());

			if(!TJSIsFrame(resaddr))
			{
				PutCode(VM_CP, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
				resaddr = frame;
				frame++;
			}
			PutCode(VM_TYPEOF, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			return resaddr;

		}
	  }

	case T_DELETE:			// 'delete' operator
	case T_INCREMENT:		// pre-positioned '++' operator
	case T_DECREMENT:		// pre-positioned '--' operator
	case T_POSTINCREMENT:	// post-positioned '++' operator
	case T_POSTDECREMENT:	// post-positioned '--' operator
	  {
		// delete, typeof, increment and decrement
		if(TJSIsModifySubType(param.SubType)) _yyerror(TJSCannotModifyLHS, Block);
		tSubParam param2;
		switch(node->GetOpecode())
		{
		case T_TYPEOF:			param2.SubType = stTypeOf;		break;
		case T_DELETE:			param2.SubType = stDelete;		break;
		case T_INCREMENT:		param2.SubType = stPreInc;		break;
		case T_DECREMENT:		param2.SubType = stPreDec;		break;
		case T_POSTINCREMENT:	param2.SubType = stPostInc;		break;
		case T_POSTDECREMENT:	param2.SubType = stPostDec;		break;
		}
//		param2.SubAddress = frame-1;
		return _GenNodeCode(frame, (*node)[0], restype, reqresaddr, param2);
	  }

	case T_LPARENTHESIS:	// '( )' operator
	case T_NEW:				// 'new' operator
	  {
		// function call or create-new object

		// does (*node)[0] have a node that acceesses any properties ?
		bool haspropnode, hasnonlocalsymbol;
		tTJSExprNode *cnode = (*node)[0];
		if(node->GetOpecode() == T_LPARENTHESIS &&
			(cnode->GetOpecode() == T_DOT || cnode->GetOpecode() == T_LBRACKET))
			haspropnode = true;
		else
			haspropnode = false;

		// does (*node)[0] have a node that accesses non-local functions ?
		if(node->GetOpecode() == T_LPARENTHESIS && cnode->GetOpecode() == T_SYMBOL)
		{
			if(AsGlobalContextMode)
			{
				hasnonlocalsymbol = true;
			}
			else
			{
				tTJSVariantString *str = cnode->GetValue().AsString();
				if(Namespace.Find(str->operator const tjs_char *()) == -1)
					hasnonlocalsymbol = true;
				else
					hasnonlocalsymbol = false;
				str->Release();
			}
		}
		else
		{
			hasnonlocalsymbol = false;
		}

		// flag which indicates whether to do direct or indirect call access
		bool do_direct_access = haspropnode || hasnonlocalsymbol;

		// reserve frame
		if(!do_direct_access && (restype & TJS_RT_NEEDED) )
			frame++; // reserve the frame for a result value

		// generate function call codes
		StartFuncArg();
		tjs_int framestart = frame;
		tjs_int res;
		try
		{
			// arguments is
			if((*node)[1]->GetSize() == 1 && (*(*node)[1])[0] == NULL)
			{
				// empty
			}
			else
			{
				// exist
				_GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, tSubParam());
			}

			// compilation of expression that represents the function
			tSubParam param2;


			if(do_direct_access)
			{
				param2.SubType = stFuncCall; // creates code with stFuncCall
				res = _GenNodeCode(frame, (*node)[0], restype, reqresaddr, param2);
			}
			else
			{
				param2.SubType = stNone;
				resaddr = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, param2);

				// code generatio of function calling
				PutCode(node->GetOpecode() == T_NEW ? VM_NEW: VM_CALL, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(
					res = (restype & TJS_RT_NEEDED)?(framestart-1):0),
					node_pos); // result target
				PutCode(TJS_TO_VM_REG_ADDR(resaddr),
					node_pos); // iTJSDispatch2 that points the function

				// generate argument code
				GenerateFuncCallArgCode();

				// clears the frame
				ClearFrame(frame, framestart);

			}
		}
		catch(...)
		{
			EndFuncArg();
			throw;
		}

		EndFuncArg();

		return res;
	  }

	case T_ARG:
		// a function argument
		if(node->GetSize() >= 2)
		{
			if((*node)[1]) _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, tSubParam());
		}
		if((*node)[0])
		{
			tTJSExprNode *n = (*node)[0];
			if(n->GetOpecode() == T_EXPANDARG)
			{
				// expanding argument
				if((*n)[0])
					AddFuncArg(_GenNodeCode(
						frame, (*n)[0], TJS_RT_NEEDED, 0, tSubParam()), fatExpand);
				else
					AddFuncArg(0, fatUnnamedExpand);
			}
			else
			{
				AddFuncArg(_GenNodeCode(
					frame, (*node)[0], TJS_RT_NEEDED, 0, tSubParam()), fatNormal);
			}
		}
		else
		{
			AddFuncArg(0, fatNormal);
		}
		return 0;

	case T_OMIT:
		// omitting of the function arguments
		AddOmitArg();
        return 0;

	case T_DOT:			// '.' operator
	case T_LBRACKET:	// '[ ]' operator
	  {
		// member access ( direct or indirect )
		bool direct = node->GetOpecode() == T_DOT;
		tjs_int dp;

		tSubParam param2;
		param2.SubType = stNone;
		resaddr = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, param2);

		if(direct)
			dp = PutData((*node)[1]->GetValue());
		else
			dp = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, tSubParam());

		switch(param.SubType)
		{
		case stNone:
		case stIgnorePropGet:
			if(param.SubType == stNone)
				PutCode(direct ? VM_GPD : VM_GPI, node_pos);
			else
				PutCode(direct ? VM_GPDS : VM_GPIS, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
			frame++;
			return frame-1;

		case stEqual:
		case stIgnorePropSet:
			if(param.SubType == stEqual)
			{
				if((*node)[0]->GetOpecode() == T_THIS_PROXY)
					PutCode(direct ? VM_SPD : VM_SPI, node_pos);
				else
					PutCode(direct ? VM_SPDE : VM_SPIE, node_pos);
			}
			else
			{
				PutCode(direct ? VM_SPDS : VM_SPIS, node_pos);
			}
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(param.SubAddress), node_pos);
			return param.SubAddress;

		case stBitAND:
		case stBitOR:
		case stBitXOR:
		case stSub:
		case stAdd:
		case stMod:
		case stDiv:
		case stIDiv:
		case stMul:
		case stLogOR:
		case stLogAND:
		case stSAR:
		case stSAL:
		case stSR:
			PutCode((tjs_int32)param.SubType + (direct?1:2), node_pos);
				// here adds 1 or 2 to the ope-code
				// ( see the ope-code's positioning order )
			PutCode(TJS_TO_VM_REG_ADDR((restype & TJS_RT_NEEDED) ? frame: 0), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(param.SubAddress), node_pos);
			if(restype & TJS_RT_NEEDED) frame++;
			return (restype & TJS_RT_NEEDED)?frame-1:0;

		case stPreInc:
		case stPreDec:
			PutCode((param.SubType == stPreInc ? VM_INC : VM_DEC) +
				(direct? 1:2), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR((restype & TJS_RT_NEEDED) ? frame: 0), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
			if((restype & TJS_RT_NEEDED)) frame++;
			return (restype & TJS_RT_NEEDED)?frame-1:0;

		case stPostInc:
		case stPostDec:
		  {
			tjs_int retresaddr = 0;
			if(restype & TJS_RT_NEEDED)
			{
				// need result ...
				PutCode(direct ? VM_GPD : VM_GPI, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
				retresaddr = frame;
				frame++;
			}
			PutCode((param.SubType == stPostInc ? VM_INC : VM_DEC) +
				(direct? 1:2), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(0), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
			return retresaddr;
		  }
		case stTypeOf:
		  {
			// typeof
			PutCode(direct? VM_TYPEOFD:VM_TYPEOFI, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR((restype & TJS_RT_NEEDED) ? frame:0), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
			if(restype & TJS_RT_NEEDED) frame++;
			return (restype & TJS_RT_NEEDED)?frame-1:0;
		  }
		case stDelete:
		  {
			// deletion
			PutCode(direct? VM_DELD:VM_DELI, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR((restype & TJS_RT_NEEDED) ? frame:0), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
			PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
			if(restype & TJS_RT_NEEDED) frame++;
			return (restype & TJS_RT_NEEDED)?frame-1:0;
		  }
		case stFuncCall:
		  {
			// function call
			PutCode(direct ? VM_CALLD:VM_CALLI, node_pos);
			PutCode(TJS_TO_VM_REG_ADDR((restype & TJS_RT_NEEDED) ? frame:0),
				node_pos); // result target
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos); // the object
			PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos); // function name

			// generate argument code
			GenerateFuncCallArgCode();

			// extend frame and return
			if(restype & TJS_RT_NEEDED) frame++;
			return (restype & TJS_RT_NEEDED)?frame-1:0;
		  }

		default:
			_yyerror(TJSCannotModifyLHS, Block);
			return 0;
		}
	  }


	case T_SYMBOL:	// symbol
	  {
		// accessing to a variable
		tjs_int n;
		if(AsGlobalContextMode)
		{
			n = -1; // global mode cannot access local variables
		}
		else
		{
			tTJSVariantString *str = node->GetValue().AsString();
			n = Namespace.Find(str->operator const tjs_char *());
			str->Release();
		}

		if(n!=-1)
		{
			bool isstnone = !TJSIsModifySubType(param.SubType);

			if(!isstnone)
			{
				// substitution, or like it
				switch(param.SubType)
				{
				case stEqual:
					PutCode(VM_CP, node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(param.SubAddress), node_pos);
					break;

				case stBitAND:
				case stBitOR:
				case stBitXOR:
				case stSub:
				case stAdd:
				case stMod:
				case stDiv:
				case stIDiv:
				case stMul:
				case stLogOR:
				case stLogAND:
				case stSAR:
				case stSAL:
				case stSR:
					PutCode(param.SubType, node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(param.SubAddress), node_pos);
					return (restype & TJS_RT_NEEDED)?-n-VariableReserveCount-1:0;

				case stPreInc: // pre-positioning
					PutCode(VM_INC, node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), node_pos);
					return (restype & TJS_RT_NEEDED)?-n-VariableReserveCount-1:0;

				case stPreDec: // pre-
					PutCode(VM_DEC, node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), node_pos);
					return (restype & TJS_RT_NEEDED)?-n-VariableReserveCount-1:0;

				case stPostInc: // post-
					if(restype & TJS_RT_NEEDED)
					{
						PutCode(VM_CP, node_pos);
						PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
						PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), node_pos);
						frame++;
					}
					PutCode(VM_INC, node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), node_pos);
					return (restype & TJS_RT_NEEDED)?frame-1:0;

				case stPostDec: // post-
					if(restype & TJS_RT_NEEDED)
					{
						PutCode(VM_CP, node_pos);
						PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
						PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), node_pos);
						frame++;
					}
					PutCode(VM_DEC, node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), node_pos);
					return (restype & TJS_RT_NEEDED)?frame-1:0;

				case stDelete: // deletion
				  {
#if 0
					PutCode(VM_CL, node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), node_pos);
#endif
					tTJSVariantString *str = node->GetValue().AsString();
					Namespace.Remove(*str);
					str->Release();
					if(restype & TJS_RT_NEEDED)
					{
						tjs_int dp = PutData(tTJSVariant(tTVInteger(true)));
						PutCode(VM_CONST, node_pos);
						PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
						PutCode(TJS_TO_VM_REG_ADDR(dp), node_pos);
						return frame-1;
					}
					return 0;
				  }
				default:
					_yyerror(TJSCannotModifyLHS, Block);
				}
				return 0;
			}
			else
			{
				// read
				tTJSVariantString *str = node->GetValue().AsString();
//				Namespace.Add(str->operator tjs_char *());
				tjs_int n = Namespace.Find(str->operator const tjs_char *());
				str->Release();
				return -n-VariableReserveCount-1;
			}
		}
		else
		{
			// n==-1 ( indicates the variable is not found in the local  )
			// assume the variable is in "this".
			// make nodes that refer "this" and process it
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_this_proxy);
#endif
			tTJSExprNode nodep;
			nodep.SetOpecode(T_DOT);
			nodep.SetPosition(node_pos);
			tTJSExprNode *node1 = new tTJSExprNode;
			NodeToDeleteVector.push_back(node1);
			nodep.Add(node1);
			node1->SetOpecode(AsGlobalContextMode?T_GLOBAL:T_THIS_PROXY);
			node1->SetPosition(node_pos);
			tTJSExprNode *node2 = new tTJSExprNode;
			NodeToDeleteVector.push_back(node2);
			nodep.Add(node2);
			node2->SetOpecode(T_SYMBOL);
			node2->SetPosition(node_pos);
			node2->SetValue(node->GetValue());
			return _GenNodeCode(frame, &nodep, restype, reqresaddr, param);
		}
	  }

	case T_IGNOREPROP: // unary '&' operator
	case T_PROPACCESS: // unary '*' operator
		if(node->GetOpecode() ==
			(TJSUnaryAsteriskIgnoresPropAccess?T_PROPACCESS:T_IGNOREPROP))
		{
			// unary '&' operator
			// substance accessing (ignores property operation)
		  	tSubParam sp = param;
			if(sp.SubType == stNone) sp.SubType = stIgnorePropGet;
			else if(sp.SubType == stEqual) sp.SubType = stIgnorePropSet;
			else _yyerror(TJSCannotModifyLHS, Block);
			return _GenNodeCode(frame, (*node)[0], restype, reqresaddr, sp);
		}
		else
		{
			// unary '*' operator
			// force property access
			resaddr = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, tSubParam());
			switch(param.SubType)
			{
			case stNone: // read from property object
				PutCode(VM_GETP, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
				frame ++;
				return frame - 1;

			case stEqual: // write to property object
				PutCode(VM_SETP, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(param.SubAddress), node_pos);
				return param.SubAddress;

			case stBitAND:
			case stBitOR:
			case stBitXOR:
			case stSub:
			case stAdd:
			case stMod:
			case stDiv:
			case stIDiv:
			case stMul:
			case stLogOR:
			case stLogAND:
			case stSAR:
			case stSAL:
			case stSR:
				PutCode((tjs_int32)param.SubType + 3, node_pos);
					// +3 : property access
					// ( see the ope-code's positioning order )
				PutCode(TJS_TO_VM_REG_ADDR((restype & TJS_RT_NEEDED) ? frame: 0), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(param.SubAddress), node_pos);
				if(restype & TJS_RT_NEEDED) frame++;
				return (restype & TJS_RT_NEEDED)?frame-1:0;

			case stPreInc:
			case stPreDec:
				PutCode((param.SubType == stPreInc ? VM_INC : VM_DEC) + 3, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR((restype & TJS_RT_NEEDED) ? frame: 0), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
				if((restype & TJS_RT_NEEDED)) frame++;
				return (restype & TJS_RT_NEEDED)?frame-1:0;

			case stPostInc:
			case stPostDec:
			  {
				tjs_int retresaddr = 0;
				if(restype & TJS_RT_NEEDED)
				{
					// need result ...
					PutCode(VM_GETP, node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
					PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
					retresaddr = frame;
					frame++;
				}
				PutCode((param.SubType == stPostInc ? VM_INC : VM_DEC) + 3, node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(0), node_pos);
				PutCode(TJS_TO_VM_REG_ADDR(resaddr), node_pos);
				return retresaddr;
			  }

			default:
				_yyerror(TJSCannotModifyLHS, Block);
				return 0;
			}
		}


	case T_SUPER: // 'super'
	  {
		// refer super class

		//tjs_int dp;
		tTJSExprNode * node;
		if(Parent && Parent->ContextType == ctProperty)
		{
			if((node = Parent->Parent->SuperClassExpr) == NULL)
			{
				_yyerror(TJSCannotGetSuper, Block);
				return 0;
			}
		}
		else
		{
			if(!Parent || (node = Parent->SuperClassExpr) == NULL)
			{
				_yyerror(TJSCannotGetSuper, Block);
				return 0;
			}
		}

		AsGlobalContextMode = true;
			// the code must be generated in global context
			
		try
		{
			resaddr = _GenNodeCode(frame, node, restype, reqresaddr, param);
		}
		catch(...)
		{
			AsGlobalContextMode = false;
			throw;
		}

   		AsGlobalContextMode = false;

		return resaddr;
	  }

	case T_THIS:
		if(param.SubType) _yyerror(TJSCannotModifyLHS, Block);
		return -1;

	case T_THIS_PROXY:
		// this-proxy is a special register that points
		// both "objthis" and "global"
		// if refering member is not in "objthis", this-proxy
		// refers "global".
		return -VariableReserveCount;

	case T_WITHDOT: // unary '.' operator
	  {
		// dot operator omitting object name
		tTJSExprNode nodep;
		nodep.SetOpecode(T_DOT);
		nodep.SetPosition(node_pos);
		tTJSExprNode *node1 = new tTJSExprNode;
		NodeToDeleteVector.push_back(node1);
		nodep.Add(node1);
		node1->SetOpecode(T_WITHDOT_PROXY);
		node1->SetPosition(node_pos);
		nodep.Add((*node)[0]);
		return _GenNodeCode(frame, &nodep, restype, reqresaddr, param);
 	  }

	case T_WITHDOT_PROXY:
	  {
		// virtual left side of "." operator which omits object

		// search in NestVector
		tjs_int i = (tjs_int)(NestVector.size() -1);
		for(; i>=0; i--)
		{
			tNestData &data = NestVector[i];
			if(data.Type == ntWith)
			{
				// found
				return data.RefRegister;
			}
		}

		// not found in NestVector ...
	  }

		// NO "break" HERE!!!!!! (pass thru to global)

	case T_GLOBAL:
	  {
		if(param.SubType) _yyerror(TJSCannotModifyLHS, Block);
		if(!(restype & TJS_RT_NEEDED)) return 0;
		PutCode(VM_GLOBAL, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame), node_pos);
		frame++;
		return frame-1;
	  }

	case T_INLINEARRAY:
	  {
		// inline array

		tjs_int arraydp = PutData(tTJSVariant(TJS_W("Array")));
		//	global %frame0
		//	gpd %frame1, %frame0 . #arraydp // #arraydp = Array
		tjs_int frame0 = frame;
		PutCode(VM_GLOBAL, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+0), node_pos);
		PutCode(VM_GPD, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+1), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+0), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(arraydp), node_pos);
		//	new %frame0, %frame1()
		PutCode(VM_NEW, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+0), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+1), node_pos);
		PutCode(0);  // argument count for "new Array"
		//	const %frame1, #zerodp
		tjs_int zerodp = PutData(tTJSVariant(tTVInteger(0)));
		PutCode(VM_CONST, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+1), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(zerodp), node_pos);
		frame += 2;

		ArrayArgStack.push(tArrayArg());
		ArrayArgStack.top().Object = frame0;
		ArrayArgStack.top().Counter = frame0 + 1;

		tjs_int nodesize = node->GetSize();
		if(node->GetSize() == 1 && (*(*node)[0])[0] == NULL)
		{
			// the element is empty
		}
		else
		{
			for(tjs_int i = 0; i<nodesize; i++)
			{
				_GenNodeCode(frame, (*node)[i], TJS_RT_NEEDED, 0, tSubParam()); // elements
			}
		}

		ArrayArgStack.pop();
		return (restype & TJS_RT_NEEDED)?(frame0):0;
	  }

	case T_ARRAYARG:
	  {
		// an element of inline array
		tjs_int framestart = frame;

		resaddr = (*node)[0]?_GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, tSubParam()):0;

		// spis %object.%count, %resaddr
		PutCode(VM_SPIS, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(ArrayArgStack.top().Object));
		PutCode(TJS_TO_VM_REG_ADDR(ArrayArgStack.top().Counter));
		PutCode(TJS_TO_VM_REG_ADDR(resaddr));
		// inc %count
		PutCode(VM_INC);
		PutCode(TJS_TO_VM_REG_ADDR(ArrayArgStack.top().Counter));

		ClearFrame(frame, framestart);

		return 0;
	  }


	case T_INLINEDIC:
	  {
		// inline dictionary
		tjs_int dicdp = PutData(tTJSVariant(TJS_W("Dictionary")));
		//	global %frame0
		//	gpd %frame1, %frame0 . #dicdp // #dicdp = Dictionary
		tjs_int frame0 = frame;
		PutCode(VM_GLOBAL, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+0), node_pos);
		PutCode(VM_GPD, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+1), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+0), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(dicdp), node_pos);
		//	new %frame0, %frame1()
		PutCode(VM_NEW, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+0), node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame+1), node_pos);
		PutCode(0);  // argument count for "Dictionary" class
		frame += 2;
		ClearFrame(frame, frame0 + 1);  // clear register at frame+1

		ArrayArgStack.push(tArrayArg());
		ArrayArgStack.top().Object = frame0;

		tjs_int nodesize = node->GetSize();
		for(tjs_int i = 0; i < nodesize; i++)
		{
			_GenNodeCode(frame, (*node)[i], TJS_RT_NEEDED, 0, tSubParam()); // element
		}

		ArrayArgStack.pop();
		return (restype & TJS_RT_NEEDED) ? (frame0): 0;
	  }

	case T_DICELM:
	  {
		// an element of inline dictionary
		tjs_int framestart = frame;
		tjs_int name;
		tjs_int value;
		name = _GenNodeCode(frame, (*node)[0], TJS_RT_NEEDED, 0, tSubParam());
		value = _GenNodeCode(frame, (*node)[1], TJS_RT_NEEDED, 0, tSubParam());
		// spis %object.%name, %value
		PutCode(VM_SPIS, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(ArrayArgStack.top().Object));
		PutCode(TJS_TO_VM_REG_ADDR(name));
		PutCode(TJS_TO_VM_REG_ADDR(value));

		ClearFrame(frame, framestart);

		return 0;
	  }

	case T_REGEXP:
	  {
		// constant regular expression
		if(!(restype & TJS_RT_NEEDED)) return 0;
		tjs_int regexpdp = PutData(tTJSVariant(TJS_W("RegExp")));
		tjs_int patdp = PutData(node->GetValue());
		tjs_int compiledp = PutData(tTJSVariant(TJS_W("_compile")));
		// global %frame0
		//	gpd %frame1, %frame0 . #regexpdp // #regexpdp = RegExp
		tjs_int frame0 = frame;
		PutCode(VM_GLOBAL, node_pos);
		PutCode(TJS_TO_VM_REG_ADDR(frame));
		PutCode(VM_GPD);
		PutCode(TJS_TO_VM_REG_ADDR(frame + 1));
		PutCode(TJS_TO_VM_REG_ADDR(frame));
		PutCode(TJS_TO_VM_REG_ADDR(regexpdp));
		// const frame2, patdp;
		PutCode(VM_CONST);
		PutCode(TJS_TO_VM_REG_ADDR(frame + 2));
		PutCode(TJS_TO_VM_REG_ADDR(patdp));
		// new frame0 , frame1();
		PutCode(VM_NEW);
		PutCode(TJS_TO_VM_REG_ADDR(frame));
		PutCode(TJS_TO_VM_REG_ADDR(frame+1));
		PutCode(0);
		// calld 0, frame0 . #compiledp(frame2)
		PutCode(VM_CALLD);
		PutCode(TJS_TO_VM_REG_ADDR(0));
		PutCode(TJS_TO_VM_REG_ADDR(frame0));
		PutCode(TJS_TO_VM_REG_ADDR(compiledp));
		PutCode(1);
		PutCode(TJS_TO_VM_REG_ADDR(frame+2));
		frame+=3;
		ClearFrame(frame, frame0 + 1);

		return frame0;
	  }

	case T_VOID:
		if(param.SubType) _yyerror(TJSCannotModifyLHS, Block);
		if(!(restype & TJS_RT_NEEDED)) return 0;
		return 0; // 0 is always void
	}

	return 0;
}
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint time_GenNodeCode = 0;
#endif

tjs_int tTJSInterCodeContext::_GenNodeCode(tjs_int & frame, tTJSExprNode *node,
	tjs_uint32 restype, tjs_int reqresaddr,
	const tSubParam & param)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_GenNodeCode);
#endif
	tjs_int res = GenNodeCode(frame, node, restype, reqresaddr, param);
	return res;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::StartFuncArg()
{
	// notify the start of function arguments
	// create a stack for function arguments
	tFuncArg arg;
	FuncArgStack.push(arg);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::AddFuncArg(const tjs_int addr, tTJSFuncArgType type)
{
	// add a function argument
	// addr = register address to add
	FuncArgStack.top().ArgVector.push_back(tFuncArgItem(addr, type));
	if(type == fatExpand || type == fatUnnamedExpand)
		FuncArgStack.top().HasExpand = true; // has expanding node
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EndFuncArg()
{
	// notify the end of function arguments
	FuncArgStack.pop();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::AddOmitArg()
{
	// omit of the function arguments
	if(ContextType != ctFunction && ContextType != ctExprFunction)
	{
		_yyerror(TJSCannotOmit, Block);
	}
	FuncArgStack.top().IsOmit = true;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::DoNestTopExitPatch(void)
{
	// process the ExitPatchList which must be in the top of NextVector
	std::vector<tjs_int> & vector = NestVector.back().ExitPatchVector;
	std::vector<tjs_int>::iterator i;
	for(i = vector.begin(); i != vector.end(); i++)
	{
		CodeArea[*i +1] = CodeAreaSize - *i;
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::DoContinuePatch(tNestData & nestdata)
{
	// process the ContinuePatchList which must be in the top of NextVector

	std::vector<tjs_int> & vector = nestdata.ContinuePatchVector;
	std::vector<tjs_int>::iterator i;
	for(i = vector.begin(); i != vector.end(); i++)
	{
		CodeArea[*i +1] = CodeAreaSize - *i;
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ClearLocalVariable(tjs_int top, tjs_int bottom)
{
	// clear local variable registers from top-1 to bottom
#if 0
	if(top - bottom >= 3)
	{
		PutCode(VM_CCL); // successive clear instruction
		PutCode(TJS_TO_VM_REG_ADDR(-(top-1)-VariableReserveCount-1));
		PutCode(top-bottom);
	}
	else
	{
		for(tjs_int i = bottom; i<top; i++)
		{
			PutCode(VM_CL);
			PutCode(TJS_TO_VM_REG_ADDR(-i-VariableReserveCount-1));
		}
	}
#endif
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ClearFrame(tjs_int &frame, tjs_int base)
{
	// clear frame registers from "frame-1" to "base"
	// "base" is regaeded as "FrameBase" when "base" had been omitted.
	// "frame" may be changed.

	if(base == -1) base = FrameBase;

	if(frame-1 > MaxFrameCount) MaxFrameCount = frame-1;

	if(frame - base >= 3)
	{
#if 0
		PutCode(VM_CCL);
		PutCode(TJS_TO_VM_REG_ADDR(base));
		PutCode(frame-base);
#endif
		frame = base;
	}
	else
	{
		while(frame > base)
		{
			frame--;
#if 0
			PutCode(VM_CL);
			PutCode(TJS_TO_VM_REG_ADDR(frame));
#endif
		}
	}
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void tTJSInterCodeContext::AddLocalVariable(const tjs_char *name, tjs_int init)
{
	// create a local variable
	// ( however create it in "this" when the variable is defined at global )
	// name = variable name

	// init = register address that points initial value ( 0 = no initial value )

	tjs_int base = ContextType == ctClass ? 2: 1;
	if(Namespace.GetLevel() >= base)
	{
		// create on local
//		tjs_int ff = Namespace.Find(name);
		Namespace.Add(name);
		if(init != 0)
		{
			// initial value is given
			tjs_int n = Namespace.Find(name);
#ifdef ENABLE_DEBUGGER
			int regoffset = TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1);
			// class name, func name, file name, code offset, var name, reg offset
			TJSDebuggerAddLocalVariable( GetClassName().c_str(), GetName(), Block->GetName(), FunctionRegisterCodePoint, name, regoffset );
#endif // ENABLE_DEBUGGER
			PutCode(VM_CP, LEX_POS);
			PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), LEX_POS);
			PutCode(TJS_TO_VM_REG_ADDR(init), LEX_POS);
		}
		else/* if(ff==-1) */
		{
			// first initialization
			tjs_int n = Namespace.Find(name);
#ifdef ENABLE_DEBUGGER
			int regoffset = TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1);
			// class name, func name, file name, code offset, var name, reg offset
			TJSDebuggerAddLocalVariable( GetClassName().c_str(), GetName(), Block->GetName(), FunctionRegisterCodePoint, name, regoffset );
#endif // ENABLE_DEBUGGER
			PutCode(VM_CL, LEX_POS);
			PutCode(TJS_TO_VM_REG_ADDR(-n-VariableReserveCount-1), LEX_POS);
		}
	}
	else
	{
		// create member on this
		tjs_int	dp = PutData(tTJSVariant(name));
#ifdef ENABLE_DEBUGGER
		TJSDebuggerAddClassVariable( GetSelfClassName().c_str(), name, TJS_TO_VM_REG_ADDR(dp) );
#endif // ENABLE_DEBUGGER
		PutCode(VM_SPDS, LEX_POS);
		PutCode(TJS_TO_VM_REG_ADDR(-1), LEX_POS);
		PutCode(TJS_TO_VM_REG_ADDR(dp), LEX_POS);
		PutCode(TJS_TO_VM_REG_ADDR(init), LEX_POS);
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::InitLocalVariable(const tjs_char *name, tTJSExprNode *node)
{
	// create a local variable named "name", with inial value of the
	// expression node of "node".

	tjs_int fr = FrameBase;
	tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED, 0, tSubParam());
	AddLocalVariable(name, resaddr);
	ClearFrame(fr);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::InitLocalFunction(const tjs_char *name, tjs_int data)
{
	// create a local function variable pointer by data ( in DataArea ),
	// named "name".

	tjs_int fr = FrameBase;
	PutCode(VM_CONST, LEX_POS);
	PutCode(TJS_TO_VM_REG_ADDR(fr), LEX_POS);
	PutCode(TJS_TO_VM_REG_ADDR(data));
	fr++;
	AddLocalVariable(name, fr-1);
	ClearFrame(fr);
}
//---------------------------------------------------------------------------

void tTJSInterCodeContext::CreateExprCode(tTJSExprNode *node)
{
	// create code of node
	tjs_int fr = FrameBase;
	GenNodeCode(fr, node, 0, 0, tSubParam());
	ClearFrame(fr);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EnterWhileCode(bool do_while)
{
	// enter to "while"
	// ( do_while = true indicates do-while syntax )
	NestVector.push_back(tNestData());
	NestVector.back().Type = do_while?ntDoWhile:ntWhile;
	NestVector.back().LoopStartIP = CodeAreaSize;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::CreateWhileExprCode(tTJSExprNode *node, bool do_while)
{
	// process the condition expression "node"

	if(do_while)
	{
		DoContinuePatch(NestVector.back());
	}

	tjs_int fr = FrameBase;
	tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED|TJS_RT_CFLAG, 0, tSubParam());
	bool inv = false;
	if(!TJSIsCondFlagRetValue(resaddr))
	{
		PutCode(VM_TT, NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
	}
	else
	{
		if(resaddr == TJS_GNC_CFLAG_I) inv = true;
	}
	ClearFrame(fr);

	if(!do_while)
	{
		NestVector.back().ExitPatchVector.push_back(CodeAreaSize);
		AddJumpList();
		PutCode(inv?VM_JF:VM_JNF, NODE_POS);
		PutCode(0, NODE_POS);
	}
	else
	{
		tjs_int jmp_ip = CodeAreaSize;
		AddJumpList();
		PutCode(inv?VM_JNF:VM_JF, NODE_POS);
		PutCode(NestVector.back().LoopStartIP - jmp_ip, NODE_POS);
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ExitWhileCode(bool do_while)
{
	// exit from "while"
	if(NestVector.size() == 0)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}
	if(do_while)
	{
		if(NestVector.back().Type != ntDoWhile)
		{
			_yyerror(TJSSyntaxError, Block);
			return;
		}
	}
	else
	{
		if(NestVector.back().Type != ntWhile)
		{
			_yyerror(TJSSyntaxError, Block);
			return;
		}
	}

	if(!do_while)
	{
		tjs_int jmp_ip = CodeAreaSize;
		AddJumpList();
		PutCode(VM_JMP, LEX_POS);
		PutCode(NestVector.back().LoopStartIP - jmp_ip, LEX_POS);
	}
	DoNestTopExitPatch();
	NestVector.pop_back();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EnterIfCode()
{
	// enter to "if"

	NestVector.push_back(tNestData());
	NestVector.back().Type = ntIf;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::CreateIfExprCode(tTJSExprNode *node)
{
	// process condition expression "node"

	tjs_int fr = FrameBase;
	tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED|TJS_RT_CFLAG, 0, tSubParam());
	bool inv = false;
	if(!TJSIsCondFlagRetValue(resaddr))
	{
		PutCode(VM_TT, NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
	}
	else
	{
		if(resaddr == TJS_GNC_CFLAG_I) inv = true;
	}
	ClearFrame(fr);
	NestVector.back().Patch1 = CodeAreaSize;
	AddJumpList();
	PutCode(inv?VM_JF:VM_JNF, NODE_POS);
	PutCode(0, NODE_POS);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ExitIfCode()
{
	// exit from if
	if(NestVector.size() == 0)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}
	if(NestVector.back().Type != ntIf)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}

	CodeArea[NestVector.back().Patch1 + 1] = CodeAreaSize - NestVector.back().Patch1;
	PrevIfExitPatch = NestVector.back().Patch1;
	NestVector.pop_back();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EnterElseCode()
{
	// enter to "else".
	// before is "if", is clear from syntax definition.

	NestVector.push_back(tNestData());
	NestVector.back().Type = ntElse;
	NestVector.back().Patch2 = CodeAreaSize;
	AddJumpList();
	PutCode(VM_JMP, LEX_POS);
	PutCode(0, LEX_POS);
	CodeArea[PrevIfExitPatch + 1] = CodeAreaSize - PrevIfExitPatch;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ExitElseCode()
{
	// exit from else
	if(NestVector.size() == 0)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}
	if(NestVector.back().Type != ntElse)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}

	CodeArea[NestVector.back().Patch2 + 1] = CodeAreaSize - NestVector.back().Patch2;
	NestVector.pop_back();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EnterForCode()
{
	// enter to "for".
	// ( varcreate = true, indicates that the variable is to be created in the
	//	first clause )

	NestVector.push_back(tNestData());
	NestVector.back().Type = ntFor;
	EnterBlock();
	// create a scope for "for" initializing clause even it does not introduce
	//   local variables, due to brevity of semantics
	//NestVector.back().VariableCreated = false;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::CreateForExprCode(tTJSExprNode *node)
{
	// process the "for"'s second clause; a condition expression

	NestVector.back().LoopStartIP = CodeAreaSize;
	if(node)
	{
		tjs_int fr = FrameBase;
		tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED|TJS_RT_CFLAG,
			0, tSubParam());
		bool inv = false;
		if(!TJSIsCondFlagRetValue(resaddr))
		{
			PutCode(VM_TT, NODE_POS);
			PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
		}
		else
		{
			if(resaddr == TJS_GNC_CFLAG_I) inv = true;
		}
		ClearFrame(fr);
		NestVector.back().ExitPatchVector.push_back(CodeAreaSize);
		AddJumpList();
		PutCode(inv?VM_JF:VM_JNF, NODE_POS);
		PutCode(0, NODE_POS);
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::SetForThirdExprCode(tTJSExprNode *node)
{
	// process the "for"'s third clause; a post-loop expression

	NestVector.back().PostLoopExpr = node;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ExitForCode()
{
	// exit from "for"
	tjs_int nestsize = (tjs_int)NestVector.size();
	if(nestsize == 0)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}
	if(NestVector.back().Type != ntFor && NestVector.back().Type != ntBlock)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}

	if(NestVector.back().Type == ntFor)
		DoContinuePatch(NestVector.back());
	if(nestsize >= 2 && NestVector[nestsize-2].Type == ntFor)
		DoContinuePatch(NestVector[nestsize-2]);


	if(NestVector.back().PostLoopExpr)
	{
		tjs_int fr = FrameBase;
		GenNodeCode(fr, NestVector.back().PostLoopExpr, false, 0, tSubParam());
		ClearFrame(fr);
	}
	tjs_int jmp_ip = CodeAreaSize;
	AddJumpList();
	PutCode(VM_JMP, LEX_POS);
	PutCode(NestVector.back().LoopStartIP - jmp_ip, LEX_POS);
	DoNestTopExitPatch();
	ExitBlock();
	DoNestTopExitPatch();
	NestVector.pop_back();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EnterSwitchCode(tTJSExprNode *node)
{
	// enter to "switch"
	// "node" indicates a reference expression

	NestVector.push_back(tNestData());
	NestVector.back().Type = ntSwitch;
	NestVector.back().Patch1 = -1;
	NestVector.back().Patch2 = -1;
	NestVector.back().IsFirstCase = true;

	tjs_int fr = FrameBase;
	tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED, 0, tSubParam());

	if(FrameBase != resaddr)
	{
		PutCode(VM_CP, NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(FrameBase), NODE_POS); // FrameBase points the reference value
		PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
	}

	NestVector.back().RefRegister = FrameBase;

	if(fr-1 > MaxFrameCount) MaxFrameCount = fr-1;

	FrameBase ++; // increment FrameBase
	if(FrameBase-1 > MaxFrameCount) MaxFrameCount = FrameBase-1;

	ClearFrame(fr);

	EnterBlock();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ExitSwitchCode()
{
	// exit from switch

	ExitBlock();

	if(NestVector.size() == 0)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}
	if(NestVector.back().Type != ntSwitch)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}

	tjs_int patch3;
	if(!NestVector.back().IsFirstCase)
	{
		patch3 = CodeAreaSize;
		AddJumpList();
		PutCode(VM_JMP, LEX_POS);
		PutCode(0, LEX_POS);
	}


	if(NestVector.back().Patch1 != -1)
	{
		CodeArea[NestVector.back().Patch1 +1] = CodeAreaSize - NestVector.back().Patch1;
	}

	if(NestVector.back().Patch2 != -1)
	{
		AddJumpList();
		tjs_int jmp_start = CodeAreaSize;
		PutCode(VM_JMP, LEX_POS);
		PutCode(NestVector.back().Patch2 - jmp_start, LEX_POS);
	}

	if(!NestVector.back().IsFirstCase)
	{
		CodeArea[patch3 +1] = CodeAreaSize - patch3;
	}


	DoNestTopExitPatch();
#if 0
	PutCode(VM_CL, LEX_POS);
	PutCode(TJS_TO_VM_REG_ADDR(NestVector.back().RefRegister), LEX_POS);
#endif
	FrameBase--;
	NestVector.pop_back();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ProcessCaseCode(tTJSExprNode *node)
{
	// process "case expression :".
	// process "default :" if node == NULL.

	tjs_int nestsize = (tjs_int)NestVector.size();

	if(nestsize < 3)
	{
		_yyerror(TJSMisplacedCase, Block);
		return;
	}
	if(NestVector[nestsize-1].Type != ntBlock ||
		NestVector[nestsize-2].Type != ntBlock ||
		NestVector[nestsize-3].Type != ntSwitch)
	{
		// the stack layout must be ( from top )
		// ntBlock, ntBlock, ntSwitch
		_yyerror(TJSMisplacedCase, Block);
		return;
	}

	tNestData &data = NestVector[NestVector.size()-3];

	tjs_int patch3;
	if(!data.IsFirstCase)
	{
		patch3 = CodeAreaSize;
		AddJumpList();
		PutCode(VM_JMP, NODE_POS);
		PutCode(0, NODE_POS);
	}

	ExitBlock();
	if(data.Patch1 != -1)
	{
		CodeArea[data.Patch1 +1] = CodeAreaSize -data.Patch1;
	}

	if(node)
	{
		tjs_int fr = FrameBase;
		tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED, 0, tSubParam());
		PutCode(VM_CEQ, NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(data.RefRegister), NODE_POS);
			// compare to reference value with normal comparison
		PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
		ClearFrame(fr);
		data.Patch1 = CodeAreaSize;
		AddJumpList();
		PutCode(VM_JNF, NODE_POS);
		PutCode(0, NODE_POS);
	}
	else
	{
		data.Patch1 = CodeAreaSize;
		AddJumpList();
		PutCode(VM_JMP, NODE_POS);
		PutCode(0, NODE_POS);

		data.Patch2 = CodeAreaSize; // Patch2 = "default:"'s position
	}


	if(!data.IsFirstCase)
	{
		CodeArea[patch3 +1] = CodeAreaSize - patch3;
	}
	data.IsFirstCase = false;

	EnterBlock();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EnterWithCode(tTJSExprNode *node)
{
	// enter to "with"
	// "node" indicates a reference expression

	// this method and ExitWithCode are very similar to switch's code.
	// (those are more simple than that...)

	tjs_int fr = FrameBase;
	tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED, 0, tSubParam());

	if(FrameBase != resaddr)
	{
		// bring the reference variable to frame base top
		PutCode(VM_CP, NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(FrameBase), NODE_POS); // FrameBase points the reference value
		PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
	}

	NestVector.push_back(tNestData());
	NestVector.back().Type = ntWith;

	NestVector.back().RefRegister = FrameBase;

	if(fr-1 > MaxFrameCount) MaxFrameCount = fr-1;

	FrameBase ++; // increment FrameBase
	if(FrameBase-1 > MaxFrameCount) MaxFrameCount = FrameBase-1;

	ClearFrame(fr);

	EnterBlock();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ExitWithCode()
{
	// exit from switch
	ExitBlock();

	if(NestVector.size() == 0)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}
	if(NestVector.back().Type != ntWith)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}

#if 0
	PutCode(VM_CL, LEX_POS);
	PutCode(TJS_TO_VM_REG_ADDR(NestVector.back().RefRegister), LEX_POS);
#endif
	FrameBase--;
	NestVector.pop_back();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::DoBreak(void)
{
	// process "break".

	// search in NestVector backwards
	tjs_int vc = Namespace.GetCount();
	tjs_int pvc = vc;

	tjs_int i = (tjs_int)(NestVector.size() -1);
	for(; i>=0; i--)
	{
		tNestData &data = NestVector[i];
		if(data.Type == ntSwitch ||
			data.Type == ntWhile || data.Type == ntDoWhile ||
			data.Type == ntFor)
		{
			// "break" can apply on this syntax
			ClearLocalVariable(vc, pvc); // clear local variables
			data.ExitPatchVector.push_back(CodeAreaSize);
			AddJumpList();
			PutCode(VM_JMP, LEX_POS);
			PutCode(0, LEX_POS); // later patches here
			return;
		}
		else if(data.Type == ntBlock)
		{
			pvc = data.VariableCount;
		}
		else if(data.Type == ntTry)
		{
			PutCode(VM_EXTRY);
		}
		else if(data.Type == ntSwitch || data.Type == ntWith)
		{
			// clear reference register of "switch" or "with" syntax
#if 0
			PutCode(VM_CL, LEX_POS);
			PutCode(TJS_TO_VM_REG_ADDR(data.RefRegister), LEX_POS);
#endif
		}
	}

	_yyerror(TJSMisplacedBreakContinue, Block);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::DoContinue(void)
{
	// process "continue".

	// generate code that jumps before '}' ( the end of the loop ).
	// for "while" loop, the jump code immediately jumps to the condition check code.

	// search in NestVector backwards
	tjs_int vc = Namespace.GetCount();
	tjs_int pvc = vc;

	tjs_int i = (tjs_int)(NestVector.size() -1);
	for(; i>=0; i--)
	{
		tNestData &data = NestVector[i];
		if(data.Type == ntWhile)
		{
			// for "while" loop
			ClearLocalVariable(vc, pvc); // clear local variables
			tjs_int jmpstart = CodeAreaSize;
			AddJumpList();
			PutCode(VM_JMP, LEX_POS);
			PutCode(data.LoopStartIP - jmpstart, LEX_POS);
			return;
		}
		else if(data.Type == ntDoWhile || data.Type == ntFor)
		{
			// "do-while" or "for" loop needs forward jump
			ClearLocalVariable(vc, pvc); // clears local variables
			data.ContinuePatchVector.push_back(CodeAreaSize);
			AddJumpList();
			PutCode(VM_JMP, LEX_POS);
			PutCode(0, LEX_POS); // later patch this
			return;
		}
		else if(data.Type == ntBlock)
		{
			// does not count variables which created at for loop's
			// first clause
			if(i < 1 || NestVector[i-1].Type != ntFor ||
				!NestVector[i].VariableCreated)
				pvc = data.VariableCount;
		}
		else if(data.Type == ntTry)
		{
			PutCode(VM_EXTRY, LEX_POS);
		}
		else if(data.Type == ntSwitch || data.Type == ntWith)
		{
			// clear reference register of "switch" or "with" syntax
#if 0
			PutCode(VM_CL, LEX_POS);
			PutCode(TJS_TO_VM_REG_ADDR(data.RefRegister), LEX_POS);
#endif
		}
	}

	_yyerror(TJSMisplacedBreakContinue, Block);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::DoDebugger()
{
	// process "debugger" statement.
	PutCode(VM_DEBUGGER, LEX_POS);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ReturnFromFunc(tTJSExprNode *node)
{
	// precess "return"
	// note: the "return" positioned in global immediately returns without
	// execution of the remainder code.

	if(!node)
	{
		// no return value
		PutCode(VM_SRV, NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(0), NODE_POS);  // returns register #0 = void
	}
	else
	{
		// generates return expression
		tjs_int fr = FrameBase;
		tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED, 0, tSubParam());

		PutCode(VM_SRV, NODE_POS);

		PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);

		ClearFrame(fr);

	}

	// clear the frame
	tjs_int org_framebase = FrameBase;
	ClearFrame(FrameBase, 1);
	FrameBase = org_framebase;

	// clear local variables
	ClearLocalVariable(Namespace.GetCount(), 0);

	// check try block
	tjs_int i = (tjs_int)(NestVector.size() -1);
	for(; i>=0; i--)
	{
		tNestData &data = NestVector[i];
		if(data.Type == ntTry)
		{
			PutCode(VM_EXTRY, LEX_POS); // exit from try-protected block
		}
	}


	PutCode(VM_RET, LEX_POS);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EnterTryCode()
{
	// enter to "try"

	NestVector.push_back(tNestData());
	NestVector.back().Type = ntTry;
	NestVector.back().VariableCreated = false;

	NestVector.back().Patch1 = CodeAreaSize;
	AddJumpList();
	PutCode(VM_ENTRY, LEX_POS);
	PutCode(0, LEX_POS);
	PutCode(TJS_TO_VM_REG_ADDR(FrameBase), LEX_POS); // an exception object will be held here

	if(FrameBase > MaxFrameCount) MaxFrameCount = FrameBase;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EnterCatchCode(const tjs_char *name)
{
	// enter to "catch"

	PutCode(VM_EXTRY, LEX_POS);
	NestVector.back().Patch2 = CodeAreaSize;
	AddJumpList();
	PutCode(VM_JMP, LEX_POS);
	PutCode(0, LEX_POS);

	CodeArea[NestVector.back().Patch1 + 1] = CodeAreaSize - NestVector.back().Patch1;

	// clear local variables
	ClearLocalVariable(Namespace.GetMaxCount(), Namespace.GetCount());

	// clear frame
	tjs_int fr = MaxFrameCount + 1;
	tjs_int base = name ? FrameBase+1 : FrameBase;
	ClearFrame(fr, base);

	// change nest type to ntCatch
	NestVector.back().Type = ntCatch;

	// create variable if the catch clause has a receiver variable name
	if(name)
	{
		NestVector.back().VariableCreated = true;
		EnterBlock();
		AddLocalVariable(name, FrameBase);
			// cleate a variable that receives the exception object
	}

}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ExitTryCode()
{
	// exit from "try"
	if(NestVector.size() >= 2)
	{
		if(NestVector[NestVector.size()-2].Type == ntCatch)
		{
			if(NestVector[NestVector.size()-2].VariableCreated)
			{
				ExitBlock();
			}
		}
	}
	
	if(NestVector.size() == 0)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}
	if(NestVector.back().Type != ntCatch)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}


	tjs_int p2addr = NestVector.back().Patch2;

	CodeArea[p2addr + 1] = CodeAreaSize - p2addr;
	NestVector.pop_back();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ProcessThrowCode(tTJSExprNode *node)
{
	// process "throw".
	// node = expressoin to throw

	tjs_int fr = FrameBase;
	tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED, 0, tSubParam());
	PutCode(VM_THROW, NODE_POS);
	PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
	if(fr-1 > MaxFrameCount) MaxFrameCount = fr-1;
#if 0
	while(fr-->FrameBase)
	{
		PutCode(VM_CL);
		PutCode(TJS_TO_VM_REG_ADDR(fr));
	}
#endif
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::CreateExtendsExprCode(tTJSExprNode *node, bool hold)
{
	// process class extender

	//tjs_int num;

	tjs_int fr = FrameBase;
	tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED, 0, tSubParam());

	PutCode(VM_CHGTHIS, NODE_POS);
	PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
	PutCode(TJS_TO_VM_REG_ADDR(-1), NODE_POS);

	PutCode(VM_CALL, NODE_POS);
	PutCode(0, NODE_POS);
	PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
	PutCode(0, NODE_POS);

	if(hold)
	{
		SuperClassExpr = node;
	}

	FunctionRegisterCodePoint = CodeAreaSize; // update FunctionRegisterCodePoint

	// create a Super Class Proxy context
	if(!SuperClassGetter)
	{
		SuperClassGetter =
			new tTJSInterCodeContext(this, Name, Block, ctSuperClassGetter);
	}

	SuperClassGetter->CreateExtendsExprProxyCode(node);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::CreateExtendsExprProxyCode(tTJSExprNode *node)
{
	// create super class proxy to retrieve super class
	SuperClassGetterPointer.push_back(CodeAreaSize);

	tjs_int fr = FrameBase;
	tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED, 0, tSubParam());

	PutCode(VM_SRV);
	PutCode(TJS_TO_VM_REG_ADDR(resaddr));
	ClearFrame(fr);

	PutCode(VM_RET);

	PutCode(VM_NOP, NODE_POS);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::EnterBlock()
{
	// enter to block

	Namespace.Push();
	tjs_int varcount = Namespace.GetCount();
	NestVector.push_back(tNestData());
	NestVector.back().Type = ntBlock;
	NestVector.back().VariableCount = varcount;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ExitBlock()
{
	// exit from block
	if(NestVector.size() == 0)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}
	if(NestVector.back().Type != ntBlock)
	{
		_yyerror(TJSSyntaxError, Block);
		return;
	}

	NestVector.pop_back();
	tjs_int prevcount = Namespace.GetCount();
	Namespace.Pop();
	tjs_int curcount = Namespace.GetCount();
	ClearLocalVariable(prevcount, curcount);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::GenerateFuncCallArgCode()
{
	if(FuncArgStack.top().IsOmit)
	{
		PutCode(-1, LEX_POS); // omit (...) is specified
	}
	else if(FuncArgStack.top().HasExpand)
	{
		PutCode(-2, LEX_POS); // arguments have argument expanding node
		std::vector<tFuncArgItem> & vec = FuncArgStack.top().ArgVector;
		PutCode((tjs_int)vec.size(), LEX_POS); // count of the arguments
		tjs_uint i;
		for(i=0; i<vec.size(); i++)
		{
			PutCode((tjs_int32)vec[i].Type, LEX_POS);
			PutCode(TJS_TO_VM_REG_ADDR(vec[i].Register), LEX_POS);
		}
	}
	else
	{
		std::vector<tFuncArgItem> & vec = FuncArgStack.top().ArgVector;
		PutCode((tjs_int)vec.size(), LEX_POS); // count of arguments
		tjs_uint i;
		for(i=0; i<vec.size(); i++)
			PutCode(TJS_TO_VM_REG_ADDR(vec[i].Register), LEX_POS);
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::AddFunctionDeclArg(const tjs_char *varname, tTJSExprNode *node)
{
	// process the function argument of declaration
	// varname = argument name
	// init = initial expression

	Namespace.Add(varname);
//	AddLocalVariable(varname);

	if(node)
	{
		PutCode(VM_CDEQ, NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(-3 - FuncDeclArgCount), NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(0), NODE_POS);
		tjs_int jmp_ip = CodeAreaSize;
		AddJumpList();
		PutCode(VM_JNF), NODE_POS;
		PutCode(0, NODE_POS);

		tjs_int fr = FrameBase;
		tjs_int resaddr = GenNodeCode(fr, node, TJS_RT_NEEDED, 0, tSubParam());
		PutCode(VM_CP, NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(-3 - FuncDeclArgCount), NODE_POS);
		PutCode(TJS_TO_VM_REG_ADDR(resaddr), NODE_POS);
		ClearFrame(fr);

		CodeArea[jmp_ip+1] = CodeAreaSize - jmp_ip;

	}
	FuncDeclArgCount++;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::AddFunctionDeclArgCollapse(const tjs_char *varname)
{
	// process the function "collapse" argument of declaration.
	// collapse argument is available to receive arguments in array object form.

	if(varname == NULL)
	{
		// receive arguments in unnamed array
		FuncDeclUnnamedArgArrayBase = FuncDeclArgCount;
	}
	else
	{
		// receive arguments in named array
		FuncDeclCollapseBase = FuncDeclArgCount;
		Namespace.Add(varname);		
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::SetPropertyDeclArg(const tjs_char *varname)
{
	// process the setter argument

	Namespace.Add(varname);
	FuncDeclArgCount = 1;
}
//---------------------------------------------------------------------------
const tjs_char * tTJSInterCodeContext::GetName() const
{
	return Name;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::PushCurrentNode(tTJSExprNode *node)
{
	CurrentNodeVector.push_back(node);
}
//---------------------------------------------------------------------------
tTJSExprNode * tTJSInterCodeContext::GetCurrentNode()
{
	tjs_uint size = (tjs_uint)CurrentNodeVector.size();
	if(size == 0) return NULL;
	return CurrentNodeVector[size-1];
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::PopCurrentNode()
{
	CurrentNodeVector.pop_back();
}
//---------------------------------------------------------------------------
tTJSExprNode * tTJSInterCodeContext::MakeConstValNode(const tTJSVariant &val)
{
	tTJSExprNode * n = new tTJSExprNode;
	NodeToDeleteVector.push_back(n);
	n->SetOpecode(T_CONSTVAL);
	n->SetValue(val);
	n->SetPosition(LEX_POS);
	return n;
}
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint time_make_np = 0;
#endif

tTJSExprNode * tTJSInterCodeContext::MakeNP0(tjs_int opecode)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_make_np);
#endif
	tTJSExprNode * n = new tTJSExprNode;
	NodeToDeleteVector.push_back(n);
	n->SetOpecode(opecode);
	n->SetPosition(LEX_POS);
	return n;
}
//---------------------------------------------------------------------------
tTJSExprNode * tTJSInterCodeContext::MakeNP1(tjs_int opecode, tTJSExprNode * node1)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_make_np);
#endif

#ifndef TJS_NO_CONSTANT_FOLDING
	if(node1 && node1->GetOpecode() == T_CONSTVAL)
	{
		// constant folding
		tTJSExprNode * ret = NULL;

		switch(opecode)
		{
		case T_EXCRAMATION:
			ret = MakeConstValNode(!node1->GetValue());
			break;
		case T_TILDE:
			ret = MakeConstValNode(~node1->GetValue());
			break;
		case T_SHARP:
		  {
			tTJSVariant val(node1->GetValue());
			CharacterCodeOf(val);
			ret = MakeConstValNode(val);
		  }
			break;
		case T_DOLLAR:
		  {
			tTJSVariant val(node1->GetValue());
			CharacterCodeFrom(val);
			ret = MakeConstValNode(val);
		  }
			break;
		case T_UPLUS:
		  {
			tTJSVariant val(node1->GetValue());
			val.tonumber();
			ret = MakeConstValNode(val);
		  }
			break;
		case T_UMINUS:
		  {
			tTJSVariant val(node1->GetValue());
			val.changesign();
			ret = MakeConstValNode(val);
		  }
			break;
		case T_INT:
		  {
			tTJSVariant val(node1->GetValue());
			val.ToInteger();
			ret = MakeConstValNode(val);
		  }
			break;
		case T_REAL:
		  {
			tTJSVariant val(node1->GetValue());
			val.ToReal();
			ret = MakeConstValNode(val);
		  }
			break;
		case T_STRING:
		  {
			tTJSVariant val(node1->GetValue());
			val.ToString();
			ret = MakeConstValNode(val);
		  }
			break;
		case T_OCTET:
		  {
			tTJSVariant val(node1->GetValue());
			val.ToOctet();
			ret = MakeConstValNode(val);
		  }
			break;
		default: ;
		}
		if(ret)
		{
			node1->Clear(); // not to have data no longer
			return ret;
		}
	}
#endif

	tTJSExprNode * n = new tTJSExprNode;
	NodeToDeleteVector.push_back(n);
	n->SetOpecode(opecode);
	n->SetPosition(LEX_POS);
	n->Add(node1);
	return n;
}
//---------------------------------------------------------------------------
tTJSExprNode * tTJSInterCodeContext::MakeNP2(tjs_int opecode, tTJSExprNode * node1, tTJSExprNode * node2)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_make_np);
#endif

#ifndef TJS_NO_CONSTANT_FOLDING
	if(node1 && node1->GetOpecode() == T_CONSTVAL &&
		node2 && node2->GetOpecode() == T_CONSTVAL)
	{
		// constant folding
		switch(opecode)
		{
		case T_COMMA:
			return MakeConstValNode(node2->GetValue());
		case T_LOGICALOR:
			return MakeConstValNode(node1->GetValue() || node2->GetValue());
		case T_LOGICALAND:
			return MakeConstValNode(node1->GetValue() && node2->GetValue());
		case T_VERTLINE:
			return MakeConstValNode(node1->GetValue() | node2->GetValue());
		case T_CHEVRON:
			return MakeConstValNode(node1->GetValue() ^ node2->GetValue());
		case T_AMPERSAND:
			return MakeConstValNode(node1->GetValue() & node2->GetValue());
		case T_NOTEQUAL:
			return MakeConstValNode(node1->GetValue() != node2->GetValue());
		case T_EQUALEQUAL:
			return MakeConstValNode(node1->GetValue() == node2->GetValue());
		case T_DISCNOTEQUAL:
			return MakeConstValNode(!(node1->GetValue().DiscernCompare(node2->GetValue())));
		case T_DISCEQUAL:
			return MakeConstValNode( (node1->GetValue().DiscernCompare(node2->GetValue())));
		case T_LT:
			return MakeConstValNode(node1->GetValue() < node2->GetValue());
		case T_GT:
			return MakeConstValNode(node1->GetValue() > node2->GetValue());
		case T_LTOREQUAL:
			return MakeConstValNode(node1->GetValue() <= node2->GetValue());
		case T_GTOREQUAL:
			return MakeConstValNode(node1->GetValue() >= node2->GetValue());
		case T_RARITHSHIFT:
			return MakeConstValNode(node1->GetValue() >> node2->GetValue());
		case T_LARITHSHIFT:
			return MakeConstValNode(node1->GetValue() << node2->GetValue());
		case T_RBITSHIFT:
			return MakeConstValNode( (node1->GetValue().rbitshift(node2->GetValue())));
		case T_PLUS:
			return MakeConstValNode(node1->GetValue() + node2->GetValue());
		case T_MINUS:
			return MakeConstValNode(node1->GetValue() - node2->GetValue());
		case T_PERCENT:
			return MakeConstValNode(node1->GetValue() % node2->GetValue());
		case T_SLASH:
			return MakeConstValNode(node1->GetValue() / node2->GetValue());
		case T_BACKSLASH:
			return MakeConstValNode( (node1->GetValue().idiv(node2->GetValue())));
		case T_ASTERISK:
			return MakeConstValNode(node1->GetValue() * node2->GetValue());
		default: ;
		}
	}
#endif


	tTJSExprNode * n = new tTJSExprNode;
	NodeToDeleteVector.push_back(n);
	n->SetOpecode(opecode);
	n->SetPosition(LEX_POS);
	n->Add(node1);
	n->Add(node2);
	return n;
}
//---------------------------------------------------------------------------
tTJSExprNode * tTJSInterCodeContext::MakeNP3(tjs_int opecode, tTJSExprNode * node1, tTJSExprNode * node2,
	tTJSExprNode * node3)
{
#ifdef TJS_DEBUG_PROFILE_TIME
	tTJSTimeProfiler prof(time_make_np);
#endif
	tTJSExprNode * n = new tTJSExprNode;
	NodeToDeleteVector.push_back(n);
	n->SetOpecode(opecode);
	n->SetPosition(LEX_POS);
	n->Add(node1);
	n->Add(node2);
	n->Add(node3);
	return n;
}
//---------------------------------------------------------------------------

/**
 * バイトコードを出力する
 * @return
 */
std::vector<tjs_uint8>* tTJSInterCodeContext::ExportByteCode( bool outputdebug, tTJSScriptBlock *block, tjsConstArrayData& constarray )
{
	int parent = -1;
	if( Parent != NULL ) {
		parent = block->GetCodeIndex(Parent);
	}
	int propSetter = -1;
	if( PropSetter != NULL ) {
		propSetter = block->GetCodeIndex(PropSetter);
	}
	int propGetter = -1;
	if( PropGetter != NULL ) {
		propGetter = block->GetCodeIndex(PropGetter);
	}
	int superClassGetter = -1;
	if( SuperClassGetter != NULL ) {
		superClassGetter = block->GetCodeIndex(SuperClassGetter);
	}
	int name = -1;
	if( Name != NULL ) {
		name = constarray.PutString(Name);
	}
	// 13 * 4 データ部分のサイズ
	int count = 0;
	if (outputdebug) {
		count = SourcePosArraySize;
	}
	int srcpossize = count * 8;
	int codesize = (CodeAreaSize % 2) == 1 ? CodeAreaSize * 2 + 2 : CodeAreaSize * 2;
	int datasize = DataAreaSize * 4;
	int scgpsize = (int)(SuperClassGetterPointer.size() * 4);
	int propsize = (int)((Properties != NULL ? Properties->size() * 8 : 0)+4);
	int size = 12*4 + srcpossize + codesize + datasize + scgpsize + propsize + 4*4;
	std::vector<tjs_uint8>* result = new std::vector<tjs_uint8>();
	result->reserve( size );

	Add4ByteToVector( result, parent );
	Add4ByteToVector( result, name );
	Add4ByteToVector( result, ContextType );
	Add4ByteToVector( result, MaxVariableCount );
	Add4ByteToVector( result, VariableReserveCount );
	Add4ByteToVector( result, MaxFrameCount );
	Add4ByteToVector( result, FuncDeclArgCount );
	Add4ByteToVector( result, FuncDeclUnnamedArgArrayBase );
	Add4ByteToVector( result, FuncDeclCollapseBase );
	Add4ByteToVector( result, propSetter );
	Add4ByteToVector( result, propGetter );
	Add4ByteToVector( result, superClassGetter );

	Add4ByteToVector( result, count);
	if( outputdebug ) {
		for( int i = 0; i < count ; i++ ) {
			Add4ByteToVector( result, SourcePosArray[i].CodePos );
		}
		for( int i = 0; i < count ; i++ ) {
			Add4ByteToVector( result, SourcePosArray[i].SourcePos );
		}
	}

	count = CodeAreaSize;
	Add4ByteToVector( result, count);

	block->TranslateCodeAddress( CodeArea, CodeAreaSize );
	for( int i = 0; i < CodeAreaSize; i++ ) {
		Add2ByteToVector( result, CodeArea[i] );
	}
	if( (count%2) == 1 ) { // alignment
		Add2ByteToVector( result, 0 );
	}

	count = DataAreaSize;
	Add4ByteToVector( result, count);
	for( int i = 0; i < count ; i++ ) {
		tjs_int16 type = constarray.GetType( DataArea[i], block );
		tjs_int16 v = (tjs_int16)constarray.PutVariant( DataArea[i], block );
		Add2ByteToVector( result, type );
		Add2ByteToVector( result, v );
	}
	count = (int)SuperClassGetterPointer.size();
	Add4ByteToVector( result, count);
	for( int i = 0; i < count ; i++ ) {
		int v = SuperClassGetterPointer.at(i);
		Add4ByteToVector( result, v);
	}
	count = 0;
	if( Properties != NULL ) {
		count = (int)Properties->size();
		Add4ByteToVector( result, count);
		if( count > 0 ) {
			for( int i = 0; i < count; i++ ) {
				tProperty* prop = (*Properties).at(i);
				int propname = constarray.PutString(prop->Name);
				int propobj = -1;
				if( prop->Value != NULL ) {
					propobj = block->GetCodeIndex(prop->Value);
				}
				Add4ByteToVector( result, propname );
				Add4ByteToVector( result, propobj );
				delete prop;
			}
		}
		delete Properties;
	} else {
		Add4ByteToVector( result, count);
	}
	return result;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
} // namespace TJS


