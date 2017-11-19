//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Intermediate Code Context
//---------------------------------------------------------------------------
#ifndef tjsInterCodeGenH
#define tjsInterCodeGenH


#include <vector>
#include <stack>
#include <list>
#include "tjsVariant.h"
#include "tjsInterface.h"
#include "tjsNamespace.h"
#include "tjsError.h"
#include "tjsObject.h"

#ifdef ENABLE_DEBUGGER
#include "tjsDebug.h"
#endif // ENABLE_DEBUGGER


namespace TJS
{
//---------------------------------------------------------------------------
//
#define TJS_TO_VM_CODE_ADDR(x)  ((x) * (tjs_int)sizeof(tjs_uint32))
#define TJS_TO_VM_REG_ADDR(x) ((x) * (tjs_int)sizeof(tTJSVariant))
#define TJS_FROM_VM_CODE_ADDR(x)  ((tjs_int)(x) / (tjs_int)sizeof(tjs_uint32))
#define TJS_FROM_VM_REG_ADDR(x) ((tjs_int)(x) / (tjs_int)sizeof(tTJSVariant))
#define TJS_ADD_VM_CODE_ADDR(dest, x)  ((*(char **)&(dest)) += (x))
#define TJS_GET_VM_REG_ADDR(base, x) ((tTJSVariant*)((char *)(base) + (tjs_int)(x)))
#define TJS_GET_VM_REG(base, x) (*(TJS_GET_VM_REG_ADDR(base, x)))

//---------------------------------------------------------------------------
extern int _yyerror(const tjs_char * msg, void *pm, tjs_int pos = -1);
//---------------------------------------------------------------------------
#define TJS_NORMAL_AND_PROPERTY_ACCESSER(x) x, x##PD, x##PI, x##P

enum tTJSVMCodes{

	VM_NOP, VM_CONST, VM_CP, VM_CL, VM_CCL, VM_TT, VM_TF, VM_CEQ, VM_CDEQ, VM_CLT,
	VM_CGT, VM_SETF, VM_SETNF, VM_LNOT, VM_NF, VM_JF, VM_JNF, VM_JMP,

	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_INC),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_DEC),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_LOR),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_LAND),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_BOR),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_BXOR),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_BAND),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_SAR),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_SAL),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_SR),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_ADD),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_SUB),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_MOD),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_DIV),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_IDIV),
	TJS_NORMAL_AND_PROPERTY_ACCESSER(VM_MUL),

	VM_BNOT, VM_TYPEOF, VM_TYPEOFD, VM_TYPEOFI, VM_EVAL, VM_EEXP, VM_CHKINS,
	VM_ASC, VM_CHR, VM_NUM, VM_CHS, VM_INV, VM_CHKINV,
	VM_INT, VM_REAL, VM_STR, VM_OCTET,
	VM_CALL, VM_CALLD, VM_CALLI, VM_NEW,
	VM_GPD, VM_SPD, VM_SPDE, VM_SPDEH, VM_GPI, VM_SPI, VM_SPIE,
	VM_GPDS, VM_SPDS, VM_GPIS, VM_SPIS,  VM_SETP, VM_GETP,
	VM_DELD, VM_DELI, VM_SRV, VM_RET, VM_ENTRY, VM_EXTRY, VM_THROW,
	VM_CHGTHIS, VM_GLOBAL, VM_ADDCI, VM_REGMEMBER, VM_DEBUGGER,

	__VM_LAST /* = last mark ; this is not a real operation code */} ;

#undef TJS_NORMAL_AND_PROPERTY_ACCESSER
//---------------------------------------------------------------------------
enum tTJSSubType{ stNone=VM_NOP, stEqual=VM_CP, stBitAND=VM_BAND, stBitOR=VM_BOR,
	stBitXOR=VM_BXOR, stSub=VM_SUB, stAdd=VM_ADD, stMod=VM_MOD, stDiv=VM_DIV,
	stIDiv = VM_IDIV,
	stMul=VM_MUL, stLogOR=VM_LOR, stLogAND=VM_LAND, stSAR=VM_SAR, stSAL=VM_SAL,
	stSR=VM_SR,

