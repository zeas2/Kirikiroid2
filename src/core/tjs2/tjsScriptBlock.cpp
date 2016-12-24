//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Script Block Management
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsScriptBlock.h"
#include "tjsInterCodeGen.h"
#include "tjsConstArrayData.h"
#include "tjs.h"

namespace TJS
{
//---------------------------------------------------------------------------
int yyparse(void*);
//---------------------------------------------------------------------------
// tTJSScriptBlock
//---------------------------------------------------------------------------
tTJSScriptBlock::tTJSScriptBlock(tTJS * owner)
{
	RefCount = 1;
	Owner = owner;
	Owner->AddRef();

	Script = NULL;
	Name = NULL;

	InterCodeContext = NULL;
	TopLevelContext = NULL;
	LexicalAnalyzer = NULL;

	UsingPreProcessor = false;

	LineOffset = 0;

	Owner->AddScriptBlock(this);
}
//---------------------------------------------------------------------------
// for Bytecode
tTJSScriptBlock::tTJSScriptBlock( tTJS* owner,  const tjs_char* name, tjs_int lineoffset ) {
	RefCount = 1;
	Owner = owner;
	Owner->AddRef();
	Name = NULL;
	if(name)
	{
		Name = new tjs_char[ TJS_strlen(name) + 1];
		TJS_strcpy(Name, name);
	}
	LineOffset = lineoffset;
	Script = NULL;

	InterCodeContext = NULL;
	TopLevelContext = NULL;
	LexicalAnalyzer = NULL;

	UsingPreProcessor = false;

	Owner->AddScriptBlock(this);
}
//---------------------------------------------------------------------------
tTJSScriptBlock::~tTJSScriptBlock()
{
	if(TopLevelContext) TopLevelContext->Release(), TopLevelContext = NULL;
	while(ContextStack.size())
	{
		ContextStack.top()->Release();
		ContextStack.pop();
	}

	Owner->RemoveScriptBlock(this);

	if(LexicalAnalyzer) delete LexicalAnalyzer;

	if(Script) delete [] Script;
	if(Name) delete [] Name;

	Owner->Release();
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::AddRef(void)
{
	RefCount ++;
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::Release(void)
{
	if(RefCount <= 1)
		delete this;
	else
		RefCount--;
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::SetName(const tjs_char *name, tjs_int lineofs)
{
	if(Name) delete [] Name, Name = NULL;
	if(name)
	{
		LineOffset = lineofs;
		Name = new tjs_char[ TJS_strlen(name) + 1];
		TJS_strcpy(Name, name);
	}
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::Add(tTJSInterCodeContext * cntx)
{
	InterCodeContextList.push_back(cntx);
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::Remove(tTJSInterCodeContext * cntx)
{
	InterCodeContextList.remove(cntx);
}
//---------------------------------------------------------------------------
tjs_uint tTJSScriptBlock::GetTotalVMCodeSize() const
{
	tjs_uint size = 0;

	std::list<tTJSInterCodeContext *>::const_iterator i;
	for(i = InterCodeContextList.begin(); i != InterCodeContextList.end(); i++)
	{
		size += (*i)->GetCodeSize();
	}
	return size;
}
//---------------------------------------------------------------------------
tjs_uint tTJSScriptBlock::GetTotalVMDataSize() const
{
	tjs_uint size = 0;

	std::list<tTJSInterCodeContext *>::const_iterator i;
	for(i = InterCodeContextList.begin(); i != InterCodeContextList.end(); i++)
	{
		size += (*i)->GetDataSize();
	}
	return size;
}
//---------------------------------------------------------------------------
const tjs_char * tTJSScriptBlock::GetLine(tjs_int line, tjs_int *linelength) const
{
	if( Script == NULL ) {
		*linelength = 10;
		return TJS_W("Bytecode.");
	}
	// note that this function DOES matter LineOffset
	line -= LineOffset;
	if(linelength) *linelength = LineLengthVector[line];
	return Script + LineVector[line];
}
//---------------------------------------------------------------------------
tjs_int tTJSScriptBlock::SrcPosToLine(tjs_int pos) const
{
	tjs_uint s = 0;
	tjs_uint e = (tjs_uint)LineVector.size();
	while(true)
	{
		if(e-s <= 1) return s + LineOffset; // LineOffset is added
		tjs_uint m = s + (e-s)/2;
		if(LineVector[m] > pos)
			e = m;
		else
			s = m;
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSScriptBlock::LineToSrcPos(tjs_int line) const
{
	// assumes line is added by LineOffset
	line -= LineOffset;
	return LineVector[line];
}
//---------------------------------------------------------------------------
ttstr tTJSScriptBlock::GetLineDescriptionString(tjs_int pos) const
{
	// get short description, like "mainwindow.tjs(321)"
	// pos is in character count from the first of the script
	tjs_int line =SrcPosToLine(pos)+1;
	ttstr name;
	if(Name)
	{
		name = Name;
	}
	else
	{
		tjs_char ptr[128];
		TJS_snprintf(ptr, sizeof(ptr)/sizeof(tjs_char), TJS_W("0x%p"), this);
		name = ttstr(TJS_W("anonymous@")) + ptr;
	}

	return name + TJS_W("(") + ttstr(line) + TJS_W(")");
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::ConsoleOutput(const tjs_char *msg, void *data)
{
	tTJSScriptBlock *blk = (tTJSScriptBlock*)data;
	blk->Owner->OutputToConsole(msg);
}
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint parsetime = 0;
extern tjs_uint time_make_np;
extern tjs_uint time_PutData;
extern tjs_uint time_PutCode;
extern tjs_uint time_this_proxy;
extern tjs_uint time_Commit;
extern tjs_uint time_yylex;
extern tjs_uint time_GenNodeCode;

extern tjs_uint time_ns_Push;
extern tjs_uint time_ns_Pop;
extern tjs_uint time_ns_Find;
extern tjs_uint time_ns_Add;
extern tjs_uint time_ns_Remove;
extern tjs_uint time_ns_Commit;

#endif

void tTJSScriptBlock::SetText(tTJSVariant *result, const tjs_char *text,
	iTJSDispatch2 * context, bool isexpression)
{
	TJS_F_TRACE("tTJSScriptBlock::SetText");


	// compiles text and executes its global level scripts.
	// the script will be compiled as an expression if isexpressn is true.
	if(!text) return;
	if(!text[0]) return;

	TJS_D((TJS_W("Counting lines ...\n")))

	Script = new tjs_char[TJS_strlen(text)+1];
	TJS_strcpy(Script, text);

	// calculation of line-count
	tjs_char *ls = Script;
	tjs_char *p = Script;
	while(*p)
	{
		if(*p == TJS_W('\r') || *p == TJS_W('\n'))
		{
			LineVector.push_back(int(ls - Script));
			LineLengthVector.push_back(int(p - ls));
			if(*p == TJS_W('\r') && p[1] == TJS_W('\n')) p++;
			p++;
			ls = p;
		}
		else
		{
			p++;
		}
	}

	if(p!=ls)
	{
		LineVector.push_back(int(ls - Script));
		LineLengthVector.push_back(int(p - ls));
	}

	try
	{

		// parse and execute
#ifdef TJS_DEBUG_PROFILE_TIME
		{
		tTJSTimeProfiler p(parsetime);
#endif

		Parse(text, isexpression, result != NULL);

#ifdef TJS_DEBUG_PROFILE_TIME
		}

		{
			char buf[256];
			TJS_nsprintf(buf, "parsing : %d", parsetime);
			OutputDebugString(buf);
			if(parsetime)
			{
			TJS_nsprintf(buf, "Commit : %d (%d%%)", time_Commit, time_Commit*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "yylex : %d (%d%%)", time_yylex, time_yylex*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "MakeNP : %d (%d%%)", time_make_np, time_make_np*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "GenNodeCode : %d (%d%%)", time_GenNodeCode, time_GenNodeCode*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "  PutCode : %d (%d%%)", time_PutCode, time_PutCode*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "  PutData : %d (%d%%)", time_PutData, time_PutData*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "  this_proxy : %d (%d%%)", time_this_proxy, time_this_proxy*100/parsetime);
			OutputDebugString(buf);

			TJS_nsprintf(buf, "ns::Push : %d (%d%%)", time_ns_Push, time_ns_Push*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "ns::Pop : %d (%d%%)", time_ns_Pop, time_ns_Pop*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "ns::Find : %d (%d%%)", time_ns_Find, time_ns_Find*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "ns::Remove : %d (%d%%)", time_ns_Remove, time_ns_Remove*100/parsetime);
			OutputDebugString(buf);
			TJS_nsprintf(buf, "ns::Commit : %d (%d%%)", time_ns_Commit, time_ns_Commit*100/parsetime);
			OutputDebugString(buf);

			}
		}
#endif

#ifdef TJS_DEBUG_DISASM
		std::list<tTJSInterCodeContext *>::iterator i =
			InterCodeContextList.begin();
		while(i != InterCodeContextList.end())
		{
			ConsoleOutput(TJS_W(""), (void*)this);
			ConsoleOutput((*i)->GetName(), (void*)this);
			(*i)->Disassemble(ConsoleOutput, (void*)this);
			i++;
		}
#endif

		// execute global level script
		ExecuteTopLevelScript(result, context);
	}
	catch(...)
	{
		if(InterCodeContextList.size() != 1)
		{
			if(TopLevelContext) TopLevelContext->Release(), TopLevelContext = NULL;
			while(ContextStack.size())
			{
				ContextStack.top()->Release();
				ContextStack.pop();
			}
		}
		throw;
	}

	if(InterCodeContextList.size() != 1)
	{
		// this is not a single-context script block
		// (may hook itself)
		// release all contexts and global at this time
		if(TopLevelContext) TopLevelContext->Release(), TopLevelContext = NULL;
		while(ContextStack.size())
		{
			ContextStack.top()->Release();
			ContextStack.pop();
		}
	}
}
//---------------------------------------------------------------------------
// for Bytecode
void tTJSScriptBlock::ExecuteTopLevel( tTJSVariant *result, iTJSDispatch2 * context )
{
	try {
#ifdef TJS_DEBUG_DISASM
		std::list<tTJSInterCodeContext *>::iterator i = InterCodeContextList.begin();
		while(i != InterCodeContextList.end())
		{
			ConsoleOutput(TJS_W(""), (void*)this);
			ConsoleOutput((*i)->GetName(), (void*)this);
			(*i)->Disassemble(ConsoleOutput, (void*)this);
			i++;
		}
#endif

		// execute global level script
		ExecuteTopLevelScript(result, context);
	} catch(...) {
		if(InterCodeContextList.size() != 1) {
			if(TopLevelContext) TopLevelContext->Release(), TopLevelContext = NULL;
			while(ContextStack.size()) {
				ContextStack.top()->Release();
				ContextStack.pop();
			}
		}
		throw;
	}

	if(InterCodeContextList.size() != 1) {
		// this is not a single-context script block
		// (may hook itself)
		// release all contexts and global at this time
		if(TopLevelContext) TopLevelContext->Release(), TopLevelContext = NULL;
		while(ContextStack.size()) {
			ContextStack.top()->Release();
			ContextStack.pop();
		}
	}
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::ExecuteTopLevelScript(tTJSVariant *result,
	iTJSDispatch2 * context)
{
	if(TopLevelContext)
	{
#ifdef TJS_DEBUG_PROFILE_TIME
		clock_t start = clock();
#endif
		TopLevelContext->FuncCall(0, NULL, NULL, result, 0, NULL, context);
#ifdef TJS_DEBUG_PROFILE_TIME
		tjs_char str[100];
		TJS_sprintf(str, TJS_W("%d"), clock() - start);
		ConsoleOutput(str, (void*)this);
#endif
	}
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::PushContextStack(const tjs_char *name, tTJSContextType type)
{
	tTJSInterCodeContext *cntx;
	cntx = new tTJSInterCodeContext(InterCodeContext, name, this, type);
	if(InterCodeContext==NULL)
	{
		if(TopLevelContext) TJS_eTJSError(TJSInternalError);
		TopLevelContext = cntx;
		TopLevelContext->AddRef();
	}
	ContextStack.push(cntx);
	InterCodeContext = cntx;
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::PopContextStack(void)
{
	InterCodeContext->Commit();
	InterCodeContext->Release();
	ContextStack.pop();
	if(ContextStack.size() >= 1)
		InterCodeContext = ContextStack.top();
	else
		InterCodeContext = NULL;
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::Parse(const tjs_char *script, bool isexpr, bool resultneeded)
{
	TJS_F_TRACE("tTJSScriptBlock::Parse");

	if(!script) return;

	CompileErrorCount = 0;


	LexicalAnalyzer = new tTJSLexicalAnalyzer(this, script, isexpr, resultneeded);

	try
	{
		yyparse(this);
	}
	catch(...)
	{
		delete LexicalAnalyzer; LexicalAnalyzer=NULL;
		throw;
	}

	delete LexicalAnalyzer; LexicalAnalyzer=NULL;

	if(CompileErrorCount)
	{
		TJS_eTJSScriptError(FirstError, this, FirstErrorPos);
	}
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::SetFirstError(const tjs_char *error, tjs_int pos)
{
	if(CompileErrorCount == 0)
	{
		FirstError = error;
		FirstErrorPos = pos;
	}
}
//---------------------------------------------------------------------------
ttstr tTJSScriptBlock::GetNameInfo() const
{
	if(LineOffset == 0)
	{
		return ttstr(Name);
	}
	else
	{
		return ttstr(Name) + TJS_W("(line +") + ttstr(LineOffset) + TJS_W(")");
	}
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::Dump() const
{
	std::list<tTJSInterCodeContext *>::const_iterator i =
		InterCodeContextList.begin();
	while(i != InterCodeContextList.end())
	{
		ConsoleOutput(TJS_W(""), (void*)this);
		tjs_char ptr[256];
		TJS_snprintf(ptr, sizeof(ptr)/sizeof(tjs_char), TJS_W(" 0x%p"), (*i));
		ConsoleOutput((ttstr(TJS_W("(")) + ttstr((*i)->GetContextTypeName()) +
			TJS_W(") ") + ttstr((*i)->GetName()) + ptr).c_str(), (void*)this);
		(*i)->Disassemble(ConsoleOutput, (void*)this);
		i++;
	}
}
//---------------------------------------------------------------------------
// for Bytecode
tjs_int tTJSScriptBlock::GetCodeIndex( const tTJSInterCodeContext* ctx ) const {
	tjs_int index = 0;
	for( std::list<tTJSInterCodeContext *>::const_iterator i = InterCodeContextList.begin(); i != InterCodeContextList.end(); i++, index++ ) {
		if( (*i) == ctx ) {
			return index;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
const tjs_uint8 tTJSScriptBlock::BYTECODE_FILE_TAG[BYTECODE_FILE_TAG_SIZE] = { 'T', 'J', 'S', '2', '1', '0', '0', 0 };
const tjs_uint8 tTJSScriptBlock::BYTECODE_CODE_TAG[BYTECODE_TAG_SIZE] = { 'T', 'J', 'S', '2' };
const tjs_uint8 tTJSScriptBlock::BYTECODE_OBJ_TAG[BYTECODE_TAG_SIZE] = { 'O', 'B', 'J', 'S' };
const tjs_uint8 tTJSScriptBlock::BYTECODE_DATA_TAG[BYTECODE_TAG_SIZE] = { 'D', 'A', 'T', 'A' };
//---------------------------------------------------------------------------
void tTJSScriptBlock::ExportByteCode( bool outputdebug, tTJSBinaryStream* output ) {
	const int count = (int)InterCodeContextList.size();
	std::vector<std::vector<tjs_uint8>* > objarray;
	objarray.reserve( count * 2 );
	tjsConstArrayData* constarray = new tjsConstArrayData();
	int objsize = 0;
	for( std::list<tTJSInterCodeContext *>::const_iterator i = InterCodeContextList.begin(); i != InterCodeContextList.end(); i++ ) {
		tTJSInterCodeContext* obj = (*i);
		std::vector<tjs_uint8>* buf = obj->ExportByteCode( outputdebug, this, *constarray );
		objarray.push_back( buf );
		objsize += (int)buf->size() + BYTECODE_TAG_SIZE + BYTECODE_CHUNK_SIZE_LEN; // tag + size
	}

	objsize += BYTECODE_TAG_SIZE + BYTECODE_CHUNK_SIZE_LEN + 4 + 4; // OBJS tag + size + toplevel + count
	std::vector<tjs_uint8>* dataarea = constarray->ExportBuffer();
	int datasize = (int)dataarea->size() + BYTECODE_TAG_SIZE + BYTECODE_CHUNK_SIZE_LEN; // DATA tag + size
	int filesize = objsize + datasize + BYTECODE_FILE_TAG_SIZE + BYTECODE_CHUNK_SIZE_LEN; // TJS2 tag + file size
	int toplevel = -1;
	if( TopLevelContext != NULL ) {
		toplevel = GetCodeIndex( TopLevelContext );
	}
	tjs_uint8 tmp[4];
	output->Write( BYTECODE_FILE_TAG, 8 );
	Write4Byte( tmp, filesize );
	output->Write( tmp, 4 );
	output->Write( BYTECODE_DATA_TAG, 4 );
	Write4Byte( tmp, datasize );
	output->Write( tmp, 4 );
	output->Write( &((*dataarea)[0]), (tjs_uint)dataarea->size() );
	output->Write( BYTECODE_OBJ_TAG, 4 );
	Write4Byte( tmp, objsize );
	output->Write( tmp, 4 );
	Write4Byte( tmp, toplevel );
	output->Write( tmp, 4 );
	Write4Byte( tmp, count );
	output->Write( tmp, 4 );
	for( int i = 0; i < count; i++ ) {
		std::vector<tjs_uint8>* buf = objarray[i];
		int size = (int)buf->size();
		output->Write( BYTECODE_CODE_TAG, 4 );
		Write4Byte( tmp, size );
		output->Write( tmp, 4 );
		output->Write( &((*buf)[0]), size );
		delete buf;
		objarray[i] = NULL;
	}
	objarray.clear();
	delete constarray;
	delete dataarea;
}
//---------------------------------------------------------------------------
void tTJSScriptBlock::Compile( const tjs_char *text, bool isexpression, bool isresultneeded, bool outputdebug, tTJSBinaryStream* output ) {
	if(!text) return;
	if(!text[0]) return;

	tTJSInterCodeContext::IsBytecodeCompile = true;
	try {
		Script = new tjs_char[TJS_strlen(text)+1];
		TJS_strcpy(Script, text);

		// calculation of line-count
		tjs_char *ls = Script;
		tjs_char *p = Script;
		while( *p ) {
			if(*p == TJS_W('\r') || *p == TJS_W('\n')) {
				LineVector.push_back(int(ls - Script));
				LineLengthVector.push_back(int(p - ls));
				if(*p == TJS_W('\r') && p[1] == TJS_W('\n')) p++;
				p++;
				ls = p;
			} else {
				p++;
			}
		}
		if( p != ls ) {
			LineVector.push_back(int(ls - Script));
			LineLengthVector.push_back(int(p - ls));
		}

		Parse( text, isexpression, isresultneeded );

		ExportByteCode( outputdebug, output );
	} catch(...) {
		if(InterCodeContextList.size() != 1) {
			if( TopLevelContext ) TopLevelContext->Release(), TopLevelContext = NULL;
			while( ContextStack.size() ) {
				ContextStack.top()->Release();
				ContextStack.pop();
			}
		}
		tTJSInterCodeContext::IsBytecodeCompile = false;
		throw;
	}
	if(InterCodeContextList.size() != 1) {
		if( TopLevelContext ) TopLevelContext->Release(), TopLevelContext = NULL;
		while( ContextStack.size() ) {
			ContextStack.top()->Release();
			ContextStack.pop();
		}
	}
	tTJSInterCodeContext::IsBytecodeCompile = false;
}
//---------------------------------------------------------------------------

#define TJS_OFFSET_VM_REG_ADDR( x ) ( (x) = TJS_FROM_VM_REG_ADDR(x) )
#define TJS_OFFSET_VM_CODE_ADDR( x ) ( (x) = TJS_FROM_VM_CODE_ADDR(x) )
/**
 * バイトコード中のアドレスは配列のインデックスを指すので、それに合わせて変換
 */
void tTJSScriptBlock::TranslateCodeAddress( tjs_int32* code, const tjs_int32 codeSize )
{
	tjs_int i = 0;
	for( ; i < codeSize; ) {
		tjs_int size;
		switch( code[i] ) {
		case VM_NOP: size = 1; break;
		case VM_NF: size = 1; break;
		case VM_CONST:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			size = 3;
			break;

#define OP2_DISASM(c) \
	case c: \
		TJS_OFFSET_VM_REG_ADDR(code[i+1]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+2]); \
		size = 3; \
		break

		OP2_DISASM(VM_CP);
		OP2_DISASM(VM_CEQ);
		OP2_DISASM(VM_CDEQ);
		OP2_DISASM(VM_CLT);
		OP2_DISASM(VM_CGT);
		OP2_DISASM(VM_CHKINS);
#undef OP2_DISASM

#define OP2_DISASM(c) \
	case c: \
		TJS_OFFSET_VM_REG_ADDR(code[i+1]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+2]); \
		size = 3; \
		break; \
	case c+1: \
		TJS_OFFSET_VM_REG_ADDR(code[i+1]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+2]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+3]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+4]); \
		size = 5; \
		break; \
	case c+2: \
		TJS_OFFSET_VM_REG_ADDR(code[i+1]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+2]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+3]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+4]); \
		size = 5; \
		break; \
	case c+3: \
		TJS_OFFSET_VM_REG_ADDR(code[i+1]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+2]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+3]); \
		size = 4; \
		break

		OP2_DISASM(VM_LOR);
		OP2_DISASM(VM_LAND);
		OP2_DISASM(VM_BOR);
		OP2_DISASM(VM_BXOR);
		OP2_DISASM(VM_BAND);
		OP2_DISASM(VM_SAR);
		OP2_DISASM(VM_SAL);
		OP2_DISASM(VM_SR);
		OP2_DISASM(VM_ADD);
		OP2_DISASM(VM_SUB);
		OP2_DISASM(VM_MOD);
		OP2_DISASM(VM_DIV);
		OP2_DISASM(VM_IDIV);
		OP2_DISASM(VM_MUL);
#undef OP2_DISASM

#define OP1_DISASM TJS_OFFSET_VM_REG_ADDR(code[i+1]); size = 2;
		case VM_TT:			OP1_DISASM;	break;
		case VM_TF:			OP1_DISASM;	break;
		case VM_SETF:		OP1_DISASM;	break;
		case VM_SETNF:		OP1_DISASM;	break;
		case VM_LNOT:		OP1_DISASM;	break;
		case VM_BNOT:		OP1_DISASM;	break;
		case VM_ASC:		OP1_DISASM;	break;
		case VM_CHR:		OP1_DISASM;	break;
		case VM_NUM:		OP1_DISASM;	break;
		case VM_CHS:		OP1_DISASM;	break;
		case VM_CL:			OP1_DISASM;	break;
		case VM_INV:		OP1_DISASM;	break;
		case VM_CHKINV:		OP1_DISASM;	break;
		case VM_TYPEOF:		OP1_DISASM;	break;
		case VM_EVAL:		OP1_DISASM;	break;
		case VM_EEXP:		OP1_DISASM;	break;
		case VM_INT:		OP1_DISASM;	break;
		case VM_REAL:		OP1_DISASM;	break;
		case VM_STR:		OP1_DISASM;	break;
		case VM_OCTET:		OP1_DISASM;	break;
#undef OP1_DISASM

		case VM_CCL:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			size = 3;
			break;

#define OP1_DISASM(c) \
	case c: \
		TJS_OFFSET_VM_REG_ADDR(code[i+1]); \
		size = 2; \
		break; \
	case c+1: \
		TJS_OFFSET_VM_REG_ADDR(code[i+1]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+2]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+3]); \
		size = 4; \
		break; \
	case c+2: \
		TJS_OFFSET_VM_REG_ADDR(code[i+1]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+2]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+3]); \
		size = 4; \
		break; \
	case c+3: \
		TJS_OFFSET_VM_REG_ADDR(code[i+1]); \
		TJS_OFFSET_VM_REG_ADDR(code[i+2]); \
		size = 3; \
		break

		OP1_DISASM(VM_INC);
		OP1_DISASM(VM_DEC);
#undef OP1_DISASM

#define OP1A_DISASM TJS_OFFSET_VM_CODE_ADDR(code[i+1]); size = 2;
		case VM_JF:	OP1A_DISASM; break;
		case VM_JNF:OP1A_DISASM; break;
		case VM_JMP:OP1A_DISASM; break;
#undef OP1A_DISASM

		case VM_CALL:
		case VM_CALLD:
		case VM_CALLI:
		case VM_NEW:
		  {
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);

			tjs_int st; // start of arguments
			if(code[i] == VM_CALLD || code[i] == VM_CALLI) {
				TJS_OFFSET_VM_REG_ADDR(code[i+3]);
				st = 5;
			} else {
				st = 4;
			}
			tjs_int num = code[i+st-1];     // st-1 = argument count
			tjs_int c = 0;
			if(num == -1) {
				size = st;
			} else if(num == -2) {
				st++;
				num = code[i+st-1];
				size = st + num * 2;
				for(tjs_int j = 0; j < num; j++) {
					switch(code[i+st+j*2]) {
					case fatNormal:
						TJS_OFFSET_VM_REG_ADDR(code[i+st+j*2+1]);
						break;
					case fatExpand:
						TJS_OFFSET_VM_REG_ADDR(code[i+st+j*2+1]);
						break;
					case fatUnnamedExpand:
						break;
					}
				}
			} else {
				// normal operation
				size = st + num;
				while(num--) {
					TJS_OFFSET_VM_REG_ADDR(code[i+c+st]);
					c++;
				}
			}
			break;
		  }

		case VM_GPD:
		case VM_GPDS:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			TJS_OFFSET_VM_REG_ADDR(code[i+3]);
			size = 4;
			break;

		case VM_SPD:
		case VM_SPDE:
		case VM_SPDEH:
		case VM_SPDS:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			TJS_OFFSET_VM_REG_ADDR(code[i+3]);
			size = 4;
			break;

		case VM_GPI:
		case VM_GPIS:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			TJS_OFFSET_VM_REG_ADDR(code[i+3]);
			size = 4;
			break;

		case VM_SPI:
		case VM_SPIE:
		case VM_SPIS:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			TJS_OFFSET_VM_REG_ADDR(code[i+3]);
			size = 4;
			break;

		case VM_SETP:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			size = 3;
			break;

		case VM_GETP:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			size = 3;
			break;

		case VM_DELD:
		case VM_TYPEOFD:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			TJS_OFFSET_VM_REG_ADDR(code[i+3]);
			size = 4;
			break;

		case VM_DELI:
		case VM_TYPEOFI:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			TJS_OFFSET_VM_REG_ADDR(code[i+3]);
			size = 4;
			break;

		case VM_SRV:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			size = 2;
			break;

		case VM_RET: size = 1; break;

		case VM_ENTRY:
			TJS_OFFSET_VM_CODE_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			size = 3;
			break;

		case VM_EXTRY: size = 1; break;

		case VM_THROW:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			size = 2;
			break;

		case VM_CHGTHIS:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			size = 3;
			break;

		case VM_GLOBAL:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			size = 2;
			break;

		case VM_ADDCI:
			TJS_OFFSET_VM_REG_ADDR(code[i+1]);
			TJS_OFFSET_VM_REG_ADDR(code[i+2]);
			size = 3;
			break;

		case VM_REGMEMBER: size = 1; break;
		case VM_DEBUGGER: size = 1; break;
		default: size = 1; break;
		} /* switch */
		i+=size;
	}
	if( codeSize != i ) {
		TJS_eTJSScriptError( TJSInternalError, this, 0 );
	}
}
#undef TJS_OFFSET_VM_REG_ADDR
#undef TJS_OFFSET_VM_CODE_ADDR
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

} // namespace



