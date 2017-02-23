//---------------------------------------------------------------------------
/*
	TJS2 Script Engine( Byte Code )
	Copyright (c), Takenori Imoto

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjs.h"
#include "tjsScriptBlock.h"
#include "tjsByteCodeLoader.h"
#include "tjsGlobalStringMap.h"

namespace TJS
{

bool tTJSByteCodeLoader::IsTJS2ByteCode( const tjs_uint8* buff )
{
	// TJS2
	int tag = read4byte( buff );
	if( tag != FILE_TAG_LE ) return false;
	// 100'\0'
	int ver = read4byte( &(buff[4]) );
	if( ver != VER_TAG_LE ) return false;
	return true;
}
tTJSScriptBlock* tTJSByteCodeLoader::ReadByteCode( tTJS* owner, const tjs_char* name, const tjs_uint8* buf, size_t size ) {
	ReadBuffer = buf;
	ReadIndex = 0;
	ReadSize = (tjs_uint32)size;

	const tjs_uint8* databuff = ReadBuffer;

	// TJS2
	int tag = read4byte( databuff );
	if( tag != FILE_TAG_LE ) return NULL;
	// 100'\0'
	int ver = read4byte( &(databuff[4]) );
	if( ver != VER_TAG_LE ) return NULL;

	int filesize = read4byte( &(databuff[8]) );
	if( filesize != size ) return NULL;

	//// DATA
	tag = read4byte( &(databuff[12]) );
	if( tag != DATA_TAG_LE ) return NULL;
	size = read4byte( &(databuff[16]) );
	ReadDataArea( databuff, 20, size );

	int offset = (int)(12 + size); // これがデータエリア後の位置
	// OBJS
	tag = read4byte( &(databuff[offset]) );
	offset+=4;
	if( tag != OBJ_TAG_LE ) return NULL;
	//int objsize = ibuff.get();
	int objsize = read4byte( &(databuff[offset]) );
	offset+=4;
	tTJSScriptBlock* block = new tTJSScriptBlock(owner, name, 0 );
	ReadObjects( block, databuff, offset, objsize );
	return block;
}

void tTJSByteCodeLoader::ReadDataArea( const tjs_uint8* buff, int offset, size_t size ) {
	int count = read4byte( &(buff[offset]) );
	offset += 4;
	if( count > 0 ) {
		ByteArray.set( (tjs_int8*)&buff[offset], count );
		int stride = ( count + 3 ) >> 2;
		offset += stride << 2;
	}
	count = read4byte( &(buff[offset]) );
	offset += 4;
	if( count > 0 ) {	// load short
		ShortArray.clear();
		ShortArray.reserve( count );
		for( int i = 0; i < count; i++ ) {
			ShortArray.push_back( read2byte( &(buff[offset]) ) );
			offset += 2;
		}
		offset += (count & 1) << 1;
	}
	count = read4byte( &(buff[offset]) );
	offset += 4;
	if( count > 0 ) {
		LongArray.clear();
		LongArray.reserve( count );
		for( int i = 0; i < count; i++ ) {
			LongArray.push_back( read4byte( &(buff[offset]) ) );
			offset += 4;
		}
	}
	count = read4byte( &(buff[offset]) );
	offset += 4;
	if( count > 0 ) {	// load long
		LongLongArray.clear();
		LongLongArray.reserve( count );
		for( int i = 0; i < count; i++ ) {
			LongLongArray.push_back( read8byte( &(buff[offset]) ) );
			offset += 8;
		}
	}
	count = read4byte( &(buff[offset]) );
	offset += 4;
	if( count > 0 ) {	// load double
		DoubleArray.clear();
		DoubleArray.reserve( count );
		for( int i = 0; i < count; i++ ) {
			tjs_uint64 tmp = read8byte( &(buff[offset]) );
			DoubleArray.push_back( *(double*)&tmp );
			offset += 8;
		}
	}
	count = read4byte( &(buff[offset]) );
	offset += 4;
	if( count > 0 ) {
		StringArray.clear();
		StringArray.reserve( count );
		for( int i = 0; i < count; i++ ) {
			int len = read4byte( &(buff[offset]) );
			offset += 4;
			std::vector<tjs_uint16> ch(len+1);
			ch[len] = 0;
			for( int j = 0; j < len; j++ ) {
				ch[j] = read2byte( &(buff[offset]) );
				offset += 2;
			}
			StringArray.push_back( TJSMapGlobalStringMap( (const tjs_char *)&(ch[0]) ) );
			offset += (len & 1) << 1;
		}
	}
	count = read4byte( &(buff[offset]) );
	offset += 4;
	if( count > 0 ) {
		OctetArray.clear();
		OctetArray.reserve( count );
		for( int i = 0; i < count; i++ ) {
			int len = read4byte( &(buff[offset]) );
			offset += 4;
			tTJSVariantOctet* octet = new tTJSVariantOctet( &(buff[offset]), len ); // データはコピーされる
			OctetArray.push_back( octet );
			offset += (( len + 3 ) >> 2) << 2;
		}
	}
}

void tTJSByteCodeLoader::ReadObjects( tTJSScriptBlock* block, const tjs_uint8* buff, int offset, int size ) {
	int toplevel = read4byte( &(buff[offset]) );
	offset += 4;
	int objcount = read4byte( &(buff[offset]) );
	offset += 4;

	//tTJSInterCodeContext** objs = new tTJSInterCodeContext*[objcount];
	std::vector<tTJSInterCodeContext*> objs(objcount);
	std::vector<VariantRepalace> work;
	std::vector<int> parent(objcount);
	std::vector<int> propSetter(objcount);
	std::vector<int> propGetter(objcount);
	std::vector<int> superClassGetter(objcount);
	std::vector<std::vector<int> > properties(objcount);
	for( int o = 0; o < objcount; o++ ) {
		int tag = read4byte( &(buff[offset]) );
		offset += 4;
		if( tag != FILE_TAG_LE ) {
			//throw new TJSException(Error.ByteCodeBroken);
			TJS_eTJSScriptError( TJSByteCodeBroken, block, 0 );
		}
		//int objsize = read4byte( &(buff[offset]) );
		offset += 4;
		parent[o]  = read4byte( &(buff[offset]) );
		offset += 4;
		int name  = read4byte( &(buff[offset]) );
		offset += 4;
		int contextType = read4byte( &(buff[offset]) );
		offset += 4;
		int maxVariableCount = read4byte( &(buff[offset]) );
		offset += 4;
		int variableReserveCount = read4byte( &(buff[offset]) );
		offset += 4;
		int maxFrameCount = read4byte( &(buff[offset]) );
		offset += 4;
		int funcDeclArgCount = read4byte( &(buff[offset]) );
		offset += 4;
		int funcDeclUnnamedArgArrayBase = read4byte( &(buff[offset]) );
		offset += 4;
		int funcDeclCollapseBase = read4byte( &(buff[offset]) );
		offset += 4;
		propSetter[o] = read4byte( &(buff[offset]) );
		offset += 4;
		propGetter[o] = read4byte( &(buff[offset]) );
		offset += 4;
		superClassGetter[o] = read4byte( &(buff[offset]) );
		offset += 4;

		int count = read4byte( &(buff[offset]) );
		offset += 4;

		// デバッグ用のソース位置を読み込む
		tTJSInterCodeContext::tSourcePos* srcPos = NULL;
		tjs_int srcPosArraySize = 0;
		if( count > 0 ) {
			srcPos = new tTJSInterCodeContext::tSourcePos[count];
			srcPosArraySize = count;
			for( int i = 0; i < count; i++ ) {
				srcPos[i].CodePos = read4byte( &(buff[offset]) );
				offset += 4;
			}
			for( int i = 0; i < count; i++ ) {
				srcPos[i].SourcePos = read4byte( &(buff[offset]) );
				offset += 4;
			}
		} else {
			offset += count << 3;
		}

		count = read4byte( &(buff[offset]) );
		const tjs_int codeSize = count;
		offset += 4;
		tjs_int32* code = (tjs_int32*)TJS_malloc(count * sizeof(tjs_int32));
		for( int i = 0; i < count; i++ ) {
			tjs_int16 c = (tjs_int16)read2byte( &(buff[offset]) );
			code[i] = c;
			offset += 2;
		}
		TranslateCodeAddress( block, code, codeSize );
		offset += (count & 1) << 1;

		count = read4byte( &(buff[offset]) );
		offset += 4;
		int vcount = count<<1;
		std::vector<short> data(vcount);
		for( int i = 0; i < vcount; i++ ) {
			data[i] = read2byte( &(buff[offset]) );
			offset += 2;
		}

		tTJSVariant* vdata = new tTJSVariant[count];
		const tjs_int datacount = count;
		for( int i = 0; i < datacount; i++ ) {
			int pos = i << 1;
			int type = data[pos];
			int index = data[pos+1];
			switch( type ) {
			case TYPE_VOID:
				vdata[i].Clear();
				break;
			case TYPE_OBJECT:
				vdata[i] = (iTJSDispatch2*)NULL;
				break;
			case TYPE_INTER_OBJECT:
				work.push_back( VariantRepalace( &(vdata[i]), index ) );
				break;
			case TYPE_INTER_GENERATOR:
				work.push_back( VariantRepalace( &(vdata[i]), index ) );
				break;
			case TYPE_STRING:
				vdata[i] = StringArray[index].c_str(); // tTJSString
				break;
			case TYPE_OCTET:
				vdata[i] = OctetArray[index]; // tTJSVariantOctet
				break;
			case TYPE_REAL:
				vdata[i] = (tjs_real)DoubleArray[index];
				break;
			case TYPE_BYTE:
				vdata[i] = (tjs_int)ByteArray[index];
				break;
			case TYPE_SHORT:
				vdata[i] = (tjs_int)ShortArray[index];
				break;
			case TYPE_INTEGER:
				vdata[i] = (tjs_int)LongArray[index];
				break;
			case TYPE_LONG:
				vdata[i] = (tjs_int64)LongLongArray[index];
				break;
			case TYPE_UNKNOWN:
			default:
				vdata[i].Clear();
				break;
			}
		}
		count = read4byte( &(buff[offset]) );
		offset += 4;
		//int* scgetterps = new int[count];
		std::vector<tjs_int> scgetterps(count);
		for( int i = 0; i < count; i++ ) {
			scgetterps[i] = read4byte( &(buff[offset]) );
			offset += 4;
		}
		// properties
		count = read4byte( &(buff[offset]) );
		offset += 4;
		if( count > 0 ) {
			int pcount = count << 1;
			std::vector<int>& props = properties[o];
			props.resize(pcount);
			for( int i = 0; i < pcount; i++ ) {
				props[i] = read4byte( &(buff[offset]) );
				offset += 4;
			}
		}

		tTJSInterCodeContext* obj = new tTJSInterCodeContext( block, StringArray[name].c_str(), (tTJSContextType)contextType,
			code, codeSize, vdata, datacount, maxVariableCount, variableReserveCount, maxFrameCount, funcDeclArgCount, funcDeclUnnamedArgArrayBase,
			funcDeclCollapseBase, true, srcPos, srcPosArraySize, scgetterps );
		objs[o] = obj;
	}
	tTJSVariant val;
	for( int o = 0; o < objcount; o++ ) {
		tTJSInterCodeContext* parentObj = NULL;
		tTJSInterCodeContext* propSetterObj = NULL;
		tTJSInterCodeContext* propGetterObj = NULL;
		tTJSInterCodeContext* superClassGetterObj = NULL;

		if( parent[o] >= 0 ) {
			parentObj = objs[parent[o]];
		}
		if( propSetter[o] >= 0 ) {
			propSetterObj = objs[propSetter[o]];
		}
		if( propGetter[o] >= 0 ) {
			propGetterObj = objs[propGetter[o]];
		}
		if( superClassGetter[o] >= 0 ) {
			superClassGetterObj = objs[superClassGetter[o]];
		}
		objs[o]->SetCodeObject(parentObj, propSetterObj, propGetterObj, superClassGetterObj );

		if( properties[o].size() > 0 ) {
			tTJSInterCodeContext* obj = parentObj;
			std::vector<int>& prop = properties[o];
			int length = (int)(prop.size() >> 1);
			for( int i = 0; i < length; i++ ) {
				int pos = i << 1;
				int pname = prop[pos];
				int pobj = prop[pos+1];
				// register members to the parent object
				val = objs[pobj];
				obj->PropSet( TJS_MEMBERENSURE|TJS_IGNOREPROP, StringArray[pname].c_str(), NULL, &val, obj );
			}
		}
	}
	int count = (int)work.size();
	for( int i = 0; i < count; i++ ) {
		VariantRepalace& w = work[i];
		(*w.Work) = objs[w.Index];
	}
	work.clear();
	tTJSInterCodeContext* top = NULL;
	if( toplevel >= 0 ) {
		top = objs[toplevel];
	}
	block->SetObjects( top, objs, objcount );
	//delete[] objs;
}
#define TJS_OFFSET_VM_REG_ADDR( x ) ( (x) = TJS_TO_VM_REG_ADDR(x) )
#define TJS_OFFSET_VM_CODE_ADDR( x ) ( (x) = TJS_TO_VM_CODE_ADDR(x) )
/**
 * バイトコード中のアドレスは配列のインデックスを指しているので、それをアドレスに変換する
 */
void tTJSByteCodeLoader::TranslateCodeAddress( tTJSScriptBlock* block, tjs_int32* code, const tjs_int32 codeSize )
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
		TJS_eTJSScriptError( TJSByteCodeBroken, block, 0 );
	}
}

} // namespace
