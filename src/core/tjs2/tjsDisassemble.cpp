//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// VM code disassembler
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsInterCodeGen.h"
#include "tjsScriptBlock.h"
#include "tjsUtils.h"

//---------------------------------------------------------------------------
namespace TJS  // following is in the namespace
{
//---------------------------------------------------------------------------
tTJSString tTJSInterCodeContext::GetValueComment(const tTJSVariant &val)
{
	// make val a human readable string and return it
	return TJSVariantToReadableString(val);
}
//---------------------------------------------------------------------------
static tjs_uint32 dummy = (tjs_uint32)-1; // for data alignment
void tTJSInterCodeContext::Disassemble(
	void (*output_func)(const tjs_char *msg, const tjs_char *comment, tjs_int addr,
	const tjs_int32 *codestart, tjs_int size, void *data),
	void (*output_func_src)(const tjs_char *msg, const tjs_char *name, tjs_int line,
	void *data), void *data, tjs_int start, tjs_int end)
{
	// dis-assemble the intermediate code.
	// "output_func" points a line output function.

//	tTJSVariantString * s;

	tTJSString msg;
	tTJSString com;

	tjs_int prevline = -1;
	tjs_int curline = -1;

	if(end <= 0) end = CodeAreaSize;
	if(end > CodeAreaSize) end = CodeAreaSize;

	for(tjs_int i = start; i < end; )
	{
		msg.Clear();
		com.Clear();
		tjs_int size;
		tjs_int srcpos = CodePosToSrcPos(i);
		tjs_int line = Block->SrcPosToLine(srcpos);

		// output source lines as comment
		if(curline == -1 || curline <= line)
		{
			if(curline == -1) curline = line;
			tjs_int nl = line - curline;
			while(curline <= line)
			{
				if(nl<3 || (nl >= 3 && line-curline <= 2))
				{
					tjs_int len;
					const tjs_char *src = Block->GetLine(curline, &len);
					tjs_char * buf = new tjs_char[len + 1];
					TJS_strcpy_maxlen(buf, src, len);
					try
					{
						output_func_src(buf, TJS_W(""), curline, data);
					}
					catch(...)
					{
						delete [] buf;
						throw;
					}
					delete [] buf;
					curline++;
				}
				else
				{
					curline = line - 2;
				}
			}
		}
		else if(prevline != line)
		{
			tjs_int len;
			const tjs_char *src = Block->GetLine(line, &len);
			tjs_char * buf = new tjs_char[len + 1];
			TJS_strcpy_maxlen(buf, src, len);
			try
			{
				output_func_src((const tjs_char*)buf, TJS_W(""), line, data);
			}
			catch(...)
			{
				delete [] buf;
				throw;
			}
			delete [] buf;
		}

		prevline = line;

		// decode each instructions
		switch(CodeArea[i])
		{
		case VM_NOP:
			msg.printf(TJS_W("nop"));
			size = 1;
			break;

		case VM_NF:
			msg.printf(TJS_W("nf"));
			size = 1;
			break;

		case VM_CONST:
			msg.printf(TJS_W("const %%%d, *%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]));
			if(DataArea)
			{
				com.printf(TJS_W("*%d = %ls"), TJS_FROM_VM_REG_ADDR(CodeArea[i+2]),
					GetValueComment(TJS_GET_VM_REG(DataArea, CodeArea[i+2])).c_str());
			}
			size = 3;
			break;


#define OP2_DISASM(c, x) \
	case c: \
		msg.printf(TJS_W(x) TJS_W(" %%%d, %%%d"), TJS_FROM_VM_REG_ADDR(CodeArea[i+1]), \
										TJS_FROM_VM_REG_ADDR(CodeArea[i+2])); \
		size = 3; \
		break
		// instructions that
		// 1. have two operands that represent registers.
		// 2. do not have property access variants.
		OP2_DISASM(VM_CP,		"cp");
		OP2_DISASM(VM_CEQ,		"ceq");
		OP2_DISASM(VM_CDEQ,		"cdeq");
		OP2_DISASM(VM_CLT,		"clt");
		OP2_DISASM(VM_CGT,		"cgt");
		OP2_DISASM(VM_CHKINS,	"chkins");
#undef OP2_DISASM


#define OP2_DISASM(c, x) \
	case c: \
		msg.printf(TJS_W(x) TJS_W(" %%%d, %%%d"), TJS_FROM_VM_REG_ADDR(CodeArea[i+1]), \
									TJS_FROM_VM_REG_ADDR(CodeArea[i+2])); \
		size = 3; \
		break; \
	case c+1: \
		msg.printf(TJS_W(x) TJS_W("pd") TJS_W(" %%%d, %%%d.*%d, %%%d"), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+1]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+2]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+3]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+4])); \
		if(DataArea) \
		{ \
			com.printf(TJS_W("*%d = %ls"), TJS_FROM_VM_REG_ADDR(CodeArea[i+3]), \
				GetValueComment(TJS_GET_VM_REG(DataArea, CodeArea[i+3])).c_str()); \
		} \
		size = 5; \
		break; \
	case c+2: \
		msg.printf(TJS_W(x) TJS_W("pi") TJS_W(" %%%d, %%%d.%%%d, %%%d"), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+1]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+2]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+3]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+4])); \
		size = 5; \
		break; \
	case c+3: \
		msg.printf(TJS_W(x) TJS_W("p") TJS_W(" %%%d, %%%d, %%%d"), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+1]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+2]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+3])); \
		size = 4; \
		break
		// instructions that
		// 1. have two operands that represent registers.
		// 2. have property access variants
		OP2_DISASM(VM_LOR,		"lor");
		OP2_DISASM(VM_LAND,		"land");
		OP2_DISASM(VM_BOR,		"bor");
		OP2_DISASM(VM_BXOR,		"bxor");
		OP2_DISASM(VM_BAND,		"band");
		OP2_DISASM(VM_SAR,		"sar");
		OP2_DISASM(VM_SAL,		"sal");
		OP2_DISASM(VM_SR,		"sr");
		OP2_DISASM(VM_ADD,		"add");
		OP2_DISASM(VM_SUB,		"sub");
		OP2_DISASM(VM_MOD,		"mod");
		OP2_DISASM(VM_DIV,		"div");
		OP2_DISASM(VM_IDIV,		"idiv");
		OP2_DISASM(VM_MUL,		"mul");