	stPreInc = __VM_LAST, stPreDec, stPostInc, stPostDec, stDelete, stFuncCall,
		stIgnorePropGet, stIgnorePropSet, stTypeOf} ;
//---------------------------------------------------------------------------
enum tTJSFuncArgType { fatNormal, fatExpand, fatUnnamedExpand };
//---------------------------------------------------------------------------
enum tTJSContextType
{
	ctTopLevel,
	ctFunction,
	ctExprFunction,
	ctProperty,
	ctPropertySetter,
	ctPropertyGetter,
	ctClass,
	ctSuperClassGetter,
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// tTJSExprNode
//---------------------------------------------------------------------------
class tTJSExprNode
{
	tjs_int Op;
	tjs_int Position;
	std::vector<tTJSExprNode*> *Nodes;
	tTJSVariant *Val;

public:
	tTJSExprNode();
	~tTJSExprNode() { Clear(); }

	void Clear();

	void SetOpecode(tjs_int op) { Op = op; }
	void SetPosition(tjs_int pos) { Position = pos; }
	void SetValue(const tTJSVariant &val)
	{
		if(!Val)
			Val = new tTJSVariant(val);
		else
			Val->CopyRef(val);
	}
	void Add(tTJSExprNode *n);

	tjs_int GetOpecode() const { return Op; }
	tjs_int GetPosition() const { return Position; }
	tTJSVariant & GetValue() { if(!Val) return *(tTJSVariant*)NULL; else return *Val; }
	tTJSExprNode * operator [] (tjs_int i) const { if(!Nodes) return NULL; else return (*Nodes)[i]; }
	unsigned int GetSize() const { if(Nodes) return (unsigned int)Nodes->size(); else return 0;}

	// for array and dictionary constant value
	void AddArrayElement(const tTJSVariant & val);
	void AddDictionaryElement(const tTJSString & name, const tTJSVariant & val);
};
//---------------------------------------------------------------------------
// tTJSInterCodeContext - Intermediate Code Context
//---------------------------------------------------------------------------
/*
	this class implements iTJSDispatch2;
	the object can be directly treated as function, class, property handlers.
*/

class tTJSScriptBlock;
class tTJSInterCodeContext : public tTJSCustomObject
{
	typedef tTJSCustomObject inherited;

public:
	tTJSInterCodeContext(tTJSInterCodeContext *parant,
		const tjs_char *name, tTJSScriptBlock *block, tTJSContextType type);
	virtual ~tTJSInterCodeContext();

	// is bytecode export
	static bool IsBytecodeCompile;
protected:
	void Finalize(void);
	//------------------------------------------------------- compiling stuff

public:
	void ClearNodesToDelete(void);

public:
	struct tSubParam
	{
		tTJSSubType SubType;
		tjs_int SubFlag;
		tjs_int SubAddress; 

		tSubParam()
			{ SubType = stNone, SubFlag = 0, SubAddress = 0; }
	};
	struct tSourcePos
	{
		tjs_int CodePos;
		tjs_int SourcePos;
		static int TJS_USERENTRY
			SortFunction(const void *a, const void *b);
	};

private:

	enum tNestType { ntBlock, ntWhile, ntDoWhile, ntFor,
		ntSwitch, ntIf, ntElse, ntTry, ntCatch, ntWith };

	struct tNestData
	{
		tNestType Type;
		tjs_int VariableCount;
		union
		{
			bool VariableCreated;
			bool IsFirstCase;
		};
		tjs_int RefRegister;
		tjs_int StartIP;
		tjs_int LoopStartIP;
		std::vector<tjs_int> ContinuePatchVector;
		std::vector<tjs_int> ExitPatchVector;
		tjs_int Patch1;
		tjs_int Patch2;
		tTJSExprNode *PostLoopExpr;
	};

	struct tFixData
	{
		tjs_int StartIP;
		tjs_int Size;
		tjs_int NewSize;
		bool BeforeInsertion;
		tjs_int32 *Code;

		tFixData(tjs_int startip, tjs_int size, tjs_int newsize,
			tjs_int32 *code, bool beforeinsertion)
			{ StartIP =startip, Size = size, NewSize = newsize,
				Code = code, BeforeInsertion = beforeinsertion; }
		tFixData(const tFixData &fixdata)
			{
				Code = NULL;
				operator =(fixdata);
			}
		tFixData & operator = (const tFixData &fixdata)
			{
				if(Code) delete [] Code;
				StartIP = fixdata.StartIP;
				Size = fixdata.Size;
				NewSize = fixdata.NewSize;
				BeforeInsertion = fixdata.BeforeInsertion;
				Code = new tjs_int32[NewSize];
				memcpy(Code, fixdata.Code, sizeof(tjs_int32)*NewSize);
				return *this;
			}
		~tFixData() { if(Code) delete [] Code; }
	};

	struct tNonLocalFunctionDecl
	{
		tjs_int DataPos;
		tjs_int NameDataPos;
		bool ChangeThis;
		tNonLocalFunctionDecl(tjs_int datapos, tjs_int namedatapos, bool changethis)
			{ DataPos = datapos, NameDataPos = namedatapos; ChangeThis = changethis; }
	};

	struct tFuncArgItem
	{
		tjs_int Register;
		tTJSFuncArgType Type;
		tFuncArgItem(tjs_int reg, tTJSFuncArgType type = fatNormal)
		{
			Register = reg;
			Type = type;
		}
	};

	struct tFuncArg
	{
		bool IsOmit;
		bool HasExpand;
		std::vector <tFuncArgItem> ArgVector;
		tFuncArg() { IsOmit = HasExpand = false; }
	};

	struct tArrayArg
	{
		tjs_int Object;
		tjs_int Counter;
	};
	
	// for Bytecode
	struct tProperty {
		const tjs_char* Name;
		const tTJSInterCodeContext* Value;
		tProperty( const tjs_char* name, const tTJSInterCodeContext* val ) {
			Name = name;
			Value = val;
		}
	};
	std::vector<tProperty*>* Properties;

	tjs_int FrameBase;

	tjs_int32 * CodeArea;
	tjs_int CodeAreaCapa;
	tjs_int CodeAreaSize;

	tTJSVariant ** _DataArea;
	tjs_int _DataAreaSize;
	tjs_int _DataAreaCapa;
	tTJSVariant * DataArea;
	tjs_int DataAreaSize;

	tTJSLocalNamespace Namespace;

	std::vector<tTJSExprNode *> NodeToDeleteVector;

	std::vector<tTJSExprNode *> CurrentNodeVector;

	std::stack<tFuncArg> FuncArgStack;

	std::stack<tArrayArg> ArrayArgStack;

	tjs_int PrevIfExitPatch;
	std::vector<tNestData> NestVector;


	std::list<tjs_int> JumpList;
	std::list<tFixData> FixList;

	std::vector<tNonLocalFunctionDecl> NonLocalFunctionDeclVector;

	tjs_int FunctionRegisterCodePoint;

	tjs_int PrevSourcePos;
	bool SourcePosArraySorted;
//	std::vector<tSourcePos> SourcePosVector;
	tSourcePos *SourcePosArray;
	tjs_int SourcePosArraySize;
	tjs_int SourcePosArrayCapa;

	tTJSExprNode *SuperClassExpr;

	tjs_int VariableReserveCount;

	bool AsGlobalContextMode;

	tjs_int MaxFrameCount;
	tjs_int MaxVariableCount;

	tjs_int FuncDeclArgCount;
	tjs_int FuncDeclUnnamedArgArrayBase;
	tjs_int FuncDeclCollapseBase;

	std::vector<tjs_int> SuperClassGetterPointer;

	tjs_char *Name;
	tTJSInterCodeContext *Parent;
	tTJSScriptBlock *Block;
	tTJSContextType ContextType;
	tTJSInterCodeContext *PropSetter;
	tTJSInterCodeContext *PropGetter;
	tTJSInterCodeContext *SuperClassGetter;

#ifdef ENABLE_DEBUGGER
	ScopeKey		DebuggerScopeKey;		//!< for exec
	tTJSVariant*	DebuggerRegisterArea;	//!< for exec
#endif	// ENABLE_DEBUGGER

public:
	tTJSContextType GetContextType() const { return ContextType; }
	const tjs_char *GetContextTypeName() const;