#undef OP2_DISASM

#define OP1_DISASM(x) \
	msg.printf(TJS_W(x) TJS_W(" %%%d"), TJS_FROM_VM_REG_ADDR(CodeArea[i+1])); \
	size = 2
		// instructions that have one operand which represent a register,
		// except for inc, dec
		case VM_TT:			OP1_DISASM("tt");		break;
		case VM_TF:			OP1_DISASM("tf");		break;
		case VM_SETF:		OP1_DISASM("setf");		break;
		case VM_SETNF:		OP1_DISASM("setnf");	break;
		case VM_LNOT:		OP1_DISASM("lnot");		break;
		case VM_BNOT:		OP1_DISASM("bnot");		break;
		case VM_ASC:		OP1_DISASM("asc");		break;
		case VM_CHR:		OP1_DISASM("chr");		break;
		case VM_NUM:		OP1_DISASM("num");		break;
		case VM_CHS:		OP1_DISASM("chs");		break;
		case VM_CL:			OP1_DISASM("cl");		break;
		case VM_INV:		OP1_DISASM("inv");		break;
		case VM_CHKINV:		OP1_DISASM("chkinv");	break;
		case VM_TYPEOF:		OP1_DISASM("typeof");	break;
		case VM_EVAL:		OP1_DISASM("eval");		break;
		case VM_EEXP:		OP1_DISASM("eexp");		break;
		case VM_INT:		OP1_DISASM("int");		break;
		case VM_REAL:		OP1_DISASM("real");		break;
		case VM_STR:		OP1_DISASM("str");		break;
		case VM_OCTET:		OP1_DISASM("octet");	break;
#undef OP1_DISASM

		case VM_CCL:
			msg.printf(TJS_W("ccl %%%d-%%%d"), TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]) + CodeArea[i+2] -1);
			size = 3;
			break;