	ttstr GetShortDescription() const;
	ttstr GetShortDescriptionWithClassName() const;

	tTJSScriptBlock * GetBlock() const { return Block; }

#ifdef ENABLE_DEBUGGER
	ttstr GetClassName() const;
	ttstr GetSelfClassName() const;

	const ScopeKey& GetDebuggerScopeKey() { return DebuggerScopeKey; }
	tTJSVariant* GetDebuggerRegisterArea() { return DebuggerRegisterArea; }
	tTJSVariant* GetDebuggerDataArea() { return DataArea; }
#endif	// ENABLE_DEBUGGER
private:
	void OutputWarning(const tjs_char *msg, tjs_int pos = -1);

private:
	tjs_int PutCode(tjs_int32 num, tjs_int32 pos=-1);
	tjs_int PutData(const tTJSVariant &val);

	void AddJumpList(void) { JumpList.push_back(CodeAreaSize); }

	void SortSourcePos();

	void FixCode(void);
	void RegisterFunction();

	tjs_int _GenNodeCode(tjs_int & frame, tTJSExprNode *node, tjs_uint32 restype,
		tjs_int reqresaddr, const tSubParam & param);
	tjs_int GenNodeCode(tjs_int & frame, tTJSExprNode *node, tjs_uint32 restype,
		tjs_int reqresaddr, const tSubParam & param);

	// restypes
	#define TJS_RT_NEEDED 0x0001   // result needed
	#define TJS_RT_CFLAG  0x0002   // condition flag needed
	// result value
	#define TJS_GNC_CFLAG (1<<(sizeof(tjs_int)*8-1)) // true logic
	#define TJS_GNC_CFLAG_I (TJS_GNC_CFLAG+1) // inverted logic


	void StartFuncArg();
	void AddFuncArg(tjs_int addr, tTJSFuncArgType type = fatNormal);
	void EndFuncArg();
	void AddOmitArg();

	void DoNestTopExitPatch(void);
	void DoContinuePatch(tNestData & nestdata);

	void ClearLocalVariable(tjs_int top, tjs_int bottom);

	void ClearFrame(tjs_int &frame, tjs_int base = -1);

	static void _output_func(const tjs_char *msg, const tjs_char *comment,
		tjs_int addr, const tjs_int32 *codestart, tjs_int size, void *data);
	static void _output_func_src(const tjs_char *msg, const tjs_char *name,
		tjs_int line, void *data);

public:
	void Commit();

	tjs_uint GetCodeSize() const { return CodeAreaSize; }
	tjs_uint GetDataSize() const { return DataAreaSize; }

	tjs_int CodePosToSrcPos(tjs_int codepos) const;
	tjs_int FindSrcLineStartCodePos(tjs_int codepos) const;

	ttstr GetPositionDescriptionString(tjs_int codepos) const;

	void AddLocalVariable(const tjs_char *name, tjs_int init=0);
	void InitLocalVariable(const tjs_char *name, tTJSExprNode *node);
	void InitLocalFunction(const tjs_char *name, tjs_int data);

	void CreateExprCode(tTJSExprNode *node);

	void EnterWhileCode(bool do_while);
	void CreateWhileExprCode(tTJSExprNode *node, bool do_while);
	void ExitWhileCode(bool do_while);

	void EnterIfCode();
	void CreateIfExprCode(tTJSExprNode *node);
	void ExitIfCode();
	void EnterElseCode();
	void ExitElseCode();

	void EnterForCode();
	void CreateForExprCode(tTJSExprNode *node);
	void SetForThirdExprCode(tTJSExprNode *node);
	void ExitForCode();

	void EnterSwitchCode(tTJSExprNode *node);
	void ExitSwitchCode();
	void ProcessCaseCode(tTJSExprNode *node);

	void EnterWithCode(tTJSExprNode *node);
	void ExitWithCode();

	void DoBreak();
	void DoContinue();

	void DoDebugger();

	void ReturnFromFunc(tTJSExprNode *node);

	void EnterTryCode();
	void EnterCatchCode(const tjs_char *name);
	void ExitTryCode();

	void ProcessThrowCode(tTJSExprNode *node);

	void CreateExtendsExprCode(tTJSExprNode *node, bool hold);
	void CreateExtendsExprProxyCode(tTJSExprNode *node);

	void EnterBlock();
	void ExitBlock();

	void GenerateFuncCallArgCode();

	void AddFunctionDeclArg(const tjs_char *varname, tTJSExprNode *init);
	void AddFunctionDeclArgCollapse(const tjs_char *varname);

	void SetPropertyDeclArg(const tjs_char *varname);

	const tjs_char * GetName() const ;

	void PushCurrentNode(tTJSExprNode *node);
	tTJSExprNode * GetCurrentNode();
	void PopCurrentNode();

	tTJSExprNode * MakeConstValNode(const tTJSVariant &val);

	tTJSExprNode * MakeNP0(tjs_int opecode);
	tTJSExprNode * MakeNP1(tjs_int opecode, tTJSExprNode * node1);
	tTJSExprNode * MakeNP2(tjs_int opecode, tTJSExprNode * node1, tTJSExprNode * node2);
	tTJSExprNode * MakeNP3(tjs_int opecode, tTJSExprNode * node1, tTJSExprNode * node2,
		tTJSExprNode * node3);

	//---------------------------------------------------------- disassembler
	// implemented in tjsDisassemble.cpp

	static tTJSString GetValueComment(const tTJSVariant &val);

	void Disassemble(
		void (*output_func)(const tjs_char *msg, const tjs_char *comment,
		tjs_int addr, const tjs_int32 *codestart, tjs_int size, void *data),
		void (*output_func_src)(const tjs_char *msg, const tjs_char *name,
			tjs_int line, void *data), void *data, tjs_int start = 0, tjs_int end = 0);
	void Disassemble(void (*output_func)(const tjs_char *msg, void* data), void *data,
		tjs_int start = 0, tjs_int end = 0);
	void Disassemble(tjs_int start = 0, tjs_int end = 0);
	void DisassembleSrcLine(tjs_int codepos);


	//--------------------------------------------------------- execute stuff
	// implemented in InterCodeExec.cpp
private:
	void DisplayExceptionGeneratedCode(tjs_int codepos, const tTJSVariant *ra);

	void ThrowScriptException(tTJSVariant &val, tTJSScriptBlock *block, tjs_int srcpos);

//	void ExecuteAsGlobal(tTJSVariant *result);
	void ExecuteAsFunction(iTJSDispatch2 *objthis, tTJSVariant **args,
		tjs_int numargs,tTJSVariant *result, tjs_int start_ip);
	tjs_int ExecuteCode(tTJSVariant *ra, tjs_int startip, tTJSVariant **args,
		tjs_int numargs, tTJSVariant *result);
	tjs_int ExecuteCodeInTryBlock(tTJSVariant *ra, tjs_int startip,
		tTJSVariant **args, tjs_int numargs, tTJSVariant *result,
		tjs_int catchip, tjs_int exobjreg);

	static void ContinuousClear(tTJSVariant *ra, const tjs_int32 *code);
	void GetPropertyDirect(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 flags);
	void SetPropertyDirect(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 flags);
	static void GetProperty(tTJSVariant *ra, const tjs_int32 *code);
	static void SetProperty(tTJSVariant *ra, const tjs_int32 *code);
	static void GetPropertyIndirect(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 flags);
	static void SetPropertyIndirect(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 flags);
	void OperatePropertyDirect(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 ope);
	static void OperatePropertyIndirect(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 ope);
	static void OperateProperty(tTJSVariant *ra, const tjs_int32 *code, tjs_uint32 ope);
	void OperatePropertyDirect0(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 ope);
	static void OperatePropertyIndirect0(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 ope);
	static void OperateProperty0(tTJSVariant *ra, const tjs_int32 *code, tjs_uint32 ope);
	void DeleteMemberDirect(tTJSVariant *ra, const tjs_int32 *code);
	static void DeleteMemberIndirect(tTJSVariant *ra, const tjs_int32 *code);
	void TypeOfMemberDirect(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 flags);
	static void TypeOfMemberIndirect(tTJSVariant *ra, const tjs_int32 *code,
		tjs_uint32 flags);
	tjs_int CallFunction(tTJSVariant *ra, const tjs_int32 *code,
		tTJSVariant **args,
		tjs_int numargs);
	tjs_int CallFunctionDirect(tTJSVariant *ra, const tjs_int32 *code,
		tTJSVariant **args, tjs_int numargs);
	tjs_int CallFunctionIndirect(tTJSVariant *ra, const tjs_int32 *code,
		tTJSVariant **args, tjs_int numargs);
	static void AddClassInstanceInfo(tTJSVariant *ra, const tjs_int32 *code);
	void ProcessStringFunction(const tjs_char *member, const ttstr & target,
		tTJSVariant **args, tjs_int numargs, tTJSVariant *result);
	void ProcessOctetFunction(const tjs_char *member, const tTJSVariantOctet * target,
		tTJSVariant **args, tjs_int numargs, tTJSVariant *result);
	static void TypeOf(tTJSVariant &val);
	void Eval(tTJSVariant &val, iTJSDispatch2 * objthis, bool resneed);
	static void CharacterCodeOf(tTJSVariant &val);
	static void CharacterCodeFrom(tTJSVariant &val);
	static void InstanceOf(const tTJSVariant &name, tTJSVariant &targ);