#define OP1_DISASM(c, x) \
	case c: \
		msg.printf(TJS_W(x) TJS_W(" %%%d"), TJS_FROM_VM_REG_ADDR(CodeArea[i+1])); \
		size = 2; \
		break; \
	case c+1: \
		msg.printf(TJS_W(x) TJS_W("pd") TJS_W(" %%%d, %%%d.*%d"), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+1]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+2]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+3])); \
		if(DataArea) \
		{ \
			com.printf(TJS_W("*%d = %ls"), \
				TJS_FROM_VM_REG_ADDR(CodeArea[i+3]), \
				GetValueComment(TJS_GET_VM_REG(DataArea, CodeArea[i+3])).c_str()); \
		} \
		size = 4; \
		break; \
	case c+2: \
		msg.printf(TJS_W(x) TJS_W("pi") TJS_W(" %%%d, %%%d.%%%d"), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+1]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+2]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+3])); \
		size = 4; \
		break; \
	case c+3: \
		msg.printf(TJS_W(x) TJS_W("p") TJS_W(" %%%d, %%%d"), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+1]), \
			TJS_FROM_VM_REG_ADDR(CodeArea[i+2])); \
		size = 3; \
		break

		// inc and dec
		OP1_DISASM(VM_INC,	"inc");
		OP1_DISASM(VM_DEC,	"dec");
#undef OP1_DISASM



#define OP1A_DISASM(x) \
	msg.printf(TJS_W(x) TJS_W(" %09d"), TJS_FROM_VM_CODE_ADDR(CodeArea[i+1]) + i); \
	size = 2
		// instructions that have one operand which represents code area
		case VM_JF:		OP1A_DISASM("jf");		break;
		case VM_JNF:	OP1A_DISASM("jnf");		break;
		case VM_JMP:	OP1A_DISASM("jmp");		break;