	void RegisterObjectMember(iTJSDispatch2 * dest);

	// for Byte code
	static inline void Add4ByteToVector( std::vector<tjs_uint8>* array, int value ) {
		array->push_back( (tjs_uint8)((value>>0)&0xff) );
		array->push_back( (tjs_uint8)((value>>8)&0xff) );
		array->push_back( (tjs_uint8)((value>>16)&0xff) );
		array->push_back( (tjs_uint8)((value>>24)&0xff) );
	}
	static inline void Add2ByteToVector( std::vector<tjs_uint8>* array, tjs_int16 value ) {
		array->push_back( (tjs_uint8)((value>>0)&0xff) );
		array->push_back( (tjs_uint8)((value>>8)&0xff) );
	}
public:
	// iTJSDispatch2 implementation
	tjs_error TJS_INTF_METHOD
	FuncCall(
		tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
			tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropGet(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropSet(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis);


	tjs_error TJS_INTF_METHOD
	CreateNew(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		iTJSDispatch2 **result, tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tjs_char *classname,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
		GetCount(tjs_int *result, const tjs_char *membername, tjs_uint32 *hint,
		 iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropSetByVS(tjs_uint32 flag, tTJSVariantString *mambername,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis)
	{
		return TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
		DeleteMember(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		 iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
		Invalidate(tjs_uint32 flag, const tjs_char *membername,  tjs_uint32 *hint,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
		IsValid(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		 iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
		Operation(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		 tTJSVariant *result,
			const tTJSVariant *param,	iTJSDispatch2 *objthis);

	// for Byte code
	void SetCodeObject( tTJSInterCodeContext* parent, tTJSInterCodeContext* setter, tTJSInterCodeContext* getter, tTJSInterCodeContext* superclass ) {
		Parent = parent;
		PropSetter = setter;
		if( setter ) setter->AddRef();
		PropGetter = getter;
		if( getter ) getter->AddRef();
		SuperClassGetter = superclass;
#ifdef ENABLE_DEBUGGER
		if (Parent) Parent->AddRef();
#endif	// ENABLE_DEBUGGER
	}
	
	tTJSInterCodeContext( tTJSScriptBlock *block, const tjs_char *name, tTJSContextType type,
		tjs_int32* code, tjs_int codeSize, tTJSVariant* data, tjs_int dataSize,
		tjs_int varcount, tjs_int verrescount, tjs_int maxframe, tjs_int argcount, tjs_int arraybase, tjs_int colbase, bool srcsorted,
		tSourcePos* srcPos, tjs_int srcPosSize, std::vector<tjs_int>& superpointer );

	std::vector<tjs_uint8>* ExportByteCode( bool outputdebug, tTJSScriptBlock *block, class tjsConstArrayData& constarray );

protected:
	void TJSVariantArrayStackAddRef();
	void TJSVariantArrayStackRelease();

	class tTJSVariantArrayStack *TJSVariantArrayStack = nullptr;
};
//---------------------------------------------------------------------------
}
#endif