#undef OP1A_DISASM

		case VM_CALL:
		case VM_CALLD:
		case VM_CALLI:
		case VM_NEW:
		  {
			// function call variants

			msg.printf(
				CodeArea[i] == VM_CALL  ?TJS_W("call %%%d, %%%d("):
				CodeArea[i] == VM_CALLD ?TJS_W("calld %%%d, %%%d.*%d("):
				CodeArea[i] == VM_CALLI ?TJS_W("calli %%%d, %%%d.%%%d("):
										 TJS_W("new %%%d, %%%d("),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+3]));
			tjs_int st; // start of arguments
			if(CodeArea[i] == VM_CALLD || CodeArea[i] == VM_CALLI)
				st = 5;
			else
				st = 4;
			tjs_int num = CodeArea[i+st-1];     // st-1 = argument count
			bool first = true;
			tjs_char buf[256];
			tjs_int c = 0;
			if(num == -1)
			{
				// omit arg
				size = st;
				msg += TJS_W("...");
			}
			else if(num == -2)
			{
				// expand arg
				st ++;
				num = CodeArea[i+st-1];
				size = st + num * 2;
				for(tjs_int j = 0; j < num; j++)
				{
					if(!first) msg += TJS_W(", ");
					first = false;
					switch(CodeArea[i+st+j*2])
					{
					case fatNormal:
						TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), TJS_W("%%%d"),
							TJS_FROM_VM_REG_ADDR(CodeArea[i+st+j*2+1]));
						break;
					case fatExpand:
						TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), TJS_W("%%%d*"),
							TJS_FROM_VM_REG_ADDR(CodeArea[i+st+j*2+1]));
						break;
					case fatUnnamedExpand:
						TJS_strcpy(buf, TJS_W("*"));
						break;
					}
					msg += buf;
				}
			}
			else
			{
				// normal operation
				size = st + num;
				while(num--)
				{
					if(!first) msg += TJS_W(", ");
					first = false;
					TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), TJS_W("%%%d"),
						TJS_FROM_VM_REG_ADDR(CodeArea[i+c+st]));
					c++;
					msg += buf;
				}
			}

			msg += TJS_W(")");
			if(DataArea && CodeArea[i] == VM_CALLD)
			{
				com.printf(TJS_W("*%d = %ls"), TJS_FROM_VM_REG_ADDR(CodeArea[i+3]),
					GetValueComment(TJS_GET_VM_REG(DataArea, CodeArea[i+3])).c_str());
			}

			break;
		  }

		case VM_GPD:
		case VM_GPDS:
			// property get direct
			msg.printf(
				CodeArea[i] == VM_GPD?TJS_W("gpd %%%d, %%%d.*%d"):
									  TJS_W("gpds %%%d, %%%d.*%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+3]));
			if(DataArea)
			{
				com.printf(TJS_W("*%d = %ls"), TJS_FROM_VM_REG_ADDR(CodeArea[i+3]),
					GetValueComment(TJS_GET_VM_REG(DataArea, CodeArea[i+3])).c_str());
			}
			size = 4;
			break;


		case VM_SPD:
		case VM_SPDE:
		case VM_SPDEH:
		case VM_SPDS:
			// property set direct
			msg.printf(
				CodeArea[i] == VM_SPD ? TJS_W("spd %%%d.*%d, %%%d"):
				CodeArea[i] == VM_SPDE? TJS_W("spde %%%d.*%d, %%%d"):
				CodeArea[i] == VM_SPDEH?TJS_W("spdeh %%%d.*%d, %%%d"):
										TJS_W("spds %%%d.*%d, %%%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+3]));
			if(DataArea)
			{
				com.printf(TJS_W("*%d = %ls"), TJS_FROM_VM_REG_ADDR(CodeArea[i+2]),
					GetValueComment(TJS_GET_VM_REG(DataArea, CodeArea[i+2])).c_str());
			}

			size = 4;
			break;


		case VM_GPI:
		case VM_GPIS:
			// property get indirect
			msg.printf(
				CodeArea[i] == VM_GPI ?  TJS_W("gpi %%%d, %%%d.%%%d"):
										 TJS_W("gpis %%%d, %%%d.%%%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+3]));
			size = 4;
			break;


		case VM_SPI:
		case VM_SPIE:
		case VM_SPIS:
			// property set indirect
			msg.printf(
				CodeArea[i] == VM_SPI  ?TJS_W("spi %%%d.%%%d, %%%d"):
				CodeArea[i] == VM_SPIE ?TJS_W("spie %%%d.%%%d, %%%d"):
										TJS_W("spis %%%d.%%%d, %%%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+3]));
			size = 4;
			break;


		case VM_SETP:
			// property set
			msg.printf(
				TJS_W("setp %%%d, %%%d"),
					TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
					TJS_FROM_VM_REG_ADDR(CodeArea[i+2]));
			size = 3;
			break;

		case VM_GETP:
			// property get
			msg.printf(
				TJS_W("getp %%%d, %%%d"),
					TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
					TJS_FROM_VM_REG_ADDR(CodeArea[i+2]));
			size = 3;
			break;


		case VM_DELD:
		case VM_TYPEOFD:
			// member delete direct / typeof direct
			msg.printf(
				CodeArea[i] == VM_DELD   ?TJS_W("deld %%%d, %%%d.*%d"):
										  TJS_W("typeofd %%%d, %%%d.*%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+3]));
			if(DataArea)
			{
				com.printf(TJS_W("*%d = %ls"), TJS_FROM_VM_REG_ADDR(CodeArea[i+3]),
					GetValueComment(TJS_GET_VM_REG(DataArea, CodeArea[i+3])).c_str());
			}
			size = 4;
			break;

		case VM_DELI:
		case VM_TYPEOFI:
			// member delete indirect / typeof indirect
			msg.printf(
				CodeArea[i] == VM_DELI   ?TJS_W("deli %%%d, %%%d.%%%d"):
										  TJS_W("typeofi %%%d, %%%d.%%%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+3]));
			size = 4;
			break;

		case VM_SRV:
			// set return value
			msg.printf(TJS_W("srv %%%d"), TJS_FROM_VM_REG_ADDR(CodeArea[i+1]));
			size = 2;
			break;

		case VM_RET:
			// return
			msg.printf(TJS_W("ret"));
			size = 1;
			break;

		case VM_ENTRY:
			// enter try-protected block
			msg.printf(TJS_W("entry %09d, %%%d"),
				TJS_FROM_VM_CODE_ADDR(CodeArea[i+1]) + i,
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]));
			size = 3;
			break;


		case VM_EXTRY:
			// exit from try-protected block
			msg.printf(TJS_W("extry"));
			size = 1;
			break;

		case VM_THROW:
			msg.printf(TJS_W("throw %%%d"), TJS_FROM_VM_REG_ADDR(CodeArea[i+1]));
			size = 2;
			break;

		case VM_CHGTHIS:
			msg.printf(TJS_W("chgthis %%%d, %%%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]));
			size = 3;
			break;

		case VM_GLOBAL:
			msg.printf(TJS_W("global %%%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]));
			size = 2;
			break;

		case VM_ADDCI:
			msg.printf(TJS_W("addci %%%d, %%%d"),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+1]),
				TJS_FROM_VM_REG_ADDR(CodeArea[i+2]));
			size = 3;
			break;

		case VM_REGMEMBER:
			msg.printf(TJS_W("regmember"));
			size = 1;
			break;

		case VM_DEBUGGER:
			msg.printf(TJS_W("debugger"));
			size = 1;
			break;

		default:
			msg.printf(TJS_W("unknown instruction %d"), CodeArea[i]);
			size = 1;
			break;
		} /* switch */

		output_func(msg.c_str(), com.c_str(), i,
			CodeArea + i, size, data);  // call the callback

		i+=size;
	}

}
//---------------------------------------------------------------------------
struct of_data
{
	void (*func)(const tjs_char*, void *);
	void *funcdata;
};

void tTJSInterCodeContext::_output_func(const tjs_char *msg,
	const tjs_char *comment, tjs_int addr, const tjs_int32 *codestart,
		tjs_int size, void *data)

{
	tjs_int buflen = (tjs_int)(TJS_strlen(msg) + TJS_strlen(comment) + 20);
	tjs_char *buf = new tjs_char[buflen];

	TJS_snprintf(buf, buflen, TJS_W("%08d %ls"), addr, msg);
	if(comment[0])
	{
		TJS_strcat(buf, TJS_W("\t// "));
		TJS_strcat(buf, comment);
	}

	try
	{
		of_data *dat = (of_data *)(data);
		dat->func(buf, dat->funcdata);
	}
	catch(...)
	{
		delete [] buf;
		throw;
	}

	delete [] buf;
}
void tTJSInterCodeContext::_output_func_src(const tjs_char *msg,
	const tjs_char *name, tjs_int line, void *data)
{
	tjs_int buflen = (tjs_int)(TJS_strlen(msg) + TJS_strlen(name) + 20);
	tjs_char *buf = new tjs_char[buflen];
	if(line >= 0)
		TJS_snprintf(buf, buflen, TJS_W("#%ls(%d) %ls"), name, line+1, msg);
	else
		TJS_snprintf(buf, buflen, TJS_W("#%ls %ls"), name, msg);
	try
	{
		of_data *dat = (of_data *)(data);
		dat->func(buf, dat->funcdata);
	}
	catch(...)
	{
		delete [] buf;
		throw;
	}

	delete [] buf;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::Disassemble(
	void (*output_func)(const tjs_char *msg,void *data), void *data, tjs_int start,
		tjs_int end)

{
	// dis-assemble
	of_data dat;
	dat.func =  output_func;
	dat.funcdata = data;
	Disassemble(_output_func, _output_func_src, (void*)&dat, start, end);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::Disassemble(tjs_int start, tjs_int end)
{
	Disassemble(tTJSScriptBlock::GetConsoleOutput(), Block, start, end);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::DisassembleSrcLine(tjs_int codepos)
{
	tjs_int start = FindSrcLineStartCodePos(codepos);
	Disassemble(start, codepos + 1);
}
//---------------------------------------------------------------------------
} // namespace TJS


